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

/// \cond
/**
 *  nzaehandles.h
 *  Netezza Analytic Executables (NZAE)
 *  AE C language include file for handle definitions.
 */
/// \endcond

#ifndef __NZAEHANDLES_H__
#define __NZAEHANDLES_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
   @brief The Function Handle. An opaque handle used with Function AE functions.
   @ingroup Function
   @see Function
*/
typedef struct NZAE_HANDLE_S {} * NZAE_HANDLE;


/**
   @brief The Aggregate Handle. An opaque handle used with Aggregate AE functions.
   @ingroup Aggregate
   @see Aggregate
*/
typedef struct NZAEAGG_HANDLE_S {} * NZAEAGG_HANDLE;


/**
   @brief The Shaper Handle. An opaque handle used with Shaper and Sizer AE functions.
   @ingroup Shaper
   @see Shaper
*/
typedef struct NZAESHP_HANDLE_S {} * NZAESHP_HANDLE;


/**
   @brief The Remote Protocol Handle. An opaque handle used with Remote Protocol AE functions.
   @ingroup remoteinit
   @see remoteinit
*/
typedef struct NZAEREMPROT_HANDLE_S {} * NZAEREMPROT_HANDLE;


/**
   @brief The ConnectionPoint Handle. An opaque handle used with Connection Point AE functions.
   @ingroup remoteinit
   @see remoteinit
*/
typedef struct NZAECONPT_HANDLE_S {} * NZAECONPT_HANDLE;

//******************************************************************************
// external AE is not supported
/// \cond

/**
   @brief External Host Handle. Opaque handle used with External Host AE functions.
   @ingroup remoteinit
   @see remoteinit
*/
typedef struct NZAEEXTHOST_HANDLE_S {} * NZAEEXTHOST_HANDLE;

/**
   @brief External Host Handle. Opaque handle used with
   External Host AE information functions.
   @ingroup remoteinit
   @see remoteinit
*/
typedef struct NZAEEXTHOSTGLOBAL_HANDLE_S {} * NZAEEXTHOSTGLOBAL_HANDLE;
/// \endcond


typedef struct NZAEENV_HANDLE_S {} * NZAEENV_HANDLE;

#include "nzaeapiscommon.h"

/**
   @brief Contains a data connection handle.
   @ingroup initialization
   @see nzaeRemprotAcceptApi
   @see nzaeRemprotAcceptApiWithTimeout
   @see nzaeLocprotGetApi
*/
typedef struct NzaeApi
{
    union
    {
        /** Aggregation AE Data Connection Handle. */
        NZAEAGG_HANDLE aggregation;
        /** Function AE Data Connection Handle. */
        NZAE_HANDLE function;
        /** Shaper or Sizer AE Data Connection Handle. */
        NZAESHP_HANDLE shaper;
        /** Used internally. */
        void * any;
    } handle;
    /** Indicates the type of data connection handle stored in the union. */
    NzaeApiTypes apiType;  // defined in nzaeapiscommon.h
} NzaeApi;


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
