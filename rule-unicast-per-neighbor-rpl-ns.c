
#include "contiki.h"
#include "orchestra.h"
#include "net/packetbuf.h"
#include "net/ipv6/uip-icmp6.h"
#include "net/routing/routing.h"
#if ROUTING_CONF_RPL_LITE
#include "net/routing/rpl-lite/rpl.h"
#elif ROUTING_CONF_RPL_CLASSIC
#include "net/routing/rpl-classic/rpl.h"
#endif
#include "stdlib.h"


#include "net/ipv6/uip-debug.h"

#include "net/ipv6/uip-ds6-route.h"
#include "net/packetbuf.h"

static uint16_t slotframe_handle = 0;
static uint16_t channel_offset = 0;
static struct tsch_slotframe *sf_unicast;


/* The current RPL preferred parent's link-layer address */
linkaddr_t orchestra_parent_linkaddr;
/* Set to one only after getting an ACK for a DAO sent to our preferred parent */
int orchestra_parent_knows_us = 0;

/*---------------------------------------------------------------------------*/
static uint16_t
get_node_timeslot(const linkaddr_t *addr)
{
  if(addr != NULL && ORCHESTRA_UNICAST_PERIOD > 0) {
    return ORCHESTRA_LINKADDR_HASH(addr) % ORCHESTRA_UNICAST_PERIOD;
  } else {
    return 0xffff;
  }
}
/*---------------------------------------------------------------------------*/
// static uint16_t
// get_node_channel_offset(const linkaddr_t *addr)
// {
//   if(addr != NULL && ORCHESTRA_UNICAST_MAX_CHANNEL_OFFSET >= ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET) {
//     return ORCHESTRA_LINKADDR_HASH(addr) % (ORCHESTRA_UNICAST_MAX_CHANNEL_OFFSET - ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET + 1)
//         + ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET;
//   } else {
//     return 0xffff;
//   }
// }
/*---------------------------------------------------------------------------*/
static void
add_uc_link(const linkaddr_t *linkaddr)
{
  if(linkaddr != NULL) {
    uint16_t timeslot = get_node_timeslot(linkaddr);

    /* Add a Tx link to the neighbor; do not replace any existing links
     * at that cell. The channel offset here does not matter:
     * select_packet() always sets the right channel offset per packet. */
    tsch_schedule_add_link(sf_unicast,
        LINK_OPTION_SHARED | LINK_OPTION_TX,
        LINK_TYPE_NORMAL, &tsch_broadcast_address,
        timeslot, 0, 0);
  }
}
/*---------------------------------------------------------------------------*/
static void
remove_uc_link(const linkaddr_t *linkaddr)
{
  if(linkaddr != NULL) {
    uint16_t timeslot = get_node_timeslot(linkaddr);
    tsch_schedule_remove_link_by_offsets(sf_unicast, timeslot, 0);
    tsch_queue_free_packets_to(linkaddr);
  }
}
/*---------------------------------------------------------------------------*/
static void
neighbor_updated(const linkaddr_t *linkaddr, uint8_t is_added)
{
  if(is_added) {
    add_uc_link(linkaddr);
  } else {
    remove_uc_link(linkaddr);
  }
}
/*---------------------------------------------------------------------------*/
static int
select_packet(uint16_t *slotframe, uint16_t *timeslot, uint16_t *channel_offset)
{
    /* Select data packets we have a unicast link to */
     const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
     if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) == FRAME802154_DATAFRAME
        && !linkaddr_cmp(dest, &linkaddr_null)) {
       if(slotframe != NULL) {
         *slotframe = slotframe_handle;
       }
       if(timeslot != NULL) {
         *timeslot = get_node_timeslot(dest);
       }
       return 1;
     }
     return 0;
}
/*---------------------------------------------------------------------------*/
static void
new_time_source(const struct tsch_neighbor *old, const struct tsch_neighbor *new)
{

}

static void
child_added(const linkaddr_t *linkaddr)
{
}
/*---------------------------------------------------------------------------*/
static void
child_removed(const linkaddr_t *linkaddr)
{
}
/*---------------------------------------------------------------------------*/
static void
init(uint16_t sf_handle)
{
    printf("Entrou na funcao!\n");

    uint16_t rx_timeslot;
    linkaddr_t *local_addr = &linkaddr_node_addr;

    slotframe_handle = sf_handle;
    /* Slotframe for unicast transmissions */
    sf_unicast = tsch_schedule_add_slotframe(slotframe_handle, ORCHESTRA_UNICAST_PERIOD);
    rx_timeslot = get_node_timeslot(local_addr);
    /* Add a Rx link at our own timeslot. */
    tsch_schedule_add_link(sf_unicast,
        LINK_OPTION_RX,
        LINK_TYPE_NORMAL, &tsch_broadcast_address,
        rx_timeslot, sf_handle);

}
struct orchestra_rule* create_orchestra_rule() {
  struct orchestra_rule* rule = malloc(sizeof(struct orchestra_rule));
  if (!rule) {
    fprintf(stderr, "Error: Failed to allocate memory for orchestra_rule.\n");
    return NULL;
  }
  rule->init = init;
  rule->new_time_source = new_time_source;
  rule->select_packet = select_packet;
  rule->child_added = child_added;
  rule->child_removed = child_removed;
  printf("Sucess in the allocation\n");
  return rule;
}
