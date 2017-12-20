/******************************************************************************
包含头文件
******************************************************************************/
#include "includes.h"
/******************************************************************************
数据缓冲区定义
******************************************************************************/
u8 GPRSLine = 0, GPRSRow = 0;//接收的行和列计数
u8 GPRSReceiveBuff[10][GPRS_RECEIVE_STRING_BUFFER_LENGTH];//GPRS接收数据缓冲区
u8 GPRSSendBuff[100];//GPRS发送数据缓冲区
/******************************************************************************
函数定义
******************************************************************************/
/*****************************************************************************/
// FUNCTION NAME: SIM800A_RebootIOInit
//
// DESCRIPTION:
//  sim900a重启引脚初始化
//
// CREATED: 2017-11-05 by zilin Wang
//
// FILE: SIM900A.c
/*****************************************************************************/
void SIM800A_RebootIOInit()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(SIM800A_Reboot_RCC_APBPERIPH_GPIOX, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = SIM800A_Reboot_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(SIM800A_Reboot_GPIOX, &GPIO_InitStructure);
	
	GPIO_SetBits(SIM800A_Reboot_GPIOX, SIM800A_Reboot_GPIO_PIN);//开机
}
/*****************************************************************************/
// FUNCTION NAME: SIM800A_CommandHandleDeinit
//
// DESCRIPTION:
//  初始化结构体
//
// CREATED: 2017-11-05 by zilin Wang
//
// FILE: network.c
/*****************************************************************************/
void SIM800A_CommandHandleDeinit(SIM800A_QueryTypeDef * CommandHandle, OS_EVENT * MsgQ)
{
    CommandHandle->DebugString = NULL;
    CommandHandle->SendString = NULL;
    CommandHandle->ReturnString = NULL;
    CommandHandle->ReturnStringExt1 = NULL;
    CommandHandle->ReturnStringExt2 = NULL;
    CommandHandle->GPRSUSART = USART_GPRS;
    CommandHandle->DEBUGUSART = USART_DEBUG;
    CommandHandle->MsgQ = MsgQ;
    CommandHandle->delayMs = 800;
    CommandHandle->retryTimes = 3;
}
/*****************************************************************************/
// FUNCTION NAME: SIM800A_CommandHandleInit
//
// DESCRIPTION:
//  初始化发送结构体
//
// CREATED: 2017-10-28 by zilin Wang
//
// FILE: network.c
/*****************************************************************************/
void SIM800A_CommandHandleInit(SIM800A_QueryTypeDef * CommandHandle, USART_TypeDef * GPRSUSART, USART_TypeDef * DEBUGUSART, OS_EVENT * MsgQ)
{
    CommandHandle->DebugString = NULL;
    CommandHandle->SendString = NULL;
    CommandHandle->ReturnString = NULL;
    CommandHandle->ReturnStringExt1 = NULL;
    CommandHandle->ReturnStringExt2 = NULL;
    CommandHandle->GPRSUSART = GPRSUSART;
    CommandHandle->DEBUGUSART = DEBUGUSART;
    CommandHandle->MsgQ = MsgQ;
    CommandHandle->delayMs = 800;
    CommandHandle->retryTimes = 3;
}
/*****************************************************************************/
// FUNCTION NAME: SIM800A_SendSampleCommand
//
// DESCRIPTION:
//  发送一个只有一个返回值的命令
//
// CREATED: 2017-10-27 by zilin Wang
//
// FILE: network.c
/*****************************************************************************/
Status SIM800A_SendSampleCommand(SIM800A_QueryTypeDef * CommandHandle)
{ 
    u8 retryCnt = 0;
    u8 waitCnt = 0;
    u8 * receiveString;
    OS_Q_DATA ReceiveMsgQData;			//存放消息队列状态的结构
    INT8U error;
    Status returnStatus;
    USARTSendString(CommandHandle->GPRSUSART, (u8*)CommandHandle->SendString);//发送的消息
    OSTimeDlyHMSM(0, 0, 1, CommandHandle->delayMs);
    while(1)
    {
        if (waitCnt < 5 && retryCnt <= CommandHandle->retryTimes)
        {
            
            OSQQuery(CommandHandle->MsgQ, &ReceiveMsgQData);
            if(ReceiveMsgQData.OSNMsgs > 0)
            {
                receiveString = OSQPend(CommandHandle->MsgQ, 0, &error);		//消息出队
                if(strstr((char*)receiveString, CommandHandle->ReturnString) != NULL)		//如果消息匹配
                {
                    ClearStringBuff(receiveString, GPRS_RECEIVE_STRING_BUFFER_LENGTH);                       //清空消息缓冲区
                    USARTSendString(CommandHandle->DEBUGUSART, (u8*)CommandHandle->DebugString);           //根据参数显示返回成功信息
                    returnStatus = ERROR_NONE;
                    break;
                }
                else
                {
                    //这里可以加入识别返回字符串的代码
                    ClearStringBuff(receiveString, GPRS_RECEIVE_STRING_BUFFER_LENGTH);  
                    returnStatus = ERROR_MISMACHING;
                }
            }
            waitCnt++;
            OSTimeDlyHMSM(0, 0, 0, CommandHandle->delayMs);
        }
        else if (retryCnt < CommandHandle->retryTimes)//超过等待时间
        {
            waitCnt = 0;//等待时间清0
            retryCnt++;
            USARTSendString(CommandHandle->GPRSUSART, (u8*)CommandHandle->SendString);//重新发送消息
            CommandHandle->DebugString = "Command retrying\r\n";
            USARTSendString(CommandHandle->DEBUGUSART, (u8*)CommandHandle->DebugString);   
            OSTimeDlyHMSM(0, 0, 0, CommandHandle->delayMs);                   
        }
        else
        {
            CommandHandle->DebugString = "Failed\r\n";
            USARTSendString(CommandHandle->DEBUGUSART, (u8*)CommandHandle->DebugString);   
            returnStatus = ERROR_OVERTIME;
            break;
        }
    }
    return returnStatus;
} 
/*****************************************************************************/
// FUNCTION NAME: SIM800A_TryToHandshake
//
// DESCRIPTION:
//  开机尝试握手
//
// CREATED: 2017-10-29 by zilin Wang
//
// FILE: network.c
/*****************************************************************************/
Status SIM800A_TryToHandshake(SIM800A_QueryTypeDef * CommandHandle)
{
    CommandHandle->SendString = "AT\r\n";
    CommandHandle->ReturnString = "OK\r\n";
    CommandHandle->DebugString = "hand shaking\r\n";
    CommandHandle->delayMs = 500;
    CommandHandle->retryTimes = 8;
    return SIM800A_SendSampleCommand(CommandHandle);
}
/*****************************************************************************/
// FUNCTION NAME: SIM800A_Reboot
//
// DESCRIPTION:
//  --
//
// CREATED: 2017-11-05 by zilin Wang
//
// FILE: SIM900A_init.c
/*****************************************************************************/
void SIM800A_Reboot(void)
{
    GPIO_SetBits(GPIOB, GPIO_Pin_14);
    OSTimeDlyHMSM(0, 0, 2, 0);
    GPIO_ResetBits(GPIOB, GPIO_Pin_14);
}
/*****************************************************************************/
// FUNCTION NAME: NetWorkSendHTTPActionCommand
//
// DESCRIPTION:
//  发送HTTP读取网页命令，该命令过于特殊：发送一个命令，返回两个数据，无法放入常规命令函数中
//
// CREATED: 2017-10-30 by zilin Wang
//
// FILE: network.c
/*****************************************************************************/
Status NetWorkSendHTTPActionCommand(SIM800A_QueryTypeDef * NetWorkCommandHandle, RequestMethod Method)
{ 
    if (Method == GET)//如果使用GET方法：
    {
        NetWorkCommandHandle->SendString = "AT+HTTPACTION=0\r\n";
        NetWorkCommandHandle->ReturnString = "OK\r\n";
        NetWorkCommandHandle->ReturnStringExt1 = "+HTTPACTION: 0,200,";//200是正常的返回网页，sim800比sim900多一个空格
        NetWorkCommandHandle->DebugString = "send action success\r\n";
        Status str1Status = SIM800A_SendSampleCommand(NetWorkCommandHandle);
        Status returnStatus;
        if (ERROR_NONE != str1Status)
        {
            NetWorkCommandHandle->DebugString = "Send action failed";
            USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString);   
            return str1Status;
        }
        OSTimeDlyHMSM(0, 0, 3, 0);
        u8 waitCnt = 0;
        u8 * receiveString;
        OS_Q_DATA ReceiveMsgQData;			//存放消息队列状态的结构
        INT8U error;
        while(1)
        {
            if (waitCnt < 5)
            {
                OSQQuery(NetWorkCommandHandle->MsgQ, &ReceiveMsgQData);
                if(ReceiveMsgQData.OSNMsgs > 0)
                {
                    receiveString = OSQPend(NetWorkCommandHandle->MsgQ, 0, &error);		//消息出队
                    if(strstr((char *)receiveString, NetWorkCommandHandle->ReturnStringExt1) != NULL)//如果包含消息
                    {
                        ClearStringBuff(receiveString, GPRS_RECEIVE_STRING_BUFFER_LENGTH);  //清空消息缓冲区
                        NetWorkCommandHandle->DebugString = "HTTP get success";
                        USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString);   
                        returnStatus = ERROR_NONE;
                        break;//如果收到了ACTION消息，直接退出循环
                    }
                    else
                    {
                        ClearStringBuff(receiveString, GPRS_RECEIVE_STRING_BUFFER_LENGTH); 
                        returnStatus = ERROR_MISMACHING;
                    }     
                }
                waitCnt++;//最多再等待5s
                OSTimeDlyHMSM(0, 0, 1, NetWorkCommandHandle->delayMs);
            }
            else//尝试次数过多
            {
                NetWorkCommandHandle->DebugString = "Failed:HTTP get failed"; //根据参数显示返回信息
                USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString);          
                returnStatus = ERROR_OVERTIME;
                break;
            }
        }
        return returnStatus;           
    }
    else if (Method == POST)//post以后再写
    {
        return ERROR_WRONGARGUMENT;
    }
    else
    {
        return ERROR_WRONGARGUMENT;
    }
}



/*****************************************************************************/
// FUNCTION NAME: NetWorkHTTPReadCommand
//
// DESCRIPTION:
//  读取http返回的字符串
//
// CREATED: 2017-10-31 by zilin Wang
//
// FILE: network.c
/*****************************************************************************/
Status NetWorkSendHTTPReadCommand(SIM800A_QueryTypeDef * NetWorkCommandHandle, u8 * returnString, int returnStringLength)
{
    //配置发送字符串
    char SendString[25];
    sprintf(SendString, "AT+HTTPREAD=0,%d\r\n", returnStringLength);
    NetWorkCommandHandle->SendString = SendString;
    NetWorkCommandHandle->ReturnString = "+HTTPREAD:";//包含即可
    NetWorkCommandHandle->DebugString = "HTTP reading..\n";
    //发送命令
    SIM800A_SendSampleCommand(NetWorkCommandHandle);
    u8 waitCnt = 0;
    Status returnStatus;
    u8 error;
    OS_Q_DATA ReceiveMsgQData;			//存放消息队列状态的结构
    u8 * receiveString;
    while(1)
    {
        if (waitCnt < 5)
        {
            OSTimeDlyHMSM(0, 0, 0, NetWorkCommandHandle->delayMs);
            OSQQuery(NetWorkCommandHandle->MsgQ, &ReceiveMsgQData);
            if(ReceiveMsgQData.OSNMsgs > 0)
            {
                receiveString = OSQPend(NetWorkCommandHandle->MsgQ, 0, &error);		//消息出队
                for (u8 i = 0; i < returnStringLength; i++)
                {
                    *returnString = *receiveString;
                    returnString++;
                    receiveString++;
                }
                ClearStringBuff(receiveString, GPRS_RECEIVE_STRING_BUFFER_LENGTH);  
                NetWorkCommandHandle->DebugString = "HTTP read success"; //根据参数显示返回信息
                USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString);        
                returnStatus = ERROR_NONE;
                break;
            }
            waitCnt++;
        }
        else//等待时间过长
        {
            NetWorkCommandHandle->DebugString = "Failed:HTTP read"; //根据参数显示返回信息
            USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString);  
            returnStatus = ERROR_OVERTIME;
            break;
        }
    }
    ClearStringQueue(NetWorkCommandHandle->MsgQ, GPRS_RECEIVE_STRING_BUFFER_LENGTH);//清空消息队列
    return returnStatus;
}
/*****************************************************************************/
// FUNCTION NAME: SIM800A_HTTPConnect
//
// DESCRIPTION:
//  建立一次HTTP连接并断开
//
// CREATED: 2017-10-29 by zilin Wang
//
// FILE: network.c
/*****************************************************************************/
Status SIM800A_HTTPConnect(SIM800A_QueryTypeDef * NetWorkCommandHandle, u8 * requestURL, u8 * returnString, int returnStringLength)
{
    Status returnStatus = ERROR_NONE;
    u8 retryCnt = 0;
    while(retryCnt < NetWorkCommandHandle->retryTimes)
    {
        do//如果任何一步有问题，直接跳出循环，结束连接
        {
            NetWorkCommandHandle->ReturnString = "OK\r\n";
            NetWorkCommandHandle->SendString = "ATE0\r\n";   
            NetWorkCommandHandle->DebugString = "HTTP 0%\r\n";    
            SIM800A_SendSampleCommand(NetWorkCommandHandle);
            NetWorkCommandHandle->SendString = "AT+CFUN=1\r\n";   
            NetWorkCommandHandle->DebugString = "HTTP 10%\r\n"; 
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;
            NetWorkCommandHandle->SendString = "AT+SAPBR=1,1\r\n";   
            NetWorkCommandHandle->DebugString = "HTTP 15%\r\n"; 
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;
            NetWorkCommandHandle->SendString = "AT+HTTPINIT\r\n";   
            NetWorkCommandHandle->DebugString = "HTTP 20%\r\n"; 
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;
            NetWorkCommandHandle->SendString = "AT+HTTPPARA=\"CID\",\"1\"\r\n";   
            NetWorkCommandHandle->DebugString = "HTTP 30%\r\n"; 
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;
            //配置URL：
            char FinalURLStr[140] = "AT+HTTPPARA=\"URL\",\"";
            char *str2 = "\"\r\n";
            if (ERROR_STACKOVERFLOW == MergeString(FinalURLStr, (char*)requestURL, 140))
            {
                USARTSendString(USART_DEBUG, "ERROR_STACKOVERFLOW when config url1\n");
            }
            if (ERROR_STACKOVERFLOW == MergeString(FinalURLStr, (char*)str2, 140))
            {
                USARTSendString(USART_DEBUG, "ERROR_STACKOVERFLOW when config url2\n");
            }
            NetWorkCommandHandle->SendString = FinalURLStr;   
            NetWorkCommandHandle->DebugString = "HTTP 40%\r\n"; 
            //USARTSendString(USART_DISPLAY, FinalURLStr);
            //发送网站url
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;
            //选择访问方法：
            if (ERROR_NONE != (returnStatus = NetWorkSendHTTPActionCommand(NetWorkCommandHandle, GET)))
                break;        
            //请求访问数据：
            if (ERROR_NONE != (returnStatus = NetWorkSendHTTPReadCommand(NetWorkCommandHandle, returnString, returnStringLength)))
                break;           
        }
        while(0);
        //结束连接：
        NetWorkCommandHandle->SendString = "AT+HTTPTERM\r\n";   
        NetWorkCommandHandle->ReturnString = "OK\r\n";
        NetWorkCommandHandle->DebugString = "HTTP 95%\r\n"; 
        SIM800A_SendSampleCommand(NetWorkCommandHandle);
        NetWorkCommandHandle->SendString = "AT+SAPBR=0,1\r\n";  
        NetWorkCommandHandle->DebugString = "HTTP 100%\r\n"; 
        SIM800A_SendSampleCommand(NetWorkCommandHandle);
        if (returnStatus == ERROR_NONE)
            return ERROR_NONE;
        else
        {
            retryCnt++;
            USARTSendString(USART_DEBUG, "HTTP retrying\n");
        }
    }
    return returnStatus;
}
/*****************************************************************************/
// FUNCTION NAME: SIM800A_TCPSendPackage
//
// DESCRIPTION:
//  TCP仅发送一个数据包，服务器无返回内容
//
// CREATED: 2017-11-02 by zilin Wang
//
// FILE: network.c
/*****************************************************************************/
Status SIM800A_TCPSendPackage(SIM800A_QueryTypeDef * NetWorkCommandHandle, u8 * data, int dataLength, u8 * IPString, int port)
{
    Status returnStatus = ERROR_NONE;
    u8 retryCnt = 0;    
    while(retryCnt < NetWorkCommandHandle->retryTimes)
    {
        do
        {
            NetWorkCommandHandle->SendString = "ATE0\r\n";
            NetWorkCommandHandle->ReturnString = "OK\r\n";
            NetWorkCommandHandle->DebugString = "TCP connecting 33%\r\n";
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;          
            NetWorkCommandHandle->SendString = "AT+CLPORT=\"TCP\",\"4000\"\r\n";
            NetWorkCommandHandle->DebugString = "TCP connecting 66%\r\n";
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;
            /******************************************************************************
            发送TCP服务器配置：
            ******************************************************************************/
            char SendString[80] = "AT+CIPSTART=\"TCP\",\"";
            //在前半句中放入url或ip：
            MergeString(SendString, (char*)IPString, 80);//此时的字符串就像这样：AT+CIPSTART=\"TCP\",\"sys.czbtzn.cn
            //在后半句中放入端口号
            char SendStringEnd[20];
            sprintf(SendStringEnd, "\",\"%d\"\r\n", port);//此时的字符串就像这样：\",\"2000\"\r\n
            //合并前半句和后半句：
            MergeString(SendString, SendStringEnd, 80);//此时的字符串就像这样：AT+CIPSTART=\"TCP\",\"sys.czbtzn.cn\",\"2000\"\r\n
            NetWorkCommandHandle->SendString = SendString;
            NetWorkCommandHandle->ReturnString = "CONNECT OK\r\n";
            NetWorkCommandHandle->DebugString = "TCP connecting 100%\r\n";
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;  
            //开始发送数据：
            if (ERROR_NONE != (returnStatus = NetworkSendTCPSendCommand(NetWorkCommandHandle, data, dataLength)))
            {
                NetWorkCommandHandle->DebugString = "TCP send failed";
                break;             
            }
        }
        while(0);
        //无论发送成功或失败，都要关闭连接：
        NetWorkCommandHandle->ReturnString = "OK\r\n";
        NetWorkCommandHandle->SendString = "AT+CIPCLOSE=1\r\n";
        NetWorkCommandHandle->DebugString = "End TCP connection 50%\r\n";
        SIM800A_SendSampleCommand(NetWorkCommandHandle);
        NetWorkCommandHandle->SendString = "AT+CIPSHUT\r\n";
        NetWorkCommandHandle->DebugString = "End TCP connection 100%\r\n";
        SIM800A_SendSampleCommand(NetWorkCommandHandle);
        if (returnStatus == ERROR_NONE)
            return ERROR_NONE;
        else
        {
            retryCnt++;
            USARTSendString(USART_DEBUG, "TCP retrying\n");
        }
    }
    return returnStatus;
}
/*****************************************************************************/
// FUNCTION NAME: NetworkTCPSendCommand
//
// DESCRIPTION:
//  TCP发送数据命令，该命令有两次交互，不能使用简单的命令发送函数
//
// CREATED: 2017-11-02 by zilin Wang
//
// FILE: network.c
/*****************************************************************************/
Status NetworkSendTCPSendCommand(SIM800A_QueryTypeDef * NetWorkCommandHandle, u8 * data, int dataLength)
{
    Status returnStatus;
    char sendString[20];
    sprintf(sendString, "AT+CIPSEND=%d\r\n", dataLength);
    NetWorkCommandHandle->SendString = sendString;
    NetWorkCommandHandle->ReturnString = "> ";
    NetWorkCommandHandle->DebugString = "Start sending data\r\n";
    do
    {
        if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle))) 
            break;    //如果第一次交互失败，则直接退出，关闭连接
        USARTSendArray(NetWorkCommandHandle->GPRSUSART, data, dataLength); //发送数据
        u8 waitCnt = 0;
        OS_Q_DATA ReceiveMsgQData;
        u8 error;
        u8 * receiveString;
        OSTimeDlyHMSM(0, 0, 0, NetWorkCommandHandle->delayMs);
        while(1)
        {
            if (waitCnt < 5)
            {         
                OSQQuery(NetWorkCommandHandle->MsgQ, &ReceiveMsgQData);
                if(ReceiveMsgQData.OSNMsgs > 0)
                {
                    receiveString = OSQPend(NetWorkCommandHandle->MsgQ, 0, &error);		//消息出队
                    if(strcmp((char *)receiveString, "SEND OK\r\n") == 0)		//如果消息匹配
                    {
                        ClearStringBuff(receiveString, GPRS_RECEIVE_STRING_BUFFER_LENGTH);                       //清空消息缓冲区
                        NetWorkCommandHandle->DebugString = "TCP success"; //根据参数显示返回信息
                        USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString);  
                        returnStatus = ERROR_NONE;
                        break;
                    }
                    else
                    {
                        ClearStringBuff(receiveString, GPRS_RECEIVE_STRING_BUFFER_LENGTH);  
                    }                
                }
                OSTimeDlyHMSM(0, 0, 0, NetWorkCommandHandle->delayMs);
                waitCnt++;
            }
            else//等待时间过长
            {
                NetWorkCommandHandle->DebugString = "Failed:send data\n"; //根据参数显示返回信息
                USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString);  
                returnStatus = ERROR_OVERTIME;
                break;
            }
        }
    }
    while(0);
    return returnStatus;
}
/*****************************************************************************/
// FUNCTION NAME: SIM800A_TCPHandleInit
//
// DESCRIPTION:
//  初始化TCP结构体，注意，返回值也是个指针，必须先分配空间才可以使用
//
// CREATED: 2017-11-22 by zilin Wang
//
// FILE: SIM800A.c
/*****************************************************************************/
void SIM800A_TCPHandleInit(TCP_TypeDef * TCPHandle, char* ipString, int port, char* data, int dataLength, char * returnData, int returnDataLength)
{
    TCPHandle->data = data;
    TCPHandle->dataLength = dataLength;
    TCPHandle->returnData = returnData;
    TCPHandle->returnDataLength = returnDataLength;
    TCPHandle->ipString = ipString;
    TCPHandle->port = port;
}
Status SIM800A_SetUpTCPConnection(SIM800A_QueryTypeDef * NetWorkCommandHandle, TCP_TypeDef * TCPHandle)
{
    Status returnStatus = ERROR_NONE;
    u8 retryCnt = 0;    
    while(retryCnt < NetWorkCommandHandle->retryTimes)
    {
        do
        {
            NetWorkCommandHandle->SendString = "ATE0\r\n";
            NetWorkCommandHandle->ReturnString = "OK\r\n";
            NetWorkCommandHandle->DebugString = "TCP connecting 33%\r\n";
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;          
            NetWorkCommandHandle->SendString = "AT+CLPORT=\"TCP\",\"4000\"\r\n";
            NetWorkCommandHandle->DebugString = "TCP connecting 66%\r\n";
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;
            /******************************************************************************
            发送TCP服务器配置：
            ******************************************************************************/
            char SendString[80] = "AT+CIPSTART=\"TCP\",\"";
            //在前半句中放入url或ip：
            MergeString(SendString, TCPHandle->ipString, 80);//此时的字符串就像这样：AT+CIPSTART=\"TCP\",\"sys.czbtzn.cn
            //在后半句中放入端口号
            char SendStringEnd[20];
            sprintf(SendStringEnd, "\",\"%d\"\r\n", TCPHandle->port);//此时的字符串就像这样：\",\"2000\"\r\n
            //合并前半句和后半句：
            MergeString(SendString, SendStringEnd, 80);//此时的字符串就像这样：AT+CIPSTART=\"TCP\",\"sys.czbtzn.cn\",\"2000\"\r\n
            NetWorkCommandHandle->SendString = SendString;
            NetWorkCommandHandle->ReturnString = "CONNECT OK\r\n";
            NetWorkCommandHandle->DebugString = "TCP connecting 100%\r\n";
            if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle)))
                break;  
        }
        while(0);      
        if (returnStatus == ERROR_NONE)
            return ERROR_NONE;
        else
        {
            //如果无法连接，则关闭TCP
            NetWorkCommandHandle->SendString = "AT+CIPSHUT\r\n";
            NetWorkCommandHandle->DebugString = "TCP retrying\n";
            USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString); 
            SIM800A_SendSampleCommand(NetWorkCommandHandle);
            retryCnt++;
        }
    }
    return returnStatus;
}
/*****************************************************************************/
// FUNCTION NAME: SIM800A_TCPCommunication
//
// DESCRIPTION:
//  TCP进行一次交互，注意，如果失败不会重试
//
// CREATED: 2017-11-22 by zilin Wang
//
// FILE: SIM800A.c
/*****************************************************************************/
Status SIM800A_TCPCommunication(SIM800A_QueryTypeDef * NetWorkCommandHandle, TCP_TypeDef * TCPHandle)
{
    //验证参数正确性：
    if (TCPHandle->data == NULL)
        return ERROR_NULLPOINTER;
    Status returnStatus;
    char sendString[30];
    sprintf(sendString, "AT+CIPSEND=%d\r\n", TCPHandle->dataLength);
    NetWorkCommandHandle->SendString = sendString;
    NetWorkCommandHandle->ReturnString = "> ";
    NetWorkCommandHandle->DebugString = "Start sending data\r\n";
    do
    {
        if (ERROR_NONE != (returnStatus = SIM800A_SendSampleCommand(NetWorkCommandHandle))) 
            break;    //如果第一次交互失败，则直接退出，关闭连接
        USARTSendArray(NetWorkCommandHandle->GPRSUSART, (u8*)TCPHandle->data, TCPHandle->dataLength); //发送数据
        u8 waitCnt = 0;
        OS_Q_DATA ReceiveMsgQData;
        u8 error;
        u8 * receiveString;
        OSTimeDlyHMSM(0, 0, 0, NetWorkCommandHandle->delayMs);
        while(1)
        {
            if (waitCnt < 5)
            {         
                OSQQuery(NetWorkCommandHandle->MsgQ, &ReceiveMsgQData);
                if(ReceiveMsgQData.OSNMsgs > 0)
                {
                    receiveString = OSQPend(NetWorkCommandHandle->MsgQ, 0, &error);		//消息出队
                    if(strcmp((char *)receiveString, "SEND OK\r\n") == 0)		//如果消息匹配
                    {
                        ClearStringBuff(receiveString, GPRS_RECEIVE_STRING_BUFFER_LENGTH);                       //清空消息缓冲区
                        NetWorkCommandHandle->DebugString = "TCP success\n"; //根据参数显示返回信息
                        USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString);  
                        returnStatus = ERROR_NONE;
                        break;
                    }
                    else
                    {
                        ClearStringBuff(receiveString, GPRS_RECEIVE_STRING_BUFFER_LENGTH);  
                    }                
                }
                OSTimeDlyHMSM(0, 0, 0, NetWorkCommandHandle->delayMs);
                waitCnt++;
            }
            else//等待时间过长
            {
                NetWorkCommandHandle->DebugString = "Failed:send data"; //根据参数显示返回信息
                USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString);  
                returnStatus = ERROR_OVERTIME;
                break;
            }
        } 
        if (TCPHandle->returnData != NULL)//等待返回的信息（如果有的话）
        {
            waitCnt = 0; 
            while(1)
            {
                if (waitCnt < 15)
                {         
                    OSQQuery(NetWorkCommandHandle->MsgQ, &ReceiveMsgQData);
                    if(ReceiveMsgQData.OSNMsgs > 0)
                    {
                        receiveString = OSQPend(NetWorkCommandHandle->MsgQ, 0, &error);		//消息出队
                        while(*receiveString != '\n')
                        {
                            *(TCPHandle->returnData) = *receiveString;
                            TCPHandle->returnData++;
                            receiveString++;             
                        }
                        *(++TCPHandle->returnData) = '\0';
                        ClearStringBuff(receiveString, GPRS_RECEIVE_STRING_BUFFER_LENGTH);
                        returnStatus = ERROR_NONE;
                        break;
                    }
                    OSTimeDlyHMSM(0, 0, 0, NetWorkCommandHandle->delayMs);
                    waitCnt++;
                }
                else//等待时间过长
                {
                    NetWorkCommandHandle->DebugString = "Failed:TCP no response"; //根据参数显示返回信息
                    USARTSendString(NetWorkCommandHandle->DEBUGUSART, (u8*)NetWorkCommandHandle->DebugString);  
                    returnStatus = ERROR_NORESPONSE;
                    break;
                }
            }
        }
    }
    while(0);
    return returnStatus;
}
/*****************************************************************************/
// FUNCTION NAME: SIM800A_DisConnectTCPConnection
//
// DESCRIPTION:
//  断开TCP连接
//
// CREATED: 2017-11-22 by zilin Wang
//
// FILE: SIM800A.c
/*****************************************************************************/
void SIM800A_DisConnectTCPConnection(SIM800A_QueryTypeDef * NetWorkCommandHandle)
{
    NetWorkCommandHandle->retryTimes = 1;
    NetWorkCommandHandle->ReturnString = "OK\r\n";
    NetWorkCommandHandle->SendString = "AT+CIPCLOSE=1\r\n";
    NetWorkCommandHandle->DebugString = "End TCP connection 50%\r\n";
    SIM800A_SendSampleCommand(NetWorkCommandHandle);
    NetWorkCommandHandle->SendString = "AT+CIPSHUT\r\n";
    NetWorkCommandHandle->DebugString = "End TCP connection 100%\r\n";
    SIM800A_SendSampleCommand(NetWorkCommandHandle);
    NetWorkCommandHandle->retryTimes = 3;//关闭连接部分无需过多重试
}
