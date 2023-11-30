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
 *  nzae.h
 *  Netezza Analytic Executables (NZAE)
 *  main client C language include file for NZAE framework.
 */

#ifndef __NZAE_H__
#define __NZAE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "nzaemetadata.h"
#include "nzaehandles.h"
#include "nzaeapidefs.h"
#include "nzudsudxtypes.h"
#include "nzudsdata.h"
#include "nzaeruntime.h"
#include "nzaeusercodes.h"


/** size of error message buffer */
#define NZAE_ERROR_MESSAGE_LENGTH 1024

/**
    @brief Return codes from nzae functions.
    @ingroup Function
    @see Function
*/
typedef enum NzaeRcCode
{
    NZAE_RC_ERROR = -1,
    NZAE_RC_NORMAL = 0,
    NZAE_RC_END = 1
} NzaeRcCode;

/**
    @brief Argument to function nzaeIntialize.
    Output parameters are handle and errorMessage.
    @ingroup specialinit
    @see nzaeInitialize
*/
typedef struct NzaeInitialization
{
    int ldkVersion;
    NZAE_HANDLE handle;
    char errorMessage[NZAE_ERROR_MESSAGE_LENGTH];
    NZAEENV_HANDLE hEnv;   // NULL except for remote mode
                           // if non-NULL AE assumes ownership of this handle
} NzaeInitialization;



/**
    @brief Initialization must be called near the beginning of the process.
    @ingroup specialinit
    @param arg The initialization argument.
    @return The function return code.
*/
NzaeRcCode nzaeInitialize(NzaeInitialization * arg);

/**
    @brief Closes the handle when done.
    @ingroup Function
    @param handle The function handle.
*/
void nzaeClose(NZAE_HANDLE handle);

/**
    @brief Gets metadata about the AE.
    @ingroup Function
    @param handle The function handle.
    @param arg Metadata to be filled out. Created by the caller.
    @return The function return code.

*/
NzaeRcCode nzaeGetMetadata(NZAE_HANDLE handle, NzaeMetadata * arg);

/**
    @brief Gets runtime information about the AE.
    @param handle The function handle.
    @param arg Runtime to be filled out. Created by the caller.
    @ingroup Function
    @return The function return code.
*/
NzaeRcCode nzaeGetRuntime(NZAE_HANDLE handle, NzaeRuntime * arg);

/**
    @brief Gets the next input row; returns NZAE_RC_END at End of File.

    Invalidates previous data returned by nzaeGetInputColumn.
    @param handle The function handle.
    @ingroup Function
    @return The function return code.

*/
NzaeRcCode nzaeGetNext(NZAE_HANDLE handle);

/**
    @brief Gets the next partition; returns NZAE_RC_END at End of Partition.

    Invalidates previous data retured by nzaeGetInputColumn.
    @param handle The function handle.
    @ingroup Function
    @return The function return code.

*/
NzaeRcCode nzaeGetNextPartition(NZAE_HANDLE handle);

/**
    @brief Outputs a result row containing the current column values.
    @param handle The function handle.
    @ingroup Function
    @return The function return code.

*/
NzaeRcCode nzaeOutputResult(NZAE_HANDLE handle);

/**
    @brief Indicates that the AE is still active and not hanging.
    @param handle The function handle.
    @ingroup Function
    @return The function return code.
*/
NzaeRcCode nzaePing(NZAE_HANDLE handle);

/**
    @brief Indicates that the AE is finishing and does not get any more rows
    or output any more results.
    @param handle The function handle.
    @ingroup Function
    @return The function return code.
*/
NzaeRcCode nzaeDone(NZAE_HANDLE handle);

/**
    @brief Indicates this AE has encountered an error condition.

    Implies nzaeDone.
    Message is built like printf.
    @param handle The function handle.
    @param _template The printf-style template.
    @ingroup Function
    @return The function return code.
*/
NzaeRcCode nzaeUserError(NZAE_HANDLE handle, const char * _template, ...);

/**
    @brief Logs the specified message.
    @param handle The function handle.
    @param level The log level.
    @param message The log message.
    @ingroup Function
    @return The function return code.
*/
NzaeRcCode nzaeLog(NZAE_HANDLE handle, NzaeLogLevel level, const char * message);

/**
   @brief Get the message text for the last error that occurred.
    @param handle The function handle.
   @ingroup Function
   @return The message text of the last occurring error.
*/
const char * nzaeGetLastErrorText(NZAE_HANDLE handle);

/**
   @brief Gets the code for the last error that occurred.
    @param handle The function handle.
   @ingroup Function
   @return The function error code for the last occurring error.
*/
AeUserCode nzaeGetLastErrorCode(NZAE_HANDLE handle);

/**
    @brief Gets input column data. The index is zero-based.

    NzudsData is defined in nzuds.h.
    The data belongs to the framework and should not be freed.
    Called after nzaeGetNext is used to return the next row.
    @param handle The function handle.
    @param index The input index.
    @param data The UDS data.
    @ingroup Function
    @return The function return code.

 **************************************************************************/
NzaeRcCode nzaeGetInputColumn(NZAE_HANDLE handle, int index, NzudsData ** data);

/**
   @brief Sets the output column to NULL. The index is zero-based.

   Column values are "sticky" and remain set until changed.
   @param handle The function handle.
   @param index The field index.
   @return The function return code.


 **************************************************************************/

NzaeRcCode nzaeSetOutputNull(NZAE_HANDLE handle, int index);

// string

/**
   @brief Sets the string field value.

   The value is expected to be NULL-terminated.
   A copy of the string value is created.
    @param handle The function handle.
   @param index The field index.
   @param value The string value, with length determined by strlen.
   @return The function return code.
*/
NzaeRcCode nzaeSetOutputString(NZAE_HANDLE handle, int index, const char * value);
/**
   @brief Sets the string field value.

    The string length is determined by the length argument.
    A copy of the string value is created.
    NULL termination does not apply.
    @param handle The function handle.
    @param index The field index.
    @param value The string value.
    @param length The length of the string.
   @return The function return code.
*/
NzaeRcCode nzaeSetOutputStringLength(NZAE_HANDLE handle, int index,
    const char * value, int length);

// integer
/**
   @brief Sets the int8 field value.
   @param handle The function handle.
   @param index The field index.
   @param value The int8 value.
   @return The function return code.
*/
NzaeRcCode nzaeSetOutputInt8(NZAE_HANDLE handle, int index, int8_t value);

/**
   @brief Sets the int16 field value.
   @param handle The function handle.
   @param index The field index.
   @param value The int16 value.
   @return The function return code.
*/
NzaeRcCode nzaeSetOutputInt16(NZAE_HANDLE handle, int index, int16_t value);

/**
   @brief Sets the int32 field value.
   @param handle The function handle.
   @param index The field index.
   @param value The int32 value.
   @return The function return code.
*/
NzaeRcCode nzaeSetOutputInt32(NZAE_HANDLE handle, int index, int32_t value);

/**
   @brief Sets the int64 field value.
   @param handle The function handle.
   @param index The field index.
   @param value Th int64 value.
   @return The function return code.
*/
NzaeRcCode nzaeSetOutputInt64(NZAE_HANDLE handle, int index, int64_t value);

// float
/**
   @brief Sets the float field value.
   @param handle The function handle.
   @param index The field index.
   @param value The float value.
   @return The function return code.
*/
NzaeRcCode nzaeSetOutputFloat(NZAE_HANDLE handle, int index, float value);

/**
   @brief Sets the double field value.
   @param handle The function handle.
   @param index The field index.
   @param value The double value.
   @return The function return code.
*/
NzaeRcCode nzaeSetOutputDouble(NZAE_HANDLE handle, int index, double value);

// date and time
/**
   @brief Sets the date field value.
   @param handle The function handle.
   @param index The field index.
   @param value The date value.
   @return The function return code.
*/
NzaeRcCode nzaeSetOutputDate(NZAE_HANDLE handle, int index, int32_t value);

/**
   @brief Sets the time field value.
   @param handle The function handle.
   @param index The field index.
   @param value The time value.
   @return The function return code.
*/
NzaeRcCode nzaeSetOutputTime(NZAE_HANDLE handle, int index, int64_t value);

/**
   @brief Sets the timestamp field value.
   @param handle The function handle.
   @param index The field index.
   @param value The timestamp value.
   @return The function return code.
*/
NzaeRcCode nzaeSetOutputTimeStamp(NZAE_HANDLE handle, int index, int64_t value);

/**
   @brief Sets the timetz field value.
   @param handle The function handle.
   @param index The field index.
   @param value The timeTz value.
   @return The function return code.
*/
NzaeRcCode nzaeSetOutputTimeTz(NZAE_HANDLE handle, int index, const NzudsTimeTz * value);

/**
   @brief Sets the interval field value.
   @param handle The function handle.
   @param index The field index.
   @param value The interval value.
   @return The function return code.
*/
NzaeRcCode nzaeSetOutputInterval(NZAE_HANDLE handle, int index, NzudsInterval * value);

// numeric fixed decimal
/**
   @brief Sets the numeric32 field value.
    @param handle The function handle.
   @param index The field index.
   @param value The numeric32 value.
   @return The function return code.
*/
NzaeRcCode nzaeSetOutputNumeric32(NZAE_HANDLE handle, int index, const NzudsNumeric32 * value);

/**
   @brief Sets the numeric64 field value.
    @param handle The function handle.
   @param index The field index.
   @param value The numeric64 value.
   @return The function return code.
*/
NzaeRcCode nzaeSetOutputNumeric64(NZAE_HANDLE handle, int index, const NzudsNumeric64 * value);

/**
   @brief Sets the numeric128 field value.
    @param handle The function handle.
   @param index The field index.
   @param value The numeric128 value.
   @return The function return code.
*/
NzaeRcCode nzaeSetOutputNumeric128(NZAE_HANDLE handle, int index, const NzudsNumeric128 * value);

// logical bool
/**
   @brief Sets the bool field value.
    @param handle The function handle.
   @param index The field index.
   @param value The bool value.
   @return The function return code.
*/
NzaeRcCode nzaeSetOutputBool(NZAE_HANDLE handle, int index, bool value);

// generic value
/**
   @brief Sets the field value.

   Useful for input to output copying.
    @param handle The function handle.
   @param index The field index.
   @param data The Value.
   @return The function return code.
*/
NzaeRcCode nzaeSetOutputColumn(NZAE_HANDLE handle, int index, const NzudsData * arg);

//
/**
   @brief Get Value.

   Returns the sticky value of an output column.
   This result becomes invalid when the column is set to a new value.
   The memory pointed to by NzudsData data belongs to the AE.
   The NzudsData data value should not be modified directly.
    @param handle The function handle.
   @param index The field index.
   @param data The returned Field data.
   @return The function return code.
*/
NzaeRcCode nzaeGetOutputColumn(NZAE_HANDLE handle, int index, NzudsData ** data);

/**
   @brief Gets an AE or system environment variable. The AE has precedence.
    @param handle The function handle.
   @param name The variable name.
   @param result The variable value or NULL if not found.
   @ingroup Function
   @return The function return code.
*/
NzaeRcCode nzaeGetEnv(NZAE_HANDLE handle, const char * name, const char ** result);

/**
    @brief Returns the first environment entry.

    This function call is followed by repeated calls to nzaeGetNextEnvironmentEntry.
    The AE system owns the memory from this call.
    @param handle The function handle.
    @param entry First entry.
    @ingroup Function

*/
void nzaeGetFirstEnvironmentEntry(NZAE_HANDLE handle,
    NzaeEnvironmentEntry * entry);

/**
    @brief Returns the next environment entry.

    The first nzaeGetNextEnvironmentEntry must follow a
    call to nzaeGetFirstEnvironmentEntry.
    Returns FALSE on end.
    Key names may repeat but the current version of a keyname comes last.
    The AE system owns the memory from this call.
    @param handle The function handle.
    @param entry The next entry.
    @ingroup Function
    @return FALSE on end.
*/
bool nzaeGetNextEnvironmentEntry(NZAE_HANDLE handle, NzaeEnvironmentEntry * entry);

/**
    @brief Gets the file path for a library name.

    Returns NULL if the library is not found.
    The AE system owns the memory from this call.
    @param h The function handle.
    @param libraryName The library name.
    @param caseSensitive If TRUE, the lookup is case-sensitive.
    @ingroup Function
    @return File path if found; NULL otherwise
*/
const char * nzaeGetLibraryFullPath(NZAE_HANDLE h, const char * libraryName,
    bool caseSensitive);

/**
    @brief Returns NzaeSharedLibraryInfo of the shared library for the
        request.

    The AE system owns the memory from this call.

    @param h The function handle.
    @ingroup Function
    @return The Shared Library information.
*/
NzaeSharedLibraryInfo * nzaeGetLibraryInfo(NZAE_HANDLE h);

/**
    @brief Returns NzaeSharedLibraryInfo of the shared library for the process.

        Returns NULL if this is not a Remote AE.
    The AE system owns the memory from this call.

    @param h The function handle.
    @ingroup Function
    @return The Shared Library information
*/
NzaeSharedLibraryInfo * nzaeGetLibraryProcessInfo(NZAE_HANDLE h);

/**
    @brief Returns the number of parameters.

    @param h The function handle.
    @ingroup Function.
    @return The number of parameters.
*/
int nzaeGetNumberOfParameters(NZAE_HANDLE h);

/**
    @brief Returns a parameter.

    The index is zero-based.
    @param h The function handle.
    @param index The parameter index.
    @ingroup Function
    @return The parameter value.
*/
const char * nzaeGetParameter(NZAE_HANDLE h, int index);

/**
    @brief Returns the handle for a local AE.

    Returns 0 on success, -1 on error.
    The caller provides the errorMessage buffer and size.
    The suggested rrror message buffer size is 1050.
    @ingroup localinit
    @param result The returned API.
    @param ldkVersion The expected version.
    @param errorMessage The error message buffer.
    @param errorMessageSize The error message buffer size.
    @return A value of 0 on success, -1 on error.

*/
extern int nzaeLocprotGetApi(NzaeApi * result, int ldkVersion,
    char * errorMessage, int errorMessageSize);

/**
    @brief Returns TRUE if the AE is local.

    The lifecycle of a local process is controlled by the Netezza software.
    @ingroup remoteinit
    @ingroup localinit
    @return TRUE if the AE is local.
*/
extern int nzaeIsLocal();

/**
    @brief Returns a true value if this is a Remote AE.

    @ingroup remoteinit
    @ingroup localinit
    @return True if this a local AE
*/
extern int nzaeIsRemote();

// Temporarily excluded from build
//#include "nzaenumeric.h"
#include "nzaedatetime.h"

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
