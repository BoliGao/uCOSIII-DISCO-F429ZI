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
#define TASK_STK_SIZE 128u

#define APP_TASK_START_PRIO 1u
#define HIGH_PRIO_TASK_PRIO 2u
#define MEDIUM_PRIO_TASK_PRIO 12u
#define LOW_PRIO_TASK_PRIO 22u

UART_HandleTypeDef huart1;

/*
*********************************************************************************************************
*                                           GLOBAL VARIABLES
*********************************************************************************************************
*/

static OS_TCB AppTaskStartTCB;
static OS_TCB HighPrioTaskTCB;
static OS_TCB MediumPrioTaskTCB;
static OS_TCB LowPrioTaskTCB;

static CPU_STK AppTaskStartStk[APP_TASK_START_STK_SIZE];
static CPU_STK HighPrioTaskStk[TASK_STK_SIZE];
static CPU_STK MediumPrioTaskStk[TASK_STK_SIZE];
static CPU_STK LowPrioTaskStk[TASK_STK_SIZE];

OS_SEM testSem; //Used to protect share resource

CPU_INT32U sharedInt;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static void AppTaskStart(void *p_arg);
static void HighPrioTask(void *p_arg);
static void MediumPrioTask(void *p_arg);
static void LowPrioTask(void *p_arg);

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

    OSSemCreate((OS_SEM *)&testSem,
                (CPU_CHAR *)"TestSemaphore",
                (OS_SEM_CTR)1,
                (OS_ERR *)&err);

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

    OSTaskCreate((OS_TCB *)&HighPrioTaskTCB,
                 (CPU_CHAR *)"High Priority Task",
                 (OS_TASK_PTR)HighPrioTask,
                 (void *)0,
                 (OS_PRIO)HIGH_PRIO_TASK_PRIO,
                 (CPU_STK *)&HighPrioTaskStk[0],
                 (CPU_STK_SIZE)TASK_STK_SIZE / 10,
                 (CPU_STK_SIZE)TASK_STK_SIZE,
                 (OS_MSG_QTY)5u,
                 (OS_TICK)0u,
                 (void *)0,
                 (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&err);

    OSTaskCreate((OS_TCB *)&MediumPrioTaskTCB,
                 (CPU_CHAR *)"Medium Priority Task",
                 (OS_TASK_PTR)MediumPrioTask,
                 (void *)0,
                 (OS_PRIO)MEDIUM_PRIO_TASK_PRIO,
                 (CPU_STK *)&MediumPrioTaskStk[0],
                 (CPU_STK_SIZE)TASK_STK_SIZE / 10,
                 (CPU_STK_SIZE)TASK_STK_SIZE,
                 (OS_MSG_QTY)5u,
                 (OS_TICK)0u,
                 (void *)0,
                 (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&err);

    OSTaskCreate((OS_TCB *)&LowPrioTaskTCB,
                 (CPU_CHAR *)"Low Priority Task",
                 (OS_TASK_PTR)LowPrioTask,
                 (void *)0,
                 (OS_PRIO)LOW_PRIO_TASK_PRIO,
                 (CPU_STK *)&LowPrioTaskStk[0],
                 (CPU_STK_SIZE)TASK_STK_SIZE / 10,
                 (CPU_STK_SIZE)TASK_STK_SIZE,
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

static void HighPrioTask(void *p_arg)
{
    OS_ERR err;
    CPU_TS ts;

    CPU_CHAR txPend[] = {"HighPrioTask is waiting for the semaphore.\r\n"};
    CPU_CHAR run[] = {"HighPrioTask is running.\r\n"};
    CPU_CHAR txPost[] = {"HighPrioTask released the semaphore.\r\n"};

    while (DEF_TRUE)
    {
        HAL_UART_Transmit(&huart1, (uint8_t *)txPend, strlen(txPend), 1);

        OSSemPend((OS_SEM *)&testSem,
                  (OS_TICK)0,
                  (OS_OPT)OS_OPT_PEND_BLOCKING,
                  (CPU_TS *)&ts,
                  (OS_ERR *)&err);

        HAL_UART_Transmit(&huart1, (uint8_t *)run, strlen(run), 1);
        HAL_UART_Transmit(&huart1, (uint8_t *)txPost, strlen(txPost), 1);

        OSSemPost((OS_SEM *)&testSem,
                  (OS_OPT)OS_OPT_POST_NONE,
                  (OS_ERR *)&err);

        OSTimeDlyHMSM((CPU_INT16U)0,
                      (CPU_INT16U)0,
                      (CPU_INT16U)0,
                      (CPU_INT32U)100u,
                      (OS_OPT)OS_OPT_TIME_HMSM_STRICT,
                      (OS_ERR *)&err);
    }
}

static void MediumPrioTask(void *p_arg)
{
    OS_ERR err;

    CPU_CHAR txString[] = {"MediumPrioTask is running.\r\n"};

    while (DEF_TRUE)
    {
        HAL_UART_Transmit(&huart1, (uint8_t *)txString, strlen(txString), 1);

        OSTimeDlyHMSM((CPU_INT16U)0,
                      (CPU_INT16U)0,
                      (CPU_INT16U)0,
                      (CPU_INT32U)100u,
                      (OS_OPT)OS_OPT_TIME_HMSM_STRICT,
                      (OS_ERR *)&err);
    }
}

static void LowPrioTask(void *p_arg)
{
    OS_ERR err;
    CPU_TS ts;

    CPU_CHAR txPend[] = {"LowPrioTask is waiting for the semaphore.\r\n"};
    CPU_CHAR run[] = {"LowPrioTask is running.\r\n"};
    CPU_CHAR txPost[] = {"LowPrioTask released the semaphore.\r\n"};

    while (DEF_TRUE)
    {
        HAL_UART_Transmit(&huart1, (uint8_t *)txPend, strlen(txPend), 1);

        OSSemPend((OS_SEM *)&testSem,
                  (OS_TICK)0,
                  (OS_OPT)OS_OPT_PEND_BLOCKING,
                  (CPU_TS *)&ts,
                  (OS_ERR *)&err);

        /* simulate a situation that a shared resouce is being hold by a low-priority task while schedulings take place */
        for (sharedInt = 0; sharedInt < 300000; sharedInt++)
        {
            OSSched();
        }

        HAL_UART_Transmit(&huart1, (uint8_t *)run, strlen(run), 1);
        HAL_UART_Transmit(&huart1, (uint8_t *)txPost, strlen(txPost), 1);

        OSSemPost((OS_SEM *)&testSem,
                  (OS_OPT)OS_OPT_POST_NONE,
                  (OS_ERR *)&err);

        OSTimeDlyHMSM((CPU_INT16U)0,
                      (CPU_INT16U)0,
                      (CPU_INT16U)0,
                      (CPU_INT32U)100u,
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
    if (HAL_UART_Init(&huart1) == HAL_OK)
    {
        BSP_LED_On(LED3);
    }
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