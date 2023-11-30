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
 * nzaeshp.h
 * Netezza Analytic Executables (NZAE Shapper and Sizer API)
 * main client C language include file for NZAE framework.
 */

#ifndef __NZAESHP_H__
#define __NZAESHP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "nzaehandles.h"
#include "nzaeapidefs.h"
#include "nzudsudxtypes.h"
#include "nzudsdata.h"
#include "nzaeruntime.h"
#include "nzaeusercodes.h"

/** size of error message buffer */
#define NZAESHP_ERROR_MESSAGE_LENGTH 1024

/** @brief Return codes from nzaeShp Shaper functions.
    @ingroup Shaper
    @see Shaper
 */
typedef enum NzaeShpRcCode
{
    NZAESHP_RC_ERROR = -1,
    NZAESHP_RC_NORMAL = 0,
} NzaeShpRcCode;

/**
    @brief Argument to function nzaeShpIntialize.
    Output parameters are handle and errorMessage.
    @ingroup specialinit
    @see nzaeShpInitialize
*/
typedef struct NzaeShpInitialization
{
    int ldkVersion;
    NZAESHP_HANDLE handle;
    char errorMessage[NZAESHP_ERROR_MESSAGE_LENGTH];
    NZAEENV_HANDLE hEnv;   // NULL except for remote mode
                           // if non-NULL AE assumes ownership of this handle
} NzaeShpInitialization;

/**
    @brief Maximum numeric precision constants.
    @ingroup Shaper
    @see nzaeShpAddOutputColumnNumeric
*/
static const int NZAE_NUMERIC32_MAX_PRECISION = 9;

/**
    @brief Maximum numeric precision constants.
    @ingroup Shaper
    @see nzaeShpAddOutputColumnNumeric
*/
static const int NZAE_NUMERIC64_MAX_PRECISION = 18;

/**
    @brief Maximum numeric precision constants.
    @ingroup Shaper
    @see nzaeShpAddOutputColumnNumeric
*/
static const int NZAE_NUMERIC128_MAX_PRECISION = 38;

/**
    @brief Initialization to be called near the beginning of the process.
    @ingroup specialinit
    @param arg The initialization argument.
    @return The Shaper return code.
*/
NzaeShpRcCode nzaeShpInitialize(NzaeShpInitialization * arg);

/**
    @brief Closes the handle when done.
    @param handle The Shaper handle.
    @ingroup Shaper
*/
void nzaeShpClose(NZAESHP_HANDLE handle);

/**
    @brief Gets metadata about the AE Shaper.
    @ingroup Shaper
    @param handle The Shaper handle.
    @param arg The metadata to be filled out. Created by the caller.
    @return The Shaper return code.
*/
NzaeShpRcCode nzaeShpGetMetadata(NZAESHP_HANDLE handle, NzaeShpMetadata * arg);

/**
    @brief Gets runtime information about the AE Shaper.
    @param handle The Shaper handle.
    @param arg The runtime to be filled out. Created by the caller.
    @ingroup Shaper
    @return The Shaper return code.
*/
NzaeShpRcCode nzaeShpGetRuntime(NZAESHP_HANDLE handle, NzaeRuntime * arg);

/**
    @brief Returns TRUE if the default for system catalog names is upper
    case.
    @param handle The Shaper handle.
    @param result TRUE if catalog is upper case.
    @ingroup Shaper
    @return The Shaper return code.
*/
NzaeShpRcCode nzaeShpSystemCatalogIsUpper(NZAESHP_HANDLE handle, bool * result);

/**
    @brief Gets the input column data.

    If isConstant is FALSE then the value is always NULL.
    @param handle The Shaper handle.
    @param index The input index.
    @param data The input data.
    @ingroup Shaper
    @return The Shaper return code.
*/
NzaeShpRcCode nzaeShpGetInputColumn(NZAESHP_HANDLE handle, int index, NzudsData ** data);

/**
    @brief Adds String Output Columns.
    @param handle The Shaper handle.
    @param dataType The data type.
    @param columnName The column name.
    @param size The column size.
    @ingroup Shaper
    @return The Shaper return code.
*/
NzaeShpRcCode nzaeShpAddOutputColumnString(NZAESHP_HANDLE handle, NzudsDataType dataType,
    const char * columnName, int size);

/**
    @brief Adds Numeric Output Columns.
    @param handle The Shaper handle.
    @param dataType The data type.
    @param columnName The column name.
    @param precision The column precision.
    @param scale The column scale.
    @ingroup Shaper
    @return The Shaper return code.
*/
NzaeShpRcCode nzaeShpAddOutputColumnNumeric(NZAESHP_HANDLE handle, NzudsDataType dataType,
    const char * columnName, int precision, int scale);

/**
    @brief Adds Non string/numeric Output Columns.
    @param handle The Shaper handle.
    @param dataType The data type.
    @param columnName The column name.
    @ingroup Shaper
    @return The Shaper return code.
*/
NzaeShpRcCode nzaeShpAddOutputColumn(NZAESHP_HANDLE handle, NzudsDataType dataType,
    const char * columnName);

/**
    @brief Returns the number of output columns added by the user.
    @param handle The Shaper handle.
    @ingroup Shaper
    @return The number of output columns.
*/
int nzaeShpGetNumOutputColumns(NZAESHP_HANDLE handle);

/**
    @brief Gets information about an output column added by the user.
    @param handle The Shaper handle.
    @param index The output column index.
    @param info The output information.
    @ingroup Shaper
    @return The Shaper return code.
*/
NzaeShpRcCode nzaeShpGetOutputColumnInfo(NZAESHP_HANDLE handle,
    int index, NzaeShpOutputColumnInfo * info);

/**
    @brief Updates the shape and size information in the Netezza system.
    @param handle The Shaper handle.
    @ingroup Shaper
    @return The Shaper return code.
*/
NzaeShpRcCode nzaeShpUpdate(NZAESHP_HANDLE handle);

/**
    @brief Indicates that this AE has encountered an error condition.

    The AE is complete and should exit after this call.
    Message is built like printf.
    @param handle The Shaper handle.
    @param _template The printf-stlye template.
    @ingroup Shaper
    @return The Shaper return code.
*/
NzaeShpRcCode nzaeShpUserError(NZAESHP_HANDLE handle, const char * _template, ...);

/**
    @brief Indicates that the AE Shaper is still active and not hanging
    @param handle The Shaper handle.
    @ingroup Shaper
    @return The Shaper return code.
 */
NzaeShpRcCode nzaeShpPing(NZAESHP_HANDLE handle);

/**
    @brief Logs the specified message.
    @param handle The Shaper handle.
    @param level The log level.
    @param message The log message.
    @ingroup Shaper
    @return The Shaper return code.
*/
NzaeShpRcCode nzaeShpLog(NZAESHP_HANDLE handle, NzaeLogLevel level, const char * message);

/**
   @brief Gets the message text for last error that occurred.
    @param handle The Shaper handle.
   @ingroup Shaper
   @return The message text of the last occurring error.
*/
const char * nzaeShpGetLastErrorText(NZAESHP_HANDLE handle);

/**
   @brief Gets the code for last error that occurred.
    @param handle The Shaper handle.
   @ingroup Shaper
   @return The error code of the last occurring error.
*/
AeUserCode nzaeShpGetLastErrorCode(NZAESHP_HANDLE handle);

/**
   @brief Gets the AE System Log File name.
    @param handle The Shaper handle.
   @ingroup Shaper
   @return The log file name
*/
const char * nzaeShpGetSystemLogFileName(NZAESHP_HANDLE handle);

/**
   @brief Gets an AE or system environment variable. AE has precedence.
    @param handle The Shaper handle.
   @param name The variable name.
   @param result The variable value or NULL if not found.
   @ingroup Shaper
    @return The Shaper return code.
*/
NzaeShpRcCode nzaeShpGetEnv(NZAESHP_HANDLE handle, const char * name, const char ** result);

/**
    @brief For a UDF only, gets the predetermined single return data type.
    @param handle The Shaper handle.
    @param dataType The return type.
    @ingroup Shaper
    @return The Shaper return code.
*/
NzaeShpRcCode nzaeShpGetUdfReturnType(NZAESHP_HANDLE handle, NzudsDataType * dataType);

/**
    @brief Returns the first environment entry.

    This function call is followed by repeated calls to nzaeGetNextEnvironmentEntry.
    The AE system owns the memory from this call.
    @param handle The Shaper handle.
    @param entry The first entry.
    @ingroup Shaper
*/
void nzaeShpGetFirstEnvironmentEntry(NZAESHP_HANDLE handle,
    NzaeEnvironmentEntry * entry);

/**
    @brief Returns the next environment entry.

    The first nzaeGetNextEnvironmentEntry must follow a
    call to nzaeGetFirstEnvironmentEntry.
    Returns FALSE on end.
    Key names may repeat but the current version of a keyname is given last.
    The AE system owns the memory from this call.

    @param handle The Shaper handle.
    @param entry The next entry.
    @ingroup Shaper
    @return FALSE on end.
*/
bool nzaeShpGetNextEnvironmentEntry(NZAESHP_HANDLE handle, NzaeEnvironmentEntry * entry);

/**
    @brief Gets the file path for a library name.

    Returns NULL if the library is not found.
    The AE system owns the memory from this call.
    @param h The Shaper handle.
    @param libraryName The library name.
    @param caseSensitive If TRUE, the lookup is case-sensitive.
    @ingroup Shaper
    @return The file path if found; NULL otherwise.
*/
const char * nzaeShpGetLibraryFullPath(NZAESHP_HANDLE h, const char * libraryName,
    bool caseSensitive);

/**
    @brief Returns NzaeSharedLibraryInfo of the shared library
    for this request.

    The AE system owns the memory from this call.

    @param h The Shaper handle.
    @ingroup Shaper
    @return The Shared Library information
*/
NzaeSharedLibraryInfo * nzaeShpGetLibraryInfo(NZAESHP_HANDLE h);

/**
    @brief Return NzaeSharedLibraryInfo of the shared library
    for the process. Returns NULL if the AE is not remote.

    The AE system owns the memory from this call.

    @param h The Shaper handle.
    @ingroup Shaper
    @return The Shared Library information
*/
NzaeSharedLibraryInfo * nzaeShpGetLibraryProcessInfo(NZAESHP_HANDLE h);

/**
    @brief Returns the number of parameters.
    @param h The Shaper handle.
    @ingroup Shaper
    @return The number of parameters
*/
int nzaeShpGetNumberOfParameters(NZAESHP_HANDLE h);

/**
    @brief Returns a parameter.

    The index is zero-based.
    @param h The Shaper handle.
    @param index The parameter index.
    @ingroup Shaper
    @return Parameter value
*/
const char * nzaeShpGetParameter(NZAESHP_HANDLE h, int index);


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
