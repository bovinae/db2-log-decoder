#include <string>
#include <memory>
#include "sqladef.h"
#include "DB2ReadLogApp.h"
#include "tool.h"
#include "tool_extern.h"
#include "log_imp.h"
#include <condition_variable>
#include <signal.h>
#include "lri_recorder.h"

bool cache_switch = false;

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
	int rc = message_callback_funcs_.push_dml_message_func_(move(payload), any_of(begin(ReorgPendingFunctionIDS), end(ReorgPendingFunctionIDS), [functionId](auto i) {return functionId == i; }));
	return rc;
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

int ReadLogWrap::sendHeartbeatMessage() const
{
	tapdata::ReadLogPayload payload;
	payload.set_op(tapdata::ReadLogOp::HEARTBEAT);
	payload.set_scn(scn_);
	payload.set_transactionid("-1");
	payload.set_pendingminscn(pending_scn_wrap_.get_min_scn());
	auto currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	payload.set_transactiontime(currentTime);
	return message_callback_funcs_.push_heartbeat_message_func_(move(payload));
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
	DB2ContentWraper(const ConnectDbSet& connect_set, size_t buffer_size = 3 * 1024 * 1024, db2Uint32 db2_version = db2Version1010) :
		db2_version_{ db2_version }
	{
		instance_.setInstance((char*)connect_set.nodeName, (char*)connect_set.user, (char*)connect_set.pswd);
		db_emb_.setDb((char*)connect_set.alias, (char*)connect_set.user, (char*)connect_set.pswd);
		log_buffer_.resize(buffer_size);
		readlog_mutex_.init_multi_process_mutex(false);
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
	mutex_wrapper readlog_mutex_;

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

	// static void log_lri(std::string prefix, const db2LRI &lri) {
	// 	LOG_DEBUG("{}: {}.{}.{}", prefix, tool::reverse_value(lri.lriType), tool::reverse_value(lri.part1), tool::reverse_value(lri.part2));
	// }
	#define log_lri(prefix, lri) LOG_DEBUG("{}: {}.{}.{}", prefix, tool::reverse_value(lri.lriType), tool::reverse_value(lri.part1), tool::reverse_value(lri.part2))

	static void log_read_log_info(db2ReadLogInfoStruct& read_log_info) {
		return;
		LOG_DEBUG("read_log_info initialLRI: {}.{}.{}", tool::reverse_value(read_log_info.initialLRI.lriType), tool::reverse_value(read_log_info.initialLRI.part1), tool::reverse_value(read_log_info.initialLRI.part2));
		LOG_DEBUG("read_log_info firstReadLRI: {}.{}.{}", tool::reverse_value(read_log_info.firstReadLRI.lriType), tool::reverse_value(read_log_info.firstReadLRI.part1), tool::reverse_value(read_log_info.firstReadLRI.part2));
		LOG_DEBUG("read_log_info nextStartLRI: {}.{}.{}", tool::reverse_value(read_log_info.nextStartLRI.lriType), tool::reverse_value(read_log_info.nextStartLRI.part1), tool::reverse_value(read_log_info.nextStartLRI.part2));
		LOG_DEBUG("read_log_info firstReusedLRI: {}.{}.{}", tool::reverse_value(read_log_info.firstReusedLRI.lriType), tool::reverse_value(read_log_info.firstReusedLRI.part1), tool::reverse_value(read_log_info.firstReusedLRI.part2));
		LOG_DEBUG("read_log_info logRecsWritten: {}", tool::reverse_value(read_log_info.logRecsWritten));
		LOG_DEBUG("read_log_info logBytesWritten: {}", tool::reverse_value(read_log_info.logBytesWritten));
		LOG_DEBUG("read_log_info timeOfLRIReuse: {}", tool::reverse_value(read_log_info.timeOfLRIReuse));
		LOG_DEBUG("read_log_info currentTimeValue.seconds: {}", tool::reverse_value(read_log_info.currentTimeValue.seconds));
		LOG_DEBUG("read_log_info oldestInFlightLSN.lsnU64: {}", tool::reverse_value(read_log_info.oldestInFlightLSN.lsnU64));
	}

	int64_t read_log(int64_t sleep_interval, ReadLogWrap& rlw)
	{
		sqlca sqlca{};
		rc_ = init_info_and_lris(rlw);
		LOG_DEBUG("init info and lris rc_:{}", rc_);
		LOG_INFO("out start lri:{}.{}.{}", tool::reverse_value(rlw.get_current_lri().lriType), tool::reverse_value(rlw.get_current_lri().part1), tool::reverse_value(rlw.get_current_lri().part2));
		if (rc_ < 0)
			return rc_;

		read_log_input_.iCallerAction = DB2READLOG_READ;
		read_log_input_.piStartLRI = &rlw.get_current_lri();
		read_log_input_.piEndLRI = &rlw.get_end_lri_();
		read_log_input_.poLogBuffer = log_buffer_.data();
		read_log_input_.iLogBufferSize = log_buffer_.size();
		read_log_input_.iFilterOption = DB2READLOG_FILTER_ON;
		read_log_input_.poReadLogInfo = &read_log_info_;

		int step = 10;
		int retry_cnt = 1000;
retry:
		// readlog_mutex_.multi_process_mutex_lock();
		rc_ = db2ReadLog(db2_version_, &read_log_input_, &sqlca);
		// readlog_mutex_.multi_process_mutex_unlock();
		log_read_log_info(read_log_info_);
		if (sqlca.sqlcode == SQLU_RLOG_INVALID_PARM && retry_cnt > 0) {
			read_log_input_.piStartLRI->part1 = tool::reverse_value(tool::reverse_value(read_log_input_.piStartLRI->part1) - step);
			read_log_input_.piStartLRI->part2 = tool::reverse_value(tool::reverse_value(read_log_input_.piStartLRI->part2) - 2*step);
			LOG_DEBUG("remaining retry cnt:{}", retry_cnt);
			auto tmp = *read_log_input_.piStartLRI;
			log_lri("found start lri, but recall db2ReadLog sqlcode is -2650, retry", tmp);
			retry_cnt--;
			goto retry;
		}
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

			// LOG_DEBUG("db2ReadLog begin");
			// readlog_mutex_.multi_process_mutex_lock();
			rc_ = db2ReadLog(db2_version_, &read_log_input_, &sqlca);
			// readlog_mutex_.multi_process_mutex_unlock();
			log_read_log_info(read_log_info_);
			if (sqlca.sqlcode != SQLU_RLOG_READ_TO_CURRENT)
			{
				DB2_API_CHECK("database logs -- read");
			}

			//从db2来的数值，需翻转
			rc_ = LogBufferDisplay(log_buffer_.data(), tool::reverse_value(read_log_info_.logRecsWritten), 1, rlw);
			CHECKRC(rc_, "LogBufferDisplay");
			if (!read_log_info_.logRecsWritten) {
				// sleep(2);
				rlw.sendHeartbeatMessage();
				msleep(sleep_interval);
			}
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
	// int64_t init_info_and_lris1(int64_t timeOffset, db2LRI& outStartLri, db2LRI& outEndLri)
	// {
	// 	rc_ = init_read_log_struct();
	// 	if (rc_ < 0)
	// 		return rc_;

	// 	{
	// 		//装载end lri
	// 		outEndLri.lriType = read_log_info_.initialLRI.lriType;
	// 		outEndLri.part1 = (decltype(outEndLri.part1))-1;
	// 		outEndLri.part2 = (decltype(outEndLri.part2))-1;

	// 		tool::reverse_bytes(&outEndLri.part1);
	// 		tool::reverse_bytes(&outEndLri.part2);
	// 	}
	// 	LOG_DEBUG("outStartLri.lriType:{}", outStartLri.lriType);
	// 	LOG_DEBUG("outStartLri.part1:{}", outStartLri.part1);
	// 	LOG_DEBUG("outStartLri.part2:{}", outStartLri.part2);
	// 	if (outStartLri.lriType || outStartLri.part1 || outStartLri.part2)//既然有值,在此结束
	// 		return 0;
	// 	LOG_DEBUG("startLri.......");

	// 	db2LRI startLri;
	// 	//从db2来的数值进行大小比较，需翻转
	// 	if (tool::reverse_value(read_log_info_.initialLRI.part1) > tool::reverse_value(read_log_info_.nextStartLRI.part1))
	// 		memcpy(&startLri, &(read_log_info_.nextStartLRI), sizeof(startLri));//从db2来的又返回db2，不需要翻转
	// 	else
	// 		memcpy(&startLri, &(read_log_info_.initialLRI), sizeof(startLri));//从db2来的又返回db2，不需要翻转

	// 	outStartLri = startLri;
	// 	LOG_DEBUG("timeOffset:{}", timeOffset);

	// 	if (timeOffset <= 0)//既然没有配置timeOffset,在此结束,使用init_read_log_struct查询出来的结果
	// 		return 0;

	// 	//下方开始进行lri跳转

	// 	db2LRI leftLri = startLri; //下界lri
	// 	db2LRI lastLri;

	// 	read_log_input_.iCallerAction = DB2READLOG_READ;
	// 	read_log_input_.iLogBufferSize = log_buffer_.size();
	// 	read_log_input_.iFilterOption = DB2READLOG_FILTER_ON;

	// 	//之所以开始这么大，是因为找不到日志要很久才返回
	// 	int64_t jump_size_start = (int64_t)1 << 58;
	// 	int64_t jump_size_min = jump_size_start >> 57;
	// 	// int64_t jump_size_max = jump_size_start << 4;
	// 	int64_t lri_time;
	// 	int64_t jump_size = jump_size_start;
	// 	LOG_DEBUG("time off set:{}", timeOffset);

	// 	startLri.part1 = tool::reverse_value(tool::reverse_value(leftLri.part1) + jump_size);

	// 	sqlca sqlca{};
	// 	while (true) {

	// 		LOG_DEBUG("jump size:{}", jump_size);
	// 		LOG_DEBUG("start lri:{}.{}.{}", tool::reverse_value(startLri.lriType), tool::reverse_value(startLri.part1), tool::reverse_value(startLri.part2));

	// 		lastLri = startLri;//记录开始的Lri, get_time_of_lri里 startLri会被修改
	// 		lri_time = get_time_of_lri(db2_version_, read_log_input_, read_log_info_, sqlca, outEndLri, startLri, log_buffer_.data());
	// 		LOG_DEBUG("lri time:{}", lri_time);

	// 		if (lri_time > 0)//lri时间有意义
	// 		{
	// 			if (lri_time < timeOffset)//lri的时间发生在offset之前，比offset小
	// 			{
	// 				if (lri_time > 0)
	// 				{
	// 					outStartLri = lastLri;
	// 					leftLri = lastLri;//下界右移
	// 					LOG_INFO("get useful time:{}", lri_time);
	// 					LOG_INFO("set start lri:{}.{}.{}", tool::reverse_value(outStartLri.lriType), tool::reverse_value(outStartLri.part1), tool::reverse_value(outStartLri.part2));
	// 				}
	// 				if (jump_size == jump_size_min)//现在已经跳跃了足够多次
	// 					break;
	// 				startLri.part1 = tool::reverse_value(tool::reverse_value(leftLri.part1) + jump_size);//从下界移动
	// 			}
	// 			else//lri_time不大于offset,jump_size减少一半
	// 			{
	// 				jump_size = jump_size >> 1;
	// 				if (jump_size == jump_size_min)//现在已经跳跃了足够多次
	// 					break;
	// 				startLri.part1 = tool::reverse_value(tool::reverse_value(leftLri.part1) + jump_size);//从下界移动
	// 			}
	// 		}
	// 		else if (lri_time == 0)//找不到有效日志
	// 		{
	// 			leftLri = lastLri;//下界移动到找不到日志时间的地方，继续寻找下一条日志
	// 			startLri.part1 = tool::reverse_value(tool::reverse_value(leftLri.part1) + jump_size);
	// 			//startLri = readLogInfo.nextStartLRI;//找不到有效日期就在下一个lri里找
	// 		}
	// 		else if (lri_time == SQLU_RLOG_INVALID_PARM)//lri越界缩短起跳范围，jump_size减少一半
	// 		{
	// 			jump_size = jump_size >> 1;
	// 			if (jump_size == jump_size_min)//现在已经跳跃了足够多次
	// 				break;
	// 			startLri.part1 = tool::reverse_value(tool::reverse_value(leftLri.part1) + jump_size);//从下界移动
	// 		}
	// 		else if (lri_time == SQLU_RLOG_EXTENT_REQUIRED)//lri日志被清理，增加起跳范围
	// 		{
	// 			leftLri = lastLri;//下界移动到被清理日志的地方
	// 			if (jump_size == jump_size_min)//现在已经跳跃了足够多次
	// 				break;
	// 			startLri.part1 = tool::reverse_value(tool::reverse_value(leftLri.part1) + jump_size);
	// 		}
	// 		else//其余按越界处理
	// 		{
	// 			jump_size = jump_size >> 1;
	// 			if (jump_size == jump_size_min)//现在已经跳跃了足够多次
	// 				break;
	// 			startLri.part1 = tool::reverse_value(tool::reverse_value(leftLri.part1) + jump_size);//从下界移动
	// 		}
	// 	}

	// 	LOG_INFO("out start lri:{}.{}.{}", tool::reverse_value(outStartLri.lriType), tool::reverse_value(outStartLri.part1), tool::reverse_value(outStartLri.part2));
	// 	return 0;
	// }

// 	int64_t init_info_and_lris(int64_t timeOffset, db2LRI& outStartLri, db2LRI& outEndLri)
// 	{
// 		rc_ = init_read_log_struct();
// 		if (rc_ < 0) return rc_;

// 		{
// 			//装载end lri
// 			outEndLri.lriType = read_log_info_.initialLRI.lriType;
// 			outEndLri.part1 = (decltype(outEndLri.part1))-1;
// 			outEndLri.part2 = (decltype(outEndLri.part2))-1;

// 			tool::reverse_bytes(&outEndLri.part1);
// 			tool::reverse_bytes(&outEndLri.part2);
// 		}

// 		if (outStartLri.lriType || outStartLri.part1 || outStartLri.part2)//既然有值,在此结束
// 			return 0;

// 		LOG_DEBUG("start finding lri......., timeOffset:{}", timeOffset);

// 		//装载end lri
// 		outEndLri.lriType = read_log_info_.initialLRI.lriType;
// 		outEndLri.part1 = (decltype(outEndLri.part1))0xFFFFFFFFFFFF;
// 		outEndLri.part2 = (decltype(outEndLri.part2))-1;
// 		tool::reverse_bytes(&outEndLri.lriType); tool::reverse_bytes(&outEndLri.part1); tool::reverse_bytes(&outEndLri.part2);

// 		db2LRI currLri;
// 		//从db2来的数值进行大小比较，需翻转
// 		bool log_wrapped;
// 		if (tool::reverse_value(read_log_info_.initialLRI.part1) > tool::reverse_value(read_log_info_.nextStartLRI.part1)) {
// 			// memcpy(&currLri, &(read_log_info_.nextStartLRI), sizeof(currLri));//从db2来的又返回db2，不需要翻转
// 			memcpy(&currLri, &(read_log_info_.initialLRI), sizeof(currLri));//从db2来的又返回db2，不需要翻转
// 			LOG_DEBUG("log wrap around");
// 			log_wrapped = true;
// 		} else{ 
// 			memcpy(&currLri, &(read_log_info_.initialLRI), sizeof(currLri));//从db2来的又返回db2，不需要翻转
// 			LOG_DEBUG("normal log");
// 			log_wrapped = false;
// 		}

// 		//既然没有配置timeOffset,在此结束,使用init_read_log_struct查询出来的结果
// 		if (timeOffset < 0) {
// 			outStartLri = read_log_info_.initialLRI;
// 			return 0;
// 		}

// 		//下方开始进行lri跳转
// 		read_log_input_.iCallerAction = DB2READLOG_READ;
// 		read_log_input_.iLogBufferSize = log_buffer_.size();
// 		read_log_input_.iFilterOption = DB2READLOG_FILTER_ON;

// 		sqlca sqlca{};
// 		int64_t lri_time = get_time_of_lri(db2_version_, read_log_input_, read_log_info_, sqlca, outEndLri, currLri, log_buffer_.data());
// 		if (lri_time > 0 && timeOffset < lri_time) {
// 			outStartLri = currLri;
// 			return 0;
// 		}

// 		db2LRI leftLri, rightLri;
// 		bool first_part_searched = false;
// 		db2LRI initial_lri = read_log_info_.initialLRI;
// 		db2LRI result1 = {0}, result2 = {0};
// 		int64_t lri_time1 = 0, lri_time2 = 0;
// search_the_other_part:
// 		if (!first_part_searched) {
// 			if (log_wrapped) {
// 				leftLri = {0}, rightLri = initial_lri;
// 			} else {
// 				leftLri = initial_lri, rightLri = outEndLri;
// 			}
// 		} else {
// 			if (log_wrapped) {
// 				leftLri = initial_lri, rightLri = outEndLri;
// 			} else {
// 				leftLri = {0}, rightLri = initial_lri;
// 			}
// 		}
// 		while (tool::reverse_value(leftLri.part1) < tool::reverse_value(rightLri.part1)) {
// 			LOG_DEBUG("leftLri: {}.{}.{}", leftLri.lriType, leftLri.part1, leftLri.part2);
// 			LOG_DEBUG("rightLri: {}.{}.{}", rightLri.lriType, rightLri.part1, rightLri.part2);
// 			currLri.part1 = tool::reverse_value(tool::reverse_value(leftLri.part1) + (tool::reverse_value(rightLri.part1) - tool::reverse_value(leftLri.part1)) / 2);
// 			currLri.part2 = tool::reverse_value(tool::reverse_value(leftLri.part2) + (tool::reverse_value(rightLri.part2) - tool::reverse_value(leftLri.part2)) / 2);
// 			LOG_DEBUG("current lri:{}.{}.{}", currLri.lriType, currLri.part1, currLri.part2);

// 			lri_time = get_time_of_lri(db2_version_, read_log_input_, read_log_info_, sqlca, outEndLri, currLri, log_buffer_.data());
// 			LOG_DEBUG("lri time:{}", lri_time);

// 			//找不到有效日志或者lri日志被清理
// 			if (lri_time == 0 || lri_time == SQLU_RLOG_EXTENT_REQUIRED) {				
// 				rightLri = currLri;
// 				continue;
// 			}

// 			//lri越界
// 			if (lri_time == SQLU_RLOG_INVALID_PARM || lri_time < 0) {
// 				rightLri = currLri;
// 				continue;
// 			}

// 			LOG_DEBUG("firstReadLRI part1: {}, nextStartLRI part1: {}", read_log_info_.firstReadLRI.part1, read_log_info_.nextStartLRI.part1);

// 			// lri_time > 0, lri时间有意义
// 			if (lri_time == timeOffset) {
// 				outStartLri = read_log_info_.firstReadLRI;
// 				LOG_DEBUG("found lri_time == timeOffset");
// 				return 0;
// 			}
// 			if (lri_time < timeOffset) {
// 				if (leftLri.part1 == read_log_info_.firstReadLRI.part1 && leftLri.part2 == read_log_info_.firstReadLRI.part2) {
// 					LOG_DEBUG("lri_time < timeOffset and cannot move leftLri");
// 					break;
// 				}
// 				leftLri = read_log_info_.firstReadLRI;
// 				leftLri.part1 = tool::reverse_value(tool::reverse_value(leftLri.part1) + 1);
// 				leftLri.part2 = tool::reverse_value(tool::reverse_value(leftLri.part2) + 1);
// 			} else {
// 				if (rightLri.part1 == read_log_info_.firstReadLRI.part1 && rightLri.part2 == read_log_info_.firstReadLRI.part2) {
// 					LOG_DEBUG("lri_time > timeOffset and cannot move rightLri");
// 					break;
// 				}
// 				rightLri = read_log_info_.firstReadLRI;
// 			}
// 		}
// 		LOG_DEBUG("leftLri lri:{}.{}.{}", leftLri.lriType, leftLri.part1, leftLri.part2);
// 		LOG_DEBUG("rightLri lri:{}.{}.{}", rightLri.lriType, rightLri.part1, rightLri.part2);

// 		// db2LRI preLri = leftLri;
// 		// for (currLri = leftLri; tool::reverse_value(currLri.part1) < tool::reverse_value(rightLri.part1); )
// 		// {
// 		// 	LOG_DEBUG("currLri: {},{},{}", currLri.lriType, currLri.part1, currLri.part2);
// 		// 	lri_time = get_time_of_lri(db2_version_, read_log_input_, read_log_info_, sqlca, outEndLri, currLri, log_buffer_.data());
// 		// 	LOG_DEBUG("lri time:{}", lri_time);
// 		// 	if (lri_time < 0) {
// 		// 		currLri.part1 = tool::reverse_value(tool::reverse_value(currLri.part1) + 1);
// 		// 		currLri.part2 = tool::reverse_value(tool::reverse_value(currLri.part2) + 1);
// 		// 		continue;
// 		// 	}
// 		// 	if (lri_time == 0) {
// 		// 		LOG_DEBUG("lri_time should not be 0");
// 		// 		break;
// 		// 	}
// 		// 	if (lri_time == timeOffset) {
// 		// 		outStartLri = currLri;
// 		// 		return 0;
// 		// 	}
// 		// 	if (lri_time > timeOffset) {
// 		// 		break;
// 		// 	}
// 		// 	// lri_time < timeOffset
// 		// 	if (tool::reverse_value(currLri.part1) < tool::reverse_value(read_log_info_.firstReadLRI.part1)) {
// 		// 		currLri = read_log_info_.firstReadLRI;
// 		// 		// if (tool::reverse_value(preLri.part1) == tool::reverse_value(currLri.part1)) break;
// 		// 		preLri = currLri;
// 		// 	}
// 		// 	currLri.part1 = tool::reverse_value(tool::reverse_value(currLri.part1) + 1);
// 		// 	currLri.part2 = tool::reverse_value(tool::reverse_value(currLri.part2) + 1);
// 		// }

// 		if (!first_part_searched) {
// 			first_part_searched = true;
// 			result1 = currLri;
// 			lri_time1 = lri_time;
// 			LOG_DEBUG("lri_time1:{}", lri_time1);
// 			LOG_DEBUG("result1 lri:{}.{}.{}", result1.lriType, result1.part1, result1.part2);
// 			goto search_the_other_part;
// 		} else {
// 			result2 = currLri;
// 			lri_time2 = lri_time;
// 			LOG_DEBUG("lri_time2:{}", lri_time2);
// 			LOG_DEBUG("result2 lri:{}.{}.{}", result2.lriType, result2.part1, result2.part2);
// 		}

// 		if (lri_time1 > 0 && lri_time2 <= 0) {
// 			outStartLri = result1;
// 			LOG_DEBUG("using result1");
// 			return 0;
// 		}
// 		if (lri_time2 > 0 && lri_time1 <= 0) {
// 			outStartLri = result2;
// 			LOG_DEBUG("using result2");
// 			return 0;
// 		}
// 		if (lri_time1 > 0 && lri_time2 > 0) {
// 			int64_t delta1 = abs(timeOffset - lri_time1);
// 			int64_t delta2 = abs(timeOffset - lri_time2);
// 			if (delta1 < delta2) {
// 				outStartLri = result1;
// 				LOG_DEBUG("using result1");
// 				return 0;
// 			} else {
// 				outStartLri = result2;
// 				LOG_DEBUG("using result2");
// 				return 0;
// 			}
// 		}
// 		LOG_INFO("lri not found");
// 		outStartLri = {0};
// 		return -1;
// 	}

	typedef struct lri_and_time {
		db2LRI lri;
		int time;
	}lri_and_time;
	std::string encode_lri(const db2LRI& lri) {
		return to_string(lri.lriType) + "." + to_string(lri.part1) + "." + to_string(lri.part2);
	}
	void split_string(const string& str, const string& split, vector<string>& res)
	{
		char* strc = new char[str.size() + 1];
		strcpy(strc, str.c_str());   // 将str拷贝到 char类型的strc中
		char* temp = strtok(strc, split.c_str());
		while (temp != NULL)
		{
			res.push_back(string(temp));		
			temp = strtok(NULL, split.c_str());	// 下一个被分割的串
		}
		delete[] strc;
	}
	db2LRI decode_lri(const std::string& lri_str) {
		db2LRI lri{};
		if (lri_str.empty()) {
			return lri;
		}
		vector<string> res;
		split_string(lri_str.c_str(), ".", res);
		if (res.size() < 3) {
			return lri;
		}
		lri.lriType = atoi(res[0].c_str());
		lri.part1 = atoll(res[1].c_str());
		lri.part2 = atoll(res[2].c_str());
		return lri;
	}

	int64_t read_lri_sequentially(db2Uint32 versionNumber, db2ReadLogStruct& readLogInput, db2ReadLogInfoStruct& readLogInfo, struct sqlca& sqlca,
		db2LRI& endLRI, db2LRI& startLRI, vector<char>& logBuffer, std::vector<lri_and_time>& lri_and_time_vec)
	{
		readLogInput.iCallerAction = DB2READLOG_READ;
		readLogInput.poLogBuffer = logBuffer.data();
		readLogInput.piStartLRI = &startLRI;
		readLogInput.piEndLRI = &endLRI;
		readLogInput.poReadLogInfo = &readLogInfo;
		readLogInput.iLogBufferSize = logBuffer.size();
		readLogInput.iFilterOption = DB2READLOG_FILTER_ON;

		// readlog_mutex_.multi_process_mutex_lock();
		db2ReadLog(versionNumber, &readLogInput, &sqlca);
		// readlog_mutex_.multi_process_mutex_unlock();
		log_read_log_info(readLogInfo);

		if (sqlca.sqlcode != 0 && sqlca.sqlcode != SQLU_RLOG_READ_TO_CURRENT) {
			LOG_INFO("sql code:{}", sqlca.sqlcode);
			return sqlca.sqlcode;//SQLU_RLOG_INVALID_PARM表示越界，SQLU_RLOG_EXTENT_REQUIRED表示该日志已经被删除，SQLU_RLOG_READ_TO_CURRENT
		}

		sqluint32 numLogRecords = tool::reverse_value(readLogInfo.logRecsWritten);

		char* recordBuffer = readLogInput.poLogBuffer;
		sqluint32 recordSize;
		sqluint16 recordType;
		sqluint16 recordFlag;

		LOG_DEBUG("read lri sequentially numLogRecords: {}", numLogRecords);
		for (sqluint32 logRecordNb = 0; logRecordNb < numLogRecords; logRecordNb++)
		{
			db2ReadLogFilterData* filterData = (db2ReadLogFilterData*)recordBuffer;
			recordBuffer += sizeof(db2ReadLogFilterData);

			recordSize = tool::reverse_value(*(sqluint32*)(recordBuffer));
			recordType = tool::reverse_value(*(sqluint16*)(recordBuffer + sizeof(sqluint32)));
			recordFlag = tool::reverse_value(*(sqluint16*)(recordBuffer + sizeof(sqluint32) + sizeof(sqluint16)));

			int64_t result = get_commit_time(recordBuffer, recordType, recordFlag);
			if (result > 0) {
				LOG_DEBUG("get_commit_time:{}", result);
				if (lri_and_time_vec.size() == 0 || (lri_and_time_vec.size() > 0 && lri_and_time_vec.back().time + 60 < result)) {
					lri_and_time lat;
					lat.lri = filterData->recordLRIType1;
					lat.time = result;
					lri_and_time_vec.push_back(lat);
				}
			}
			recordBuffer += recordSize;
		}

		return 0;
	}

	// 从currLri顺序往前读
	void async_read_lri_forward(tapdata::LriRecorder& lri_recorder, db2LRI& currLri) {
		LOG_DEBUG("enter async read lri forward");
		// rc_ = init_read_log_struct();
		// if (rc_ < 0) return ;

		sqlca sqlca{};
		int step = 20000;
		db2LRI beginLri = {0};
		db2LRI endLri = currLri;
		while(1) {
			beginLri = endLri;
			endLri.part1 = tool::reverse_value((tool::reverse_value(endLri.part1) + step) % 0xFFFFFFFFFFFF);
			endLri.part2 = tool::reverse_value((tool::reverse_value(endLri.part2) + 2*step) % 0xFFFFFFFFFFFFFFFF);
			log_lri("async read lri forward, beginLri", beginLri);
			log_lri("async read lri forward, endLri", endLri);
retry:
			std::vector<lri_and_time> lri_and_time_vec;
			int ret = read_lri_sequentially(db2_version_, read_log_input_, read_log_info_, sqlca, endLri, beginLri, log_buffer_, lri_and_time_vec);
			if (ret < 0) {
				LOG_DEBUG("async read lri forward, no record");
				sleep(60);
				goto retry;
			}
			if (lri_and_time_vec.size() == 0) {
				LOG_DEBUG("async read lri forward, no lri and time");
				sleep(10);
				continue;
			}
			for (auto &&lri_and_time : lri_and_time_vec) {
				lri_recorder.Insert(encode_lri(lri_and_time.lri), lri_and_time.time);
			}
			if (sqlca.sqlcode == SQLU_RLOG_READ_TO_CURRENT) {
				LOG_DEBUG("async read lri forward, read to current");
				sleep(60);
				goto retry;
			}
			// endLri = lri_and_time_vec.back().lri;
		}
	}

	int64_t init_read_log_struct_backward(db2ReadLogStruct& read_log_input, db2ReadLogInfoStruct& read_log_info)
	{
		db2ReadLogStruct t{};
		std::swap(read_log_input, t);

		read_log_input.iCallerAction = DB2READLOG_QUERY;
		read_log_input.piStartLRI = NULL;
		read_log_input.piEndLRI = NULL;
		read_log_input.poLogBuffer = NULL;
		read_log_input.iLogBufferSize = 0;

		db2ReadLogInfoStruct tt{};
		std::swap(read_log_info, tt);

		read_log_input.iFilterOption = DB2READLOG_FILTER_ON;
		read_log_input.poReadLogInfo = &read_log_info;
		read_log_input.piMinRequiredLRI = NULL;

		sqlca sqlca = { 0 };

		// readlog_mutex_.multi_process_mutex_lock();
		int rc = db2ReadLog(db2_version_, &read_log_input, &sqlca);
		// readlog_mutex_.multi_process_mutex_unlock();
		log_read_log_info(read_log_info);
		LOG_DEBUG("db2ReadLog rc:{}", rc);
		EXPECTED_ERR_CHECK("database log info -- get");
		if (sqlca.sqlcode < 0)
			return sqlca.sqlcode;
		return 0;
	}

	// 从currLri顺序往后读
	void async_read_lri_backward(tapdata::LriRecorder& lri_recorder, 
								db2LRI& currLri, 
								db2ReadLogStruct& read_log_input, 
								db2ReadLogInfoStruct& read_log_info,
								vector<char>& log_buffer) {
		LOG_DEBUG("enter async read lri backward");
		int rc = init_read_log_struct_backward(read_log_input, read_log_info);
		if (rc < 0) {
			LOG_ERROR("init read log struct backward failed, rc: {}", rc);
			return ;
		}

		sqlca sqlca{};
		int step = 20000;
		db2LRI beginLri;
		db2LRI endLri = read_log_info.initialLRI;
		while(tool::reverse_value(endLri.part1) < tool::reverse_value(currLri.part1)) {
			beginLri = endLri;
			// if (tool::reverse_value(endLri.part1) <= (long unsigned int)step) {
			// 	LOG_INFO("async read lri backward end");
			// 	return ;
			// }
			endLri.part1 = tool::reverse_value((tool::reverse_value(endLri.part1) + step) % 0xFFFFFFFFFFFF);
			endLri.part2 = tool::reverse_value((tool::reverse_value(endLri.part2) + 2*step) % 0xFFFFFFFFFFFFFFFF);
			log_lri("async read lri backward, beginLri", beginLri);
			log_lri("async read lri backward, endLri", endLri);
			std::vector<lri_and_time> lri_and_time_vec;
			int ret = read_lri_sequentially(db2_version_, read_log_input, read_log_info, sqlca, endLri, beginLri, log_buffer, lri_and_time_vec);
			if (ret < 0) {
				LOG_DEBUG("async read lri backward, no record");
				// return ;
				// sleep(1);
				continue;
			}
			if (lri_and_time_vec.size() == 0) {
				LOG_DEBUG("async read lri backward, no lri and time");
				// return ;
				// sleep(1);
				continue;
			}
			for (auto &&lri_and_time : lri_and_time_vec) {
				lri_recorder.Insert(encode_lri(lri_and_time.lri), lri_and_time.time);
			}
			if (sqlca.sqlcode == SQLU_RLOG_READ_TO_CURRENT) {
				LOG_DEBUG("async read lri backward, read to current");
				return ;
			}
			// endLri = lri_and_time_vec.back().lri;
		}
		LOG_DEBUG("async read lri backward finish");
	}

// 	// forwardDirection=true，从startLri向前读；forwardDirection=false，从startLri向后读
// 	void async_read_lri_forward(tapdata::LriRecorder& lri_recorder, db2LRI& startLri) {
// 		// rc_ = init_read_log_struct();
// 		// if (rc_ < 0) return ;

// 		sqlca sqlca{};
// 		int step = 10000;
// 		db2LRI beginLRI;
// 		db2LRI endLRI = startLri;
// 		while(1) {
// 			beginLRI = endLRI;
// 			endLRI.part1 = tool::reverse_value((tool::reverse_value(endLRI.part1) + step) % 0xFFFFFFFFFFFF);
// 			endLRI.part2 = tool::reverse_value((tool::reverse_value(endLRI.part2) + 2*step) % 0xFFFFFFFFFFFFFFFF);
// 			log_lri("beginLRI", beginLRI);
// 			log_lri("endLRI", endLRI);
// 			std::vector<lri_and_time> lri_and_time_vec;
// retry:
// 			int ret = async_read_lri_sequentially1(db2_version_, read_log_input_, read_log_info_, sqlca, endLRI, beginLRI, log_buffer_.data(), lri_and_time_vec);
// 			if (ret < 0 || ret == SQLU_RLOG_READ_TO_CURRENT) {
// 				LOG_DEBUG("no record, wait 10s");
// 				sleep(10);
// 				goto retry;
// 			}
// 			if (lri_and_time_vec.size() == 0) {
// 				LOG_DEBUG("no record found");
// 				sleep(1);
// 				continue;
// 			}
// 			// todo: record lri and time
// 			for (auto &&lri_and_time : lri_and_time_vec) {
// 				lri_recorder.Insert(encode_lri(lri_and_time.lri), lri_and_time.time);
// 			}
// 			beginLRI = lri_and_time_vec.back().lri;
// 		}
// 	}

// 	void async_read_lri_backward(tapdata::LriRecorder& lri_recorder, db2LRI& startLri) {
// 		// rc_ = init_read_log_struct();
// 		// if (rc_ < 0) return ;

// 		sqlca sqlca{};
// 		int step = 10000;
// 		db2LRI beginLRI = startLri;
// 		db2LRI endLRI;
// 		while(1) {
// 			endLRI = beginLRI;
// 			beginLRI.part1 = tool::reverse_value((tool::reverse_value(beginLRI.part1) - step) % 0xFFFFFFFFFFFF);
// 			beginLRI.part2 = tool::reverse_value((tool::reverse_value(beginLRI.part2) - 2*step) % 0xFFFFFFFFFFFFFFFF);
// 			log_lri("beginLRI", beginLRI);
// 			log_lri("endLRI", endLRI);
// 			std::vector<lri_and_time> lri_and_time_vec;
// retry:
// 			int ret = async_read_lri_sequentially1(db2_version_, read_log_input_, read_log_info_, sqlca, endLRI, beginLRI, log_buffer_.data(), lri_and_time_vec);
// 			if (ret < 0 || ret == SQLU_RLOG_READ_TO_CURRENT) {
// 				LOG_DEBUG("no record, wait 10s");
// 				sleep(10);
// 				goto retry;
// 			}
// 			if (lri_and_time_vec.size() == 0) {
// 				LOG_DEBUG("no record found");
// 				sleep(1);
// 				continue;
// 			}
// 			// todo: record lri and time
// 			for (auto &&lri_and_time : lri_and_time_vec) {
// 				lri_recorder.Insert(encode_lri(lri_and_time.lri), lri_and_time.time);
// 			}
// 			endLRI = lri_and_time_vec.front().lri;
// 		}
// 	}

	int get_step(int sqlcode) {
		const int init_step = 20000;
		const int step_limit = 100000;

		if (sqlcode == 0) return init_step;

		static int step = init_step;
		if (sqlcode == SQLU_RLOG_INVALID_PARM) {
			if (step >= step_limit) return step_limit;

			step *= 2;
			if (step <= step_limit) {
				return step;
			}

			step /= 2;
			step += init_step;
			return step < step_limit ? step : step_limit;
		}

		return init_step;
	}

	int64_t accelerate_by_part2(db2LRI& outStartLri) {
		int64_t delta = (int64_t)tool::reverse_value(outStartLri.part2) - (int64_t)tool::reverse_value(read_log_info_.initialLRI.part2);
		if (delta == 0) {
			outStartLri = read_log_info_.initialLRI;
			return 0;
		}
		LOG_DEBUG("lri part2 delta: {}", delta);
		int step = get_step(0);
		db2LRI tmpEndLRI = outStartLri;
		// set part1 upper boundary: (part2 delta) / 2, because at least on DML before COMMIT statement
		tmpEndLRI.part1 = tool::reverse_value((int64_t)tool::reverse_value(read_log_info_.initialLRI.part1) + delta/2);
		// initialLRI is the search lower boundary
		outStartLri.part1 = read_log_info_.initialLRI.part1;
		// delta is too large, narrow the search range to accelerate search
		if (delta > step) {
			outStartLri.part1 = tool::reverse_value(tool::reverse_value(tmpEndLRI.part1) - step);
			// outStartLri.part2 = tool::reverse_value(tool::reverse_value(tmpEndLRI.part2) - 2*step);
		}

		read_log_input_.iCallerAction = DB2READLOG_READ;
		read_log_input_.poLogBuffer = log_buffer_.data();
		read_log_input_.piStartLRI = &outStartLri;
		read_log_input_.piEndLRI = &tmpEndLRI;
		read_log_input_.poReadLogInfo = &read_log_info_;
		read_log_input_.iLogBufferSize = log_buffer_.size();
		read_log_input_.iFilterOption = DB2READLOG_FILTER_ON;
		int ret = -1;
		int round = 0;
		sqlca sqlca{};
		while(1) {
			round++;
			LOG_DEBUG("round:{}", round);
			log_lri("outStartLri", outStartLri);
			log_lri("tmpEndLRI", tmpEndLRI);
			db2LRI preStartLri = outStartLri;
			ret = accelerate_find(db2_version_, read_log_input_, read_log_info_, sqlca, tmpEndLRI, outStartLri, log_buffer_.data());
			if (ret == 0) {
				log_lri("found lri, outStartLri", outStartLri);
				return 0;
			}
			if (ret < 0) {
				if (outStartLri.part1 == preStartLri.part1) {
					LOG_DEBUG("outStartLri has no change, break");
					break;
				}
				LOG_DEBUG("next search right side");
				continue;
			}
			LOG_DEBUG("next search left side");
			tmpEndLRI.part1 = outStartLri.part1;
			step = get_step(sqlca.sqlcode);
			if ((unsigned long long)step > tool::reverse_value(outStartLri.part1)) {
				step = tool::reverse_value(outStartLri.part1) > 10000 ? 10000 : tool::reverse_value(outStartLri.part1);
			}
			outStartLri.part1 = tool::reverse_value(tool::reverse_value(outStartLri.part1) - step);
			// outStartLri.part2 = tool::reverse_value(tool::reverse_value(outStartLri.part2) - 2*step);
			if (tool::reverse_value(outStartLri.part1) < tool::reverse_value(read_log_info_.initialLRI.part1)) {
				outStartLri.part1 = read_log_info_.initialLRI.part1;
			}
			if (tool::reverse_value(outStartLri.part1) >= tool::reverse_value(tmpEndLRI.part1)) {
				LOG_DEBUG("found lri");
				log_lri("outStartLri", outStartLri);
				log_lri("tmpEndLRI", tmpEndLRI);
				outStartLri = tmpEndLRI;
				return 0;
			}
		}
		LOG_DEBUG("not found lri");
		log_lri("outStartLri", outStartLri);
		log_lri("tmpEndLRI", tmpEndLRI);
		return 0;
	}

	int64_t init_info_and_lris(ReadLogWrap& rlw)
	{
		int64_t timeOffset = rlw.time_off_set();
		db2LRI& outStartLri = rlw.get_current_lri();
		db2LRI& outEndLri = rlw.get_end_lri_();
		std::string lri_record_name = rlw.lri_record_name();

		log_lri("outStartLri", outStartLri);
		if (outStartLri.part1 == 0) {
			outStartLri.lriType = 1;
			tool::reverse_bytes(&outStartLri.lriType);
			tool::reverse_bytes(&outStartLri.part1);
			tool::reverse_bytes(&outStartLri.part2);
			log_lri("reversed outStartLri", outStartLri);
		}
		rc_ = init_read_log_struct();
		if (rc_ < 0) return rc_;

		{
			//装载end lri
			outEndLri.lriType = read_log_info_.initialLRI.lriType;
			outEndLri.part1 = (decltype(outEndLri.part1))0xFFFFFFFFFFFF;
			outEndLri.part2 = (decltype(outEndLri.part2))-1;

			tool::reverse_bytes(&outEndLri.part1);
			tool::reverse_bytes(&outEndLri.part2);
		}
		LOG_DEBUG("outEndLri: {}.{}.{}", tool::reverse_value(outEndLri.lriType), tool::reverse_value(outEndLri.part1), tool::reverse_value(outEndLri.part2));
		LOG_DEBUG("read_log_info_.initialLRI: {}.{}.{}", tool::reverse_value(read_log_info_.initialLRI.lriType), tool::reverse_value(read_log_info_.initialLRI.part1), tool::reverse_value(read_log_info_.initialLRI.part2));

		if (outStartLri.lriType && outStartLri.part1 && outStartLri.part2)//既然有值,在此结束
			return 0;

		sqlca sqlca{};
		if (outStartLri.lriType && !outStartLri.part1 && outStartLri.part2) {
			std::string recordlri_lock_name = "/lockfiles/" + lri_record_name;
			mutex_wrapper recordlri_mutex(recordlri_lock_name);
			if (rlw.cache_lri() || cache_switch) {
				int rc = recordlri_mutex.init_multi_process_mutex();
				if (rc < 0) return rc;
				rc = recordlri_mutex.multi_process_mutex_trylock();
				if (rc > 0) {
					// 已经有其他进程在记录lri cache了	
					LOG_DEBUG("other process is recording lri cache");				
					return 0;
				}
				LOG_DEBUG("now got the lock");
			}

			auto rc = accelerate_by_part2(outStartLri);
			if (rc < 0) return rc;

			if (rlw.cache_lri() || cache_switch) {
				tapdata::LriRecorder lri_recorder(lri_record_name);
				int rc = lri_recorder.OpenDatabase();
				if (rc != 0) {
					exit(rc);
				}
				rc = lri_recorder.CreateTable();
				if (rc != 0) {
					exit(rc);
				}
				// 先记录下来，防止被forward修改。
				db2LRI tmp = outStartLri;
				std::thread readlri_backward([=]{
					db2LRI outStartLriBackward = tmp;
					tapdata::LriRecorder lri_recorder_backward(lri_record_name);
					int rc = lri_recorder_backward.OpenDatabase();
					if (rc != 0) {
						exit(rc);
					}
					rc = lri_recorder_backward.CreateTable();
					if (rc != 0) {
						exit(rc);
					}
					vector<char> log_buffer;
					LOG_DEBUG("##############: {}", log_buffer_.size());
					log_buffer.resize(log_buffer_.size());
					db2ReadLogStruct read_log_input;
					db2ReadLogInfoStruct read_log_info;
					async_read_lri_backward(lri_recorder_backward, outStartLriBackward, read_log_input, read_log_info, log_buffer);
				});
				readlri_backward.detach();
				async_read_lri_forward(lri_recorder, outStartLri);
				recordlri_mutex.multi_process_mutex_unlock();
				// recordlri_mutex.destroy_multi_process_mutex();
			}
			return 0;
		}

		LOG_DEBUG("start finding lri......., timeOffset:{}", timeOffset);

		db2LRI currLri;
		//从db2来的数值进行大小比较，需翻转
		bool log_wrapped;
		if (tool::reverse_value(read_log_info_.initialLRI.part1) > tool::reverse_value(read_log_info_.nextStartLRI.part1)) {
			// memcpy(&currLri, &(read_log_info_.nextStartLRI), sizeof(currLri));//从db2来的又返回db2，不需要翻转
			memcpy(&currLri, &(read_log_info_.initialLRI), sizeof(currLri));//从db2来的又返回db2，不需要翻转
			LOG_DEBUG("log wrap around");
			log_wrapped = true;
		} else{ 
			memcpy(&currLri, &(read_log_info_.initialLRI), sizeof(currLri));//从db2来的又返回db2，不需要翻转
			LOG_DEBUG("normal log");
			log_wrapped = false;
		}

		//既然没有配置timeOffset,在此结束,使用init_read_log_struct查询出来的结果
		if (timeOffset < 0) {
			outStartLri = read_log_info_.initialLRI;
			return 0;
		}

		// 先去sqlite3查询
		if (cache_switch) {
			tapdata::LriRecorder lri_recorder(lri_record_name);
			int rc = lri_recorder.OpenDatabase();
			if (rc != 0) {
				return rc;
			}
			string lri_record;
			int query_time = timeOffset;
			rc = lri_recorder.Query(lri_record, query_time);
			LOG_DEBUG("rc:{}, lri:{}, query_time:{}", rc, lri_record, query_time);
			if (rc == 0 && timeOffset - query_time <= 300) {
				outStartLri = decode_lri(lri_record);
				lri_recorder.Close();
				return 0;
			}
			lri_recorder.Close();
		}

		//下方开始进行lri跳转
		read_log_input_.iCallerAction = DB2READLOG_READ;
		read_log_input_.iLogBufferSize = log_buffer_.size();
		read_log_input_.iFilterOption = DB2READLOG_FILTER_ON;

		int64_t lri_time = 0;
		db2LRI leftLri, rightLri;
		bool first_part_searched = false;
		db2LRI initial_lri = read_log_info_.initialLRI;
		db2LRI result1 = {0}, result2 = {0};
		int64_t lri_time1 = 0, lri_time2 = 0;
//search_the_other_part:
		if (!first_part_searched) {
			if (log_wrapped) {
				leftLri = {1, 0, 0}, rightLri = initial_lri;
				tool::reverse_bytes(&leftLri.lriType);
			} else {
				leftLri = initial_lri, rightLri = outEndLri;
			}
		} else {
			if (log_wrapped) {
				leftLri = initial_lri, rightLri = outEndLri;
			} else {
				leftLri = {1, 0, 0}, rightLri = initial_lri;
				tool::reverse_bytes(&leftLri.lriType);
			}
		}

		bool valid_lri_time = false;
		db2LRI pre_lri = leftLri;
		int64_t pre_lri_time = 0;
		while (tool::reverse_value(leftLri.part1) < tool::reverse_value(rightLri.part1)) {
			LOG_DEBUG();
			LOG_DEBUG("leftLri: {}.{}.{}", tool::reverse_value(leftLri.lriType), tool::reverse_value(leftLri.part1), tool::reverse_value(leftLri.part2));
			LOG_DEBUG("rightLri: {}.{}.{}", tool::reverse_value(rightLri.lriType), tool::reverse_value(rightLri.part1), tool::reverse_value(rightLri.part2));
			currLri.part1 = tool::reverse_value(tool::reverse_value(leftLri.part1) + (tool::reverse_value(rightLri.part1) - tool::reverse_value(leftLri.part1)) / 2);
			currLri.part2 = tool::reverse_value(tool::reverse_value(leftLri.part2) + (tool::reverse_value(rightLri.part2) - tool::reverse_value(leftLri.part2)) / 2);
			LOG_DEBUG("current lri:{}.{}.{}", tool::reverse_value(currLri.lriType), tool::reverse_value(currLri.part1), tool::reverse_value(currLri.part2));

			lri_time = get_time_of_lri(db2_version_, read_log_input_, read_log_info_, sqlca, rightLri, currLri, log_buffer_.data(), timeOffset);
			LOG_DEBUG("lri time:{}", lri_time);
			if (lri_time == -99999999) {
				break;
			}

			//找不到有效日志或者lri日志被清理
			if (lri_time <= 0 || lri_time == SQLU_RLOG_EXTENT_REQUIRED) {
				if (valid_lri_time) {
					// 如果之前lri有效，但是本轮lri无效，则使用之前的lri并立即break。
					LOG_DEBUG("using pre_lri: {}.{}.{}", tool::reverse_value(pre_lri.lriType), tool::reverse_value(pre_lri.part1), tool::reverse_value(pre_lri.part2));
					currLri = pre_lri;
					lri_time = pre_lri_time;
					break;
				}
				LOG_DEBUG("lri_time <= 0 and no valid lri time previously, shrink right boundary");
				rightLri = currLri;
				continue;
			}

			LOG_DEBUG("firstReadLRI part1: {}, nextStartLRI part1: {}", tool::reverse_value(read_log_info_.firstReadLRI.part1), tool::reverse_value(read_log_info_.nextStartLRI.part1));

			valid_lri_time = true;
			pre_lri = currLri;
			pre_lri_time = lri_time;
			// lri_time > 0, lri时间有意义
			if (lri_time <= timeOffset && timeOffset - lri_time <= 300) {
				outStartLri = currLri;
				LOG_DEBUG("Approximately found lri_time.");
				return 0;
			}

			if (lri_time < timeOffset) {
				if (leftLri.part1 == read_log_info_.firstReadLRI.part1 && leftLri.part2 == read_log_info_.firstReadLRI.part2) {
					LOG_DEBUG("lri_time < timeOffset and cannot move leftLri");
					// break;	
				}
				leftLri = currLri;
				leftLri.part1 = tool::reverse_value(tool::reverse_value(leftLri.part1) + 1);
				leftLri.part2 = tool::reverse_value(tool::reverse_value(leftLri.part2) + 1);
			} else {
				if (rightLri.part1 == read_log_info_.firstReadLRI.part1 && rightLri.part2 == read_log_info_.firstReadLRI.part2) {
					LOG_DEBUG("lri_time > timeOffset and cannot move rightLri");
					// break;
				}
				rightLri = currLri;
				if (tool::reverse_value(rightLri.part1) < 1 || tool::reverse_value(rightLri.part2) < 1) {
					break;
				}
				rightLri.part1 = tool::reverse_value(tool::reverse_value(rightLri.part1) - 1);
				rightLri.part2 = tool::reverse_value(tool::reverse_value(rightLri.part2) - 1);
			}
		}
		LOG_DEBUG("leftLri lri:{}.{}.{}", tool::reverse_value(leftLri.lriType), tool::reverse_value(leftLri.part1), tool::reverse_value(leftLri.part2));
		LOG_DEBUG("rightLri lri:{}.{}.{}", tool::reverse_value(rightLri.lriType), tool::reverse_value(rightLri.part1), tool::reverse_value(rightLri.part2));

		if (!first_part_searched) {
			first_part_searched = true;
			result1 = currLri;
			lri_time1 = lri_time;
			LOG_DEBUG("lri_time1:{}", lri_time1);
			LOG_DEBUG("result1 lri:{}.{}.{}", tool::reverse_value(result1.lriType), tool::reverse_value(result1.part1), tool::reverse_value(result1.part2));
			//goto search_the_other_part;
		} else {
			result2 = currLri;
			lri_time2 = lri_time;
			LOG_DEBUG("lri_time2:{}", lri_time2);
			LOG_DEBUG("result2 lri:{}.{}.{}", tool::reverse_value(result2.lriType), tool::reverse_value(result2.part1), tool::reverse_value(result2.part2));
		}

		if (lri_time1 > 0 && lri_time2 <= 0) {
			outStartLri = result1;
			LOG_DEBUG("using result1");
			return 0;
		}
		if (lri_time2 > 0 && lri_time1 <= 0) {
			outStartLri = result2;
			LOG_DEBUG("using result2");
			return 0;
		}
		if (lri_time1 > 0 && lri_time2 > 0) {
			int64_t delta1 = abs(timeOffset - lri_time1);
			int64_t delta2 = abs(timeOffset - lri_time2);
			if (delta1 < delta2) {
				outStartLri = result1;
				LOG_DEBUG("using result1");
				return 0;
			} else {
				outStartLri = result2;
				LOG_DEBUG("using result2");
				return 0;
			}
		}
		LOG_INFO("lri not found");
		outStartLri = read_log_info_.initialLRI;;
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
	static int64_t get_closest_commit_time(char* logBuffer, sqluint32 numLogRecords, int64_t timeOffset, db2LRI& startLRI)
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

		LOG_DEBUG("get closest commit time numLogRecords: {}", numLogRecords);
		int64_t preTime = 0;
		db2LRI preLRI = startLRI;
		for (sqluint32 logRecordNb = 0; logRecordNb < numLogRecords; logRecordNb++)
		{
			db2ReadLogFilterData* filterData = (db2ReadLogFilterData*)recordBuffer;
			recordBuffer += sizeof(db2ReadLogFilterData);

			recordSize = tool::reverse_value(*(sqluint32*)(recordBuffer));
			recordType = tool::reverse_value(*(sqluint16*)(recordBuffer + sizeof(sqluint32)));
			recordFlag = tool::reverse_value(*(sqluint16*)(recordBuffer + sizeof(sqluint32) + sizeof(sqluint16)));

			result = get_commit_time(recordBuffer, recordType, recordFlag);
			if (result > 0) {
				// LOG_DEBUG("get closest commit time result: {}", result);
				if (timeOffset < result) {
					if (preTime == 0) {
						return result;
					}
					startLRI = preLRI;
					return preTime;
				}
				if (timeOffset == result) {
					startLRI = filterData->recordLRIType1;
					return result;
				}
				preTime = result;
				preLRI = filterData->recordLRIType1;
			}
			recordBuffer += recordSize;
		}

		LOG_DEBUG("get closest commit time: {}", preTime);
		startLRI = preLRI;
		return preTime;
	}

	// return -1: current search is less; 0: found; 1: current search is bigger
	int64_t find_exactly_part1(char* logBuffer, sqluint32 numLogRecords, db2LRI& startLRI, db2LRI& endLRI)
	{
		int64_t result = 0;
		if (!logBuffer)
		{
			LOG_DEBUG("logBuffer is NULL");
			if (numLogRecords == 0)
				return result = 0;
			else
				return result = -1;
		}

		char* recordBuffer = logBuffer;
		sqluint32 recordSize;

		LOG_DEBUG("find_exactly_part1 numLogRecords: {}", numLogRecords);
		for (sqluint32 logRecordNb = 0; logRecordNb < numLogRecords; logRecordNb++)
		{
			db2ReadLogFilterData* filterData = (db2ReadLogFilterData*)recordBuffer;
			recordBuffer += sizeof(db2ReadLogFilterData);

			recordSize = tool::reverse_value(*(sqluint32*)(recordBuffer));

			log_lri("walk through lri", filterData->recordLRIType1);
			if (tool::reverse_value(filterData->recordLRIType1.part2) == tool::reverse_value(endLRI.part2)) {
				startLRI = filterData->recordLRIType1;
				LOG_DEBUG("find_exactly_part1 equal");
				return 0;
			}
			if (tool::reverse_value(filterData->recordLRIType1.part2) > tool::reverse_value(endLRI.part2)) {
				LOG_DEBUG("find_exactly_part1 bigger");
				return 1;
			}

			startLRI = filterData->recordLRIType1;
			recordBuffer += recordSize;
		}
		if (numLogRecords == 0) {
			LOG_DEBUG("no record, end early");
			return 0;
		}

		LOG_DEBUG("find_exactly_part1 smaller");
		return -1;
	}

	static void db2readlog_sig_term_proc(int sig_no) {
			// readlog_mutex_.multi_process_mutex_unlock();
			pthread_exit(NULL);
	}
	//返回值：0表示时间无效，小于0中SQLU_RLOG_INVALID_PARM表示日志超出越界，SQLU_RLOG_EXTENT_REQUIRED表示该日志被清理，大于0表示lri时间
	static int64_t get_time_of_lri_multithread(db2Uint32 versionNumber, db2ReadLogStruct& readLogInput, db2ReadLogInfoStruct& readLogInfo, struct sqlca& sqlca,
		db2LRI& endLRI, db2LRI& startLRI, char* logBuffer, int64_t timeOffset)
	{
		readLogInput.poLogBuffer = logBuffer;
		readLogInput.piStartLRI = &startLRI;
		readLogInput.piEndLRI = &endLRI;
		readLogInput.poReadLogInfo = &readLogInfo;

		std::mutex mutex_wait;                 // 互斥锁
		std::condition_variable cond_wait;     // 条件变量

		std::thread thread_readlog([&]{
			signal(SIGTERM, db2readlog_sig_term_proc);
			// readlog_mutex_.multi_process_mutex_lock();
			db2ReadLog(versionNumber, &readLogInput, &sqlca);
			// readlog_mutex_.multi_process_mutex_unlock();
			log_read_log_info(readLogInfo);
			cond_wait.notify_one();
		});

		std::unique_lock<std::mutex> lockWait(mutex_wait);
		std::cv_status cvsts = cond_wait.wait_for(lockWait, std::chrono::seconds(3600));

		// 消息接收超时
		if (cvsts == std::cv_status::timeout) {
			LOG_WARN("db2ReadLog timeout");
			pthread_t id = thread_readlog.native_handle();
			LOG_DEBUG("db2ReadLog pthread id: {}", id)
			// thread_readlog.detach();
			pthread_kill(id, SIGTERM);
			// pthread_cancel(id);
			return -99999999;
		}
		// 接收到条件变量信号，未超时

		if (thread_readlog.joinable())
			thread_readlog.join();

		// readlog_mutex_.multi_process_mutex_lock();
		// db2ReadLog(versionNumber, &readLogInput, &sqlca);
		// readlog_mutex_.multi_process_mutex_unlock();
		// log_read_log_info(readLogInfo);

		if (sqlca.sqlcode < 0) {
			return sqlca.sqlcode;
		}

		else if (sqlca.sqlcode == 0 || sqlca.sqlcode == SQLU_RLOG_READ_TO_CURRENT)
		{
			return get_closest_commit_time(logBuffer, tool::reverse_value(readLogInfo.logRecsWritten), timeOffset, startLRI);
		}
		else
		{
			LOG_INFO("sql code:{}", sqlca.sqlcode);
			return sqlca.sqlcode;//SQLU_RLOG_INVALID_PARM表示越界，SQLU_RLOG_EXTENT_REQUIRED表示该日志已经被删除
		}
	}
	int64_t get_time_of_lri(db2Uint32 versionNumber, db2ReadLogStruct& readLogInput, db2ReadLogInfoStruct& readLogInfo, struct sqlca& sqlca,
		db2LRI& endLRI, db2LRI& startLRI, char* logBuffer, int64_t timeOffset)
	{
		readLogInput.poLogBuffer = logBuffer;
		readLogInput.piStartLRI = &startLRI;
		readLogInput.piEndLRI = &endLRI;
		readLogInput.poReadLogInfo = &readLogInfo;

		// readlog_mutex_.multi_process_mutex_lock();
		db2ReadLog(versionNumber, &readLogInput, &sqlca);
		// readlog_mutex_.multi_process_mutex_unlock();
		log_read_log_info(readLogInfo);

		if (sqlca.sqlcode < 0) {
			return sqlca.sqlcode;
		}

		else if (sqlca.sqlcode == 0 || sqlca.sqlcode == SQLU_RLOG_READ_TO_CURRENT)
		{
			return get_closest_commit_time(logBuffer, tool::reverse_value(readLogInfo.logRecsWritten), timeOffset, startLRI);
		}
		else
		{
			LOG_INFO("sql code:{}", sqlca.sqlcode);
			return sqlca.sqlcode;//SQLU_RLOG_INVALID_PARM表示越界，SQLU_RLOG_EXTENT_REQUIRED表示该日志已经被删除
		}
	}

	int64_t accelerate_find(db2Uint32 versionNumber, db2ReadLogStruct& readLogInput, db2ReadLogInfoStruct& readLogInfo, struct sqlca& sqlca, db2LRI& endLRI, db2LRI& startLRI, char* logBuffer)
	{
		// readlog_mutex_.multi_process_mutex_lock();
		db2ReadLog(versionNumber, &readLogInput, &sqlca);
		// readlog_mutex_.multi_process_mutex_unlock();
		log_read_log_info(readLogInfo);

		if (sqlca.sqlcode != 0) {
			LOG_DEBUG("sqlca.sqlcode:{}", sqlca.sqlcode);
		}

		// -2650在这里一般是指定的起始lri太大
		if (sqlca.sqlcode == SQLU_RLOG_INVALID_PARM) { // || sqlca.sqlcode == SQLU_DBSYSTEM_ERROR
			LOG_DEBUG("input parameter is too big");
			return 1;
		}
		// SQLU_RLOG_EXTENT_REQUIRED 表示该日志已经被删除
		if (sqlca.sqlcode != 0 && sqlca.sqlcode != SQLU_RLOG_READ_TO_CURRENT) {
			LOG_DEBUG("some error occured in accelerate_find, sqlca.sqlcode:{}", sqlca.sqlcode);
			// 暂时处理成太大吧
			return 1;
		}
		if (tool::reverse_value(readLogInfo.logRecsWritten) == 0) {
			LOG_DEBUG("none log records, end early");
			if (sqlca.sqlcode == SQLU_RLOG_READ_TO_CURRENT) {
				// 由于后面可能会报-2650，所以这里把起始lri减一个很小的值。
				startLRI.part1 = tool::reverse_value(tool::reverse_value(startLRI.part1) - 100);
				startLRI.part2 = tool::reverse_value(tool::reverse_value(startLRI.part2) - 100);
			}
			return 0;
		}

		// have read the log records
		// db2LRI preStartLRI = startLRI;
		int ret = find_exactly_part1(logBuffer, tool::reverse_value(readLogInfo.logRecsWritten), startLRI, endLRI);
		// if (ret >= 0) {
		// 	LOG_DEBUG("the lri in the search range is greater than or equal to the target lri");
		// }
		LOG_DEBUG("find_exactly_part1 return: {}", ret);
		if (sqlca.sqlcode == SQLU_RLOG_READ_TO_CURRENT) {
			LOG_DEBUG("found the current lri");
			return 0;
		}
		return ret;
		// log_lri("preStartLRI", preStartLRI);
		// log_lri("startLRI", startLRI);
		// if (sqlca.sqlcode == 0 && startLRI.part1 != preStartLRI.part1) {
		// 	LOG_DEBUG("maybe not read out all the records in the search range, read recursively");
		// 	return accelerate_find(versionNumber, readLogInput, readLogInfo, sqlca, endLRI, startLRI, logBuffer);
		// }
		// LOG_DEBUG("read error or startLRI no change, end early");
		// return 0;
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

		// readlog_mutex_.multi_process_mutex_lock();
		rc_ = db2ReadLog(db2_version_, &read_log_input_, &sqlca);
		// readlog_mutex_.multi_process_mutex_unlock();
		log_read_log_info(read_log_info_);
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


