/*
*********************************************************************************************************
*                                           LOCAL INCLUDES
*********************************************************************************************************
*/

#include "stm32f4xx_hal.h"
#include "stm32f429i_discovery.h"
#include "os.h"
#include "string.h"
#include "stdio.h"

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define APP_TASK_START_STK_SIZE 128u
#define TASK_A_STK_SIZE 128u
#define TASK_B_STK_SIZE 128u

#define APP_TASK_START_PRIO 1u
#define TASK_A_PRIO 2u
#define TASK_B_PRIO 3u

/*
*********************************************************************************************************
*                                           GLOBAL VARIABLES
*********************************************************************************************************
*/

static OS_TCB AppTaskStartTCB;
static OS_TCB TaskATCB;
static OS_TCB TaskBTCB;

static CPU_STK AppTaskStartStk[APP_TASK_START_STK_SIZE];
static CPU_STK TaskAStk[TASK_A_STK_SIZE];
static CPU_STK TaskBStk[TASK_B_STK_SIZE];

UART_HandleTypeDef huart1;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static void AppTaskStart(void *p_arg);
static void TaskA(void *p_arg);
static void TaskB(void *p_arg);

static void MX_USART1_UART_Init(void);

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

    OSTaskCreate((OS_TCB *)&TaskATCB,
                 (CPU_CHAR *)"TaskA",
                 (OS_TASK_PTR)TaskA,
                 (void *)0,
                 (OS_PRIO)TASK_A_PRIO,
                 (CPU_STK *)&TaskAStk[0],
                 (CPU_STK_SIZE)TASK_A_STK_SIZE / 10,
                 (CPU_STK_SIZE)TASK_A_STK_SIZE,
                 (OS_MSG_QTY)5u,
                 (OS_TICK)0u,
                 (void *)0,
                 (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&err);

    OSTaskCreate((OS_TCB *)&TaskBTCB,
                 (CPU_CHAR *)"TaskB",
                 (OS_TASK_PTR)TaskB,
                 (void *)0,
                 (OS_PRIO)TASK_B_PRIO,
                 (CPU_STK *)&TaskBStk[0],
                 (CPU_STK_SIZE)TASK_B_STK_SIZE / 10,
                 (CPU_STK_SIZE)TASK_B_STK_SIZE,
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

static void TaskA(void *p_arg)
{
    OS_ERR err;
    CPU_INT08U cntA = 0;
    CPU_CHAR cntAsent[30];
    CPU_INT08U *cntB;
    OS_MSG_SIZE msg_size;
    CPU_TS ts;

    while (DEF_TRUE)
    {
        cntA++;
        sprintf(cntAsent, "TaskA counts: %d\r\n", cntA);
        
        OSTaskQPost((OS_TCB *)&TaskBTCB,
                    (void *)cntAsent,
                    (OS_MSG_SIZE)strlen(cntAsent),
                    (OS_OPT)OS_OPT_POST_FIFO,
                    (OS_ERR *)&err);

        cntB = OSTaskQPend((OS_TICK)0,
                           (OS_OPT)OS_OPT_PEND_BLOCKING,
                           (OS_MSG_SIZE *)&msg_size,
                           (CPU_TS *)&ts,
                           (OS_ERR *)&err);

        HAL_UART_Transmit(&huart1, cntB, msg_size, 1);

        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        OSTimeDlyHMSM((CPU_INT16U)0,
                      (CPU_INT16U)0,
                      (CPU_INT16U)1u,
                      (CPU_INT32U)0,
                      (OS_OPT)OS_OPT_TIME_HMSM_STRICT,
                      (OS_ERR *)&err);
    }
}

static void TaskB(void *p_arg)
{
    OS_ERR err;
    CPU_INT08U cntB = 0;
    CPU_CHAR cntBsent[30];
    CPU_INT08U *cntA;
    OS_MSG_SIZE msg_size;
    CPU_TS ts;

    while (DEF_TRUE)
    {
        cntA = OSTaskQPend((OS_TICK)0,
                           (OS_OPT)OS_OPT_PEND_BLOCKING,
                           (OS_MSG_SIZE *)&msg_size,
                           (CPU_TS *)&ts,
                           (OS_ERR *)&err);

        HAL_UART_Transmit(&huart1, cntA, msg_size, 1);

        cntB++;
        sprintf(cntBsent, "TaskB counts: %d\r\n", cntB);

        OSTaskQPost((OS_TCB *)&TaskATCB,
                    (void *)cntBsent,
                    (OS_MSG_SIZE)strlen(cntBsent),
                    (OS_OPT)OS_OPT_POST_FIFO,
                    (OS_ERR *)&err);

        OSTimeDlyHMSM((CPU_INT16U)0,
                      (CPU_INT16U)0,
                      (CPU_INT16U)1u,
                      (CPU_INT32U)0,
                      (OS_OPT)OS_OPT_TIME_HMSM_STRICT,
                      (OS_ERR *)&err);
    }
}
/*
*********************************************************************************************************
*                                      NON-TASK FUNCTIONS
*********************************************************************************************************
*/

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
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX 
    */
        GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}