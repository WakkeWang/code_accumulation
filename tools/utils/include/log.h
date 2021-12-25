#ifndef __LOG__
#define __LOG__

enum LOG_LEVEL
{
    LOG_LEVEL_OFF = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4,
    LOG_LEVEL_ALL = 5,
};

extern int log_level;

#define LOG_DEBUG(format,...)	\
do {							\
	if (log_level >= LOG_LEVEL_DEBUG) {	\
		printf("[%20s -- %20s -- %5d] [DEBUG]  "format"\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__);	\
	}	\
} while(0)
 
#define LOG_INFO(format,...)	\
do {							\
	if (log_level >= LOG_LEVEL_INFO) {	\
		printf("[%20s -- %20s -- %5d] [ INFO]  "format"\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__);	\
	}	\
} while(0)
 
#define LOG_WARN(format,...)	\
do {							\
	if (log_level >= LOG_LEVEL_WARN) {	\
		printf("[%20s -- %20s -- %5d] [ WARN]  "format"\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__);	\
	}	\
} while(0)
 
#define LOG_ERROR(format,...)	\
do {							\
	if (log_level >= LOG_LEVEL_ERROR) {	\
		printf("[%20s -- %20s -- %5d] [ERROR]  "format"\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__);	\
	}	\
} while(0)

#endif
