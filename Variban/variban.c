
/**  
 * Separated module for variban 
 * more easy to reuse and to just activate and deactivate 
 * The purpose it's to works as Orchestra or others scheduler completly modular 
 * 
**/  


 
#include "variban.h"
  
#define peso 1 
#define no_raiz 1     

#define Channel 16
#define Timeslot 8
static uint16_t unicast_slotframe_handle = 2;

#define endereco "/home/user/contiki-ng/os/arvore.txt"  
#define endereco_T_CH  "/home/user/contiki-ng/os/TCH.txt"



/* Log configuration */
#define LOG_MODULE "TSCH Sched"
#define LOG_LEVEL LOG_LEVEL_MAC 
#define DBUG 1

#define NBR_TSCH 1  

MEMB(link_memb, struct tsch_link, TSCH_SCHEDULE_MAX_LINKS);

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
 /*------------------------------------------------------------------------------------------------------------*/

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

    memb_init(&link_memb);
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

/*------------------------------------------------------------------------------------------------------------*/

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


/*------------------------------------------------------------------------------------------------------------*/

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
                  0, 0);  
                
              }


              else LOG_PRINT("Link already exists!\n "); 
          }
      flag = 0; 
            
      }   
      fclose(fl);  
}  
/*------------------------------------------------------------------------------------------------------------*/

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
/*------------------------------------------------------------------------------------------------------------*/


int setZero_Id_reception(){ 
  for(int i = 0 ; i < MAX_NOS; i++){ 
    PossNeighbor[i] = 0 ;  
  } 
  return 1;  
}  
/*------------------------------------------------------------------------------------------------------------*/


int fill_id(uint8_t id){ 
  for(int i = 0 ; i < MAX_NOS; i++){ 
      if(PossNeighbor[i] == 0){ 
          PossNeighbor[i] = id;  
          return 1;  
      }
  } 
  return 0 ; 
} 
/*------------------------------------------------------------------------------------------------------------*/

int verify_link_by_id(uint8_t id){ 
  for(int i = 0 ; i < MAX_NOS; i++ ){  
  	LOG_PRINT("%d\n",PossNeighbor[i]);
    if(PossNeighbor[i] == id ) return 1; 
  } 
  return 0 ;  
}
/*------------------------------------------------------------------------------------------------------------*/

//#if NBR_TSCH 
  // inicia  
int func_test(int i){ 
  printf("teste"); 
  return 0; 
}

int get_random_node(int n){  
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
 //#endif  
 /*------------------------------------------------------------------------------------------------------------*/


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