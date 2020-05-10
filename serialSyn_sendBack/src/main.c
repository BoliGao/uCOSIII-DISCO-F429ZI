/*
*********************************************************************************************************
*                                            LOCAL INCLUDES
*********************************************************************************************************
*/

#include "stm32f4xx_hal.h"
#include "stm32f429i_discovery.h"
#include "os.h"

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define APP_TASK_START_STK_SIZE 128u
#define UART_TASK_STK_SIZE 128u

#define APP_TASK_START_PRIO 1u
#define UART_TRANSMIT_TASK_PRIO 12u

#define MAX_SIZE 65535u //Maximum UART transmission size

/*
*********************************************************************************************************
*                                           GLOBAL VARIABLES
*********************************************************************************************************
*/

static OS_TCB AppTaskStartTCB;
static OS_TCB UartTransmitTaskTCB;

static CPU_STK AppTaskStartStk[APP_TASK_START_STK_SIZE];
static CPU_STK UartTransmitTaskStk[UART_TASK_STK_SIZE];

UART_HandleTypeDef huart1;

CPU_INT08U rxData[MAX_SIZE];
CPU_INT16U rxLen;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static void AppTaskStart(void *p_arg);
static void UartTransmitTask(void *p_arg);

static void MX_USART1_UART_Init(void);
void USART1_IRQHandler(void);

/*
*********************************************************************************************************
*                                                MAIN
*********************************************************************************************************
*/

int main(void)
{
    OS_ERR err;

    OSInit(&err);

    OSTaskCreate((OS_TCB *)&AppTaskStartTCB,
                 (CPU_CHAR *)"App Task Start",
                 (OS_TASK_PTR)AppTaskStart,
                 (void *)0,
                 (OS_PRIO)APP_TASK_START_PRIO,
                 (CPU_STK *)&AppTaskStartStk[0],
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE / 10,
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY)5u,
                 (OS_TICK)0u,
                 (void *)0,
                 (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&err);

    OSStart(&err);
}

/*
*********************************************************************************************************
*                                              STARTUP TASK
*********************************************************************************************************
*/

static void AppTaskStart(void *p_arg)
{
    OS_ERR err;

    HAL_Init();

    BSP_LED_Init(LED3);
    BSP_LED_Init(LED4);
    MX_USART1_UART_Init();

    __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE); //Enable UART IDLE Interrupt
    HAL_UART_Receive_IT(&huart1, rxData, MAX_SIZE);

    OSTaskCreate((OS_TCB *)&UartTransmitTaskTCB,
                 (CPU_CHAR *)"Uart Transmit Task",
                 (OS_TASK_PTR)UartTransmitTask,
                 (void *)0,
                 (OS_PRIO)UART_TRANSMIT_TASK_PRIO,
                 (CPU_STK *)&UartTransmitTaskStk[0],
                 (CPU_STK_SIZE)UART_TASK_STK_SIZE / 10,
                 (CPU_STK_SIZE)UART_TASK_STK_SIZE,
                 (OS_MSG_QTY)5u,
                 (OS_TICK)0u,
                 (void *)0,
                 (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&err);
}

/*
*********************************************************************************************************
*                                                  TASKS
*********************************************************************************************************
*/

static void UartTransmitTask(void *p_arg)
{
    OS_ERR err;
    CPU_INT08U *txData;
    OS_MSG_SIZE msg_size;
    CPU_TS ts;

    while (DEF_TRUE)
    {
        txData = OSTaskQPend((OS_TICK)0, //Wait for messages from UART interrupt
                             (OS_OPT)OS_OPT_PEND_BLOCKING,
                             (OS_MSG_SIZE *)&msg_size,
                             (CPU_TS *)&ts,
                             (OS_ERR *)&err);

        HAL_UART_Transmit_IT(&huart1, txData, msg_size); //Transmit back
        HAL_UART_Receive_IT(&huart1, rxData, MAX_SIZE);  //Make ready for next interrupt
    }
}

/*
*********************************************************************************************************
*                                      NON-TASK FUNCTIONS
*********************************************************************************************************
*/

void USART1_IRQHandler(void)
{
    OS_ERR err;

    uint32_t isrflags = READ_REG(huart1.Instance->SR);
    uint32_t cr1its = READ_REG(huart1.Instance->CR1);

    /* IDLE interrupt detection and handler to achieve unknown size transmission */
    if (((isrflags & USART_SR_IDLE) != RESET) && ((cr1its & USART_CR1_IDLEIE)))
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);
        huart1.RxState = HAL_UART_STATE_READY;

        rxLen = MAX_SIZE - huart1.RxXferCount;
        OSTaskQPost((OS_TCB *)&UartTransmitTaskTCB,
                    (void *)rxData,
                    (OS_MSG_SIZE)rxLen,
                    (OS_OPT)OS_OPT_POST_FIFO,
                    (OS_ERR *)&err);
    }

    HAL_UART_IRQHandler(&huart1);
}

static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);
}

/** 
 * This UART Callback function configures the hardware resources for UART Initialization 
**/
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (huart->Instance == USART1)
    {
        /* Peripheral clock enable */
        __HAL_RCC_USART1_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**USART1 GPIO Configuration    
        PA9     ------> USART1_RX
        PA10     ------> USART1_TX 
        */
        GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* USART1 interrupt Init */
        HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
}