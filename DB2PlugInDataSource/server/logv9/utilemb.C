static char sqla_program_id[292] = 
{
 '\xac','\x0','\x41','\x45','\x41','\x56','\x41','\x49','\x5a','\x41','\x76','\x32','\x43','\x4f','\x4d','\x6c','\x30','\x31','\x31','\x31',
 '\x31','\x20','\x32','\x20','\x20','\x20','\x20','\x20','\x20','\x20','\x20','\x20','\x8','\x0','\x44','\x42','\x32','\x49','\x4e','\x53',
 '\x54','\x31','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0',
 '\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0',
 '\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0',
 '\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0',
 '\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0',
 '\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0',
 '\x0','\x0','\x8','\x0','\x55','\x54','\x49','\x4c','\x45','\x4d','\x42','\x20','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0',
 '\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0',
 '\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0',
 '\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0',
 '\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0',
 '\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0',
 '\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0','\x0'
};

#include "sqladef.h"

static struct sqla_runtime_info sqla_rtinfo = 
{{'S','Q','L','A','R','T','I','N'}, sizeof(wchar_t), 0, {' ',' ',' ',' '}};


static const short sqlIsLiteral   = SQL_IS_LITERAL;
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
** SOURCE FILE NAME: utilemb.sqC
**
** SAMPLE: Checks for and prints to the screen SQL warnings and errors
**
**         This utility file is compiled and linked in as an object module 
**         with embedded SQL sample programs by the supplied makefile and 
**         build files.
**
** SQL STATEMENTS USED:
**         BEGIN DECLARE SECTION
**         END DECLARE SECTION
**   ?    ROLLBACK
**         CONNECT
**
** CLASS FUNCTIONS:
**         DbEmb::TransRollback() - rolls back the transaction
**         DbEmb::Connect() - connects to the database
**         DbEmb::Disconnect() - disconnects from the database
**
*****************************************************************************
**
** For more information on the sample programs, see the README file.
**
** For information on developing C++ applications, see the Application
** Development Guide.
**
** For information on using SQL statements, see the SQL Reference.
**
** For the latest information on programming, compiling, and running DB2
** applications, visit the DB2 application development website at
**     http://www.software.ibm.com/data/db2/udb/ad
****************************************************************************/

#include <string.h>
#include <sql.h>
#include <sqlenv.h>
#include <sqlda.h>
#include <sqlca.h>
#include "utilemb.h"
#if ( defined( DB2NT ) )
#include "utilapi.cxx"
#else //UNIX
#include "utilapi.C"
#endif
#if ((__cplusplus >= 199711L) && !defined DB2HP && !defined DB2AIX) || \
    (DB2LINUX && (__LP64__ || (__GNUC__ >= 3)) )
   #include <iostream>
   using namespace std;
#else
   #include <iostream.h>
#endif


/*
EXEC SQL BEGIN DECLARE SECTION;
*/


  char dbAlias[15];
  char dbUser[129];
  char dbPswd[15];

/*
EXEC SQL END DECLARE SECTION;
*/



void DbEmb::TransRollback()
{
#if 0
  struct sqlca sqlca;

  // rollback the transaction
  cout << "\n  Rolling back the transaction..." << endl;

  
/*
EXEC SQL ROLLBACK;
*/

{
  sqlastrt(sqla_program_id, &sqla_rtinfo, &sqlca);
  sqlacall((unsigned short)28,0,0,0,0L);
  sqlastop(0L);
}


  SqlInfo::SqlInfoPrint("ROLLBACK", &sqlca, __LINE__, __FILE__);
  if (sqlca.sqlcode == 0)
  {
    cout << "  The transaction was rolled back." << endl;
  }

#endif
} //DbEmb::TransRollback

int DbEmb::Connect()
{
  struct sqlca sqlca;

  strcpy(dbAlias, alias);
  strcpy(dbUser, user);
  strcpy(dbPswd, pswd);

  cout << "\n  Connecting to '";
  cout << alias << "' database..." << endl;
  if (strlen(dbUser) == 0)
  {
    
/*
EXEC SQL CONNECT TO :dbAlias;
*/

{
  sqlastrt(sqla_program_id, &sqla_rtinfo, &sqlca);
  sqlaaloc(2,1,1,0L);
    {
      struct sqla_setdata_list sql_setdlist[1];
      sql_setdlist[0].sqltype = 460; sql_setdlist[0].sqllen = 15;
      sql_setdlist[0].sqldata = (void*)dbAlias;
      sql_setdlist[0].sqlind = 0L;
      sqlasetdata(2,0,1,sql_setdlist,0L,0L);
    }
  sqlacall((unsigned short)29,4,2,0,0L);
  sqlastop(0L);
}


    EMB_SQL_CHECK("CONNECT");
  }
  else
  {
    
/*
EXEC SQL CONNECT TO :dbAlias USER :dbUser USING :dbPswd;
*/

{
  sqlastrt(sqla_program_id, &sqla_rtinfo, &sqlca);
  sqlaaloc(2,3,2,0L);
    {
      struct sqla_setdata_list sql_setdlist[3];
      sql_setdlist[0].sqltype = 460; sql_setdlist[0].sqllen = 15;
      sql_setdlist[0].sqldata = (void*)dbAlias;
      sql_setdlist[0].sqlind = 0L;
      sql_setdlist[1].sqltype = 460; sql_setdlist[1].sqllen = 129;
      sql_setdlist[1].sqldata = (void*)dbUser;
      sql_setdlist[1].sqlind = 0L;
      sql_setdlist[2].sqltype = 460; sql_setdlist[2].sqllen = 15;
      sql_setdlist[2].sqldata = (void*)dbPswd;
      sql_setdlist[2].sqlind = 0L;
      sqlasetdata(2,0,3,sql_setdlist,0L,0L);
    }
  sqlacall((unsigned short)29,5,2,0,0L);
  sqlastop(0L);
}


    EMB_SQL_CHECK("CONNECT");
  }
  cout << "  Connected to '" << dbAlias << "' database." << endl;

  return 0;

} //DbEmb::Connect

int DbEmb::Disconnect()
{
  struct sqlca sqlca;

  cout << "\n  Disconnecting from '";
  cout << alias << "' database..." << endl;

  
/*
EXEC SQL CONNECT RESET;
*/

{
  sqlastrt(sqla_program_id, &sqla_rtinfo, &sqlca);
  sqlacall((unsigned short)29,3,0,0,0L);
  sqlastop(0L);
}


  EMB_SQL_CHECK("CONNECT RESET");

  cout << "  Disconnected from '";
  cout << alias << "' database." << endl;

  return 0;

} //DbEmb::Disconnect
