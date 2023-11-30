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

#ifndef __NZAEAPISCOMMON_H__
#define __NZAEAPISCOMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
   @ingroup initialization
   @brief The Data Connection API Type.
   @see NzaeApi
   @see nzaeRemprotGetEnvironmentApiType
*/
typedef enum NzaeApiTypes
{
    // Note that for python UDFs we need to destinguish between scalar and table
    // functions. Therefore, NZAE_API_FUNCTION is not used - it's only kept here
    // because it might be needed for the R UDFs. Since the according R UDF code
    // can't be found in clearcase we keep this type though. Python UDFs use all
    // other types, but do not need NZAE_API_FUNCTION. See also sqng/sqlng_udf.C
    NZAE_API_UNKNOWN = 0,
    NZAE_API_FUNCTION = 1,
    NZAE_API_AGGREGATION = 2,
    NZAE_API_SHAPER = 3,
    NZAE_API_SCALAR_FUNCTION = 4,
    NZAE_API_TABLE_FUNCTION = 5
} NzaeApiTypes;

/**
   @brief The Aggregate Function Type.
   @ingroup Aggregate
*/

typedef enum NzaeAggType
{
    NzaeAggUnknown,
    NzaeAggGrouped,
    NzaeAggAnalytic
} NzaeAggType;

/*
   The AE Communication Model
   not currently in documentation
*/

typedef enum NzaeModel
{
    NzaeModelUnknown,
    NzaeModelLocal,
    NzaeModelRemote,
    NzaeModelExternal
} NzaeModel;

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
