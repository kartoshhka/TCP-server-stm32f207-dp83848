#ifndef NET_H_
#define NET_H_

//-----------------------------------------------

#include "stm32f2xx_hal.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "lwip.h"
#include "lwip/tcp.h"
#include "netif.h"

void tcp_server_init(void);
void sendstring(char* buf_str);

//-----------------------------------------------

#endif /* NET_H_ */
