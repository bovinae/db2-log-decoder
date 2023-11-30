#!/bin/bash

#暂时没有生效
\cp -r app/DB2ReadLogServer/bin/centos7/v10/db210_src_srv DB2ReadLogServer/bin/centos7/v10/db210_src_srv
rm DB2ReadLogServer.tgz -rf
tar acf DB2ReadLogServer.tgz DB2ReadLogServer

