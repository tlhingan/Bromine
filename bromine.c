#include <stdio.h>
#include <string.h>

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
char str_str_PortSwitchGPOs[32];
char str_InputList[32];
char str_str_InputList[32];
char str_OutputList[32];
char str_str_OutputList[32];
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
char str_HopTable[32];
char str_str_HopTable[32];
char str_description[32];

void ReceiveData()
{
   char *incoming;
   int i;

   incoming = NULL;
   printf("Content-Type:text/plain;charset=us-ascii\n\n");
   incoming = getenv("QUERY_STRING");
   for(i = 0; i < strlen(incoming); i++)   
   {
      if(incoming[i] == '&')
        incoming[i] = ' ';
   }

   sscanf(incoming, "ReaderDescription=%s ReaderTime=%s CurrentAntenna=%s BaudRate=%s CommandTimeout=%s TransportTimeout=%s EnableReadFilter=%s ReadFilterTimeout=%s ASyncOffTime=%s \
     ASyncOnTime=%s ReadPower=%s WritePower=%s CheckAntenna=%s PortSwitchPins=%s InputPins=%s OutputPins=%s SettlingTime=%s PortRead=%s PortWrite=%s TxRxMap=%s EarlyExit=%s PowerSave=%s \
     SJC=%s ExtendedEPC=%s HighestRSSI=%s UniqueAntenna=%s UniqueData=%s PowerMode=%s UserMode=%s ReportAntenna=%s ReportFrequency=%s ReportTemperature=%s GEN2WriteReplyTimeout=%s \
     GEN2WriteMode=%s GEN2AccessPassword=%s GEN2TagEncoding=%s GEN2Session=%s GEN2Target=%s GEN2Q=%s Tari=%s ReadPlan=%s ISOBLF=%s ISOModulationDepth=%s ISODelimiter=%s CurrentRegion=%s \
     FrequencyHopTime=%s FrequencyHopTable=%s", str_description, str_currentTime, str_antenna, str_BaudRate, str_commandTimeout, str_transportTimeout, str_EnableReadFilter, str_ReadFilterTimeout,\
     str_ASyncOffTime, str_ASyncOnTime, str_ReadPower, str_WritePower, str_CheckAntenna, str_PortSwitchGPOs, str_InputList, str_OutputList, str_SettlingTime, str_PortRead, str_PortWrite,\
     str_TxRxMap, str_AllowEarlyExit, str_EnablePowerSave, str_SJC, str_ExtendedEPC, str_HighestRSSI, str_UniqueAntenna, str_UniqueData, str_PowerMode, str_UserMode, str_ReportAntenna,\
     str_ReportFrequency, str_ReportTemperature, str_WriteReplyTimeout, str_WriteMode, str_AccessPassword, str_TagEncoding, str_session, str_target, str_q, str_tari, str_readPlan, str_ISOBLF,\
     str_modulation, str_delimiter, str_region, str_hopTime, str_HopTable);
}

void WriteData()
{

   FILE *outfile;
   outfile = fopen("brominedump", "w");
   fprintf(outfile, "ReaderDescription=%s ReaderTime=%s CurrentAntenna=%s BaudRate=%s CommandTimeout=%s TransportTimeout=%s EnableReadFilter=%s ReadFilterTimeout=%s ASyncOffTime=%s \
ASyncOnTime=%s ReadPower=%s WritePower=%s CheckAntenna=%s PortSwitchPins=%s InputPins=%s OutputPins=%s SettlingTime=%s PortRead=%s PortWrite=%s TxRxMap=%s EarlyExit=%s PowerSave=%s \
SJC=%s ExtendedEPC=%s HighestRSSI=%s UniqueAntenna=%s UniqueData=%s PowerMode=%s UserMode=%s ReportAntenna=%s ReportFrequency=%s ReportTemperature=%s GEN2WriteReplyTimeout=%s \
GEN2WriteMode=%s GEN2AccessPassword=%s GEN2TagEncoding=%s GEN2Session=%s GEN2Target=%s GEN2Q=%s Tari=%s ReadPlan=%s ISOBLF=%s ISOModulationDepth=%s ISODelimiter=%s CurrentRegion=%s \
FrequencyHopTime=%s FrequencyHopTable=%s", str_description, str_currentTime, str_antenna, str_BaudRate, str_commandTimeout, str_transportTimeout, str_EnableReadFilter, str_ReadFilterTimeout,\
      str_ASyncOffTime, str_ASyncOnTime, str_ReadPower, str_WritePower, str_CheckAntenna, str_PortSwitchGPOs, str_InputList, str_OutputList, str_SettlingTime, str_PortRead, str_PortWrite,\
      str_TxRxMap, str_AllowEarlyExit, str_EnablePowerSave, str_SJC, str_ExtendedEPC, str_HighestRSSI, str_UniqueAntenna, str_UniqueData, str_PowerMode, str_UserMode, str_ReportAntenna,\
      str_ReportFrequency, str_ReportTemperature, str_WriteReplyTimeout, str_WriteMode, str_AccessPassword, str_TagEncoding, str_session, str_target, str_q, str_tari, str_readPlan, str_ISOBLF,\
      str_modulation, str_delimiter, str_region, str_hopTime, str_HopTable);

      fclose(outfile);
}

int main (int argc, char *argv[])
{
   ReceiveData();
   WriteData();
   return 0;
}
