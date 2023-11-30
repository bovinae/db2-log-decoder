# db2_src_srv docker程序部署

```bash
#该镜像目前放在132下，拷贝到本地
scp root@192.168.1.132:/home/db2_src_srv.image.v10_125.tgz .
```

## docker操作

```bash
#docker装入db2_src_srv的镜像
docker load -i db2_src_srv.image.v10_125.tgz

#查询已装入镜像
docker images | grep db2_src_srv
#以下是输出
db2_src_srv      v1.25                176ea9ae1b3f   6 minutes ago       2.57GB

#用以下命令启动db2_src_srv，注意容器内部50050是v9监听端口，50051是v10监听端口
docker run -itd --privileged --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --restart=always --name db2_src_srv -p 50030:50050  -p 50031:50051 db2_src_srv:v4 /bin/bash  -c "/home/DB2ReadLogServer/db2_src_start.sh /home/DB2ReadLogServer"

#查询一下运行的镜像
docker ps | grep db2_src_srv

#以下是输出
92214f110d01  db2_src_srv:v1.25  "/bin/bash -c '/home…"  11 minutes ago  Up 11 minutes  0.0.0.0:50030->50050/tcp, :::50030->50050/tcp, 0.0.0.0:50031->50051/tcp, :::50031->50051/tcp

#可查看docker内日志
docker logs -ft db2_src_srv

#可进入docker查看
docker exec -it db2_src_srv bash

#在/home/DB2ReadLogServer/bin/centos7/vx下有外挂数据源日志
tree /home/DB2ReadLogServer/bin/centos7

#以下是输出
/home/DB2ReadLogServer/bin/centos7
|-- v10
|   |-- db210_src_srv
|   |-- db210_src_srv_clogs
|   |   `-- log_2022-05-11.log
|   |-- db210_src_srv_config
|   |   `-- config.json
|   `-- db210_src_srv_slogs
|       `-- log_2022-05-11.log
`-- v9
    |-- db209_src_srv
    |-- db209_src_srv_config
    |   `-- config.json
    `-- db209_src_srv_slogs
        `-- log_2022-05-11.log

#通常查看文件夹尾部为slogs的日志，该日止为主进程日志

```
