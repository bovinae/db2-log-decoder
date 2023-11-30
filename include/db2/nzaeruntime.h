/* Licensed Materials - Property of IBM
* Program Name: IBM Netezza Analytics
*
* © Copyright IBM Corporation 2011, 2012 
* All rights reserved.
*
* US Government Users Restricted Rights - Use, duplication or 
* disclosure restricted by GSA ADP Schedule Contract with 
* IBM Corp.
*/

/*
 *  nzaeruntime.h
 *  Netezza Analytic Executables (NZAE)
 *  NZAE framework: constant runtime information
 */

#ifndef __NZAERUNTIME_H__
#define __NZAERUNTIME_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
   @brief The execution locus.
   @ingroup runtimeenv
   @see NzaeRuntime
*/
typedef enum NzaeLocus
{
    NZAE_LOCUS_POSTGRES = 0,
    NZAE_LOCUS_DBOS = 1,
    NZAE_LOCUS_SPU = 2
} NzaeLocus;


/**
   @brief Adapter types.
   @ingroup runtimeenv
   @see NzaeRuntime
*/

typedef enum NzaeAdapterType
{
    NZAE_ADAPTER_OTHER = 0,
    NZAE_ADAPTER_UDTF = 1,
    NZAE_ADAPTER_UDF = 2,
    NZAE_ADAPTER_UDA = 3
} NzaeAdapterType;


/**
   @brief Log levels.
   @ingroup runtimeenv
   @see nzaeLog
   @see nzaeAggLog
   @see nzaeShpLog
*/

typedef enum NzaeLogLevel
{
   // powers of two
    NZAE_LOG_TRACE = 1,
    NZAE_LOG_DEBUG = 2

} NzaeLogLevel;


/**
   @brief Runtime information.
   @ingroup runtimeenv
   @see nzaeGetRuntime
   @see nzaeAggGetRuntime
   @see nzaeShpGetRuntime
*/

typedef struct NzaeRuntime
{
    int32_t sessionId;
    int32_t dataSliceId;
    int64_t transactionId;
    int32_t hardwareId;
    int32_t numberDataSlices;
    int32_t numberSpus;
    int64_t suggestedMemoryLimit;
    NzaeLocus locus;
    NzaeAdapterType adapterType;
    bool userQuery;
    int logMask;
    bool loggingEnabled;
    char userName[1024];
    uint64_t aeQueryId;
    uint64_t aeCallId;
} NzaeRuntime;


/*
   @brief Gets the AE System Log File name.
   @ingroup runtimeenv
   @return Log file name
*/
const char * nzaeGetSystemLogFileName();

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif


#endif
