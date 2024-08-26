#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "lri_recorder.h"
#include "log_imp.h"

tapdata::LriRecorder::LriRecorder(const std::string& lri_recorder_name) : db_name_(lri_recorder_name + ".db"), table_name_("lri")
{
}

tapdata::LriRecorder::~LriRecorder()
{
   sqlite3_close(db_);
}

int tapdata::LriRecorder::OpenDatabase()
{
   /* Open database */
   int rc = sqlite3_open(db_name_.c_str(), &db_); // "lri.db_"
   if( rc ){
      LOG_ERROR("Can't open database, rc:{}, msg:{}", rc, sqlite3_errmsg(db_));
      return rc;
   }

   LOG_DEBUG("Opened database successfully.");
   return rc;
}

int tapdata::LriRecorder::CreateTable()
{
   char *zErrMsg = 0;
   std::string sql = "CREATE TABLE if not exists " + table_name_ + "("  \
         "id INTEGER PRIMARY KEY AUTOINCREMENT    NOT NULL," \
         "lri            TEXT    NOT NULL," \
         "time           INT unique NOT NULL);" \
         "CREATE INDEX if not exists index_time ON " + table_name_ + "(time)";
 
   int rc = sqlite3_exec(db_, sql.c_str(), callback, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
      LOG_ERROR("SQL error: {}", zErrMsg);
      sqlite3_free(zErrMsg);
   }else{
      LOG_DEBUG("Table created successfully");
   }
   return rc;
}

int tapdata::LriRecorder::Insert(std::string lri, int time)
{
   char *zErrMsg = 0;
   /* insert SQL statement */
   std::string sql = "INSERT INTO " + table_name_ + " (lri,time) "  \
         "VALUES ('" + lri + "', " + std::to_string(time) + ");";
 
   /* Execute SQL statement */
   int rc = sqlite3_exec(db_, sql.c_str(), callback, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
      LOG_ERROR("SQL error: {}", zErrMsg);
      sqlite3_free(zErrMsg);
   }else{
      LOG_DEBUG("Records created successfully");
   }
   return rc;
}

int tapdata::LriRecorder::Query(std::string& lri, int& time)
{
   char *zErrMsg = 0;
   /* select SQL statement */
   std::string sql = "SELECT * from " + table_name_ + " where time <=" + std::to_string(time) + " order by time desc limit 1;";
   // const char* data = "Callback function called";

   char **resultp = NULL;
   int row = 0;
   int col = 0;
   int rc = sqlite3_get_table(db_, sql.c_str(), &resultp, &row, &col, &zErrMsg);
   if( rc != SQLITE_OK ){
      LOG_ERROR("SQL error: {}", zErrMsg);
   }else{
      LOG_DEBUG("Operation done successfully");
   }
   for (int i = 1; i < row + 1; i++) {
      for (int j = 1; j < col; j++) {
         if (j == 1) {
            lri = resultp[j +i * col];
         } else {
            time = atoi(resultp[j +i * col]);
         }
      }
   }

   sqlite3_free(zErrMsg);
   sqlite3_free_table(resultp);
   return rc;
}

int tapdata::LriRecorder::Delete(int time)
{
   char *zErrMsg = 0;
   std::string sql = "DELETE from " + table_name_ + " where time<=" + std::to_string(time) + ";";
   const char* data = "Callback function called";

   /* Execute SQL statement */
   int rc = sqlite3_exec(db_, sql.c_str(), callback, (void*)data, &zErrMsg);
   if( rc != SQLITE_OK ){
      LOG_ERROR("SQL error: {}", zErrMsg);
      sqlite3_free(zErrMsg);
   }else{
      LOG_DEBUG("Operation done successfully");
   }
   return rc;
}

int tapdata::LriRecorder::DropTable()
{
   char *zErrMsg = 0;
   /* drop table SQL statement */
   std::string sql = "drop table if exists " + table_name_ + ";";
 
   /* Execute SQL statement */
   int rc = sqlite3_exec(db_, sql.c_str(), callback, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
      LOG_ERROR("SQL error: {}", zErrMsg);
      sqlite3_free(zErrMsg);
   }else{
      LOG_DEBUG("Table droped successfully");
   }
   return rc;
}

int tapdata::LriRecorder::Close()
{
   int rc = sqlite3_close(db_);
   if(rc != SQLITE_OK) {
      LOG_ERROR("Can't close database: {}", sqlite3_errmsg(db_));
      return rc;
   }
   LOG_DEBUG("Database closed successfully");
   return SQLITE_OK;
}

// int main(int argc, char* argv[])
// {
//    std::string table_name = "lri";
//    tapdata::LriRecorder rec;
//    rec.OpenDatabase();
//    rec.DropTable(table_name);
//    rec.CreateTable(table_name);
//    rec.Insert(table_name, "1.2.3", 1722077141);
//    rec.Insert(table_name, "1.3.5", 1722077143);
//    rec.Insert(table_name, "1.4.7", 1722077145);
//    std::string lri;
//    int time = 1722077142;
//    rec.Query(table_name, lri, time);
//    printf("lri:%s, time:%d\n", lri.c_str(), time);
//    rec.Delete(table_name, 1722077143);
//    time = 1722077145;
//    rec.Query(table_name, lri, time);
//    printf("lri:%s, time:%d\n", lri.c_str(), time);
//    rec.DropTable(table_name);
//    return 0;
// }
