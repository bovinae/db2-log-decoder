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