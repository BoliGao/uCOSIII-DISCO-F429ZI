/*
*********************************************************************************************************
*                                           LOCAL INCLUDES
*********************************************************************************************************
*/

#include "stm32f4xx_hal.h"
#include "stm32f429i_discovery.h"
#include "os.h"
#include "string.h"

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#define APP_TASK_START_STK_SIZE 128u
#define LED3_TASK_STK_SIZE 128u
#define LED4_TASK_STK_SIZE 128u
#define SEND_STRING_TASK_STK_SIZE 128u

#define APP_TASK_START_PRIO 1u
#define LED3_TASK_PRIO 2u
#define LED4_TASK_PRIO 12u
#define SEND_STRING_TASK_PRIO 22u

/*
*********************************************************************************************************
*                                           GLOBAL VARIABLES
*********************************************************************************************************
*/

static OS_TCB AppTaskStartTCB;
static OS_TCB LED3TaskTCB;
static OS_TCB LED4TaskTCB;
static OS_TCB SendStringTaskTCB;

static CPU_STK AppTaskStartStk[APP_TASK_START_STK_SIZE];
static CPU_STK LED3TaskStk[LED3_TASK_STK_SIZE];
static CPU_STK LED4TaskStk[LED4_TASK_STK_SIZE];
static CPU_STK SendStringTaskStk[SEND_STRING_TASK_STK_SIZE];

UART_HandleTypeDef huart1;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static void AppTaskStart(void *p_arg);
static void LED3Task(void *p_arg);
static void LED4Task(void *p_arg);
static void SendStringTask(void *p_arg);

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

    OSTaskCreate((OS_TCB *)&LED3TaskTCB,
                 (CPU_CHAR *)"LED3 TASK",
                 (OS_TASK_PTR)LED3Task,
                 (void *)0,
                 (OS_PRIO)LED3_TASK_PRIO,
                 (CPU_STK *)&LED3TaskStk[0],
                 (CPU_STK_SIZE)LED3_TASK_STK_SIZE / 10,
                 (CPU_STK_SIZE)LED3_TASK_STK_SIZE,
                 (OS_MSG_QTY)5u,
                 (OS_TICK)0u,
                 (void *)0,
                 (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&err);

    OSTaskCreate((OS_TCB *)&LED4TaskTCB,
                 (CPU_CHAR *)"LED4 TASK",
                 (OS_TASK_PTR)LED4Task,
                 (void *)0,
                 (OS_PRIO)LED4_TASK_PRIO,
                 (CPU_STK *)&LED4TaskStk[0],
                 (CPU_STK_SIZE)LED4_TASK_STK_SIZE / 10,
                 (CPU_STK_SIZE)LED4_TASK_STK_SIZE,
                 (OS_MSG_QTY)5u,
                 (OS_TICK)0u,
                 (void *)0,
                 (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&err);

    OSTaskCreate((OS_TCB *)&SendStringTaskTCB,
                 (CPU_CHAR *)"Send String Task",
                 (OS_TASK_PTR)SendStringTask,
                 (void *)0,
                 (OS_PRIO)SEND_STRING_TASK_PRIO,
                 (CPU_STK *)&SendStringTaskStk[0],
                 (CPU_STK_SIZE)SEND_STRING_TASK_STK_SIZE / 10,
                 (CPU_STK_SIZE)SEND_STRING_TASK_STK_SIZE,
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

static void LED3Task(void *p_arg)
{
    OS_ERR err;

    while (DEF_TRUE)
    {
        BSP_LED_Toggle(LED3);

        OSTimeDlyHMSM((CPU_INT16U)0,
                      (CPU_INT16U)0,
                      (CPU_INT16U)1u,
                      (CPU_INT32U)0,
                      (OS_OPT)OS_OPT_TIME_HMSM_STRICT,
                      (OS_ERR *)&err);

        OSTaskSemPost((OS_TCB *)&LED4TaskTCB,
                      (OS_OPT)OS_OPT_POST_NONE,
                      (OS_ERR *)&err);
    }
}

static void LED4Task(void *p_arg)
{
    OS_ERR err;
    CPU_TS ts;

    while (DEF_TRUE)
    {
        OSTaskSemPend((OS_TICK)0,
                      (OS_OPT)OS_OPT_PEND_BLOCKING,
                      (CPU_TS *)&ts,
                      (OS_ERR *)&err);

        BSP_LED_Toggle(LED4);

        OSTaskSemPost((OS_TCB *)&SendStringTaskTCB,
                      (OS_OPT)OS_OPT_POST_NONE,
                      (OS_ERR *)&err);
    }
}

static void SendStringTask(void *p_arg)
{
    OS_ERR err;
    CPU_TS ts;

    CPU_CHAR txString[] = "Hello uCOS-III!\r\n";

    while (DEF_TRUE)
    {
        OSTaskSemPend((OS_TICK)0,
                      (OS_OPT)OS_OPT_PEND_BLOCKING,
                      (CPU_TS *)&ts,
                      (OS_ERR *)&err);

        HAL_UART_Transmit(&huart1, (uint8_t *)txString, strlen(txString), 1);
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