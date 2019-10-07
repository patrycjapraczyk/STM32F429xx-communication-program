#include "user_code.h"
#include "main.h"
#include "fsdata_custom.h"
#define adcBufferLen 512
unsigned int timFlag;
extern ADC_HandleTypeDef hadc1;
unsigned int tcpStream;
struct tcp_pcb *acqConnClient;	
struct acqDataContainer acqConn1, acqConn2;
struct acqDataContainer *acqConActive;
struct acqDataContainer *acqSendBuffer;
uint32_t rawData;

extern DMA_HandleTypeDef hdma_adc1;
extern ADC_HandleTypeDef hadc1;

uint16_t adcBuffer[adcBufferLen];

uint32_t procCnt = 0;


void userLeds_play(void) {
	static int state = 0;
	static u8_t cnt = 0;
	u16_t idx;
	err_t tcpStat;
	uint32_t tData;
	
	timFlag |= 0x01;
	
	switch (state) {
		case 0:
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7 | GPIO_PIN_14, GPIO_PIN_RESET);
			state += 1;
			break;
		case 1:
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_14, GPIO_PIN_RESET);
			state += 1;
		
			tData = (uint32_t)adcBuffer[0] +
							(uint32_t)adcBuffer[1] +
							(uint32_t)adcBuffer[2] +
							(uint32_t)adcBuffer[3] +
							(uint32_t)adcBuffer[4];
			tData = 10 * tData / 5;
			tData = (tData + 5) / 10;
			fsdata_updateData(1, tData);
			rawData = tData;
			break;
		case 2:
/*
			if (tcpStream) {
				idx = acqConn.acqDataLen;
				acqConn.acqData[idx++] = 0xaa;
				acqConn.acqData[idx++] = 8;
				acqConn.acqData[idx++] = cnt++;
				acqConn.acqData[idx++] = (u8_t)((rawData >> 24) & 0xff);
				acqConn.acqData[idx++] = (u8_t)((rawData >> 16) & 0xff);
				acqConn.acqData[idx++] = (u8_t)((rawData >> 8) & 0xff);
				acqConn.acqData[idx++] = (u8_t)((rawData >> 0) & 0xff);
				acqConn.acqData[idx++] = 0x81;
				acqConn.acqDataLen = idx;
				tcpAcq_write(acqConn.tcpAcq_client, acqConn.acqData, &idx);
				acqConn.acqDataLen -= idx;
				tcpStat = tcp_output(acqConn.tcpAcq_client);
			}
*/			
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_7, GPIO_PIN_RESET);
			state = 0;
			break;
		default:
			state = 0;
			break;
	}
}

err_t processData(void) {
	static uint16_t dataPoint = 0;
	uint16_t currentDataPoint;
	uint16_t dataCnt, dataLen;
	uint16_t sendBufIndex;
	uint16_t sendBufLen;
	err_t tcpStat;
	static u16_t errorCnt = 0;
	
	if (tcpStream) {	
		procCnt += 1;
		currentDataPoint = 512 - hdma_adc1.Instance->NDTR;
		dataLen = (currentDataPoint - dataPoint) & 511;
//			sendBufLen = dataLen * 4 + 4;
		sendBufLen = dataLen * 4 + 4 + (6 + 4);
		
		if (acqConActive->acqDataNextToSend != 0) {
			acqConActive = acqConActive->nextBuffer; 
		}
		sendBufIndex = acqConActive->acqDataLen;
		if (sendBufIndex + sendBufLen >= 32768) {
			dataPoint = currentDataPoint;
//			Error_Handler();
			errorCnt += 1;
			return ERR_OK;
		}
		else if (sendBufIndex == 0) {
			errorCnt = 0;
		}
		
		acqConActive->acqDataLock = 1;
		
		acqConActive->acqData[sendBufIndex++] = 0xAA;
		acqConActive->acqData[sendBufIndex++] = (u8_t)((sendBufLen >> 8) & 0xFF);
		acqConActive->acqData[sendBufIndex++] = (u8_t)((sendBufLen >> 0) & 0xFF);
		
		acqConActive->acqData[sendBufIndex++] = (u8_t)((procCnt >> 24) & 0xFF);
		acqConActive->acqData[sendBufIndex++] = (u8_t)((procCnt >> 16) & 0xFF);
		acqConActive->acqData[sendBufIndex++] = (u8_t)((procCnt >> 8) & 0xFF);
		acqConActive->acqData[sendBufIndex++] = (u8_t)((procCnt >> 0) & 0xFF);
//		acqConActive->acqData[sendBufIndex++] = (u8_t)(((acqConActive->acqDataLen) >> 8) & 0xFF);
//		acqConActive->acqData[sendBufIndex++] = (u8_t)(((acqConActive->acqDataLen) >> 0) & 0xFF);
		acqConActive->acqData[sendBufIndex++] = (u8_t)(((acqConn1.acqDataLen) >> 8) & 0xFF);
		acqConActive->acqData[sendBufIndex++] = (u8_t)(((acqConn1.acqDataLen) >> 0) & 0xFF);
		acqConActive->acqData[sendBufIndex++] = (u8_t)(((acqConn2.acqDataLen) >> 8) & 0xFF);
		acqConActive->acqData[sendBufIndex++] = (u8_t)(((acqConn2.acqDataLen) >> 0) & 0xFF);
		acqConActive->acqData[sendBufIndex++] = (u8_t)((errorCnt >> 8) & 0xFF);
		acqConActive->acqData[sendBufIndex++] = (u8_t)((errorCnt >> 0) & 0xFF);
		
		for (dataCnt = 0; dataCnt <  dataLen; dataCnt++) {
			acqConActive->acqData[sendBufIndex++] = (u8_t)((dataCnt >> 8) & 0xFF);
			acqConActive->acqData[sendBufIndex++] = (u8_t)((dataCnt >> 0) & 0xFF);
/*
			acqConn.acqData[sendBufIndex++] = (u8_t)((dataPoint >> 8) & 0xFF);
			acqConn.acqData[sendBufIndex++] = (u8_t)((dataPoint >> 0) & 0xFF);
*/
			acqConActive->acqData[sendBufIndex++] = (u8_t)((adcBuffer[dataPoint] >> 8) & 0xFF);
			acqConActive->acqData[sendBufIndex++] = (u8_t)((adcBuffer[dataPoint] >> 0) & 0xFF);
			dataPoint = (dataPoint + 1) & 511;
		}
		
		acqConActive->acqData[sendBufIndex++] = 0x81;
		acqConActive->acqDataLen = sendBufIndex;
	
		acqConActive->acqDataLock = 0;
		
		return ERR_OK;
	}
	else {
		dataPoint = 512 - hdma_adc1.Instance->NDTR;
		return ERR_CONN;
	}
	return ERR_OK;
}

err_t dispatchData(void) {
	uint16_t sendBufIndex;
	const void* dataPtr;
	err_t tcpStat;
	
	if (tcpStream) {
		if (acqSendBuffer->acqDataLen == 0) {
			if (acqSendBuffer->nextBuffer->acqDataLen == 0) {
				return ERR_OK;
			}
			else {
				acqSendBuffer = acqSendBuffer->nextBuffer;
			}
		}
		else {
			if (acqSendBuffer->acqDataLock == 1) {
				return ERR_OK;
			}
		}
		
		sendBufIndex = acqSendBuffer->acqDataLen;
		dataPtr = acqSendBuffer->acqData + acqSendBuffer->acqDataNextToSend;
		if (sendBufIndex > 0) {
			tcpAcq_write(acqConnClient, dataPtr, &sendBufIndex);
			if (sendBufIndex > 0) {
				acqSendBuffer->acqDataLen -= sendBufIndex;
				if (acqSendBuffer->acqDataLen == 0) {
					acqSendBuffer->acqDataNextToSend = 0;
				}
				else {
					acqSendBuffer->acqDataNextToSend += sendBufIndex;
				}
				tcpStat = tcp_output(acqConnClient);
				return tcpStat;
			}
			else {
				return ERR_OK;
			}
		}
		else {
			return ERR_OK;
		}
	}
	else {
		return ERR_CONN;
	}
	return ERR_OK;
}

void tcpAcq_init(void) {
	
  struct tcp_pcb *pcb;
  err_t err;

	tcpStream = 0;
	tcpAcq_initDataStr(0);
 
  pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
  tcp_setprio(pcb, TCP_PRIO_NORMAL);
  err = tcp_bind(pcb, IP4_ADDR_ANY, 7771);
  pcb = tcp_listen(pcb);
  tcp_accept(pcb, tcpAcq_accept);

  	/* pcb = tcp_new();
	ip_addr_t host_addr, client_adddr;
	IP4_ADDR(&host_addr, 192, 168, 1, 1);
	err = tcp_bind(pcb, IP4_ADDR_ANY, 7771); //bind socket IP and port address
	err = tcp_connect(pcb, &host_addr, 7772, tcpAcq_accept);*/
}

err_t tcpAcq_accept(void *arg, struct tcp_pcb *pcb, err_t err) {
		
	tcpStream = 1;

  /* Set priority */
  tcp_setprio(pcb, TCP_PRIO_NORMAL);
	
	/* Disable Nagle's algorithm - min latency */
	tcp_nagle_disable(pcb);

	acqConnClient = pcb;

  /* Tell TCP that this is the structure we wish to be passed for our
     callbacks. */
  tcp_arg(pcb, acqConnClient);

  /* Set up the various callback functions */
  tcp_recv(pcb, tcpAcq_recv);
  tcp_err(pcb, tcpAcq_err);
  tcp_poll(pcb, tcpAcq_poll, HTTPD_POLL_INTERVAL);
  tcp_sent(pcb, tcpAcq_sent);

  return ERR_OK;
}

err_t tcpAcq_close(struct tcp_pcb *pcb) {
	
	err_t err;
	
	tcpStream = 0;
	
	tcp_arg(pcb, NULL);
	tcp_recv(pcb, NULL);
	tcp_err(pcb, NULL);
	tcp_poll(pcb, NULL, 0);
  tcp_sent(pcb, NULL);
	
	err = tcp_close(pcb);
	while (err != ERR_OK);
	
	return ERR_OK;
}

err_t tcpAcq_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *data, err_t err ) {
	
	return ERR_OK;
}

err_t tcpAcq_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
	
	return ERR_OK;
}

err_t tcpAcq_poll(void *arg, struct tcp_pcb *pcb) {
	
	if (pcb->state == CLOSE_WAIT) {
		tcpAcq_close(acqConnClient);
	}
	
	return ERR_OK;
}

void tcpAcq_err(void *arg , err_t err) {
	
}

err_t tcpAcq_write(struct tcp_pcb *pcb, const void* ptr, u16_t *length) {
	
	u16_t len, max_len;
	err_t err;
	
	len = *length;
	if (len == 0) {
		return ERR_OK;
	}
	max_len = tcp_sndbuf(pcb);
	if (max_len < len) {
		len = max_len;
	}
	
	err = tcp_write(pcb, ptr, len, TCP_WRITE_FLAG_COPY);
	
	if (err == ERR_OK) {
		*length = len;
	}
	else {
		*length = 0;
//		Error_Handler();
	}
	
	return err;
}

struct acqDataContainer* tcpAcq_initDataStr(u16_t length) {
	
	struct acqDataContainer *ac;
	
	ac = &acqConn1;
	ac->acqDataLen = 0;
	ac->acqDataNextToSend = 0;
	ac->acqDataLock = 0;
	ac->nextBuffer = &acqConn2;
	
	acqConActive = ac;
	acqSendBuffer = ac;
	
	ac = &acqConn2;
	ac->acqDataLen = 0;
	ac->acqDataNextToSend = 0;
	ac->acqDataLock = 0;
	ac->nextBuffer = &acqConn1;
	
	return NULL;
}

void adc_dma_transfer_init(void) {
	
	hdma_adc1.Instance = DMA2_Stream0;
	hdma_adc1.Init.Channel = DMA_CHANNEL_0;
	hdma_adc1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
	hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
	hdma_adc1.Init.MemBurst = DMA_MBURST_SINGLE;
	hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
	hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
	hdma_adc1.Init.Mode = DMA_CIRCULAR;
	hdma_adc1.Init.PeriphBurst = DMA_PBURST_SINGLE;
	hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
	hdma_adc1.Init.Priority = DMA_PRIORITY_HIGH;
	
	if (HAL_DMA_Init(&hdma_adc1) != HAL_OK) {
		Error_Handler();
	}
	if (HAL_DMA_Start(&hdma_adc1, (uint32_t)(&(hadc1.Instance->DR)), (uint32_t)(adcBuffer), adcBufferLen) != HAL_OK) {
		Error_Handler();
	}
	if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adcBuffer, (uint32_t)adcBufferLen) != HAL_OK) {
		Error_Handler();
	}
}

