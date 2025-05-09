#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "net/mac/tsch/tsch.h"
#include "lib/random.h"
#include "sys/node-id.h"
#include "net/routing/routing.h"
#include "net/mac/tsch/tsch-schedule.h"

//#include "os/services/orchestra/orchestra.h"
#include "os/services/orchestra/orchestra.c"

#include "net/ipv6/uip-ds6-nbr.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-debug.h"




// this is the embbeding that im using to run the orchestra 

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define APP_UNICAST_TIMESLOT 4
#define APP_CHANNEL_OFSETT 8

#define UDP_PORT	8765
#define SEND_INTERVAL		  (60 * CLOCK_SECOND)


PROCESS(node_process, "TSCH Schedule Node");
AUTOSTART_PROCESSES(&node_process);
int count_link = 0;

#define APP_SLOTFRAME_HANDLE 1
#define FIRST_SLOTFRAME_HANDLE 2
#define SECOND_SLOTFRAME_HANDLE 3




static void
initialize_tsch_schedule(void)
{
  //int i, j;
  struct tsch_slotframe *sf_common = tsch_schedule_add_slotframe(APP_SLOTFRAME_HANDLE, 8);
  uint16_t slot_offset;
  uint16_t channel_offset;

  /* A "catch-all" cell at (0, 0) */

  slot_offset = 0;
  channel_offset = random_rand() % APP_CHANNEL_OFSETT;
  // broadcast
  tsch_schedule_add_link(sf_common,
      LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED,
      LINK_TYPE_ADVERTISING, &tsch_broadcast_address,
      slot_offset, channel_offset);
      struct tsch_slotframe *sf_common1 = tsch_schedule_add_slotframe(2, 8);

   tsch_schedule_add_link(sf_common1,
      LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED,
      LINK_TYPE_ADVERTISING, &tsch_broadcast_address,
      slot_offset, channel_offset+1);
init(APP_SLOTFRAME_HANDLE);
 tsch_schedule_print();

}

static void
rx_packet(struct simple_udp_connection *c,
          const uip_ipaddr_t *sender_addr,
          uint16_t sender_port,
          const uip_ipaddr_t *receiver_addr,
          uint16_t receiver_port,
          const uint8_t *data,
          uint16_t datalen)
{
  uint32_t seqnum;

  if(datalen >= sizeof(seqnum)) {
    memcpy(&seqnum, data, sizeof(seqnum));

    LOG_INFO("Received from ");
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_(", seqnum %" PRIu32 "\n", seqnum);
  }
}

PROCESS_THREAD(node_process, ev, data)
{
  static struct simple_udp_connection udp_conn;
  static struct etimer periodic_timer;
  static uint32_t seqnum;
  uip_ipaddr_t dst;

  PROCESS_BEGIN();

  if(node_id == 1) {  /* Running on the root? */
    NETSTACK_ROUTING.root_start();
    printf("This is the coordinator\n");
    tsch_set_coordinator(1);
  }

  initialize_tsch_schedule();
  printf("count: %d\n",count_link++);
  //find_y_for_x();



  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, rx_packet);
  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);

    /* Main loop */
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    if(NETSTACK_ROUTING.node_is_reachable()
       && NETSTACK_ROUTING.get_root_ipaddr(&dst)) {
      /* Send network uptime timestamp to the network root node */
      seqnum++;
      LOG_INFO("Send to ");
      LOG_INFO_6ADDR(&dst);
      LOG_INFO_(", seqnum %" PRIu32 "\n", seqnum);
      simple_udp_sendto(&udp_conn, &seqnum, sizeof(seqnum), &dst);

    //  broadcast_send(&broadcast);

      //list_neighbors();
    }
    etimer_set(&periodic_timer, SEND_INTERVAL);
  }

  PROCESS_END();
}
