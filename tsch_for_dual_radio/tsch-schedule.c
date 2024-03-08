/*
 * Copyright (c) 2014, SICS Swedish ICT.
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
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         IEEE 802.15.4 TSCH MAC schedule manager.
 * \author
 *         Simon Duquennoy <simonduq@sics.se>
 *         Beshr Al Nahas <beshr@sics.se>
 *         Atis Elsts <atis.elsts@edi.lv>
 */

/**
 * \addtogroup tsch
 * @{
*/

#include "contiki.h"
#include "dev/leds.h"
#include "lib/memb.h"
#include "net/nbr-table.h" 
#include "net/packetbuf.h"
#include "net/queuebuf.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/framer/frame802154.h"
#include "sys/process.h"
#include "sys/rtimer.h" 
#include "net/link-stats.h"
#include <string.h> 
#include <stdio.h> 
#include <stdlib.h>
#include "conf.h"  
#define peso 1 
#define no_raiz 1     

#define Channel 16
#define Timeslot 8
static uint16_t unicast_slotframe_handle = 2;

#define endereco "/home/user/contiki-ng/os/arvore.txt"  
#define endereco_T_CH  "/home/user/contiki-ng/os/TCH.txt"


//uint16_t Rpackets = 0 ; 
uint8_t flag_schedule = 0 ;   
uint32_t Packets_sent[MAX_NOS]; 
uint32_t STpacks = 0 ;  
uint32_t Packets_received[MAX_NOS]; 
uint8_t PossNeighbor[MAX_NOS];  

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "TSCH Sched"
#define LOG_LEVEL LOG_LEVEL_MAC 
#define DBUG 1

/* Pre-allocated space for links */
MEMB(link_memb, struct tsch_link, TSCH_SCHEDULE_MAX_LINKS);
/* Pre-allocated space for slotframes */
MEMB(slotframe_memb, struct tsch_slotframe, TSCH_SCHEDULE_MAX_SLOTFRAMES);
/* List of slotframes (each slotframe holds its own list of links) */
LIST(slotframe_list);  

#if NBR_TSCH 
  int NBRlist[MAX_NEIGHBORS]; 
#endif  

/* Adds and returns a slotframe (NULL if failure) */
struct tsch_slotframe *
tsch_schedule_add_slotframe(uint16_t handle, uint16_t size)
{
  if(size == 0) {
    return NULL;
  }

  if(tsch_schedule_get_slotframe_by_handle(handle)) {
    /* A slotframe with this handle already exists */
    return NULL;
  }

  if(tsch_get_lock()) {
    struct tsch_slotframe *sf = memb_alloc(&slotframe_memb);
    if(sf != NULL) {
      /* Initialize the slotframe */
      sf->handle = handle;
      TSCH_ASN_DIVISOR_INIT(sf->size, size);
      LIST_STRUCT_INIT(sf, links_list);
      /* Add the slotframe to the global list */
      list_add(slotframe_list, sf);
    }
    LOG_INFO("add_slotframe %u %u\n",
           handle, size);
    tsch_release_lock();
    return sf;
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
/* Removes all slotframes, resulting in an empty schedule */
int
tsch_schedule_remove_all_slotframes(void)
{
  struct tsch_slotframe *sf;
  while((sf = list_head(slotframe_list))) {
    if(tsch_schedule_remove_slotframe(sf) == 0) {
      return 0;
    }
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
/* Removes a slotframe Return 1 if success, 0 if failure */
int
tsch_schedule_remove_slotframe(struct tsch_slotframe *slotframe)
{
  if(slotframe != NULL) {
    /* Remove all links belonging to this slotframe */
    struct tsch_link *l;
    while((l = list_head(slotframe->links_list))) {
      tsch_schedule_remove_link(slotframe, l);
    }

    /* Now that the slotframe has no links, remove it. */
    if(tsch_get_lock()) {
      LOG_INFO("remove slotframe %u %u\n", slotframe->handle, slotframe->size.val);
      memb_free(&slotframe_memb, slotframe);
      list_remove(slotframe_list, slotframe);
      tsch_release_lock();
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/* Looks for a slotframe from a handle */
struct tsch_slotframe *
tsch_schedule_get_slotframe_by_handle(uint16_t handle)
{
  if(!tsch_is_locked()) {
    struct tsch_slotframe *sf = list_head(slotframe_list);
    while(sf != NULL) {
      if(sf->handle == handle) {
        return sf;
      }
      sf = list_item_next(sf);
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
/* Looks for a link from a handle */
struct tsch_link *
tsch_schedule_get_link_by_handle(uint16_t handle)
{
  if(!tsch_is_locked()) {
    struct tsch_slotframe *sf = list_head(slotframe_list);
    while(sf != NULL) {
      struct tsch_link *l = list_head(sf->links_list);
      /* Loop over all items. Assume there is max one link per timeslot */
      while(l != NULL) {
        if(l->handle == handle) {
          return l;
        }
        l = list_item_next(l);
      }
      sf = list_item_next(sf);
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
static const char *
print_link_options(uint16_t link_options)
{
  static char buffer[20];
  unsigned length;

  buffer[0] = '\0';
  if(link_options & LINK_OPTION_TX) {
    strcat(buffer, "Tx|");
  }
  if(link_options & LINK_OPTION_RX) {
    strcat(buffer, "Rx|");
  }
  if(link_options & LINK_OPTION_SHARED) {
    strcat(buffer, "Sh|");
  }
  length = strlen(buffer);
  if(length > 0) {
    buffer[length - 1] = '\0';
  }

  return buffer;
}
/*---------------------------------------------------------------------------*/
static const char *
print_link_type(uint16_t link_type)
{
  switch(link_type) {
  case LINK_TYPE_NORMAL:
    return "NORMAL";
  case LINK_TYPE_ADVERTISING:
    return "ADV";
  case LINK_TYPE_ADVERTISING_ONLY:
    return "ADV_ONLY";
  default:
    return "?";
  }
}
/*---------------------------------------------------------------------------*/
/* Adds a link to a slotframe, return a pointer to it (NULL if failure) */
struct tsch_link *
tsch_schedule_add_link(struct tsch_slotframe *slotframe,
                       uint8_t link_options, enum link_type link_type, const linkaddr_t *address,
                       uint16_t timeslot, uint16_t channel_offset, uint8_t do_remove)
{
  struct tsch_link *l = NULL; 
  uint16_t node_neighbor, node;
  if(slotframe != NULL) {
    /* We currently support only one link per timeslot in a given slotframe. */

    /* Validation of specified timeslot and channel_offset */
    if(timeslot > (slotframe->size.val - 1)) {
      LOG_ERR("! add_link invalid timeslot: %u\n", timeslot);
      return NULL;
    }

    if(do_remove) {
      /* Start with removing the link currently installed at this timeslot (needed
       * to keep neighbor state in sync with link options etc.) */
      tsch_schedule_remove_link_by_timeslot(slotframe, timeslot, channel_offset);
    }
   if(!tsch_get_lock()) {
      LOG_ERR("! add_link memb_alloc couldn't take lock\n");
    } else {
      l = memb_alloc(&link_memb);
      if(l == NULL) {
        LOG_ERR("! add_link memb_alloc failed\n");
        tsch_release_lock();
      } else {
        struct tsch_neighbor *n;
        /* Add the link to the slotframe */ 
        //static int count = 0 ;
        list_add(slotframe->links_list, l);
        /* Initialize link */
        //l->handle = count++ ; 
        l->handle = count_lines();
        LOG_PRINT("Handle : %u\n ", l->handle);
        l->link_options = link_options;
        l->link_type = link_type;
        l->slotframe_handle = slotframe->handle;
        l->timeslot = timeslot;
        l->channel_offset = channel_offset;
        l->data = NULL; 
        l-> value = 0 ; 
        if(address == NULL) {
          address = &linkaddr_null;
        }
        linkaddr_copy(&l->addr, address);

        LOG_INFO("add_link sf=%u opt=%s type=%s ts=%u ch=%u addr=",
                 slotframe->handle,
                 print_link_options(link_options),
                 print_link_type(link_type), timeslot, channel_offset);
        LOG_INFO_LLADDR(address);
        LOG_INFO_("\n");
        /* Release the lock before we update the neighbor (will take the lock) */
        tsch_release_lock();
        if(l->link_options & LINK_OPTION_RX){ 
          l->aux_options = 1;  
          l->handle = -2;   
          node_neighbor =  l->addr.u8[LINKADDR_SIZE - 1]
                 + (l->addr.u8[LINKADDR_SIZE - 2] << 8);  
          fill_id(node_neighbor); 

        }
        if(l->link_options & LINK_OPTION_TX) {
          l->aux_options = 2; 
          n = tsch_queue_add_nbr(&l->addr);
          /* We have a tx link to this neighbor, update counters */
          if(n != NULL) {
            n->tx_links_count++;
            if(!(l->link_options & LINK_OPTION_SHARED)) {
              n->dedicated_tx_links_count++; 
              node = linkaddr_node_addr.u8[LINKADDR_SIZE - 1]
                 + (linkaddr_node_addr.u8[LINKADDR_SIZE - 2] << 8);  
              node_neighbor =  l->addr.u8[LINKADDR_SIZE - 1]
                 + (l->addr.u8[LINKADDR_SIZE - 2] << 8);  
              
              tsch_write_in_file(node, node_neighbor);   
               
                          }
          }
        }
      }
    }
  } 

  return l;
} 
/*---------------------------------------------------------------------------*/
/* Removes a link from slotframe. Return 1 if success, 0 if failure */
int
tsch_schedule_remove_link(struct tsch_slotframe *slotframe, struct tsch_link *l)
{
  if(slotframe != NULL && l != NULL && l->slotframe_handle == slotframe->handle) {
    if(tsch_get_lock()) {
      uint8_t link_options;
      linkaddr_t addr;

      /* Save link option and addr in local variables as we need them
       * after freeing the link */
      link_options = l->link_options;
      linkaddr_copy(&addr, &l->addr);

      /* The link to be removed is scheduled as next, set it to NULL
       * to abort the next link operation */
      if(l == current_link) {
        current_link = NULL;
      }
      LOG_INFO("remove_link sf=%u opt=%s type=%s ts=%u ch=%u addr=",
               slotframe->handle,
               print_link_options(l->link_options),
               print_link_type(l->link_type), l->timeslot, l->channel_offset);
      LOG_INFO_LLADDR(&l->addr);
      LOG_INFO_("\n");

      list_remove(slotframe->links_list, l);
      memb_free(&link_memb, l);

      /* Release the lock before we update the neighbor (will take the lock) */
      tsch_release_lock();

      /* This was a tx link to this neighbor, update counters */
      if(link_options & LINK_OPTION_TX) {
        struct tsch_neighbor *n = tsch_queue_get_nbr(&addr);
        if(n != NULL) {
          n->tx_links_count--;
          if(!(link_options & LINK_OPTION_SHARED)) {
            n->dedicated_tx_links_count--;
          }
        }
      }

      return 1;
    } else {
      LOG_ERR("! remove_link memb_alloc couldn't take lock\n");
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/* Removes a link from slotframe and timeslot. Return a 1 if success, 0 if failure */
int
tsch_schedule_remove_link_by_timeslot(struct tsch_slotframe *slotframe,
                                      uint16_t timeslot, uint16_t channel_offset)
{
  int ret = 0;
  if(!tsch_is_locked()) {
    if(slotframe != NULL) {
      struct tsch_link *l = list_head(slotframe->links_list);
      /* Loop over all items and remove all matching links */
      while(l != NULL) {
        struct tsch_link *next = list_item_next(l);
        if(l->timeslot == timeslot && l->channel_offset == channel_offset) {
          if(tsch_schedule_remove_link(slotframe, l)) {
            ret = 1;
          }
        }
        l = next;
      }
    }
  }
  return ret;
}
/*---------------------------------------------------------------------------*/
/* Looks within a slotframe for a link with a given timeslot */
struct tsch_link *
tsch_schedule_get_link_by_timeslot(struct tsch_slotframe *slotframe,
                                   uint16_t timeslot, uint16_t channel_offset)
{
  if(!tsch_is_locked()) {
    if(slotframe != NULL) {
      struct tsch_link *l = list_head(slotframe->links_list);
      /* Loop over all items. Assume there is max one link per timeslot and channel_offset */
      while(l != NULL) {
        if(l->timeslot == timeslot && l->channel_offset == channel_offset) {
          return l;
        }
        l = list_item_next(l);
      }
      return l;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
static struct tsch_link *
default_tsch_link_comparator(struct tsch_link *a, struct tsch_link *b)
{
  if(!(a->link_options & LINK_OPTION_TX)) {
    /* None of the links are Tx: simply return the first link */
    return a;
  }

  /* Two Tx links at the same slotframe; return the one with most packets to send */
  if(!linkaddr_cmp(&a->addr, &b->addr)) {
    struct tsch_neighbor *an = tsch_queue_get_nbr(&a->addr);
    struct tsch_neighbor *bn = tsch_queue_get_nbr(&b->addr);
    int a_packet_count = an ? ringbufindex_elements(&an->tx_ringbuf) : 0;
    int b_packet_count = bn ? ringbufindex_elements(&bn->tx_ringbuf) : 0;
    /* Compare the number of packets in the queue */
    return a_packet_count >= b_packet_count ? a : b;
  }

  /* Same neighbor address; simply return the first link */
  return a;
}

/*---------------------------------------------------------------------------*/
/* Returns the next active link after a given ASN, and a backup link (for the same ASN, with Rx flag) */
struct tsch_link *
tsch_schedule_get_next_active_link(struct tsch_asn_t *asn, uint16_t *time_offset,
    struct tsch_link **backup_link)
{
  uint16_t time_to_curr_best = 0;
  struct tsch_link *curr_best = NULL;
  struct tsch_link *curr_backup = NULL; /* Keep a back link in case the current link
  turns out useless when the time comes. For instance, for a Tx-only link, if there is
  no outgoing packet in queue. In that case, run the backup link instead. The backup link
  must have Rx flag set. */
  if(!tsch_is_locked()) {
    struct tsch_slotframe *sf = list_head(slotframe_list);
    /* For each slotframe, look for the earliest occurring link */
    while(sf != NULL) {
      /* Get timeslot from ASN, given the slotframe length */
      uint16_t timeslot = TSCH_ASN_MOD(*asn, sf->size);
      struct tsch_link *l = list_head(sf->links_list);
      while(l != NULL) {
        uint16_t time_to_timeslot =
          l->timeslot > timeslot ?
          l->timeslot - timeslot :
          sf->size.val + l->timeslot - timeslot;
        if(curr_best == NULL || time_to_timeslot < time_to_curr_best) {
          time_to_curr_best = time_to_timeslot;
          curr_best = l;
          curr_backup = NULL;
        } else if(time_to_timeslot == time_to_curr_best) {
          struct tsch_link *new_best = NULL;
          /* Two links are overlapping, we need to select one of them.
           * By standard: prioritize Tx links first, second by lowest handle */
          if((curr_best->link_options & LINK_OPTION_TX) == (l->link_options & LINK_OPTION_TX)) {
            /* Both or neither links have Tx, select the one with lowest handle */
            if(l->slotframe_handle != curr_best->slotframe_handle) {
              if(l->slotframe_handle < curr_best->slotframe_handle) {
                new_best = l;
              }
            } else {
              /* compare the link against the current best link and return the newly selected one */
              new_best = TSCH_LINK_COMPARATOR(curr_best, l);
            }
          } else {
            /* Select the link that has the Tx option */
            if(l->link_options & LINK_OPTION_TX) {
              new_best = l;
            }
          }

          /* Maintain backup_link */
          /* Check if 'l' best can be used as backup */
          if(new_best != l && (l->link_options & LINK_OPTION_RX)) { /* Does 'l' have Rx flag? */
            if(curr_backup == NULL || l->slotframe_handle < curr_backup->slotframe_handle) {
              curr_backup = l;
            }
          }
          /* Check if curr_best can be used as backup */
          if(new_best != curr_best && (curr_best->link_options & LINK_OPTION_RX)) { /* Does curr_best have Rx flag? */
            if(curr_backup == NULL || curr_best->slotframe_handle < curr_backup->slotframe_handle) {
              curr_backup = curr_best;
            }
          }

          /* Maintain curr_best */
          if(new_best != NULL) {
            curr_best = new_best;
          }
        }

        l = list_item_next(l);
      }
      sf = list_item_next(sf);
    }
    if(time_offset != NULL) {
      *time_offset = time_to_curr_best;
    }
  }
  if(backup_link != NULL) {
    *backup_link = curr_backup;
  }
  return curr_best;
}
/*---------------------------------------------------------------------------*/
/* Module initialization, call only once at startup. Returns 1 is success, 0 if failure. */
int
tsch_schedule_init(void)
{
  if(tsch_get_lock()) {
    memb_init(&link_memb);
    memb_init(&slotframe_memb);
    list_init(slotframe_list);  
    
    #if NBR_TSCH
      list_init_nbr();  
      setZero_Id_reception(); 
    #endif 
    

    tsch_release_lock();
    return 1;
  } else {
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
/* Create a 6TiSCH minimal schedule */
void
tsch_schedule_create_minimal(void)
{
  struct tsch_slotframe *sf_min;
  /* First, empty current schedule */
  tsch_schedule_remove_all_slotframes();
  /* Build 6TiSCH minimal schedule.
   * We pick a slotframe length of TSCH_SCHEDULE_DEFAULT_LENGTH */
  sf_min = tsch_schedule_add_slotframe(0, TSCH_SCHEDULE_DEFAULT_LENGTH);
  /* Add a single Tx|Rx|Shared slot using broadcast address (i.e. usable for unicast and broadcast).
   * We set the link type to advertising, which is not compliant with 6TiSCH minimal schedule
   * but is required according to 802.15.4e if also used for EB transmission.
   * Timeslot: 0, channel offset: 0. */
  tsch_schedule_add_link(sf_min,
      (LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED | LINK_OPTION_TIME_KEEPING),
      LINK_TYPE_ADVERTISING, &tsch_broadcast_address,
      0, 0, 1);
}
/*---------------------------------------------------------------------------*/
struct tsch_slotframe *
tsch_schedule_slotframe_head(void)
{
  return list_head(slotframe_list);
}
/*---------------------------------------------------------------------------*/
struct tsch_slotframe *
tsch_schedule_slotframe_next(struct tsch_slotframe *sf)
{
  return list_item_next(sf);
}
/*---------------------------------------------------------------------------*/
/* Prints out the current schedule (all slotframes and links) */
void
tsch_schedule_print(void)
{
  if(!tsch_is_locked()) {
    struct tsch_slotframe *sf = list_head(slotframe_list);

    LOG_PRINT("----- start slotframe list -----\n");

    while(sf != NULL) {
      struct tsch_link *l = list_head(sf->links_list);

      LOG_PRINT("Slotframe Handle %u, size %u\n", sf->handle, sf->size.val);

      while(l != NULL) {
        LOG_PRINT("* Link Options %02x, type %u, handle: %u, timeslot %u, channel offset %u, address %u\n",
               l->link_options, l->link_type,l->handle, l->timeslot, l->channel_offset, l->addr.u8[7]);
        l = list_item_next(l);
      }

      sf = list_item_next(sf);
    }

    LOG_PRINT("----- end slotframe list -----\n");
  }
}
/*---------------------------------------------------------------------------*/
void executa(int  num_aresta, int  num_no,  int **aloca_canal, int tempo, int (*mapa_graf_conf)[num_aresta][2], int *pacote_entregue, int raiz, int (*pacotes)[num_no]){
    int i;

    for(i = 0; i < Channel; i++){
        if( aloca_canal[i][tempo] == -1)
            continue;
        if((*pacotes)[(*mapa_graf_conf)[aloca_canal[i][tempo]][0]] > 0){
            (*pacotes)[(*mapa_graf_conf)[aloca_canal[i][tempo]][0]] -= peso;
            (*pacotes)[(*mapa_graf_conf)[aloca_canal[i][tempo]][1]] += peso;
        }
        if((*mapa_graf_conf)[aloca_canal[i][tempo]][1] == raiz)
            (*pacote_entregue) += peso;
    }
} 
/*------------------------------------------------------------------------------------------------------------*/

 void verify_packs(){  
   FILE *fl;  
   linkaddr_t addr_src ;
   //, addr_dst; 
   int node_origin, node_destin;  
   fl = fopen(endereco, "r"); 
    if(fl == NULL){
        printf("The file was not opened\n");
        return ; 
    }
    while(!feof(fl)){      
        fscanf(fl,"%d %d",&node_origin, &node_destin);   
        //printf("%d ->  %d\n", node_origin, node_destin);    
        
        if(node_origin <= MAX_NOS && node_destin <= MAX_NOS){
            
            for(int j = 0; j < sizeof(addr_src); j += 2) {
              addr_src.u8[j + 1] = node_origin & 0xff;
              addr_src.u8[j + 0] = node_origin >> 8;
            }    

            struct tsch_neighbor *dst = tsch_queue_get_nbr(&addr_src);
            int a_packet_count = dst ? ringbufindex_elements(&dst->tx_ringbuf) : 0; 
            LOG_INFO_LLADDR(&addr_src);
            LOG_INFO("| Pacotes: %d \n", a_packet_count);
    
        }
    } 
    fclose(fl);
 }

void alocaPacotes2(uint8_t num_no, ng *adj, int (*vetor)[num_no]){
    int x, y, qtd_pacotes = 0;
    //Percorre o vetor de pacotes
    for(x = 1; x < num_no; x++){
        //Percorre a linha da matriz para saber se o nó X está conectado à alguém
        for(y = 1; y < num_no; y++)
            //Se sim, adiciona um pacote
            if(adj->mat_adj[x][y]){
                qtd_pacotes = peso;
                break;
            }

        if(qtd_pacotes)
            (*vetor)[x] = qtd_pacotes;
        else
            (*vetor)[x] = 0;

        //Reseta o contador
        qtd_pacotes = 0;
    }
    
}   

   
/*------------------------------------------------------------------------------------------------------------*/

 // Return the number of nodes defined for this network     
int tsch_num_nos(){ 
  int i = MAX_NOS; 
  return i; 
}  
/*---------------------------------------------------------------------------*/
void tsch_write_in_file(int n_origin, int n_destin){ 
  FILE *file; 
  file = fopen(endereco, "a");
  if(file == NULL){
        printf("The file was not opened\n");
        return ; 
  } 
  fprintf(file, "%d %d\n",n_origin,n_destin);
  fclose(file);
} 
/*---------------------------------------------------------------------------*/
int count_lines() 
{ 
    FILE *fp; 
    int count = 0;    
    char c;  
    fp = fopen(endereco, "r"); 
    if (fp == NULL) return 0; 
    for (c = getc(fp); c != EOF; c = getc(fp)) 
        if (c == '\n') 
            count = count + 1; 
    fclose(fp); 
    return count; 
}      
void count_sent_packs(){ 
    uint32_t node = linkaddr_node_addr.u8[LINKADDR_SIZE -1] 
              + (linkaddr_node_addr.u8[LINKADDR_SIZE -2 ] << 8);  
    if(flag_schedule == 1){ 
      STpacks +=1 ; 
    } 
    LOG_INFO("Tx_try %u %u \n",node,STpacks); 
} 

void count_packs( const linkaddr_t *address ){  
  // receive only  
  LOG_INFO("list of packets\n");
  uint8_t  node_src = (*address).u8[LINKADDR_SIZE - 1]
                + ((*address).u8[LINKADDR_SIZE - 2] << 8);   
  uint8_t node = linkaddr_node_addr.u8[LINKADDR_SIZE -1] 
              + (linkaddr_node_addr.u8[LINKADDR_SIZE -2 ] << 8);  

  if(flag_schedule){      
    //if(verify_in_topology(node_src, node)){
      Packets_sent[node_src] += 1 ;  
      Packets_received[node] += 1 ;  
    //}
    for(int i = 1 ; i < MAX_NOS; i++){ 
      LOG_INFO("Pckt %u %u %u \n",i,Packets_sent[i], Packets_received[i]);
    } 
  } 
}  
/*---------------------------------------------------------------------------*/

int SCHEDULE_static(){  
    int  tamNo; 
    int  verify = 0 ;  
    //int **adj = (int**)malloc(MAX_NOS * sizeof(int*));                  //grafo da rede
    ng adj;
    //uint16_t timeslot, slotframe, channel_offset; 
    int  tamAresta,                  //Nº de arestas da rede
    z, i,j ;                       //Variáveis temporárias
    int pacote_entregue = 0, 
    total_pacotes = 0, 
    raiz ;                  
    int  cont = 0;               //Time do slotframe
    int x, y, canal = 0,            //Variáveis temporárias
    edge_selected, temp;        //Variáveis temporárias
    int node_origin, node_destin ;    
    uint8_t channel_bandwidth = 0 ; 
 
    /*******************************************************************/ 
    FILE *fl;     
    struct tsch_slotframe *sf = tsch_schedule_get_slotframe_by_handle(unicast_slotframe_handle);  
    
    #ifdef DEBUG_SCHEDULE_STATIC 
      LOG_PRINT("-----Slotframe handle:%d----\n", sf->handle);  
    #endif 
    
    if(tsch_get_lock()){  
    
    int  **aloca_canais = (int**)malloc(Channel * sizeof(int*));
    for(x = 0; x < Channel; x++){
         aloca_canais[x] = (int*)malloc(Timeslot * sizeof(int));

     }
    tamNo = MAX_NOS;  
    tamAresta = 0 ;    
    fl = fopen(endereco, "r"); 
    if(fl == NULL){
        printf("The file was not opened\n");
        return 0 ; 
    } 
    // matriz  

    for( i = 0 ; i < tamNo; i++){ 
        for( j = 0 ; j< tamNo; j++){  
            adj.mat_adj[i][j] = 0 ; 
        }
    }  

    i = 0;
    printf("Enter here!\n");
    while(!feof(fl)){      
        fscanf(fl,"%d %d",&node_origin, &node_destin);   
        if(node_origin <= MAX_NOS && node_destin <= MAX_NOS){
            if (adj.mat_adj[node_origin][node_destin] == 0 && node_origin != no_raiz){ 
                printf("handle: %d - %d-> %d\n",i, node_origin, node_destin);   
                adj.mat_adj[node_origin][node_destin] = 1;
                i++; 

            }
        }
    }
    tamAresta = i;   
    printf("Numero de nós : %d | Numero de arestas: %d", tamNo, tamAresta);
    fclose(fl);
    #ifdef DEBUG_SCHEDULE_STATIC 
      printf("\nMatriz de adacência do grafo da rede:\n");
      for(i = 1; i < tamNo; i++){ 
          for( j = 1 ;j < tamNo ; j++)
              printf("%d ", adj.mat_adj[i][j]);
          printf("\n");
      } 
    #endif
    
    int pacotes[tamNo];               //Pacotes por nó no grafo da rede
    alocaPacotes2(tamNo, &adj, &pacotes);
    printf("\nPacotes atribuidos!\n");
    //Mapeia os nós do grafo de conflito para os respectivos nós do grafo da rede
    #ifdef DEBUG_SCHEDULE_STATIC 
      for(x = 0; x < tamNo ; x++)
          printf("Nó %d: %d pacotes\n", x, pacotes[x]);
    #endif

    int conf[tamAresta][2];
    mapGraphConf(&adj, tamNo, tamAresta, &conf);
    
    #ifdef DEBUG_SCHEDULE_STATIC 
      printf("\nMapa da matriz de conflito gerada:\n"); 
      for(x = 0; x < tamAresta ; x++)
          printf("Nó %d: %d -> %d\n", x, conf[x][0], conf[x][1]);
    #endif
    
    //Gera a matriz de conflito
    int matconf[tamAresta][tamAresta];
    fazMatrizConf(tamAresta, &conf, &matconf);

    for(x = 0; x < Channel; x++){
         for(y = 0; y < Timeslot; y++)
             aloca_canais[x][y] = -1; 
      }
    //Busca pelo nó raiz da rede
    
    //Por hora definimos ele manualmente
    raiz = no_raiz;

    //Guarda o total de pacotes a serem enviados pela
    for(z = 1; z < tamNo; z++)
        if(z != raiz)
            total_pacotes += pacotes[z];

    
    // otimizar a criação de matrizes 
    int vetor[tamAresta][2]; 
    for(x = 0 ; x < tamAresta; x++) 
      for(y = 0; y < 2; y++ ) 
        vetor[x][y] = 0 ; 

    DCFL(tamAresta, tamNo, &pacotes, &matconf, &conf, raiz, &adj, &vetor);
    
    while(pacote_entregue < total_pacotes){

        for(x = 1 ; x < tamNo; x ++){
            for(y = 1; y < tamNo; y++){
                if(adj.mat_adj[x][y]){
                    for(temp = 0; temp < tamAresta; temp++)
                        if(conf[temp][0] == x && conf[temp][1] == y)
                            break;  
                    edge_selected = temp;
                    
                    for(temp = 0; temp < pacotes[conf[edge_selected][0]]; temp++){
                            if(canal == 8)
                              break;   
                            
                            aloca_canais[canal][cont] = edge_selected;     
                            canal++;   
                    }   
                }
                if(canal == Channel)
                    break;
            }
            if(canal == Channel)
                break;
        }
        if(cont == Timeslot) cont = 0;
        executa(tamAresta, tamNo, aloca_canais, cont, &conf, &pacote_entregue, raiz, &pacotes); 
        cont++;
        canal = 0; 
        for(x = 0 ; x < tamAresta; x++) 
          for(y = 0; y < 2; y++ ) 
            vetor[x][y] = 0 ; 
        DCFL(tamAresta, tamNo, &pacotes, &matconf, &conf, raiz, &adj,&vetor);
    
    }
    #ifdef DEBUG_SCHEDULE_STATIC 
      printf("\nCanais alocados  | |");
      printf("\n                \\   /");
      printf("\n                 \\ /\n\n");
      
      for(x = 0 ; x < Channel; x++){
          for(y = 0; y < Timeslot; y++) 
              // linhas = tempo - coluna = canal  
              printf("%d  ", aloca_canais[x][y]);  
              
          printf("\n"); 
      } 
    #endif  
     
    
      
    struct tsch_link *l =   NULL; 
    struct tsch_link *l_aux = NULL;  
    l_aux = memb_alloc(&link_memb);  
    l = memb_alloc(&link_memb); 
    for(x = 0 ; x < Channel; x++){ 
    for(y = 0 ; y < Timeslot; y++){   
        l = list_head(sf->links_list);        
        while(l!= NULL){   
          if(aloca_canais[x][y]  == l->handle && l->link_type == LINK_TYPE_NORMAL){  

            LOG_PRINT("----HANDLE: %u-----\n", l->handle); 
           
            
            if(verify == 0){ 
            LOG_PRINT("---------------------------\n"); 
            LOG_PRINT("----HANDLE: %u-----\n", l->handle); 
            LOG_PRINT("----TIMESLOT: %u-----\n", l->timeslot); 
            LOG_PRINT("----CHANNEL: %u-----\n\n", l->channel_offset);     
            // indica que é de TX 
            if(l->aux_options == 2){  
              // pesquisa se já tem algum link no timeslot, caso haja a verify continua desmarcada para nova adequação  
              l_aux = tsch_schedule_get_link_by_timeslot(sf,y+1,x+1); 
              if(l_aux != NULL){  
                  verify = 0 ; 
              } 
              else{
              l-> timeslot = y+1; 
              l-> channel_offset = x+1 ; 
              channel_bandwidth = 1 ;    
              
              
              LOG_PRINT("----CHANGE-Tx----\n"); 
              LOG_PRINT("----TIMESLOT: %u-----\n", l->timeslot); 
              LOG_PRINT("----CHANNEL: %u-----\n", l->channel_offset); 
              LOG_PRINT("-----------------------------\n\n");       
              
              node_origin = linkaddr_node_addr.u8[LINKADDR_SIZE -1] 
                      + (linkaddr_node_addr.u8[LINKADDR_SIZE -2 ] << 8);  
              node_destin =  l->addr.u8[LINKADDR_SIZE - 1]
                      + (l->addr.u8[LINKADDR_SIZE - 2] << 8);  
              
              fl = fopen(endereco_T_CH, "a"); 
              if(fl == NULL) 
                break;    

              fprintf(fl, "%d %d (%d %d)\n",node_origin,node_destin,l->timeslot, l->channel_offset);
              fclose(fl);
              
              verify = 1 ;  
              } 
            } 
              
          }
          else{   
            
            if(x != l->channel_offset){ 
              
              node_origin = linkaddr_node_addr.u8[LINKADDR_SIZE -1] 
                      + (linkaddr_node_addr.u8[LINKADDR_SIZE -2] << 8);   
              channel_bandwidth +=1;    
              
            }

          }
           
        } // 1st if 
        l = list_item_next(l);
      } // while      
    } // internal for 
  } // external for 

  // schedule Rx links (with the same parameters of TX) 
  l = list_head(sf->links_list); 
  while(l!= NULL){  
    if(l->aux_options == 1  && l->link_type == LINK_TYPE_NORMAL)
      rx_schedule_intern(l);
        
      l = list_item_next(l);

  }

  flag_schedule = 1 ; // allow count packets 
  LOG_PRINT("increased_Bandwidth %d %u\n",node_origin, channel_bandwidth);  
  LOG_PRINT("Escalonamento Concluido\n");  
  tsch_release_lock();   
  } 
  return 1;
}      


void  
rx_schedule_intern(struct tsch_link *l){  
  FILE *fl;  
  fl = fopen(endereco_T_CH, "r");   
  int node_origin, nbr, aux_timeslot, aux_channel_offset;
  uint8_t node_destin = linkaddr_node_addr.u8[LINKADDR_SIZE -1] 
                      + (linkaddr_node_addr.u8[LINKADDR_SIZE -2 ] << 8);    
  uint8_t node = l->addr.u8[LINKADDR_SIZE -1] 
                      + (l->addr.u8[LINKADDR_SIZE -2 ] << 8);
  while(!feof(fl)){ 
                fscanf(fl, "%d %d (%d %d)",&node_origin,&nbr,&aux_timeslot, &aux_channel_offset); 
                if(node_origin == node && nbr == node_destin){    
                  l->timeslot = aux_timeslot; 
                  l->channel_offset = aux_channel_offset;  
                  #ifdef DEBUG_SCHEDULE_STATIC 
                    LOG_PRINT("----CHANGE-Rx----\n"); 
                    LOG_PRINT("----TIMESLOT: %u-----\n", l->timeslot); 
                    LOG_PRINT("----CHANNEL: %u-----\n", l->channel_offset); 
                    LOG_PRINT("-----------------------------\n\n");    
                  #endif      
                } 
              }
    fclose(fl);
  
}



void find_neighbor_to_Rx(uint8_t node, int handle){  
    struct tsch_slotframe *sf = tsch_schedule_add_slotframe(unicast_slotframe_handle, SLOTFRAME_SIZE); 
    if(sf == NULL){ 
      sf = tsch_schedule_get_slotframe_by_handle(unicast_slotframe_handle);
    }  
    LOG_PRINT("-----Slotframe handle:%d----\n", sf->handle);  
    linkaddr_t addr;    
    int node_origin, node_destin;
    FILE *fl;  
    uint8_t flag = 0 ;  
    #ifdef DEBUG_SCHEDULE_STATIC 
      LOG_PRINT("Finding neighbor to Rx\n");
    #endif // DEBUG
      fl = fopen(endereco, "r"); 
      if(fl == NULL){
          LOG_PRINT("The file was not opened\n");
          return  ; 
      }  

      while(!feof(fl)){       
          fscanf(fl,"%d %d",&node_origin, &node_destin);   
          LOG_PRINT("%d %d\n",node_origin, node_destin);
          if(node_destin == node){
              #ifdef DEBUG_SCHEDULE_STATIC 
                LOG_PRINT("Match - %u <- %d\n",node,node_origin);              
              #endif // DEBUG
              
              for(int j = 0; j < sizeof(addr); j += 2) {
                addr.u8[j + 1] = node_origin & 0xff;
                addr.u8[j + 0] = node_origin >> 8;
              }   
              
              if(verify_link_by_id(node_origin) == 1) flag = 1;  
              LOG_PRINT("FLAG= %d\n",flag);
              if(flag == 0){
                tsch_schedule_add_link(sf,
                  LINK_OPTION_RX,
                  LINK_TYPE_NORMAL, &addr,
                  0, 0,0);  
                
              }


              else LOG_PRINT("Link already exists!\n "); 
          }
      flag = 0; 
            
      }   
      fclose(fl);  
} 
int tsch_get_same_link(const linkaddr_t *addr, struct tsch_slotframe *sf){ 
  if(!tsch_is_locked()) {
    struct tsch_link *l = list_head(sf->links_list);
    while(l != NULL) {
        if(linkaddr_cmp(addr,&l->addr))  
          return 1; 
        l = list_item_next(l);
      }
      sf = list_item_next(sf);
    } 
    return 0 ; 
  
}    

int setZero_Id_reception(){ 
  for(int i = 0 ; i < MAX_NOS; i++){ 
    PossNeighbor[i] = 0 ;  
  } 
  return 1;  
} 

int fill_id(uint8_t id){ 
  for(int i = 0 ; i < MAX_NOS; i++){ 
      if(PossNeighbor[i] == 0){ 
          PossNeighbor[i] = id;  
          return 1;  
      }
  } 
  return 0 ; 
} 

int verify_link_by_id(uint8_t id){ 
  for(int i = 0 ; i < MAX_NOS; i++ ){  
  	LOG_PRINT("%d\n",PossNeighbor[i]);
    if(PossNeighbor[i] == id ) return 1; 
  } 
  return 0 ;  
}

#if NBR_TSCH 
  // inicia  


int sort_node_to_create_link(int n){  
  for(int i = 0 ; i < MAX_NEIGHBORS ; i++){  

    if(n > NBRlist[i] && NBRlist[i] != 0){ 
      return NBRlist[i]; 
    }  
    } 
    return 0; 
 } 


  void list_init_nbr(void){ 
      for(int i = 0 ; i < MAX_NEIGHBORS;i++) NBRlist[i] = 0;
  } 
  // preenche  
  // implementei ontem 
  void tsch_print_neighbors(int nbr){   
    int count = 0 ;
    
    for(count = 0; count < MAX_NEIGHBORS ; count++ ){ 
        if(NBRlist[count] == nbr) return ; 
        
        if(NBRlist[count] == 0 ){  
          NBRlist[count] = nbr; 
          break; 
        }   
    }

  }
  // imprime
  void show_nbr(){   

    LOG_INFO("Lista de vizinhos que mandaram mensagem:\n");
    for(int i = 0 ; i < MAX_NEIGHBORS; i++) LOG_INFO("%d\n",NBRlist[i]);
    
  } 

  int change_slotframe(){   
    // verifica se tem algum nó na lista  
    // soma da > 0 
    static int counter = 0;
    
    for(int i = 0 ; i < MAX_NEIGHBORS; i++)  
      counter += NBRlist[i]; 
    
    if (counter > 0) return 1 ; 
    
    else return 0 ;  
  } 
 #endif  
 

 // not used 
 int verify_in_topology(int sender, int receiver){ 
    int node_origin, node_destin; 
    FILE *fl;   
    fl = fopen(endereco_T_CH, "r");    
    if(fl == NULL) return 0; 
    while(!feof(fl)){       
          fscanf(fl,"%d %d",&node_origin, &node_destin);   
          if(node_origin == sender && node_destin == receiver){
              return 1 ;
      } 
    }
      fclose(fl); 
      return 0 ;    
 } 





/** @} */
