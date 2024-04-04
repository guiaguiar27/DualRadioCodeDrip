
/**  
 * Separated module for variban 
 * more easy to reuse and to just activate and deactivate 
 * The purpose it's to works as Orchestra or others scheduler completly modular 
 * 
**/ 

#include "contiki.h"
#include "net/linkaddr.h" 

#include "contiki.h"
#include "dev/leds.h"
#include "lib/memb.h"
#include "net/nbr-table.h" 
#include "net/packetbuf.h"
#include "net/queuebuf.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/framer/frame802154.h"
#include "net/mac/tsch/tsch-schedule.h"
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
#include "sys/log.h"  // don't know about this include

#if NBR_TSCH 
  int NBRlist[MAX_NEIGHBORS]; 
#endif  


void executa(int  num_aresta, int  num_no,  int **aloca_canal, int tempo, int (*mapa_graf_conf)[num_aresta][2], int *pacote_entregue, int raiz, int (*pacotes)[num_no]); 
/*---------------------------------------------------------------------------*/
 void verify_packs();

/*---------------------------------------------------------------------------*/
void alocaPacotes2(uint8_t num_no, ng *adj, int (*vetor)[num_no]);  
/*---------------------------------------------------------------------------*/
int tsch_num_nos(); 
/*---------------------------------------------------------------------------*/
void tsch_write_in_file(int n_origin, int n_destin); 
/*---------------------------------------------------------------------------*/
int count_lines(); 
/*---------------------------------------------------------------------------*/

int SCHEDULE_static(); 
/*---------------------------------------------------------------------------*/

void rx_schedule_intern(struct tsch_link *l);  

/*---------------------------------------------------------------------------*/

void find_neighbor_to_Rx(uint8_t node, int handle);  

/*---------------------------------------------------------------------------*/

int tsch_get_same_link(const linkaddr_t *addr, struct tsch_slotframe *sf);  

/*---------------------------------------------------------------------------*/


int setZero_Id_reception(); 

/*---------------------------------------------------------------------------*/


int fill_id(uint8_t id);  

/*---------------------------------------------------------------------------*/

int verify_link_by_id(uint8_t id);  

/*---------------------------------------------------------------------------*/ 

 int verify_in_topology(int sender, int receiver); 

#if NBR_TSCH 
  int sort_node_to_create_link(int n); 
  void list_init_nbr(void);
  void tsch_print_neighbors(int nbr); 
  void show_nbr();
  int change_slotframe();
 #endif   

/*---------------------------------------------------------------------------*/