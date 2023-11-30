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
 *  nzaemetadata.h
 *  Netezza Analytic Executables (NZAE)
 *  main client C language include file for NZAE framework.
 */

#ifndef __NZAEMETADATA_H__
#define __NZAEMETADATA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "nzudsudxtypes.h"
#include "nzudsdata.h"

/**
    @brief Specialized information about how this AE is being invoked.

    This function is used infrequently.
    @ingroup Function
    @see NzaeMetadata
*/
typedef enum NzaeCorrelationType
{
    NzaeUnknownCorrelationType = 0,
    NzaeUncorrelated = 1,
    NzaeInnerCorrelation = 2,
    NzaeLeftCorrelation = 3
} NzaeCorrelationType;

/**
    @brief Metadata describing the input and output rows of the AE.

    The memory pointed to by inputTypes and outputTypes
    belongs to the handle and should not be freed by the user.
    @ingroup function
    @see nzaeGetMetadata
*/

typedef struct NzaeMetadata
{
    /** The number of input columns. */
    int inputColumnCount;
    /** The number of output columns. */
    int outputColumnCount;
    /** The input data types. NzudsDataType is defined elsewhere. */
    NzudsDataType * inputTypes;
    /** Determines if the input type is a constant, 0 or 1. */
    int * inputIsConstant;
    /** The output data types. NzudsDataType is defined elsewhere. */
    NzudsDataType * outputTypes;
    /** The precision of the numeric or the max size of the string. */
    int * inputSizes;
    /** The precision of the numeric or the max size of the string. */
    int * outputSizes;
    /** The scale of the numeric, otherwise 0. */
    int * inputScales;
    /** The scale of the numeric, otherwise 0. */
    int * outputScales;

    /** Row restriction; if TRUE, exactly one output row
       is required per input row
       and no output is allowed after the end of the data */
    bool oneOutputRowRestriction;

    /** Correlation: see definition of NzaeCorrelationType. */
    NzaeCorrelationType correlationType;
    /** Invoked with OVER. */
    bool hasOver;
    /** Invoked with SORT. */
    bool hasSort;
    /** Has partition. */
    bool hasPartition;

} NzaeMetadata;

typedef struct NzaeAggMetadata
{
    int numColumns;
    NzudsDataType * types;
    int * sizes;
    int * scales;
} NzaeAggMetadata;

typedef enum NzaeAggMessageType
{
    NZAEAGG_NOT_SET = -2,
    NZAEAGG_ERROR = -1,
    NZAEAGG_END = 0,
    NZAEAGG_INITIALIZE = 1,
    NZAEAGG_ACCUMULATE = 2,
    NZAEAGG_MERGE = 3,
    NZAEAGG_FINAL_RESULT = 4,
} NzaeAggMessageType;


#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
