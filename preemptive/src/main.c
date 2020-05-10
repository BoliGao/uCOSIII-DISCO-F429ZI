/*
*********************************************************************************************************
*                                           LOCAL INCLUDES
*********************************************************************************************************
*/

#include "stm32f4xx_hal.h"
#include "stm32f429i_discovery.h"
#include "stm32f429i_discovery_lcd.h"
#include "os.h"

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

/* Task Stack Size */
#define APP_TASK_START_STK_SIZE 128u
#define LED3_TASK_STK_SIZE 128u
#define LED4_TASK_STK_SIZE 128u

/* Task Priority */
#define APP_TASK_START_PRIO 1u
#define LED3_TASK_PRIO 2u
#define LED4_TASK_PRIO 3u

/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

/* Task Control Block */
static OS_TCB AppTaskStartTCB;
static OS_TCB LED3TaskTCB;
static OS_TCB LED4TaskTCB;

/* Task Stack */
static CPU_STK AppTaskStartStk[APP_TASK_START_STK_SIZE];
static CPU_STK LED3TaskStk[LED3_TASK_STK_SIZE];
static CPU_STK LED4TaskStk[LED4_TASK_STK_SIZE];

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static void AppTaskStart(void *p_arg);
static void LED3Task(void *p_arg);
static void LED4Task(void *p_arg);

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
	}
}

static void LED4Task(void *p_arg)
{
	OS_ERR err;

	while (DEF_TRUE)
	{
		BSP_LED_Toggle(LED4);

		OSTimeDlyHMSM((CPU_INT16U)0,
					  (CPU_INT16U)0,
					  (CPU_INT16U)2u,
					  (CPU_INT32U)0,
					  (OS_OPT)OS_OPT_TIME_HMSM_STRICT,
					  (OS_ERR *)&err);
	}
}