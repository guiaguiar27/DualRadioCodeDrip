#include "contiki.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#define NUM_PACKETS 2 // Define the number of packets to work with

PROCESS(example_process, "Example Process");
AUTOSTART_PROCESSES(&example_process);

static struct simple_udp_connection udp_conn;
static char packet_payload[NUM_PACKETS][20]; // Array to hold packet payloads

PROCESS_THREAD(example_process, ev, data)
{
  static int packet_index = 0;

  PROCESS_BEGIN();

  simple_udp_register(&udp_conn, 12345, NULL, 12345, NULL);

  // Generate payloads for multiple packets
  for (int i = 0; i < NUM_PACKETS; i++) {
    snprintf(packet_payload[i], sizeof(packet_payload[i]), "Packet %d", i);
  }

  while (1) {
    if (packet_index < NUM_PACKETS) {
      // Send the next packet
      simple_udp_sendto(&udp_conn, packet_payload[packet_index], strlen(packet_payload[packet_index]) + 1, &rimeaddr_node_addr);
      packet_index++;
    }

    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_CONTINUE);
  }

  PROCESS_END();
}
