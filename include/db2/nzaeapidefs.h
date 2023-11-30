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
 *  nzaeapidefs.h
 *  Netezza Analytic Executables (NZAE)
 *  Data definitions used by multiple AE API's.
 */

#ifndef __NZAEAPIDEFS_H__
#define __NZAEAPIDEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

static int NZAE_LDK_VERSION = 3;

/**
   @brief Shared library information.
   @ingroup runtimeenv
   @see nzaeGetLibraryInfo
   @see nzaeAggGetLibraryInfo
   @see nzaeShpGetLibraryInfo
*/
typedef struct NzaeSharedLibraryInfo
{
    /** @brief The number of libraries. */
    int numLibraries;
    /** @brief An array of library names. */
    const char ** libraryNames;
    /** @brief An array of the library's full paths. */
    const char ** libraryFullPaths;

    /** @brief An array of the autoload settings. */
    bool * autoLoad;
} NzaeSharedLibraryInfo;

/**
   @brief The environment entry.
   @ingroup runtimeenv
   @see nzaeGetFirstEnvironmentEntry
   @see nzaeGetNextEnvironmentEntry
   @see nzaeAggGetFirstEnvironmentEntry
   @see nzaeAggGetNextEnvironmentEntry
   @see nzaeShpGetFirstEnvironmentEntry
   @see nzaeShpGetNextEnvironmentEntry
*/
typedef struct NzaeEnvironmentEntry
{
    const char * name;
    const char * value;
} NzaeEnvironmentEntry;

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
