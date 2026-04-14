#include "ti_msp_dl_config.h"

#if 0

uint8_t Serial_TxPacket[4];         // 定义发送数据包数组，数据包格式：FF 01 02 03 04 FE
uint8_t Serial_RxPacket[4];         // 定义接收数据包数组（16进制）
uint32_t Serial_RxPacket_Decimal[4]; // 新增：定义接收数据包数组（10进制）
uint8_t Serial_RxFlag;              // 定义接收数据包标志位
int a;
void UART1_Init()
{
  

    SYSCFG_DL_UART_1l_init(); // 添加硬件配置
    NVIC_EnableIRQ(UART_1l_INST_INT_IRQN);

    NVIC_ClearPendingIRQ(UART_1l_INST_INT_IRQN);
   
}

void UART_1l_INST_IRQHandler(void)
{
    static uint8_t RxState = 0;     // 定义表示当前状态机状态的静态变量
    static uint8_t pRxPacket = 0;   // 定义表示当前接收数据位置的静态变量
     a++;
    uint8_t RxData = DL_UART_Main_receiveData(UART_1l_INST);                
        
    if (RxState == 0)
    {
        if (RxData == 0xFF)         // 如果数据确实是包头
        {
            RxState = 1;            // 置下一个状态
            pRxPacket = 0;          // 数据包的位置归零
        }
    }
    /* 当前状态为1，接收数据包数据 */
    else if (RxState == 1)
    {
        Serial_RxPacket[pRxPacket] = RxData;    // 将数据存入数据包数组的指定位置
        pRxPacket++;                // 数据包的位置自增
        if (pRxPacket >= 4)         // 如果收够4个数据
        {
            RxState = 2;            // 置下一个状态
        }
    }
    /* 当前状态为2，接收数据包包尾 */
    else if (RxState == 2)
    {
        if (RxData == 0xFE)         // 如果数据确实是包尾部
        {
            RxState = 0;            // 状态归0
            
            // 新增：将16进制数据转换为10进制
            for (uint8_t i = 0; i < 4; i++)
            {
                Serial_RxPacket_Decimal[i] = (uint32_t)Serial_RxPacket[i];
            }
            
            Serial_RxFlag = 1;      // 接收数据包标志位置1，成功接收一个数据包
        }
    }
}

#endif
