#include <string>
#include <memory>
#include "sqladef.h"
#include "DB2ReadLogApp.h"
#include "tool.h"
#include "tool_extern.h"
#include "log_imp.h"

static const short sqlIsLiteral = SQL_IS_LITERAL;
static const short sqlIsInputHvar = SQL_IS_INPUT_HVAR;


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
** SOURCE FILE NAME: dblogconn.sqC
**
** SAMPLE: How to read Asynchronous Log for Compressed and Uncompressed Tables
**
**         Note:
**           You must be disconnected from the sample database to run
**           this program. To ensure you are, enter 'db2 connect reset'
**           on the command line prior to read the logs.
**
** DB2 API USED:
**         db2CfgSet -- Set Configuration
**         db2ReadLog -- Asynchronous Read Log
**         db2Reorg -- Reorganize a Table or Index
**         sqle_deactivate_db -- Deactivate database
**
** SQL STATEMENTS USED:
**         ALTER TABLE
**         COMMIT
**         INSERT
**         UPDATE
**         DELETE
**         ROLLBACK
**         CONNECT RESET
**
** OUTPUT FILE: dblogconn.out (available in the online documentation)
*****************************************************************************
**
** For more information on the sample programs, see the README file.
**
** For information on developing C++ applications, see the Application
** Development Guide.
**
** For information on using SQL statements, see the SQL Reference.
**
** For information on DB2 APIs, see the Administrative API Reference.
**
** For the latest information on programming, building, and running DB2
** applications, visit the DB2 application development website:
**     http://www.software.ibm.com/data/db2/udb/ad
****************************************************************************/
#ifdef DB2NT
#include "utilrecov.cxx"
#else
#include "utilrecov.C"
#endif

#include "dblogconn.h"

#ifdef _MSC_VER
#include <windows.h>
#define sleep(sec)   Sleep((sec) * 1000)
#define msleep(msec) Sleep(msec)
#else
#include <unistd.h>
#define msleep(msec) usleep((msec) * 1000)
#endif

#pragma GCC diagnostic ignored "-Wwrite-strings"

using namespace std;

bool ReadLogWrap::isRunning() const
{
	using namespace tapdata;
	return get_app<DB2ReadLogApp>()->keep_run();
}

enum ReorgPendingFunctionID { ChangingRowVersion = 171 };

static ReorgPendingFunctionID ReorgPendingFunctionIDS[] = { ChangingRowVersion };

int ReadLogWrap::sendDMLMessage(int functionId, tapdata::ReadLogOp op, sqluint32 logRecordSize, const char* logRecordBuffer, sqluint32 beforeLogRecordSize, const char* beforeLogRecordBuffer,
	const LocalDMSLogRecordHeader* header, const char* rid) const
{
	if (!header)
		return -1;

	tapdata::ReadLogPayload payload;
	if (op != decltype(op)::UNKNOWN)
		payload.set_op(op);

	payload.set_scn(scn_);
	payload.set_transactionid(sql_tid_);

	if (logRecordBuffer)
		payload.set_logbytes(logRecordBuffer, logRecordSize);

	if (beforeLogRecordBuffer)
		payload.set_beforelogbytes(beforeLogRecordBuffer, beforeLogRecordSize);

	if (rid && strlen(rid) > 0)
		payload.set_rid(rid);

	payload.set_tableid(tool::reverse_value(header->tableIdentifier));
	payload.set_tablespaceid(tool::reverse_value(header->tableSpaceIdentifier));

	pending_scn_wrap_.add(scn_, payload.transactionid());
	payload.set_pendingminscn(pending_scn_wrap_.get_min_scn());
	return message_callback_funcs_.push_dml_message_func_(move(payload), any_of(begin(ReorgPendingFunctionIDS), end(ReorgPendingFunctionIDS), [functionId](auto i) {return functionId == i; }));
}

int ReadLogWrap::sendNormalCommitMessage(sqluint32 transactionTime) const
{
	tapdata::ReadLogPayload payload;
	payload.set_op(tapdata::ReadLogOp::COMMIT);
	payload.set_scn(scn_);
	payload.set_transactionid(sql_tid_);
	payload.set_transactiontime(transactionTime);

	pending_scn_wrap_.remove(payload.transactionid());
	payload.set_pendingminscn(pending_scn_wrap_.get_min_scn());
	return message_callback_funcs_.push_commit_message_func_(move(payload));
}

//返回小于0表示出错，直接退出
int ReadLogWrap::sendAbortMessage() const
{
	tapdata::ReadLogPayload payload;

	payload.set_scn(scn_);
	payload.set_transactionid(sql_tid_);

	pending_scn_wrap_.remove(payload.transactionid());
	payload.set_pendingminscn(pending_scn_wrap_.get_min_scn());
	return message_callback_funcs_.push_abort_message_func_(move(payload));
}

int ReadLogWrap::sendDDLMessage(const LocalDDLStatementLogRecord* record) const
{
	if (!record)
		return -1;

	int32_t schemaEntryStart = -1;
	int32_t schemaEntrySize = -1;
	int32_t entrySize = 0;

	const auto recordNumEntries = tool::reverse_value(record->numEntries);
	for (remove_const<decltype(recordNumEntries)>::type i = 0; i < recordNumEntries; ++i)
	{
		if (tool::reverse_value(record->entrys[i].type) == 1)//Default schema name
		{
			schemaEntryStart = entrySize;
			schemaEntrySize = tool::reverse_value(record->entrys[i].length);
		}
		entrySize += tool::reverse_value(record->entrys[i].length);
	}
	if (schemaEntrySize < 0)
		return -1;

	const char* const entryText = (const char*)(&record->entrys[recordNumEntries]);
	const char* const ddlText = entryText + entrySize;
	//string entryStr{ entryText, entrySize };
	const string ddlStr{ ddlText, tool::reverse_value(record->ddlTextLength) };

	LOG_INFO("get ddl:{}", ddlStr);

	const string defaultSchema(tool::trim_copy(string(entryText + schemaEntryStart, schemaEntrySize)));

	tapdata::ReadLogPayload payload;
	payload.set_op(tapdata::ReadLogOp::DDL);
	payload.set_scn(scn_);
	payload.set_transactionid(sql_tid_);

	tapdata::LocalDDLInfo info;
	if (info.setInfo(defaultSchema, ddlStr))//如果设置失败，表示当前的ddl语句没有被识别
	{
		payload.set_schema(info.schema);
		payload.set_tablename(info.tableName);
	}
	else
	{
		LOG_WARN("this ddl not recognized, keep going");
	}

	payload.set_logbytes(ddlStr.data(), ddlStr.size());
	payload.set_defaultschema(defaultSchema);

	pending_scn_wrap_.add(scn_, payload.transactionid());
	payload.set_pendingminscn(pending_scn_wrap_.get_min_scn());
	return message_callback_funcs_.push_ddl_message_func_(move(payload), move(info));
}

int ReadLogWrap::sendUndoDDLMessage(const LocalRDSLogRecordHeader* header) const
{
	if (!header)
		return -1;

	tapdata::ReadLogPayload payload;
	payload.set_transactionid(sql_tid_);

	pending_scn_wrap_.remove(payload.transactionid());
	payload.set_pendingminscn(pending_scn_wrap_.get_min_scn());
	return message_callback_funcs_.push_undo_ddl_message_func_(move(payload));
}

int ReadLogWrap::sendUndoDMLMessage(const LocalDMSLogRecordHeader* header, const char* rid) const
{
	if (!rid || strlen(rid) <= 0)
		return -1;

	tapdata::ReadLogPayload payload;

	payload.set_op(tapdata::ReadLogOp::ROLLBACK);

	payload.set_scn(scn_);
	payload.set_transactionid(sql_tid_);

	payload.set_rid(rid);

	payload.set_tableid(tool::reverse_value(header->tableIdentifier));
	payload.set_tablespaceid(tool::reverse_value(header->tableSpaceIdentifier));

	pending_scn_wrap_.remove(payload.transactionid());
	payload.set_pendingminscn(pending_scn_wrap_.get_min_scn());
	return message_callback_funcs_.push_undo_dml_message_func_(move(payload));
}

int ReadLogWrap::sendReorgTableMessage(const LocalDOMLogRecordHeader* header) const
{
	if (!header)
		return -1;

	tapdata::ReadLogPayload payload;
	payload.set_transactionid(sql_tid_);
	payload.set_tableid(tool::reverse_value(header->tableIdentifier));
	payload.set_tablespaceid(tool::reverse_value(header->tableSpaceIdentifier));

	return message_callback_funcs_.push_reorg_table_message_func_(move(payload));
}

int32_t ReadLogWrap::flush(bool flush_all)
{
	return message_callback_funcs_.flush_func_(flush_all);
}

static db2LRI parse_string(const string& start_scn, bool& error)
{
	db2LRI lri{};
	error = false;
	if (start_scn.empty())
		return lri;

	vector<string> out;
	tool::split(out, start_scn);
	if (out.size() != 3)
	{
		error = true;
		return lri;
	}

	try
	{
		lri.lriType = stoull(out[0]);
		lri.part1 = stoull(out[1]);
		lri.part2 = stoull(out[2]);
	}
	catch (const exception& ex)
	{
		LOG_ERROR("parse start scn fail, start scn:{}", start_scn);
		error = true;
	}

	return lri;
}

class DB2ContentWraper : public UtilRecov, public UtilLog
{
public:
	DB2ContentWraper(const ConnectDbSet& connect_set, size_t buffer_size = 64 * 1024 * 50, db2Uint32 db2_version = db2Version1010) :
		db2_version_{ db2_version }
	{
		instance_.setInstance((char*)connect_set.nodeName, (char*)connect_set.user, (char*)connect_set.pswd);
		db_emb_.setDb((char*)connect_set.alias, (char*)connect_set.user, (char*)connect_set.pswd);
		log_buffer_.resize(buffer_size);
	}

	int64_t read_log_loop(const db2LRI& startLri, int64_t sleep_interval, ReadLogWrap& rlw)
	{
		int64_t result{};
		rlw.get_current_lri() = startLri;
		do
		{
			if (rlw.isRunning())
			{
				result = try_connect();
				if (result != 0)
				{
					LOG_WARN("connect fail, result:{}", result);
					return result;
				}

				result = read_log(sleep_interval, rlw);
				if (result < 0)
				{
					LOG_WARN("read_log back, result:{}", result);
				}
			}
		} while (result == -30081);//-30081，1.docker断开网桥，2.db2 force application all 都是这个错误
		rlw.flush(true);
		return result;
	}

	~DB2ContentWraper()
	{
		disconnect();
		detachInstance();
	}

private:
	int try_connect(uint32_t try_times = 3, uint32_t try_time_interval_s = 30)
	{
		for (uint32_t t = 0; t < try_times; ++t)
		{
			if (attachInstance() == 0)
				break;
			else
				sleep(try_time_interval_s);
		}

		if (rc_ != 0)
			return rc_;

		for (uint32_t t = 0; t < try_times; ++t)
		{
			if (connect() == 0)
				break;
			else
				sleep(try_time_interval_s);
		}
		return rc_;
	}


	int64_t read_log(int64_t sleep_interval, ReadLogWrap& rlw)
	{
		sqlca sqlca{};
		rc_ = init_info_and_lris(rlw.time_off_set(), rlw.get_current_lri(), rlw.get_end_lri_());
		LOG_DEBUG("init_info_and_lris rc_:{}", rc_);

		if (rc_ < 0)
			return rc_;

		read_log_input_.iCallerAction = DB2READLOG_READ;
		read_log_input_.piStartLRI = &rlw.get_current_lri();
		read_log_input_.piEndLRI = &rlw.get_end_lri_();
		read_log_input_.poLogBuffer = log_buffer_.data();
		read_log_input_.iLogBufferSize = log_buffer_.size();
		read_log_input_.iFilterOption = DB2READLOG_FILTER_ON;
		read_log_input_.poReadLogInfo = &read_log_info_;

		rc_ = db2ReadLog(db2_version_, &read_log_input_, &sqlca);
		if (sqlca.sqlcode != SQLU_RLOG_READ_TO_CURRENT)
		{
			DB2_API_CHECK("database logs -- read");
		}
		else
		{

			if (read_log_info_.logRecsWritten == 0)
			{
				cout << "  Database log empty." << endl;
			}
		}

		//从db2来的数值，需翻转
		rc_ = LogBufferDisplay(log_buffer_.data(), tool::reverse_value(read_log_info_.logRecsWritten), 1, rlw);
		CHECKRC(rc_, "UtilLog.LogBufferDisplay");

		while (rlw.isRunning())
		{
			// Read the next log sequence
			memcpy(&rlw.get_current_lri(), &(read_log_info_.nextStartLRI), sizeof(rlw.get_current_lri()));

			// Extract a log record from the database logs, and
			// read the next log sequence asynchronously.

			rc_ = db2ReadLog(db2_version_, &read_log_input_, &sqlca);
			if (sqlca.sqlcode != SQLU_RLOG_READ_TO_CURRENT)
			{
				DB2_API_CHECK("database logs -- read");
			}

			//从db2来的数值，需翻转
			rc_ = LogBufferDisplay(log_buffer_.data(), tool::reverse_value(read_log_info_.logRecsWritten), 1, rlw);
			CHECKRC(rc_, "LogBufferDisplay");
			if (!read_log_info_.logRecsWritten)
				msleep(sleep_interval);
		}

#if 0
		//官方例子，现在不生效
		while (keep_run())
		{
			// Read the next log sequence
			memcpy(&startLRI, &(readLogInfo.nextStartLRI), sizeof(startLRI));

			// Extract a log record from the database logs, and
			// read the next log sequence asynchronously.
			rc = db2ReadLog(versionNumber, &readLogInput, &sqlca);

			if (sqlca.sqlcode == 0)
			{
				if (readLogInfo.logRecsWritten > 0)
				{
					// Display the log buffer
					rc = LogBufferDisplay(logBuffer, readLogInfo.logRecsWritten, 1, pJavaWrap);
					CHECKRC(rc, "LogBufferDisplay");
					msleep(interval);
				}
				else if (readLogInfo.logRecsWritten == 0)
				{
					DB2_API_CHECK("database logs -- no data returned");
					break;
				}
			}
			else if (sqlca.sqlcode == SQLU_RLOG_READ_TO_CURRENT)
			{
				DB2_API_CHECK("database logs -- end of logs");
				break;
			}
			else if (sqlca.sqlcode != 0)
			{
				DB2_API_CHECK("database logs -- sqlcode error: sqlca.sqlcode");
				break;
			}
		}
#endif
		return 0;
	}

	//小于0表示获取lri失败
	int64_t init_info_and_lris(int64_t timeOffset, db2LRI& outStartLri, db2LRI& outEndLri)
	{
		rc_ = init_read_log_struct();
		if (rc_ < 0)
			return rc_;

		{
			//装载end lri
			outEndLri.lriType = read_log_info_.initialLRI.lriType;
			outEndLri.part1 = (decltype(outEndLri.part1))-1;
			outEndLri.part2 = (decltype(outEndLri.part2))-1;

			tool::reverse_bytes(&outEndLri.part1);
			tool::reverse_bytes(&outEndLri.part2);
		}
		LOG_DEBUG("outStartLri.lriType:{}", outStartLri.lriType);
		LOG_DEBUG("outStartLri.part1:{}", outStartLri.part1);
		LOG_DEBUG("outStartLri.part2:{}", outStartLri.part2);
		if (outStartLri.lriType || outStartLri.part1 || outStartLri.part2)//既然有值,在此结束
			return 0;
		LOG_DEBUG("startLri.......");

		db2LRI startLri;
		//从db2来的数值进行大小比较，需翻转
		if (tool::reverse_value(read_log_info_.initialLRI.part1) > tool::reverse_value(read_log_info_.nextStartLRI.part1))
			memcpy(&startLri, &(read_log_info_.nextStartLRI), sizeof(startLri));//从db2来的又返回db2，不需要翻转
		else
			memcpy(&startLri, &(read_log_info_.initialLRI), sizeof(startLri));//从db2来的又返回db2，不需要翻转

		outStartLri = startLri;
		LOG_DEBUG("timeOffset:{}", timeOffset);

		if (timeOffset <= 0)//既然没有配置timeOffset,在此结束,使用init_read_log_struct查询出来的结果
			return 0;

		//下方开始进行lri跳转

		db2LRI leftLri = startLri; //下界lri
		db2LRI lastLri;

		read_log_input_.iCallerAction = DB2READLOG_READ;
		read_log_input_.iLogBufferSize = log_buffer_.size();
		read_log_input_.iFilterOption = DB2READLOG_FILTER_ON;

		enum { jump_size_start = 1 << 25, jump_size_min = jump_size_start >> 24, jump_size_max = jump_size_start << 4 };//之所以开始这么大，是因为找不到日志要很久才返回
		int64_t lri_time;
		int64_t jump_size = jump_size_start;
		LOG_DEBUG("time off set:{}", timeOffset);

		startLri.part1 = tool::reverse_value(tool::reverse_value(leftLri.part1) + jump_size);

		sqlca sqlca{};
		while (true) {

			LOG_DEBUG("jump size:{}", jump_size);
			LOG_DEBUG("start lri:{}.{}.{}", tool::reverse_value(startLri.lriType), tool::reverse_value(startLri.part1), tool::reverse_value(startLri.part2));

			lastLri = startLri;//记录开始的Lri, get_time_of_lri里 startLri会被修改
			lri_time = get_time_of_lri(db2_version_, read_log_input_, read_log_info_, sqlca, outEndLri, startLri, log_buffer_.data());
			LOG_DEBUG("lri time:{}", lri_time);

			if (lri_time > 0)//lri时间有意义
			{
				if (lri_time < timeOffset)//lri的时间发生在offset之前，比offset小
				{
					if (lri_time > 0)
					{
						outStartLri = lastLri;
						leftLri = lastLri;//下界右移
						LOG_INFO("get useful time:{}", lri_time);
						LOG_INFO("set start lri:{}.{}.{}", tool::reverse_value(outStartLri.lriType), tool::reverse_value(outStartLri.part1), tool::reverse_value(outStartLri.part2));
					}
					if (jump_size == jump_size_min)//现在已经跳跃了足够多次
						break;
					startLri.part1 = tool::reverse_value(tool::reverse_value(leftLri.part1) + jump_size);//从下界移动
				}
				else//lri_time不大于offset,jump_size减少一半
				{
					jump_size = jump_size >> 1;
					if (jump_size == jump_size_min)//现在已经跳跃了足够多次
						break;
					startLri.part1 = tool::reverse_value(tool::reverse_value(leftLri.part1) + jump_size);//从下界移动
				}
			}
			else if (lri_time == 0)//找不到有效日志
			{
				leftLri = lastLri;//下界移动到找不到日志时间的地方，继续寻找下一条日志
				startLri.part1 = tool::reverse_value(tool::reverse_value(leftLri.part1) + jump_size);
				//startLri = readLogInfo.nextStartLRI;//找不到有效日期就在下一个lri里找
			}
			else if (lri_time == SQLU_RLOG_INVALID_PARM)//lri越界缩短起跳范围，jump_size减少一半
			{
				jump_size = jump_size >> 1;
				if (jump_size == jump_size_min)//现在已经跳跃了足够多次
					break;
				startLri.part1 = tool::reverse_value(tool::reverse_value(leftLri.part1) + jump_size);//从下界移动
			}
			else if (lri_time == SQLU_RLOG_EXTENT_REQUIRED)//lri日志被清理，增加起跳范围
			{
				leftLri = lastLri;//下界移动到被清理日志的地方
				if (jump_size == jump_size_min)//现在已经跳跃了足够多次
					break;
				startLri.part1 = tool::reverse_value(tool::reverse_value(leftLri.part1) + jump_size);
			}
			else//其余按越界处理
			{
				jump_size = jump_size >> 1;
				if (jump_size == jump_size_min)//现在已经跳跃了足够多次
					break;
				startLri.part1 = tool::reverse_value(tool::reverse_value(leftLri.part1) + jump_size);//从下界移动
			}
		}

		LOG_INFO("out start lri:{}.{}.{}", tool::reverse_value(outStartLri.lriType), tool::reverse_value(outStartLri.part1), tool::reverse_value(outStartLri.part2));
		return 0;
	}

	//返回0表示没有找到时间,大于0正确
	static sqluint32 get_commit_time(char* recordBuffer, sqluint16 recordType, sqluint16 recordFlag)
	{
		auto logManagerLogRecordHeaderSize = sizeof(LocalLogRecordHeader);
		if (recordType == 0x0043)  // compensation
		{
			logManagerLogRecordHeaderSize += sizeof(LocalLogRecordHeaderExtra); //V10 Extra Log stream ID 
			if (recordFlag & 0x0002)    // propagatable
			{
				logManagerLogRecordHeaderSize += sizeof(LocalLogRecordHeaderExtraCompensation);
			}
		}

		switch (recordType)
		{
		case 138:
		case 132:
			return  tool::reverse_value(*(sqluint32*)(recordBuffer + logManagerLogRecordHeaderSize));
		default:
			return 0;
		}

		return 0;
	}

	//返回0没有找到合法时间,大于0时间有效,-1错误
	static int64_t get_first_commit_time(char* logBuffer, sqluint32 numLogRecords)
	{
		int64_t result = 0;
		if (!logBuffer)
		{
			if (numLogRecords == 0)
				return result = 0;
			else
				return result = -1;
		}

		char* recordBuffer = logBuffer;
		sqluint32 recordSize;
		sqluint16 recordType;
		sqluint16 recordFlag;

		for (sqluint32 logRecordNb = 0; logRecordNb < numLogRecords; logRecordNb++)
		{
			recordBuffer += sizeof(db2ReadLogFilterData);

			recordSize = tool::reverse_value(*(sqluint32*)(recordBuffer));
			recordType = tool::reverse_value(*(sqluint16*)(recordBuffer + sizeof(sqluint32)));
			recordFlag = tool::reverse_value(*(sqluint16*)(recordBuffer + sizeof(sqluint32) + sizeof(sqluint16)));

			result = get_commit_time(recordBuffer, recordType, recordFlag);
			if (result > 0)
				return result;
			recordBuffer += recordSize;
		}

		return result;
	}

	//返回值：0表示时间无效，小于0中SQLU_RLOG_INVALID_PARM表示日志超出越界，SQLU_RLOG_EXTENT_REQUIRED表示该日志被清理，大于0表示lri时间
	static int64_t get_time_of_lri(db2Uint32 versionNumber, db2ReadLogStruct& readLogInput, db2ReadLogInfoStruct& readLogInfo, struct sqlca& sqlca,
		db2LRI& endLRI, db2LRI& startLRI, char* logBuffer)
	{
		readLogInput.poLogBuffer = logBuffer;
		readLogInput.piStartLRI = &startLRI;
		readLogInput.piEndLRI = &endLRI;
		readLogInput.poReadLogInfo = &readLogInfo;

		db2ReadLog(versionNumber, &readLogInput, &sqlca);

		if (sqlca.sqlcode < 0)
			return sqlca.sqlcode;

		else if (sqlca.sqlcode == 0 || sqlca.sqlcode == SQLU_RLOG_READ_TO_CURRENT)
		{
			return get_first_commit_time(logBuffer, tool::reverse_value(readLogInfo.logRecsWritten));
		}
		else
		{
			LOG_INFO("sql code:{}", sqlca.sqlcode);
			return sqlca.sqlcode;//SQLU_RLOG_INVALID_PARM表示越界，SQLU_RLOG_EXTENT_REQUIRED表示该日志已经被删除
		}
	}

	//小于0表示失败，大于等于0成功
	int64_t init_read_log_struct()
	{
		db2ReadLogStruct t{};
		std::swap(read_log_input_, t);

		read_log_input_.iCallerAction = DB2READLOG_QUERY;
		read_log_input_.piStartLRI = NULL;
		read_log_input_.piEndLRI = NULL;
		read_log_input_.poLogBuffer = NULL;
		read_log_input_.iLogBufferSize = 0;

		db2ReadLogInfoStruct tt{};
		std::swap(read_log_info_, tt);

		read_log_input_.iFilterOption = DB2READLOG_FILTER_ON;
		read_log_input_.poReadLogInfo = &read_log_info_;
		read_log_input_.piMinRequiredLRI = NULL;

		sqlca sqlca = { 0 };

		rc_ = db2ReadLog(db2_version_, &read_log_input_, &sqlca);
		LOG_DEBUG("db2ReadLog rc_:{}", rc_);
		EXPECTED_ERR_CHECK("database log info -- get");
		if (sqlca.sqlcode < 0)
			return sqlca.sqlcode;
		return 0;
	}

	int attachInstance()
	{
		rc_ = instance_.Attach();
		LOG_INFO("Attach rc:{}", rc_);
		CHECKRC(rc_, "Attach");
		return rc_;
	}

	int detachInstance()
	{
		rc_ = instance_.Detach();
		LOG_INFO("Detach rc:{}", rc_);
		CHECKRC(rc_, "Detach");
		return rc_;
	}

	int connect()
	{
		rc_ = db_emb_.Connect();
		LOG_INFO("Connect rc:{}", rc_);
		CHECKRC(rc_, "Connect");
		return rc_;
	}

	int disconnect()
	{
		rc_ = db_emb_.Disconnect();
		LOG_INFO("Disconnect rc:{}", rc_);
		CHECKRC(rc_, "Disconnect");
		return rc_;
	}

	db2Uint32 db2_version_;
	int64_t rc_{};
	Instance instance_;
	DbEmb db_emb_;
	vector<char> log_buffer_;
	db2ReadLogStruct read_log_input_;
	db2ReadLogInfoStruct read_log_info_;
};


int64_t ReadLogWrap::db2_read_log(const ConnectDbSet& connect_set, const string& start_scn, int64_t sleep_interval)
{
	bool parse_error;
	LOG_INFO("start_scn   :{}", start_scn);
	const auto startLRI = parse_string(start_scn, parse_error);
	if (parse_error)
		return -1;

	DB2ContentWraper content(connect_set);
	return content.read_log_loop(startLRI, sleep_interval, *this);
}


