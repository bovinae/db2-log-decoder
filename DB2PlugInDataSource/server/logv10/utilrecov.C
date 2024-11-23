/****************************************************************************
** (c) Copyright IBM Corp. 2007 All rights reserved.
**
** The following sample of source code ("Sample") is owned by International
** Business Machines Corporation or one of its subsidiaries ("IBM") and is
** copyrighted and licensed, not sold. You may use, copy, modify, and
** distribute the Sample in any form without payment to IBM, for the purpose of
** assisting you in the development of your applications.
**
** The Sample code is provided to you on an "AS IS" basis, without warranty of
** any kind. IBM HEREBY EXPRESSLY DISCLAIMS ALL WARRANTIES, EITHER EXPRESS OR
** IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Some jurisdictions do
** not allow for the exclusion or limitation of implied warranties, so the above
** limitations or exclusions may not apply to you. IBM shall not be liable for
** any damages you suffer as a result of using, copying, modifying or
** distributing the Sample, even if IBM has been advised of the possibility of
** such damages.
*****************************************************************************
**
** SOURCE FILE NAME: utilrecov.C
**
** SAMPLE: Utilities for the backup, restore and log file samples
**
**         This set of utilities gets the server working path, prunes
**         recovery history file, creates a database, backup a database,
**         drop a database, saves and restores log retain values, displays
**         the log buffer files.
**
** DB2 APIs USED:
**         db2CfgGet -- Get Configuration
**         db2CfgSet -- Set Configuration
**         db2Prune -- Prune Recovery History File
**         db2Backup -- Backup Database
**         sqledrpd -- Drop and uncatalog a database
**         sqlecrea -- creates a database
**
** STRUCTURES USED:
**         sqlca
**         sqledbdesc
**         db2PruneStruct
**         db2HistoryData
*****************************************************************************
**
** For more information on the sample programs, see the README file.
**
** For information on developing C++ applications, see the Application
** Development Guide.
**
** For information on DB2 APIs, see the Administrative API Reference.
**
** For the latest information on programming, building, and running DB2
** applications, visit the DB2 application development website:
**     http://www.software.ibm.com/data/db2/udb/ad
****************************************************************************/
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <sqlenv.h>
#include <sqlutil.h>
#include <sqlca.h>
#include <db2ApiDf.h>
#include <string.h>
#include <ctype.h>
#include <sqludf.h>
#include <iomanip>
#include "utilapi.h"
#include "utilemb.h"
#include "dblogconn.h"
#include "DB2PlugInDataSource.pb.h"
#include "finally_imp.h"
#include "tool_extern.h"
#include "log_imp.h"

#if ((__cplusplus >= 199711L) && !defined DB2HP && !defined DB2AIX) || \
    (DB2LINUX && (__LP64__ || (__GNUC__ >= 3)) )
#include <iostream>
using namespace std;
#else
#include <iostream.h>
#endif

#define ADD_BYTES 2
#define CHECKRC(x,y)                                   \
    if ((x) != 0)                                      \
    {                                                  \
       printf("Non-zero rc from function %s.\n", (y)); \
       return (x);                                     \
    }

#define MIN(x,y)                                       \
    ((x)<(y)?(x):(y))

#pragma GCC diagnostic ignored "-Wwrite-strings"

class UtilRecov
{
public:
	int ServerWorkingPathGet(db2Uint32 versionNumber, DbEmb*, char*);
	int DbLogArchMeth1ValueSave(db2Uint32 versionNumber, DbEmb*, char*);
	int DbLogArchMeth1ValueRestore(db2Uint32 versionNumber, DbEmb*, char*);
	int DbRecoveryHistoryFilePrune(db2Uint32 versionNumber, DbEmb*);
	int DbBackup(db2Uint32 versionNumber, DbEmb*, char*, db2BackupStruct*);
	int DbCreate(db2Uint32 versionNumber, char*, char*);
	int DbDrop(db2Uint32 versionNumber, char*);
};

class UtilLog
{
public:
	int LogBufferDisplay(char*, sqluint32, int, ReadLogWrap& pJavaWrap);
	int LogRecordDisplay(char*, sqluint32, sqluint16, sqluint16, ReadLogWrap& pJavaWrap);
	int SimpleLogRecordDisplay(sqluint16, sqluint16, char*, sqluint32, ReadLogWrap& pJavaWrap);
	int ComplexLogRecordDisplay(sqluint16, sqluint16, char*, sqluint32,
		sqluint8, char*, sqluint32, ReadLogWrap& pJavaWrap);
	int LogSubRecordDisplay(char*, sqluint16, ReadLogWrap& pJavaWrap, bool& isInternal);
	int UserDataDisplay(char*, sqluint16, sqluint16 userDataFixedLength);
};

class RID
{
private:
	char ridParts[6]{};
	char ridString[14]{};

	void toString();

public:
	int size() { return 6; };
	void set(char* buf);
	char* getString();
};

void RID::toString()
{
	char* ptrBuf = this->ridParts;

	memset(&ridString[0], 0, sizeof(ridString));
	sprintf(&ridString[0], "x%2.2X%2.2X%2.2X%2.2X%2.2X%2.2X",
		ptrBuf[0], ptrBuf[1], ptrBuf[2],
		ptrBuf[3], ptrBuf[4], ptrBuf[5]);
}

void RID::set(char* buf)
{
	memcpy(this->ridParts, buf, this->size());
}

char* RID::getString()
{
	this->toString();
	return ridString;
}

//**************************************************************************
// ServerWorkingPathGet
// Get the server working directory path where the backup images are kept
//**************************************************************************
int UtilRecov::ServerWorkingPathGet(db2Uint32 versionNumber, DbEmb* db,
	char serverWorkingPath[])
{
	struct sqlca sqlca = { 0 };
	char         serverLogPath[SQL_PATH_SZ + 1] = { 0 };
	char         dbAlias_upper[SQL_ALIAS_SZ + 1] = { 0 };
	db2CfgParam  cfgParameters[1] = { 0 };
	db2Cfg       cfgStruct = { 0 };
	int          len = 0;
	size_t          ctr = 0;
	char node[5] = "NODE";

	// initialize cfgParameters
	cfgParameters[0].flags = 0;
	cfgParameters[0].token = SQLF_DBTN_LOGPATH;
	cfgParameters[0].ptrvalue = new char[SQL_PATH_SZ + 1];

	// initialize cfgStruct
	cfgStruct.numItems = 1;
	cfgStruct.paramArray = cfgParameters;
	cfgStruct.flags = db2CfgDatabase;
	cfgStruct.dbname = db->getAlias();

	cout << "\nUSE THE DB2 API:\n" << endl;
	cout << "  db2CfgGet -- GET CONFIGURATION\n";
	cout << "TO GET THE DATABASE CONFIGURATION AND DETERMINE\n";
	cout << "THE SERVER WORKING PATH.\n" << endl;

	// get database configuration
	//db2CfgGet( db2Version1010, (void *)&cfgStruct, &sqlca );
	db2CfgGet(versionNumber, (void*)&cfgStruct, &sqlca);
	DB2_API_CHECK("server log path -- get");

	strncpy(serverLogPath, cfgParameters[0].ptrvalue, SQL_PATH_SZ);
	delete[] cfgParameters[0].ptrvalue;

	// choose server working path
	// Let's say serverLogPath = "C:\DB2\NODE0001\....".
	// Keep for serverWorkingPath "C:\DB2" only.

	for (ctr = 0; ctr < strlen(node); ctr++)
	{
		dbAlias_upper[ctr] = toupper(node[ctr]);
	}
	dbAlias_upper[ctr] = '\0';  /* terminate the constructed string */

	len = (int)(strstr(serverLogPath, dbAlias_upper) - serverLogPath - 1);
	memcpy(serverWorkingPath, serverLogPath, len);
	serverWorkingPath[len] = '\0';

	return 0;
} // UtilRecov::ServerWorkingPathGet


//**************************************************************************
// DbLogArchMeth1ValueSave
// Save LOGARCHMETH1 value for the database
//**************************************************************************
int UtilRecov::DbLogArchMeth1ValueSave(db2Uint32 versionNumber, DbEmb* db,
	char* pLogArchMeth1Value)
{
	struct sqlca sqlca = { 0 };
	db2CfgParam  cfgParameters[1] = { 0 };
	db2Cfg       cfgStruct = { 0 };

	// save log arch meth value
	cout << "\n******* Save LOGARCHMETH1 for '" << db->getAlias()
		<< "' database. *******" << endl;
	cfgParameters[0].flags = 0;
	cfgParameters[0].token = SQLF_DBTN_LOGARCHMETH1;
	cfgParameters[0].ptrvalue = pLogArchMeth1Value;

	// initialize cfgStruct
	cfgStruct.numItems = 1;
	cfgStruct.paramArray = cfgParameters;
	cfgStruct.flags = db2CfgDatabase;
	cfgStruct.dbname = db->getAlias();

	// get database configuration
	//db2CfgGet(db2Version1010, (void *)&cfgStruct, &sqlca);
	db2CfgGet(versionNumber, (void*)&cfgStruct, &sqlca);
	DB2_API_CHECK("log arch meth1  value -- save");

	return 0;
} // UtilRecov::DbLogArchMeth1ValueSave

//**************************************************************************
// DbLogArchMeth1ValueRestore
// Restore the LOGARCHMETH1 value for the database
//**************************************************************************
int UtilRecov::DbLogArchMeth1ValueRestore(db2Uint32 versionNumber, DbEmb* db,
	char* pLogArchMeth1Value)
{
	struct sqlca sqlca = { 0 };
	db2CfgParam  cfgParameters[1] = { 0 };
	db2Cfg       cfgStruct = { 0 };

	// restore the log arch meth1 value
	cout << "\n***** Restore LOGARCHMETH1 for '" << db->getAlias()
		<< "' database. *****" << endl;
	cfgParameters[0].flags = 0;
	cfgParameters[0].token = SQLF_DBTN_LOGARCHMETH1;
	cfgParameters[0].ptrvalue = (char*)pLogArchMeth1Value;

	// initialize cfgStruct
	cfgStruct.numItems = 1;
	cfgStruct.paramArray = cfgParameters;
	cfgStruct.flags = db2CfgDatabase | db2CfgDelayed;
	cfgStruct.dbname = db->getAlias();

	// set database configuration
	db2CfgSet(versionNumber, (void*)&cfgStruct, &sqlca);
	DB2_API_CHECK("log arch meth1 value -- restore");

	return 0;
} // UtilRecov::DbLogArchMeth1ValueRestore


//**************************************************************************
// DbRecoveryHistoryFilePrune
// Prunes the recovery history file by calling db2Prune API
//**************************************************************************
int UtilRecov::DbRecoveryHistoryFilePrune(db2Uint32 versionNumber, DbEmb* db)
{
	int                   rc = 0;
	struct sqlca          sqlca = { 0 };
	struct db2PruneStruct histPruneParam = { 0 };
	char                  timeStampPart[SQLU_TIME_STAMP_LEN + 1] = { 0 };

	cout << "\n***************************************\n";
	cout << "*** PRUNE THE RECOVERY HISTORY FILE ***\n";
	cout << "***************************************\n";
	cout << "\n-----------------------------------------------------------";
	cout << "\nUSE THE DB2 API:" << endl;
	cout << "  db2Prune -- PRUNE RECOVERY HISTORY FILE" << endl;
	cout << "AND THE SQL STATEMENTS:" << endl;
	cout << "  CONNECT" << endl;
	cout << "  CONNECT RESET" << endl;
	cout << "TO PRUNE THE RECOVERY HISTORY FILE." << endl;

	// connect to the database
	rc = db->Connect();
	CHECKRC(rc, "db->Connect");

	// prune the recovery history file
	cout << "\n  Prune the recovery history file for '" << db->getAlias()
		<< "' database." << endl;
	histPruneParam.piString = timeStampPart;
	strcpy(timeStampPart, "2010");        // year 2010
	histPruneParam.iAction = DB2PRUNE_ACTION_HISTORY;
	histPruneParam.iOptions = DB2PRUNE_OPTION_FORCE;

	// Prune Recovery History File
	db2Prune(versionNumber, &histPruneParam, &sqlca);
	DB2_API_CHECK("recovery history file -- prune");

	// disconnect from the database
	rc = db->Disconnect();
	CHECKRC(rc, "db->Disconnect");

	return 0;
} // UtilRecov::DbRecoveryHistoryFilePrune

//**************************************************************************
// DbBackup
// Performs the database backup
//**************************************************************************
int UtilRecov::DbBackup(db2Uint32 versionNumber, DbEmb* db,
	char            serverWorkingPath[],
	db2BackupStruct* backupStruct)

{
	struct sqlca        sqlca = { 0 };
	db2TablespaceStruct tablespaceStruct = { 0 };
	db2MediaListStruct  mediaListStruct = { 0 };

	//******************************
	//    BACK UP THE DATABASE
	//******************************
	cout << "\n  Backing up the \'" << db->getAlias() << "\' database...\n";

	tablespaceStruct.tablespaces = NULL;
	tablespaceStruct.numTablespaces = 0;

	mediaListStruct.locations = &serverWorkingPath;
	mediaListStruct.numLocations = 1;
	mediaListStruct.locationType = SQLU_LOCAL_MEDIA;

	backupStruct->piDBAlias = db->getAlias();
	backupStruct->piTablespaceList = &tablespaceStruct;
	backupStruct->piMediaList = &mediaListStruct;
	backupStruct->piUsername = db->getUser();
	backupStruct->piPassword = db->getPswd();
	backupStruct->piVendorOptions = NULL;
	backupStruct->iVendorOptionsSize = 0;
	backupStruct->iCallerAction = DB2BACKUP_BACKUP;
	backupStruct->iBufferSize = 16;        /*  16 x 4KB */
	backupStruct->iNumBuffers = 2;
	backupStruct->iParallelism = 1;
	backupStruct->iOptions = DB2BACKUP_OFFLINE | DB2BACKUP_DB;

	// The API db2Backup creates a backup copy of a database.
	// This API automatically establishes a connection to the specified
	// database. (This API can also be used to create a backup copy of a
	//  table space).
	db2Backup(versionNumber, backupStruct, &sqlca);
	DB2_API_CHECK("Database -- Backup");

	while (sqlca.sqlcode != 0)
	{
		// continue the backup operation

		// depending on the sqlca.sqlcode value, user action may be
		// required, such as mounting a new tape

		cout << "\n  Continuing the backup process..." << endl;

		backupStruct->iCallerAction = DB2BACKUP_CONTINUE;

		db2Backup(versionNumber, backupStruct, &sqlca);

		DB2_API_CHECK("Database -- Backup");
	}

	cout << "  Backup finished." << endl;
	cout << "    - backup image size     : "
		<< backupStruct->oBackupSize << " MB" << endl;
	cout << "    - backup image path     : "
		<< mediaListStruct.locations[0] << endl;
	cout << "    - backup image timestamp: "
		<< backupStruct->oTimestamp << endl;

	return 0;
} // UtilRecov::DbBackup

//**************************************************************************
// DbCreate
// Create the specified database
//**************************************************************************
int UtilRecov::DbCreate(db2Uint32 versionNumber, char existingDbAlias[],
	char newDbAlias[])
{
	struct sqlca        sqlca = { 0 };
	char                dbName[SQL_DBNAME_SZ + 1] = { 0 };
	char                dbLocalAlias[SQL_ALIAS_SZ + 1] = { 0 };
	char                dbPath[SQL_PATH_SZ + 1] = { 0 };
	struct sqledbdesc   dbDescriptor = { 0 };
	SQLEDBTERRITORYINFO territoryInfo = { 0 };
	struct db2CfgParam  cfgParameters[2] = { 0 };
	struct db2Cfg       cfgStruct = { 0 };

	cout << "\n  Create '" << newDbAlias
		<< "' empty db. with the same codeset as '" << existingDbAlias
		<< "' db." << endl;

	// initialize cfgParameters
	cfgParameters[0].flags = 0;
	cfgParameters[0].token = SQLF_DBTN_TERRITORY;
	cfgParameters[0].ptrvalue = new char[SQL_LOCALE_SIZE + 1];
	memset(cfgParameters[0].ptrvalue, '\0', SQL_LOCALE_SIZE + 1);
	cfgParameters[1].flags = 0;
	cfgParameters[1].token = SQLF_DBTN_CODESET;
	cfgParameters[1].ptrvalue = new char[SQL_CODESET_SIZE + 1];
	memset(cfgParameters[1].ptrvalue, '\0', SQL_CODESET_SIZE + 1);

	// initialize cfgStruct
	cfgStruct.numItems = 2;
	cfgStruct.paramArray = cfgParameters;
	cfgStruct.flags = db2CfgDatabase;
	cfgStruct.dbname = existingDbAlias;

	// get two database configuration parameters
	db2CfgGet(versionNumber, (void*)&cfgStruct, &sqlca);
	DB2_API_CHECK("DB Config. -- Get");

	// create a new database
	strcpy(dbName, newDbAlias);
	strcpy(dbLocalAlias, newDbAlias);
	strcpy(dbPath, "");

	strcpy(dbDescriptor.sqldbdid, SQLE_DBDESC_2);
	dbDescriptor.sqldbccp = 0;
	dbDescriptor.sqldbcss = SQL_CS_NONE;

	strcpy(dbDescriptor.sqldbcmt, "");
	dbDescriptor.sqldbsgp = 0;
	dbDescriptor.sqldbnsg = 10;
	dbDescriptor.sqltsext = -1;
	dbDescriptor.sqlcatts = NULL;
	dbDescriptor.sqlusrts = NULL;
	dbDescriptor.sqltmpts = NULL;

	strcpy(territoryInfo.sqldbcodeset, (char*)cfgParameters[1].ptrvalue);
	strcpy(territoryInfo.sqldblocale, (char*)cfgParameters[0].ptrvalue);

	// create database
	sqlecrea(dbName,
		dbLocalAlias,
		dbPath, &dbDescriptor, &territoryInfo, '\0', NULL, &sqlca);

	DB2_API_CHECK("Database -- Create");

	// release the allocated memory
	delete[] cfgParameters[0].ptrvalue;
	delete[] cfgParameters[1].ptrvalue;

	return 0;
} // UtilRecov::DbCreate

//**************************************************************************
// DbDrop
// Drops and uncatalogs the specified database alias
//**************************************************************************
int UtilRecov::DbDrop(db2Uint32 versionNumber, char dbAlias[])
{
	struct sqlca sqlca = { 0 };

	cout << "\n  Drop the '" << dbAlias << "' database." << endl;

	// drop and uncatalog the database
	sqledrpd(dbAlias, &sqlca);
	DB2_API_CHECK("Database -- Drop");

	return 0;
} // UtilRecov::DbDrop

//*************************************************************************
// LogBufferDisplay
// Displays the log buffer
//*************************************************************************
int UtilLog::LogBufferDisplay(char* logBuffer,
	sqluint32 numLogRecords,
	int conn, ReadLogWrap& pJavaWrap)
{
	int       rc = 0;
	sqluint32 logRecordNb = 0;
	sqluint32 recordSize = 0;
	sqluint16 recordType = 0;
	sqluint16 recordFlag = 0;
	char* recordBuffer = NULL;
	int headerSize = 0;

	// initialize the recordBuffer
	if (logBuffer == NULL)
	{
		if (numLogRecords == 0)
		{
			/* there's nothing to do */
			return 0;
		}
		else
		{
			/* we can't display NULL log records */
			return 1;
		}
	}

	// If there is no connection to the database or if the iFilterOption
	// is OFF, the 8-byte LRI 'db2LRI' is prefixed to the log records.
	// If there is a connection to the database and the iFilterOption is
	// ON, the db2ReadLogFilterData structure will be prefixed to all
	// log records returned by the db2ReadLog API ( for compressed and
	// uncompressed data )

	if (conn == 0)
	{
		headerSize = 2 * sizeof(db2LRI);
	}
	else
	{
		headerSize = sizeof(db2ReadLogFilterData);
	}
	recordBuffer = logBuffer;

	for (logRecordNb = 0; logRecordNb < numLogRecords; logRecordNb++)
	{
		if (conn == 1)
		{
			db2ReadLogFilterData* filterData = (db2ReadLogFilterData*)recordBuffer;

			//db2来的数据翻转成正确的值，发送给grpc
			pJavaWrap.set_scn(std::to_string(tool::reverse_value(filterData->recordLRIType1.lriType)) + ","
				+ std::to_string(tool::reverse_value(filterData->recordLRIType1.part1)) + "," + std::to_string(tool::reverse_value(filterData->recordLRIType1.part2)));


			//cout << "\nRLOG_FILTERDATA:\n" << endl;
			//cout << "    realLogRecLen: " << tool::reverse_value(filterData->realLogRecLen) << endl;
			//cout << "    sqlcode: " << filterData->sqlcode << endl;
		}

		//cout << "headerSize:" << headerSize << endl;
		recordBuffer += headerSize;

		//db2来的数据翻转成正确的值
		recordSize = tool::reverse_value(*(sqluint32*)(recordBuffer));
		recordType = tool::reverse_value(*(sqluint16*)(recordBuffer + sizeof(sqluint32)));
		recordFlag = tool::reverse_value(*(sqluint16*)(recordBuffer + sizeof(sqluint32) + sizeof(sqluint16)));

		//cout << "    recordSize: " << recordSize << endl;

		//cout << "    recordType: " << recordType << endl;

		//cout << "    recordFlag: " << recordFlag << endl;

		rc = LogRecordDisplay(recordBuffer, recordSize, recordType, recordFlag, pJavaWrap);
		CHECKRC(rc, "LogRecordDisplay");

		// // output every record
		// printf("record %d: ", logRecordNb);
		// for (sqluint32 i = 0; i < recordSize; i++) {
		// 	printf("%02x ", (unsigned char)recordBuffer[i]); 
		// }
		// printf("\n");

		// update the recordBuffer
		recordBuffer += recordSize;
	}

	return 0;
} // UtilLog::LogBufferDisplay

//**************************************************************************
// LogRecordDisplay
// Displays the log records
//**************************************************************************
int UtilLog::LogRecordDisplay(char* recordBuffer,
	sqluint32 recordSize,
	sqluint16 recordType,
	sqluint16 recordFlag, ReadLogWrap& pJavaWrap)
{
	int       rc = 0;
	sqluint32 logManagerLogRecordHeaderSize = 0;
	char* recordDataBuffer = NULL;
	sqluint32 recordDataSize = 0;
	char* recordHeaderBuffer = NULL;
	sqluint8  componentIdentifier = 0;
	sqluint32 recordHeaderSize = 0;

	// determine the logManagerLogRecordHeaderSize
	logManagerLogRecordHeaderSize = sizeof(LocalLogRecordHeader);
	if (recordType == 0x0043)  // compensation
	{
		logManagerLogRecordHeaderSize += sizeof(LocalLogRecordHeaderExtra); //V10 Extra Log stream ID 
		if (recordFlag & 0x0002)    // propagatable
		{
			logManagerLogRecordHeaderSize += sizeof(LocalLogRecordHeaderExtraCompensation);
		}
	}

	LocalLogRecordHeader* header = (LocalLogRecordHeader*)recordBuffer;

	//设置事务ID字段
	pJavaWrap.set_sql_tid(header->sqlTid);

	switch (recordType)
	{
	case 0x008A:                // Local Pending List
	case 0x0084:                // Normal Commit
	case 0x0041:                // Normal Abort
		recordDataBuffer = recordBuffer + logManagerLogRecordHeaderSize;
		recordDataSize = recordSize - logManagerLogRecordHeaderSize;
		rc = SimpleLogRecordDisplay(recordType,
			recordFlag,
			recordDataBuffer,
			recordDataSize, pJavaWrap);
		CHECKRC(rc, "SimpleLogRecordDisplay");
		break;
	case 0x004E:                // Normal
	case 0x0043:                // Compensation
	case 0x0052:                // Compensation Redo
	case 0x0055:                // Compensation Undo
		recordHeaderBuffer = recordBuffer + logManagerLogRecordHeaderSize;
		componentIdentifier = *(sqluint8*)recordHeaderBuffer;
		switch (componentIdentifier)
		{
		case 1:                 // Data Manager Log Record
			recordHeaderSize = 6;
			break;
		case 4:                 // Data Object Log Record
			recordHeaderSize = 12;
			break;
		case 5:                 // LOB Log Record
			recordHeaderSize = 12;
			break;
		case 11:                 // Relation Manager Log Record
			recordHeaderSize = 8;
			break;

		default:
			//LOG_WARN("    Unknown complex log record, recordType:{}, componentIdentifier:{}", recordType, (int)componentIdentifier);
			return 0;
		}
		recordDataBuffer = recordBuffer +
			logManagerLogRecordHeaderSize + recordHeaderSize;
		recordDataSize = recordSize -
			logManagerLogRecordHeaderSize - recordHeaderSize;
		rc = ComplexLogRecordDisplay(recordType,
			recordFlag,
			recordHeaderBuffer,
			recordHeaderSize,
			componentIdentifier,
			recordDataBuffer,
			recordDataSize, pJavaWrap);
		CHECKRC(rc, "ComplexLogRecordDisplay");
		break;
	default:
		//LOG_WARN("    Unknown log record, recordType:{}", recordType);
		break;
	}

	return 0;
} // UtilLog::LogRecordDisplay

//**************************************************************************
// SimpleLogRecordDisplay
// Prints the minimum details of the log record
//**************************************************************************
int UtilLog::SimpleLogRecordDisplay(sqluint16 recordType,
	sqluint16 recordFlag,
	char* recordDataBuffer,
	sqluint32 recordDataSize, ReadLogWrap& pJavaWrap)
{
	sqluint32 timeTransactionCommited = 0;
	//sqluint16 authIdLen = 0;
	//char* authId = NULL;
	//auto cleanAuthId = finally([&authId] 
	//	{
	//		if (authId)
	//			free(authId);
	//	});

	switch (recordType)
	{
	case 0x008A:
	{
		//cout << "\n    Record type: Local pending list" << endl;

		//db2来的数据翻转成正确的值
		timeTransactionCommited = tool::reverse_value(*(sqluint32*)(recordDataBuffer));
		//authIdLen = tool::reverse_value(*(sqluint16*)(recordDataBuffer + 2 * sizeof(sqluint32)));
		//authId = (char*)malloc(authIdLen + 1);
		//memset(authId, '\0', (authIdLen + 1));
		//memcpy(authId, (char*)(recordDataBuffer + 2 * sizeof(sqluint32) + sizeof(sqluint16)), authIdLen);
		//authId[authIdLen] = '\0';
		//cout << "      UTC pending committed(in secs since 70-01-01)" << ": "
		//	<< dec << timeTransactionCommited << endl;
		//cout << "      authorization ID of the application: " << authId << endl;

		auto re = pJavaWrap.sendNormalCommitMessage(timeTransactionCommited);
		if (re < 0)
			return re;
		return 0;
	}

	case 0x0084:
	{
		//cout << "\n    Record type: Normal commit" << endl;

		//db2来的数据翻转成正确的值
		timeTransactionCommited = tool::reverse_value(*(sqluint32*)(recordDataBuffer));
		//authIdLen = tool::reverse_value(*(sqluint16*)(recordDataBuffer + 2 * sizeof(sqluint32)));
		//authId = (char*)malloc(authIdLen + 1);
		//memset(authId, '\0', (authIdLen + 1));
		//memcpy(authId, (char*)(recordDataBuffer + 2 * sizeof(sqluint32) + sizeof(sqluint16)), authIdLen);
		//authId[authIdLen] = '\0';
		//cout << "      UTC transaction committed(in secs since 70-01-01)" << ": "
		//	<< dec << timeTransactionCommited << endl;
		//cout << "      authorization ID of the application: " << authId << endl;

		auto re = pJavaWrap.sendNormalCommitMessage(timeTransactionCommited);
		if (re < 0)
			return re;
		return 0;
	}
	case 0x0041:
	{
		auto re = pJavaWrap.sendAbortMessage();
		if (re < 0)
			return re;
		return 0;
	}

	default:
		LOG_WARN("    Unknown simple log, recordType:{}", recordType);

		break;
	}

	return 0;

} // UtilLog::SimpleLogRecordDisplay

//**************************************************************************
// ComplexLogRecordDisplay
// Prints a detailed information of the log record
//**************************************************************************
int UtilLog::ComplexLogRecordDisplay(sqluint16 recordType,
	sqluint16 recordFlag,
	char* recordHeaderBuffer,
	sqluint32 recordHeaderSize,
	sqluint8  componentIdentifier,
	char* recordDataBuffer,
	sqluint32 recordDataSize, ReadLogWrap& pJavaWrap)
{
	int rc = 0;
	sqluint8 functionIdentifier = 0;

	// for insert, delete, undo delete
	RID       recid;
	sqluint16 subRecordLen = 0;
	sqluint16 subRecordOffset = 0;
	char* subRecordBuffer = NULL;

	// for update
	RID       newRID;
	sqluint16 newSubRecordLen = 0;
//	sqluint16 newSubRecordOffset = 0;
	char* newSubRecordBuffer = NULL;
	RID       oldRID;
	sqluint16 oldSubRecordLen = 0;
//	sqluint16 oldSubRecordOffset = 0;
	char* oldSubRecordBuffer = NULL;

	// for alter table attributes
	sqluint64 alterBitMask = 0;
	sqluint64 alterBitValues = 0;

#ifdef SERVER_SUBLOG_DEBUG
	switch (recordType)
	{
	case 0x004E:
		//cout << "\n    Record type: Normal" << endl;
		break;
	case 0x0043:
		//cout << "\n    Record type: Compensation." << endl;
		break;
	case 0x0052:
		//cout << "\n    Record type: Compensation Redo." << endl;
		break;
	case 0x0055:
		//cout << "\n    Record type: Compensation Undo." << endl;
		break;
	default:
		cout << "\n    Unknown complex log record type: " << recordType
			<< endl;
		break;
	}

	switch (componentIdentifier)
	{
	case 1:
	{
		//cout << "      component ID: DMS log record" << endl;
		//LocalDMSLogRecordHeader* logRecordHeader = (LocalDMSLogRecordHeader*)recordHeaderBuffer;
		//cout << "      table Space ID:" << tool::reverse_value(logRecordHeader->tableSpaceIdentifier) << endl;
		//cout << "      table ID:" << tool::reverse_value(logRecordHeader->tableIdentifier) << endl;
		break;
	}
	case 4:
	{
		//cout << "      component ID: DOM log record" << endl;
		//LocalDOMLogRecordHeader* logRecordHeader = (LocalDOMLogRecordHeader*)recordHeaderBuffer;
		//cout << "      table Space ID:" << tool::reverse_value(logRecordHeader->tableSpaceIdentifier) << endl;
		//cout << "      table ID:" << tool::reverse_value(logRecordHeader->tableIdentifier) << endl;
		break;
	}
	case 5:
	{
		//cout << "      component ID: LOB log record" << endl;
		//LocalLOBLogRecordHeader* logRecordHeader = (LocalLOBLogRecordHeader*)recordHeaderBuffer;
		//cout << "      table Space ID:" << tool::reverse_value(logRecordHeader->tableSpaceIdentifier) << endl;
		//cout << "      object ID:" << tool::reverse_value(logRecordHeader->objectIdentifier) << endl;
		break;
	}
	case 11:
	{
		//cout << "      component ID: Relation Manager record" << endl;
		//LocalRDSLogRecordHeader* logRecordHeader = (LocalRDSLogRecordHeader*)recordHeaderBuffer;
		//cout << "      table Space ID:" << tool::reverse_value(logRecordHeader->tableSpaceIdentifier) << endl;
		//cout << "      table ID:" << tool::reverse_value(logRecordHeader->tableIdentifier) << endl;
		break;
	}
	default:
		cout << "      unknown component ID: " << (int)componentIdentifier << endl;
		break;
	}
#endif //SERVER_SUBLOG_DEBUG

	functionIdentifier = *(sqluint8*)(recordHeaderBuffer + 1);
	tapdata::ReadLogOp operation{ tapdata::ReadLogOp::UNKNOWN };

	char* logRecordBuff = NULL;
	sqluint16 userDataSize = 0;

	char* beforLogRecordBuff = NULL;
	sqluint16 beforUserDataSize = 0;

	//cout << "      functionIdentifier: " << (int)functionIdentifier << endl;
	bool isInternal = false;

	switch (functionIdentifier)
	{
	case 0x04:
	{
		//cout << "      function ID: DDL statement Record" << endl;
		const LocalDDLStatementLogRecord* record = (const LocalDDLStatementLogRecord*)recordDataBuffer;

		//cout << "\n >>>> operation " << tapdata::ReadLogOp::DDL;
		cout << "\n >>>> ddlTextLeft " << tool::reverse_value(record->ddlTextLeft);
		cout << "\n >>>> ddlTextLength " << tool::reverse_value(record->ddlTextLength);
		//cout << "\n >>>> function id " << (int)functionIdentifier;
		cout << endl;

		if (tool::reverse_value(record->ddlTextLeft) > 0 && tool::reverse_value(record->ddlTextLength) > 0)
		{
			rc = pJavaWrap.sendDDLMessage(record);
			if (rc < 0)
				return rc;
		}

		return 0;
	}
	case 0x05:
	{
		//cout << "      function ID: Undo DDL statement Record" << endl;
		const LocalRDSLogRecordHeader* record = (const LocalRDSLogRecordHeader*)recordHeaderBuffer;

		cout << "\n >>>> tableSpaceID " << tool::reverse_value(record->tableSpaceIdentifier);
		cout << "\n >>>> tableID " << tool::reverse_value(record->tableIdentifier);
		cout << "\n >>>> function id " << (int)functionIdentifier;
		cout << endl;

		rc = pJavaWrap.sendUndoDDLMessage(record);
		if (rc < 0)
			return rc;

		return 0;
	}

	case 0x23:
	{
		cout << "      function ID: Reorg Table" << endl;

		cout << "\n >>>> recordHeaderSize " << recordHeaderSize;
		cout << "\n >>>> userDataSize " << sizeof(ReorgTableLogRecord);
		cout << "\n >>>> function id " << (int)functionIdentifier;
		cout << endl;

		rc = pJavaWrap.sendReorgTableMessage((LocalDOMLogRecordHeader*)recordHeaderBuffer);
		if (rc < 0)
			return rc;

		return 0;
	}
	case 0x80:
		cout << "      function ID: Initial Table Record" << endl;
		break;
	case 0xAB:
	{
		cout << "      function ID: Changing Row Version Operation" << endl;
		cout << "\n >>>> recordHeaderSize " << recordHeaderSize;
		cout << "\n >>>> function id " << (int)functionIdentifier;
		cout << endl;

		rc = pJavaWrap.sendDMLMessage(functionIdentifier, operation, 0, 0, 0, 0, (LocalDMSLogRecordHeader*)recordHeaderBuffer, newRID.getString());
		if (rc < 0)
			return rc;

		return 0;
	}
	case 0xA1:
		//cout << "      function ID: Delete Record" << endl;
		subRecordLen = tool::reverse_value(*((sqluint16*)(recordDataBuffer + sizeof(sqluint16))));
		recid.set(recordDataBuffer + 3 * sizeof(sqluint16));
		//subRecordOffset = tool::reverse_value(*((sqluint16*)(recordDataBuffer + 3 * sizeof(sqluint16) + recid.size())));
		//cout << "        RID: " << dec << recid.getString() << endl;
		//cout << "        subrecord length: " << subRecordLen << endl;
		//cout << "        subrecord offset: " << subRecordOffset << endl;
		subRecordBuffer = recordDataBuffer + 3 * sizeof(sqluint16) +
			recid.size() + sizeof(sqluint16);
		rc = LogSubRecordDisplay(subRecordBuffer, subRecordLen, pJavaWrap, isInternal);
		operation = decltype(operation)::DELETE;
		logRecordBuff = subRecordBuffer + 8;
		userDataSize = subRecordLen - 8;
		CHECKRC(rc, "LogSubRecordDisplay");
		break;
	case 0x70:
		//LOG_DEBUG("      function ID: Undo Update Record");
		subRecordLen = tool::reverse_value(*((sqluint16*)(recordDataBuffer + sizeof(sqluint16))));
		recid.set(recordDataBuffer + 3 * sizeof(sqluint16));
		//LOG_DEBUG("        RID: {}", recid.getString());
		subRecordBuffer = recordDataBuffer + 3 * sizeof(sqluint16) + recid.size() + sizeof(sqluint16);
		rc = LogSubRecordDisplay(subRecordBuffer, subRecordLen, pJavaWrap, isInternal);
		CHECKRC(rc, "LogSubRecordDisplay");
		operation = decltype(operation)::ROLLBACK;
		break;
	case 0x6E:
		//LOG_DEBUG("      function ID: Undo Insert Record");
		subRecordLen = tool::reverse_value(*((sqluint16*)(recordDataBuffer + sizeof(sqluint16))));
		recid.set(recordDataBuffer + 3 * sizeof(sqluint16));
		//LOG_DEBUG("        RID: {}", recid.getString());
		operation = decltype(operation)::ROLLBACK;
		break;
	case 0x6F:
		//LOG_DEBUG("      function ID: Undo Delete Record");
		subRecordLen = tool::reverse_value(*((sqluint16*)(recordDataBuffer + sizeof(sqluint16))));
		recid.set(recordDataBuffer + 3 * sizeof(sqluint16));
		//LOG_DEBUG("        RID: {}", recid.getString());
		subRecordBuffer = recordDataBuffer + 3 * sizeof(sqluint16) + recid.size() + sizeof(sqluint16);
		rc = LogSubRecordDisplay(subRecordBuffer, subRecordLen, pJavaWrap, isInternal);
		CHECKRC(rc, "LogSubRecordDisplay");
		operation = decltype(operation)::ROLLBACK;
		break;
	case 0xA2:
		//cout << "      function ID: Insert Record" << endl;
		subRecordLen = tool::reverse_value(*((sqluint16*)(recordDataBuffer + sizeof(sqluint16))));
		recid.set(recordDataBuffer + 3 * sizeof(sqluint16));
		//subRecordOffset = tool::reverse_value(*((sqluint16*)(recordDataBuffer + 3 * sizeof(sqluint16) + recid.size())));
		//cout << "        RID: " << dec << recid.getString() << endl;
		//cout << "        subrecord length: " << subRecordLen << endl;
		//cout << "        subrecord offset: " << subRecordOffset << endl;
		subRecordBuffer = recordDataBuffer + 3 * sizeof(sqluint16) + recid.size() + sizeof(sqluint16);
		rc = LogSubRecordDisplay(subRecordBuffer, subRecordLen, pJavaWrap, isInternal);
		CHECKRC(rc, "LogSubRecordDisplay");
		logRecordBuff = subRecordBuffer + 8;
		operation = decltype(operation)::INSERT;
		userDataSize = subRecordLen - 8;
		break;
	case 0xA3:
		//cout << "      function ID: Update Record" << endl;
		newSubRecordLen = tool::reverse_value(*((sqluint16*)(recordDataBuffer + sizeof(sqluint16))));
		oldSubRecordLen = recordDataSize + 6 -       //NEW
			2 * 20 -
			newSubRecordLen;
		oldRID.set(recordDataBuffer + 3 * sizeof(sqluint16));

		//oldSubRecordOffset = tool::reverse_value(*((sqluint16*)(recordDataBuffer + 3 * sizeof(sqluint16) + oldRID.size())));

		newRID.set(recordDataBuffer + 3 * sizeof(sqluint16) +
			oldRID.size() + sizeof(sqluint16) + oldSubRecordLen +
			recordHeaderSize + 3 * sizeof(sqluint16));

		//newSubRecordOffset = *(sqluint16*)(recordDataBuffer +
		//	3 * sizeof(sqluint16) +
		//	oldRID.size() +
		//	sizeof(sqluint16) +
		//	oldSubRecordLen +
		//	recordHeaderSize +
		//	newRID.size() +
		//	sizeof(sqluint16));

		//cout << "        oldRID: " << dec << oldRID.getString() << endl;
		//cout << "        old subrecord length: " << oldSubRecordLen << endl;
		//cout << "        old subrecord offset: " << oldSubRecordOffset << endl;
		oldSubRecordBuffer = recordDataBuffer + 3 * sizeof(sqluint16) + oldRID.size() + sizeof(sqluint16);
		rc = LogSubRecordDisplay(oldSubRecordBuffer, oldSubRecordLen, pJavaWrap, isInternal);
		CHECKRC(rc, "LogSubRecordDisplay");

		//cout << "        newRID: " << dec << newRID.getString() << endl;
		//cout << "        new subrecord length: " << newSubRecordLen << endl;
		//cout << "        new subrecord offset: " << newSubRecordOffset << endl;
		newSubRecordBuffer = recordDataBuffer +
			3 * sizeof(sqluint16) +
			oldRID.size() +
			sizeof(sqluint16) +
			oldSubRecordLen +
			recordHeaderSize +
			3 * sizeof(sqluint16) +
			newRID.size() +
			sizeof(sqluint16);
		rc = LogSubRecordDisplay(newSubRecordBuffer, newSubRecordLen, pJavaWrap, isInternal);
		CHECKRC(rc, "LogSubRecordDisplay");
		operation = decltype(operation)::UPDATE;

		logRecordBuff = newSubRecordBuffer + 8;
		userDataSize = newSubRecordLen - 8;

		beforLogRecordBuff = oldSubRecordBuffer + 8;
		beforUserDataSize = oldSubRecordLen - 8;

		break;
	case 0xA5:
		cout << "      function ID: Insert Record to Empty Page" << endl;
		subRecordLen = tool::reverse_value(*((sqluint16*)(recordDataBuffer + sizeof(sqluint16))));
		recid.set(recordDataBuffer + 3 * sizeof(sqluint16));
		subRecordOffset = tool::reverse_value(*((sqluint16*)(recordDataBuffer + 3 * sizeof(sqluint16) + recid.size())));
		cout << "        RID: " << dec << recid.getString() << endl;
		cout << "        subrecord length: " << subRecordLen << endl;
		cout << "        subrecord offset: " << subRecordOffset << endl;
		subRecordBuffer = recordDataBuffer + 3 * sizeof(sqluint16) +
			recid.size() + sizeof(sqluint16);
		rc = LogSubRecordDisplay(subRecordBuffer, subRecordLen, pJavaWrap, isInternal);
		CHECKRC(rc, "LogSubRecordDisplay");
		break;
	case 0xA4:
		cout << "      function ID: Delete Record to Empty Page" << endl;
		subRecordLen = tool::reverse_value(*((sqluint16*)(recordDataBuffer + sizeof(sqluint16))));
		recid.set(recordDataBuffer + 3 * sizeof(sqluint16));
		subRecordOffset = tool::reverse_value(*((sqluint16*)(recordDataBuffer + 3 * sizeof(sqluint16) + recid.size())));
		cout << "        RID: " << dec << recid.getString() << endl;
		cout << "        subrecord length: " << subRecordLen << endl;
		cout << "        subrecord offset: " << subRecordOffset << endl;
		subRecordBuffer = recordDataBuffer + 3 * sizeof(sqluint16) +
			recid.size() + sizeof(sqluint16);
		rc = LogSubRecordDisplay(subRecordBuffer, subRecordLen, pJavaWrap, isInternal);
		CHECKRC(rc, "LogSubRecordDisplay");
		break;
	case 0xA6:
		cout << "      function ID: Rollback delete Record to Empty Page" << endl;
		subRecordLen = tool::reverse_value(*((sqluint16*)(recordDataBuffer + sizeof(sqluint16))));
		recid.set(recordDataBuffer + 3 * sizeof(sqluint16));
		subRecordOffset = tool::reverse_value(*((sqluint16*)(recordDataBuffer + 3 * sizeof(sqluint16) + recid.size())));
		cout << "        RID: " << dec << recid.getString() << endl;
		cout << "        subrecord length: " << subRecordLen << endl;
		cout << "        subrecord offset: " << subRecordOffset << endl;
		subRecordBuffer = recordDataBuffer + 3 * sizeof(sqluint16) +
			recid.size() + sizeof(sqluint16);
		rc = LogSubRecordDisplay(subRecordBuffer, subRecordLen, pJavaWrap, isInternal);
		CHECKRC(rc, "LogSubRecordDisplay");
		break;
	case 0x7C:
		cout << "      function ID:  Alter Table Attribute" << endl;
		alterBitMask = *(sqluint64*)(recordDataBuffer);
		alterBitValues = *((sqluint64*)(recordDataBuffer + sizeof(sqluint64)));
		if (alterBitMask & 0x00000001)
		{
			// Propagation attribute altered
			cout << "        Propagation attribute is changed to ";
			if (alterBitValues & 0x00000001)
			{
				cout << "ON" << endl;
			}
			else
			{
				cout << "OFF" << endl;
			}
		}
		if (alterBitMask & 0x00000002)
		{
			// Check Pending attribute altered
			cout << "        Check Pending attr. changed to: ";
			if (alterBitValues & 0x00000002)
			{
				cout << "ON" << endl;
			}
			else
			{
				cout << "OFF" << endl;
			}
		}
		if (alterBitMask & 0x00010000)
		{
			// Append Mode attribute altered
			cout << "        Append Mode attr. changed to: ";
			if (alterBitValues & 0x00010000)
			{
				cout << "ON" << endl;
			}
			else
			{
				cout << "OFF" << endl;
			}
		}
		if (alterBitMask & 0x00200000)
		{
			// LF Propagation attribute altered
			cout << "        LF Propagation attribute is changed to ";
			if (alterBitValues & 0x00200000)
			{
				cout << "ON" << endl;
			}
			else
			{
				cout << "OFF" << endl;
			}
		}
		if (alterBitMask & 0x00400000)
		{
			// LOB Propagation attribute altered
			cout << "        LOB Propagation attr.changed to: ";
			if (alterBitValues & 0x00400000)
			{
				cout << "ON" << endl;
			}
			else
			{
				cout << "OFF" << endl;
			}
		}
		break;
	default:
		//LOG_DEBUG("      unknown function identifier:{}", (int)functionIdentifier);
		break;
	}

	if (operation != decltype(operation)::UNKNOWN && !isInternal)
	{
		if (operation == decltype(operation)::UPDATE)
			rc = pJavaWrap.sendDMLMessage(functionIdentifier, operation, userDataSize, logRecordBuff, beforUserDataSize, beforLogRecordBuff, (LocalDMSLogRecordHeader*)recordHeaderBuffer, newRID.getString());
		else if (operation == decltype(operation)::ROLLBACK)
			rc = pJavaWrap.sendUndoDMLMessage((LocalDMSLogRecordHeader*)recordHeaderBuffer, recid.getString());
		else
			rc = pJavaWrap.sendDMLMessage(functionIdentifier, operation, userDataSize, logRecordBuff, beforUserDataSize, beforLogRecordBuff, (LocalDMSLogRecordHeader*)recordHeaderBuffer, recid.getString());

		if (rc < 0)
			return rc;
	}

	return 0;
} // UtilLog::ComplexLogRecordDisplay

/***************************************************************************/
/* LogSubRecordDisplay                                                     */
/* Prints the sub records for the log                                      */
/***************************************************************************/
int UtilLog::LogSubRecordDisplay(char* recordBuffer,
	sqluint16 recordSize, ReadLogWrap& pJavaWrap, bool& isInternal)
{
	int       rc = 0;
	sqluint8  recordType = 0;
	sqluint8  updatableRecordType = 0;

#ifdef SERVER_SUBLOG_DEBUG
	sqluint16 userDataFixedLength = 0;
	char* userDataBuffer = NULL;
	sqluint16 userDataSize = 0;
#endif //SERVER_SUBLOG_DEBUG

	recordType = *(sqluint8*)(recordBuffer);
	if ((recordType != 0) && (recordType != 4) && (recordType != 16))
	{
		// LOG_DEBUG("        Unknown subrecord type:{}", recordType);
	}
	else if (recordType == 4)
	{
		LOG_DEBUG("        subrecord type: Special control");
	}
	else
		// recordType == 0 or recordType == 16
		// record Type 0 indicates a normal record
		// record Type 16, for the purposes of this program, should be treated
		// as type 0
	{

		updatableRecordType = *(sqluint8*)(recordBuffer + 4);
		if (updatableRecordType != 1)
		{
			isInternal = true;
			//LOG_DEBUG("        subrecord type: Updatable, Internal control");
		}
#ifdef SERVER_SUBLOG_DEBUG
		else
		{
			LOG_DEBUG("        subrecord type: Updatable, Formatted user data");
			userDataFixedLength = tool::reverse_value(*(sqluint16*)(recordBuffer + 6));
			LOG_DEBUG("        user data fixed length:{}", userDataFixedLength);

			userDataBuffer = recordBuffer + 8;
			userDataSize = recordSize - 8;
			rc = UserDataDisplay(userDataBuffer, userDataSize, userDataFixedLength);
			CHECKRC(rc, "UserDataDisplay");
		}
#endif //SERVER_SUBLOG_DEBUG
	}
	return rc;
} // UtilLog::LogSubRecordDisplay

//**************************************************************************
// UserDataDisplay
// Displays the user data section
//**************************************************************************
#include <sstream>
int UtilLog::UserDataDisplay(char* dataBuffer,
	sqluint16 dataSize, sqluint16 userDataFixedLength)
{
	int       rc = 0;
	sqluint16 line = 0;
	sqluint16 col = 0;
	const int rowLength = 10;

	std::stringstream str;
	str << "\n        user data:" << endl;

	for (line = 0; line * rowLength < dataSize; line = line + 1)
	{
		str << "        ";
		for (col = 0; col < rowLength; col = col + 1)
		{
			if (line * rowLength + col < dataSize)
			{
				str.fill('0');
				str.width(2);
				str.setf(ios::uppercase);
				str << hex << (int)(dataBuffer[line * rowLength + col] & 0x0ff) <<
					" ";
			}
			else
			{
				str << "   ";
			}
		}
		str << "*";
		for (col = 0; col < rowLength; col = col + 1)
		{
			if (line * rowLength + col < dataSize)
			{
				if (isalpha(dataBuffer[line * rowLength + col]) ||
					isdigit(dataBuffer[line * rowLength + col]))
				{
					str << dataBuffer[line * rowLength + col];
				}
				else
				{
					str << ".";
				}
			}
			else
			{
				str << " ";
			}
		}
		str << "*" << endl;
	}
	str.setf(ios::dec);

	LOG_DEBUG("{}", str.str());
	return rc;
} // UtilLog::UserDataDisplay
