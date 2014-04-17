/**
 * Small application to have the reader turn on and send
 * a tag stream to a URL provided as below:
 * http://192.168.2.222/log
 * @file readasync.c
 */

#include <tm_reader.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include "socket_comms.h"



char URI[256];
char IP[32];
char READERID[64];

int getPathFromConfig(){
    FILE *fp;
    fp = fopen("config.app", "r");
    
    fscanf(fp, "%s", READERID);
    fscanf(fp, "%s", IP);
    fscanf(fp, "%s", URI);

    printf("ReaderID is: %s\n", READERID);
    printf("IP is: %s\n", IP);
    printf("URI is: %s\n", URI);
    fclose(fp);

    return 0;
}

void errx(int exitval, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);

  exit(exitval);
}

void checkerr(TMR_Reader* rp, TMR_Status ret, int exitval, const char *msg)
{
  if (TMR_SUCCESS != ret)
  {
    errx(exitval, "Error %s: %s\n", msg, TMR_strerr(rp, ret));
  }
}

void serialPrinter(bool tx, uint32_t dataLen, const uint8_t data[],
                   uint32_t timeout, void *cookie)
{
  FILE *out = cookie;
  uint32_t i;

  fprintf(out, "%s", tx ? "Sending: " : "Received:");
  for (i = 0; i < dataLen; i++)
  {
    if (i > 0 && (i & 15) == 0)
      fprintf(out, "\n         ");
    fprintf(out, " %02x", data[i]);
  }
  fprintf(out, "\n");
}

void stringPrinter(bool tx,uint32_t dataLen, const uint8_t data[],uint32_t timeout, void *cookie)
{
  FILE *out = cookie;

  fprintf(out, "%s", tx ? "Sending: " : "Received:");
  fprintf(out, "%s\n", data);
}

void callback(TMR_Reader *reader, const TMR_TagReadData *t, void *cookie);
void exceptionCallback(TMR_Reader *reader, TMR_Status error, void *cookie);

int main(int argc, char *argv[])
{

#ifndef TMR_ENABLE_BACKGROUND_READS
  errx(1, "This sample requires background read functionality.\n"
          "Please enable TMR_ENABLE_BACKGROUND_READS in tm_config.h\n"
          "to run this codelet\n");
  return -1;
#else

  TMR_Reader r, *rp;
  TMR_Status ret;
  TMR_Region region;
  TMR_TransportListenerBlock tb;
  TMR_ReadListenerBlock rlb;
  TMR_ReadExceptionListenerBlock reb;
  int count;

  getPathFromConfig();
  if (argc < 2)
  {
    errx(1, "Please provide reader URL, such as:\n"
           "tmr:///com4\n"
           "tmr://my-reader.example.com\n");
  }
  
  rp = &r;
  ret = TMR_create(rp, argv[1]);
  checkerr(rp, ret, 1, "creating reader");

  if (TMR_READER_TYPE_SERIAL == rp->readerType)
  {
    tb.listener = serialPrinter;
  }
  else
  {
    tb.listener = stringPrinter;
  }
  tb.cookie = stdout;
#if 0
  TMR_addTransportListener(rp, &tb);
#endif
  
  ret = TMR_connect(rp);
  checkerr(rp, ret, 1, "connecting reader");


  /** this is the test area *****************************************/
    int readPowerToSet = 3000; 
  ret = TMR_paramSet(rp, TMR_PARAM_RADIO_READPOWER,&readPowerToSet);   
  checkerr(rp, ret, 1, "setting read power");
  
  /** end test area *****************************************************/


  region = TMR_REGION_NONE;
  ret = TMR_paramGet(rp, TMR_PARAM_REGION_ID, &region);
  checkerr(rp, ret, 1, "getting region");

  if (TMR_REGION_NONE == region)
  {
    TMR_RegionList regions;
    TMR_Region _regionStore[32];
    regions.list = _regionStore;
    regions.max = sizeof(_regionStore)/sizeof(_regionStore[0]);
    regions.len = 0;

    ret = TMR_paramGet(rp, TMR_PARAM_REGION_SUPPORTEDREGIONS, &regions);
    checkerr(rp, ret, __LINE__, "getting supported regions");

    if (regions.len < 1)
    {
      checkerr(rp, TMR_ERROR_INVALID_REGION, __LINE__, "Reader doesn't supportany regions");
    }
    region = regions.list[0];
    ret = TMR_paramSet(rp, TMR_PARAM_REGION_ID, &region);
    checkerr(rp, ret, 1, "setting region");  
  }

  rlb.listener = callback;
  rlb.cookie = NULL;

  reb.listener = exceptionCallback;
  reb.cookie = NULL;

  ret = TMR_addReadListener(rp, &rlb);
  checkerr(rp, ret, 1, "adding read listener");

  ret = TMR_addReadExceptionListener(rp, &reb);
  checkerr(rp, ret, 1, "adding exception listener");

  while(1)
  {
      ret = TMR_startReading(rp);
      checkerr(rp, ret, 1, "starting reading");
          sleep(2);
      ret = TMR_stopReading(rp);
      printf("Stopping reading\n");
      checkerr(rp, ret, 1, "stopping reading");
      //delay(400);
      sleep(1);
  }

  TMR_destroy(rp);
  return 0;

#endif /* TMR_ENABLE_BACKGROUND_READS */
}

void
callback(TMR_Reader *reader, const TMR_TagReadData *t, void *cookie)
{
  char epcStr[128];
  char uri1[256]= "GET http://"; 
  char uri2[256] = "\0";
  char temp[90];
  char temp2[10];
  int RSSI = 0;
  int socket = 0;
  
  strcpy(uri2, URI);
  strcpy(temp, uri1);
  strcat(temp, IP);
  strcat(temp, uri2);
  //  socket = open_socket(ShortURI, "80");
  //printf("opening socket\n");
  socket = open_socket(IP,"80");
  //printf("IP in callback is %s\n", IP);
  //printf("%d is the socket\n", socket);
  TMR_bytesToHex(t->tag.epc, t->tag.epcByteCount, epcStr);
  RSSI = t->rssi;
  int antenna = t->antenna - 1;
  //Convert the RSSI int to a string into the temp2 char buffer
  sprintf(temp2, "%d", RSSI);
  //printf("just before digital write\n");

  //Create the string that will be sent to the socket. 
  //strcpy(temp, uri1);
  strcat(temp, READERID); 
  //printf("After READERID assignment\n");
  strcat(temp, "&antenna=");
  sprintf(temp2, "%d", antenna);
  strcat(temp, temp2);
  //printf("After Antenna assignment\n");
  strcat(temp, "&TagNumber=");
  strcat(temp, epcStr);
  //printf("After epc assignment\n");
  strcat(temp, "\r\n");
  printf("%s\n", temp);
  // write the tag number to the socket. 
  send_data(socket, temp);
  close(socket);
}


void 
exceptionCallback(TMR_Reader *reader, TMR_Status error, void *cookie)
{
  fprintf(stdout, "Error:%s\n", TMR_strerr(reader, error));
}
