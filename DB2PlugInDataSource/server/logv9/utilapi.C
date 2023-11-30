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
** SOURCE FILE NAME: utilapi.C
**
** SAMPLE: Checks for and prints to the screen SQL warnings and errors
**
**         This utility file is compiled and linked in as an object module
**         with non-embedded SQL sample programs by the supplied makefile
**         and build files.
**
** DB2 APIs USED:
**         sqlaintp -- Get Error Message
**         sqlogstt -- Get SQLSTATE Message
**         sqleatin -- Attach to an Instance
**         sqledtin -- Detach from an Instance
**
** CLASS FUNCTIONS:
**         Db::setDb - set alias, user, password into Db
**         Db::getAlias - get alias from Db
**         Db::getUser  - get user from Db
**         Db::getPswd  - get password from Db
**         Instance::setInstance - set node, user, password into Instance
**         Instance::getNode - get nodenamce from Instance
**         Instance::getUser - get user from Instance
**         Instance::getPswd - get password from Instance
**         Instance::Attach - attach to instance
**         Instance::Detach - detach from instance
**         SqlInfo::SqlInfoPrint - prints on screen SQL warnings and errors
**         CmdLineArgs::CmdLineArgsCheck1 - checks command line arguments
**         CmdLineArgs::CmdLineArgsCheck2 - checks command line arguments
**         CmdLineArgs::CmdLineArgsCheck3 - checks command line arguments
**         CmdLineArgs::CmdLineArgsCheck4 - checks command line arguments
**
*****************************************************************************
**
** For more information on the sample programs, see the README file.
**
** For information on developing C++ applications, see the Application
** Development Guide.
**
** For information on DB2 APIs, see the Administrative API Reference.
**
** For the latest information on programming, compiling, and running DB2
** applications, visit the DB2 application development website at
**     http://www.software.ibm.com/data/db2/udb/ad
****************************************************************************/

#include <string.h>
#include <sqlenv.h>
#include <sqlda.h>
#include <sqlca.h>
#include "utilapi.h"
#include "log_imp.h"

#if ((__cplusplus >= 199711L) && !defined DB2HP && !defined DB2AIX) || \
    (DB2LINUX && (__LP64__ || (__GNUC__ >= 3)) )
#include <sstream>
using namespace std;
#else
#include <sstream.h>
#endif

#pragma GCC diagnostic ignored "-Wwrite-strings"


void Db::setDb(char* a, char* u, char* p)
{
	strcpy(alias, a);
	strcpy(user, u);
	strcpy(pswd, p);
} //Db::setDb

char* Db::getAlias()
{
	return alias;
} //Db::getAlias

char* Db::getUser()
{
	return user;
} //Db::getUser

char* Db::getPswd()
{
	return pswd;
} //Db::getPswd

void Instance::setInstance(char* n, char* u, char* p)
{
	strcpy(nodeName, n);
	strcpy(user, u);
	strcpy(pswd, p);
} //Instance::setInstance

char* Instance::getNode()
{
	return nodeName;
} //Instance::getNode

char* Instance::getUser()
{
	return user;
} //Instance::getUser

char* Instance::getPswd()
{
	return pswd;
} //Instance::getPswd

int Instance::Attach()
{
	if (strlen(nodeName) > 0)
	{
		stringstream str;
		str << "\n\n##############  ATTACH TO THE INSTANCE: " << nodeName
			<< " #######" << endl << endl;
		LOG_INFO("{}", str.str());

		// attach to an instance
		sqleatin(nodeName, user, pswd, &sqlca);
		DB2_API_CHECK("instance -- attach");
	}

	return 0;
} //Instance::Attach

int Instance::Detach()
{
	if (strlen(nodeName) > 0)
	{
		stringstream str;
		str << "\n\n##############  DETACH FROM THE INSTANCE: " << nodeName
			<< " #######" << endl << endl;
		LOG_INFO("{}", str.str());

		// detach from an instance
		sqledtin(&sqlca);
		DB2_API_CHECK("instance -- detach");
	}

	return 0;
} //Instance::Detach

void SqlInfo::SqlInfoPrint(char* appMsg,
	struct sqlca* pSqlca,
	int line,
	char* file)
{
	int rc = 0;
	char sqlstateMsg[1024];
	char errorMsg[1024];

	if (pSqlca->sqlcode != 0 && pSqlca->sqlcode != 100)
	{
		stringstream str;
		str << (pSqlca->sqlcode < 0 ?
			"\n---- error report ----------------------------" :
			"\n---- warning report --------------------------") << endl;

		str << "\napp. message = " << appMsg << endl;
		str << "line         = " << line << endl;
		str << "file         = " << file << endl;
		str << "SQLCODE      = " << pSqlca->sqlcode << endl;
		str << endl;

		// get error message
		rc = sqlaintp(errorMsg, 1024, 80, pSqlca);
		if (rc > 0)
			str << errorMsg << endl;

		// get SQLSTATE message
		rc = sqlogstt(sqlstateMsg, 1024, 80, pSqlca->sqlstate);
		if (rc > 0)
			str << sqlstateMsg << endl;

		str << (pSqlca->sqlcode < 0 ?
			"---- end error report ------------------------" :
			"---- end warning report ----------------------") << endl;
		LOG_WARN("{}", str.str());
	} //endif
} //SqlInfo::SqlInfoPrint

