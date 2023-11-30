/*
 *  nzuds.h
 *  Netezza User Data Serialization
 * (C) Copyright IBM Corp. 2008, 2011  All Rights Reserved.
 */

#ifndef __NZUDSDATA_H__
#define __NZUDSDATA_H__
#include "nzudsudxtypes.h"

/** @ingroup Data
    field data to serialize / deserialize<br>
    The memory that this struct points to belongs
    to the class.<br><br>

    The data in this struct is valid until it is used in another
    nzudsReadNext, any write operation is performed, or the handle is closed.<br><br>

    string types always have an extra null terminator not
    included in the length<br>
    @see Data
*/
typedef struct NzudsData
{
    union
    {
        const int8_t * pInt8;
        const int16_t * pInt16;
        const int32_t * pInt32;
        const int64_t * pInt64;
        const float * pFloat;
        const double * pDouble;
        const NzudsNumeric32 * pNumeric32;
        const NzudsNumeric64 * pNumeric64;
        const NzudsNumeric128 * pNumeric128;
        const int32_t * pDate;
        const int64_t * pTime;
        const int64_t * pTimeStamp;
        const NzudsInterval * pInterval;
        const NzudsTimeTz * pTimeTz;
        const int8_t * pBool;
        const char * pFixedString;
        const char * pVariableString;
        const char * pNationalFixedString;
        const char * pNationalVariableString;
        const char * pGeometryString;
        const char * pVarbinaryString;
    } data;
    /**
        Data Type of this struct<br>
        Determines correct pointer in data union
        if data in not null.
        @see NzudsDataType
    */
    NzudsDataType type;
    /**
        length of data
    */
    int32_t length;
    /**
        if value is true then data column is SQL null
    */
    int8_t isNull;

    /**
        This union should be considered private
    */
    union
    {
        NzudsNumeric32 numeric32;
        NzudsNumeric64 numeric64;
        NzudsNumeric128 numeric128;
        NzudsTimeTz timeTz;
        NzudsInterval interval;
        int8_t int8Val;
        int16_t int16Val;
        int32_t int32Val;
        int64_t int64Val;
        float floatVal;
        double doubleVal;
        int32_t dateVal;
        int64_t timeVal;
        int64_t timeStampVal;
        int8_t boolVal;
    } privateData;

} NzudsData;

#endif
