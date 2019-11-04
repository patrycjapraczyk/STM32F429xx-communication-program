#include "user_code.h"
#include "main.h"
#include "fsdata_custom.h"

#define adcBufferLen 512


///TODO: try with only one data structiure,
//add a check if the data array is overloaded in processData

unsigned int timFlag;
// TRUE if client has been connected to the tcp socket,
unsigned int tcpStream = 0;
// TCP control block for the connected client
struct tcp_pcb *acqConnClient;
//number of data packets sent so farA
uint32_t dataCnt = 0;

struct acqDataContainer acqConn1, acqConn2;
struct acqDataContainer *acqConActive;
struct acqDataContainer *acqSendBuffer;

u16_t curr_data_index = 0;

int state = 0;

void userLeds_play(void)
{
	switch (state)
	{
	case 0:
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7 | GPIO_PIN_14, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
		state += 1;
		break;
	case 1:
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_14, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
		state += 1;
		break;
	case 2:
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_7, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
		state = 0;
		break;
	default:
		state = 0;
		break;
	}
}

//fill in data array with a number sequence
void generateData(void)
{
    for (u16_t i = 0; i < DATA_LEN; i++)
    {
        data_arr[i] = i;
    }
}

/* create a data packet */
err_t prepareDataPacket(void)
{
	if(!tcpStream)
	{
		return ERR_OK;
	}

	if (acqConActive->dataNextToSend != 0) {
		acqConActive = acqConActive->nextBuffer; 
	}

	int curr_index = acqConActive->dataLen;
	if(curr_index + PACKET_LEN >= DATA_LEN)
	{
		curr_index = 0;
	}

	//lock data_struct
	acqConActive->dataLock = 1;
	//array of bytes to be sent
	//start code
	acqConActive->data[curr_index++] = START_CODE;
	//data packet length
	acqConActive->data[curr_index++] = (u8_t)((PACKET_LEN >> 8) & 0xFF);
	acqConActive->data[curr_index++] = (u8_t)((PACKET_LEN >> 0) & 0xFF);
	// data index
	acqConActive->data[curr_index++] = (u8_t)((dataCnt >> 24) & 0xFF);
	acqConActive->data[curr_index++] = (u8_t)((dataCnt >> 16) & 0xFF);
	acqConActive->data[curr_index++] = (u8_t)((dataCnt >> 8) & 0xFF);
	acqConActive->data[curr_index++] = (u8_t)((dataCnt >> 0) & 0xFF);

	//try sending as much as possible from data_arr
	//index every 2 bytes, index length- 2 bytes, data length 2 bytes
	//should start with index 13
	for (int i = 0; i < DATA_PAYLOAD_LEN/4; i++)
	{
		//int dataPieceCnt = i;
		acqConActive->data[curr_index++] = (u8_t)((i >> 8) & 0xFF);
		acqConActive->data[curr_index++] = (u8_t)((i >> 0) & 0xFF);

		//TAKE DATA STARTING FROM THE SAVED INDEX
		//u16_t curr_data = data_arr[curr_data_index];
		acqConActive->data[curr_index++] = (u8_t)((data_arr[curr_data_index] >> 8) & 0xFF);
		acqConActive->data[curr_index++] = (u8_t)((data_arr[curr_data_index] >> 0) & 0xFF);
		//increment data index
		if(curr_data_index < DATA_LEN)
		{
			curr_data_index++;
		}
		else
		{
			curr_data_index = 0;
		}
	}
	//end code
	acqConActive->data[curr_index] = END_CODE;
	//update length
	acqConActive->dataLen = curr_index + 1;

	dataCnt++;

	//unlock data_struct
	acqConActive->dataLock = 0;
}

void setUpDataStruct(void)
{

	struct acqDataContainer *ac;

	ac = &acqConn1;
	ac->dataLen = 0;
	ac->dataNextToSend = 0;
	ac->dataLock = 0;
	ac->nextBuffer = &acqConn2;
	
	acqConActive = ac;
	acqSendBuffer = ac;
	
	ac = &acqConn2;
	ac->dataLen = 0;
	ac->dataNextToSend = 0;
	ac->dataLock = 0;
	ac->nextBuffer = &acqConn1;

}

void tcpAcq_init(void)
{
	// TCP Protocol Control Block
	struct tcp_pcb *pcb;
	err_t err;
/* 
//SERVER
	tcpStream = 0;
	setUpDataStruct();
	pcb = tcp_new_ip_type(IPADDR_TYPE_V4);
	tcp_setprio(pcb, TCP_PRIO_NORMAL);
	err = tcp_bind(pcb, IP4_ADDR_ANY, 7771); //bind socket IP and port address
	pcb = tcp_listen(pcb);					 //listen for connection request messages from clients
	tcp_accept(pcb, tcpAcq_accept);			 //creates a new socket associated with the address pair of this connection
*/
	tcpStream = 0;
	setUpDataStruct();
	pcb = tcp_new();
	ip_addr_t host_addr, client_adddr;
	IP4_ADDR(&host_addr, 192, 168, 1, 1);
	err = tcp_bind(pcb, IP4_ADDR_ANY, 7771); //bind socket IP and port address
	err = tcp_connect(pcb, &host_addr, 7772, tcpAcq_accept); 
}

//prepare and send data
err_t dispatchData(void)
{
	u16_t sendBufIndex;
	const void *dataPtr;

	//if not connected to any client
	if (!tcpStream) 
	{ 
		return ERR_CONN; 
	}

	//if acqSendBuffer is locked
	if (acqSendBuffer->dataLock) 
	{ 
		return ERR_OK; 
	}

	//no data to send in current structure
	if (acqSendBuffer->dataLen == 0)
	{ 
		//no data to send in the next structure
		if (acqSendBuffer->nextBuffer->dataLen == 0) 
		{
			return ERR_OK;
		}
		acqSendBuffer = acqSendBuffer->nextBuffer;
	}

	//lock acqConn
	acqSendBuffer->dataLock = 1;

	err_t status = ERR_OK;

	sendBufIndex = acqSendBuffer->dataLen;
	dataPtr = acqSendBuffer->data + acqSendBuffer->dataNextToSend;
	if (sendBufIndex > 0) {
			tcpAcq_write(acqConnClient, dataPtr, &sendBufIndex);
			if (sendBufIndex > 0) {
				acqSendBuffer->dataLen -= sendBufIndex;
				if (acqSendBuffer->dataLen == 0) {
					acqSendBuffer->dataNextToSend = 0;
				}
				else {
					acqSendBuffer->dataNextToSend += sendBufIndex;
				}
				status = tcp_output(acqConnClient);
			}
		}
	acqSendBuffer->dataLock = 0;
	return status;
}

err_t tcpAcq_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{
	tcpStream = 1;
	tcp_setprio(pcb, TCP_PRIO_NORMAL);
	tcp_nagle_disable(pcb); //disable Nagle's algorithm - min latency

	acqConnClient = pcb;

	tcp_arg(pcb, acqConnClient); //pass acqConnClient as an argument to TCP socket functions callbacks

	//set up tcp functions callbacks
	tcp_recv(pcb, tcpAcq_recv); //receive data callback
	tcp_err(pcb, tcpAcq_err);   //fatal error callback
	tcp_sent(pcb, tcpAcq_sent);

	return ERR_OK;
}

err_t tcpAcq_write(struct tcp_pcb *pcb, const void *ptr, u16_t *length)
{
	u16_t tcp_len;
	err_t status;

	tcp_len = tcp_sndbuf(pcb);
	if (tcp_len < *length)
	{
		*length = tcp_len;
	}

	status = tcp_write(pcb, ptr, *length, TCP_WRITE_FLAG_COPY);

	if (status != ERR_OK)
	{
		*length = 0;
	}

	return status;
}

err_t tcpAcq_close(struct tcp_pcb *pcb)
{
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

err_t tcpAcq_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *data, err_t err)
{
	return ERR_OK;
}

err_t tcpAcq_sent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
	return ERR_OK;
}

err_t tcpAcq_poll(void *arg, struct tcp_pcb *pcb)
{
	if (pcb->state == CLOSE_WAIT)
	{
		tcpAcq_close(acqConnClient);
	}

	return ERR_OK;
}

void tcpAcq_err(void *arg, err_t err)
{
}