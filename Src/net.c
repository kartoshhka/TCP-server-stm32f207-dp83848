#include "net.h"

//-----------------------------------------------
//uint8_t ipaddr_dest[4];
//uint16_t port_dest;
char input_str[100];
//u8_t data[100];
struct tcp_pcb *server_pcb;

__IO uint32_t message_count=0;

//-----------------------------------------------
enum server_states
{
  ES_NONE = 0,
  ES_ACCEPTED,
  ES_RECEIVED,
  ES_CLOSING
};
//-----------------------------------------------
struct server_struct
{
  u8_t state; /* current connection state */
  u8_t retries;
  struct tcp_pcb *pcb; /* pointer on the current tcp_pcb */
  struct pbuf *p; /* pointer on the received/to be transmitted pbuf */
};
struct server_struct *ss;
//-----------------------------------------------
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static void tcp_server_connection_close(struct tcp_pcb *tpcb, struct server_struct *es);
static void tcp_server_error(void *arg, err_t err);
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void tcp_server_send(struct tcp_pcb *tpcb, struct server_struct *es);
static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb);
//-----------------------------------------------
void tcp_server_init(void)
{
	  server_pcb = tcp_new();
	  if (server_pcb != NULL)
	  {
	    err_t err;
			 // Assign to the new pcb a local IP address and a port number 
	    err = tcp_bind(server_pcb, IP_ADDR_ANY, 7);
	    if (err == ERR_OK)
	    {
				// Set the connection to the LISTEN state 
	      server_pcb = tcp_listen(server_pcb);
				// Specify the function to be called when a connection is established 
	      tcp_accept(server_pcb, tcp_server_accept);
	    }
	    else
	    {
				// We enter here if a conection to the addr IP address already exists 
				// so we don't need to establish a new one 
	      memp_free(MEMP_TCP_PCB, server_pcb);
	    }
	  }
}
//----------------------------------------------------------
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  err_t ret_err;
  struct server_struct *es;
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(err);
  tcp_setprio(newpcb, TCP_PRIO_MIN);
  es = (struct server_struct *)mem_malloc(sizeof(struct server_struct));
  ss = (struct server_struct *)mem_malloc(sizeof(struct server_struct));
  if (es != NULL)
  {
    es->state = ES_ACCEPTED;
    es->pcb = newpcb;
    ss->pcb = newpcb;
    es->retries = 0;
    es->p = NULL;
		/* Tell LwIP to associate this structure with this connection. */
    tcp_arg(newpcb, es);
		/* Specify the function that should be called when the TCP connection receives data */
    tcp_recv(newpcb, tcp_server_recv);
    tcp_err(newpcb, tcp_server_error);
    tcp_sent(newpcb, tcp_server_sent);
    tcp_poll(newpcb, tcp_server_poll, 0);
    ret_err = ERR_OK;
  }
  else
  {
    tcp_server_connection_close(newpcb, es);
    ret_err = ERR_MEM;
  }
  return ret_err;
}
//----------------------------------------------------------
static void tcp_server_connection_close(struct tcp_pcb *tpcb, struct server_struct *es)
{
  // remove all callbacks
  tcp_arg(tpcb, NULL);
  tcp_sent(tpcb, NULL);
  tcp_recv(tpcb, NULL);
  tcp_err(tpcb, NULL);
  tcp_poll(tpcb, NULL, 0);
  if (es != NULL)
  {
    mem_free(es);
  }
  tcp_close(tpcb);
}
//----------------------------------------------------------
static void tcp_server_error(void *arg, err_t err)
{
	struct server_struct *es;

  LWIP_UNUSED_ARG(err);

  es = (struct server_struct *)arg;
  if (es != NULL)
  {
    /*  free es structure */
    mem_free(es);
  }
}
//-----------------------------------------------
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
	struct server_struct *es;
	err_t ret_err;
	LWIP_ASSERT("arg != NULL",arg != NULL);
	es = (struct server_struct *)arg;
	if (p == NULL)
	{
		es->state = ES_CLOSING;
		if(es->p == NULL)
		{
			tcp_recved(tpcb, p->tot_len);
		}
		else
		{
			//acknowledge received packet
			tcp_sent(tpcb, tcp_server_sent);
			//send remaining data
			tcp_server_send(tpcb, es);
		}
		ret_err = ERR_OK;
	}
	else if(err != ERR_OK)
	{
		  if (p != NULL)
		  {
		    es->p = NULL;
		    pbuf_free(p);
		  }
		  ret_err = err;
	}
	else if(es->state == ES_ACCEPTED)
	{
		if (strncmp(p->payload, "\r\n", 2) == 0)
		{
				pbuf_free(p);
				ret_err = ERR_OK;
		}
		else
		{
		  tcp_recved(tpcb, p->tot_len);
			// don't need string below if input_str would be local
			strncpy(input_str,"\0",strlen(input_str)); 
		  strncpy(input_str,p->payload,p->len);
		  input_str[p->len] = '\0';
			pbuf_free(p);
		  ret_err = ERR_OK;
		}
	}
	else if (es->state == ES_RECEIVED)
	{
		  if(es->p == NULL)
		  {
		    ret_err = ERR_OK;
		  }
		  else
		  {
		    struct pbuf *ptr;
		    ptr = es->p;
		    pbuf_chain(ptr,p);
		  }
		  ret_err = ERR_OK;
	}
	else if(es->state == ES_CLOSING)
	{
		  tcp_recved(tpcb, p->tot_len);
		  es->p = NULL;
		  pbuf_free(p);
		  ret_err = ERR_OK;
	}
	else
	{
		  tcp_recved(tpcb, p->tot_len);
		  es->p = NULL;
		  pbuf_free(p);
		  ret_err = ERR_OK;
	}
	return ret_err;
}
//-----------------------------------------------
static void tcp_server_send(struct tcp_pcb *tpcb, struct server_struct *es)
{
  struct pbuf *ptr;
  err_t wr_err = ERR_OK;
  while ((wr_err == ERR_OK) &&
    (es->p != NULL) &&
    (es->p->len <= tcp_sndbuf(tpcb)))
  {
    ptr = es->p;
    wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);
    if (wr_err == ERR_OK)
    {
      u16_t plen;
      u8_t freed;
      plen = ptr->len;
      es->p = ptr->next;
      if(es->p != NULL)
      {
        pbuf_ref(es->p);
      }
      do
      {
        freed = pbuf_free(ptr);
      }
      while(freed == 0);
      tcp_recved(tpcb, plen);
    }
    else if(wr_err == ERR_MEM)
    {
      es->p = ptr;
    }
    else
    {
      //other problem
    }
  }
}
//----------------------------------------------------------
static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
	struct server_struct *es;
	LWIP_UNUSED_ARG(len);
	es = (struct server_struct *)arg;
	es->retries = 0;
	if(es->p != NULL)
	{
		tcp_server_send(tpcb, es);
	}
	else
	{
		if(es->state == ES_CLOSING)
			tcp_server_connection_close(tpcb, es);
	}
	return ERR_OK;
}
//-----------------------------------------------
static err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb)
{
	err_t ret_err;
	struct server_struct *es;
	es = (struct server_struct *)arg;
	if (es != NULL)
	{
	  if (es->p != NULL)
	  {
			tcp_sent(tpcb, tcp_server_sent);
      /* there is a remaining pbuf (chain) , try to send data */
      tcp_server_send(tpcb, es);
	  }
	  else
	  {
	    if(es->state == ES_CLOSING)
	    {
	      tcp_server_connection_close(tpcb, es);
	    }
	  }
	  ret_err = ERR_OK;
	}
	else
	{
	  tcp_abort(tpcb);
	  ret_err = ERR_ABRT;
	}
	return ret_err;
}
//----------------------------------------------------------
/*void net_ini(void)
{
	usartprop.usart_buf[0]=0;
	usartprop.usart_cnt=0;
	usartprop.is_tcp_connect=0;
	usartprop.is_text=0;
}*/
//-----------------------------------------------
void sendstring(char* buf_str)
{
	tcp_sent(ss->pcb, tcp_server_sent);
	tcp_write(ss->pcb, (void*)buf_str, strlen(buf_str), 1);
	tcp_recved(ss->pcb, strlen(buf_str));
}
//-----------------------------------------------
/*void string_parse(char* buf_str)
{
	  HAL_UART_Transmit(&huart6, (uint8_t*)buf_str,strlen(buf_str),0x1000);
	  sendstring(buf_str);
	  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13, GPIO_PIN_RESET);
}
//-----------------------------------------------
void UART6_RxCpltCallback(void)
{
	uint8_t b;
	b = str[0];
	if (usartprop.usart_cnt>25)
	{
		usartprop.usart_cnt=0;
		HAL_UART_Receive_IT(&huart6,(uint8_t*)str,1);
		return;
	}
	usartprop.usart_buf[usartprop.usart_cnt] = b;
	if(b==0x0A)
	{
		usartprop.usart_buf[usartprop.usart_cnt+1]=0;
		string_parse((char*)usartprop.usart_buf);
		usartprop.usart_cnt=0;
		HAL_UART_Receive_IT(&huart6,(uint8_t*)str,1);
		return;
	}
	usartprop.usart_cnt++;
	HAL_UART_Receive_IT(&huart6,(uint8_t*)str,1);
}*/
