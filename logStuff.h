/*
    logging macros, for my personal sanity...
*/

#ifndef LOGSTUFF_H
#define LOGSTUFF_H

#include <syslog.h>
#include <dlfcn.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    no  = 0, off = 0,
    yes = 1, on = 1
} tBool;

typedef enum
{
    kLogEmergency = LOG_EMERG,    /* 0: system is unusable */
    kLogAlert     = LOG_ALERT,    /* 1: action must be taken immediately */
    kLogCritical  = LOG_CRIT,     /* 2: critical conditions */
    kLogError     = LOG_ERR,      /* 3: error conditions */
    kLogWarning   = LOG_WARNING,  /* 4: warning conditions */
    kLogNotice    = LOG_NOTICE,   /* 5: normal but significant condition */
    kLogInfo      = LOG_INFO,     /* 6: informational */
    kLogDebug     = LOG_DEBUG,    /* 7: debug-level messages */
    kLogFunctions,                /* (used for function entry/exit logging */
    kLogMaxPriotity
} eLogPriority;

typedef enum
{
    kLogToTheVoid,
    kLogToSyslog,
    kLogToFile,
    kLogToStderr,
    kLogMaxDestination
} eLogDestination;

typedef enum
{
    kLogNothing = 0,
    kLogNormal,
    kLogWithLocation
} tLogMode;

/* set up the logging mechanisms. Call once, very early. */
void initLogStuff( const char * name );

/* tidy up the current logging mechanism */
void stopLoggingStuff( void );

void setLogStuffDestination( eLogPriority priority, eLogDestination logDestination );

/* start/stop logging function entry & exit */
void logFunctionTrace( tBool onOff );

/* private helper, which the preprocessor macros expand to. Please don't use directly! */
void _log( const char * inFile,
           unsigned int atLine,
           const char * inFunction,
           error_t error,
           eLogPriority priority,
           const char * format,
           ... )
__attribute__((__format__ (__printf__, 6, 7)))
__attribute__((no_instrument_function));

#define log( priority, ... ) \
    do { _log( __FILE__, __LINE__, __func__, errno, priority, __VA_ARGS__ ); } while (0)

#define logEmergency( ... )  log( kLogEmergency, __VA_ARGS__ )
#define logAlert( ... )      log( kLogAlert,     __VA_ARGS__ )
#define logCritical( ... )   log( kLogCritical,  __VA_ARGS__ )
#define logError( ... )      log( kLogError,     __VA_ARGS__ )
#define logWarning( ... )    log( kLogWarning,   __VA_ARGS__ )
#define logNotice( ... )     log( kLogNotice,    __VA_ARGS__ )
#define logInfo( ... )       log( kLogInfo,      __VA_ARGS__ )

#ifdef DEBUG
#define logDebug(...)      log( kLogDebug, __VA_ARGS__ )
#define logCheckpoint()    do { _log( __FILE__, __LINE__, kLogDebug, "reached" ); } while (0)
#else
#define logDebug( ... )      do {} while (0)
#define logCheckpoint()      do {} while (0)
#define logSetErrno( val )   do {} while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif
