#ifndef __SIM800A_H
#define __SIM800A_H
/******************************************************************************
重启引脚宏定义
******************************************************************************/
#define SIM800A_Reboot_GPIOX GPIOB
#define SIM800A_Reboot_RCC_APBPERIPH_GPIOX RCC_APB2Periph_GPIOB
#define SIM800A_Reboot_GPIO_PIN GPIO_Pin_14
/******************************************************************************
数据缓冲区声明
******************************************************************************/
extern u8 GPRSLine, GPRSRow;			//接收的行和列计数
extern u8 GPRSReceiveBuff[10][GPRS_RECEIVE_STRING_BUFFER_LENGTH];		//GPRS接收数据缓冲区
extern u8 GPRSSendBuff[100];			//GPRS发送数据缓冲
/******************************************************************************
命令结构体声明：
******************************************************************************/
typedef struct
{
    USART_TypeDef * GPRSUSART;//发送命令串口
    USART_TypeDef * DEBUGUSART;//调试信息输出串口
    OS_EVENT * MsgQ;
    char * SendString;//发送的字符串
    char * ReturnString;//远程返回的字符串
    char * ReturnStringExt1;//远程返回的第二个字符串，需要提前建立缓冲区
    char * ReturnStringExt2;//远程返回的第三个字符串，需要提前建立缓冲区
    char * DebugString;//输出调试信息
    u8 retryTimes;//重复发送命令次数
    u16 delayMs;//发送完命令之后等待回复的时间，等待5次该时间后重复发送命令
}SIM800A_QueryTypeDef;
/******************************************************************************
TCP结构体声明：
******************************************************************************/
typedef struct
{
    char* data;
    int dataLength;
    char* returnData;
    int returnDataLength;
    char* ipString;
    int port;
}TCP_TypeDef;
/******************************************************************************
状态结构体声明
******************************************************************************/
typedef enum status
{
    ERROR_NONE = 0,
    ERROR_OVERTIME,
    ERROR_MISMACHING,
    ERROR_NULLPOINTER,
    ERROR_STACKOVERFLOW,
    ERROR_UNKNOWN,
    ERROR_WRONGARGUMENT,
    ERROR_NORESPONSE,
}Status;
/******************************************************************************
HTTP请求方法声明
******************************************************************************/
typedef enum requestMethod
{
    GET = 0,
    POST,
}RequestMethod;
/******************************************************************************
函数声明（接口）：
******************************************************************************/
//Public：
void SIM800A_RebootIOInit(void);//被放到bsp的函数中调用
void SIM800A_Reboot(void);//重启设备
void SIM800A_CommandHandleDeinit(SIM800A_QueryTypeDef * CommandHandle, OS_EVENT * MsgQ);//默认初始化设备
void SIM800A_CommandHandleInit(SIM800A_QueryTypeDef * CommandHandle, USART_TypeDef * GPRSUSART, USART_TypeDef * DEBUGUSART, OS_EVENT * MsgQ);//初始化句柄
Status SIM800A_TryToHandshake(SIM800A_QueryTypeDef * CommandHandle);//尝试与设备握手
//HTTP：
Status SIM800A_HTTPConnect(SIM800A_QueryTypeDef * NetWorkCommandHandle, u8 * requestURL, u8 * returnString, int returnStringLength);
Status SIM800A_TCPSendPackage(SIM800A_QueryTypeDef * NetWorkCommandHandle, u8 * data, int dataLength, u8 * IPString, int port);
//TCP
void SIM800A_TCPHandleInit(TCP_TypeDef * TCPHandle, char* ipString, int port, char* data, int dataLength, char * returnData, int returnDataLength);//初始化TCP句柄
Status SIM800A_SetUpTCPConnection(SIM800A_QueryTypeDef * NetWorkCommandHandle, TCP_TypeDef * TCPHandle);//建立TCP连接
Status SIM800A_TCPCommunication(SIM800A_QueryTypeDef * NetWorkCommandHandle, TCP_TypeDef * TCPHandle);//进行一次TCP交互
void SIM800A_DisConnectTCPConnection(SIM800A_QueryTypeDef * NetWorkCommandHandle);//断开TCP连接
/******************************************************************************
函数声明（内部）：
******************************************************************************/
//内建函数，对外隐藏 函数第一个单词是功能，如Network就是用于网络连接的函数，public就是公用的内建函数
static Status PublicSendSampleCommand(SIM800A_QueryTypeDef * CommandHandle);//发送一个返回OK的命令
static Status NetWorkSendHTTPActionCommand(SIM800A_QueryTypeDef * NetWorkCommandHandle, RequestMethod Method);
static Status NetWorkSendHTTPReadCommand(SIM800A_QueryTypeDef * NetWorkCommandHandle, u8 * returnString, int returnStringLength);
static Status NetworkSendTCPSendCommand(SIM800A_QueryTypeDef * NetWorkCommandHandle, u8 * data, int dataLength);
#endif