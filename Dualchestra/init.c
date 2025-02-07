/*
 * Copyright (c) 2016, Inria.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * \file
 *         Orchestra: a slotframe dedicated to unicast data transmission.
 *         Designed primarily for RPL non-storing mode but would work with any mode-of-operation.
 *         Uses uIPv6 DS6 neighbor tables to manage child nodes.
 *
 * \author Simon Duquennoy <simon.duquennoy@inria.fr>
 *         Atis Elsts <atis.elsts@edi.lv>
 */

#include "contiki.h"
#include "orchestra.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/packetbuf.h"
#include "stdlib.h"

static uint16_t slotframe_handle = 0;
static struct tsch_slotframe *sf_unicast = NULL;

/*---------------------------------------------------------------------------*/
/* Helper Functions */

// pode puxar direto da funcao local


// static uint16_t get_node_timeslot(const linkaddr_t *addr) {
//   if (addr != NULL && ORCHESTRA_UNICAST_PERIOD > 0) {
//     return ORCHESTRA_LINKADDR_HASH(addr) % ORCHESTRA_UNICAST_PERIOD;
//   }
//   return 0xffff;
// }

// static uint16_t get_node_channel_offset(const linkaddr_t *addr) {
//   if (addr != NULL && ORCHESTRA_UNICAST_MAX_CHANNEL_OFFSET >= ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET) {
//     return ORCHESTRA_LINKADDR_HASH(addr) %
//            (ORCHESTRA_UNICAST_MAX_CHANNEL_OFFSET - ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET + 1)
//            + ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET;
//   }
//   return 0xffff;
// }

/*---------------------------------------------------------------------------*/
/* Rule Functions */
static void init_function(uint16_t handle) {
  slotframe_handle = handle;
  sf_unicast = tsch_schedule_add_slotframe(slotframe_handle, ORCHESTRA_UNICAST_PERIOD);
  if (sf_unicast != NULL) {
    uint16_t rx_timeslot = get_node_timeslot(&linkaddr_node_addr);
    tsch_schedule_add_link(sf_unicast, LINK_OPTION_RX, LINK_TYPE_NORMAL,
                           &tsch_broadcast_address, rx_timeslot,
                           get_node_channel_offset(&linkaddr_node_addr));
  }
}

static void new_time_source_function(const struct tsch_neighbor *old, const struct tsch_neighbor *new) {
  const linkaddr_t *old_addr = tsch_queue_get_nbr_address(old);
  const linkaddr_t *new_addr = tsch_queue_get_nbr_address(new);
  if (old_addr) {
    tsch_schedule_remove_link_by_offsets(sf_unicast, get_node_timeslot(old_addr));
  }
  if (new_addr) {
    tsch_schedule_add_link(sf_unicast, LINK_OPTION_SHARED | LINK_OPTION_TX,
                           LINK_TYPE_NORMAL, &tsch_broadcast_address,
                           get_node_timeslot(new_addr), 0);
  }
}

static int select_packet_function(uint16_t *slotframe, uint16_t *timeslot, uint16_t *channel_offset) {
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if (packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) == FRAME802154_DATAFRAME
      && !linkaddr_cmp(dest, &linkaddr_null)) {
    if (slotframe) *slotframe = slotframe_handle;
    if (timeslot) *timeslot = get_node_timeslot(dest);
    if (channel_offset) *channel_offset = get_node_channel_offset(dest);
    return 1;
  }
  return 0;
}

static void neighbor_updated_function(const linkaddr_t *addr, uint8_t is_added) {
  if (is_added) {
    tsch_schedule_add_link(sf_unicast, LINK_OPTION_SHARED | LINK_OPTION_TX,
                           LINK_TYPE_NORMAL, &tsch_broadcast_address,
                           get_node_timeslot(addr), 0);
  } else {
    tsch_schedule_remove_link_by_offsets(sf_unicast, get_node_timeslot(addr), 0);
  }
}

/*---------------------------------------------------------------------------*/
/* Rule Initialization */
struct orchestra_rule* create_orchestra_rule() {
  struct orchestra_rule *rule = malloc(sizeof(struct orchestra_rule));
  if (rule) {
    rule->init = init_function;
    rule->new_time_source = new_time_source_function;
    rule->select_packet = select_packet_function;
    rule->neighbor_updated = neighbor_updated_function;
    rule->name = "unicast per neighbor non-storing";
    rule->slotframe_length = ORCHESTRA_UNICAST_PERIOD;
  }
  return rule;
}

void orchestra_init() {
  struct orchestra_rule* rule = create_orchestra_rule();
  if (rule == NULL) {
    return ;
  }

  //register_orchestra_rule(rule);
  rule->init(0);

  free(rule);

}
