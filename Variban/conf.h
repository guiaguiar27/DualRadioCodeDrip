#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "network-graph.h" 
#include "lib/random.h"  
#define DEBUG 1

void DCFL(int num_aresta, int num_no, int (*pacotes)[num_no], int (*graf_conf)[num_aresta][num_aresta], int (*mapa_graf_conf)[num_aresta][2], int raiz, ng *matching, int (*vetor)[num_aresta][2]); 
void geraMaching(int tam, int tam_rede, int (*pacotes)[tam_rede], int (*graf_conf)[tam][tam], int (*mapa_graf_conf)[tam][2], int node, ng *resultado, int (*vetor)[tam][2]);
void selecao(int **conf, int pai, int tam);
void mapGraphConf(ng *mat, int tam_no, int tam_aresta, int (*alocado)[tam_aresta][2]);
void fazMatrizConf(int tam_arest, int (*mapConf)[tam_arest][2], int (*grafoconf)[tam_arest][tam_arest]);    //Tam é nº de arestas do grafo da rede

