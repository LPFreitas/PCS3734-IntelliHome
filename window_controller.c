#include "contiki.h"
#include "dev/leds.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdlib.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY 1
#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

static uint32_t LUMINOSITY_LIMIT = 20000;

static struct simple_udp_connection udp_conn;

PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
                const uip_ipaddr_t *sender_addr,
                uint16_t sender_port,
                const uip_ipaddr_t *receiver_addr,
                uint16_t receiver_port,
                const uint8_t *data,
                uint16_t datalen)
{
    // LOG_INFO("Received luminosity value '%.*s' from ", datalen, (char *)data);
    // LOG_INFO_6ADDR(sender_addr);
    // LOG_INFO_("\n");
    // LOG_INFO_("\n");
    uint32_t received_luminosity_value = strtoul((const char *)data, NULL, 10);
    printf("Received luminosity: %lu", received_luminosity_value);
    if (received_luminosity_value >= LUMINOSITY_LIMIT)
    {
        leds_off(LEDS_RED);
        leds_on(LEDS_GREEN);
    }
    else if (received_luminosity_value < LUMINOSITY_LIMIT)
    {
        leds_off(LEDS_GREEN);
        leds_on(LEDS_RED);
    }

#if WITH_SERVER_REPLY
    /* send back the same string to the client as an echo reply */
    // LOG_INFO("Sending response.\n");
    simple_udp_sendto(&udp_conn, data, datalen, sender_addr);
#endif /* WITH_SERVER_REPLY */
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
    PROCESS_BEGIN();

    /* Initialize DAG root */
    NETSTACK_ROUTING.root_start();

    /* Initialize UDP connection */
    simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                        UDP_CLIENT_PORT, udp_rx_callback);

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
