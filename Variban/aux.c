
#define endereco "/home/user/contiki-ng/os/arvore.txt"  
#define endereco_T_CH  "/home/user/contiki-ng/os/TCH.txt"
 

// n = tsch_queue_add_nbr(&l->addr);
          /* We have a tx link to this neighbor, update counters */
          if(n != NULL) {
            n->tx_links_count++;
            if(!(l->link_options & LINK_OPTION_SHARED)) {
              n->dedicated_tx_links_count++; 
              node = linkaddr_node_addr.u8[LINKADDR_SIZE - 1]
                 + (linkaddr_node_addr.u8[LINKADDR_SIZE - 2] << 8);  
              node_neighbor =  l->addr.u8[LINKADDR_SIZE - 1]
                 + (l->addr.u8[LINKADDR_SIZE - 2] << 8);  
// adicionado no tsch add_linkstsch_write_in_file(node, node_neighbor);    

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



// funcao leitura 
int find_y_for_x(const char *filename, int x) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    printf("Erro ao abrir o arquivo %s\n", filename);
    return -1;
  }

  int x_read, y_read;
  while (fscanf(file, "%d %d", &x_read, &y_read) != EOF) {
    if (x_read == x) {
      fclose(file);
      return y_read;
    }
  }

  fclose(file);
  return -1; // Retorna -1 se não encontrar o valor
}

// - cria dois canais de broadcast 
// - faz rapida descoberta de vizinhos via time source 
