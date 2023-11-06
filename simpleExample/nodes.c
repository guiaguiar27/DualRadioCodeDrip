#include "contiki.h"
#include "net/netstack.h"
#include "net/mac/tsch/tsch.h"

PROCESS(example_process, "Example Process");
AUTOSTART_PROCESSES(&example_process);

PROCESS_THREAD(example_process, ev, data)
{
  PROCESS_BEGIN();

  // Initialize TSCH and radios
  NETSTACK_MAC.init();
  NETSTACK_RADIO.init();
  NETSTACK_RADIO.on();

  // Create two packets
  static uint8_t packet1[] = "Packet 1";
  static uint8_t packet2[] = "Packet 2";

  while (1) {
    // Create two pseudo-threads for simultaneous transmission
    PT_BEGIN(&pt1);
    PT_BEGIN(&pt2);

    // Simultaneously send Packet 1 using Radio 1
    if (PT_SCHEDULE(&pt1)) {
      NETSTACK_RADIO.send(packet1, sizeof(packet1));
    }

    // Simultaneously send Packet 2 using Radio 2
    if (PT_SCHEDULE(&pt2)) {
      NETSTACK_RADIO.send(packet2, sizeof(packet2));
    }

    // Rest of your application logic can continue here
    // ...

    PROCESS_YIELD();
  }

  PROCESS_END();
}
