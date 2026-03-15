/*
 * Minimal WebSocket server that consumes text commands from the browser
 * and returns authoritative state as JSON.
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(loop_ws, LOG_LEVEL_INF);

#include <zephyr/net/socket.h>
#include <zephyr/net/websocket.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/http/server.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/* --- Service definition (listen on 0.0.0.0:80) --- */
static uint16_t http_service_port = 80;
/* name "my_service" must match the linker section we added in sections-rom.ld */
HTTP_SERVICE_DEFINE(my_service, "0.0.0.0", &http_service_port,
                    /* max IPv4 + IPv6 listeners */ 1,
                    /* max accepted clients per listener */ 10,
                    /* no per-service fallback resource */ NULL,
                    /* no per-service config */ NULL,
                    /* default socket create/config */ NULL);

/* --- Application state --- */

enum loop_mode {
    MODE_NONE = 0,
    MODE_LOOP_A,
    MODE_LOOP_B,
    MODE_CROSSED,
    MODE_VERTICAL,
};

struct app_state {
    enum loop_mode mode;    /* one of A/B/CROSSED/VERTICAL */
    int angle;              /* 0, -45, or +45 */
};

static struct app_state g_state = {
    .mode = MODE_LOOP_A,
    .angle = 0,
};

/* --- Connected clients (very small fixed-size list for demo) --- */

#define MAX_CLIENTS 4
static int g_clients[MAX_CLIENTS] = { -1, -1, -1, -1 };
static struct k_mutex g_clients_lock;

static void clients_add(int ws)
{
    k_mutex_lock(&g_clients_lock, K_FOREVER);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i] < 0) { g_clients[i] = ws; break; }
    }
    k_mutex_unlock(&g_clients_lock);
}

static void clients_remove(int ws)
{
    k_mutex_lock(&g_clients_lock, K_FOREVER);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_clients[i] == ws) { g_clients[i] = -1; break; }
    }
    k_mutex_unlock(&g_clients_lock);
}

/* --- Helpers to convert to/from strings --- */

static const char* mode_to_cmd(enum loop_mode m)
{
    switch (m) {
    case MODE_LOOP_A:   return "LoopA";
    case MODE_LOOP_B:   return "LoopB";
    case MODE_CROSSED:  return "Crossed";
    case MODE_VERTICAL: return "Vertical";
    default:            return "";
    }
}

static enum loop_mode cmd_to_mode(const char *s)
{
    if (strcmp(s, "LoopA") == 0) return MODE_LOOP_A;
    if (strcmp(s, "LoopB") == 0) return MODE_LOOP_B;
    if (strcmp(s, "Crossed") == 0) return MODE_CROSSED;
    if (strcmp(s, "Vertical") == 0) return MODE_VERTICAL;
    return MODE_NONE;
}

/* --- JSON state builder --- */
/* Example: {"type":"state","active":["LoopA","+45"]} */
static int build_state_json(char *out, size_t out_sz)
{
    const char *mode_str = mode_to_cmd(g_state.mode);
    char angle_buf[5] = {0};
    if (g_state.angle > 0) {
        strcpy(angle_buf, "+45");
    } else if (g_state.angle < 0) {
        strcpy(angle_buf, "-45");
    } else {
        angle_buf[0] = '\0';
    }

    if (angle_buf[0]) {
        return snprintk(out, out_sz,
            "{ \"type\":\"state\", \"active\":[\"%s\",\"%s\"] }",
            mode_str, angle_buf);
    } else {
        return snprintk(out, out_sz,
            "{ \"type\":\"state\", \"active\":[\"%s\"] }", mode_str);
    }
}

/* --- State update + broadcast --- */

static void broadcast_state(void)
{
    char json[128];
    int len = build_state_json(json, sizeof(json));
    if (len <= 0) return;

    /* Server frames: opcode TEXT; server must NOT mask (mask=false). */
    k_mutex_lock(&g_clients_lock, K_FOREVER);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        int ws = g_clients[i];
        if (ws < 0) continue;
        int ret = websocket_send_msg(ws, (const uint8_t *)json, len,
                                     WEBSOCKET_OPCODE_DATA_TEXT,
                                     false /*mask*/, true /*final*/, K_NO_WAIT);
        if (ret < 0) {
            LOG_WRN("send state to ws=%d failed (%d)", ws, ret);
        }
    }
    k_mutex_unlock(&g_clients_lock);
}

/* Apply a single command from the browser */
static void apply_command(const char *cmd)
{
    /* Trim whitespace */
    while (isspace((unsigned char)*cmd)) cmd++;

    if (!*cmd) return;

    if (strcmp(cmd, "+45") == 0) {
        g_state.angle = +45;
    } else if (strcmp(cmd, "-45") == 0) {
        g_state.angle = -45;
    } else {
        enum loop_mode m = cmd_to_mode(cmd);
        if (m != MODE_NONE) {
            g_state.mode = m;
        } else if (strcmp(cmd, "0") == 0 || strcmp(cmd, "Reset") == 0) {
            g_state.angle = 0;
        } else {
            LOG_WRN("Unknown command: '%s'", cmd);
            return;
        }
    }

    /* After every accepted command, broadcast full state */
    broadcast_state();
}

/* --- Per-connection handler (runs in the HTTP server's callback context).
 * For production, you can spawn a dedicated thread per connection if desired.
 */

#define WS_TEMP_BUF 512

static int ws_handler_loop(int ws)
{
    uint8_t buf[WS_TEMP_BUF];

    for (;;) {
        uint32_t msg_type = 0;
        uint64_t remaining = 0;

        /* Receive one fragment (or full message if small); blocks */
        int rc = websocket_recv_msg(ws, buf, sizeof(buf) - 1,
                                    &msg_type, &remaining, K_FOREVER);
        if (rc <= 0) {
            LOG_INF("ws=%d closed or error (%d)", ws, rc);
            return rc;
        }

        /* TEXT frames carry our commands; CLOSE ends connection */
        if (msg_type & WEBSOCKET_FLAG_CLOSE) {
            LOG_INF("ws=%d close frame", ws);
            return 0;
        }

        if (msg_type & WEBSOCKET_FLAG_TEXT) {
            buf[rc] = '\0';
            apply_command((const char *)buf);

            /* Drain any continuation frames of this message (we do not expect
             * fragmentation from the browser for these small commands, but be
             * robust anyway).
             */
            while (remaining > 0) {
                int rd = websocket_recv_msg(ws, buf, MIN(remaining, sizeof(buf)),
                                            &msg_type, &remaining, K_FOREVER);
                if (rd <= 0) break;
            }
        }
    }
}

/* --- WebSocket resource callback: called when HTTP upgrade happens --- */

static uint8_t ws_recv_buffer[1024];  /* scratch buffer used by the server */

static int ws_setup(int sock, struct http_request_ctx *request_ctx, void *user_data)
{
    ARG_UNUSED(request_ctx);
    ARG_UNUSED(user_data);

    /* Register the just-upgraded TCP socket as a WebSocket so we can use
     * websocket_* helpers to recv/send frames.
     */
    int ws = websocket_register(sock, ws_recv_buffer, sizeof(ws_recv_buffer));
    if (ws < 0) {
        LOG_ERR("websocket_register failed: %d", ws);
        zsock_close(sock);
        return ws;
    }

    LOG_INF("WebSocket connected (fd=%d)", ws);
    clients_add(ws);

    /* Send initial state immediately after connect */
    broadcast_state();

    /* Run the receive loop (blocking). Return when client disconnects. */
    int rc = ws_handler_loop(ws);

    clients_remove(ws);
    (void)websocket_unregister(ws);
    (void)zsock_close(ws);
    LOG_INF("WebSocket disconnected (fd=%d, rc=%d)", ws, rc);
    return rc;
}

/* Declare the WS resource at path "/ws" under our service */
static struct http_resource_detail_websocket ws_resource_detail = {
    .common = {
        .type = HTTP_RESOURCE_TYPE_WEBSOCKET,
        /* Upgrade starts from HTTP GET */
        .bitmask_of_supported_http_methods = BIT(HTTP_GET),
    },
    .cb = ws_setup,
    .data_buffer = ws_recv_buffer,
    .data_buffer_len = sizeof(ws_recv_buffer),
    .user_data = NULL,
};

/* Make it visible to the HTTP server */
HTTP_RESOURCE_DEFINE(ws_resource, my_service, "/ws", &ws_resource_detail);

/* --- App entry --- */
void main(void)
{
    k_mutex_init(&g_clients_lock);
    LOG_INF("Loop WebSocket server ready at ws://<device-ip>/ws");
}