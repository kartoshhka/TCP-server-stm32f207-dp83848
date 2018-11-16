#include "app_http.h"

uint16_t http_var1;

static char const* TAGCHAR[] = {"p","r","s","t"};
static char const** TAGS=TAGCHAR;

static const tCGI MSGS_CGI={"/msg.cgi", MSGS_CGI_Handler};
static tCGI CGI_TAB[1];
	
void SSI_Init(void)
{
  http_set_ssi_handler(SSI_Handler, (char const **)TAGS, 4);
}

void CGI_Init(void)
{
  CGI_TAB[0] = MSGS_CGI;
	http_set_cgi_handlers(CGI_TAB, 1);
}

uint16_t SSI_Handler(int iIndex, char *pcInsert, int iInsertLen)
{
	static uint32_t n = 0;
	switch(iIndex)
	{
		case 0:
		{
			n++;
			sprintf(pcInsert,"%u", n);
			return strlen(pcInsert);
		}
		case 1:
		{
			sprintf(pcInsert,"%u",n+5);
			return strlen(pcInsert);
		}
		case 2:
		{
			sprintf(pcInsert,"%u",n+10);
			return strlen(pcInsert);
		}
		case 3:
		{
			sprintf(pcInsert,"%u",n+15);
			return strlen(pcInsert);
		}		
	}
  return 0;
}

const char * MSGS_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  if (iIndex==0)
  {
		for (uint32_t i=0; i < iNumParams; i++)
    {
			  if (strcmp(pcParam[i] , "Var1")==0)
        {
					http_var1 = atoi(pcValue[i]); 
				}
		}
	}
	return "/send.html";
}
