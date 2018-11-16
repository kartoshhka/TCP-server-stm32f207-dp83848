#ifndef __APP_HTTP_H
#define __APP_HTTP_H

#include "lwip/apps/httpd.h"
#include "lwip.h"
#include "stm32f2xx_hal.h"
#include <string.h>

void SSI_Init(void);
void CGI_Init(void);

uint16_t SSI_Handler(int iIndex, char *pcInsert, int iInsertLen);
const char * MSGS_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

#endif /* __APP_ETHERNET_H */
