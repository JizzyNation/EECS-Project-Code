#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "pico/time.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"

/* =========================
   USER CONFIGURATION
   ========================= */

#define WIFI_SSID           "YOUR_WIFI_NAME"
#define WIFI_PASSWORD       "YOUR_WIFI_PASSWORD"

#define SERVER_IP           "192.168.1.100"   // Java server IP
#define SERVER_PORT         5000

#define DEVICE_ID           "PICO2W_NODE_01"

/* Choose protocol:
 * 1 = TCP
 * 0 = UDP
 */
#define USE_TCP             1

/* ADC sensor configuration */
#define SENSOR_ADC_INPUT    0       // GPIO26 = ADC0
#define SENSOR_ADC_GPIO     26

/* Actuator configuration */
#define ACTUATOR_LED_GPIO   CYW43_WL_GPIO_LED_PIN

/* Sampling */
#define DEFAULT_SAMPLE_MS   1000
#define WIFI_CONNECT_RETRY_DELAY_MS 3000

/* =========================
   APPLICATION STATE
   ========================= */

typedef struct {
    uint16_t adc_raw;
    float voltage;
    bool led_state;
    uint64_t timestamp_ms;
} sensor_data_t;

typedef struct {
    uint32_t sample_interval_ms;
    bool led_state;
    bool wifi_connected;
#if USE_TCP
    struct tcp_pcb *tcp_pcb;
    bool tcp_connected;
#else
    struct udp_pcb *udp_pcb;
#endif
    ip_addr_t server_addr;
} app_state_t;

static app_state_t g_app = {
    .sample_interval_ms = DEFAULT_SAMPLE_MS,
    .led_state = false,
    .wifi_connected = false,
#if USE_TCP
    .tcp_pcb = NULL,
    .tcp_connected = false,
#else
    .udp_pcb = NULL,
#endif
};

/* =========================
   SENSOR LAYER
   ========================= */

static void sensor_init(void) {
    adc_init();
    adc_gpio_init(SENSOR_ADC_GPIO);
    adc_select_input(SENSOR_ADC_INPUT);
}

static sensor_data_t sensor_read(void) {
    sensor_data_t data;
    data.adc_raw = adc_read();
    data.voltage = (data.adc_raw * 3.3f) / 4095.0f;
    data.led_state = g_app.led_state;
    data.timestamp_ms = to_ms_since_boot(get_absolute_time());
    return data;
}

/* =========================
   ACTUATOR LAYER
   ========================= */

static void actuator_init(void) {
    g_app.led_state = false;
    cyw43_arch_gpio_put(ACTUATOR_LED_GPIO, 0);
}

static void actuator_set_led(bool on) {
    g_app.led_state = on;
    cyw43_arch_gpio_put(ACTUATOR_LED_GPIO, on ? 1 : 0);
}

/* =========================
   MESSAGE / PROTOCOL LAYER
   ========================= */

static void build_sensor_message(const sensor_data_t *data, char *buffer, size_t len) {
    snprintf(buffer, len,
             "SENSOR,%s,%llu,%u,%.3f,%d\n",
             DEVICE_ID,
             data->timestamp_ms,
             data->adc_raw,
             data->voltage,
             data->led_state ? 1 : 0);
}

static void handle_command(const char *cmd) {
    if (cmd == NULL) return;

    if (strncmp(cmd, "LED,ON", 6) == 0) {
        actuator_set_led(true);
        printf("Command received: LED ON\n");
    } else if (strncmp(cmd, "LED,OFF", 7) == 0) {
        actuator_set_led(false);
        printf("Command received: LED OFF\n");
    } else if (strncmp(cmd, "SAMPLE,", 7) == 0) {
        int val = atoi(cmd + 7);
        if (val >= 100 && val <= 60000) {
            g_app.sample_interval_ms = (uint32_t)val;
            printf("Command received: SAMPLE interval set to %u ms\n", g_app.sample_interval_ms);
        } else {
            printf("Invalid SAMPLE interval: %d\n", val);
        }
    } else {
        printf("Unknown command: %s\n", cmd);
    }
}

/* =========================
   WIFI LAYER
   ========================= */

static bool wifi_connect(void) {
    printf("Initializing WiFi...\n");

    if (cyw43_arch_init()) {
        printf("WiFi init failed\n");
        return false;
    }

    cyw43_arch_enable_sta_mode();

    printf("Connecting to WiFi SSID: %s\n", WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(
            WIFI_SSID,
            WIFI_PASSWORD,
            CYW43_AUTH_WPA2_AES_PSK,
            20000) != 0) {
        printf("WiFi connection failed\n");
        cyw43_arch_deinit();
        return false;
    }

    printf("WiFi connected successfully\n");
    g_app.wifi_connected = true;
    return true;
}

/* =========================
   TCP NETWORK LAYER
   ========================= */
#if USE_TCP

static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    (void)arg;

    if (!p) {
        printf("TCP connection closed by server\n");
        g_app.tcp_connected = false;
        if (tpcb) {
            tcp_close(tpcb);
        }
        g_app.tcp_pcb = NULL;
        return ERR_OK;
    }

    if (err != ERR_OK) {
        pbuf_free(p);
        return err;
    }

    char recv_buf[128] = {0};
    size_t copy_len = (p->tot_len < sizeof(recv_buf) - 1) ? p->tot_len : sizeof(recv_buf) - 1;
    pbuf_copy_partial(p, recv_buf, copy_len, 0);
    recv_buf[copy_len] = '\0';

    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);

    printf("TCP received: %s\n", recv_buf);
    handle_command(recv_buf);

    return ERR_OK;
}

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    (void)arg;

    if (err != ERR_OK) {
        printf("TCP connection callback error: %d\n", err);
        g_app.tcp_connected = false;
        return err;
    }

    printf("TCP connected to server\n");
    g_app.tcp_connected = true;
    g_app.tcp_pcb = tpcb;
    tcp_recv(tpcb, tcp_client_recv);

    return ERR_OK;
}

static bool tcp_client_connect_to_server(void) {
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!pcb) {
        printf("Failed to create TCP PCB\n");
        return false;
    }

    err_t err = tcp_connect(pcb, &g_app.server_addr, SERVER_PORT, tcp_client_connected);
    if (err != ERR_OK) {
        printf("TCP connect failed: %d\n", err);
        tcp_abort(pcb);
        return false;
    }

    g_app.tcp_pcb = pcb;
    return true;
}

static bool tcp_send_message(const char *msg) {
    if (!g_app.tcp_connected || !g_app.tcp_pcb || !msg) {
        return false;
    }

    size_t len = strlen(msg);
    err_t err = tcp_write(g_app.tcp_pcb, msg, len, TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        printf("TCP write failed: %d\n", err);
        g_app.tcp_connected = false;
        return false;
    }

    err = tcp_output(g_app.tcp_pcb);
    if (err != ERR_OK) {
        printf("TCP output failed: %d\n", err);
        g_app.tcp_connected = false;
        return false;
    }

    return true;
}

#else

/* =========================
   UDP NETWORK LAYER
   ========================= */

static bool udp_client_init(void) {
    g_app.udp_pcb = udp_new_ip_type(IPADDR_TYPE_V4);
    if (!g_app.udp_pcb) {
        printf("Failed to create UDP PCB\n");
        return false;
    }

    printf("UDP client initialized\n");
    return true;
}

static bool udp_send_message(const char *msg) {
    if (!g_app.udp_pcb || !msg) return false;

    size_t len = strlen(msg);
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, (u16_t)len, PBUF_RAM);
    if (!p) {
        printf("UDP pbuf allocation failed\n");
        return false;
    }

    memcpy(p->payload, msg, len);

    err_t err = udp_sendto(g_app.udp_pcb, p, &g_app.server_addr, SERVER_PORT);
    pbuf_free(p);

    if (err != ERR_OK) {
        printf("UDP send failed: %d\n", err);
        return false;
    }

    return true;
}
#endif

/* =========================
   APP LAYER
   ========================= */

static bool network_setup(void) {
    if (!ip4addr_aton(SERVER_IP, ip_2_ip4(&g_app.server_addr))) {
        printf("Invalid server IP address: %s\n", SERVER_IP);
        return false;
    }

#if USE_TCP
    printf("Using TCP transport\n");
    return tcp_client_connect_to_server();
#else
    printf("Using UDP transport\n");
    return udp_client_init();
#endif
}

static void app_send_sensor_data(void) {
    sensor_data_t data = sensor_read();
    char msg[128];

    build_sensor_message(&data, msg, sizeof(msg));
    printf("Sending: %s", msg);

#if USE_TCP
    if (!tcp_send_message(msg)) {
        printf("TCP send failed, connection may be down\n");
    }
#else
    if (!udp_send_message(msg)) {
        printf("UDP send failed\n");
    }
#endif
}

static void app_main_loop(void) {
    absolute_time_t next_sample = make_timeout_time_ms(g_app.sample_interval_ms);

    while (true) {
#if USE_TCP
        if (!g_app.tcp_connected) {
            printf("TCP not connected. Reconnecting...\n");
            sleep_ms(2000);
            tcp_client_connect_to_server();
        }
#endif

        if (absolute_time_diff_us(get_absolute_time(), next_sample) <= 0) {
            app_send_sensor_data();
            next_sample = make_timeout_time_ms(g_app.sample_interval_ms);
        }

        /* Small sleep to avoid busy loop */
        sleep_ms(20);
    }
}

/* =========================
   MAIN
   ========================= */

int main(void) {
    stdio_init_all();
    sleep_ms(2000);

    printf("\n=== Wireless Sensor Dashboard: Pico 2 W ===\n");

    if (!wifi_connect()) {
        printf("Fatal: WiFi setup failed\n");
        return 1;
    }

    sensor_init();
    actuator_init();

    if (!network_setup()) {
        printf("Fatal: Network setup failed\n");
        return 1;
    }

    app_main_loop();

    cyw43_arch_deinit();
    return 0;
}