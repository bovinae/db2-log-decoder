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
 *  nzaeusercodes.h
 *  Netezza Analytic Executables (NZAE)
 *  Numeric user return codes. Useful for multi human-language support.
 */

#ifndef __NZAEUSERCODES_H__
#define __NZAEUSERCODES_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
   Error codes for user visible feedback
   @ingroup usercodes
*/
typedef enum AeUserCode
{
    AE_UC_OK = 0,
    AE_UC_INTERNAL = 1,  // internal error occured
    AE_UC_BAD_INPUT_INDEX = 2,  // an input index is out of range
    AE_UC_BAD_OUTPUT_INDEX = 3,  // an output index is out of range
    AE_UC_INVALID_NULL_PARM = 4,  // a required parameter was NULL
    AE_UC_INVALID_NON_POSITIVE_PARM = 5,  // invalid non-positive parameter
    AE_UC_FEATURE_NOT_IMPLEMENTED = 6,  // feature not implemented
    AE_UC_INPUT_NOT_AVAILABLE = 7,  // input not avaiable
    AE_UC_INPUT_NOT_ALLOWED = 8,  // input not allowed
    AE_UC_OUTPUT_NOT_ALLOWED = 9,  // output not allowed
    AE_UC_INVALID_CONVERSION = 10,  // invalid data conversion
    AE_UC_INVALID_HANDLE = 11,  // invalid handle
    AE_UC_UPDATE_EXPECTED = 12,  // agg: update expected
    AE_UC_UPDATE_NOT_EXPECTED = 13,  // agg: update not expected
    AE_UC_INVALID_SHAPER_TYPE = 14,  // wrong shaper function for this data type
    AE_UC_INVALID_SHAPER_PRECISION = 15,  // precision too large for numeric type
    AE_UC_INVALID_SIZER_TYPE = 16,  // type doesn't match sizer return
    AE_UC_INVALID_SIZER_COUNT = 17,  // more than one sizer result
    AE_UC_INVALID_SIZER_CATALOG_IS_UPPER = 18,   // not available in sizer
    AE_UC_INVALID_SIZER_GET_UDF_RETURN_TYPE = 19,  // not available in sizer
    AE_UC_BAD_NEXT_ENVIRONMENT = 20,  // next environment before first
    AE_UC_NOT_IN_PARTITION_MODE = 21, // not in partition mode
    AE_UC_PARTITION_NOT_ALLOWED = 22, // next partition not allowed
    AE_UC_BAD_SQL_PARAMETER_INDEX = 23,   // bad SQL parameter index
    AE_UC_INVALID_SHAPER_COUNT = 24, // Exceeded shaper output column limit
    AE_UC_DUPLICATE_COLUMN_NAME = 25 // Duplicate column name
} AeUserCode;

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
