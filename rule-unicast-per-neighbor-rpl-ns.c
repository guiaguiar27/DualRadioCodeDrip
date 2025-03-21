#include "orchestra/orchestra-conf.h"
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
static struct tsch_slotframe *sf_unicast;

/* The current RPL preferred parent's link-layer address */
linkaddr_t orchestra_parent_linkaddr;
/* Set to one only after getting an ACK for a DAO sent to our preferred parent */
int orchestra_parent_knows_us = 0;

#define RADIO1_OFFSET 0
#define RADIO2_OFFSET 1

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
static uint16_t
get_node_channel_offset(const linkaddr_t *addr, int radio)
{
  if(addr != NULL && ORCHESTRA_UNICAST_MAX_CHANNEL_OFFSET >= ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET) {
    uint16_t hash = ORCHESTRA_LINKADDR_HASH(addr);
    if(radio == 1) {
      return (hash % (ORCHESTRA_UNICAST_MAX_CHANNEL_OFFSET - ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET + 1)) + ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET;
    } else {
      return ((hash + 1) % (ORCHESTRA_UNICAST_MAX_CHANNEL_OFFSET - ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET + 1)) + ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET;
    }
  } else {
    return 0xffff;
  }
}

/*---------------------------------------------------------------------------*/
static int
select_packet(uint16_t *slotframe, uint16_t *timeslot)
{
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) == FRAME802154_DATAFRAME && !linkaddr_cmp(dest, &linkaddr_null)) {
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
init(uint16_t sf_handle)
{
  printf("Entrou na funcao!\n");
  int i;
  uint16_t rx_timeslot;

  linkaddr_t *local_addr = &linkaddr_node_addr;
  slotframe_handle = sf_handle;
  sf_unicast = tsch_schedule_add_slotframe(slotframe_handle, ORCHESTRA_UNICAST_PERIOD);
  rx_timeslot = get_node_timeslot(local_addr);

  for(i = 0; i < ORCHESTRA_UNICAST_PERIOD; i+=2){
    if(i == rx_timeslot){
      tsch_schedule_add_link(sf_unicast, LINK_OPTION_SHARED | LINK_OPTION_TX | LINK_OPTION_RX, LINK_TYPE_NORMAL, &tsch_broadcast_address, i, get_node_channel_offset(local_addr, 1));
      tsch_schedule_add_link(sf_unicast, LINK_OPTION_SHARED | LINK_OPTION_TX | LINK_OPTION_RX, LINK_TYPE_NORMAL, &tsch_broadcast_address, i+1, get_node_channel_offset(local_addr, 2));
    } else {
      tsch_schedule_add_link(sf_unicast, LINK_OPTION_SHARED | LINK_OPTION_TX, LINK_TYPE_NORMAL, &tsch_broadcast_address, i, get_node_channel_offset(local_addr, 1));
      tsch_schedule_add_link(sf_unicast, LINK_OPTION_SHARED | LINK_OPTION_TX, LINK_TYPE_NORMAL, &tsch_broadcast_address, i+1, get_node_channel_offset(local_addr, 2));
    }
  }
}

/*---------------------------------------------------------------------------*/
struct orchestra_rule* create_orchestra_rule() {
  struct orchestra_rule* rule = malloc(sizeof(struct orchestra_rule));
  if (!rule) {
    fprintf(stderr, "Error: Failed to allocate memory for orchestra_rule.\n");
    return NULL;
  }
  rule->init = init;
  rule->new_time_source = NULL;
  rule->select_packet = select_packet;
  rule->child_added = NULL;
  rule->child_removed = NULL;
  printf("Success in the allocation\n");
  return rule;
}
