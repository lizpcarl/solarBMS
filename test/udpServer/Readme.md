
##测试程序使用node.js实现udp监听端口，用于收集BMS中各电池上报电压及工作状态。

##UDP接收各控制器的上报数据，48v系统，服务端监听8048端口。
收到的数据大概如下：
```
$ node develop/nodejs/udps.js
server listening 0.0.0.0:8048
from 172.18.8.160:56293, receive: {name:"BMS_1",voltage:13.56, state:0}

from 172.18.8.237:62097, receive: {name:"BMS_2",voltage:13.29, state:0}

from 172.18.8.121:64574, receive: {name:"BMS_3",voltage:13.38, state:0}

from 172.18.8.120:64163, receive: {name:"BMS_4",voltage:13.44, state:0}

```