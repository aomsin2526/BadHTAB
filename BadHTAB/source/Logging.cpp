#include "Include.h"

FILE *LoggingFile1 = NULL;
FILE *LoggingFile2 = NULL;

char printToFileBuf[64 * 1024];

void InitLogging()
{
	LoggingFile1 = fopen("/app_home/BadHTAB.txt", "wb");
	LoggingFile2 = fopen("/dev_hdd0/BadHTAB.txt", "wb");
}

void DestroyLogging()
{
	if (LoggingFile2 != NULL)
	{
		fflush(LoggingFile2);
		fclose(LoggingFile2);
		LoggingFile2 = NULL;
	}

	if (LoggingFile1 != NULL)
	{
		fflush(LoggingFile1);
		fclose(LoggingFile1);
		LoggingFile1 = NULL;
	}
}