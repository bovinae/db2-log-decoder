#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h> 
#include <string>

namespace tapdata {

   class LriRecorder
   {
   private:
      sqlite3 *db_;
      const std::string db_name_;
      const std::string table_name_;

      static int callback(void *NotUsed, int argc, char **argv, char **azColName){
         int i;
         for(i=0; i<argc; i++){
            printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
         }
         printf("\n");
         return 0;
      }

   public:
      LriRecorder(const std::string& lri_recorder_name);
      ~LriRecorder();

      int OpenDatabase();
      int CreateTable();
      int Insert(std::string lri, int time);
      // query one record <= input time
      int Query(std::string& lri, int& time);
      int Delete(int time);
      int DropTable();
   };

}
