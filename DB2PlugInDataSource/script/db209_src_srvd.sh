#!/bin/bash
if [ $# -eq 1 ]; then 
    echo $1 
    cd $1
fi

PRO_LIB_PATH=lib/db297
PRO_BIN_PATH=bin/centos7/v9
PRO_NAME=db209_src_srv
PRO_PID=0

curpath=$(cd `dirname $0`; pwd)
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$curpath/${PRO_LIB_PATH}
cd $curpath
ulimit -c unlimited
echo $LD_LIBRARY_PATH

while :
do
    NUMS=()
    c=0
    for p in `ps aux | grep -w  ${PRO_NAME} | grep -v grep |awk '{print $2}'`
        do
            NUMS[$c]=$p
            ((c++))
        done
    if [ "${#NUMS[*]}" -lt "1" ]||[ "${PRO_PID}" -eq "0" ];then
        echo "${PRO_NAME} was killed"
        $curpath/${PRO_BIN_PATH}/${PRO_NAME} &
        PRO_PID=$!
    else
        ex=0
        for value in ${NUMS[*]}
            do
                if [ "$value" -eq "${PRO_PID}" ];then
                    ex=1
                    break
                fi
            done
        if [ "$ex" -eq "0" ];then            
            echo "${PRO_NAME} was killed"
            for value in ${NUMS[*]}
                do
                    kill -9 $value
                done
            $curpath/${PRO_BIN_PATH}/${PRO_NAME} &
            PRO_PID=$!
        fi
    fi
    sleep 5s
done
 



