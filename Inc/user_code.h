#ifndef USER_CODE_H
#define USER_CODE_H

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "err.h"

#if LWIP_TCP

#include "lwip/tcp.h"

#endif

struct acqDataContainer {
	u16_t acqDataLen;
	u16_t acqDataNextToSend;
	u8_t acqDataLock;
	struct acqDataContainer *nextBuffer;
	u8_t acqData[32768];
};

void userLeds_play(void);
err_t processData(void);
err_t dispatchData(void);
void tcpAcq_init(void);
err_t tcpAcq_accept(void*, struct tcp_pcb*, err_t);
err_t tcpAcq_close(struct tcp_pcb *pcb);
err_t tcpAcq_recv(void*, struct tcp_pcb*, struct pbuf*, err_t);
err_t tcpAcq_sent(void*, struct tcp_pcb*, u16_t);
err_t tcpAcq_poll(void*, struct tcp_pcb*);
void tcpAcq_err(void*, err_t);
err_t tcpAcq_write(struct tcp_pcb *pcb, const void* ptr, u16_t *length);
struct acqDataContainer* tcpAcq_initDataStr(u16_t length);
void adc_dma_transfer_init(void);

#endif
