# SIM800A_by_STM32
## Device:STM32F103C8T6,RTOS:UCOS2,platform:IAR7.1,Features:TCP,HTTP,Automatic reconnection
## 描述：这几个月给导师做项目时用到了SIM800A模块（其实最开始用到的是SIM900A，但是由于SIM900A停产了所以换成了这个，SIM800A基本兼容SIM900A，所以之前写的SIM900A的库就不上传了），项目用到了SIM800A的TCP连接功能和HTTP连接功能，其中TCP有长连接短连接模式，HTTP使用get方式请求页面数据，以上功能都要求保证连接的有效性，即如果连接失败要自动重连（这也是我花费心血最大的地方）。由于时间仓促，我不准备把这个库写的拿来就能用，如果真有业务需求，还是需要把代码看一遍，修改下必要的部分（主要是用了UCOS2消息队列的API，如果你也使用了UCOS2，那么基本就不需要修改了）
## 开发环境：
###IDE：IAR7.1
###CORE：STM32F103C8T6
###RTOS：UCOS2
###PORT：USART1（PA8，PA9）
## 代码思路：
### 我使用的SIM800A模块只需要向他发送串口AT命令即可做出动作，所以底层是串口通讯，上层是交互及重连逻辑
