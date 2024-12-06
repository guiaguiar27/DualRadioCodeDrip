 /*
 * Copyright (c) 2015, Swedish Institute of Computer Science.
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
 *
 */

/**
 * \file
 *         Orchestra: an autonomous scheduler for TSCH exploiting RPL state.
 *         See "Orchestra: Robust Mesh Networks Through Autonomously Scheduled TSCH", ACM SenSys'15
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

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


/* The current RPL preferred parent's link-layer address */
linkaddr_t orchestra_parent_linkaddr;
/* Set to one only after getting an ACK for a DAO sent to our preferred parent */
int orchestra_parent_knows_us = 0;

/* The set of Orchestra rules in use */
// const struct orchestra_rule *all_rules[] = ORCHESTRA_RULES;
// #define NUM_RULES (sizeof(all_rules) / sizeof(struct orchestra_rule *))

void init_function(uint16_t slotframe_handle) {
  struct tsch_slotframe *sf = tsch_schedule_add_slotframe(slotframe_handle, 10); // Example period
  if (sf == NULL) {
    fprintf(stderr, "Error: Failed to add slotframe.\n");
    return;
  }
  tsch_schedule_add_link(sf, LINK_OPTION_TX | LINK_OPTION_RX, LINK_TYPE_NORMAL, &tsch_broadcast_address, 1, 0);
}

void new_time_source_function(const struct tsch_neighbor *old, const struct tsch_neighbor *new) {
  if (new != NULL) {
    printf("New time source: ");
    //linkaddr_print(&new->addr);
    printf("\n");
  } else {
    printf("Time source removed.\n");
  }
}

// pode ser alterado
int select_packet_function(uint16_t *slotframe, uint16_t *timeslot) {
  *slotframe = 0;
  *timeslot = 1;
  return 1;
}

void child_added_function(const linkaddr_t *addr) {
  printf("Child added: ");
  //linkaddr_print(addr);
  printf("\n");
}

void child_removed_function(const linkaddr_t *addr) {
  printf("Child removed: ");
  //linkaddr_print(addr);
  printf("\n");
}

/* Structure initialization */
struct orchestra_rule* create_orchestra_rule() {
  struct orchestra_rule* rule = malloc(sizeof(struct orchestra_rule));
  if (!rule) {
    fprintf(stderr, "Error: Failed to allocate memory for orchestra_rule.\n");
    return NULL;
  }
  rule->init = init_function;
  rule->new_time_source = new_time_source_function;
  rule->select_packet = select_packet_function;
  rule->child_added = child_added_function;
  rule->child_removed = child_removed_function;
  printf("Sucess in the allocation\n");
  return rule;
}

// extern struct orchestra_rule *orchestra_rules[];
// void register_orchestra_rule(struct orchestra_rule *rule) {
//   static int rule_count = 0;
//   if (rule_count < 3) {
//     orchestra_rules[rule_count++] = rule;
//   } else {
//     fprintf(stderr, "Error: Cannot register more orchestra rules.\n");
//   }
// }

void orchestra_init() {
  struct orchestra_rule* rule = create_orchestra_rule();
  if (rule == NULL) {
    return ;
  }

  //register_orchestra_rule(rule);
  rule->init(0);

  free(rule);

}
