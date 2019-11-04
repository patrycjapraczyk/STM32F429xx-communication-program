#ifndef USER_CODE_H
#define USER_CODE_H

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "err.h"

#if LWIP_TCP

#include "lwip/tcp.h"

#endif

#define DATA_LEN 16384
#define START_CODE 0xAA
#define DATA_PAYLOAD_LEN 256
#define END_CODE 0x81
#define PACKET_LEN (DATA_PAYLOAD_LEN + 8)
#define PACKETS_PER_GENERATION 10

//data to be sent over tcp
u16_t data_arr[DATA_LEN];

typedef enum
{
    ON,
    OFF
} SWITCH_STATUS;

//data struct
struct acqDataContainer
{
    u16_t dataLen;
    u8_t dataLock;
    u16_t dataNextToSend;
    u8_t data[DATA_LEN];
    struct acqDataContainer *nextBuffer;
};

void generateData(void);
void userLeds_play(void);
void switchOnLed(SWITCH_STATUS onOff);
void setUpDataStruct(void);
void generateData(void);
void tcpAcq_init(void);
err_t tcpAcq_accept(void *arg, struct tcp_pcb *pcb, err_t err);
err_t tcpAcq_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *data, err_t err);
void tcpAcq_err(void *arg, err_t err);
err_t prepareDataPacket(void);
err_t dispatchData(void);
err_t tcpAcq_write(struct tcp_pcb *pcb, const void *ptr, u16_t *length);
void populateData(void);
err_t tcpAcq_close(struct tcp_pcb *pcb);
err_t tcpAcq_sent(void *arg, struct tcp_pcb *pcb, u16_t len);
err_t tcpAcq_poll(void *arg, struct tcp_pcb *pcb);

#endif
