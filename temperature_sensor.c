#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include <stdint.h>
#include <inttypes.h>

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY 1
#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define SEND_INTERVAL_TEMP (30 * CLOCK_SECOND)

static uint32_t max_temperature = 36;
static uint32_t min_temperature = 14;
static uint32_t temperature_var = 2;

static struct simple_udp_connection udp_conn;
static uint32_t rx_count = 0;
static uint32_t temperature_value = 20;
static bool rising = true;

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);
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

    //     LOG_INFO("Received response '%.*s' from ", datalen, (char *)data);
    //     LOG_INFO_6ADDR(sender_addr);
    // #if LLSEC802154_CONF_ENABLED
    //     LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
    // #endif
    //     LOG_INFO_("\n");
    rx_count++;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
    static struct etimer periodic_timer;
    static char str[32];
    uip_ipaddr_t dest_ipaddr;
    static uint32_t tx_count;
    static uint32_t missed_tx_count;

    PROCESS_BEGIN();

    /* Initialize UDP connection */
    simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                        UDP_SERVER_PORT, udp_rx_callback);

    etimer_set(&periodic_timer, SEND_INTERVAL_TEMP);
    while (1)
    {
        if (rising)
        {
            temperature_value += temperature_var;
            if (temperature_value >= max_temperature)
                rising = false;
        }
        else
        {
            temperature_value -= temperature_var;
            if (temperature_value <= min_temperature)
                rising = true;
        }

        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

        if (NETSTACK_ROUTING.node_is_reachable() &&
            NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr))
        {

            /* Print statistics every 10th TX */
            // if (tx_count % 10 == 0)
            // {
            //   LOG_INFO("Tx/Rx/MissedTx: %" PRIu32 "/%" PRIu32 "/%" PRIu32 "\n",
            //            tx_count, rx_count, missed_tx_count);
            // }

            /* Send to DAG root */
            printf("Mesured temperature: %d\n", (int)temperature_value);
            // LOG_INFO("Sending request %" PRIu32 " to ", tx_count);
            // LOG_INFO_6ADDR(&dest_ipaddr);
            // LOG_INFO_("\n");
            snprintf(str, sizeof(str), "%" PRIu32 "", temperature_value);
            simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
            tx_count++;
        }
        else
        {
            LOG_INFO("Not reachable yet\n");
            if (tx_count > 0)
            {
                missed_tx_count++;
            }
        }

        /* Add some jitter */
        etimer_set(&periodic_timer, SEND_INTERVAL_TEMP - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
    }

    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
