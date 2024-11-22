#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "tsch_neighbor.h"
#include "linkaddr.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/tsch-schedule.h"

/* Function declarations */
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
    linkaddr_print(&new->addr);
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
  linkaddr_print(addr);
  printf("\n");
}

void child_removed_function(const linkaddr_t *addr) {
  printf("Child removed: ");
  linkaddr_print(addr);
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
  return rule;
}

extern struct orchestra_rule *orchestra_rules[];
void register_orchestra_rule(struct orchestra_rule *rule) {
  static int rule_count = 0;
  if (rule_count < ORCHESTRA_RULES_COUNT) {
    orchestra_rules[rule_count++] = rule;
  } else {
    fprintf(stderr, "Error: Cannot register more orchestra rules.\n");
  }
}

int main() {
  struct orchestra_rule* rule = create_orchestra_rule();
  if (rule == NULL) {
    return -1;
  }

  register_orchestra_rule(rule);
  rule->init(0);  

  free(rule);
  return 0;
}
