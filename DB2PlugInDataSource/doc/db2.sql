-- 初始化
create table "HstestUser"(ID      INTEGER not null constraint "HstestUser_pk" primary key, TITLE   VARCHAR(50), CREATED TIMESTAMP(6));

-- 丢数场景一
begin
    SET CURRENT ISOLATION UR;
    insert into "HstestUser"(ID, TITLE, CREATED) values (3, '3', CURRENT_TIMESTAMP); -- 事件丢失
    SAVEPOINT A ON ROLLBACK RETAIN CURSORS;
    insert into "HstestUser"(ID, TITLE, CREATED) values (7, '7', CURRENT_TIMESTAMP); -- 正常回滚
    insert into "HstestUser"(ID, TITLE, CREATED) values (8, '8', CURRENT_TIMESTAMP); -- 正常回滚
    ROLLBACK WORK TO SAVEPOINT A;
    insert into "HstestUser"(ID, TITLE, CREATED) values (9, '9', CURRENT_TIMESTAMP); -- 正常同步
    insert into "HstestUser"(ID, TITLE, CREATED) values (10, '10', CURRENT_TIMESTAMP); -- 正常同步
    COMMIT;
end;

-- 第一次修复后，出现丢数场景二
begin
SET CURRENT ISOLATION UR;
    insert into "HstestUser"(ID, TITLE, CREATED) values (3, '3', CURRENT_TIMESTAMP); -- 事件丢失
    SAVEPOINT A ON ROLLBACK RETAIN CURSORS;
    insert into "HstestUser"(ID, TITLE, CREATED) values (7, '7', CURRENT_TIMESTAMP); -- 正常回滚
    insert into "HstestUser"(ID, TITLE, CREATED) values (8, '8', CURRENT_TIMESTAMP); -- 正常回滚
    ROLLBACK WORK TO SAVEPOINT A;
    COMMIT;
end;

db2 'ALTER TABLE "DB      "."TBACT2DAY_20230628"         ADD PRIMARY KEY                ("DATADATE",                 "GROUPID",                 "PGRMID",                 "SRLNO");'
GRANT SELECT ON TABLE "DB      "."TBACT2DAY_20230628" TO USER "JAVAREP ";
GRANT CONTROL ON TABLE "DB      "."TBACT2DAY_20230628" TO USER "TAPUSER ";
db2 "insert into DB.TBACT2DAY_20230628 (DATADATE, GROUPID, PGRMID, SRLNO, WCE, QTY, UNIT, AMT, DPD, \"INPUT\", \"OUTPUT\", MEMO, CREATEEMP, CREATEDATE) VALUES('20231208', 'ACVER01', '091', 'FQT00910600080', '600080', -450.1700, '', -877831.5000, 'FQT00910', '091', '091', ' 来自T0', 'ICSC01', '20171231');"
CREATE TABLE "DB      "."TBACT2DAY_20230628"  (
                  "DATADATE" VARCHAR(8) NOT NULL WITH DEFAULT '' , 
                  "GROUPID" VARCHAR(30) NOT NULL WITH DEFAULT '' , 
                  "PGRMID" VARCHAR(20) NOT NULL WITH DEFAULT '' , 
                  "SRLNO" VARCHAR(100) NOT NULL WITH DEFAULT '' , 
                  "WCE" VARCHAR(10) WITH DEFAULT '' , 
                  "QTY" DECIMAL(17,4) WITH DEFAULT 0 , 
                  "UNIT" VARCHAR(10) WITH DEFAULT '' , 
                  "AMT" DECIMAL(17,4) WITH DEFAULT 0 , 
                  "DPD" VARCHAR(100) WITH DEFAULT '' , 
                  "INPUT" VARCHAR(10) WITH DEFAULT '' , 
                  "OUTPUT" VARCHAR(10) WITH DEFAULT '' , 
                  "MEMO" VARCHAR(100) WITH DEFAULT '' , 
                  "CREATEEMP" VARCHAR(10) WITH DEFAULT '' , 
                  "CREATEDATE" VARCHAR(8) WITH DEFAULT '' )   
                 DATA CAPTURE CHANGES  
                 IN "FINANCE_SPACE1"  
                 ORGANIZE BY ROW;

db2 "CREATE TABLE t4(a DECIMAL(17,4) WITH DEFAULT 0 ,b DECIMAL(17,4) WITH DEFAULT 0) DATA CAPTURE CHANGES ORGANIZE BY ROW;"
db2 "CREATE TABLE t5(a DECIMAL(17,4) WITH DEFAULT 0) DATA CAPTURE CHANGES ORGANIZE BY ROW;"
db2 "CREATE TABLE t6(a VARCHAR(4) NOT NULL WITH DEFAULT '', b DECIMAL(17,4) WITH DEFAULT 0, c VARCHAR(4) NOT NULL WITH DEFAULT '') DATA CAPTURE CHANGES ORGANIZE BY ROW;"
db2 "CREATE TABLE t7(a VARCHAR(10) WITH DEFAULT '', b DECIMAL(17,4) WITH DEFAULT 0, c VARCHAR(10) WITH DEFAULT '') DATA CAPTURE CHANGES ORGANIZE BY ROW;"
db2 "CREATE TABLE t8(a VARCHAR(10) NOT NULL WITH DEFAULT '', b DECIMAL(17,4) WITH DEFAULT 0, c VARCHAR(10) NOT NULL WITH DEFAULT '') DATA CAPTURE CHANGES ORGANIZE BY ROW;"

db2 "insert into t5(a) values(-450.1700)"
db2 "insert into t4(a,b) values(-450.1700, -877831.5000)"
db2 "insert into t6(a,b,c) values('a', -450.1700, 'c')"
db2 "insert into t6(a,b,c) values('', -450.1700, 'c')"
db2 "insert into t7(a,b,c) values('', -450.1700, 'c')"
db2 "insert into t8(a,b,c) values('', -450.1700, 'c')"
db2 'CREATE TABLE "SAMPLE"."TALLTYPE"  (    "TDATE" DATE ,     "TGRAPHIC" GRAPHIC(50 CODEUNITS16) ,     "TREAL" REAL )     DATA CAPTURE CHANGES';

db2 "insert into t1 values('设置')"

db2 +c "INSERT INTO t1 VALUES('test sp9')"
db2 +c "SAVEPOINT mark1 UNIQUE ON ROLLBACK RETAIN CURSORS"
db2 +c "INSERT INTO t1 VALUES('test sp10')"
db2 +c "ROLLBACK TO SAVEPOINT mark1"
db2 COMMIT

db2 "create table \"HstestUser\"(ID INTEGER not null constraint \"HstestUser_pk\" primary key, TITLE VARCHAR(50), CREATED TIMESTAMP(6)) DATA CAPTURE CHANGES;"
db2 "SET CURRENT ISOLATION UR;"
db2 +c "insert into \"HstestUser\"(ID, TITLE, CREATED) values (3, '3', CURRENT_TIMESTAMP);"
db2 +c "SAVEPOINT A ON ROLLBACK RETAIN CURSORS;"
db2 +c "insert into \"HstestUser\"(ID, TITLE, CREATED) values (7, '7', CURRENT_TIMESTAMP);"
db2 +c "insert into \"HstestUser\"(ID, TITLE, CREATED) values (8, '8', CURRENT_TIMESTAMP);"
db2 +c "ROLLBACK WORK TO SAVEPOINT A;"
db2 +c "insert into \"HstestUser\"(ID, TITLE, CREATED) values (9, '9', CURRENT_TIMESTAMP);"
db2 +c "insert into \"HstestUser\"(ID, TITLE, CREATED) values (10, '10', CURRENT_TIMESTAMP);"
db2 COMMIT


db2 +c "insert into \"HstestUser\"(ID, TITLE, CREATED) values (3, '3', CURRENT_TIMESTAMP);"
db2 +c "SAVEPOINT A ON ROLLBACK RETAIN CURSORS;"
db2 +c "insert into \"HstestUser\"(ID, TITLE, CREATED) values (7, '7', CURRENT_TIMESTAMP);"
db2 +c "insert into \"HstestUser\"(ID, TITLE, CREATED) values (8, '8', CURRENT_TIMESTAMP);"
db2 +c "ROLLBACK WORK TO SAVEPOINT A;"
db2 COMMIT

docker run -itd --name db2 --privileged=true -p 50008:50000 -e LICENSE=accept -e DB2INST1_PASSWORD=password -e DBNAME=testdb -v /d/volumes/db2:/database taskana/db2:11.1

db2 "create table t(ID INTEGER not null constraint \"t_pk\" primary key, TITLE VARCHAR(50), CREATED TIMESTAMP(6)) DATA CAPTURE CHANGES"

db2 TRUNCATE TABLE t IMMEDIATE

db2 -vtf t.sql

关闭事务自动提交
db2set DB2OPTIONS=+c
打开事务自动提交
db2set DB2OPTIONS=
查看事务自动提交设置
db2 list command options | egrep 'Auto-Commit|DB2OPTIONS'

测试lob类字段:
db2 "create table testlob(id INTEGER not null constraint \"t_pk\" primary key, title VARCHAR(50), detail CLOB(10485760)) DATA CAPTURE CHANGES;"
db2 "insert into testlob(id, title, detail) values (1, 'test title', 'test detail');"
