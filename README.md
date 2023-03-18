# solarBMS
using Arduino(WeMos D1 wifi) to control battery (valve regulated Gel lead-acid) group.

#BMS蓄电池管理系统(家用太阳能离网发电系统的一部分:蓄电池电力电压自动补偿调试系统)

###Features
1.用4块Arduino独立控制每个蓄电池，再串联成48V储能系统；实现家用太阳能离网发电时的储能调度，4个备用蓄电池，在过充时分流、过放电时自动供电补偿。

###Background
电力补偿系统开发背景，每4块太阳能发电板串联成一组，给4个胶体太阳能蓄电池充电，蓄电池为12v基础电压，充电最高电压为14.6v，放电最低电压为10.8v，超过该电压区间可能损坏电池；
太阳能发电板在阳光普照时电压在20~22v左右，4块串联后，电压在88~90v左右。
充电放电的电压区间在43.2~58v之间。
蓄电池在串联时，各电池因型号不同，或者生产批次不完全一样、或者充放电性能不完全一样时，就会因为分压而出现电压高低不均衡的现象，需要自行调节电压，避免过充或者过放。
每个电池都外接了一个备用分流的电池，共4个备用电池，为一组，都分别对应其主电池，4个备用电池闲时是12v并联状态，互充互放，达到等电压平衡状态；

###使用的元器件
1.48v蓄电池组分4个独立控制子系统，每个子系统含：主蓄电池、备蓄电池、arduino D1(WeMos)、DC-DC降压电源模块(LM2596)、电压检测模块(Voltage Sensor)、5v继电器(10A)、12v大功率继电器(30A)、连接线等。
当前使用了2种开发板，Arduino wifi D1(WeMOS D1)、Arduino UNO(R3)；
2.使用了电压传感器 voltage sensor，每秒测试一次电压，该元件的电压测量范围是0~25v，所以每个电池需要一套独立的检测模块；
3.对于带wifi功能的Arduino，会主动连接一个SSID，向指定服务器的udp端口上报当前的电压；上报电压的UDP端口，4套子系统分别使用这4个端口号：12v-->4801、24v-->4802、36v-->4803、48v-->4804；
4.Arduino的正常用电范围是5~9v，经过降压稳态型变压器降压到8.8v向Arduino供电；

###Trigger Parameters
1.当主电池电压大于14.2v时，就启动对应的一个备用电池连接，对应的备用电池分流充电；
2.当主电池电压低于11.8v时，就启动对应的一个备用电池并联，对应的主电池得到补偿电流，对外供电；
3.充电防抖技术：大于14.2v时，触发给备用电池充电的动作，此时备用电池分流后，电压可能迅速下降，为了防止反复高频次通断，计算电压在连接后低于13.6v后，开启倒计时断开，低于12.1v时立即断开，避免主电池被放电；
4.放电防抖技术：小于11.8v时，启动对应的备用电池补偿，补偿状态下，如果电压突然升高，高于12.1v时，启动断电倒计时；高于13.6v时就直接断开；

