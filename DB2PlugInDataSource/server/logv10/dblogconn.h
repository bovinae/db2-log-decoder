#ifndef DBLOGCONN_H
#define DBLOGCONN_H
#include <sqlenv.h>
#include <db2ApiDf.h>
#include <string>
#include <functional>
#include <cstdint>
#include "DB2PlugInDataSource.pb.h"
#include "LocalDDLInfo.h"
#include "pending_scn_wrap.h"

#ifndef USERID_SZ
#define USERID_SZ 128
#endif // !USERID_SZ

#ifndef PSWD_SZ
#define PSWD_SZ 14
#endif // !PSWD_SZ

struct ConnectDbSet
{
	char alias[SQL_ALIAS_SZ + 1]{};
	char nodeName[SQL_INSTNAME_SZ + 1]{};
	char user[USERID_SZ + 1]{};
	char pswd[PSWD_SZ + 1]{};
};

#pragma pack(push)
#pragma pack(1)
struct LocalSqluTid
{
	unsigned short ids[3];
	operator std::string() const
	{
		return std::to_string(ids[0]) + "." + std::to_string(ids[1]) + "." + std::to_string(ids[2]);
	}
};

struct LocalLogRecordHeader
{
	db2Uint32 lengthOfTheLogRecord;

	db2Uint16 typeOfTheLogRecord;
	db2Uint16 logRecordFlags;
	db2Uint64 logSequenceNumber;
	db2Uint64 logFlushSequence;
	db2Uint64 logSequenceOffset;
	LocalSqluTid sqlTid;
	db2LogStreamIDType logStreamId;
};

struct LocalLogRecordHeaderExtra
{
	db2LogStreamIDType extraLogStreamId;
	db2Uint8 reserved[6];
	db2Uint64 extraLsoForCompensationLogRecords;
};

struct LocalLogRecordHeaderExtraCompensation
{
	db2Uint64 extraLsoForPropagatableCompensationLogRecords;
};

struct LocalDMSLogRecordHeader
{
	uint8_t componentIdentifier;
	uint8_t functionIdentifier;
	uint16_t tableSpaceIdentifier;
	uint16_t tableIdentifier;
};

struct LocalRDSLogRecordHeader
{
	uint8_t componentIdentifier;
	uint8_t functionIdentifier;
	uint16_t tableSpaceIdentifier;
	uint16_t tableIdentifier;
	uint16_t rdsLogRecFlags;
};

struct LocalLOBLogRecordHeader
{
	uint8_t componentIdentifier;
	uint8_t operationType;
	uint16_t tableSpaceIdentifier;
	uint16_t objectIdentifier;
	uint16_t parentTableSpaceIdentifier;
	uint16_t parentObjectIdentifier;
	uint16_t internal;
};

struct LocalDOMLogRecordHeader
{
	uint8_t componentIdentifier;
	uint8_t functionIdentifier;
	uint16_t objTableSpaceIdentifier;
	uint16_t objObjectIdentifier;
	uint16_t tableSpaceIdentifier;
	uint16_t tableIdentifier;
	uint8_t objectType;
	uint8_t flags;
};

struct LocalAddLobDataAndAddLobAmountLogRecord
{
	//LocalLOBLogRecordHeader logRecordHeader;
	sqluint32 length;
	sqluint64 offset;
	uint8_t fInternal;
	uint8_t operationType;
	uint16_t columnIdentifier;
	uint32_t sInternal;
	int8_t data[0];
};

struct LocalDDLStatementLogRecord
{
	struct Entry
	{
		uint16_t type;
		uint16_t length;
	};
	uint16_t operationIdentifier;
	uint16_t objectIdentifier;
	uint16_t options;
	uint16_t numEntries;
	sqluint32 ddlTextLeft;
	sqluint32 ddlTextLength;
	Entry entrys[0];
};

struct ReorgTableLogRecord
{
	//LocalDOMLogRecordHeader logRecordHeader;
	uint8_t internal[476];
	uint16_t indexToken;
	uint16_t temporaryTableSpaceID;
	uint16_t longTemporaryTableSpaceID;
};

#pragma pack(pop)

struct ReadLogWrap
{
	ReadLogWrap(tapdata::ReadLogRequest readLogRequest) : time_off_set_(readLogRequest.stime()), cache_lri_(readLogRequest.cachelri())
	{
		lri_record_name_{};
		if (readLogRequest.cachelri()) {
			lri_record_name_ = readLogRequest.source().databasehostname()
			+ "_" + readLogRequest.source().databaseservicename()
			+ "_" + readLogRequest.source().databasename();
		}
	}

	using push_dml_message_func = std::function<int32_t(tapdata::ReadLogPayload&&, bool reorgPending)>;
	using push_ddl_message_func = std::function<int32_t(tapdata::ReadLogPayload&&, tapdata::LocalDDLInfo&&)>;

	using push_undo_dml_message_func = std::function<int32_t(tapdata::ReadLogPayload&&)>;
	using push_undo_ddl_message_func = std::function<int32_t(tapdata::ReadLogPayload&&)>;

	using push_reorg_table_message_func = std::function<int32_t(tapdata::ReadLogPayload&&)>;

	using push_commit_message_func = std::function<int32_t(tapdata::ReadLogPayload&&)>;
	using push_heartbeat_message_func = std::function<int32_t(tapdata::ReadLogPayload&&)>;
	using push_pending_message_func = std::function<int32_t(tapdata::ReadLogPayload&&)>;
	using push_abort_message_func = std::function<int32_t(tapdata::ReadLogPayload&&)>;

	using flush_func = std::function<int32_t(bool)>;

	struct message_callback_funcs
	{
		push_dml_message_func push_dml_message_func_;
		push_ddl_message_func push_ddl_message_func_;

		push_undo_dml_message_func push_undo_dml_message_func_;
		push_undo_ddl_message_func push_undo_ddl_message_func_;

		push_reorg_table_message_func push_reorg_table_message_func_;

		push_commit_message_func push_commit_message_func_;
		push_heartbeat_message_func push_heartbeat_message_func_;
		push_abort_message_func push_abort_message_func_;

		flush_func flush_func_;
	};

	void set_message_callback_funcs(message_callback_funcs funcs)
	{
		message_callback_funcs_ = funcs;
	}

	void set_scn(const std::string& scn)
	{
		scn_ = scn;
	}

	void set_sql_tid(const LocalSqluTid& tid)
	{
		memcpy(&sql_tid_, &tid, sizeof(sql_tid_));
	}

	db2LRI& get_current_lri()
	{
		return current_lri_;
	}

	db2LRI& get_end_lri_()
	{
		return end_lri_;
	}

	bool isRunning() const;

	//返回小于0表示出错，直接退出
	int sendDMLMessage(int functionId, tapdata::ReadLogOp op, sqluint32 logRecordSize, const char* logRecordBuffer, sqluint32 beforeLogRecordSize,
		const char* beforeLogRecordBuffer, const LocalDMSLogRecordHeader* header, const char* rid) const;

	//返回小于0表示出错，直接退出
	int sendNormalCommitMessage(sqluint32 transactionTime) const;

	//返回小于0表示出错，直接退出
	int sendHeartbeatMessage() const;

	//返回小于0表示出错，直接退出
	int sendAbortMessage() const;

	//返回小于0表示出错，直接退出
	int sendDDLMessage(const LocalDDLStatementLogRecord* record) const;

	//返回小于0表示出错，直接退出
	int sendUndoDDLMessage(const LocalRDSLogRecordHeader* record) const;

	//返回小于0表示出错，直接退出
	int sendUndoDMLMessage(const LocalDMSLogRecordHeader* header, const char* rid) const;

	//返回小于0表示出错，直接退出
	int sendReorgTableMessage(const LocalDOMLogRecordHeader* header) const;

	int32_t flush(bool flush_all);

	int64_t db2_read_log(const ConnectDbSet& connect_set, const std::string& start_scn, int64_t sleep_interval);

	int64_t time_off_set() const
	{
		return time_off_set_;
	}

	std::string lri_record_name() const
	{
		return lri_record_name_;
	}

	bool cache_lri() const
	{
		return cache_lri_;
	}

private:
	const int64_t time_off_set_;
	const std::string lri_record_name_;
	bool cache_lri_;
	message_callback_funcs message_callback_funcs_;

	db2LRI current_lri_{};
	db2LRI end_lri_{};

	std::string scn_;
	LocalSqluTid sql_tid_;

	mutable tapdata::PendingScnWrap pending_scn_wrap_;
};

#endif


