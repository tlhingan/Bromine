#include <tm_reader.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <tmr_types.h>
#include "socket_comms.h"
#include <inttypes.h>
#include <ctype.h>

TMR_Reader r, *rp;
TMR_Status ret;
TMR_TransportListenerBlock tlb;
TMR_ReadListenerBlock rlb;
TMR_ReadExceptionListenerBlock reb;
 
char stringReaderURI[64];
char stringHardwareVersion[64];
char stringModel[64];
char stringProductGroup[64];
char stringSerialNumber[64];
char stringSoftwareVersion[64];
char stringDescription[64];
char value_data8[256];
uint8_t arrayConnectedPortList[64];
uint8_t arrayPortList[64];
uint8_t arrayPortSwitchGPOs[64];
uint8_t arrayInputList[64];
uint8_t arrayOutputList[64];
uint8_t antennaList[64];
uint32_t arrayHopTable[64];

uint32_t BaudRate = 9600;
uint32_t CommandTimeout = 0;
uint32_t TransportTimeout = 0;
uint32_t ASyncOffTime = 0;
uint32_t ASyncOnTime = 0;
uint32_t HopTime = 0;
uint32_t ReadFilterTimeout = 0;
TMR_GEN2_LinkFrequency Gen2BLF = 0;
uint16_t Gen2WriteReplyTimeout = 0;
uint16_t MaxPower = 0; /* non writable */
uint16_t MinPower = 0; /* non writable */
uint16_t ReadPower = 0;
uint16_t WritePower = 3000;
uint16_t TagOpSuccess = 0; /* non writable */
uint16_t TagOpFailures = 0; /* non writable */
uint16_t ProductGroupID = 0; /* non writable */
uint8_t Antenna = 0;
uint8_t Temperature = 0; /* non writable */

struct TMR_uint8List ConnectedPortList; /* non writable */
struct TMR_uint8List PortList; /* non writable */
struct TMR_uint8List PortSwitchGPOs;
struct TMR_uint8List InputList;
struct TMR_uint8List OutputList;
struct TMR_uint32List HopTable;

struct TMR_PortValueList SettlingTime;
struct TMR_PortValueList PortRead;
struct TMR_PortValueList PortWrite;
struct TMR_PortValue arraySettlingTime[64];
struct TMR_PortValue arrayPortRead[64];
struct TMR_PortValue arrayPortWrite[64];
struct TMR_AntennaMapList txrxMap;
struct TMR_AntennaMap *valueAntenna_Map;
struct TMR_AntennaMap arrayTxRxMap[64];

struct TMR_String ReaderURI; /* non writable */
struct TMR_String HardwareVersion; /* non writable */
struct TMR_String SoftwareVersion; /* non writable */
struct TMR_String Model; /* non writable */
struct TMR_String ProductGroup; /* non writable */
struct TMR_String SerialNumber; /* non writable */
struct TMR_String description;

bool CheckPort = 0;
bool WriteEarlyExit = 0;
bool EnablePowerSave = 0;
bool LBTEnable = 0;
bool AntennaEnable = 0;
bool FrequencyEnable = 0;
bool TemperatureEnable = 0;
bool RecordHighestRSSI = 0;
bool RSSIdBm = 0;
bool UniqueByAntenna = 0;
bool UniqueByData = 0;
bool EnableSJC = 0;
bool EnableReadFilter = 0;
bool ExtendedEPC = 0;

TMR_Region region = TMR_REGION_NA; 
TMR_Region regionValue[32];
struct TMR_RegionList regionList; /* non writable */
TMR_SR_UserMode userMode;
TMR_SR_PowerMode powerMode;
TMR_GEN2_Password accessPassword;
TMR_GEN2_TagEncoding tagEncoding;
TMR_GEN2_Session session;
TMR_GEN2_Target target;
TMR_SR_GEN2_Q q;
TMR_GEN2_Tari tari;
TMR_TagProtocolList supportedProtocols; /* non writable */
TMR_TagProtocol valueTP[32];
TMR_TagProtocol protocol;
TMR_ReadPlan readPlan;
TMR_GEN2_WriteMode writeMode;
TMR_ISO180006B_LinkFrequency ISOBLF;
TMR_ISO180006B_ModulationDepth modulation;
TMR_ISO180006B_Delimiter delimiter;
struct tm currentTime;

static const char *RegionList[] = {"Unspecified", "North America", "European Union", "Korea", "India", "Japan", "China", "European Union 2", "European Union 3", "Korea 2", "China 2", "Australia", "New Zealand", "Open"};
static const char *PowerModeList[] = {"Full", "MinSave", "MedSave", "MaxSave", "Sleep"};
static const char *UserModeList[] = {"Unspecified", "Printer", "Conveyor", "Portal", "Handheld"};
static const char *tagEncodingNames[] = {"FM0", "M2", "M4", "M8"};
static const char *sessionNames[] = {"S0", "S1", "S2", "S3"};
static const char *targetNames[] = {"A", "B", "AB", "BA"};
static const char *gen2LinkFrequencyNames[] = {"250kHz", "300kHz", "320kHz", "40kHz", "640KHz"};
static const char *tariNames[] = {"25us", "12.5us", "6.25us"};
static const char *iso180006bModulationDepthNames[] = {"99 percent", "11 percent"};
static const char *iso180006bDelimiterNames[] = {"", "Delimiter1", "", "", "Delimiter4"};
static const char *protocolNames[] = {NULL, NULL, NULL, "ISO180006B", NULL, "GEN2", "UCODE", "IPX64", "IPX256"};

void errx(int exitval, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  exit(exitval);
}

void serialPrinter(bool tx, uint32_t dataLen, const uint8_t data[], uint32_t timeout, void *cookie)
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

void callback(TMR_Reader *reader, const TMR_TagReadData *t, void *cookie)
{
  char epcStr[128];
  char uri1[256]= "GET http://"; 
  char uri2[256] = "\0";
  char temp[90];
  char temp2[10];
  int RSSI = 0;
  strcpy(temp, uri1);
  strcat(temp, uri2);
  TMR_bytesToHex(t->tag.epc, t->tag.epcByteCount, epcStr);
  RSSI = t->rssi;
  int antenna = t->antenna - 1;
  sprintf(temp2, "%d", RSSI);
  strcat(temp, "&antenna=");
  sprintf(temp2, "%d", antenna);
  strcat(temp, temp2);
  strcat(temp, "&TagNumber=");
  strcat(temp, epcStr);
  strcat(temp, "\r\n");
  printf("%s\n", temp);
}

void exceptionCallback(TMR_Reader *reader, TMR_Status error, void *cookie)
{
  fprintf(stdout, "Error:%s\n", TMR_strerr(reader, error));
}

void setReaderConfig()
{
 ret = TMR_paramSet(rp, TMR_PARAM_REGION_ID, &region);
 /* integers */

 ret = TMR_paramSet(rp, TMR_PARAM_BAUDRATE, &BaudRate);

 ret = TMR_paramSet(rp, TMR_PARAM_COMMANDTIMEOUT, &CommandTimeout);
 ret = TMR_paramSet(rp, TMR_PARAM_TRANSPORTTIMEOUT, &TransportTimeout);
 ret = TMR_paramSet(rp, TMR_PARAM_READ_ASYNCOFFTIME, &ASyncOffTime);
 ret = TMR_paramSet(rp, TMR_PARAM_READ_ASYNCONTIME, &ASyncOnTime);
 ret = TMR_paramSet(rp, TMR_PARAM_REGION_HOPTIME, &HopTime);
 ret = TMR_paramSet(rp, TMR_PARAM_GEN2_BLF, &Gen2BLF);
 ret = TMR_paramSet(rp, TMR_PARAM_READER_WRITE_REPLY_TIMEOUT, &Gen2WriteReplyTimeout);
 ret = TMR_paramSet(rp, TMR_PARAM_RADIO_READPOWER, &ReadPower);
 ret = TMR_paramSet(rp, TMR_PARAM_RADIO_WRITEPOWER, &WritePower);
 ret = TMR_paramSet(rp, TMR_PARAM_TAGOP_ANTENNA, &Antenna);
 ret = TMR_paramSet(rp, TMR_PARAM_TAGREADDATA_READFILTERTIMEOUT, &ReadFilterTimeout);

 /* integer arrays */

 InputList.list = arrayInputList;
 InputList.len = 4;
 ret = TMR_paramSet(rp, TMR_PARAM_GPIO_INPUTLIST, &InputList);
 PortSwitchGPOs.list = arrayPortSwitchGPOs;
 PortSwitchGPOs.len = 0;
 ret = TMR_paramSet(rp, TMR_PARAM_ANTENNA_PORTSWITCHGPOS, &PortSwitchGPOs);
 OutputList.list = arrayOutputList;
 OutputList.len = 0;
 ret = TMR_paramSet(rp, TMR_PARAM_GPIO_OUTPUTLIST, &OutputList);
 HopTable.list = arrayHopTable;
 HopTable.len = 0;
 ret = TMR_paramSet(rp, TMR_PARAM_REGION_HOPTABLE, &HopTable);

 /* integer arrays arrays */

 SettlingTime.list = arraySettlingTime;
 ret = TMR_paramSet(rp, TMR_PARAM_ANTENNA_SETTLINGTIMELIST, &SettlingTime);
 PortRead.list = arrayPortRead;
 ret = TMR_paramSet(rp, TMR_PARAM_RADIO_PORTREADPOWERLIST, &PortRead);
 PortWrite.list = arrayPortWrite;
 ret = TMR_paramSet(rp, TMR_PARAM_RADIO_PORTWRITEPOWERLIST, &PortWrite);
 txrxMap.list = arrayTxRxMap;
 ret = TMR_paramSet(rp, TMR_PARAM_ANTENNA_TXRXMAP, &txrxMap);
 /* strings */

 description.value = stringDescription;
 ret = TMR_paramSet(rp, TMR_PARAM_READER_DESCRIPTION, &description);

 /* booleans */

 ret = TMR_paramSet(rp, TMR_PARAM_ANTENNA_CHECKPORT, &CheckPort);
 ret = TMR_paramSet(rp, TMR_PARAM_READER_WRITE_EARLY_EXIT, &WriteEarlyExit);
 ret = TMR_paramSet(rp, TMR_PARAM_RADIO_ENABLEPOWERSAVE, &EnablePowerSave);
 ret = TMR_paramSet(rp, TMR_PARAM_REGION_LBT_ENABLE, &LBTEnable);
 ret = TMR_paramSet(rp, TMR_PARAM_STATUS_ENABLE_ANTENNAREPORT, &AntennaEnable);
 ret = TMR_paramSet(rp, TMR_PARAM_STATUS_ENABLE_FREQUENCYREPORT, &FrequencyEnable);
 ret = TMR_paramSet(rp, TMR_PARAM_STATUS_ENABLE_TEMPERATUREREPORT, &TemperatureEnable);
 ret = TMR_paramSet(rp, TMR_PARAM_TAGREADDATA_RECORDHIGHESTRSSI, &RecordHighestRSSI);
 ret = TMR_paramSet(rp, TMR_PARAM_TAGREADDATA_REPORTRSSIINDBM, &RSSIdBm);
 ret = TMR_paramSet(rp, TMR_PARAM_TAGREADDATA_UNIQUEBYANTENNA, &UniqueByAntenna);
 ret = TMR_paramSet(rp, TMR_PARAM_TAGREADDATA_UNIQUEBYDATA, &UniqueByData);
 ret = TMR_paramSet(rp, TMR_PARAM_RADIO_ENABLESJC, &EnableSJC);
 ret = TMR_paramSet(rp, TMR_PARAM_EXTENDEDEPC, &ExtendedEPC);
 ret = TMR_paramSet(rp, TMR_PARAM_TAGREADDATA_ENABLEREADFILTER, &EnableReadFilter);

 /* the rest */

 ret = TMR_paramSet(rp, TMR_PARAM_USERMODE, &userMode);
 ret = TMR_paramSet(rp, TMR_PARAM_POWERMODE, &powerMode);
 ret = TMR_paramSet(rp, TMR_PARAM_GEN2_ACCESSPASSWORD, &accessPassword);
 ret = TMR_paramSet(rp, TMR_PARAM_GEN2_TAGENCODING, &tagEncoding);
 ret = TMR_paramSet(rp, TMR_PARAM_GEN2_SESSION, &session);
 ret = TMR_paramSet(rp, TMR_PARAM_GEN2_TARGET, &target);
 ret = TMR_paramSet(rp, TMR_PARAM_GEN2_Q, &q);
 ret = TMR_paramSet(rp, TMR_PARAM_GEN2_TARI, &tari);
 ret = TMR_paramSet(rp, TMR_PARAM_TAGOP_PROTOCOL, &protocol);
 ret = TMR_paramSet(rp, TMR_PARAM_READ_PLAN, &readPlan);
 ret = TMR_paramSet(rp, TMR_PARAM_ISO180006B_BLF, &ISOBLF);
 ret = TMR_paramSet(rp, TMR_PARAM_ISO180006B_MODULATION_DEPTH, &modulation);
 ret = TMR_paramSet(rp, TMR_PARAM_ISO180006B_DELIMITER, &delimiter);
 ret = TMR_paramSet(rp, TMR_PARAM_GEN2_WRITEMODE, &writeMode);
 ret = TMR_paramSet(rp, TMR_PARAM_CURRENTTIME, &currentTime);

} 

void printUint8Array(TMR_uint8List *PassedArray)
{
    if (PassedArray->len > 64)
        PassedArray->len = 64;
    int i = 0;
    for (i = 0; i < PassedArray->len; i++)
    {
        printf(" %u", PassedArray->list[i]);
    }
    printf("\n");
}

void printUint32Array(TMR_uint32List *PassedArray)
{
    if (PassedArray->len > 64)
        PassedArray->len = 64;
    int i = 0;
    for (i = 0; i < PassedArray->len; i++)
    {
        printf(" %" PRIu32, PassedArray->list[i]);
    }
    printf("\n");
}

void printArraysOfArrays(TMR_PortValueList *PassedArray)
{
    int i = 0;
    printf("%" PRIu8 " %" PRIu8 "\n", PassedArray->len, PassedArray->max);
    for(i = 0; i < PassedArray->len; i++)
    {
        printf(" %" PRIu8 "%d", PassedArray->list[i].port, PassedArray->list[i].value);
    }
    printf("\n");
}

void printTxRxMap()
{
 int i = 0;
 for(i = 0; i < txrxMap.len; i++)
  printf("Antenna: %" PRIu8 " TX Port: %" PRIu8 " RX Port: %" PRIu8 "\n", txrxMap.list[i].antenna, txrxMap.list[i].txPort, txrxMap.list[i].rxPort);
}

void printReaderConfig()
{
 int i = 0;
 printf("/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\\n");
 /* integers */
 printf("Baud Rate: %" PRIu32 "\n", BaudRate);
 printf("Command Timeout: %" PRIu32 " milliseconds\n", CommandTimeout);
 printf("Transport Timeout: %" PRIu32 " milliseconds\n", TransportTimeout);
 printf("Async Off Time: %" PRIu32 " milliseconds\n", ASyncOffTime);
 printf("Async On Time: %" PRIu32 " milliseconds\n", ASyncOnTime);
 printf("Hop Time: %" PRIu32 " milliseconds\n", HopTime);
 printf("Read Filter Timeout: %" PRIu32 "\n", ReadFilterTimeout);
 printf("Gen2 Backscatter Link Frequency: %s\n", gen2LinkFrequencyNames[Gen2BLF]);
 printf("Gen2 Write Reply Timeout: %" PRIu16 " ms\n", Gen2WriteReplyTimeout);
 printf("Max Power: %" PRIu16 " centidBm\n", MaxPower);
 printf("Min Power: %" PRIu16 " centidBm\n", MinPower);
 printf("Read Power: %" PRIu16 " centidBm\n", ReadPower);
 printf("Write Power: %" PRIu16 " centidBm\n", WritePower);
 printf("Number of Successful Tag Operations: %" PRIu16 "\n", TagOpSuccess);
 printf("Number of Failed Tag Operations: %" PRIu16 "\n", TagOpFailures);
 printf("Product Group ID: %" PRIu16 "\n", ProductGroupID);
 printf("Antenna currently being used: %" PRIu8 "\n", Antenna);
 printf("Internal Temperature: %" PRIu8 " Celcius\n", Temperature);
 printf("========================================\n");

 /* strings */
 printf("URI: %s\n", ReaderURI.value);
 printf("Hardware Version: %s\n", HardwareVersion.value);
 printf("Software Version: %s\n", SoftwareVersion.value);
 printf("Model: %s\n", Model.value);
 printf("Product Group: %s\n", ProductGroup.value);
 printf("Serial Number: %s\n", SerialNumber.value);
 printf("Reader Description: %s\n", description.value);
 printf("========================================\n");

 /* booleans */
 printf("Check antenna port: %s\n", (CheckPort)?"true":"false");
 printf("Allow early exit: %s\n", (WriteEarlyExit)?"true":"false");
 printf("Enable power save: %s\n", (EnablePowerSave)?"true":"false");
 printf("Enable LBT: %s\n", (LBTEnable)?"true":"false");
 printf("Report antenna: %s\n", (AntennaEnable)?"true":"false");
 printf("Report frequency: %s\n", (FrequencyEnable)?"true":"false");
 printf("Report temperature: %s\n", (TemperatureEnable)?"true":"false");
 printf("Record highest RSSI from multiple tag reads: %s\n", (RecordHighestRSSI)?"true":"false");
 printf("RSSI in dBm: %s\n", (RSSIdBm)?"true":"false");
 printf("Unique by antenna: %s\n", (UniqueByAntenna)?"true":"false");
 printf("Unique by data: %s\n", (UniqueByData)?"true":"false");
 printf("Enable SJC: %s\n", (EnableSJC)?"true":"false");
 printf("Extended EPC: %s\n", (ExtendedEPC)?"true":"false");
 printf("Enable Read Filter: %s\n", (EnableReadFilter)?"true":"false");
 printf("========================================\n");

 /* integer arrays */
 printf("Connected Antenna Ports:");
 printUint8Array(&ConnectedPortList);
 printf("Available Antenna Ports:");
 printUint8Array(&PortList);
 printf("GPIO Pins Available for Port Switching:");
 printUint8Array(&PortSwitchGPOs);
 printf("Available GPIO Input Pins:");
 printUint8Array(&InputList);
 printf("Available GPIO Output Pins:");
 printUint8Array(&OutputList);
 printf("Available frequencies (kHz):");
 printUint32Array(&HopTable);
 printf("========================================\n");

 /* integer arrays of arrays */
 printf("Port Number/Settling Time:");
 if (SettlingTime.len)
   printArraysOfArrays(&SettlingTime);
 else printf("\n");
 printf("Port Number/Read Length:");
 if (PortRead.len)
   printArraysOfArrays(&PortRead);
 else printf("\n");
 printf("Port Number/Write Length:");
 if (PortWrite.len)
   printArraysOfArrays(&PortWrite);
 else printf("\n");
 printf("TX/RX Map:\n");
 if (txrxMap.len)
    printTxRxMap();
 else printf("No TxRxMap");
 printf("========================================\n");

 /* the rest */
 printf("Supported Regions:");
 for(i = 0; i < regionList.len - 1; i++)
 {
   printf(" %s%s", RegionList[regionList.list[i]], (i+1 == regionList.len - 1)?"\n":",");
 }
 if (region == TMR_REGION_OPEN)
    printf("Current Region: Unrestricted\n");
 else printf("Current Region: %s\n", RegionList[region]);
 printf("Power Mode: %s\n", PowerModeList[powerMode]);
 printf("User Mode: %s\n", UserModeList[userMode]);
 printf("Gen2 Access Password: 0x%" PRIx32 "\n", accessPassword);
 printf("Gen2 Tag Encoding Method: %s\n", tagEncodingNames[tagEncoding]);
 printf("Gen2 Session: %s\n", sessionNames[session]);
 printf("Gen2 Target Algorythm: %s\n", targetNames[target]);
 printf("Dynamic or Static Q Algorythm: %s\n", (q.type)?"Static":"Dynamic");
 printf("Tari: %s\n", tariNames[tari]);
 printf("Supported Protocols:");
 for(i = 0; i < supportedProtocols.len; i++)
 {
   printf(" %s%s", protocolNames[supportedProtocols.list[i]], (i+1 == supportedProtocols.len)?"\n":",");
 }
 printf("Current Protocol: %s\n", protocolNames[protocol]);
 if (readPlan.type != TMR_READ_PLAN_TYPE_MULTI)
 {
    printf("Read Plan:\n");
    printf("     Protocol: %s\n", protocolNames[readPlan.u.simple.protocol]);
    printf("     Antennas:");
    printUint8Array(&readPlan.u.simple.antennas);
    printf("     Filter  : ");
    if(TMR_FILTER_TYPE_TAG_DATA == readPlan.u.simple.filter)
       printf("Filter by Tag List\n");
    else if (TMR_FILTER_TYPE_GEN2_SELECT == readPlan.u.simple.filter)
       printf("Filter by Tag Operation\n");
    else if (NULL == readPlan.u.simple.filter)
       printf("No Filtering\n");
    else printf("Unknown Filtering Method\n");
    printf("     TagOp   : ");
    if (readPlan.u.simple.tagop == TMR_TAGOP_GEN2_READDATA)
       printf("Read Data from Tag\n");
    else if (readPlan.u.simple.tagop == TMR_TAGOP_GEN2_WRITEDATA)
       printf("Write Data to Tag\n");
    else if (readPlan.u.simple.tagop == TMR_TAGOP_GEN2_LOCK)
       printf("Lock Data in Tag\n");
    else if (readPlan.u.simple.tagop == TMR_TAGOP_GEN2_KILL)
       printf("Kill Tag\n");
    else if (readPlan.u.simple.tagop == TMR_TAGOP_LIST)
       printf("Multiple Tag Operations\n");
    else if (readPlan.u.simple.tagop == NULL)
       printf("No Tag Operation\n");
    else printf("Unknown Tag Operation\n");
    printf("     Weight  : %" PRIu32 "\n", readPlan.weight);
 }
 printf("ISO180006B BLF: %d kHz\n", ISOBLF);
 printf("ISO180006B Modulation Depth: %s\n", iso180006bModulationDepthNames[modulation]);
 printf("ISO180006B Delimiter: %s\n", iso180006bDelimiterNames[delimiter]);
 printf("Gen2 Write Mode: ");
 if (writeMode == TMR_GEN2_WORD_ONLY)
    printf("Word Only\n");
 else if (writeMode == TMR_GEN2_BLOCK_ONLY)
    printf("Block Only\n");
 else if (writeMode == TMR_GEN2_BLOCK_FALLBACK)
    printf("Block Fallback\n");
 printf("Reader Time: %s\n", value_data8);
 printf("========================================\n");
} 

void setupReader(char *arg)
{
  rp = &r;
  ret = TMR_create(rp, arg);
  region =  TMR_REGION_NA;
  ret = TMR_paramSet(rp, TMR_PARAM_REGION_ID, &region);
  if (TMR_READER_TYPE_SERIAL == rp->readerType)
    tlb.listener = serialPrinter;
  else
    tlb.listener = stringPrinter;
  tlb.cookie = stdout;
#if 0
  TMR_addTransportListener(rp, &tlb);
#endif
  ret = TMR_connect(rp);
  rlb.listener = callback;
  rlb.cookie = NULL;
  reb.listener = exceptionCallback;
  reb.cookie = NULL;
  ret = TMR_addReadListener(rp, &rlb);
  ret = TMR_addReadExceptionListener(rp, &reb);
}

void importReaderConfig()
{
   char str_currentTime[32];
   char str_year[5];
   char str_month[3];
   char str_day[3];
   char str_hour[3];
   char str_min[3];
   char str_sec[3];
   char str_antenna[32];
   char str_BaudRate[32];
   char str_commandTimeout[32];
   char str_transportTimeout[32];
   char str_EnableReadFilter[32];
   char str_ReadFilterTimeout[32];
   char str_ASyncOffTime[32];
   char str_ASyncOnTime[32];
   char str_ReadPower[32];
   char str_WritePower[32];
   char str_CheckAntenna[32];
   char str_PortSwitchGPOs[32];
   char str_InputList[32];
   char str_OutputList[32];
   char str_SettlingTime[32];
   char str_PortRead[32];
   char str_PortWrite[32];
   char str_TxRxMap[32];
   char str_AllowEarlyExit[32];
   char str_EnablePowerSave[32];
   char str_SJC[32];
   char str_ExtendedEPC[32];
   char str_HighestRSSI[32];
   char str_UniqueAntenna[32];
   char str_UniqueData[32];
   char str_PowerMode[32];
   char str_UserMode[32];
   char str_ReportAntenna[32];
   char str_ReportFrequency[32];
   char str_ReportTemperature[32];
   char str_WriteReplyTimeout[32];
   char str_WriteMode[32];
   char str_AccessPassword[32];
   char str_TagEncoding[32];
   char str_session[32];
   char str_target[32];
   char str_q[32];
   char str_tari[32];
   char str_readPlan[32];
   char str_ISOBLF[32];
   char str_modulation[32];
   char str_delimiter[32];
   char str_region[32];
   char str_hopTime[32];
   char str_HopTable[1024];
   char complete_string[1024];
   char temp[1024];
   FILE *infile;
   int i;
   
   infile = fopen("/var/www/radpy/Web/brominedump", "r");
   fscanf(infile, "%s", complete_string);
   fclose(infile);
   for (i = 0; i < strlen(complete_string); i++)
   {
      if (complete_string[i] == '&')
         complete_string[i] = ' ';
      if (complete_string[i] == '%' && complete_string[i+1] == '2' && complete_string[i+2] == 'F')
         complete_string[i] = '/';
      if (complete_string[i] == '%' && complete_string[i+1] == '3' && complete_string[i+2] == 'A')
         complete_string[i] = ':';
   }
   printf("%s\n", complete_string);
   sscanf(complete_string, "ReaderDescription=%s ReaderTime=%s CurrentAntenna=%s BaudRate=%s CommandTimeout=%s TransportTimeout=%s EnableReadFilter=%s ReadFilterTimeout=%s ASyncOffTime=%s \
 ASyncOnTime=%s ReadPower=%s WritePower=%s CheckAntenna=%s PortSwitchPins=%s InputPins=%s OutputPins=%s SettlingTime=%s PortRead=%s PortWrite=%s TxRxMap=%s EarlyExit=%s PowerSave=%s \
 SJC=%s ExtendedEPC=%s HighestRSSI=%s UniqueAntenna=%s UniqueData=%s PowerMode=%s UserMode=%s ReportAntenna=%s ReportFrequency=%s ReportTemperature=%s GEN2WriteReplyTimeout=%s \
 GEN2WriteMode=%s GEN2AccessPassword=%s GEN2TagEncoding=%s GEN2Session=%s GEN2Target=%s GEN2Q=%s Tari=%s ReadPlan=%s ISOBLF=%s ISOModulationDepth=%s ISODelimiter=%s CurrentRegion=%s \
 FrequencyHopTime=%s FrequencyHopTable=%s", stringDescription, str_currentTime, str_antenna, str_BaudRate, str_commandTimeout, str_transportTimeout, str_EnableReadFilter, str_ReadFilterTimeout,\
 str_ASyncOffTime, str_ASyncOnTime, str_ReadPower, str_WritePower, str_CheckAntenna, str_PortSwitchGPOs, str_InputList, str_OutputList, str_SettlingTime, str_PortRead, str_PortWrite,\
 str_TxRxMap, str_AllowEarlyExit, str_EnablePowerSave, str_SJC, str_ExtendedEPC, str_HighestRSSI, str_UniqueAntenna, str_UniqueData, str_PowerMode, str_UserMode, str_ReportAntenna,\
 str_ReportFrequency, str_ReportTemperature, str_WriteReplyTimeout, str_WriteMode, str_AccessPassword, str_TagEncoding, str_session, str_target, str_q, str_tari, str_readPlan, str_ISOBLF,\
 str_modulation, str_delimiter, str_region, str_hopTime, str_HopTable);
   if (strncmp(stringDescription, "ReaderTime", 10) == 0)
      {
          sscanf(complete_string, "%s ReaderTime=%s CurrentAntenna=%s BaudRate=%s CommandTimeout=%s TransportTimeout=%s EnableReadFilter=%s ReadFilterTimeout=%s ASyncOffTime=%s \
 ASyncOnTime=%s ReadPower=%s WritePower=%s CheckAntenna=%s PortSwitchPins=%s InputPins=%s OutputPins=%s SettlingTime=%s PortRead=%s PortWrite=%s TxRxMap=%s EarlyExit=%s PowerSave=%s \
 SJC=%s ExtendedEPC=%s HighestRSSI=%s UniqueAntenna=%s UniqueData=%s PowerMode=%s UserMode=%s ReportAntenna=%s ReportFrequency=%s ReportTemperature=%s GEN2WriteReplyTimeout=%s \
 GEN2WriteMode=%s GEN2AccessPassword=%s GEN2TagEncoding=%s GEN2Session=%s GEN2Target=%s GEN2Q=%s Tari=%s ReadPlan=%s ISOBLF=%s ISOModulationDepth=%s ISODelimiter=%s CurrentRegion=%s \
 FrequencyHopTime=%s FrequencyHopTable=%s", stringDescription, str_currentTime, str_antenna, str_BaudRate, str_commandTimeout, str_transportTimeout, str_EnableReadFilter, str_ReadFilterTimeout,\
 str_ASyncOffTime, str_ASyncOnTime, str_ReadPower, str_WritePower, str_CheckAntenna, str_PortSwitchGPOs, str_InputList, str_OutputList, str_SettlingTime, str_PortRead, str_PortWrite,\
 str_TxRxMap, str_AllowEarlyExit, str_EnablePowerSave, str_SJC, str_ExtendedEPC, str_HighestRSSI, str_UniqueAntenna, str_UniqueData, str_PowerMode, str_UserMode, str_ReportAntenna,\
 str_ReportFrequency, str_ReportTemperature, str_WriteReplyTimeout, str_WriteMode, str_AccessPassword, str_TagEncoding, str_session, str_target, str_q, str_tari, str_readPlan, str_ISOBLF,\
 str_modulation, str_delimiter, str_region, str_hopTime, str_HopTable);
         strcpy(stringDescription, "");
      }
   if (strncmp(str_PortSwitchGPOs, "InputPins", 9) == 0)
      {
          sscanf(complete_string, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s InputPins=%s OutputPins=%s SettlingTime=%s PortRead=%s PortWrite=%s TxRxMap=%s EarlyExit=%s PowerSave=%s \
 SJC=%s ExtendedEPC=%s HighestRSSI=%s UniqueAntenna=%s UniqueData=%s PowerMode=%s UserMode=%s ReportAntenna=%s ReportFrequency=%s ReportTemperature=%s GEN2WriteReplyTimeout=%s \
 GEN2WriteMode=%s GEN2AccessPassword=%s GEN2TagEncoding=%s GEN2Session=%s GEN2Target=%s GEN2Q=%s Tari=%s ReadPlan=%s ISOBLF=%s ISOModulationDepth=%s ISODelimiter=%s CurrentRegion=%s \
 FrequencyHopTime=%s FrequencyHopTable=%s", temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, str_PortSwitchGPOs, str_InputList, str_OutputList, str_SettlingTime,\
 str_PortRead, str_PortWrite, str_TxRxMap, str_AllowEarlyExit, str_EnablePowerSave, str_SJC, str_ExtendedEPC, str_HighestRSSI, str_UniqueAntenna, str_UniqueData, str_PowerMode, str_UserMode,\
 str_ReportAntenna, str_ReportFrequency, str_ReportTemperature, str_WriteReplyTimeout, str_WriteMode, str_AccessPassword, str_TagEncoding, str_session, str_target, str_q, str_tari, str_readPlan,\
 str_ISOBLF, str_modulation, str_delimiter, str_region, str_hopTime, str_HopTable);
         strcpy(str_PortSwitchGPOs, "");
      }
    if (strncmp(str_InputList, "OutputPins", 10) == 0)
      {
          sscanf(complete_string, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s OutputPins=%s SettlingTime=%s PortRead=%s PortWrite=%s TxRxMap=%s EarlyExit=%s PowerSave=%s \
 SJC=%s ExtendedEPC=%s HighestRSSI=%s UniqueAntenna=%s UniqueData=%s PowerMode=%s UserMode=%s ReportAntenna=%s ReportFrequency=%s ReportTemperature=%s GEN2WriteReplyTimeout=%s \
 GEN2WriteMode=%s GEN2AccessPassword=%s GEN2TagEncoding=%s GEN2Session=%s GEN2Target=%s GEN2Q=%s Tari=%s ReadPlan=%s ISOBLF=%s ISOModulationDepth=%s ISODelimiter=%s CurrentRegion=%s \
 FrequencyHopTime=%s FrequencyHopTable=%s", temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, str_InputList, str_OutputList, str_SettlingTime,\
 str_PortRead, str_PortWrite, str_TxRxMap, str_AllowEarlyExit, str_EnablePowerSave, str_SJC, str_ExtendedEPC, str_HighestRSSI, str_UniqueAntenna, str_UniqueData, str_PowerMode, str_UserMode,\
 str_ReportAntenna, str_ReportFrequency, str_ReportTemperature, str_WriteReplyTimeout, str_WriteMode, str_AccessPassword, str_TagEncoding, str_session, str_target, str_q, str_tari, str_readPlan,\
 str_ISOBLF, str_modulation, str_delimiter, str_region, str_hopTime, str_HopTable);
         strcpy(str_InputList, "");
      }
     if (strncmp(str_OutputList, "SettlingTime", 12) == 0)
      {
          sscanf(complete_string, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s SettlingTime=%s PortRead=%s PortWrite=%s TxRxMap=%s EarlyExit=%s PowerSave=%s \
 SJC=%s ExtendedEPC=%s HighestRSSI=%s UniqueAntenna=%s UniqueData=%s PowerMode=%s UserMode=%s ReportAntenna=%s ReportFrequency=%s ReportTemperature=%s GEN2WriteReplyTimeout=%s \
 GEN2WriteMode=%s GEN2AccessPassword=%s GEN2TagEncoding=%s GEN2Session=%s GEN2Target=%s GEN2Q=%s Tari=%s ReadPlan=%s ISOBLF=%s ISOModulationDepth=%s ISODelimiter=%s CurrentRegion=%s \
 FrequencyHopTime=%s FrequencyHopTable=%s", temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, str_OutputList, str_SettlingTime, str_PortRead, str_PortWrite,\
 str_TxRxMap, str_AllowEarlyExit, str_EnablePowerSave, str_SJC, str_ExtendedEPC, str_HighestRSSI, str_UniqueAntenna, str_UniqueData, str_PowerMode, str_UserMode, str_ReportAntenna,\
 str_ReportFrequency, str_ReportTemperature, str_WriteReplyTimeout, str_WriteMode, str_AccessPassword, str_TagEncoding, str_session, str_target, str_q, str_tari, str_readPlan, str_ISOBLF,\
 str_modulation, str_delimiter, str_region, str_hopTime, str_HopTable);
         strcpy(str_OutputList, "");
      }
     if (strncmp(str_SettlingTime, "PortRead", 8) == 0)
      {
          sscanf(complete_string, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s PortRead=%s PortWrite=%s TxRxMap=%s EarlyExit=%s PowerSave=%s \
 SJC=%s ExtendedEPC=%s HighestRSSI=%s UniqueAntenna=%s UniqueData=%s PowerMode=%s UserMode=%s ReportAntenna=%s ReportFrequency=%s ReportTemperature=%s GEN2WriteReplyTimeout=%s \
 GEN2WriteMode=%s GEN2AccessPassword=%s GEN2TagEncoding=%s GEN2Session=%s GEN2Target=%s GEN2Q=%s Tari=%s ReadPlan=%s ISOBLF=%s ISOModulationDepth=%s ISODelimiter=%s CurrentRegion=%s \
 FrequencyHopTime=%s FrequencyHopTable=%s", temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, str_SettlingTime, str_PortRead, str_PortWrite,\
 str_TxRxMap, str_AllowEarlyExit, str_EnablePowerSave, str_SJC, str_ExtendedEPC, str_HighestRSSI, str_UniqueAntenna, str_UniqueData, str_PowerMode, str_UserMode, str_ReportAntenna,\
 str_ReportFrequency, str_ReportTemperature, str_WriteReplyTimeout, str_WriteMode, str_AccessPassword, str_TagEncoding, str_session, str_target, str_q, str_tari, str_readPlan, str_ISOBLF,\
 str_modulation, str_delimiter, str_region, str_hopTime, str_HopTable);
         strcpy(str_SettlingTime, "");
      }
     if (strncmp(str_PortRead, "PortWrite", 9) == 0)
      {
          sscanf(complete_string, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s PortWrite=%s TxRxMap=%s EarlyExit=%s PowerSave=%s SJC=%s ExtendedEPC=%s HighestRSSI=%s UniqueAntenna=%s \
 UniqueData=%s PowerMode=%s UserMode=%s ReportAntenna=%s ReportFrequency=%s ReportTemperature=%s GEN2WriteReplyTimeout=%s GEN2WriteMode=%s GEN2AccessPassword=%s GEN2TagEncoding=%s GEN2Session=%s \
 GEN2Target=%s GEN2Q=%s Tari=%s ReadPlan=%s ISOBLF=%s ISOModulationDepth=%s ISODelimiter=%s CurrentRegion=%s FrequencyHopTime=%s FrequencyHopTable=%s", temp, temp, temp, temp, temp, temp, temp, temp,\
 temp, temp, temp, temp, temp, temp, temp, temp, temp, str_PortRead, str_PortWrite, str_TxRxMap, str_AllowEarlyExit, str_EnablePowerSave, str_SJC, str_ExtendedEPC, str_HighestRSSI, str_UniqueAntenna,\
 str_UniqueData, str_PowerMode, str_UserMode, str_ReportAntenna, str_ReportFrequency, str_ReportTemperature, str_WriteReplyTimeout, str_WriteMode, str_AccessPassword, str_TagEncoding, str_session,\
 str_target, str_q, str_tari, str_readPlan, str_ISOBLF, str_modulation, str_delimiter, str_region, str_hopTime, str_HopTable);
         strcpy(str_PortRead, "");
      }
     if (strncmp(str_PortWrite, "TxRxMap", 7) == 0)
      {
          sscanf(complete_string, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s TxRxMap=%s EarlyExit=%s PowerSave=%s SJC=%s ExtendedEPC=%s HighestRSSI=%s UniqueAntenna=%s \
 UniqueData=%s PowerMode=%s UserMode=%s ReportAntenna=%s ReportFrequency=%s ReportTemperature=%s GEN2WriteReplyTimeout=%s GEN2WriteMode=%s GEN2AccessPassword=%s GEN2TagEncoding=%s GEN2Session=%s \
 GEN2Target=%s GEN2Q=%s Tari=%s ReadPlan=%s ISOBLF=%s ISOModulationDepth=%s ISODelimiter=%s CurrentRegion=%s FrequencyHopTime=%s FrequencyHopTable=%s", temp, temp, temp, temp, temp, temp, temp, temp,\
 temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, str_PortWrite, str_TxRxMap, str_AllowEarlyExit, str_EnablePowerSave, str_SJC, str_ExtendedEPC, str_HighestRSSI, str_UniqueAntenna,\
 str_UniqueData, str_PowerMode, str_UserMode, str_ReportAntenna, str_ReportFrequency, str_ReportTemperature, str_WriteReplyTimeout, str_WriteMode, str_AccessPassword, str_TagEncoding, str_session,\
 str_target, str_q, str_tari, str_readPlan, str_ISOBLF, str_modulation, str_delimiter, str_region, str_hopTime, str_HopTable);
         strcpy(str_PortWrite, "");
      }
     if (strncmp(str_TxRxMap, "EarlyExit", 9) == 0)
      {
          sscanf(complete_string, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s EarlyExit=%s PowerSave=%s SJC=%s ExtendedEPC=%s HighestRSSI=%s UniqueAntenna=%s \
 UniqueData=%s PowerMode=%s UserMode=%s ReportAntenna=%s ReportFrequency=%s ReportTemperature=%s GEN2WriteReplyTimeout=%s GEN2WriteMode=%s GEN2AccessPassword=%s GEN2TagEncoding=%s GEN2Session=%s \
 GEN2Target=%s GEN2Q=%s Tari=%s ReadPlan=%s ISOBLF=%s ISOModulationDepth=%s ISODelimiter=%s CurrentRegion=%s FrequencyHopTime=%s FrequencyHopTable=%s", temp, temp, temp, temp, temp, temp, temp, temp,\
 temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, str_TxRxMap, str_AllowEarlyExit, str_EnablePowerSave, str_SJC, str_ExtendedEPC, str_HighestRSSI, str_UniqueAntenna, str_UniqueData,\
 str_PowerMode, str_UserMode, str_ReportAntenna, str_ReportFrequency, str_ReportTemperature, str_WriteReplyTimeout, str_WriteMode, str_AccessPassword, str_TagEncoding, str_session, str_target, str_q,\
 str_tari, str_readPlan, str_ISOBLF, str_modulation, str_delimiter, str_region, str_hopTime, str_HopTable);
         strcpy(str_TxRxMap, "");
      }
     if (strncmp(str_readPlan, "ISOBLF", 6) == 0)
      {
          sscanf(complete_string, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s ISOBLF=%s ISOModulationDepth=%s \
 ISODelimiter=%s CurrentRegion=%s FrequencyHopTime=%s FrequencyHopTable=%s", temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp,\
 temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, temp, str_readPlan, str_ISOBLF, str_modulation, str_delimiter, str_region,\
 str_hopTime, str_HopTable);
         strcpy(str_readPlan, "");
      }


/* settlingtime
portread
portwrite
txrxmap
readplan
*/
 
/* integers */

   BaudRate = atoi(str_BaudRate);
   CommandTimeout = atoi(str_commandTimeout);
   TransportTimeout = atoi(str_transportTimeout);
   ASyncOffTime = atoi(str_ASyncOffTime);
   ASyncOnTime = atoi(str_ASyncOnTime);
   HopTime = atoi(str_hopTime);
   Gen2WriteReplyTimeout = atoi(str_WriteReplyTimeout);
   ReadPower = atoi(str_ReadPower);
   WritePower = atoi(str_WritePower);
   Antenna = atoi(str_antenna);
   ReadFilterTimeout = atoi(str_ReadFilterTimeout);
   accessPassword = atoi(str_AccessPassword);
   ISOBLF = atoi(str_ISOBLF);

/* booleans */


   CheckPort = atoi(str_CheckAntenna);
   WriteEarlyExit = atoi(str_AllowEarlyExit);
   EnablePowerSave = atoi(str_EnablePowerSave);
   AntennaEnable = atoi(str_ReportAntenna);
   FrequencyEnable = atoi(str_ReportFrequency);
   TemperatureEnable = atoi(str_ReportTemperature);
   RecordHighestRSSI = atoi(str_HighestRSSI);
   UniqueByAntenna = atoi(str_UniqueAntenna);
   UniqueByData = atoi(str_UniqueData);
   EnableSJC = atoi(str_SJC);
   ExtendedEPC = atoi(str_ExtendedEPC);
   EnableReadFilter = atoi(str_EnableReadFilter);

/* regular string */

      for(i = 0; i < strlen(stringDescription); i++)

      {
         if(stringDescription[i] == '+')
            stringDescription[i] = ' ';
      }  
      for(i = 0; i < strlen(str_WriteMode); i++)

      {
         if(str_WriteMode[i] == '+')
            str_WriteMode[i] = ' ';
      }  


/* time */

      for(i = 0; i < strlen(str_currentTime); i++)
      {
         if(str_currentTime[i] == '/' || str_currentTime[i] == ':' || str_currentTime[i] == '+')
            str_currentTime[i] = ' ';
      }  
      sscanf(str_currentTime, "%s 2F%s 2F%s %s 3A%s 3A%s", str_year, str_month, str_day, str_hour, str_min, str_sec);
      currentTime.tm_year = atoi(str_year)-1970;
      currentTime.tm_mon = atoi(str_month);
      currentTime.tm_mday = atoi(str_day);
      currentTime.tm_hour = atoi(str_hour);
      currentTime.tm_min = atoi(str_min);
      currentTime.tm_sec = atoi(str_sec);

/* arrays */
/*
      i = 0; // 1st char of current block
      j = 0; // whitespace following current block
      k = 0; // block number
      while (str_InputList[j] != '\0')
      {
         while (str_InputList[j] != ' ')
            j++;
         while (str_InputList[i] == ' ')
            i++;
         strncpy(str_str_InputList, &str_InputList[i], j-i);
         str_str_InputList[j-i] = '\0';
         arrayInputList[k] = atoi(str_str_InputList);
         k++;
         i++;
         j++;
      }

      i = 0; // 1st char of current block
      j = 0; // whitespace following current block
      k = 0; // block number
      while (str_OutputList[j] != '\0')
      {
         while (str_OutputList[j] != ' ')
            j++;
         while (str_OutputList[i] == ' ')
            i++;
         strncpy(str_str_OutputList, &str_OutputList[i], j-i);
         str_str_OutputList[j-i] = '\0';
         arrayOutputList[k] = atoi(str_str_OutputList);
         k++;
      }

      i = 0; // 1st char of current block
      j = 0; // whitespace following current block
      k = 0; // block number
      while (str_PortSwitchGPOs[j] != '\0')
      {
         while (str_PortSwitchGPOs[j] != ' ')
            j++;
         while (str_PortSwitchGPOs[i] == ' ')
            i++;
         strncpy(str_str_PortSwitchGPOs, &str_PortSwitchGPOs[i], j-i);
         str_str_PortSwitchGPOs[j-i] = '\0';
         arrayPortSwitchGPOs[k] = atoi(str_str_PortSwitchGPOs);
         k++;
      }

      i = 0; // 1st char of current block
      j = 0; // whitespace following current block
      k = 0; // block number
      while (str_HopTable[j] != '\0')
      {
         while (str_HopTable[j] != ' ')
            j++;
         while (str_HopTable[i] == ' ')
            i++;
         strncpy(str_str_HopTable, &str_HopTable[i], j-i);
         str_str_HopTable[j-i] = '\0';
         arrayHopTable[k] = atoi(str_str_HopTable);
         k++;
      }
*/
/* picklists */
/*      if (str_region == RegionList[0])
         region = TMR_REGION_NONE;
         else if (str_region == RegionList[1])
              region = TMR_REGION_NA;
              else if (str_region == RegionList[2])
                   region = TMR_REGION_EU;
                   else if (str_region == RegionList[3])
                        region = TMR_REGION_KR;
                        else if (str_region == RegionList[4])
                             region = TMR_REGION_IN;
                             else if (str_region == RegionList[5])
                                  region = TMR_REGION_JP;
                                  else if (str_region == RegionList[6])
                                       region = TMR_REGION_PRC;
                                       else if (str_region == RegionList[7])
                                            region = TMR_REGION_EU2;
                                            else if (str_region == RegionList[8])
                                                 region = TMR_REGION_EU3;
                                                 else if (str_region == RegionList[9])
                                                      region = TMR_REGION_KR2;
                                                      else if (str_region == RegionList[10])
                                                           region = TMR_REGION_PRC2;
                                                           else if (str_region == RegionList[11])
                                                                region = TMR_REGION_AU;
                                                                else if (str_region == RegionList[12])
                                                                     region = TMR_REGION_NZ;
                                                                     else if (str_region == RegionList[13])
                                                                          region = TMR_REGION_OPEN;
*/
       if (strcmp(str_UserMode, UserModeList[0]) == 0)
         userMode = TMR_SR_USER_MODE_UNSPEC;
         else if (strcmp(str_UserMode, UserModeList[1]) == 0)
              userMode = TMR_SR_USER_MODE_PRINTER;
              else if (strcmp(str_UserMode, UserModeList[2]) == 0)
                   userMode = TMR_SR_USER_MODE_CONVEYOR;
                   else if (strcmp(str_UserMode, UserModeList[3]) == 0)
                        userMode = TMR_SR_USER_MODE_PORTAL;
                        else if (strcmp(str_UserMode, UserModeList[4]) == 0)
                             userMode = TMR_SR_USER_MODE_HANDHELD;
       if (strcmp(str_PowerMode, PowerModeList[0]) == 0)
         powerMode = 0;
         else if (strcmp(str_PowerMode, PowerModeList[1]) == 0)
              powerMode = 1;
              else if (strcmp(str_PowerMode, PowerModeList[2]) == 0)
                   powerMode = 2;
                   else if (strcmp(str_PowerMode, PowerModeList[3]) == 0)
                        powerMode = 3;
                        else if (strcmp(str_PowerMode, PowerModeList[4]) == 0)
                             powerMode = 4;
       if (strcmp(str_TagEncoding ,tagEncodingNames[0]) == 0)
         tagEncoding = TMR_GEN2_FM0;
         else if (strcmp(str_TagEncoding, tagEncodingNames[1]) == 0)
              tagEncoding = TMR_GEN2_MILLER_M_2;
              else if (strcmp(str_TagEncoding, tagEncodingNames[2]) == 0)
                   tagEncoding = TMR_GEN2_MILLER_M_4;
                   else if (strcmp(str_TagEncoding, tagEncodingNames[3]) == 0)
                        tagEncoding = TMR_GEN2_MILLER_M_8;
        if (strcmp(str_session, sessionNames[0]) == 0)
         session = TMR_GEN2_SESSION_S0;
         else if (strcmp(str_session, sessionNames[1]) == 0)
              session = TMR_GEN2_SESSION_S1;
              else if (strcmp(str_session, sessionNames[2]) == 0)
                   session = TMR_GEN2_SESSION_S2;
                   else if (strcmp(str_session, sessionNames[3]) == 0)
                        session = TMR_GEN2_SESSION_S3;
         if (strcmp(str_target, targetNames[0]) == 0)
         target = TMR_GEN2_TARGET_A;
         else if (strcmp(str_target, targetNames[1]) == 0)
              target = TMR_GEN2_TARGET_B;
              else if (strcmp(str_target, targetNames[2]) == 0)
                   target = TMR_GEN2_TARGET_AB;
                   else if (strcmp(str_target, targetNames[3]) == 0)
                        target = TMR_GEN2_TARGET_BA;
         if (strcmp(str_q, "Dynamic") == 0)
         q.type = TMR_SR_GEN2_Q_DYNAMIC;
         else if (strcmp(str_q, "Static") == 0)
              q.type = TMR_SR_GEN2_Q_STATIC;
         if (strcmp(str_WriteMode, "Word Only\0") == 0) 
            writeMode = TMR_GEN2_WORD_ONLY;
         else if (strcmp(str_WriteMode, "Block Only\0") == 0)
                 writeMode = TMR_GEN2_BLOCK_ONLY;
              else if (strcmp(str_WriteMode, "Block Fallback\0") == 0)
                      writeMode = TMR_GEN2_BLOCK_FALLBACK;
         printf("%d\n", writeMode);
         if (strcmp(str_tari, tariNames[0]) == 0)
         tari = TMR_GEN2_TARI_25US;
         else if (strcmp(str_tari, tariNames[1]) == 0)
              tari = TMR_GEN2_TARI_12_5US;
              else if (strcmp(str_tari, tariNames[2]) == 0)
                   tari = TMR_GEN2_TARI_6_25US;
         if (strcmp(str_delimiter, "Delimiter1") == 0)
         delimiter = TMR_ISO180006B_Delimiter1;
         else if (strcmp(str_delimiter, "Delimiter4") == 0)
              delimiter = TMR_ISO180006B_Delimiter4;
         if (strcmp(str_modulation, "99") == 0)
         modulation = TMR_ISO180006B_ModulationDepth99percent;
         else if (strcmp(str_modulation, "11") == 0)
              modulation = TMR_ISO180006B_ModulationDepth11percent;
}

void initVars()
{
   strcpy(stringDescription,"" );
   BaudRate = 9600;
   CommandTimeout = 0;
   TransportTimeout = 0;
   ASyncOffTime = 0;
   ASyncOnTime = 0;
   HopTime = 0;
   ReadFilterTimeout = 0;
   Gen2BLF = 0;
   Gen2WriteReplyTimeout = 0;
   ReadPower = 0;
   WritePower = 0;
   Antenna = 0;
   CheckPort = 0;
   WriteEarlyExit = 0;
   EnablePowerSave = 0;
   LBTEnable = 0;
   AntennaEnable = 0;
   FrequencyEnable = 0;
   TemperatureEnable = 0;
   RecordHighestRSSI = 0;
   RSSIdBm = 0;
   UniqueByAntenna = 0;
   UniqueByData = 0;
   EnableSJC = 0;
   EnableReadFilter = 0;
   ExtendedEPC = 0;
   region = TMR_REGION_NA; 
}

int main(int argc, char *argv[])
{

#ifndef TMR_ENABLE_BACKGROUND_READS
  errx(1, "This sample requires background read functionality.\n"
          "Please enable TMR_ENABLE_BACKGROUND_READS in tm_config.h\n"
          "to run this code.\n");
  return -1;
#else
/*  if (argc < 2)
  {
    errx(1, "Please provide reader URL, such as:\n"
           "tmr:///com4\n"
           "tmr://my-reader.example.com\n");
  }*/
  initVars();
  setupReader("tmr:///dev/ttyACM0");
  importReaderConfig();
  setReaderConfig();
  printReaderConfig();
  TMR_destroy(rp);
  return 0;
#endif /* TMR_ENABLE_BACKGROUND_READS */
}
