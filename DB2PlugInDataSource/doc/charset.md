# 查看数据库编码方式
```
db2 get db cfg for <dst_db> | grep -i code
```
- 如果数据库是UTF-8编码，则输出如下：
```
 Database code page                                      = 1208
 Database code set                                       = UTF-8
 Database country/region code                            = 86
```
- 如果数据库是GBK编码，则输出如下：
```
 Database code page                                      = 1386
 Database code set                                       = GBK
 Database country/region code                            = 86
```
数据库字符集只有建库时才能指定，建库后不能修改。

# 修改系统编码方式
- 命令行直接修改
```
export LANG=zh_CN.gbk
export LANG=zh_CN.UTF-8
```
- 修改/etc/locale.conf，然后执行source /etc/locale.conf

# 修改实例codepage
```
GBK:
db2set db2codepage=1208
UTF-8:
db2set db2codepage=1386
查看修改结果：
db2set -all
```
