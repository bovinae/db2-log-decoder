#include <string>
#include <memory>
#include <chrono>

#include "sqladef.h"
#include "DB2ReadLogApp.h"
#include "tool_extern.h"
#include "log_imp.h"
#include "lri_recorder.h"

bool cache_switch = true;

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
using namespace std::chrono;

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
	//payload.set_op(tapdata::ReadLogOp::COMMIT);
	payload.set_scn(scn_);
	payload.set_transactionid(sql_tid_);
	//payload.set_transactiontime(transactionTime);

	pending_scn_wrap_.remove(payload.transactionid());
	payload.set_pendingminscn(pending_scn_wrap_.get_min_scn());
	return message_callback_funcs_.push_abort_message_func_(move(payload));
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

int32_t ReadLogWrap::flush(bool flush_all)
{
	return message_callback_funcs_.flush_func_(flush_all);
}

static db2LSN parse_string(const string& start_scn, bool& error)
{
	db2LSN lri{};
	error = false;
	if (start_scn.empty())
		return lri;

	try
	{
		lri.lsnU64 = std::stoull(start_scn);
	}
	catch (const std::exception& ex)
	{
		LOG_ERROR("parse start scn fail, start scn:{}", start_scn);
		error = true;
	}

	return lri;
}

class DB2ContentWraper : public UtilRecov, public UtilLog
{
public:
	DB2ContentWraper(const ConnectDbSet& connect_set, size_t buffer_size = 64 * 1024 * 50, db2Uint32 db2_version = db2Version970) :
		db2_version_{ db2_version }
	{
		instance_.setInstance((char*)connect_set.nodeName, (char*)connect_set.user, (char*)connect_set.pswd);
		db_emb_.setDb((char*)connect_set.alias, (char*)connect_set.user, (char*)connect_set.pswd);
		log_buffer_.resize(buffer_size);
	}

	int64_t read_log_loop(const db2LSN& startLri, int64_t sleep_interval, ReadLogWrap& rlw)
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
		rc_ = init_info_and_lris(rlw);
		if (rc_ < 0)
			return rc_;

		read_log_input_.iCallerAction = DB2READLOG_READ;
		read_log_input_.piStartLSN = &rlw.get_current_lri();
		read_log_input_.piEndLSN = &rlw.get_end_lri_();
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

        auto last = steady_clock::now();
		while (rlw.isRunning())
		{
			// Read the next log sequence

			memcpy(&rlw.get_current_lri(), &(read_log_info_.nextStartLSN), sizeof(rlw.get_current_lri()));

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
			if (!read_log_info_.logRecsWritten) {
                auto now = steady_clock::now();
                if (duration_cast<seconds>(now - last).count() >= 3) {
                    rlw.sendHeartbeatMessage();
                    last = now;
                }
				msleep(sleep_interval);
            }
		}

#if 0
		//官方例子，现在不生效
		while (keep_run())
		{
			// Read the next log sequence
			memcpy(&rlw.get_current_lri(), &(read_log_info_.nextStartLSN), sizeof(rlw.get_current_lri()));

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

	typedef struct lri_and_time {
		db2LSN lri;
		int time;
	}lri_and_time;

	std::string encode_lri(const db2LSN& lri) {
		return to_string(lri.lsnU64);
	}

	db2LSN decode_lsn(const std::string& lsn_str) {
		db2LSN lsn{};
		if (lsn_str.empty()) {
			return lsn;
		}

		std::istringstream issLsn(lsn_str.c_str());
		issLsn >> lsn.lsnU64;
		return lsn;
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

	bool is_lri_equal(db2LSN& lri1, db2LSN& lri2) {
		return lri1.lsnU64 == lri2.lsnU64;
	}

	int64_t read_lri_sequentially(string logPrefix, db2Uint32 versionNumber, db2ReadLogStruct& readLogInput, db2ReadLogInfoStruct& readLogInfo, struct sqlca& sqlca,
		db2LSN& endLRI, db2LSN& startLRI, vector<char>& logBuffer, std::vector<lri_and_time>& lri_and_time_vec)
	{
		readLogInput.iCallerAction = DB2READLOG_READ;
		readLogInput.poLogBuffer = logBuffer.data();
		readLogInput.piStartLSN = &startLRI;
		readLogInput.piEndLSN = &endLRI;
		readLogInput.poReadLogInfo = &readLogInfo;
		readLogInput.iLogBufferSize = logBuffer.size();
		readLogInput.iFilterOption = DB2READLOG_FILTER_ON;

		// readlog_mutex_.multi_process_mutex_lock();
		LOG_DEBUG("before call db2 read log");
		db2ReadLog(versionNumber, &readLogInput, &sqlca);
		LOG_DEBUG("after call db2 read log");
		// readlog_mutex_.multi_process_mutex_unlock();

		if (sqlca.sqlcode != 0 && sqlca.sqlcode != SQLU_RLOG_READ_TO_CURRENT) {
			LOG_INFO("sql code:{}", sqlca.sqlcode);
			return sqlca.sqlcode;//SQLU_RLOG_INVALID_PARM表示越界，SQLU_RLOG_EXTENT_REQUIRED表示该日志已经被删除，SQLU_RLOG_READ_TO_CURRENT
		}

		sqluint32 numLogRecords = tool::reverse_value(readLogInfo.logRecsWritten);

		char* recordBuffer = readLogInput.poLogBuffer;
		sqluint32 recordSize;
		sqluint16 recordType;
		sqluint16 recordFlag;

		LOG_DEBUG("{}, read lri sequentially numLogRecords: {}", logPrefix, numLogRecords);
		lri_and_time lat{0};
		for (sqluint32 logRecordNb = 0; logRecordNb < numLogRecords; logRecordNb++)
		{
			db2ReadLogFilterData* filterData = (db2ReadLogFilterData*)recordBuffer;
			recordBuffer += sizeof(db2ReadLogFilterData);

			recordSize = tool::reverse_value(*(sqluint32*)(recordBuffer));
			recordType = tool::reverse_value(*(sqluint16*)(recordBuffer + sizeof(sqluint32)));
			recordFlag = tool::reverse_value(*(sqluint16*)(recordBuffer + sizeof(sqluint32) + sizeof(sqluint16)));

			int64_t result = get_commit_time(recordBuffer, recordType, recordFlag);
			if (result > 0) {
				// LOG_DEBUG("get_commit_time:{}", result);
				lat.lri = filterData->recordLSN;
				lat.time = result;
				if (lri_and_time_vec.size() == 0 || (lri_and_time_vec.size() > 0 && lri_and_time_vec.back().time + 300 < result)) {
					lri_and_time_vec.push_back(lat);
				}
			}
			recordBuffer += recordSize;
		}
		if (lri_and_time_vec.size() > 0 && !is_lri_equal(lri_and_time_vec.back().lri, lat.lri) && lri_and_time_vec.back().time != lat.time) {
			lri_and_time_vec.push_back(lat);
		}

		return 0;
	}

	// 从currLri顺序往前读
	void async_read_lri_forward(tapdata::LriRecorder& lri_recorder, db2LSN& currLri, ReadLogWrap &rlw) {
		LOG_DEBUG("enter async read lri forward");

		sqlca sqlca{};
		db2LSN beginLri = currLri;
		db2LSN endLri;
		endLri.lsnU64 = (decltype(endLri.lsnU64))-1;
		tool::reverse_bytes(&endLri.lsnU64);
		LOG_DEBUG("async read lri forward, log buffer size: {}", log_buffer_.size());
		db2LSN lastLri = {0};
		while(rlw.isRunning()) {
			LOG_DEBUG("async read lri forward, beginLri: {}", beginLri.lsnU64);
			std::vector<lri_and_time> lri_and_time_vec;
			int ret = read_lri_sequentially("async read lri forward", db2_version_, read_log_input_, read_log_info_, sqlca, endLri, beginLri, log_buffer_, lri_and_time_vec);
			if (ret < 0) {
				LOG_ERROR("read_lri_sequentially failed, ret: {}", ret);
				return ;
			}
			if (lri_and_time_vec.size() == 0) {
				LOG_DEBUG("async read lri forward, no lri and time");
				sleep(10);
				continue;
			}
			for (auto &&lri_and_time : lri_and_time_vec) {
				auto lsnStr = encode_lri(lri_and_time.lri);
				LOG_DEBUG("async read lri forward, lri:{}, time:{}", lsnStr, lri_and_time.time);
				lri_recorder.Insert(lsnStr, lri_and_time.time);
			}
			lastLri = lri_and_time_vec.back().lri;
			// Read the next log sequence
			memcpy(&beginLri, &(read_log_info_.nextStartLSN), sizeof(beginLri));
			if (sqlca.sqlcode == SQLU_RLOG_READ_TO_CURRENT) {
				LOG_DEBUG("async read lri forward, read to current");
				sleep(60);
			}
		}
	}

	//返回0没有找到合法时间,大于0时间有效,-1错误
	static int64_t get_closest_commit_time(char* logBuffer, sqluint32 numLogRecords, int64_t timeOffset, db2LSN& startLRI)
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
		db2LSN preLRI = startLRI;
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
					startLRI = filterData->recordLSN;
					return result;
				}
				preTime = result;
				preLRI = filterData->recordLSN;
			}
			recordBuffer += recordSize;
		}

		LOG_DEBUG("get closest commit time: {}", preTime);
		startLRI = preLRI;
		return preTime;
	}

	int64_t get_time_of_lri(db2Uint32 versionNumber, db2ReadLogStruct& readLogInput, db2ReadLogInfoStruct& readLogInfo, struct sqlca& sqlca,
		db2LSN& endLRI, db2LSN& startLRI, char* logBuffer, int64_t timeOffset)
	{
		readLogInput.poLogBuffer = logBuffer;
		readLogInput.piStartLSN = &startLRI;
		readLogInput.piEndLSN = &endLRI;
		readLogInput.poReadLogInfo = &readLogInfo;

		// readlog_mutex_.multi_process_mutex_lock();
		LOG_DEBUG("before call db2 read log");
		db2ReadLog(versionNumber, &readLogInput, &sqlca);
		LOG_DEBUG("after call db2 read log");
		// readlog_mutex_.multi_process_mutex_unlock();

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

	//小于0表示获取lri失败
	int64_t init_info_and_lris(ReadLogWrap& rlw)
	{
		int64_t timeOffset = rlw.time_off_set();
		db2LSN& outStartLri = rlw.get_current_lri();
		db2LSN& outEndLri = rlw.get_end_lri_();
		std::string lri_record_name = rlw.lri_record_name();
		int32_t time_back = 7 * 86400;
		if (rlw.time_back() > 0) time_back = rlw.time_back();

		rc_ = init_read_log_struct();
		if (rc_ < 0) return rc_;

		{
			//装载end lri
			outEndLri.lsnU64 = (decltype(outEndLri.lsnU64))-1;

			tool::reverse_bytes(&outEndLri.lsnU64);
		}

		LOG_DEBUG("outEndLri: {}", outEndLri.lsnU64);
		LOG_DEBUG("read_log_info_.initialLSN: {}", tool::reverse_value(read_log_info_.initialLSN.lsnU64));

		if (!rlw.cache_lri() && outStartLri.lsnU64) { //既然有值,在此结束
    		LOG_DEBUG("normal task and having input lsn: {}", outStartLri.lsnU64);
            return 0;
        }

		if (outStartLri.lsnU64 == 0) {
			outStartLri.lsnU64 = tool::reverse_value(read_log_info_.initialLSN.lsnU64);
		}

		sqlca sqlca{};
		if (rlw.cache_lri() && cache_switch) {
			std::string recordlri_lock_name = "/lockfiles/" + lri_record_name;
			mutex_wrapper recordlri_mutex(recordlri_lock_name);
			int rc = recordlri_mutex.init_multi_process_mutex();
			if (rc < 0) return rc;
			rc = recordlri_mutex.multi_process_mutex_trylock();
			if (rc > 0) {
				// 已经有其他进程在记录lri cache了	
				LOG_DEBUG("other process is recording lri cache");				
				return -1;
			}
			LOG_DEBUG("now got the lock");

			tapdata::LriRecorder lri_recorder(lri_record_name);
			rc = lri_recorder.OpenDatabase();
			if (rc != 0) {
				exit(rc);
			}
			rc = lri_recorder.CreateTable();
			if (rc != 0) {
				exit(rc);
			}
			// 从sqlite读取lri
			string lri_record_lowerbound;
			int query_time_lowerbound = time(NULL);
			rc = lri_recorder.Query(lri_record_lowerbound, query_time_lowerbound, 0, time_back);
			LOG_DEBUG("query sqlite, rc:{}, lri:{}, query_time_lowerbound:{}", rc, lri_record_lowerbound, query_time_lowerbound);
			if (!lri_record_lowerbound.empty()) {
				auto tmp = decode_lsn(lri_record_lowerbound);
				if (tmp.lsnU64 > outStartLri.lsnU64) {
					outStartLri = tmp;
				}
			}

			async_read_lri_forward(lri_recorder, outStartLri, rlw);
			recordlri_mutex.multi_process_mutex_unlock();
			// recordlri_mutex.destroy_multi_process_mutex();
			return 0;
		}

		LOG_DEBUG("start finding lri......., timeOffset:{}", timeOffset);

		//既然没有配置timeOffset,在此结束,使用init_read_log_struct查询出来的结果
		if (timeOffset <= 0) {
			outStartLri = read_log_info_.initialLSN;
			return 0;
		}

		int64_t lri_time = 0;
		db2LSN leftLri, rightLri;

		// 先去sqlite3查询
		if (cache_switch) {
			tapdata::LriRecorder lri_recorder(lri_record_name);
			int rc = lri_recorder.OpenDatabase();
			if (rc != 0) {
				return rc;
			}
			string lri_record_lowerbound, lri_record_upperbound;
			int query_time_lowerbound = timeOffset, query_time_upperbound = timeOffset;
			lri_recorder.Query(lri_record_lowerbound, query_time_lowerbound, 0, time_back);
			LOG_DEBUG("rc:{}, lri:{}, query_time_lowerbound:{}", rc, lri_record_lowerbound, query_time_lowerbound);
			lri_recorder.Query(lri_record_upperbound, query_time_upperbound, 1, time_back);
			LOG_DEBUG("rc:{}, lri:{}, query_time_upperbound:{}", rc, lri_record_upperbound, query_time_upperbound);
			if (query_time_lowerbound == query_time_upperbound && query_time_lowerbound != 0) {
				outStartLri = decode_lsn(lri_record_lowerbound);
				lri_recorder.Close();
				return 0;
			}
			if (lri_record_lowerbound.empty()) {
				if (!lri_record_upperbound.empty()) {
					outStartLri = decode_lsn(lri_record_upperbound);
					lri_recorder.Close();
					return 0;
				}
			} else {
				if (lri_record_upperbound.empty()) {
					outStartLri = decode_lsn(lri_record_lowerbound);
					lri_recorder.Close();
					return 0;
				} else {
					leftLri = decode_lsn(lri_record_lowerbound);
					rightLri = decode_lsn(lri_record_upperbound);
					lri_time = get_time_of_lri(db2_version_, read_log_input_, read_log_info_, sqlca, rightLri, leftLri, log_buffer_.data(), timeOffset);
					if (lri_time > 0) {
						outStartLri = leftLri;
						lri_recorder.Close();
						return 0;
					}
					outStartLri = decode_lsn(lri_record_lowerbound);
					lri_recorder.Close();
					return 0;
				}
			}
			LOG_DEBUG("no lri found in sqlite3, task exit");
			return -1;
		}

		//跳转代码还没有适配
		return -1;
	}

	//小于0表示失败，大于等于0成功
	int64_t init_read_log_struct()
	{
		db2ReadLogStruct t{};
		std::swap(read_log_input_, t);

		read_log_input_.iCallerAction = DB2READLOG_QUERY;
		read_log_input_.piStartLSN = NULL;
		read_log_input_.piEndLSN = NULL;
		read_log_input_.poLogBuffer = NULL;
		read_log_input_.iLogBufferSize = 0;

		db2ReadLogInfoStruct tt{};
		std::swap(read_log_info_, tt);

		read_log_input_.iFilterOption = DB2READLOG_FILTER_ON;
		read_log_input_.poReadLogInfo = &read_log_info_;

		sqlca sqlca = { 0 };

		rc_ = db2ReadLog(db2_version_, &read_log_input_, &sqlca);
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

int64_t ReadLogWrap::db2_read_log(const ConnectDbSet& connect_set, const std::string& start_scn, int64_t sleep_interval)
{
	bool parse_error;
	const auto startLRI = parse_string(start_scn, parse_error);
	if (parse_error)
		return -1;

	DB2ContentWraper content(connect_set);
	return content.read_log_loop(startLRI, sleep_interval, *this);
}


