/*
    logging macros, for my personal sanity...
*/

#ifndef LOGGING_H
#define LOGGING_H

#include    <syslog.h>

typedef enum { false = 0, no = 0, off = 0,
               true = 1, yes = 1,  on = 1 } tBool;

#ifdef USE_LOG_SCOPING
/* this is dynamically built by the Makefile */
#include "obj/logscopes.inc"

typedef struct {
    unsigned int    level;
    unsigned int    max;
    unsigned char  *site;
} gLogEntry;

extern gLogEntry gLog[kMaxLogScope];
#endif

typedef enum {
    kLogToUndefined,
    kLogToSyslog,
    kLogToFile,
    kLogToStderr
} eLogDestination;

typedef enum {
    kLogEmergency = LOG_EMERG,
    kLogAlert     = LOG_ALERT,
    kLogCritical  = LOG_CRIT,
    kLogError     = LOG_ERR,
    kLogWarning   = LOG_WARNING,
    kLogNotice    = LOG_NOTICE,
    kLogInfo      = LOG_INFO,
    kLogDebug     = LOG_DEBUG
} eLogLevel;

/* set up the logging mechanisms. Call once, very early. */
void    initLogStuff( const char *name );

/* configure the logging mechanisms, may be called multiple times */
void    startLoggingStuff( eLogLevel logLevel, eLogDestination logDest, const char *logFile );

/* tidy up the current logging mechanism */
void stopLoggingStuff( void );

void    setLogStuffDestination( eLogDestination logDestination, ... );

void    setLogStuffLevel( eLogLevel logLevel );

/* start/stop logging function entry & exit */
void    logFunctionTrace( tBool onOff );

/* private helpers, used by preprocessor macros. Please don't use directly! */
void    _log( unsigned int priority, const char *format, ... )
            __attribute__((__format__ (__printf__, 2, 3))) __attribute__((no_instrument_function));
void    _logWithLocation( const char *inFile, unsigned int atLine, unsigned int priority, const char *format, ... )
            __attribute__((__format__ (__printf__, 4, 5))) __attribute__((no_instrument_function));

#define logEmergency(...)  logWithLocation( kLogEmergency, __VA_ARGS__ )
#define logAlert(...)      logWithLocation( kLogAlert,     __VA_ARGS__ )
#define logCritical(...)   logWithLocation( kLogCritical,  __VA_ARGS__ )
#define logError(...)      logWithLocation( kLogError,     __VA_ARGS__ )
#define logWarning(...)    log( kLogWarning, __VA_ARGS__ )
#define logNotice(...)     log( kLogNotice,  __VA_ARGS__ )
#define logInfo(...)       log( kLogInfo,    __VA_ARGS__ )

#ifdef DEBUG
#define logDebug(...)      logWithLocation( kLogDebug, __VA_ARGS__ )
#define logCheckpoint()    do { _log( __FILE__, __LINE__, kLogDebug, "reached" ); } while (0)
#else
#define logDebug(...)      do {} while (0)
#define logCheckpoint()    do {} while (0)
#endif

#ifdef USE_LOG_SCOPING
#define _logCheck_expand_again(priority, scope, id)  \
        ( gLogLevel >= priority \
          && gLog[kLog_##scope].max > id \
          && gLog[kLog_##scope].level >= priority     \
          && gLog[kLog_##scope].site[id] == 0 )

#define logCheck( priority, scope, id )   _logCheck_expand_again( priority, scope, id )

#define log(priority, ...) \
    do { if (  logCheck( priority, LOG_SCOPE, __COUNTER__ ) ) _log( priority, __VA_ARGS__ ); } while (0)

#define logWithLocation(priority, ...) \
    do { \
        if (  logCheck( priority, LOG_SCOPE, __COUNTER__ ) ) _logWithLocation( __FILE__, __LINE__, priority, __VA_ARGS__ ); \
    } while (0)
#else
#define log( priority, ... ) \
    do { if (  gLogLevel >= priority ) _log( priority, __VA_ARGS__ ); } while (0)

#define logWithLocation( priority, ... ) \
    do { if (  gLogLevel >= priority ) _logWithLocation( __FILE__, __LINE__, priority, __VA_ARGS__ ); } while (0)
#endif

#endif

/*
    things to do at the end of each source file to support scoped logging
*/

#ifdef USE_LOG_SCOPING

/* this defines a 'maximum' value for the site ID for each scope, of the form gLogMax<scope> */
#define logDSMhelper(scope, counter) unsigned int gLogMax_##scope = counter
#define logDefineScopeMaximum( scope, counter)  logDSMhelper( scope, counter )

#define LOG_STUFF_EPILOGUE logDefineScopeMaximum( LOG_SCOPE , __COUNTER__ );

#else

#define LOG_STUFF_EPILOGUE

#endif

