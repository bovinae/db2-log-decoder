#include <vector>
#include <thread>
#include <chrono>
#include <algorithm> 
#include <cctype>
#include <locale>

#include <sqlenv.h>
#include <db2ApiDf.h>

#include "DB2CataLog.h"
#include "finally_imp.h"
#include "log_imp.h"
#include "tool.h"

#pragma GCC diagnostic ignored "-Wwrite-strings"

namespace tapdata
{
    namespace DB2CataLog
    {
        using namespace std;
        //return 0 OK else fail
        int get_all_remote_node_name(vector<string>& vec)
        {
            struct sqlca sqlca;
            unsigned short nodeDirHandle, nodeEntryNb, nbNodeEntries = 0;
            sqlenops(&nodeDirHandle, &nbNodeEntries, &sqlca);
            if (sqlca.sqlcode < 0)
            {
                LOG_WARN("Node Directory Open Fail");
                return -1;
            }

            // close node directory scan
            auto auto_sqlencls = finally([&nodeDirHandle, &sqlca]()
                {
                    sqlencls(nodeDirHandle, &sqlca);
                    if (sqlca.sqlcode < 0)
                    {
                        LOG_WARN("Node Directory Close Fail");
                        return -1;
                    }
                    return 0;
                });

            vec.clear();
            vec.reserve(nbNodeEntries);

            struct sqleninfo* nodeEntry;
            for (nodeEntryNb = 0; nodeEntryNb < nbNodeEntries; nodeEntryNb++)
            {
                // get next node directory entry
                sqlengne(nodeDirHandle, &nodeEntry, &sqlca);
                if (sqlca.sqlcode < 0)
                {
                    LOG_WARN("Node Directory Read Fail");
                    return -1;
                }

                if (nodeEntry && nodeEntry->protocol != SQL_PROTOCOL_LOCAL)
                {
                    string node(nodeEntry->nodename, SQL_NNAME_SZ);
                    tool::trim(node);
                    vec.emplace_back(move(node));
                }
            } //for
            return 0;
        }

        void delete_nodes(vector<string>& node_names)
        {
            struct sqlca sqlca;
            for (const auto& i : node_names)
            {
                sqleuncn((char*)i.c_str(), &sqlca);
                if (sqlca.sqlcode < 0)
                {
                    LOG_WARN("delete node fail, node:{}, code:{} ", i, sqlca.sqlcode);
                }
            }
        }

        constexpr db2Uint32 versionNumber = db2Version970;

        int get_all_remote_db_alias(vector<string>& vec)
        {
            struct sqlca sqlca;

            char* dbDirPath = NULL;
            db2Uint16 dbDirHandle = 0;

            db2DbDirOpenScanStruct dbDirOpenParmStruct;
            dbDirOpenParmStruct.piPath = dbDirPath;
            dbDirOpenParmStruct.oHandle = dbDirHandle;

            db2DbDirOpenScan(versionNumber,
                &dbDirOpenParmStruct,
                &sqlca);

            if (sqlca.sqlcode < 0)
            {
                LOG_WARN("DB Directory Open Fail");
                return -1;
            }

            struct db2DbDirInfoV9* dbEntry = NULL;
            struct db2DbDirNextEntryStructV9 dbDirNextEntryParmStruct;
            dbDirNextEntryParmStruct.iHandle = dbDirHandle;
            dbDirNextEntryParmStruct.poDbDirEntry = dbEntry;

            db2DbDirCloseScanStruct dbDirCloseParmStruct;

            // close db directory scan
            auto auto_db2DbDirCloseScan = finally([&dbDirCloseParmStruct, &dbDirHandle, &sqlca]()
                {
                    dbDirCloseParmStruct.iHandle = dbDirHandle;
                    db2DbDirCloseScan(versionNumber,
                        &dbDirCloseParmStruct,
                        &sqlca);
                    if (sqlca.sqlcode < 0)
                    {
                        LOG_WARN("DB Directory Close Fail");
                        return -1;
                    }
                    return 0;
                });

            vec.clear();
            vec.reserve(dbDirOpenParmStruct.oNumEntries);

            db2Uint16 dbEntryNb = 0;
            for (dbEntryNb = 1; dbEntryNb <= dbDirOpenParmStruct.oNumEntries; dbEntryNb++)
            {
                // get next database directory entry
                db2DbDirGetNextEntry(versionNumber,
                    &dbDirNextEntryParmStruct,
                    &sqlca);

                if (sqlca.sqlcode < 0)
                {
                    LOG_WARN("DB Directory Read Fail");
                    return -1;
                }
                dbEntry = dbDirNextEntryParmStruct.poDbDirEntry;

                if (dbEntry && dbEntry->type == SQL_REMOTE)
                {
                    string alias(dbEntry->alias, SQL_ALIAS_SZ);
                    tool::trim(alias);
                    vec.emplace_back(move(alias));
                }
            }
            return 0;
        }

        void delete_dbs(vector<string>& alias)
        {
            struct sqlca sqlca;
            for (const auto& i : alias)
            {
                sqleuncd((char*)i.c_str(), &sqlca);
                if (sqlca.sqlcode < 0)
                {
                    LOG_WARN("delete db fail, alias:{}, code:{} ", i, sqlca.sqlcode);
                }
            }
        }

        void clear_all_remote_nodes()
        {
            vector<string> nodes;
            for (int times = 0, err = 0; times < 3; ++times)
            {
                err = get_all_remote_node_name(nodes);
                delete_nodes(nodes);
                if (err != 0)
                    this_thread::sleep_for(1s);
            }
        }

        void clear_all_remote_dbs()
        {
            vector<string> alias;
            for (int times = 0, err = 0; times < 3; ++times)
            {
                err = get_all_remote_db_alias(alias);
                delete_dbs(alias);
                if (err != 0)
                    this_thread::sleep_for(1s);
            }
        }

        void clear_all()
        {
            clear_all_remote_dbs();
            clear_all_remote_nodes();
        }

#define DB2_NODE_ALREADY_EXISTS -1018
#define DB2_NODE_NAME_INVALID -1019
#define DB2_NODE_DIRECTORY_IS_FULL -1020

#define DB2_DB_ALIAS_ALREADY_EXISTS -1005
#define DB2_DB_ALIAS_INVALID -1000
#define DB2_DB_DIRECTORY_IS_FULL -1030

        std::pair<std::string, std::string> create_connect(const string& hostname, const string& service_name, const std::string& db_name)
        {
            std::pair<std::string, std::string> node_alias;

            struct sqle_node_struct newNode;
            newNode.struct_id = SQL_NODE_STR_ID;
            newNode.protocol = SQL_PROTOCOL_TCPIP;

            struct sqle_node_tcpip TCPIPprotocol;
            strncpy(TCPIPprotocol.hostname, hostname.c_str(), SQL_HOSTNAME_SZ + 1);
            strncpy(TCPIPprotocol.service_name, service_name.c_str(), SQL_SERVICE_NAME_SZ + 1);

            struct sqlca sqlca;
            do
            {
                node_alias.first = tool::random_string(SQL_NNAME_SZ);
                strncpy(newNode.nodename, node_alias.first.c_str(), SQL_NNAME_SZ + 1);
                sqlectnd(&newNode, &TCPIPprotocol, &sqlca);
            } while (sqlca.sqlcode == DB2_NODE_ALREADY_EXISTS || sqlca.sqlcode == DB2_NODE_NAME_INVALID);

            if (sqlca.sqlcode == DB2_NODE_DIRECTORY_IS_FULL)
            {
                LOG_WARN("Node Directory Is FULL");
            }

            if (sqlca.sqlcode < 0)
            {
                LOG_WARN("Catalog Node Fail, Code:{}", sqlca.sqlcode);
                std::pair<std::string, std::string> tmp;
                node_alias.swap(tmp);
                return node_alias;
            }

            do
            {
                node_alias.second = tool::random_string(SQL_NNAME_SZ);
                sqlecadb((char*)db_name.c_str(),
                    (char*)node_alias.second.c_str(),
                    SQL_REMOTE,
                    (char*)node_alias.first.c_str(),
                    nullptr,
                    "",
                    SQL_AUTHENTICATION_SERVER,
                    NULL,
                    &sqlca);
            } while (sqlca.sqlcode == DB2_DB_ALIAS_ALREADY_EXISTS || sqlca.sqlcode == DB2_DB_ALIAS_INVALID);

            if (sqlca.sqlcode == DB2_DB_DIRECTORY_IS_FULL)
            {
                LOG_WARN("DB Directory Is FULL");
            }

            if (sqlca.sqlcode < 0)
            {
                LOG_WARN("Catalog Database On Node Fail, Code:{}, Database:{}, Node:{}, Alias:{}",
                    sqlca.sqlcode, db_name, node_alias.first, node_alias.second);
                std::pair<std::string, std::string> tmp;
                node_alias.swap(tmp);
                return node_alias;
            }

            return node_alias;
        }

        void delete_connect(const std::pair<std::string, std::string>& node_alias)
        {
            struct sqlca sqlca;

            sqleuncn((char*)node_alias.first.c_str(), &sqlca);
            if (sqlca.sqlcode < 0)
            {
                LOG_WARN("delete node fail, node:{}, code:{} ", node_alias.first, sqlca.sqlcode);
            }

            sqleuncd((char*)node_alias.second.c_str(), &sqlca);
            if (sqlca.sqlcode < 0)
            {
                LOG_WARN("delete database alias fail, alias:{}, code:{} ", node_alias.second, sqlca.sqlcode);
            }
        }

    }
}


