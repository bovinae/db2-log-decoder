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
 *  nzaedatetime.h
 *  Netezza Analytic Executables (NZAE)
 *  Date / Time conversions.
 */

#ifndef __NZAEDATETIME_H__
#define __NZAEDATETIME_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "nzudsudxtypes.h"
#include "nzudsdata.h"

/***************************************************************************
 *
 * Posix Time (a.k.a. Unix Time) is relative to
 * midnight Coordinated Universal Time (UTC) of January 1, 1970
 *
 **************************************************************************/

/**
   @brief Converts an NZ Date to an Epoch time in seconds.

   @ingroup DateTime
   @param nzDate The NZ-encoded date.
   @return The Epoch time in seconds.
*/
int64_t nzaeNzDateToPosixTimeSeconds(int32_t nzDate);


/**
   @brief Converts an NZ Date to an Epoch time in milliseconds.

   @ingroup DateTime
   @param nzDate NZ encoded date.
   @return The Epoch time in milliseconds.
*/
int64_t nzaeNzDateToPosixTimeMilliseconds(int32_t nzDate);


/**
   @brief Converts an Epoch time in seconds to an NZ Date.

   @ingroup DateTime
   @param posixTimeSeconds The Posix time in seconds.
   @return The NZ Date.
*/
int32_t nzaePosixTimeSecondsToNzDate(int64_t posixTimeSeconds);


/**
   @brief Converts an Epoch time in milliseconds to an NZ Date

   @ingroup DateTime
   @param posixTimeMilliseconds The Posix time in milliseconds.
   @return An NZ Date.
*/
int32_t nzaePosixTimeMillisecondsToNzDate(int64_t posixTimeMilliseconds);


/**
   @brief Converts an NZ Time to time in seconds.

   @ingroup DateTime
   @param nzTime The NZ-encoded time.
   @return The time in seconds.
*/
int32_t nzaeNzTimeToSeconds(int64_t nzTime);


/**
   @brief Converts an NZ Time to time in milliseconds.

   @ingroup DateTime
   @param nzTime The NZ-encoded time.
   @return Time in milliseconds
*/
int32_t nzaeNzTimeToMilliseconds(int64_t nzTime);


/**
   @brief Converts Time in seconds to an NZ Time.

   @ingroup DateTime
   @param seconds The time in seconds.
   @return The NZ Time.
*/
int64_t nzaeSecondsToNzTime(int32_t seconds);


/**
   @brief Converts Time in milliseconds to an NZ Time.

   @ingroup DateTime
   @param milliseconds The time in milliseconds.
   @return The NZ Time.
*/
int64_t nzaeMillisecondsToNzTime(int32_t milliseconds);


/**
   @brief Converts an NZ Timestamp to an Epoch time in seconds.

   @ingroup DateTime
   @param nzTimestamp The NZ-encoded timestamp.
   @return The Epoch time in seconds.
*/
int64_t nzaeNzTimestampToPosixTimeSeconds(int64_t nzTimestamp);


/**
   @brief Converts an NZ Timestamp to Epoch time in milliseconds.

   @ingroup DateTime
   @param nzTimestamp NZ-encoded timestamp.
   @return The Epoch time in milliseconds.
*/
int64_t nzaeNzTimestampToPosixTimeMilliseconds(int64_t nzTimestamp);


/**
   @brief Converts an Epoch time in seconds to an NZ Timestamp.

   @ingroup DateTime
   @param posixTimeSeconds The Posix time in seconds.
   @return The NZ Timestamp.
*/
int64_t nzaePosixTimeSecondsToNzTimestamp(int64_t posixTimeSeconds);


/**
   @brief Converts an Epoch time in milliseconds to an NZ Timestamp.

   @ingroup DateTime
   @param posixTimeMilliseconds The Posix time in milliseconds.
   @return The NZ Timestamp.
*/
int64_t nzaePosixTimeMillisecondsToNzTimestamp(int64_t posixTimeMilliseconds);


/**
   @brief Converts an NZ Interval to seconds.

   @ingroup DateTime
   @param nzInterval The NZ-encoded interval.
   @return Seconds.
*/
int64_t nzaeIntervalToSeconds(const NzudsInterval * nzInterval);


/**
   @brief Converts an NZ Interval to milliseconds.

   @ingroup DateTime
   @param nzInterval The NZ-encoded interval.
   @return Milliseconds.
*/
int64_t nzaeIntervalToMilliseconds(const NzudsInterval * nzInterval);


/**
   @brief Convert Seconds to an NZ Interval.

   @ingroup DateTime
   @param seconds Seconds.
   @param nzInterval The interval output.
*/
void nzaeSecondsToInterval(int64_t seconds, NzudsInterval * nzInterval);


/**
   @brief Convert Milliseconds to an NZ Interval.

   @ingroup DateTime
   @param milliseconds Milliseconds.
   @param nzInterval The interval output.
*/
void nzaeMillisecondsToInterval(int64_t milliseconds, NzudsInterval * nzInterval);


/**
   @brief Converts an NZ TimeTz Offset to minutes.

   @ingroup DateTime
   @param nzTimeTzOffset The NZ TimeTz offset.
   @return Minutes.
*/
int32_t nzaeNzTimeTzOffsetToMinutes(int32_t nzTimeTzOffset);


/**
   @brief Convert Minutes to NZ TimeTz Offset.

   @ingroup DateTime
   @param minutes Minutes.
   @return The NZ TimeTz offset.
*/
int32_t nzaeMinutesToNzTimeTzOffset(int32_t minutes);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
