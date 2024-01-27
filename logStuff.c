#ifndef _GNU_SOURCE
#define  _GNU_SOURCE  /* dladdr is a gcc extension */
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>
#include <time.h>

#include "logStuff.h"

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) (void)x
#endif

#ifdef __cplusplus
extern "C" {
#endif

const char * gPriorityAsStr[] =
{
    [kLogEmergency] = "Emergemcy",  /* 0: system is unusable */
    [kLogAlert]	    = "Alert",      /* 1: action must be taken immediately */
    [kLogCritical]  = "Critical",   /* 2: critical conditions */
    [kLogError]	    = "Err",        /* 3: error conditions */
    [kLogWarning]   = "Wrn",    /* 4: warning conditions */
    [kLogNotice]    = "Not",     /* 5: normal but significant condition */
    [kLogInfo]	    = "Inf",       /* 6: informational */
    [kLogDebug]	    = "Dbg",      /* 7: debug-level messages */
    [kLogFunctions] = ""            /* function call output */
};

typedef enum {
    kColorBlack    = 0,
    kColorRed      = 1,
    kColorGreen    = 2,
    kColorYellow   = 3,
    kColorBlue     = 4,
    kColorMagenta  = 5,
    kColorCyan     = 6,
    kColorWhite    = 7,
    kColorReset,
    kColorNop
} eANSIColor;

const char * gSetColor[] = {
    [kColorBlack]    = "\e[1;40m",
    [kColorRed]      = "\e[1;41m",
    [kColorGreen]    = "\e[1;42m",
    [kColorYellow]   = "\e[1;43m",
    [kColorBlue]     = "\e[1;44m",
    [kColorMagenta]  = "\e[1;45m",
    [kColorCyan]     = "\e[1;46m",
    [kColorWhite]    = "\e[1;47m",
    [kColorReset]    = "\e[1;0m",
    [kColorNop]      = "",
};

eANSIColor gPrioritySetColor[] =
{
   [kLogEmergency]  = kColorRed,       /* 0: system is unusable */
   [kLogAlert]      = kColorRed,       /* 1: action must be taken immediately */
   [kLogCritical]   = kColorRed,       /* 2: critical conditions */
   [kLogError]      = kColorRed,       /* 3: error conditions */
   [kLogWarning]    = kColorYellow,    /* 4: warning conditions */
   [kLogNotice]     = kColorGreen,     /* 5: normal but significant condition */
   [kLogInfo]       = kColorGreen,     /* 6: informational */
   [kLogDebug]      = kColorNop,       /* 7: debug-level messages */
   [kLogFunctions]  = kColorNop        /* function call output */
};

eANSIColor gPriorityResetColor[] =
{
   [kLogEmergency]  = kColorReset,     /* 0: system is unusable */
   [kLogAlert]      = kColorReset,     /* 1: action must be taken immediately */
   [kLogCritical]   = kColorReset,     /* 2: critical conditions */
   [kLogError]      = kColorReset,     /* 3: error conditions */
   [kLogWarning]    = kColorReset,     /* 4: warning conditions */
   [kLogNotice]     = kColorReset,     /* 5: normal but significant condition */
   [kLogInfo]       = kColorReset,     /* 6: informational */
   [kLogDebug]      = kColorNop,       /* 7: debug-level messages */
   [kLogFunctions]  = kColorNop        /* function call output */
};

struct {
    eLogDestination destination;
    tLogMode        mode;
} gLogSetting[kLogMaxPriotity];

eLogDestination gLogDestination;

const char *    gMyName;
const char *    gLogFilePath = NULL;
FILE *          gLogFile;

void *          gDLhandle = NULL;
tBool           gFunctionTraceEnabled = off;
int             gCallDepth = 1;

static const char *leader = "..........................................................................................";

#ifdef __GNUC__
#define DISABLE_FUNCTION_INSTRUMENTATION  __attribute__((no_instrument_function))
#else
#define DISABLE_FUNCTION_INSTRUMENTATION
#endif

/* {{{{{{{{ DO NOT INSTRUMENT THE INSTRUMENTATION! {{{{{{{{ */


void initLogStuff( const char *name )    DISABLE_FUNCTION_INSTRUMENTATION;

void _log( const char * inFile,
           unsigned int atLine,
           const char * inFunction,
           error_t      error,
           eLogPriority priority,
           const char * format, ... )    DISABLE_FUNCTION_INSTRUMENTATION;

void _logToTheVoid( eLogPriority priority, const char *msg )   DISABLE_FUNCTION_INSTRUMENTATION;
void _logToSyslog(  eLogPriority priority, const char *msg )   DISABLE_FUNCTION_INSTRUMENTATION;
void _logToFile(    eLogPriority priority, const char *msg )   DISABLE_FUNCTION_INSTRUMENTATION;
void _logToStderr(  eLogPriority priority, const char *msg )   DISABLE_FUNCTION_INSTRUMENTATION;

void _profileHelper( void *leftAddr, const char *middle, void *rightAddr )  DISABLE_FUNCTION_INSTRUMENTATION;

const char * addressToString( void * addr, char * scratch )    DISABLE_FUNCTION_INSTRUMENTATION;

#ifdef __GNUC__
void __cyg_profile_func_enter( void * this_fn, void * call_site )     DISABLE_FUNCTION_INSTRUMENTATION;
void __cyg_profile_func_exit( void * this_fn, void * call_site )      DISABLE_FUNCTION_INSTRUMENTATION;
#endif

/* }}}}}}}} DO NOT INSTRUMENT THE INSTRUMENTATION! }}}}}}}} */


void _logToTheVoid( eLogPriority UNUSED(priority), const char * UNUSED(msg))  { /* just return */ }

void _logToSyslog( eLogPriority priority, const char * msg )         { syslog( priority, "%s", msg ); }

void _logToFile( eLogPriority UNUSED(priority), const char *msg )
{
    fprintf(gLogFile, "%lu: %s\n", time( NULL ), msg );
}

/* stderr is has colored output */
void _logToStderr( eLogPriority priority, const char *msg )
{
    fprintf(stderr, "%lu: %s%s%s\n",
            time( NULL ),
            gSetColor[ gPrioritySetColor[ priority ] ],
            msg,
            gSetColor[ gPriorityResetColor[ priority ] ] );
}

/* use function pointers to invoke the logging destination */
typedef void (*fpLogTo)( eLogPriority priority, const char * msg );
fpLogTo gLogOutputFP[kLogMaxDestination] =
{
    [kLogToTheVoid] = &_logToTheVoid,
    [kLogToSyslog]  = &_logToSyslog,
    [kLogToFile]    = &_logToFile,
    [kLogToStderr]  = &_logToStderr
};

/*
 * the macros eventually expand to invoke this help function
 */

void _log( const char *inPath,
           unsigned int atLine,
           const char * UNUSED(inFunction),
           error_t error,
           unsigned int priority,
           const char *format, ...)
{
    va_list vaptr;
    char    msg[1024];
    int     prefixLen;

    if ( gLogSetting[ priority ].destination != kLogToTheVoid )
    {
        va_start( vaptr, format );

        prefixLen = 0;
        /* if the destination isn't syslog, prefix the message with the priority */
        if ( priority <= kLogDebug && gLogSetting[priority].destination != kLogToSyslog)
        {
            prefixLen = snprintf( msg, sizeof( msg ), "%s [%d]: ",
                                  gPriorityAsStr[ priority ],
                                  getpid() );
        }

        prefixLen += vsnprintf( &msg[ prefixLen ], sizeof( msg ) - prefixLen, format, vaptr );

        if (error != 0)
        {
            prefixLen += snprintf( &msg[ prefixLen ], sizeof( msg ) - prefixLen, " (%d: %s)",
                                    error, strerror( error ) );
        }

        if ( gLogSetting[ priority ].mode == kLogWithLocation )
        {
            snprintf( &msg[ prefixLen ], sizeof( msg ) - prefixLen, " @ %s:%d", inFile, atLine );
        }

        ( gLogOutputFP[ gLogSetting[ priority ].destination ] )( priority, msg );

        va_end( vaptr );
    }
}


void initLogStuff( const char * name )
{
    gMyName = "<not set>";
    if ( name != NULL )
    {
        /* strip off the path, if there is one */
        const char * p = strrchr( name, '/');
        if ( p++ == NULL )
        {
            p = name;
        }

        gMyName = strdup( p );
    }
    openlog( gMyName, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

    // initialize gEvent to something safe until startLogStuff has been invoked

    for ( int i = 0; i < kLogMaxPriotity; ++i )
    {
        gLogSetting[i].mode        = kLogWithLocation;
        gLogSetting[i].destination = kLogToStderr;
    }

    gDLhandle = dlopen(NULL, RTLD_LAZY);

    logFunctionTrace( off );
}

void setLogStuffDestination( eLogPriority priority, eLogDestination logDest )
{
    // stopLoggingStuff();

}

void setLogStuffFileDestination( const char * logFile )
{
    if ( gLogFilePath != NULL)
    {
        free( (void *)gLogFilePath );
        gLogFilePath = NULL;
    }

    if ( logFile != NULL)
    {
        gLogFilePath = strdup( logFile );

        gLogFile = fopen( gLogFilePath, "a" );

        if ( gLogFile != NULL )
        {
        }
        else {
            logError( "Unable to log to \"%s\" (%d: %s), redirecting to stderr",
                      gLogFilePath, errno, strerror( errno ));
        }
    }
}

void stopLoggingStuff( void )
{
    for ( int i = kLogMaxPriotity; i <= 0; --i)
    {
        gLogOutputFP[i] = NULL;
    }

    switch (gLogDestination)
    {
    case kLogToSyslog:
        closelog();
        break;

    case kLogToFile:
        fclose( gLogFile );
        gLogFile = NULL;
        break;

    default:
        // don't do anything for the other cases
        break;
    }
}

const char * addressToString( void *addr, char *scratch)
{
    const char   *result;
    Dl_info info;

    result = NULL;
    if (gDLhandle != NULL )
    {
        dladdr(addr, &info);
        result = info.dli_sname;
    }
    if (result == NULL)
    {
        sprintf( scratch, "%p", addr);
        result = scratch;
    }
    return result;
}

#ifdef __GNUC__

/* helper function for logging function entry and exit */
void _profileHelper( void *leftAddr, const char *middle, void *rightAddr)
{
    // some scratch space, in case addressToString needs it. avoids needless malloc churn
    char leftAddrAsStr[64];
    char rightAddrAsStr[64];
    char msg[256];

    if ( gFunctionTraceEnabled && gLogOutputFP[kLogFunctions] != NULL )
    {
        addressToString( leftAddr,  leftAddrAsStr  );
        addressToString( rightAddr, rightAddrAsStr );
        snprintf( msg, sizeof(msg), "%.*s %s() %s %s()",
                  gCallDepth, leader, leftAddrAsStr, middle, rightAddrAsStr );
        (*logDestFP)( kLogFunctions, msg);
    }
}

/*
    if the -finstrument-functions option is used with gcc, it inserts calls
    to these functions at the beginning and end of every compiled function.
*/

/* just landed in a function */
void __cyg_profile_func_enter(void *this_fn, void *call_site)
{
    _profileHelper( call_site, "called", this_fn );
    ++gCallDepth;
}

/* about to leave a function */
void __cyg_profile_func_exit(void *this_fn, void *call_site)
{
    if (--gCallDepth < 1) gCallDepth = 1;
    _profileHelper( this_fn, "returned to", call_site );
}

/*
    If just left on, this generates so much logging that it's rarely useful.
    Please use logFunctionTrace to turn on tracing only around the code
    or situation you care about.
*/
void logFunctionTrace( tBool onOff )
{
    gFunctionTraceEnabled = onOff;
}

#ifdef __cplusplus
}
#endif

#endif
