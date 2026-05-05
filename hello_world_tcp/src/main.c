#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"
#include "espconn.h"

// ==================== TCP CALLBACKS ====================

void tcp_recv_cb(void *arg, char *pdata, unsigned short len)
{
    os_printf("Received: %s\n", pdata);

    // Echo back
    espconn_sent((struct espconn *)arg, (uint8 *)pdata, len);
}

void tcp_connect_cb(void *arg)
{
    os_printf("Client connected!\n");
    struct espconn *conn = (struct espconn *)arg;

    espconn_regist_recvcb(conn, tcp_recv_cb);
}

// ==================== TCP SERVER ====================

static struct espconn server_conn;
static esp_tcp server_tcp;

void start_tcp_server()
{
    server_conn.type = ESPCONN_TCP;
    server_conn.state = ESPCONN_NONE;

    server_tcp.local_port = 8080;
    server_conn.proto.tcp = &server_tcp;

    espconn_regist_connectcb(&server_conn, tcp_connect_cb);

    espconn_accept(&server_conn);

    os_printf("TCP Server started on 8080\n");
}

// ==================== SOFT AP MODE ====================

void setup_softap()
{
    struct softap_config config;

    wifi_set_opmode(SOFTAP_MODE);
    wifi_softap_get_config(&config);

    os_memset(&config, 0, sizeof(config));

    os_strcpy((char *)config.ssid, "ESP_TEST");
    os_strcpy((char *)config.password, "12345678");

    config.ssid_len = 0;
    config.authmode = AUTH_WPA_WPA2_PSK;
    config.max_connection = 4;

    wifi_softap_set_config(&config);

    os_printf("Soft AP started.\n");
    os_printf("SSID: ESP_TEST\nPassword: 12345678\n");
    os_printf("Connect your PC and use IP 192.168.4.1:8080\n");
}

// ==================== MAIN ====================

void user_init(void)
{
    uart_div_modify(0, UART_CLK_FREQ / 115200);

    os_printf("\n\n=== ESP8266 TCP Echo Server ===\n");

    setup_softap();
    start_tcp_server();
}

uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set()
{
    return 128 - 5;
}
