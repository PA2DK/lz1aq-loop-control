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