#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/http/service.h>

//LOG_MODULE_REGISTER(net_dhcpv4_client_sample, LOG_LEVEL_DBG);

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


#if !DT_NODE_EXISTS(DT_NODELABEL(relay_cross))
#error "Overlay for relay_cross not properly defined."
#endif

#if !DT_NODE_EXISTS(DT_NODELABEL(relay_dipole_loop))
#error "Overlay for relay_dipole_loop not properly defined."
#endif

static const struct gpio_dt_spec relay_cross =
	GPIO_DT_SPEC_GET_OR(DT_NODELABEL(relay_cross), gpios, {0});

static const struct gpio_dt_spec relay_dipole_loop = 
  GPIO_DT_SPEC_GET_OR(DT_NODELABEL(relay_dipole_loop), gpios, {0});

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
    },
    .static_data = index_html_gz,
    .static_data_len = sizeof(index_html_gz),
};

HTTP_RESOURCE_DEFINE(index_html_gz_resource, lz1aq_loop_control, "/",
                     &index_html_gz_resource_detail);

static struct net_if *sta_iface;
static struct wifi_connect_req_params sta_config;
static struct net_mgmt_event_callback cb;

LOG_MODULE_REGISTER(MAIN);

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

//BUILD_ASSERT(sizeof(CONFIG_WIFI_SAMPLE_SSID) > 1,
//	     "CONFIG_WIFI_SAMPLE_SSID is empty. Please set it in conf file.");
	     
/*static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			       struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT: {
		LOG_INF("Connected to %s", CONFIG_WIFI_SAMPLE_SSID);
		break;
	}
	case NET_EVENT_WIFI_DISCONNECT_RESULT: {
		LOG_INF("Disconnected from %s", CONFIG_WIFI_SAMPLE_SSID);
		break;
	}
	default:
		break;
	}
}*/

/*static int connect_to_wifi(void)
{
	if (!sta_iface) {
		LOG_INF("STA: interface no initialized");
		return -EIO;
	}

	sta_config.ssid = (const uint8_t *)CONFIG_WIFI_SAMPLE_SSID;
	sta_config.ssid_length = sizeof(CONFIG_WIFI_SAMPLE_SSID) - 1;
	sta_config.psk = (const uint8_t *)CONFIG_WIFI_SAMPLE_PSK;
	sta_config.psk_length = sizeof(CONFIG_WIFI_SAMPLE_PSK) - 1;
	sta_config.security = WIFI_SECURITY_TYPE_PSK;
	sta_config.channel = WIFI_CHANNEL_ANY;
	sta_config.band = WIFI_FREQ_BAND_2_4_GHZ;

	LOG_INF("Connecting to SSID: %s\n", sta_config.ssid);

	int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, sta_iface, &sta_config,
			   sizeof(struct wifi_connect_req_params));
	if (ret) {
		LOG_ERR("Unable to Connect to (%s)", CONFIG_WIFI_SAMPLE_SSID);
	}

	return ret;
}*/

int main()
{

//LOG_INF("Run dhcpv4 client");

//net_mgmt_init_event_callback(&mgmt_cb, handler, NET_EVENT_IPV4_ADDR_ADD);
//net_mgmt_add_event_callback(&mgmt_cb);

//net_dhcpv4_init_option_callback(&dhcp_cb, option_handler,
//			DHCP_OPTION_NTP, ntp_server,
//			sizeof(ntp_server));

//net_dhcpv4_add_option_callback(&dhcp_cb);

//net_if_foreach(start_dhcpv4_client, NULL);

//net_mgmt_init_event_callback(&cb, wifi_event_handler, NET_EVENT_WIFI_MASK);
//net_mgmt_add_event_callback(&cb);

//net_mgmt_init_event_callback(&iface_callback, callback_handler,
//                             EVENT_IFACE_SET);
//net_mgmt_init_event_callback(&ipv4_callback, callback_handler,
//                             EVENT_IPV4_SET);
//net_mgmt_add_event_callback(&iface_callback);
//net_mgmt_add_event_callback(&ipv4_callback);

/* Get STA interface */
//sta_iface = net_if_get_wifi_sta();

// connect_to_wifi();

//http_server_start();

if (!gpio_is_ready_dt(&relay_cross)) {
  LOG_INF("The relay_cross switch pin GPIO port is not ready");
  return 0;
}

if(!gpio_is_ready_dt(&relay_dipole_loop)) {
  LOG_INF("The relay_dipole_loop switch pin GPIO port is not ready");
  return 0;
}

gpio_pin_configure_dt(&relay_cross, GPIO_OUTPUT_INACTIVE);
gpio_pin_configure_dt(&relay_dipole_loop, GPIO_OUTPUT_INACTIVE);

LOG_INF("Sleep 5seconds");

k_sleep(K_MSEC(5000));

LOG_INF("Turning on relays");

gpio_pin_set_dt(&relay_dipole_loop, 1);
int err = gpio_pin_set_dt(&relay_cross, 1);
if (err != 0) {
  LOG_ERR("Setting GPIO pin level failed: %d", err);
}

LOG_INF("Sleep 5 seconds");

k_sleep(K_MSEC(5000));

LOG_INF("Turning off relays");

gpio_pin_set_dt(&relay_dipole_loop, 0);
gpio_pin_set_dt(&relay_cross, 0);

LOG_INF("Turned of relays");

http_server_start();

return 0;
}
