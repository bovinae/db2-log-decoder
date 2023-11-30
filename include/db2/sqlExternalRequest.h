/******************************************************************************
**
** Source File Name: sqlExternalRequest.h
**
** (C) COPYRIGHT International Business Machines Corp. 2013
** All Rights Reserved
** Licensed Materials - Property of IBM
**
** US Government Users Restricted Rights - Use, duplication or
** disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
**
** Function = Include File defining:
**              Class and constants for external tables.
**
** Operating System: LINUX
**
*******************************************************************************/
#ifndef SQLEXTERNALREQUEST_H_SQL
#define SQLEXTERNALREQUEST_H_SQL

#include "sql.h"
#include <string.h>

//===========================================================================
// Constants

#define SQL_MEMBER_ID_UNDEFINED    (SQL_PDB_MAX_NUM_NODE + 1)

//===========================================================================
// Scheduler scan id structure

typedef struct SQL_APP_HDL
{ 
   unsigned short    coordAgentIndex;   // agent index of coordinator agent
   SQL_PDB_NODE_TYPE coor_node;         // coordinator node
}  SQL_APP_HDL;

typedef enum SQL_OPERATION_TYPE
{ 
   SQL_OPERATION_TYPE_SCAN = 0,
   SQL_OPERATION_TYPE_INSERT = 1,
   SQL_OPERATION_TYPE_UPDATE = 2,
   SQL_OPERATION_TYPE_DELETE = 3

   // NOTE: This enumeration needs to have the same order
   // as OperationType enum from SchedulerService.thrift
}  SQL_OPERATION_TYPE;

// Unique ID used to identify table scans in a Big SQL DB2 deployment
typedef struct SQL_BIG_SQL_SCHEDULER_SCAN_ID
{
   SQL_APP_HDL       appHandle;             // Unique per client connection across cluster
   SQL_PDB_NODE_TYPE privateScanSourceNode; // Data node sending a PRIVATE scan. Default value is 1001 (SQL_PDB_NODE_UNDEFINED).
   unsigned short    numLogicalNodes;       // Number of logical nodes on local host
   // sqluint8          padding[2];         // Will worry about padding POST milestone 2
   sqluint64         coordWorkItemCounter;  // Unique per SQL statement driven by coordinator agent (or internal request)
   sqluint32         hadoopScanID;          // Unique per table scan in a section. This is NEW to bigA (generated in code gen already...).
   sqluint32         subtaskId;             // Used to uniquely identify sub-operations within a query (e.g. a PRIVATE scan).
                                            // If there is NO private scan value is 0
   SQL_OPERATION_TYPE operationType;        // Enumeration indicating operation type
} SQL_BIG_SQL_SCHEDULER_SCAN_ID;

#endif
