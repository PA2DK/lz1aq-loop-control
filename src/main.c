#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/service.h>

#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>
#include <errno.h>
#include <stdio.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(MAIN);

#if !DT_NODE_EXISTS(DT_NODELABEL(relay_cross))
#error "Overlay for relay_cross not properly defined."
#endif

#if !DT_NODE_EXISTS(DT_NODELABEL(relay_dipole_loop))
#error "Overlay for relay_dipole_loop not properly defined."
#endif

#if !DT_NODE_EXISTS(DT_NODELABEL(relay_a_b))
#error "Overlay for relay_a_b not properly defined."
#endif

#if !DT_NODE_EXISTS(DT_NODELABEL(relay_minus_plus_45))
#error "Overlay for relay_minus_plus_45 not properly defined."
#endif


static const struct gpio_dt_spec relay_cross =
    GPIO_DT_SPEC_GET_OR(DT_NODELABEL(relay_cross), gpios, {0});

static const struct gpio_dt_spec relay_dipole_loop = 
    GPIO_DT_SPEC_GET_OR(DT_NODELABEL(relay_dipole_loop), gpios, {0});

static const struct gpio_dt_spec relay_a_b = 
    GPIO_DT_SPEC_GET_OR(DT_NODELABEL(relay_a_b), gpios, {0});

static const struct gpio_dt_spec relay_minus_plus_45 = 
    GPIO_DT_SPEC_GET_OR(DT_NODELABEL(relay_minus_plus_45), gpios, {0});

static uint16_t http_service_port = 80;

HTTP_SERVICE_DEFINE(lz1aq_loop_control, "0.0.0.0", &http_service_port, 1, 10, NULL, NULL, NULL);

static const uint8_t index_html_gz[] = {
    #include "index.html.gz.inc"
};

struct http_resource_detail_static index_html_gz_resource_detail = {
    .common = {
        .type = HTTP_RESOURCE_TYPE_STATIC,
        .bitmask_of_supported_http_methods = BIT(HTTP_GET),
        .content_encoding = "gzip",
        .content_type = "text/html"
    },
    .static_data = index_html_gz,
    .static_data_len = sizeof(index_html_gz),
};

HTTP_RESOURCE_DEFINE(index_html_gz_resource, lz1aq_loop_control, "/",
                     &index_html_gz_resource_detail);

static const uint8_t main_js_gz[] = {
    #include "main.js.gz.inc"
};

struct http_resource_detail_static main_js_gz_resource_detail = {
    .common = {
        .type = HTTP_RESOURCE_TYPE_STATIC,
        .bitmask_of_supported_http_methods = BIT(HTTP_GET),
        .content_encoding = "gzip",
        .content_type = "text/javascript",
    },
    .static_data = main_js_gz,
    .static_data_len = sizeof(main_js_gz),
};

HTTP_RESOURCE_DEFINE(main_js_gz_resource, lz1aq_loop_control, "/main.js",
                     &main_js_gz_resource_detail);

static const uint8_t bootstrap_min_css_gz[] = {
    #include "bootstrap.min.css.gz.inc"
};

struct http_resource_detail_static bootstrap_min_css_gz_resource_detail = {
    .common = {
        .type = HTTP_RESOURCE_TYPE_STATIC,
        .bitmask_of_supported_http_methods = BIT(HTTP_GET),
        .content_encoding = "gzip",
        .content_type = "text/css",
    },
    .static_data = bootstrap_min_css_gz,
    .static_data_len = sizeof(bootstrap_min_css_gz),
};

HTTP_RESOURCE_DEFINE(bootstrap_min_css_resource, lz1aq_loop_control, "/bootstrap.min.css",
                     &bootstrap_min_css_gz_resource_detail);


static int loop_a_handler(struct http_client_ctx *client, enum http_transaction_status status,
			  const struct http_request_ctx *request_ctx,
			  struct http_response_ctx *response_ctx, void *user_data)
{
	gpio_pin_set_dt(&relay_dipole_loop, 1);
  	gpio_pin_set_dt(&relay_a_b, 0);
	gpio_pin_set_dt(&relay_cross, 0);
	return 0;
}

struct http_resource_detail_dynamic loop_a_handler_resource_detail = {
    .common = {
        .type = HTTP_RESOURCE_TYPE_DYNAMIC,
        .bitmask_of_supported_http_methods =
            BIT(HTTP_GET),
    },
    .cb = loop_a_handler,
    .user_data = NULL,
};

HTTP_RESOURCE_DEFINE(loop_a_handler_resource, lz1aq_loop_control, "/direction/loop_a", &loop_a_handler_resource_detail);

static int loop_b_handler(struct http_client_ctx *client, enum http_transaction_status status,
			  const struct http_request_ctx *request_ctx,
			  struct http_response_ctx *response_ctx, void *user_data)
{
	gpio_pin_set_dt(&relay_dipole_loop, 1);
  	gpio_pin_set_dt(&relay_a_b, 1);
	gpio_pin_set_dt(&relay_cross, 0);
	return 0;
}

struct http_resource_detail_dynamic loop_b_handler_resource_detail = {
    .common = {
        .type = HTTP_RESOURCE_TYPE_DYNAMIC,
        .bitmask_of_supported_http_methods =
            BIT(HTTP_GET),
    },
    .cb = loop_b_handler,
    .user_data = NULL,
};

HTTP_RESOURCE_DEFINE(loop_b_handler_resource, lz1aq_loop_control, "/direction/loop_b", &loop_b_handler_resource_detail);

static int loop_min_45_handler(struct http_client_ctx *client, enum http_transaction_status status,
			  const struct http_request_ctx *request_ctx,
			  struct http_response_ctx *response_ctx, void *user_data)
{
	gpio_pin_set_dt(&relay_dipole_loop, 1);
	gpio_pin_set_dt(&relay_cross, 1);
    	gpio_pin_set_dt(&relay_minus_plus_45, 0);
	return 0;
}

struct http_resource_detail_dynamic min_45_handler_resource_detail = {
    .common = {
        .type = HTTP_RESOURCE_TYPE_DYNAMIC,
        .bitmask_of_supported_http_methods =
            BIT(HTTP_GET),
    },
    .cb = loop_min_45_handler,
    .user_data = NULL,
};

HTTP_RESOURCE_DEFINE(min_45_handler_resource, lz1aq_loop_control, "/direction/min_45", &min_45_handler_resource_detail);

static int loop_plus_45_handler(struct http_client_ctx *client, enum http_transaction_status status,
			  const struct http_request_ctx *request_ctx,
			  struct http_response_ctx *response_ctx, void *user_data)
{
	gpio_pin_set_dt(&relay_dipole_loop, 1);
	gpio_pin_set_dt(&relay_cross, 1);
    	gpio_pin_set_dt(&relay_minus_plus_45, 1);
	return 0;
}

struct http_resource_detail_dynamic plus_45_handler_resource_detail = {
    .common = {
        .type = HTTP_RESOURCE_TYPE_DYNAMIC,
        .bitmask_of_supported_http_methods =
            BIT(HTTP_GET),
    },
    .cb = loop_plus_45_handler,
    .user_data = NULL,
};

HTTP_RESOURCE_DEFINE(plus_45_handler_resource, lz1aq_loop_control, "/direction/plus_45", &plus_45_handler_resource_detail);

static int vertical_handler(struct http_client_ctx *client, enum http_transaction_status status,
			  const struct http_request_ctx *request_ctx,
			  struct http_response_ctx *response_ctx, void *user_data)
{
	gpio_pin_set_dt(&relay_dipole_loop, 0);
	gpio_pin_set_dt(&relay_cross, 0);
    	gpio_pin_set_dt(&relay_minus_plus_45, 0);
  	gpio_pin_set_dt(&relay_a_b, 0);
	return 0;
}

struct http_resource_detail_dynamic vertical_handler_resource_detail = {
    .common = {
        .type = HTTP_RESOURCE_TYPE_DYNAMIC,
        .bitmask_of_supported_http_methods =
            BIT(HTTP_GET),
    },
    .cb = vertical_handler,
    .user_data = NULL,
};

HTTP_RESOURCE_DEFINE(vertical_handler_resource, lz1aq_loop_control, "/direction/vertical", &vertical_handler_resource_detail);

enum loop_mode {
    MODE_NONE = 0,
    MODE_LOOP_A,
    MODE_LOOP_B,
    MODE_CROSSED,
    MODE_VERTICAL,
};

/* --- Helpers to convert to/from strings --- */
static const char* mode_to_cmd(enum loop_mode m)
{
    switch (m) {
        case MODE_LOOP_A:   return "loop_a";
        case MODE_LOOP_B:   return "loop_a";
        case MODE_CROSSED:  return "crossed";
        case MODE_VERTICAL: return "vertical";
        default:            return "";
    }
}

static enum loop_mode cmd_to_mode(const char *s)
{
    if (strcmp(s, "loop_a") == 0) return MODE_LOOP_A;
    if (strcmp(s, "loop_b") == 0) return MODE_LOOP_B;
    if (strcmp(s, "crossed") == 0) return MODE_CROSSED;
    if (strcmp(s, "vertical") == 0) return MODE_VERTICAL;
    return MODE_NONE;
}

#define DHCP_OPTION_NTP (42)

static uint8_t ntp_server[4];

static struct net_mgmt_event_callback mgmt_cb;

static struct net_dhcpv4_option_callback dhcp_cb;

static void start_dhcpv4_client(struct net_if *iface, void *user_data)
{
    ARG_UNUSED(user_data);

    LOG_INF("Start on %s: index=%d", net_if_get_device(iface)->name,
        net_if_get_by_iface(iface));
    net_dhcpv4_start(iface);
}

static void handler(struct net_mgmt_event_callback *cb,
		    uint64_t mgmt_event,
		    struct net_if *iface)
{
    int i = 0;

    if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
        return;
    }

    for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
        char buf[NET_IPV4_ADDR_LEN];

        if (iface->config.ip.ipv4->unicast[i].ipv4.addr_type !=
                            NET_ADDR_DHCP) {
            continue;
        }

        LOG_INF("   Address[%d]: %s", net_if_get_by_iface(iface),
            net_addr_ntop(NET_AF_INET,
                &iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr,
                            buf, sizeof(buf)));
        LOG_INF("    Subnet[%d]: %s", net_if_get_by_iface(iface),
            net_addr_ntop(NET_AF_INET,
                        &iface->config.ip.ipv4->unicast[i].netmask,
                        buf, sizeof(buf)));
        LOG_INF("    Router[%d]: %s", net_if_get_by_iface(iface),
            net_addr_ntop(NET_AF_INET,
                            &iface->config.ip.ipv4->gw,
                            buf, sizeof(buf)));
        LOG_INF("Lease time[%d]: %u seconds", net_if_get_by_iface(iface),
            iface->config.dhcpv4.lease_time);
	}
}

static void option_handler(struct net_dhcpv4_option_callback *cb,
        size_t length,
        enum net_dhcpv4_msg_type msg_type,
        struct net_if *iface)
{
    char buf[NET_IPV4_ADDR_LEN];

    LOG_INF("DHCP Option %d: %s", cb->option,
        net_addr_ntop(NET_AF_INET, cb->data, buf, sizeof(buf)));
}

#define NET_EVENT_WIFI_MASK (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT)

int main()
{
    LOG_INF("Startup LZ1AQ Loop Control");

    net_mgmt_init_event_callback(&mgmt_cb, handler, NET_EVENT_IPV4_ADDR_ADD);
    net_mgmt_add_event_callback(&mgmt_cb);

    net_dhcpv4_init_option_callback(&dhcp_cb, option_handler, DHCP_OPTION_NTP, ntp_server,
        sizeof(ntp_server));

    net_dhcpv4_add_option_callback(&dhcp_cb);

    net_if_foreach(start_dhcpv4_client, NULL);

    if (!gpio_is_ready_dt(&relay_cross)) {
        LOG_INF("The relay_cross switch pin GPIO port is not ready");
        return 0;
    }

    if(!gpio_is_ready_dt(&relay_dipole_loop)) {
        LOG_INF("The relay_dipole_loop switch pin GPIO port is not ready");
        return 0;
    }

    if(!gpio_is_ready_dt(&relay_a_b)) {
        LOG_INF("The relay_a_b switch pin GPIO port is not ready");
        return 0;
    }

    if(!gpio_is_ready_dt(&relay_minus_plus_45)) {
        LOG_INF("The relay_minus_plus_45 switch pin GPIO port is not ready");
        return 0;
    }

    gpio_pin_configure_dt(&relay_cross, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&relay_dipole_loop, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&relay_a_b, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&relay_minus_plus_45, GPIO_OUTPUT_INACTIVE);

    http_server_start();

    // LOG_INF("Turning on relays");

    // k_sleep(K_MSEC(2000));

    // gpio_pin_set_dt(&relay_cross, 1);

    // k_sleep(K_MSEC(2000));

    // gpio_pin_set_dt(&relay_a_b, 1);

    // k_sleep(K_MSEC(2000));

    // gpio_pin_set_dt(&relay_minus_plus_45, 1);

    // k_sleep(K_MSEC(2000));

    // gpio_pin_set_dt(&relay_dipole_loop, 1);

    // LOG_INF("Sleep 5 seconds");

    // k_sleep(K_MSEC(5000));

    // LOG_INF("Turning off relays");

    // gpio_pin_set_dt(&relay_dipole_loop, 0);
    // gpio_pin_set_dt(&relay_cross, 0);
    // gpio_pin_set_dt(&relay_a_b, 0);
    // gpio_pin_set_dt(&relay_minus_plus_45, 0);

    // LOG_INF("Turned of relays");

    return 0;
}
