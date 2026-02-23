# db2 READ LOG 机制配置说明

开启READ LOG有两步：

- 数据库需要配置LOGARCHMETH1

- 抓取变化的目标表需在DDL内配置DATA CAPTURE CHANGES

```bash
#远程连接db2需要catalog
db2 catalog tcpip4 node <local_node_name> remote <remote_db_host> server <port>
db2 catalog database <remote_db_name> as <local_alias_name> at node <local_node_name>
```

## 数据库查询和配置LOGARCHMETH1

```bash
#以下命令是数据库本机操作
#连接
db2 connect to <dst_db> user <user_name> using <user_password>
#是否开启了log
db2 get db cfg for <dst_db>  | grep -i LOGARCHMETH1
#开启日志
db2 update db cfg using LOGARCHMETH1 LOGRETAIN IMMEDIATE
#开启DDL,10版本后的db2才有效
db2 update db cfg using LOG_DDL_STMTS YES IMMEDIATE
#关闭所有执行
db2 force application all
#关闭数据库
db2stop
#第一次使用日志需要全备份数据库
db2 backup db <dst_db>
#断开连接
db2 terminate
#看一看日志情况
db2 archive log for db <dst_db>
#查看日志情况
db2 get db cfg for <dst_db> show detail | grep LOGARCHMETH1
```

## 开启LOGARCHMETH1后的查询状态

```bash
#是否开启了log
db2 get db cfg for <dst_db>  | grep -i logarchmeth1
#以下是输出，如果是(LOGARCHMETH1) = OFF 表明没有开启
 First log archive method                 (LOGARCHMETH1) = LOGRETAIN
 Options for logarchmeth1                  (LOGARCHOPT1) = 
```

## 表启用DATA CAPTURE CHANGES

对表抓取READ LOG，需要在表的DDL内标注DATA CAPTURE CHANGES开启READ LOG日志，如果一张表没有DATA CAPTURE CHANGES，可以远程执行

```sql
ALTER TABLE <table_name> DATA CAPTURE CHANGES;
```

正常的建表语句举例如下,注意DATA CAPTURE CHANGES

```sql
CREATE TABLE "DB2INST1"."TALLTYPE"  (
    "TDATE" DATE , 
    "TGRAPHIC" GRAPHIC(50 CODEUNITS16) , 
    "TREAL" REAL )   
  DATA CAPTURE CHANGES;
```

FAQ: 
1. QL1116N  A connection to or activation of database "TESTDB" failed because
the database is in BACKUP PENDING state.  SQLSTATE=57019

ans: db2 backup db testdb to /dev/null

db2 activate db testdb

./test_client -host 113.98.206.142 -db testdb -passwd 'Gotapd8!'

# 分区表
db2 "CREATE TABLE user_data ( id BIGINT NOT NULL GENERATED ALWAYS AS IDENTITY (START WITH 1 INCREMENT BY 1 CACHE 20) ,name VARCHAR(100) NOT NULL ,phone VARCHAR(20) NOT NULL ,email VARCHAR(255) ,create_time TIMESTAMP NOT NULL WITH DEFAULT CURRENT TIMESTAMP ,update_time TIMESTAMP NOT NULL WITH DEFAULT CURRENT TIMESTAMP ,status CHAR(1) NOT NULL WITH DEFAULT 'A' ,CONSTRAINT pk_user_data PRIMARY KEY (id)) PARTITION BY RANGE(id) ( PARTITION part1 STARTING FROM (1) ENDING AT (1000000) ,PARTITION part2 STARTING FROM (1000001) ENDING AT (2000000) ,PARTITION part3 STARTING FROM (2000001) ENDING AT (3000000) ,PARTITION part4 STARTING FROM (3000001) ENDING AT (MAXVALUE))  DATA CAPTURE CHANGES"

-- 查看表的分区信息
db2 "SELECT tabname, datapartitionname, lowvalue, highvalue, card FROM syscat.datapartitions WHERE tabname = 'USER_DATA' ORDER BY datapartitionid"

插入测试数据验证分区：
db2 "INSERT INTO user_data (name, phone, email, status) VALUES ('jarad', '13800138001', 'zhangsan@example.com', 'A'), ('jack2', '13800138002', 'lisi@example.com', 'A'), ('jack3', '13800138003', 'wangwu@example.com', 'A')"

查spaceid
db2 "SELECT P.PARTITIONOBJECTID, P.TBSPACEID, T.TABLEID FROM SYSCAT.DATAPARTITIONS P JOIN SYSCAT.TABLES T ON T.TABSCHEMA = P.TABSCHEMA AND T.TABNAME = P.TABNAME WHERE P.TABSCHEMA = '%s' AND P.TABNAME IN (%s)"
db2 "SELECT P.PARTITIONOBJECTID, P.TBSPACEID, T.TABLEID FROM SYSCAT.DATAPARTITIONS P JOIN SYSCAT.TABLES T ON T.TABSCHEMA = P.TABSCHEMA AND T.TABNAME = P.TABNAME WHERE P.TABSCHEMA = 'public' AND P.TABNAME IN ('user_data2')"

db2 connect to SAMPLE user db2inst1 using Gotapd888
./test_client -host 192.168.1.191 -db SAMPLE -passwd 'Gotapd888' -port 50000 -bigendian 1 -scn 1,6861577,173165074
db2 get db cfg for SAMPLE  | grep -i LOGARCHMETH1
db2 get db cfg for SAMPLE show detail | grep LOGARCHMETH1

./test_client -host 192.168.1.132 -db TESTDB -passwd 'Gotapd8!' -port 50000 -scn 1,4355887,103316513

./test_client -host 192.168.1.191 -db SAMPLE -passwd 'Gotapd888' -port 50000 -bigendian 1 -st 1770203710

db2 get db cfg for SAMPLE | grep -i log
db2pd -db SAMPLE -logs

SQLCODE -2650（日志不存在）

SQLCODE -2651（非法 LRI）

db2 "CREATE TABLE user_data2 ( id BIGINT NOT NULL,name VARCHAR(100) NOT NULL ,phone VARCHAR(20) NOT NULL ,email VARCHAR(255) ,create_time TIMESTAMP NOT NULL WITH DEFAULT CURRENT TIMESTAMP ,update_time TIMESTAMP NOT NULL WITH DEFAULT CURRENT TIMESTAMP ,status CHAR(1) NOT NULL WITH DEFAULT 'A' ,CONSTRAINT pk_user_data PRIMARY KEY (id)) PARTITION BY RANGE(id) ( PARTITION part1 STARTING FROM (1) ENDING AT (1000000) ,PARTITION part2 STARTING FROM (1000001) ENDING AT (2000000) ,PARTITION part3 STARTING FROM (2000001) ENDING AT (3000000) ,PARTITION part4 STARTING FROM (3000001) ENDING AT (MAXVALUE)) DATA CAPTURE CHANGES"
db2 "INSERT INTO user_data2 (id, name, phone, email, status) VALUES (1, 'test1', '13800138011', 'zhangsan1@example.com', 'B')"
db2 "INSERT INTO user_data2 (id, name, phone, email, status) VALUES (2, 'test1', '13800138011', 'zhangsan1@example.com', 'B')"
db2 "INSERT INTO user_data2 (id, name, phone, email, status) VALUES (1000001, 'test2', '13800138011', 'zhangsan1@example.com', 'B')"
db2 "INSERT INTO user_data2 (id, name, phone, email, status) VALUES (1000002, 'test2', '13800138011', 'zhangsan1@example.com', 'B')"
db2 "INSERT INTO user_data2 (id, name, phone, email, status) VALUES (2000001, 'test2', '13800138011', 'zhangsan1@example.com', 'B')"
db2 "INSERT INTO user_data2 (id, name, phone, email, status) VALUES (3000001, 'test2', '13800138011', 'zhangsan1@example.com', 'B')"
db2 "INSERT INTO user_data2 (id, name, phone, email, status) VALUES (3000002, 'test2', '13800138011', 'zhangsan1@example.com', 'B')"
db2 "INSERT INTO user_data2 (id, name, phone, email, status) VALUES (3000003, 'test2', '13800138011', 'zhangsan1@example.com', 'B')"
db2 "SELECT tabname, datapartitionname, lowvalue, highvalue, card FROM syscat.datapartitions WHERE tabname = 'USER_DATA2' ORDER BY datapartitionid"

# 压缩表
db2 connect to TESTDB user db2inst1 using Gotapd8!
./test_client -host 192.168.1.132 -db TESTDB -passwd 'Gotapd8!' -port 50000 -st 1771048304
set follow-fork-mode child
set detach-on-fork off
b server/logv10/utilrecov.C:573

db2 'CREATE TABLE "FHCDATAHS_LOT1" (
"PROC_LOT_ID" VARCHAR(64 OCTETS) NOT NULL,
"PROC_LOT_TYPE" VARCHAR(64 OCTETS) NOT NULL,
"MONITOR_GRP_ID" VARCHAR(64 OCTETS) NOT NULL,
"PROC_PRODUCE_ID" VARCHAR(64 OCTETS) NOT NULL,
"PROC_PRODGRP_ID" VARCHAR(64 OCTETS) NOT NULL,
"PROC_TECH_ID" VARCHAR(64 OCTETS) NOT NULL,
"PROC_CUSTOMER_ID" VARCHAR(64 OCTETS) NOT NULL,
"PROC_CUSTPROD_ID" VARCHAR(64 OCTETS) NOT NULL,
"MEAS_LOT_ID" VARCHAR(64 OCTETS) NOT NULL,
"MEAS_MAINPID_ID" VARCHAR(64 OCTETS) NOT NULL,
"MEAS_OPE_NO" VARCHAR(64 OCTETS) NOT NULL,
"MEAS_PASS_COUNT" INTEGER NOT NULL,
"CLAIM_TIME" TIMESTAMP NOT NULL,
"CLAIM_SHOP_DATE" DOUBLE NOT NULL,
"PROC_MAINTID_ID" VARCHAR(64 OCTETS) NOT NULL,
"PROC_OPE_NO" VARCHAR(64 OCTETS) NOT NULL,
"PROC_PD_ID" VARCHAR(64 OCTETS) NOT NULL,
"PROC_PASS_COUNT" INTEGER NOT NULL,
"PROC_PD_NAME" VARCHAR(64 OCTETS) NOT NULL,
"PROC_AREA_ID" VARCHAR(64 OCTETS) NOT NULL,
"PROC_EQP_ID" VARCHAR(64 OCTETS) NOT NULL,
"PROC_EQP_NAME" VARCHAR(64 OCTETS) NOT NULL,
"PROC_LCRECIPE_ID" VARCHAR(64 OCTETS) NOT NULL,
"PROC_RECIPE_ID" VARCHAR(64 OCTETS) NOT NULL,
"PROC_PR RECIPE_ID" VARCHAR(64 OCTETS) NOT NULL,
"CTRL_JOB" VARCHAR(64 OCTETS) NOT NULL,
"PROC_TIME" TIMESTAMP NOT NULL,
"PROC_SHOP_DATE" DOUBLE NOT NULL,
"PROC_WFRHS_TIME" TIMESTAMP NOT NULL,
"STORE_TIME" TIMESTAMP NOT NULL
) DATA CAPTURE CHANGES COMPRESS YES ADAPTIVE'
db2 "INSERT INTO DB2INST1.FHCDATAHS_LOT1 VALUES ('ab12345c.1', 'ab12345c.123', 'ab12345c.123', 'ab12345c.123', 'ab12345c.123', 'ab12345c.123', 'ab12345c.123', 'ab12345c.123', 'ab12345c.123', 'ab12345c.123', 'ab12345c.123', 0, '2026-02-11 10:19:55.123456', 801.123456789, 'ab_abc12,100.200', 'ab_abc12,100.200', 'ab_abc12,100.200', 1,'ab_abc12,100.200', 'ab_abc12,100.200', 'ab_abc12,100.200', 'ab_abc12,100.200', 'ab_abc12,100.200', 'ab_abc12,100.200', 'ab_abc12,100.200', 'ab_abc12,100.200', '2026-02-11 10:20:45.000000', 1212.12123456, '2026-02-11 10:20:58.123456', '2026-02-11 10:20:59.123456')"
db2 "UPDATE DB2INST1.FHCDATAHS_LOT1 set PROC_LOT_TYPE='a' where PROC_LOT_ID='ab12345c.2'"

