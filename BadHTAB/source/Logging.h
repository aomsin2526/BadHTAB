extern FILE *LoggingFile1;
extern FILE *LoggingFile2;

extern char printToFileBuf[64 * 1024];

#define PrintLog(...)                                                        \
	{                                                                        \
		sprintf(printToFileBuf, __VA_ARGS__);                                \
		printf(printToFileBuf);                                              \
		if (LoggingFile1 != NULL)                                            \
		{                                                                    \
			fwrite(printToFileBuf, 1, strlen(printToFileBuf), LoggingFile1); \
			fflush(LoggingFile1);                                            \
		}                                                                    \
		if (LoggingFile2 != NULL)                                            \
		{                                                                    \
			fwrite(printToFileBuf, 1, strlen(printToFileBuf), LoggingFile2); \
			fflush(LoggingFile2);                                            \
		}                                                                    \
	}

extern void InitLogging();
extern void DestroyLogging();