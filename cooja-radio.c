/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contiki.h"

#include "sys/cooja_mt.h"
#include "lib/simEnvChange.h"

#include "net/packetbuf.h"
#include "net/netstack.h"

#include "dev/radio.h"
#include "dev/cooja-radio.h"

#define COOJA_RADIO_BUFSIZE PACKETBUF_SIZE
#define CCA_SS_THRESHOLD -95

const struct simInterface radio_interface;
const struct simInterface dummy_interface;

/* COOJA */
char simReceiving = 0;
char simInDataBuffer[COOJA_RADIO_BUFSIZE];
int simInSize = 0;
rtimer_clock_t simLastPacketTimestamp = 0;
char simOutDataBuffer[COOJA_RADIO_BUFSIZE];
int simOutSize = 0;
char simRadioHWOn = 1;
int simSignalStrength = -100;
int simLastSignalStrength = -100;
char simPower = 100;
int simRadioChannel = 26;
int simLQI = 105;
/* COOJA */

/*Dummy Radio*/
char simReceivingDummy = 0;
char simInDataBufferDummy[COOJA_RADIO_BUFSIZE]; // size of buffer - packet 
int simInSizeDummy = 0;
rtimer_clock_t simLastPacketTimestampDummy = 0;
char simOutDataBufferDummy[COOJA_RADIO_BUFSIZE];
int simOutSizeDummy = 0;
char simRadioHWOnDummy = 1;
int simSignalStrengthDummy = -100;
int simLastSignalStrengthDummy = -100;
char simPowerDummy = 100;
int simRadioChannelDummy = 1;
int simLQIDummy = 105;
/*Dummy Radio*/
static const void *pending_data; 
static const void *pending_data2;
//static int seqnr=0;
/* If we are in the polling mode, poll_mode is 1; otherwise 0 */
static int poll_mode = 0; /* default 0, disabled */
static int auto_ack = 0; /* AUTO_ACK is not supported; always 0 */
static int addr_filter = 0; /* ADDRESS_FILTER is not supported; always 0 */
static int send_on_cca = (COOJA_TRANSMIT_ON_CCA != 0);

static int radio1=0;
static int radio2=0;
static int dual=0;




PROCESS(cooja_radio_process1, "cooja radio process");
/*---------------------------------------------------------------------------*/
static void
set_send_on_cca(uint8_t enable)
{
  send_on_cca = enable;
}
/*---------------------------------------------------------------------------*/
static void
set_frame_filtering(int enable)
{
  addr_filter = enable;
}
/*---------------------------------------------------------------------------*/
static void
set_auto_ack(int enable)
{
  auto_ack = enable;
}
/*---------------------------------------------------------------------------*/
static void
set_poll_mode(int enable)
{
  poll_mode = enable;
}
/*---------------------------------------------------------------------------*/
void
radio_set_channel(int channel)
{
  simRadioChannel = channel; 
  
}
void
radio_set_channelDummy(int channel)
{
  simRadioChannelDummy = channel; 
 
}

/*---------------------------------------------------------------------------*/
void
radio_set_txpower(unsigned char power)
{
  /* 1 - 100: Number indicating output power */
  simPower = power;
  simPowerDummy = power;
}
/*---------------------------------------------------------------------------*/
int
radio_signal_strength_last(void)
{
  return simLastSignalStrength;
}
/*---------------------------------------------------------------------------*/
int
radio_signal_strength_current(void)
{
  return simSignalStrength;
}
/*---------------------------------------------------------------------------*/
int
radio_LQI(void)
{
  return simLQI;
}
/*---------------------------------------------------------------------------*/
static int
radio_on(void)
{
  simRadioHWOn = 1;
  simRadioHWOnDummy = 1;
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
radio_off(void)
{
  simRadioHWOn = 0;
  simRadioHWOnDummy = 0;
  return 1;
}
/*---------------------------------------------------------------------------*/
static void
doInterfaceActionsBeforeTick(void)
{
  if(!simRadioHWOn && !simRadioHWOnDummy) {
    simInSize = 0;
    simInSizeDummy = 0;
    return;
  }

  if(simReceiving && simReceivingDummy)
  {
     simLastSignalStrength = simSignalStrength;
     simLastSignalStrengthDummy = simSignalStrengthDummy;
     return;
  }
  else if(simReceiving && !simReceivingDummy)
   {
     simLastSignalStrength = simSignalStrength;  
   }
  else
   {
     simLastSignalStrengthDummy = simSignalStrengthDummy;  
   }
  if(simInSize > 0 || simInSizeDummy > 0) 
  {  
       process_poll(&cooja_radio_process1);
  }
}
/*---------------------------------------------------------------------------*/
static void
doInterfaceActionsAfterTick(void)
{ 

}
/*---------------------------------------------------------------------------*/
static int
radio_read(void *buf, unsigned short bufsize)
{ 
 
printf("[Read] Start\n");
 int tmp = simInSize;
 int tmp1 = simInSizeDummy;
 if(simInSize <0 && simInSizeDummy <0) {
    return 0;
  }
  
  if(bufsize < simInSize && bufsize < simInSizeDummy) {
    simInSize = 0; /* rx flush */
    simInSizeDummy = 0; /* rx flush */
    return 0;
  }     
  else if(bufsize < simInSize) {
    simInSize = 0; /* rx flush */
  }
  else if(bufsize < simInSizeDummy) {
    simInSizeDummy = 0; /* rx flush */
  }        

  if(simInDataBuffer[2]==simInDataBufferDummy[2])
  {
	  if(simInSize==103)
	  {
    	  	dual++;
	  	printf("Radio1 = %i Radio2 = %i Dual = %i\n",radio1,radio2,dual);
  	  }
	  memcpy(buf, simInDataBuffer, simInSize);
	  simInSize = 0;
          simInSizeDummy = 0;
	  if(!poll_mode) {
	    packetbuf_set_attr(PACKETBUF_ATTR_RSSI, simSignalStrength);
	    packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, simLQI);
	  }

		  return tmp;
} 
  else if(simInSize>0)
 {	
          if(simInSize==103)
	  {
         	 radio1++;
         	 printf("Radio1 = %i Radio2 = %i Dual = %i\n",radio1,radio2,dual);
	  }
          memcpy(buf, simInDataBuffer, simInSize);
	  simInSize = 0;
          simInSizeDummy = 0;
	  if(!poll_mode) {
	    packetbuf_set_attr(PACKETBUF_ATTR_RSSI, simSignalStrength);
	    packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, simLQI);
	  }

		  return tmp;
} 
 else
 {
	  if(simInSizeDummy==103)
	  {
	  	radio2++;
          	printf("Radio1 = %i Radio2 = %i Dual = %i\n",radio1,radio2,dual);
          }
	  simLastPacketTimestamp = simLastPacketTimestampDummy;
	  memcpy(buf, simInDataBufferDummy, simInSizeDummy);
	  simInSize = 0;
          simInSizeDummy = 0;
	  if(!poll_mode) {
	    packetbuf_set_attr(PACKETBUF_ATTR_RSSI, simSignalStrengthDummy);
	    packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, simLQIDummy);
	  }

		  return tmp1;
}  
} 
static int
radio_read_dual(void *buf, unsigned short bufsize, void *buf2, unsigned short bufsize2)
{  
  printf("[Read dual] Start\n"); 
  int tmp = simInSize;
  int tmp1 = simInSizeDummy;
  if(simInSize <0 && simInSizeDummy <0) { 

    printf("Flush\n");
    return 0;
  }
  
  if(bufsize < simInSize && bufsize < simInSizeDummy) {
    simInSize = 0; /* rx flush */
    simInSizeDummy = 0; /* rx flush */ 
    printf("Flush\n");
    return 0;
  }     
  else if(bufsize < simInSize) { 
    printf("No flush\n");
    simInSize = 0; /* rx flush */ 
  }
  else if(bufsize < simInSizeDummy) {
    printf("No flush\n");
    simInSizeDummy = 0; /* rx flush */
  }          
  if(bufsize2 < simInSize && bufsize2 < simInSizeDummy) {
    simInSize = 0; /* rx flush */
    simInSizeDummy = 0; /* rx flush */ 
    printf("Flush\n");
    return 0;
  }     
  else if(bufsize2 < simInSize){ 
    printf("No flush\n");
    simInSize = 0; /* rx flush */
  }
  else if(bufsize2 < simInSizeDummy){ 
    printf("No flush\n");
    simInSizeDummy = 0; /* rx flush */
  } 


  if(simInDataBuffer[2]==simInDataBufferDummy[2])
  {
	  
    memcpy(buf, simInDataBuffer, simInSize);  
    memcpy(buf2,simInDataBufferDummy, simInSizeDummy); 
	  simInSize = 0;
          simInSizeDummy = 0;
	  if(!poll_mode) {
	    packetbuf_set_attr(PACKETBUF_ATTR_RSSI, simSignalStrength);
	    packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, simLQI);
	  }

		  return tmp;
} 
  else if(simInSize>0)
 {	
    memcpy(buf, simInDataBuffer, simInSize);
	  simInSize = 0;
    simInSizeDummy = 0;
	  if(!poll_mode) {
	    packetbuf_set_attr(PACKETBUF_ATTR_RSSI, simSignalStrength);
	    packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, simLQI);
	  }

		  return tmp;
} 
 else
 {
	  simLastPacketTimestamp = simLastPacketTimestampDummy;
	  memcpy(buf2, simInDataBufferDummy, simInSizeDummy);
	  simInSize = 0;
          simInSizeDummy = 0;
	  if(!poll_mode) {
	    packetbuf_set_attr(PACKETBUF_ATTR_RSSI, simSignalStrengthDummy);
	    packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, simLQIDummy);
	  }

		  return tmp1;
} 
   
}
/*---------------------------------------------------------------------------*/
static int
channel_clear(void)
{
  if(simSignalStrength > CCA_SS_THRESHOLD) {
    return 0;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
radio_send(const void *payload, unsigned short payload_len, const void *payload_2, unsigned short payload_len_2)
{

  printf("[Send_packet] Pckt1: %p - Pckt2: %p\n",payload, payload_2);
  int radiostate = simRadioHWOn;
  simRadioHWOnDummy = simRadioHWOn;
  /* Simulate turnaround time of 2ms for packets, 1ms for acks*/
#if COOJA_SIMULATE_TURNAROUND
  simProcessRunValue = 1;
  cooja_mt_yield();
  if(payload_len > 3) {
    simProcessRunValue = 1;
    cooja_mt_yield();
  }
#endif /* COOJA_SIMULATE_TURNAROUND */

  if(!simRadioHWOn) {
    /* Turn on radio temporarily */
    simRadioHWOn = 1;
  }
  if(!simRadioHWOnDummy) {
    /* Turn on radio temporarily */
    simRadioHWOnDummy = 1;
  }

  if(payload_len > COOJA_RADIO_BUFSIZE && payload_len_2 > COOJA_RADIO_BUFSIZE) {
    return RADIO_TX_ERR;
  }
  if(payload_len == 0 && payload_len_2 == 0){
    return RADIO_TX_ERR;
  }
  if(simOutSize > 0 && simOutSizeDummy > 0) {
    return RADIO_TX_ERR;
  }

  /* Transmit on CCA */
#if COOJA_TRANSMIT_ON_CCA
  if(send_on_cca && !channel_clear()) {
    return RADIO_TX_COLLISION;
  }
#endif /* COOJA_TRANSMIT_ON_CCA */ 

       // the copy of the packets are done here 
        
        memcpy(simOutDataBuffer, payload, payload_len);
        memcpy(simOutDataBufferDummy, payload_2, payload_len_2);
        simOutSize = payload_len;
        simOutSizeDummy = payload_len_2;
	  while(simOutSize > 0 && simOutSizeDummy > 0)
	  {
	    cooja_mt_yield();
       
	  }
         simRadioHWOnDummy = radiostate;
         simRadioHWOn = radiostate;
         return RADIO_TX_OK;
       
}
/*---------------------------------------------------------------------------*/

// modificações necessária aqui 
// está para um único radio 
// definir como separar essas funções  
// radio 1: 
//  - prepare 
//  - transmit 
//  - receiving 
// radio 2: 
//  - prepare 
//  - transmit 
//  - receiving
static int
prepare_packet(const void *data, unsigned short len, const void *data2, unsigned short len2)
{ 
  pending_data = data; 
  pending_data2 = data2; 
  printf("[prepare_packet] data1:%p - data2: %p \n",pending_data, pending_data2); 

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
transmit_packet(unsigned short len, unsigned short len_2)
{  
  printf("[transmit_packet] Pck1: %p - Pck2: %p \n", len, len_2);
  int ret = RADIO_TX_ERR; 
  if(pending_data != NULL) {
    ret = radio_send(pending_data, len, pending_data2, len_2);
  }
  return ret;
}
/*---------------------------------------------------------------------------*/
static int
receiving_packet(void)
{ 
  // verificacao dos radios 
  if(simReceiving == 1 || simReceivingDummy == 1 ){ 
    if(simReceiving == 1 && simReceivingDummy == 1 ) 
      printf("[Receiving_packet] Radio1: On and Radio2: On \n");
    else if(simReceiving == 1 )
      printf("[Receiving_packet] Radio1: On\n");     
    else if(simReceivingDummy == 1) 
      printf("[Receiving_packet] Radio2: On\n");
      
      return 1;  
    }
  else return 0;
 
}
/*---------------------------------------------------------------------------*/
static int
pending_packet(void)
{ 
  // só vai retornar falso quando ambos não estiverem disponíveis  
  // o ideal seria que só funcionasse operação quando os dois radios pudessem enviar ou apenas 1? 

 if ((!simReceiving && simInSize > 0) || (!simReceivingDummy && simInSizeDummy > 0))
  { return 1;}
  else return 0; 
  
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cooja_radio_process1, ev, data)
{
  int len;

  PROCESS_BEGIN();

  while(1) 
    {
    
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
    if(poll_mode) {
      continue;
    }
   

   // modificacao para alocacao de pacote duplo  

    packetbuf_clear(); 
    // talvez venha com erro 
    len = radio_read(packetbuf_dataptr(), PACKETBUF_SIZE); 

    if(len > 0) {
    packetbuf_set_datalen(len);
    NETSTACK_MAC.input();  }
  }

  PROCESS_END();
}


/*---------------------------------------------------------------------------*/
static int
init(void)
{
  process_start(&cooja_radio_process1, NULL);
  return 1;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_value(radio_param_t param, radio_value_t *value)
{
  switch(param) {
  case RADIO_PARAM_RX_MODE:
    *value = 0;
    if(addr_filter) {
      *value |= RADIO_RX_MODE_ADDRESS_FILTER;
    }
    if(auto_ack) {
      *value |= RADIO_RX_MODE_AUTOACK;
    }
    if(poll_mode) {
      *value |= RADIO_RX_MODE_POLL_MODE;
    }
    return RADIO_RESULT_OK;
  case RADIO_PARAM_TX_MODE:
    *value = 0;
    if(send_on_cca) {
      *value |= RADIO_TX_MODE_SEND_ON_CCA;
    }
    return RADIO_RESULT_OK;
  case RADIO_PARAM_LAST_RSSI:
    *value = simSignalStrength;
    return RADIO_RESULT_OK;
  case RADIO_PARAM_LAST_LINK_QUALITY:
    *value = simLQI;
    return RADIO_RESULT_OK;
  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_value(radio_param_t param, radio_value_t value)
{
  switch(param) {
  case RADIO_PARAM_RX_MODE:
    if(value & ~(RADIO_RX_MODE_ADDRESS_FILTER |
        RADIO_RX_MODE_AUTOACK | RADIO_RX_MODE_POLL_MODE)) {
      return RADIO_RESULT_INVALID_VALUE;
    }

    /* Only disabling is acceptable for RADIO_RX_MODE_ADDRESS_FILTER */
    if ((value & RADIO_RX_MODE_ADDRESS_FILTER) != 0) {
      return RADIO_RESULT_NOT_SUPPORTED;
    }
    set_frame_filtering((value & RADIO_RX_MODE_ADDRESS_FILTER) != 0);

    /* Only disabling is acceptable for RADIO_RX_MODE_AUTOACK */
    if ((value & RADIO_RX_MODE_ADDRESS_FILTER) != 0) {
      return RADIO_RESULT_NOT_SUPPORTED;
    }
    set_auto_ack((value & RADIO_RX_MODE_AUTOACK) != 0);

    set_poll_mode((value & RADIO_RX_MODE_POLL_MODE) != 0);
    return RADIO_RESULT_OK;
  case RADIO_PARAM_TX_MODE:
    if(value & ~(RADIO_TX_MODE_SEND_ON_CCA)) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    set_send_on_cca((value & RADIO_TX_MODE_SEND_ON_CCA) != 0);
    return RADIO_RESULT_OK;
  case RADIO_PARAM_CHANNEL:
    if(value < 11 || value > 26) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    radio_set_channel(value);
    return RADIO_RESULT_OK;
 case RADIO_PARAM_CHANNELDummy:
    if(value < 11 || value > 26) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    radio_set_channelDummy(value);
    return RADIO_RESULT_OK;
  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_object(radio_param_t param, void *dest, size_t size)
{
  if(param == RADIO_PARAM_LAST_PACKET_TIMESTAMPContikiRadio) 
   {
    if(size != sizeof(rtimer_clock_t) || !dest) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    
    *(rtimer_clock_t *)dest = (rtimer_clock_t)simLastPacketTimestamp; 
    printf("Radio_1: Ok - Timestamp: %lu \n", (rtimer_clock_t)simLastPacketTimestamp);
    
    return RADIO_RESULT_OK;
  }
  else if(param == RADIO_PARAM_LAST_PACKET_TIMESTAMPDummy) {
    if(size != sizeof(rtimer_clock_t) || !dest) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    
    *(rtimer_clock_t *)dest = (rtimer_clock_t)simLastPacketTimestampDummy; 
    
    //GuilhermeA 
    printf("Radio_2: Ok - Timestamp: %lu \n", (rtimer_clock_t)simLastPacketTimestamp);
    //GuilhermeA 
    return RADIO_RESULT_OK;
  }
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_object(radio_param_t param, const void *src, size_t size)
{
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
const struct radio_driver cooja_radio_driver =
{
    init,
    prepare_packet,
    transmit_packet,
    radio_send,
    radio_read, 
    radio_read_dual,
    channel_clear,
    receiving_packet,
    pending_packet,
    radio_on,
    radio_off,
    get_value,
    set_value,
    get_object,
    set_object
};
/*---------------------------------------------------------------------------*/
SIM_INTERFACE(radio_interface,doInterfaceActionsBeforeTick,doInterfaceActionsAfterTick);
SIM_INTERFACE(dummy_interface,doInterfaceActionsBeforeTick,doInterfaceActionsAfterTick);
