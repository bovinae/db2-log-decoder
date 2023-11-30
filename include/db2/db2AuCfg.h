/**********************************************************************
**
**  Source File Name = db2AuCfg
**
**  (C) COPYRIGHT International Business Machines Corp. 1987, 1999, 2002
**  All Rights Reserved
**  Licensed Materials - Property of IBM
**
**  US Government Users Restricted Rights - Use, duplication or
**  disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
**
**  Function = Include File definitions for the APIs, db2AutoConfig and 
**             db2AutoConfigFreeMemory, and their corresponding data 
**             structures.
**
***********************************************************************/
#ifndef SQL_H_DB2AUCFG
#define SQL_H_DB2AUCFG

#include "sqlenv.h"
#include "db2ApiDf.h"

/***********************************************************************
** Type definitions used for input and output for the db2AutoConfig API
************************************************************************/
/***********************************************************************
** db2AutoConfigElement data structure
** db2AutoConfigElement data structure parameters
**
** token 
** Input or output. Specifies the configuration value for both the input
** parameters and the output diagnostics. 
**
** value 
** Input or output. Holds the data specified by the token. 
************************************************************************/
typedef struct  db2AutoConfigElement  {
  db2int32  token;
  db2int32  value;
} db2AutoConfigElement;

/***********************************************************************
** db2AutoConfigArray data structure
** db2AutoConfigArray data structure parameters
** 
** numElements 
** Input or output. The number of array elements. 
** 
** pElements 
** Input or output. A pointer to the element array. 
** 
************************************************************************/
typedef struct  db2AutoConfigArray  {
  db2Uint32  numElements;            /* number of array elements */
  db2AutoConfigElement *  pElements; /* the element array */
} db2AutoConfigArray;

typedef db2AutoConfigArray  db2AutoConfigInput;
typedef db2AutoConfigArray  db2AutoConfigDiags;

/***********************************************************************
** db2AutoConfigNameElement data structure
** db2AutoConfigNameElement data structure parameters
**
** pName
** Output. The name of the output buffer pool. 
**
** value
** Input or output. Holds the size (in pages) of the buffer pool
** specified in the name. 
************************************************************************/
typedef struct  db2AutoConfigNameElement  {
  char * pName;
  db2int32 value;
} db2AutoConfigNameElement;

/***********************************************************************
** db2AutoConfigBpElement data structure
** db2AutoConfigBpElement data structure parameters
**
** pName
** Output. The name of the output buffer pool. 
**
** value
** Output. Holds the size (in pages) of the buffer pool
** specified in the name. 
**
** automatic
** Output.  A non-zero values indicates the buffer pool specified in the
** name is automatic.
**
** reserved
** Output. Reserved for future used.
************************************************************************/
typedef struct 
{
   char * pName;
   db2int32 value;
   db2int32 automatic;
   char reserved[8];
} db2AutoConfigBpElement;

/***********************************************************************
** db2AutoConfigNameArray data structure
** db2AutoConfigNameArray data structure parameters
** 
** numElements
** Input or output. The number of array elements. 
** 
** pElements
** Input or output. A pointer to the element array.
************************************************************************/
typedef struct  db2AutoConfigNameArray  {
  db2Uint32 numElements;                /* number of array elements */
  db2AutoConfigNameElement * pElements; /* the element array */
} db2AutoConfigNameArray;


/***********************************************************************
** db2AutoConfigBpValues data structure
** db2AutoConfigBpValues data structure parameters
** 
** numElements
** Input or output. The number of array elements. 
** 
** pElements
** Input or output. A pointer to the element array.
************************************************************************/
typedef struct 
{
   db2Uint32 numElements;
   db2AutoConfigBpElement * pElements;
} db2AutoConfigBpValues;

typedef db2AutoConfigNameArray db2BpValues;

/***********************************************************************
** db2AutoConfigMemberArray data structure
** db2AutoConfigMemberArray data structure parameters
** 
** numElements
** Input or output. The number of array elements. 
** 
** pElements
** Input or output. A pointer to the element array.
************************************************************************/
typedef struct {
  db2Uint32 numElements;
  SQL_PDB_NODE_TYPE *pMemberNumber;
} db2AutoConfigMemberArray;

typedef db2AutoConfigMemberArray db2AutoConfigMembers;

/***********************************************************************
** db2ConfigValues data structure
** db2ConfigValues data structure parameters
**
** numElements
** Input or output. The number of array elements. 
**
** pConfigs
** Input or output. A pointer to an array of db2CfgParam structure. 
**
** pDataArea
** Input or output. A pointer to the data area containing the values of
** the configuration.
***********************************************************************/
typedef struct  db2ConfigValues  {
  db2Uint32  numElements;
  struct db2CfgParam * pConfigs;
  void * pDataArea;
} db2ConfigValues;

// WA and WAS
typedef struct db2WAandWASNameElements {
  db2Uint32 numElements;
  char * pName;
  char * pEnable;
} db2WAandWASNameElements;

typedef db2WAandWASNameElements db2WAValues;
typedef db2WAandWASNameElements db2WASValues;

// WCS
typedef struct db2WCSNameElements {
  db2Uint32 numElements;
  char * pName;
  double value1;                        // timercost
  double value2;                        // 
} db2WCSNameElements;

typedef db2WCSNameElements db2WCSValues;

// ThreshConcur
typedef struct db2ThreshConcurNameElement  {
  db2Uint32 numElements;
  db2Uint32 maxValue;
  char * pName;
  char * pEnable;
} db2ThreshConcurNameElement;

typedef db2ThreshConcurNameElement db2ThreshConcurValues;

// CfgCpuLimit
typedef struct db2CpuLimitNameElement  {
  db2Uint32 numElements;
  db2int32 cpuLimit;
} db2CpuLimitNameElement;

typedef db2CpuLimitNameElement db2CpuLimitValues;

/***********************************************************************
** db2AutoConfigMemberValuesArray data structure
** db2AutoConfigMemberValuesArray data structure parameters
**
** numElements
** Input or output.  The number of array elements in each array below
** where each element belongs to a single member.
**
** oOldDbValues
** Output. If the iApply value is set to update the database 
** configuration or all configurations, each elements represents the
** database configuration value prior to using the advisor on a member.
** Otherwise, this is the each elements represents the current value on 
** a member.
** 
** oNewDbValues
** Output. If the iApply value is set to update the database configuration 
** or all configurations, each element represents the current database 
** configuration value on a member . Otherwise, each element represents 
** the recommended value for the advisor on a member.
**
** oOldBpValues
** Output. If the iApply value is set to update the database configuration 
** or all configurations, each elements represents the buffer pool sizes in 
** pages before using the advisor on a member.  Otherwise, each element is 
** the current value on a member. 
**
** oNewBpValues
** Output. If the iApply value is set to update the database configuration
** or all configurations, each elements represents the current buffer pool 
** sizes in page on a members.  Otherwise, each element is the recommended 
** value for the advisor on a member. 
**
** oDiagnostics
** Output.  Includes diagnostics from the advisor where each element
** represents diagnostics from a single member.
**
** oMemberNumbers
** Output. Member numbers for the elements contained within the arrays
** above.
** 
** oMemberStates
** Output. A zero value indicates that the member is offline.
**         Members can only be offline in PureScale.
***********************************************************************/
typedef struct {
  db2Uint32  numElements;
  db2ConfigValues *oOldDbValues;
  db2ConfigValues *oNewDbValues;
  db2AutoConfigBpValues*  oOldBpValues;
  db2AutoConfigBpValues*  oNewBpValues;
  db2AutoConfigDiags *oDiagnostics;
  SQL_PDB_NODE_TYPE *oMemberNumbers;
  db2Uint16*    oMemberStates; 
} db2AutoConfigMemberValuesArray;

/***********************************************************************
** db2AutoConfigOutput data structure
** db2AutoConfigOutput data structure parameters
**
** oOldDbValues
** Output. If the iApply value is set to update the database 
** configuration or all configurations, this value represents the
** database configuration value prior to using the advisor on the 
** coordinating member. Otherwise, this is the current value on 
** the coordinating member.
**
** oOldDbmValues
** Output. If the iApply value is set to update all configurations, this
** value represents the database manager configuration value prior to
** using the advisor. Otherwise, this is the current value.
**
** oNewDbValues
** Output. If the iApply value is set to update the database configuration 
** or all configurations, this value represents the current database 
** configuration value on the coordinating member. Otherwise, this 
** is the recommended value for the advisor on the coordinating member.
**
** oNewDbmValues
** Output. If the iApply value is set to update all configurations,
** this value represents the current database manager configuration
** value. Otherwise, this is the recommended value for the advisor. 
**
** oDiagnostics
** Output. Includes diagnostics from the advisor. 
**
** oOldBpValues
** Output. If the iApply value is set to update the database configuration 
** or all configurations, this value represents the buffer pool sizes in 
** pages before using the advisor on the coordinating member.  Otherwise,
**  this value is the current value on the coordinating member. 
**
** oNewBpValues
** Output. If the iApply value is set to update the database configuration 
** or all configurations, this value represents the current buffer pool sizes
**  in pages on the coordinating member.  Otherwise, this is the recommended
**  value for the advisor on the coordinating member. 
**
** oMemberValues
** Output.  A pointer to an array of db2AutoConfigDbValues containing the old 
** and new database configuration parameter values and diagnostics 
** from the advisor. Where each elements represents this information from
** a single member.
**
** A note about down level compatibility:
** When using this structure with an API version of v10.7 and lower the 
** fields under oMemberValues will not be populated. The oOldDbValues,
** oNewDbValues, oOldBpValues, oNewBpValues and oDiagnostics will be
** populated instead. On later versions these fields will always be populated
** with data from the coordinator and oMemberValues will contains values
** from all members. 
** The field oMemberValues has been placed at the end to retain binary 
** compatibility with older releases.
***********************************************************************/
typedef struct  db2AutoConfigOutput  { 
  db2ConfigValues  oOldDbValues;    /* current/old database configurations */
  db2ConfigValues  oOldDbmValues;    /* current/old database manager configurations */
  db2ConfigValues  oNewDbValues;    /* current/new database configurations */
  db2ConfigValues  oNewDbmValues;   /* suggested/new database manager configurations */
  db2AutoConfigDiags  oDiagnostics; /* diagnostic details */
  db2BpValues  oOldBpValues;
  db2BpValues  oNewBpValues;
  // New outputs for Kepler
  db2WAValues oOldWAValues; // WorkAction
  db2WAValues oNewWAValues; // WorkAction
  db2WASValues oOldWASValues; // WorkActionSet
  db2WASValues oNewWASValues; // WorkActionSet
  db2WCSValues oOldWCSValues; // WorkClassSet
  db2WCSValues oNewWCSValues; // WorkClassSet
  db2ThreshConcurValues oOldThreshConcur; // ThresholdConcur
  db2ThreshConcurValues oNewThreshConcur; // ThresholdConcur
  // New outputs for 10.6
  db2CpuLimitValues oOldCpuLimit; // CfgCpuLimit
  db2CpuLimitValues oNewCpuLimit; // CfgCpuLimit
  db2AutoConfigMemberValuesArray oMemberValues; 
} db2AutoConfigOutput;


#define DB2_SG_PROD_VERSION_SIZE  16


/***********************************************************************
** db2AutoConfigInterface data structure
** db2AutoConfigInterface data structure parameters
** 
** iProductID 
** Input. Specifies a unique product identifier. Valid values for the
** iProductID parameter (defined in db2AuCfg.h, located in the include
** directory) are:
** - DB2_SG_PID_DEFAULT                  
** - DB2_SG_PID_WEBSPHERE_COMMERCE_SUITE 
** - DB2_SG_PID_SAP                      
** - DB2_SG_PID_WEBSPHERE_ADVANCED_SERVER
** - DB2_SG_PID_SIEBEL                   
** - DB2_SG_PID_PS_EPM                   
** - DB2_SG_PID_PS_ONLINE                
** - DB2_SG_PID_PS_BATCH                 
** - DB2_SG_PID_PS                       
** - DB2_SG_PID_LOTUS_DOMINO             
** - DB2_SG_PID_CONTENT_MANAGER          
** - DB2_SG_PID_TSM
** 
** iProductVersion 
** Input. A 16 byte string specifying the product version. 
** 
** iDbAlias 
** Input. A string specifying a database alias. 
** 
** iApply 
** Input. Updates the configuration automatically. Valid values for the
** iApply parameter (defined in db2AuCfg.h, located in the include
** directory) are:     
** - DB2_SG_NOT_APPLY 
** Do not apply any recommendations
** - DB2_SG_APPLY 
** Apply all recommendations    
** - DB2_SG_APPLY_DB  
** Apply only database (and bufferpool) recommendations
** - DB2_SG_APPLY_SUMMARY
** Don't apply, just show existing summary report       
** 
** 
** 
** iParams 
** Input. Passes parameters into the advisor. 
** 
** oResult 
** Output. Includes all results from the advisor. 
***********************************************************************/
typedef struct  db2AutoConfigInterface  {
  db2int32  iProductID;                             /* product id */
  char iProductVersion[DB2_SG_PROD_VERSION_SIZE+1]; /* product version */
  char iDbAlias[SQL_ALIAS_SZ+1];                    /* database alias */
  db2int32  iApply;                /* update the configuration automatically */
  db2AutoConfigInput  iParams;     /* input parameters */
  db2AutoConfigOutput  oResult;    /* results */
  db2AutoConfigMembers iMembers;   /* Array of members to provide detailed  */
                                   /* report on.                            */
} db2AutoConfigInterface;

/***********************************************************************
** Definitions used for input to the db2AutoConfig API
***********************************************************************/
/* Product ID for the product using the API (values for iProductID) */
#define DB2_SG_PID_DEFAULT                   0
#define DB2_SG_PID_WEBSPHERE_COMMERCE_SUITE  1
#define DB2_SG_PID_SAP                       2
#define DB2_SG_PID_WEBSPHERE_ADVANCED_SERVER 3 
#define DB2_SG_PID_SIEBEL                    4 
#define DB2_SG_PID_PS_EPM                    5
#define DB2_SG_PID_PS_ONLINE                 6
#define DB2_SG_PID_PS_BATCH                  7
#define DB2_SG_PID_PS                        8
#define DB2_SG_PID_LOTUS_DOMINO              9
#define DB2_SG_PID_CONTENT_MANAGER           10
#define DB2_SG_PID_TSM                       11

/* Input values for iProductVersion */
#define DB2_SG_PRODUCT_VERSION_1_1  "1.1"

/* Indicate whether to apply the configuration recommendations or not.       */
/* DB2_SG_APPLY_ON_ONE_NODE is meaningful only with DB2_SG_APPLY or          */
/* with DB2_SG_APPLY_DB.                                                     */
#define DB2_SG_NOT_APPLY         0 /* do not apply any recommendations */
#define DB2_SG_APPLY             1 /* apply all recommendations */
#define DB2_SG_APPLY_DB          2 /* apply only database (and bufferpool) */
                                   /* recommendations */
#define DB2_SG_APPLY_DBM         4 /* apply only dbm recommendations */
#define DB2_SG_APPLY_ON_ONE_NODE 8 /* apply database recommendations on */
                                   /* the connection node only */
#define DB2_SG_APPLY_SUMMARY    16 /* No recommendations, just gives    */
                                   /* existing summary report           */

/* Tokens for input parameters (iParams) */

#define DB2_SG_MEMORY_PERCENTAGE   1  /* The percentage of the memory the */
                                      /* database manager tries to use for all */
                                      /* logical partitions, but only for the */
                                      /* current database. Adjust accordingly */
                                      /* on an instance with multiple databases */
#define DB2_SG_WORKLOAD            2  /* The type of workload that best */
                                      /* reflects the database. */
#define DB2_SG_NUM_STATEMENT       3  /* The average number of SQL statements */
                                      /* in a single unit of work. */
#define DB2_SG_TRANS_PER_MINUTE    4  /* The number of transactions per */
                                      /* minute. */
#define DB2_SG_ADMIN_PRIORITY      5  /* The database administration */
                                      /* priority. */
#define DB2_SG_IS_POPULATED        6  /* The database is populated with */
                                      /* data. */
#define DB2_SG_LOCAL_APPLICATION   7  /* Average number of connected local */
                                      /* applications. */
#define DB2_SG_REMOTE_APPLICATION  8  /* Average number of connected remote */
                                      /* applications. */
#define DB2_SG_ISOLATION_LEVEL     9  /* Isolation level that best reflects */
                                      /* your applications. */
#define DB2_SG_BP_RESIZEABLE       10 /* Should db2AutoConfig change */
                                      /* bufferpool sizes. */
#define DB2_SG_TOLERANCE_PERCENTAGE 11 /* Tolerance Percentage for summary */
                                       /* report. */
#define DB2_SG_ANALYTICS_ENV       12 /* Make recommendations assuming    */
                                      /* analytics is on or off. */
/* Valid values for input parameters (iParams) */
#define DB2_SG_MEMORY_PERCENTAGE_MIN 1 
#define DB2_SG_MEMORY_PERCENTAGE_MAX 100 

#define DB2_SG_WORKLOAD_MIN          1
#define DB2_SG_WORKLOAD_QUERIES      1
#define DB2_SG_WORKLOAD_MIXED        2
#define DB2_SG_WORKLOAD_TRANSACTIONS 3
#define DB2_SG_WORKLOAD_MAX          3

#define DB2_SG_NUM_STATEMENT_MIN     1
#define DB2_SG_NUM_STATEMENT_MAX     1000000

#define DB2_SG_ADMIN_PRIORITY_MIN         1
#define DB2_SG_ADMIN_PRIORITY_TRANSACTION 1
#define DB2_SG_ADMIN_PRIORITY_BOTH        2
#define DB2_SG_ADMIN_PRIORITY_RECOVERY    3
#define DB2_SG_ADMIN_PRIORITY_MAX         3

#define DB2_SG_TRANS_PER_MINUTE_MIN 1     
#define DB2_SG_TRANS_PER_MINUTE_MAX 200000

#define DB2_SG_IS_POPULATED_MIN 1
#define DB2_SG_IS_POPULATED_YES 1
#define DB2_SG_IS_POPULATED_NO  2
#define DB2_SG_IS_POPULATED_MAX 2

#define DB2_SG_LOCAL_APPLICATION_MIN 0  
#define DB2_SG_LOCAL_APPLICATION_MAX 5000

#define DB2_SG_REMOTE_APPLICATION_MIN 0 
#define DB2_SG_REMOTE_APPLICATION_MAX 5000 

#define DB2_SG_ISOLATION_LEVEL_MIN              1
#define DB2_SG_ISOLATION_LEVEL_REPEAT_READ      1
#define DB2_SG_ISOLATION_LEVEL_READ_STABILITY   2
#define DB2_SG_ISOLATION_LEVEL_CURSOR_STABILITY 3
#define DB2_SG_ISOLATION_LEVEL_UNCOMMITTED_READ 4
#define DB2_SG_ISOLATION_LEVEL_MAX              4

#define DB2_SG_BP_RESIZEABLE_MIN 0
#define DB2_SG_BP_RESIZEABLE_NO  0 
#define DB2_SG_BP_RESIZEABLE_YES 1
#define DB2_SG_BP_RESIZEABLE_MAX 1

#define DB2_SG_CALLER_ID_MIN       0 
#define DB2_SG_CALLER_ID_AUTO      0
#define DB2_SG_CALLER_ID_EXPLICIT  1
#define DB2_SG_CALLER_ID_MAX       1

#define DB2_SG_REPORT_MEMBER_CURRENT -1
#define DB2_SG_REPORT_MEMBER_ALL     -2

#define DB2_SG_TOLERANCE_PERCENTAGE_MIN 0 
#define DB2_SG_TOLERANCE_PERCENTAGE_MAX 100 
#define DB2_SG_TOLERANCE_PERCENTAGE_DEF 5 

#define DB2_SG_ANALYTICS_ENV_MIN       0 
#define DB2_SG_ANALYTICS_ENV_AUTOMATIC 0 
#define DB2_SG_ANALYTICS_ENV_YES       1 
#define DB2_SG_ANALYTICS_ENV_NO        2 
#define DB2_SG_ANALYTICS_ENV_MAX       2 



/***********************************************************************
** Definitions used in the output diagnostics
** These token/values pairs may appear in the structure 
** db2AutoConfigOutput.db2AutoConfigDiags.  Some of the tokens return a 
** meaningful value, and some are just flags indicating a state (for which 
** the value will usually be DB2_SG_DIAG_NO_VALUE).
***********************************************************************/
/* The "no value" value for flag-type diagnostic tokens                      */
#define DB2_SG_DIAG_NO_VALUE                    -1

/* Token indicating that the calculations are made for default products.    */
/* This token is returned when DB2_SG_PID_DEFAULT is input to the API.       */
/* The corresponding value returned is DB2_SG_DIAG_NO_VALUE.                 */
#define DB2_SG_DIAG_DEFAULT_USED                 0

/* Token indicating that there is not enough memory on the machine for the   */
/* given inputs.                                                             */
/* The corresponding value returned is the recommended amount of memory.     */
#define DB2_SG_DIAG_NOT_ENOUGH_MEMORY           -1  

/* Token indicating that the are not enough disks for the given inputs.      */
/* The corresponding value returned is the recommended number of disks.      */
#define DB2_SG_DIAG_NOT_ENOUGH_DISKS            -2  

/* Token indicating that the input memory percentage is too low.             */
/* The corresponding value returned is the percentage of memory to expect    */
/* DB2 to consume for the given inputs.                                      */
#define DB2_SG_DIAG_LOW_MEMORY_PERCENTAGE       -3

/* Token indicating that database configurations are applied only to         */
/* the current partition.  (See also associated DB210211 message.)           */
/* The corresponding value returned is DB2_SG_DIAG_NO_VALUE.                 */
#define DB2_SG_DIAG_PARTITION_WARNING           10211 

/* Token indicating that there is not enough memory dedicated to the server, */
/* and no recommendations are made.                                          */
/* The corresponding value returned is the actual amount of physical         */
/* memory on the machine.                                                    */
#define DB2_SG_DIAG_DBA_MEMORY                  1100

/* Token indicating that the advisor was unable to allocate a minimum amount */
/* of memory to the bufferpools.  The corresponding value returned is        */
/* DB2_SG_DIAG_NO_VALUE.                                                     */
#define DB2_SG_DIAG_DBA_MINIMUM_BUFFER_POOL     1103

/* Token indicating that the bufferpools could not be increased due to other */
/* memory requirements determined from the given inputs.                     */
/* The corresponding value returned is DB2_SG_DIAG_NO_VALUE.                 */
#define DB2_SG_DIAG_DBA_BUFFER_POOL_SIZE        1108

/* Token indicating that the requested transaction rate is optimistic. That  */
/* is, the transaction rate is more than ten times the average number of     */
/* connected applications.                                                   */
/* The corresponding value returned is DB2_SG_DIAG_NO_VALUE.                 */
#define DB2_SG_DIAG_DBA_TRANSACTION_RATE        1109

/* Token indicating that analytics is turned on and there is more than       */
/* one database in existance so the value set for MEM_PERCENT may need to    */
/* be changed on one or more of the existing databases.                      */
/* The corresponding value returned is DB2_SG_DIAG_NO_VALUE.                 */
#define DB2_SG_DIAG_MULTIPLE_DBS                1110


/***********************************************************************
** API return codes
***********************************************************************/
#define DB2_SG_RC_OK     0
#define DB2_SG_RC_FAIL  -1

/***********************************************************************
** API definitions 
***********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************
** db2AutoConfig API
** Allows application programs to access the Configuration Advisor in 
** the Control Center. Detailed information about this advisor is 
** provided through the online help facility within the Control Center.
** 
** Authorization 
** 
** sysadm
**
** Required connection 
** 
** Database
**
** API include file 
** 
** db2AuCfg.h
**
** db2AutoConfig API parameters 
** 
** db2VersionNumber 
** Input. Specifies the version and release level of the structure passed
** in as the second parameter, pAutoConfigInterface. 
**
** pAutoConfigInterface 
** Input. A pointer to the db2AutoConfigInterface structure. 
**
** pSqlca 
** Output. A pointer to the sqlca structure. 
**
** Usage notes 
** To free the memory allocated by the db2AutoConfig API, call
** the db2AutoConfigFreeMemory API.
***********************************************************************/
SQL_API_RC SQL_API_FN
db2AutoConfig(
  db2Uint32 db2VersionNumber,    /* versions defined in db2ApiDf.h */
  void * pAutoConfigInterface,   /* pointer to db2AutoConfigInterface */
  struct sqlca * pSqlca);

/***********************************************************************
** db2AutoConfigFreeMemory API
** Frees the memory allocated by the db2AutoConfig API. 
**
** Authorization 
** 
** sysadm 
**
** Required connection 
** 
** Database 
** 
** API include file 
** 
** db2AuCfg.h 
**
** db2AutoConfigFreeMemory API parameters
**
** db2VersionNumber 
** Input. Specifies the version and release level of the structure
** passed in as the second parameter, pAutoConfigInterface. 
**
** pAutoConfigInterface 
** Input. A pointer to the db2AutoConfigInterface structure. 
** 
** pSqlca 
** Output. A pointer to the sqlca structure. 
***********************************************************************/
SQL_API_RC SQL_API_FN
db2AutoConfigFreeMemory(
  db2Uint32  db2VersionNumber,                
  void * pAutoConfigInterface,
  struct sqlca * pSqlca);
  
#ifdef __cplusplus
}
#endif



#endif  /* DB2AUCFG_H */
