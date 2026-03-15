#include "wifi.h"
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(wifi);

static struct net_if *wifi_interface;
static struct wifi_connect_req_params config;

void connect_wifi()
{
    if (!wifi_interface) {
        LOG_INF("STA: interface no initialized");
        return;
    }

    config.ssid = (const uint8_t *)CONFIG_WIFI_SAMPLE_SSID;
    config.ssid_length = sizeof(CONFIG_WIFI_SAMPLE_SSID) - 1;
    config.psk = (const uint8_t *)CONFIG_WIFI_SAMPLE_PSK;
    config.psk_length = sizeof(CONFIG_WIFI_SAMPLE_PSK) - 1;
    config.security = WIFI_SECURITY_TYPE_PSK;
    config.channel = WIFI_CHANNEL_ANY;
    config.band = WIFI_FREQ_BAND_2_4_GHZ;

    LOG_INF("Connecting to SSID: %s\n", config.ssid);

    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, wifi_interface, &config,
                sizeof(struct wifi_connect_req_params));
    if (ret) {
        LOG_ERR("Unable to Connect to (%s)", CONFIG_WIFI_SAMPLE_SSID);
    }
}

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

// connect_wifi();