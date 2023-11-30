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

/**
 * nzaeagg.h
 * Netezza Analytic Executables (NZAE Aggregation)
 * main client C language include file for NZAE framework.
 */

#ifndef __NZAEAGG_H__
#define __NZAEAGG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "nzaemetadata.h"
#include "nzaehandles.h"
#include "nzaeapidefs.h"
#include "nzaeapiscommon.h"
#include "nzudsudxtypes.h"
#include "nzudsdata.h"
#include "nzaeruntime.h"
#include "nzaeusercodes.h"

/** size of error message buffer */
#define NZAEAGG_ERROR_MESSAGE_LENGTH 1024

/** @brief Return codes from nzaeAgg aggregate functions.
    @ingroup Aggregate
    @see Aggregate
 */
typedef enum NzaeAggRcCode
{
    NZAEAGG_RC_ERROR = -1,
    NZAEAGG_RC_NORMAL = 0,
} NzaeAggRcCode;

/**
    @brief An argument to function nzaeAggIntialize.
    Output parameters are handle and errorMessage.
    @ingroup specialinit
    @see nzaeAggInitialize
*/
typedef struct NzaeAggInitialization
{
    int ldkVersion;
    NZAEAGG_HANDLE handle;
    char errorMessage[NZAEAGG_ERROR_MESSAGE_LENGTH];
    NZAEENV_HANDLE hEnv;   // NULL except for remote mode
                           // if non-NULL AE assumes ownership of this handle
} NzaeAggInitialization;


/** @ingroup Aggregate
    @brief NzaeAggMetatadata
    @see NzaeAggReadOnlyFieldFunctions
    @see NzaeAggFieldFunctions
*/
/*
 Moved to nzaemetadata.h

typedef struct NzaeAggMetadata
{
    int numColumns;
    NzudsDataType * types;
    int * sizes;
    int * scales;
} NzaeAggMetadata;

*/
/** @ingroup Aggregate
    @brief Read-only record functions for Aggregation.
    @see NzaeAggAccumulate
    @see NzaeAggMerge
    @see NzaeAggFinalResult
*/
typedef struct NzaeAggReadOnlyFieldFunctions
{
    /**
       @brief Get Value.
       @param handle The aggregate handle.
       @param index The field index.
       @param data The returned Field data.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*getValue)(NZAEAGG_HANDLE handle, int index, NzudsData ** data);

    /**
       @brief Specifies whether the field is NULL.
       @param handle The aggregate handle.
       @param index The field index.
       @param result TRUE if NULL.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*isNull)(NZAEAGG_HANDLE handle, int index, bool * result);

    NzaeAggMetadata metadata;
} NzaeAggReadOnlyFieldFunctions;


/** @ingroup Aggregate
    @brief Read and write record functions for Aggregation.
    @see NzaeAggInitializeState
    @see NzaeAggAccumulate
    @see NzaeAggMerge
    @see NzaeAggFinalResult
*/
typedef struct NzaeAggFieldFunctions
{
    /**
       @brief Sets the field value to NULL.
       @param handle The aggregate handle.
       @param index The field index.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*setNull)(NZAEAGG_HANDLE handle, int index);

    /**
       @brief Sets the string field value.

       The value is expected to be NULL-terminated.
       A copy of the string value is created.
       @param handle The aggregate handle.
       @param index The field index.
       @param value The string value, with length determined by strlen.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*setString)(NZAEAGG_HANDLE handle, int index, const char * value);

    /**
       @brief Sets the string field value.

       The string length is determined by the length argument.
       A copy of the string value is created.
       NULL termination does not apply.
       @param handle The aggregate handle.
       @param index The field index.
       @param value The string value.
       @param length The length of the string.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*setStringLength)(NZAEAGG_HANDLE handle, int index,
        const char * value, int length);

    /**
       @brief Sets the int8 field value.
       @param handle The aggregate handle.
       @param index The field index.
       @param value The int8 value.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*setInt8)(NZAEAGG_HANDLE handle, int index, int8_t value);

    /**
       @brief Sets the int16 field value.
       @param handle The aggregate handle.
       @param index The field index.
       @param value The int16 value.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*setInt16)(NZAEAGG_HANDLE handle, int index, int16_t value);

    /**
       @brief Sets the int32 field value.
       @param handle The aggregate handle.
       @param index The field index.
       @param value The int32 value.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*setInt32)(NZAEAGG_HANDLE handle, int index, int32_t value);

    /**
       @brief Sets the int64 field value.
       @param handle The aggregate handle.
       @param index The field index.
       @param value The int64 value.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*setInt64)(NZAEAGG_HANDLE handle, int index, int64_t value);

    /**
       @brief Sets the float field value.
       @param handle The aggregate handle.
       @param index The field index.
       @param value The float value.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*setFloat)(NZAEAGG_HANDLE handle, int index, float value);

    /**
       @brief Sets the double field value.
       @param handle The aggregate handle.
       @param index The field index.
       @param value The double value.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*setDouble)(NZAEAGG_HANDLE handle, int index, double value);

    /**
       @brief Sets the date field value.
       @param handle The aggregate handle.
       @param index The field index.
       @param value The date value.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*setDate)(NZAEAGG_HANDLE handle, int index, int32_t value);

    /**
       @brief Sets the time field value.
       @param handle The aggregate handle.
       @param index The field index.
       @param value The time value.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*setTime)(NZAEAGG_HANDLE handle, int index, int64_t value);


    /**
       @brief Sets the timestamp field value.
       @param handle The aggregate handle.
       @param value The timestamp value.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*setTimeStamp)(NZAEAGG_HANDLE handle, int index, int64_t value);


    /**
       @brief Sets the timeTz field value.
       @param handle The aggregate handle.
       @param index The field index.
       @param value The timeTz value.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*setTimeTz)(NZAEAGG_HANDLE handle, int index, const NzudsTimeTz * value);

    /**
       @brief Sets the interval field value.
       @param handle The aggregate handle.
       @param index The field index.
       @param value The interval value.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*setInterval)(NZAEAGG_HANDLE handle, int index, NzudsInterval * value);

    /**
       @brief Sets the numeric32 field value.
       @param handle The aggregate handle.
       @param index The field index.
       @param value The numeric32 value.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*setNumeric32)(NZAEAGG_HANDLE handle, int index, const NzudsNumeric32 * value);

    /**
       @brief Sets the numeric64 field value.
       @param handle The aggregate handle.
       @param index The field index.
       @param value The numeric64 value.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*setNumeric64)(NZAEAGG_HANDLE handle, int index, const NzudsNumeric64 * value);

    /**
       @brief Sets the numeric128 field value.
       @param handle The aggregate handle.
       @param index The field index.
       @param value The numeric128 value.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*setNumeric128)(NZAEAGG_HANDLE handle, int index, const NzudsNumeric128 * value);

    /**
       @brief Sets the bool field value.
       @param handle The aggregate handle.
       @param index The field index.
       @param value The bool value.
    @return The aggregate return code.
    */
    NzaeAggRcCode (*setBool)(NZAEAGG_HANDLE handle, int index, bool value);

    /**
       @brief Sets the field value
       @param handle The aggregate handle.
       @param index The field index.
       @param data The value.
    @return The aggregate return code.
    */
    NzaeAggRcCode (*setValue)(NZAEAGG_HANDLE handle, int index, NzudsData * data);

    /**
       @brief Get Value.
       @param handle The aggregate handle.
       @param index The field index.
       @param data The Returned Field data.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*getValue)(NZAEAGG_HANDLE handle, int index, NzudsData ** data);

    /**
       @brief Specifieds if the field is NULL.
       @param handle The aggregate handle.
       @param index The field index.
       @param result TRUE if NULL.
       @return The aggregate return code.
    */
    NzaeAggRcCode (*isNull)(NZAEAGG_HANDLE handle, int index, bool * result);

    NzaeAggMetadata metadata;
} NzaeAggFieldFunctions;

/** @ingroup Aggregate
    @brief The InitializeState structure.
    @see nzaeAggNext
*/
typedef struct NzaeAggInitializeState
{
    NzaeAggFieldFunctions state;
} NzaeAggInitializeState;


/** @ingroup Aggregate
    @brief The Accumulate structure.
    @see nzaeAggNext
*/
typedef struct NzaeAggAccumulate
{
    NzaeAggReadOnlyFieldFunctions input;
    NzaeAggFieldFunctions state;
} NzaeAggAccumulate;


/** @ingroup Aggregate
    @brief The Merge structure.
    @see nzaeAggNext
*/
typedef struct NzaeAggMerge
{
    NzaeAggReadOnlyFieldFunctions inputState;
    NzaeAggFieldFunctions state;
} NzaeAggMerge;


/** @ingroup Aggregate
    @brief The Final Result structure.
    @see nzaeAggNext
*/
typedef struct NzaeAggFinalResult
{
    NzaeAggReadOnlyFieldFunctions inputState;
    NzaeAggFieldFunctions result;
} NzaeAggFinalResult;

/**
    @brief Initialization to be called near the beginning of the process.
    @ingroup specialinit
    @param arg The initialization argument.
    @return The aggregate return code.
*/
NzaeAggRcCode nzaeAggInitialize(NzaeAggInitialization * arg);

/**
    @brief Closes the handle when done.
    @param handle The aggregate handle.
    @ingroup Aggregate
*/
void nzaeAggClose(NZAEAGG_HANDLE handle);

/**
    @brief Gets runtime information about the AE Aggregate.
    @param handle The aggregate handle.
    @param arg The caller-created runtime to be filled out.
    @ingroup Aggregate
    @return The aggregate return code.
*/
NzaeAggRcCode nzaeAggGetRuntime(NZAEAGG_HANDLE handle, NzaeRuntime * arg);

/**
    @brief Gets the next aggregation message.

    Returns a NzaeAggInitialize, NzaeAggAccumulate,
    NzaeAggMerge, or NzaeAggFinalResult struct pointer.
    Use the messageType parameter to determine the return type, end of input, and error.
    Returns NULL on error or at the end of data.
    @param handle The aggregate handle.
    @param messageType The returned message type.
    @ingroup Aggregate
    @return The structure as void * .

*/
void * nzaeAggNext(NZAEAGG_HANDLE handle, NzaeAggMessageType * messageType);

/**
    @brief Updates the result to the database.
    @param handle The aggregate handle.
    @ingroup Aggregate
    @return A NzaeAggInitialize, NzaeAggAccumulate, NzaeAggMerge,
    or NzaeAggFinalResult struct pointer. Can be NULL on error.
    @see NzaeAggMessageType
*/
NzaeAggRcCode nzaeAggUpdate(NZAEAGG_HANDLE handle);

/**
    @brief Indicates that the AE encountered an error condition.

    The AE is complete and should exit after this call.
    The message is built like printf.
    @param handle The aggregate handle.
    @param _template The printf-style template.
    @ingroup Aggregate
    @return The aggregate return code.
*/
NzaeAggRcCode nzaeAggUserError(NZAEAGG_HANDLE handle, const char * _template, ...);

/**
    @brief Indicates that the AE Aggregate is still active and not hanging.
    @param handle The aggregate handle.
    @ingroup Aggregate
    @return The aggregate return code.
*/
NzaeAggRcCode nzaeAggPing(NZAEAGG_HANDLE handle);

/**
    @brief Logs the specified message.
    @param handle The aggregate handle.
    @param level The log level.
    @param message The log message.
    @ingroup Aggregate
    @return The aggregate return code.
*/
NzaeAggRcCode nzaeAggLog(NZAEAGG_HANDLE handle, NzaeLogLevel level, const char * message);

/**
   @brief Gets the message text for the last error that occurred.
    @param handle The aggregate handle.
   @ingroup Aggregate
   @return The text of the last error.
*/
const char * nzaeAggGetLastErrorText(NZAEAGG_HANDLE handle);

/**
   @brief Gets the code for the last error that occurred.
    @param handle The aggregate handle.
   @ingroup Aggregate
   @return The aggregate error code.
*/
AeUserCode nzaeAggGetLastErrorCode(NZAEAGG_HANDLE handle);

/**
   @brief Gets the AE Aggregate System Log File name.
   @ingroup Aggregate
   @return The log file name
*/
const char * nzaeAggGetSystemLogFileName();

/**
   @brief Gets the AE or system environment variable. The AE variable has precedence.
   @param handle The aggregate handle.
   @param name The variable name.
   @param result The output variable value or NULL if not found.
   @ingroup Aggregate
   @return The aggregate return code.
*/
NzaeAggRcCode nzaeAggGetEnv(NZAEAGG_HANDLE handle, const char * name, const char ** result);

/**
    @brief Returns the first environment entry.

    This function call is followed by repeated calls to nzaeGetNextEnvironmentEntry.
    The AE system owns the memory from this call.
    @param handle The aggregate handle.
    @param entry The first entry.
    @ingroup Aggregate

*/
void nzaeAggGetFirstEnvironmentEntry(NZAEAGG_HANDLE handle,
    NzaeEnvironmentEntry * entry);

/**
    @brief Returns the next environment entry.

    The first nzaeGetNextEnvironmentEntry must follow a
    call to nzaeGetFirstEnvironmentEntry.
    Returns FALSE on end.
    Key names may repeat but the current version of a keyname is given last.
    The AE system owns the memory from this call.
    @param handle The aggregate handle.
    @param entry The next entry.
    @ingroup Aggregate
    @return FALSE on end.
*/
bool nzaeAggGetNextEnvironmentEntry(NZAEAGG_HANDLE handle, NzaeEnvironmentEntry * entry);

/**
    @brief Gets the file path for the library name.

    Returns NULL if the library is not found.
    The AE system owns the memory from this call.
    @param h The aggregate handle.
    @param libraryName The library name.
    @param caseSensitive If TRUE, the lookup is case-sensitive.
    @ingroup Aggregate
    @return The file path if found; NULL otherwise.
*/
const char * nzaeAggGetLibraryFullPath(NZAEAGG_HANDLE h, const char * libraryName,
    bool caseSensitive);

/**
    @brief Returns NzaeSharedLibraryInfo for the requested Shared Library information.

    The AE system owns the memory from this call.
    @param h The aggregate handle.
    @ingroup Aggregate
    @return The Shared Library information.
*/
NzaeSharedLibraryInfo * nzaeAggGetLibraryInfo(NZAEAGG_HANDLE h);

/**
    @brief Returns NzaeSharedLibraryInfo shared library information
    for the process. Returns NULL if the AE is not Remote.
    The AE system owns the memory from this call.
    @param h The aggregate handle.
    @ingroup Aggregate
    @return The Shared Library information.
*/
NzaeSharedLibraryInfo * nzaeAggGetLibraryProcessInfo(NZAEAGG_HANDLE h);

/**
    @brief Returns the number of parameters.
    @param h The aggregate handle.
    @ingroup Aggregate
    @return The number of parameters.
*/
int nzaeAggGetNumberOfParameters(NZAEAGG_HANDLE h);

/**
    @brief Returns the parameter.

    The Index is zero-based.
    @param h The aggregate handle.
    @param index The parameter index.
    @ingroup Aggregate
    @return The parameter value.
*/
const char * nzaeAggGetParameter(NZAEAGG_HANDLE h, int index);

/**
    @brief Returns the Aggregation Type.
    @param handle The aggregate handle.
    @ingroup Aggregate
    @return The aggregate type.
*/
NzaeAggType nzaeAggGetType(NZAEAGG_HANDLE handle);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
