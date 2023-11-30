#!/bin/bash
if [ $# -eq 1 ]; then 
    echo $1 
    cd $1
fi

curpath=$(cd `dirname $0`; pwd)
cd $curpath

#下面两行9版本可以注释掉
#echo Start ${curpath}/db209_src_srvd.sh
#su - db2inst1 -s /bin/bash -c "${curpath}/db209_src_srvd.sh ${curpath}" &

echo Start ${curpath}/db210_src_srvd.sh
su - db2inst2 -s /bin/bash -c "${curpath}/db210_src_srvd.sh ${curpath}" &

while :
   do
        sleep 5s
done

