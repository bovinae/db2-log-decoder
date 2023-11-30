/*******************************************************************************
 *
 * IBM CONFIDENTIAL
 * OCO SOURCE MATERIALS
 *
 * COPYRIGHT:  P#2 P#1
 *             (C) COPYRIGHT IBM CORPORATION Y1, Y1
 *
 * The source code for this program is not published or otherwise divested of
 * its trade secrets, irrespective of what has been deposited with the U.S.
 * Copyright Office.
 *
 ******************************************************************************/
#if ! defined db2ExtTableInterface_HEADER_INCLUDED
#define db2ExtTableInterface_HEADER_INCLUDED
/**
   \file db2ExtTableInterface.h
   \brief Interface between execution environement and an external table reader
 */

#include "sql.h"
#include "db2ApiDf.h"
#include "sqlExternalPredicates.h"
#include "sqlExternalTables.h"
#include "sqlExternalRequest.h"

#ifdef __cplusplus
extern "C" {
#endif

struct db2ExtTableInterface;
struct db2ExtTableInit;

//! Version type
typedef db2Uint32 db2ExtTableVersion;

//! Version identifiers
#define DB2_EXT_TABLE_INTFC_INVALID_VERSION SQL_FUTUREL
#define DB2_EXT_TABLE_INTFC_V1              SQL_REL10600

//! Current version
#define DB2_EXT_TABLE_INTFC_CURRENT_VERSION DB2_EXT_TABLE_INTFC_V1

//! Error Code Definition.
//! NOTE:  When adding entries to this enum, the extTableErrMap array
//!        in sqlerExtTableFmp.C also needs to be updated to have a 
//!        proper mapping of External table internal return codes to 
//!        appropriate sqlzRC values.    
enum db2ExtTableRC
{
   //! Success error code.
   DB2_EXT_TABLE_RC_OK             = 0,

   //! An error has occurred and was reported to the External Table infrastructure.
   DB2_EXT_TABLE_RC_ERROR_OCCURRED = -1,

   //! The external table operation was interrupted.
   DB2_EXT_TABLE_RC_INTERRUPTED    = -2,

   //! An internal External Table infrastructure error has occurred.
   DB2_EXT_TABLE_RC_INTERNAL_ERROR = -3,

   //! A signal to the writers DB2 has inserted all rows.
   DB2_EXT_TABLE_RC_NO_MORE_ROWS = -4,

   //! Bad file name
   //! NOTE: This is only used for Netezza External table support and will
   //        not be returned in Big SQL.  Will map this to internal error
   //        if detected in Big SQL.   
   DB2_EXT_TABLE_RC_BAD_FILE_NAME = -5,

   //! Need escape char
   //! NOTE: This is only used for Netezza External table support and will
   //        not be returned in Big SQL.  Will map this to internal error
   //        if detected in Big SQL.   
   DB2_EXT_TABLE_RC_NEED_ESCAPE = -6,

   //! Bad file permission
   DB2_EXT_TABLE_RC_BAD_FILE_USER = -7,

   //! Invalid codepage conversion
   DB2_EXT_TABLE_RC_BAD_CP_CONVERSION = -8,

   //! Unrecoverable Java Error Occured
   DB2_EXT_TABLE_RC_JAVA_ERROR = -9,

   //! Insufficient system resources, memory for unload
   DB2_EXT_TABLE_RC_INSSYS = SQLE_RC_INSSYS,

   DB2_EXT_TABLE_RC_FILEIO_ERR = SQLE_RC_FILEIO_ERR,

   DB2_EXT_TABLE_RC_DBBAD = SQLE_RC_DBBAD
};

//! Maximum length of a message token.
#define DB2_EXT_TABLE_MSGTKN_MAX_SZ 70

/**
   \brief Message token structure.  Used by the raiseError callback function.
 */
struct db2ExtTableMessageTokens
{
   //! Null terminated message token.
   char tokenString[DB2_EXT_TABLE_MSGTKN_MAX_SZ + 1];
};

//! Enumeration of all possible configuration parameter data types
enum db2ExtTableConfigDataType
{
   //! Invalid data type
   DB2_EXT_TABLE_CFG_TYPE_INVALID      = 0,

   //! db2Uint64 data type
   DB2_EXT_TABLE_CFG_TYPE_DB2UINT64,

   //! double data type
   DB2_EXT_TABLE_CFG_TYPE_DOUBLE,

   //! bool data type
   DB2_EXT_TABLE_CFG_TYPE_BOOL,

   //! char data type
   DB2_EXT_TABLE_CFG_TYPE_CHAR,

   //! string (null-delimited char *) data type
   DB2_EXT_TABLE_CFG_TYPE_STRING,
};

//! Enumeration of all possible configuration parameters
enum db2ExtTableConfigKey
{
   //! Invalid key
   DB2_EXT_TABLE_CFG_KEY_INVALID           = 0,

   //! Current DB2 member/node ID.  The range is 0..(SQL_PDB_MAX_NUM_NODE-1).
   //!  Typically represented as a DB2_EXT_TABLE_CFG_TYPE_DB2UINT64.
   DB2_EXT_TABLE_CFG_KEY_MEMBER_ID,

   //! Number of CPUs the external table infrastructure is configured to use.
   //!  The range is 1..n, where n is the number of processors on the machine.
   //!  Typically represented as a DB2_EXT_TABLE_CFG_TYPE_DB2UINT64.
   DB2_EXT_TABLE_CFG_KEY_ALLOWED_CPUS,

   //! Amount of memory, in bytes, that the external table infrastructure is
   //!  configured to use.  The range is 1..n, where n is the number of bytes
   //!  of RAM on the machine.
   //!  Typically represented as a DB2_EXT_TABLE_CFG_TYPE_DB2UINT64.
   DB2_EXT_TABLE_CFG_KEY_ALLOWED_MEMORY,

   //! FIXME comment
   DB2_EXT_TABLE_CFG_KEY_WRITE_BUFFER_SIZE,
};

//! Structure to represent a single External Table configuration parameter
struct db2ExtTableCfgParm
{
   //! Key for this configuration parameter
   db2ExtTableConfigKey cfgKey;

   //! Data type this configuration parameter is stored as
   db2ExtTableConfigDataType cfgDataType;

   //! Setting of this configuration parameter.  Based on the cfgDataType,
   //!  the appropriate union member should be used, i.e.
   //!  db2ExtTableCfgParm *pCfg;
   //!  if ( DB2_EXT_TABLE_CFG_TYPE_DB2UINT64 == pCfg->cfgDataType )
   //!     db2Uint64 value = pCfg->db2Uint64Val ;
   union
   {
      db2Uint64 db2Uint64Val;
      double    doubleVal;
      bool      boolVal;
      char      charVal;
      char*     stringVal;
   };
};

//! Enumeration of all possible routine langauges
enum db2RoutineLanguage
{
   //! Invalid routine language
   DB2_EXT_RTN_LANG_INVALID = 0,

   //! Java Language
   DB2_EXT_RTN_LANG_JAVA,

   //! C Language
   DB2_EXT_RTN_LANG_C,
};

//! Routine Library
struct db2RoutineLibrary
{
   //! Library language
   db2RoutineLanguage libraryLanguage;

   //! Language specific library handle.
   //!  \li Java - This is a jclass object.
   void* routineLibraryHandle;
};

//! Parameter structure passed to loadRoutineLibrary
struct db2ExtTableRoutineLibaryParms
{ 
   //! Language of the library that the caller wishes to load.
   db2RoutineLanguage routineLanguage;

   //! Null terminated name of the library.
   char*    libraryName;

   //! Node index of the routine in the external predicates.
   db2int32 rtnPredNodeIndex;

   //! Explicit padding to 8 bytes.
   db2int32 padding1;
};

/**
   \brief
      Interface provided by the target of the external table reader.
 */
struct db2ExtTableFuncPtrs
{
   /**
      \brief Invoked to initialize a scan using an external table reader.

      \param db2ExtTableInterface [in] - External Table Interface

      \retval DB2_EXT_TABLE_RC_OK
      \retval DB2_EXT_TABLE_RC_ERROR_OCCURRED
    */
   db2ExtTableRC (SQL_API_FN *openTableScan)(db2ExtTableInterface*);

   /**
      \brief Invoked to start reader scan returning data to the execution
             environment.

      \param db2ExtTableInterface [in] - External Table Interface

      \retval DB2_EXT_TABLE_RC_OK
      \retval DB2_EXT_TABLE_RC_ERROR_OCCURRED
      \retval DB2_EXT_TABLE_RC_INTERRUPTED
    */
   db2ExtTableRC (SQL_API_FN *fetchRows)(db2ExtTableInterface*);

   /**
      \brief Invoked to close a reader scan.

      \param db2ExtTableInterface [in] - External Table Interface

      \retval DB2_EXT_TABLE_RC_OK
      \retval DB2_EXT_TABLE_RC_ERROR_OCCURRED
    */
   db2ExtTableRC (SQL_API_FN *closeTableScan)(db2ExtTableInterface*);

   /**
      \brief Invoked to start an INSERT operation against an external table.

      The external table library may invoked getReadBuffer/releaseReadBuffer
      to ingest communication buffers that contain row data.

      \param db2ExtTableInterface [in] - External Table Interface

      \retval DB2_EXT_TABLE_RC_OK
      \retval DB2_EXT_TABLE_RC_ERROR_OCCURRED
      \retval DB2_EXT_TABLE_RC_INTERRUPTED
    */
   db2ExtTableRC (SQL_API_FN *insertRows)(db2ExtTableInterface*);

   /**
      \brief Invoked to close an external table library INSERT operation.

      \param db2ExtTableInterface [in] - External Table Interface

      \retval DB2_EXT_TABLE_RC_OK
      \retval DB2_EXT_TABLE_RC_ERROR_OCCURRED
    */
   db2ExtTableRC (SQL_API_FN *closeInsert)(db2ExtTableInterface*);

   /**
      \brief Invoked to interrupt the current external table library request.

      This function is not invoked from the thread that hosts the external
      table context.  The interrupt operation is expected to be asynchronous.

      param db2ExtTableInterface [in] - External Table Interface

      \retval DB2_EXT_TABLE_RC_OK
      \retval DB2_EXT_TABLE_RC_ERROR_OCCURRED
    */
   db2ExtTableRC (SQL_API_FN *interruptRequest)(db2ExtTableInterface*);

   /**
      \brief Invoked to start an UPDATE operation against an external table.

      The external table library may invoke getReadBuffer/releaseReadBuffer
      to ingest communication buffers that contain row data.

      \param db2ExtTableInterface [in] - External Table Interface

      \retval DB2_EXT_TABLE_RC_OK
      \retval DB2_EXT_TABLE_RC_ERROR_OCCURRED
      \retval DB2_EXT_TABLE_RC_INTERRUPTED
    */
   db2ExtTableRC (SQL_API_FN *updateRows)(db2ExtTableInterface*);

   /**
      \brief Invoked to close an external table library UPDATE operation.

      \param db2ExtTableInterface [in] - External Table Interface

      \retval DB2_EXT_TABLE_RC_OK
      \retval DB2_EXT_TABLE_RC_ERROR_OCCURRED
    */
   db2ExtTableRC (SQL_API_FN *closeUpdate)(db2ExtTableInterface*);

   /**
      \brief Invoked to start an DELETE operation against an external table.

      The external table library may invoke getReadBuffer/releaseReadBuffer
      to ingest communication buffers that contain row data.

      \param db2ExtTableInterface [in] - External Table Interface

      \retval DB2_EXT_TABLE_RC_OK
      \retval DB2_EXT_TABLE_RC_ERROR_OCCURRED
      \retval DB2_EXT_TABLE_RC_INTERRUPTED
    */
   db2ExtTableRC (SQL_API_FN *deleteRows)(db2ExtTableInterface*);

   /**
      \brief Invoked to close an external table library DELETE operation.

      \param db2ExtTableInterface [in] - External Table Interface

      \retval DB2_EXT_TABLE_RC_OK
      \retval DB2_EXT_TABLE_RC_ERROR_OCCURRED
    */
   db2ExtTableRC (SQL_API_FN *closeDelete)(db2ExtTableInterface*);
};

/**
   \brief
      Process Initialization callback interface provided by the execution environment.
 */
struct db2ExtTableInitCallbacks
{
   /**
      \brief Raise an SQL error that was encountered during External Table
      process initialization.

      \param db2ExtTableInit [in] - External Table DLL init interface
      \param sqlCode [in] - SQL Code
      \param numMsgTokens [in] - Number of message tokens
      \param db2ExtTableMessageTokens [in] - Array of message tokens.

      \retval DB2_EXT_TABLE_RC_OK
      \retval DB2_EXT_TABLE_RC_ERROR_OCCURRED
    */
   db2ExtTableRC (SQL_API_FN *raiseError)(db2ExtTableInit*, const SQL_API_RC sqlCode, const db2Uint16 numMsgTokens, db2ExtTableMessageTokens*);

   /**
      \brief
         Attach a thread to the execution environment.  This function must be invoked
         by a thread before it can invoke any other callback, unless the thread handled
         the dllOneTimeInit() call or a contextInit() call.

         Once this callback has been successfully invoked, the thread must invoke
         the detachFromExecutionEnvironment callback before terminating.

      \retval DB2_EXT_TABLE_RC_OK
      \retval DB2_EXT_TABLE_RC_INTERNAL_ERROR
    */
   db2ExtTableRC (SQL_API_FN *attachToExecutionEnvironment)(void);

   /**
      \brief
         Detach a thread from the execution environment.  This function must be invoked
         if a thread invoked attachToExecutionEnvironment, before the thread
         terminates.
 
         The thread can call attachToExecutionEnvironment again after this call.
         This function is a no-op if the thread is not attached to the execution
         environment.
    */
   void (SQL_API_FN *detachFromExecutionEnvironment)(void);
};

/**
   \brief
      Callback interface provided by the execution environment.
 */
struct db2ExtTableCallbacks
{
   /**
      \brief Invoked by external table reader to get write access to a buffer.

      This function is thread safe.  It may be invoked by any external table
      library thread.  The external table context must be running a fetchRows
      request at the time this function is invoked.

      A single thread is only allowed to have one outstanding writable buffer.

      \param db2ExtTableInterface [in] - External Table Interface
      \param ppBuffer [out] - Pointer to buffer of size \c pBufferSize
      \param pBufferSize [out] - Size of buffer

      \retval DB2_EXT_TABLE_RC_OK
      \retval DB2_EXT_TABLE_RC_ERROR_OCCURRED
      \retval DB2_EXT_TABLE_RC_INTERRUPTED
      \retval DB2_EXT_TABLE_RC_INTERNAL_ERROR
    */
   db2ExtTableRC (SQL_API_FN *getWriteBuffer)(db2ExtTableInterface*, db2Uint8** ppBuffer, db2Uint64* pBufferSize);

   /**
      \brief Invoked by external table reader to release buffer back to the
      execution environment.  Readers access to buffer is lost after this call.

      The external table library must release all communication buffers before
      a fetchRows request will complete.

      \param db2ExtTableInterface [in] - External Table Interface
      \param pBuffer [in/out] - Buffer to release.  NULL'ed out on exit (success or fail).
      \param bytesWritten [in] - Number of bytes written into the buffer.

      \retval DB2_EXT_TABLE_RC_OK
      \retval DB2_EXT_TABLE_RC_ERROR_OCCURRED
      \retval DB2_EXT_TABLE_RC_INTERNAL_ERROR
    */
   db2ExtTableRC (SQL_API_FN *releaseWriteBuffer)(db2ExtTableInterface*, db2Uint8*& pBuffer, const db2Uint64 bytesWritten);

   /**
      \brief Invoked by external table reader to get access to a buffer that
      contains row data.

      This function is thread safe.  It may be invoked by any external table
      library thread.  The external table context must be running a insertRows
      request at the time this function is invoked.

      A single thread is only allowed to have one outstanding readable buffer.

      \param db2ExtTableInterface [in] - External Table Interface
      \param ppBuffer [out] - Pointer to buffer of size \c pBufferSize
      \param pBufferSize [out] - Size of buffer

      \retval DB2_EXT_TABLE_RC_OK
      \retval DB2_EXT_TABLE_RC_ERROR_OCCURRED
      \retval DB2_EXT_TABLE_RC_INTERRUPTED
      \retval DB2_EXT_TABLE_RC_INTERNAL_ERROR
      \retval DB2_EXT_TABLE_RC_NO_MORE_ROWS
    */
   db2ExtTableRC (SQL_API_FN *getReadBuffer)(db2ExtTableInterface*, db2Uint8** ppBuffer, db2Uint64* pBufferSize);

   /**
      \brief Invoked by external table library to release buffer back to the
      execution environment.  Callers access to buffer is lost after this call.

      The external table library must release all communication buffers before
      an insertRows request will complete.

      \param db2ExtTableInterface [in] - External Table Interface
      \param pBuffer [in/out] - Buffer to release.  NULL'ed out on exit (success or fail).
      \param bytesWritten [in] - Number of bytes written into the buffer.

      \retval DB2_EXT_TABLE_RC_OK
      \retval DB2_EXT_TABLE_RC_ERROR_OCCURRED
      \retval DB2_EXT_TABLE_RC_INTERNAL_ERROR
    */
   db2ExtTableRC (SQL_API_FN *releaseReadBuffer)(db2ExtTableInterface*, db2Uint8*& pBuffer, const db2Uint64 bytesWritten);

   /**
      \brief Raise an SQL error that was encountered during an external table library request.

      \param db2ExtTableInterface [in] - External Table Interface
      \param sqlCode [in] - SQL Code
      \param numMsgTokens [in] - Number of message tokens
      \param db2ExtTableMessageTokens [in] - Array of message tokens.

      \retval DB2_EXT_TABLE_RC_ERROR_OCCURRED
      \retval DB2_EXT_TABLE_RC_INTERRUPTED
    */
   db2ExtTableRC (SQL_API_FN *raiseError)(db2ExtTableInterface*, const SQL_API_RC sqlCode, const db2Uint16 numMsgTokens, db2ExtTableMessageTokens*);

   /**
      \brief Invoked by the external table reader at the completion of a request.
      The result (success or failure) of the request is a required argument.
      The external table reader should take no action that may result in the
      failure of the request after this function is invoked.

      \param db2ExtTableInterface [in] - External Table Interface
      \param db2ExtTableRC [in] - Request result

      \retval DB2_EXT_TABLE_RC_OK
      \retval DB2_EXT_TABLE_RC_INTERNAL_ERROR
    */
   db2ExtTableRC (SQL_API_FN *completeRequest)(db2ExtTableInterface*, const db2ExtTableRC);

   /**
      \brief Invoked by the external table reader to load a Java class or C library
      Implementation for language C is not yet available but is here for future proofing 

      \param db2ExtTableInterface [in] - External Table Interface
      \param parms [in] - Routine library parameters
      \param routineLibrary [out] - Struct containing library handle and language info

      \retval DB2_EXT_TABLE_RC_OK
      \retval DB2_EXT_TABLE_RC_ERROR_OCCURRED
   */

   db2ExtTableRC (SQL_API_FN *loadRoutineLibrary)(db2ExtTableInterface*, db2ExtTableRoutineLibaryParms* parms, db2RoutineLibrary* routineLibrary);

   /**
      \brief Log a Java exception to the DB2 diagnostic log and adminstrative log.

      \param db2ExtTableInterface [in] - External Table Interface
      \param javaException [in] - Java Exception (jthrowable) to log
    */
   void (SQL_API_FN *logJavaException)(db2ExtTableInterface*, void*);
};

/**
   \brief
      Index descriptor.  Identifies the index (by indexSchema.indexName) that
      DB2 recommends for use in a table access.
 */
struct db2ExtTableIndexDescriptor
{
   //! Schema of the index.  Null terminated.
   char                            indexSchema[SQL_MAX_IDENT + 1];

   //! Index name.  Null terminated.
   char                            indexName[SQL_MAX_IDENT + 1];

   //! Estimated cardinality for the index
   double                          estCardinality;
};

/**
   \brief
      Information on the scheduler used by this table access.
 */
struct db2ExtTableSchedulerInfo
{
   //! Host name for the scheduler
   char                            schedulerHostName[SQL_HOSTNAME_SZ + 1];

   //! Port number for the scheduler
   db2Uint32                       schedulerPort;
};

/**
   \brief
      Table access descriptor.  Identifies the table (by tableSchema.tableName)
      and the columns which will be accessed.
 */
struct db2ExtTableAccessDescriptor
{
   //! Schema of the table.  Null terminated.
   char                            tableSchema[SQL_MAX_IDENT + 1];

   //! Table name.  Null terminated.
   char                            tableName[SQL_MAX_IDENT + 1];

   //! Column Reference List
   sqlExternalColRefs*             extColRefs;

   //! Predicate to apply when accessing the table
   sqlPredExternal*                extPred;

   //! Number of boolean factors in the predicate status array.
   db2int32                        numBooleanFactors;

   //! Predicate status array
   char*                           extPredStatus;

   //! Fetch first N rows indicator. (0 means fetch all rows)
   db2Uint32                       ffnr;

   //! Table flags. See DB2_EXTTABLE_* below.
   db2Uint32                       extTableFlags;

   //! Descriptor for the index recommended by DB2. (NULL if no index
   //! recommended)
   db2ExtTableIndexDescriptor*     indexDesc;

   //! Scheduler information for the table access
   db2ExtTableSchedulerInfo*       schedulerInfo;

   //! External table options
   void*                           pExtTableOptions;

   //! User ID for impersonation
   char                            impersonationID[SQL_MAX_IDENT + 1];

   //! Percent of rows that should be table sampled (0.0 - 100.0)
   //! Only meaningful if DB2_EXTTABLE_TABLESAMPLE_SYSTEM or
   //! DB2_EXTTABLE_TABLESAMPLE_BERNOULLI is set in
   //! extTableFlags.
   double                          samplingPercent;

   //! Repeatable seed for table sampling. Only meaningful if
   //! DB2_EXTTABLE_TABLESAMPLE_REPEATABLE is set in
   //! extTableFlags.
   db2Uint32                       samplingSeed;

   //! External Table Columns
   sqlExternalColRefs*             extTblShape;
   
   //! Column Names List
   sqlExternalColNames*            extColNames;

   //! Column Layout List
   sqlExternalColLayouts*          extColLayouts;
};

/**
   \brief
      Flags controlling table processing
 */

//! Data for inserting has been sorted on the partitioning columns
#define DB2_EXTTABLE_PART_COLS_SORTED       0x00000001

//! The partitioning columns for the insert have been bound to a single value
#define DB2_EXTTABLE_PART_COLS_BOUND        0x00000002

//! Partition elimination needs to be done at the reader instead of the scheduler
#define DB2_EXTTABLE_PARTELIM_AT_READER     0x00000004

//! Use SYSTEM table sampling for this scan
#define DB2_EXTTABLE_TABLESAMPLE_SYSTEM     0x00000008

//! Use BERNOULLI table sampling for this scan
#define DB2_EXTTABLE_TABLESAMPLE_BERNOULLI  0x00000010

//! A seed has been specified for repeatable table sampling (will be paired
//! with either DB2_EXTTABLE_TABLESAMPLE_SYSTEM or
//! DB2_EXTTABLE_TABLESAMPLE_BERNOULLI.
#define DB2_EXTTABLE_TABLESAMPLE_REPEATABLE 0x00000020

/**
   \brief
      External table monitoring stats for FMP worker thread and infrastructure.
 */
struct db2ExtTableFMPStats
{
   //! Total time the FMP worker thread spent on read and write requests.
   db2Uint64 totalTime;

   //! Total time the FMP worker thread spent processing read and write
   //! requests. It is the difference between fmp.totalTime and fmp.waitTime
   db2Uint64 processingTime;

   //! Total time the FMP worker thread spent initializing scans or inserts.
   db2Uint64 initTime;

   //! Total time the FMP worker thread spent closing scans or inserts.
   db2Uint64 closeTime;

   //! Total time the FMP worker thread spent waiting for the DFS reader to
   //! initialize a table scan.
   db2Uint64 openTableScanTime;

   //! Total time the FMP worker thread spent waiting for the DFS reader to
   //! close a table scan.
   db2Uint64 closeTableScanTime;

   //! Spare to keep the size constant, can be re-used for new metrics
   db2Uint64 spare1;

   //! Total time the FMP worker thread was waiting. This includes for
   //! instance waiting for data buffers to be filled by the scanner threads.
   db2Uint64 waitTime;

   //! Total time spent by the DFS reader threads waiting for an empty data
   //! buffer to become available.
   db2Uint64 getBufferTime;

   //! High water mark for fmp.getBufferTime
   db2Uint64 getBufferTimeHwm;

   //! Low water mark for fmp.getBufferTime
   db2Uint64 getBufferTimeLwm;

   //! Total time spent inside the DFS reader threads when releasing full
   //! buffers to the agent for further processing.
   db2Uint64 releaseBufferTime;

   //! High water mark for fmp.releaseBufferTime
   db2Uint64 releaseBufferTimeHwm;

   //! Low water mark for fmp.releaseBufferTime
   db2Uint64 releaseBufferTimeLwm;

   //! Subtotal of number of rows received by the scanner threads from the disk IO
   //! threads and transferred to the agent.
   db2Uint64 scannerRowsReceivedSubtotal;

   //! Subtotal of volume of data in bytes received by the scanner threads from the
   //! disk IO threads and transferred to the agent. 
   db2Uint64 scannerVolumeReceivedSubtotal;

   //! Spare to keep the size constant, can be re-used for new metrics
   db2Uint64 spare4;

   //! Spare to keep the size constant, can be re-used for new metrics
   db2Uint64 spare5;

   //! Spare to keep the size constant, can be re-used for new metrics
   db2Uint64 spare6;
};

/**
   \brief
      External table monitoring stats for the scanner threads.
 */
struct db2ExtTableScannerStats
{
   //! Total time spent inside the scanner threads on DFS table scan requests.
   //! This is the sum of  scanner.processingTime and scanner.waitTime.
   db2Uint64 totalTime;

   //! Total time spent by the scanner threads on processing DFS table scan
   //! requests. This includes deserialization of the data from the disk IO
   //! threads, projection and selection operations, as well as serializing
   //! into the data buffers.
   db2Uint64 processingTime;

   //! Spare to keep the size constant, can be re-used for new metrics
   db2Uint64 spare1;

   //! Total time the scanner threads were waiting for a response on a
   //! scheduler request.
   db2Uint64 schedulerWaitTime; 

   //! Total time the scanner threads were waiting for buffers to be filled
   //! by disk IO.
   db2Uint64 diskIOWaitTime;

   //! Total number of requests sent by the scanner threads to the
   //! scheduler for new splitts/blocks of HDFS data.
   db2Uint64 numSchedulerRequests;

   //! Total number of buffers received by the scanner threads from the disk
   //! IO threads.
   db2Uint64 numReceives;

   //! Volume of data in bytes received within the last receive(s) by the scanner threads 
   //! from the disk IO threads. It is the raw number of bytes read from disk.
   db2Uint64 volumeReceived;

   //! Total volume of data in bytes selected through projection and selection
   //! operations by the scanner threads.
   db2Uint64 volumeSelected;

   //! Total number of rows received within the last receive(s) 
   //! by the scanner threads from the disk IO threads.
   db2Uint64 rowsReceived;

   //! Total number of rows selected by the scanner threads. It should be equal
   //! to the number of rows received by the DB2 agents.
   db2Uint64 rowsSelected;
};


/**
   \brief
      External table monitoring stats for the writer threads.
 */
struct db2ExtTableWriterStats
{
   //! Total time spent inside the writer threads on DFS table insert requests.
   //! This is the sum of writer.processingTime and writer.diskIOWwaitTime.
   db2Uint64 totalTime;

   //! Total time spent by the writer threads on processing DFS table insert 
   //! requests. 
   db2Uint64 processingTime;

   //! Total time the writer threads were waiting for buffers to be read
   //! by disk IO.
   db2Uint64 diskIOWaitTime;

   //! Total number of buffers sent by the writer threads to the disk
   //! IO threads.
   db2Uint64 numSends;

   //! Volume of data in bytes sent by the writer threads to the
   //! disk IO threads. It is the raw number of bytes written to disk.
   db2Uint64 volumeSent;

   //! Subtotal of volume of data in bytes sent by the writer threads to the
   //! disk IO threads. It is the raw number of bytes written to disk.
   db2Uint64 volumeSentSubtotal;

   //! Number of rows sent by the writer threads to the disk IO threads.
   db2Uint64 rowsSent;

   //! Subtotal of number of rows sent by the writer threads to the disk IO
   //! threads.
   db2Uint64 rowsSentSubtotal;

};

/**
   \brief
      External table monitoring stats for the disk IO threads.
 */
struct db2ExtTableDiskIOStats
{
   //! Total time the disk-IO threads spent reading HDFS-data.
   db2Uint64 totalTime;

   //! Total time the disk-IO threads spent writing HDFS-data.
   db2Uint64 totalWriteTime;

   //! Total volume of data written by the disk-IO threads
   db2Uint64 volumeWritten;

   //! Spare to keep the size constant, can be re-used for new metrics
   db2Uint64 spare3;

   //! Spare to keep the size constant, can be re-used for new metrics
   db2Uint64 spare4;

   //! Total volume of data in bytes read by the disk-IO threads from local
   //! disks.
   db2Uint64 volumeLocalRead;

   //! Spare to keep the size constant, can be re-used for new metrics
   db2Uint64 spare5;
};

/**
   \brief
      External table monitoring statistics.
 */
struct db2ExtTableMonStats
{
   //! Structure to hold all the FMP worker thread and infrastructure-level
   //! scan-specific statistics.
   db2ExtTableFMPStats            fmp;

   //! Structure to hold all the scanner thread specific statistics.
   db2ExtTableScannerStats        scanner;

   //! Structure to hold all the disk IO thread specific statistics.
   db2ExtTableDiskIOStats         diskIO;

   //! Structure to hold all the writer thread specific statistics.
   db2ExtTableWriterStats         writer;

};

/**
   \brief
      External table interface between the execution environment and the
      external table reader.
 */
struct db2ExtTableInterface
{
   //! External Table Interface version
   db2ExtTableVersion             extTableIntfcVersion;

   //! External table scan id
   SQL_BIG_SQL_SCHEDULER_SCAN_ID  extTableScanId;

   //! Source (execution environment) control block
   void*                          pExtTableSourceCB;

   //! Target (external table reader) control block
   void*                          pExtTableTargetCB;

   //! Set of function pointers provided by external table reader
   db2ExtTableFuncPtrs*           extTableFn;

   //! Set of callback function pointers provided by execution environment
   db2ExtTableCallbacks*          callbackFn;

   //! Output table access descriptor.  Uses:
   //!  extTableFn.openTableScan - Describes table + column projection list to
   //!     to be used in the table scan.
   db2ExtTableAccessDescriptor*   pOutputTableAccessDesc;

   //! Input table access descriptor.  Uses:
   //!  extTableFn.insertRows - Describes table + column list to be used during
   //!  insert.
   db2ExtTableAccessDescriptor*   pInputTableAccessDesc;

   //! Output statistics from the external table scan, to be passed back
   //!  to the execution environment.
   db2ExtTableMonStats*           pExtTableStats;

   //! Node id
   SQL_PDB_NODE_TYPE currentDb2Node;

   //! Max number of comm buffers allowed
   db2Uint64 numMaxCommBuffers;
};

/**
   \brief
      External table process initialization and termination interface between
      the execution environment and the External Table Reader.
 */
struct db2ExtTableInit
{
   //! External Table Interface version
   db2ExtTableVersion             extTableIntfcVersion;

   //! Source (execution environment) control block
   void*                          pExtTableSourceCB;

   //! Target (external table reader) control block
   void*                          pExtTableTargetCB;

   //! Set of callback function pointers provided by execution environment
   db2ExtTableInitCallbacks*      callbackFn;

   //! Number of configuration parameters
   db2Uint64                      numCfgParm;

   //! Array of configuration parameters
   db2ExtTableCfgParm*            cfgParm;
};

#ifdef __cplusplus
}
#endif

#endif

