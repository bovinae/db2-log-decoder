#ifndef __NZUDSUDXTYPES_H__
#define __NZUDSUDXTYPES_H__

/** @ingroup Data
   Column Data Type Enumeration<br>
   This must match nz::udx::udxBase::DataType in udxbase.h
   @see Data
*/
typedef enum NzudsDataType
{
    /** unknown data type */
    NZUDSUDX_UNKNOWN=-1,
    /** fixed string */
    NZUDSUDX_FIXED,
    /** variable string */
    NZUDSUDX_VARIABLE,
    /** fixed national string */
    NZUDSUDX_NATIONAL_FIXED,
    /** variable national string */
    NZUDSUDX_NATIONAL_VARIABLE,
    /** boolean */
    NZUDSUDX_BOOL,
    /** date */
    NZUDSUDX_DATE,
    /** time */
    NZUDSUDX_TIME,
    /** time zone */
    NZUDSUDX_TIMETZ,
    /** numeric 32 */
    NZUDSUDX_NUMERIC32,
    /** numeric 64 */
    NZUDSUDX_NUMERIC64,
    /** numeric 128 */
    NZUDSUDX_NUMERIC128,
    /** float */
    NZUDSUDX_FLOAT,
    /** double */
    NZUDSUDX_DOUBLE,
    /** interval */
    NZUDSUDX_INTERVAL,
    /** 1 byte integer */
    NZUDSUDX_INT8,
    /** 2 byte integer */
    NZUDSUDX_INT16,
    /** 4 byte integer */
    NZUDSUDX_INT32,
    /** 8 byte integer */
    NZUDSUDX_INT64,
    /** time stamp */
    NZUDSUDX_TIMESTAMP,
    /** geometry */
    NZUDSUDX_GEOMETRY,
    /* varbinary */
    NZUDSUDX_VARBINARY,
    /** greater than any data type enum value */
    NZUDSUDX_MAX_TYPE
} NzudsDataType;

/**
    @brief Metadata describing input rows of the AE. 
    The memory pointed to by inputTypes
    belongs to the handle and should not be freed by the user.
    @see nzaeShpGetMetadata
*/
typedef struct NzaeShpMetadata
{
    /** @brief The number of input columns. */
    int inputColumnCount;

    /** @brief The number or input data types, NzudsDataType is defined elsewhere. */
    NzudsDataType * inputTypes;

    /** @brief Determines if the input type is a constant, 0 or 1. */
    int * inputIsConstant;

    /** @brief The precision of the numeric or the max size of string. */
    int * inputSizes;

    /** @brief The scale of the numeric, otherwise 0. */
    int * inputScales;

    /** @brief Row restriction; if TRUE exactly one output row
       is required per input row
       and no output is allowed after the end of the data. */
    bool oneOutputRowRestriction;

} NzaeShpMetadata;

/**
    @brief Information about a user-added output column. 
    @see nzaeShpGetOutputColumnInfo
*/
typedef struct NzaeShpOutputColumnInfo {
    const char * columnName;
    NzudsDataType dataType;
    int size;
    int precision;
    int scale;
} NzaeShpOutputColumnInfo;

typedef void* DB2AE_SHP_HANDLE;

// numeric

/** @ingroup Data
   Digit definition for numeric data types
   @see Data
*/
typedef int32_t NzudsNumericDigit;

/** @ingroup Data
   Numeric 32 data type definition
   @see Data
*/
typedef struct NzudsNumeric32
{
    NzudsNumericDigit digit[1];
} NzudsNumeric32;

/** @ingroup Data
   Numeric 64 data type definition
   @see Data
*/
typedef struct NzudsNumeric64
{
    NzudsNumericDigit digit[2];
} NzudsNumeric64;

/** @ingroup Data
   Numeric 128 data type definition
   @see Data
*/
typedef struct NzudsNumeric128
{
    NzudsNumericDigit digit[4];
} NzudsNumeric128;

/** @ingroup Data
   Time Zone data type definition<br>
   Uses the int64 time value and adds an int32 time zone as well.
   The time zone is represented in seconds.
   @see Data
*/
typedef struct NzudsTimeTz
{
    int64_t time;
    int32_t zone;
} NzudsTimeTz;

/** @ingroup Data
   Interval data type definition<br>
   It has microsecond resolution and ranges from +/- 178000000 years.
   The time part represents
   everything but months and years (microseconds) and the month part represents
   months and years.
   @see Data
*/
typedef struct NzudsInterval
{
    int64_t time;
    int32_t month;
} NzudsInterval;

#endif
