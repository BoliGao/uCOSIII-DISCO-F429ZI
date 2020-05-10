/*
*********************************************************************************************************
*                                            LOCAL INCLUDES
*********************************************************************************************************
*/

#include "stm32f4xx_hal.h"
#include "stm32f429i_discovery_lcd.h"
#include "os.h"
#include "string.h"

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

/* Task Stack Size */
#define APP_TASK_START_STK_SIZE 256u
#define UART_TASK_STK_SIZE 256u

/* Task Priority */
#define APP_TASK_START_PRIO 1u
#define UART_RECEIVE_TASK_PRIO 2u
#define UART_TRANSMIT_TASK_PRIO 3u

/* UART and LCD Display */
#define FILE_SIZE 1047u     //need to know the file size before transmission  
#define MAX_COLUMNS 14u
#define MAX_ROWS 12

/*
*********************************************************************************************************
*                                           GLOBAL VARIABLES
*********************************************************************************************************
*/

/* Task Control Block */
static OS_TCB AppTaskStartTCB;
static OS_TCB UartTransmitTaskTCB;
static OS_TCB UartReceiveTaskTCB;

/* Task Stack */
static CPU_STK AppTaskStartStk[APP_TASK_START_STK_SIZE];
static CPU_STK UartTransmitTaskStk[UART_TASK_STK_SIZE];
static CPU_STK UartReceiveTaskStk[UART_TASK_STK_SIZE];

/* OS Kernal Objects */
OS_SEM rxSem;

/* UART */ 
UART_HandleTypeDef huart1;
uint8_t rxData[FILE_SIZE];

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

/* Task Prototypes */
static void AppTaskStart(void *p_arg);
static void UartTransmitTask(void *p_arg);
static void UartReceiveTask(void *p_arg);

/* System Initilization Prototypes */
void SystemClock_Config(void);
static void MX_USART1_UART_Init(void);
void LCD_Init(void);

/*
*********************************************************************************************************
*                                                MAIN
*********************************************************************************************************
*/

int main(void)
{
    OS_ERR err;

    OSInit(&err);

    OSSemCreate((OS_SEM *)&rxSem,
                (CPU_CHAR *)"Synchronization Semaphore",
                (OS_SEM_CTR)0,
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

    SystemClock_Config();

    MX_USART1_UART_Init();
    HAL_UART_Receive_IT(&huart1, rxData, FILE_SIZE);

    BSP_LED_Init(LED3);
    LCD_Init();

    OSTaskCreate((OS_TCB *)&UartReceiveTaskTCB,
                 (CPU_CHAR *)"Uart Receive Task",
                 (OS_TASK_PTR)UartReceiveTask,
                 (void *)0,
                 (OS_PRIO)UART_RECEIVE_TASK_PRIO,
                 (CPU_STK *)&UartReceiveTaskStk[0],
                 (CPU_STK_SIZE)UART_TASK_STK_SIZE / 10,
                 (CPU_STK_SIZE)UART_TASK_STK_SIZE,
                 (OS_MSG_QTY)5u,
                 (OS_TICK)0u,
                 (void *)0,
                 (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR *)&err);

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

static void UartReceiveTask(void *p_arg)
{
    OS_ERR err;
    CPU_TS ts;

    while (DEF_TRUE)
    {
        OSSemPend((OS_SEM *)&rxSem,     //Wait for a semaphore sent by UART interrupt
                  (OS_TICK)0,
                  (OS_OPT)OS_OPT_PEND_BLOCKING,
                  (CPU_TS *)&ts,
                  (OS_ERR *)&err);

        for (CPU_INT32U length = 0; length < FILE_SIZE; length += MAX_COLUMNS)      //Slice whole received data into message queues  
        {
            OSTaskQPost((OS_TCB *)&UartTransmitTaskTCB,     //Send a message to transmit task
                        (void *)&rxData[length],
                        (OS_MSG_SIZE)14,
                        (OS_OPT)OS_OPT_POST_FIFO,
                        (OS_ERR *)&err);

            OSTaskSemPend((OS_TICK)0,       //Wait for a notification to send next message
                          (OS_OPT)OS_OPT_PEND_BLOCKING,
                          (CPU_TS *)&ts,
                          (OS_ERR *)&err);
        }
        BSP_LED_Toggle(LED3);       //Indicate completed display
        HAL_UART_Receive_IT(&huart1, rxData, FILE_SIZE);        //Activate UART interrupt again
    }
}

static void UartTransmitTask(void *p_arg)
{
    OS_ERR err;
    CPU_INT08U *txData;
    OS_MSG_SIZE msg_size;
    CPU_TS ts;
    CPU_INT08U line = 0;

    while (DEF_TRUE)
    {
        txData = OSTaskQPend((OS_TICK)0,        //Wait for a message from receive task 
                             (OS_OPT)OS_OPT_PEND_BLOCKING,
                             (OS_MSG_SIZE *)&msg_size,
                             (CPU_TS *)&ts,
                             (OS_ERR *)&err);

        if (++line > MAX_ROWS)      //Decide which line to display 
        {
            BSP_LCD_Clear(LCD_COLOR_WHITE);
            line = 1;
        }

        BSP_LCD_DisplayStringAtLine(line, txData);      //Display the message

        OSTimeDlyHMSM((CPU_INT16U)0,
                      (CPU_INT16U)0,
                      (CPU_INT16U)1u,
                      (CPU_INT32U)0,
                      (OS_OPT)OS_OPT_TIME_HMSM_STRICT,
                      (OS_ERR *)&err);

        OSTaskSemPost((OS_TCB *)&UartReceiveTaskTCB,        //Notify receive task to send next message
                      (OS_OPT)OS_OPT_POST_NONE,
                      (OS_ERR *)&err);
    }
}

/*
*********************************************************************************************************
*                                      NON-TASK FUNCTIONS
*********************************************************************************************************
*/

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);       //STM32 general IRQ handler
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)     //Weak function called from IRQ handler when receive completed
{
    OS_ERR err;
    OSSemPost((OS_SEM *)&rxSem,
              (OS_OPT)OS_OPT_POST_1,
              (OS_ERR *)&err);
}

void HAL_Delay(uint32_t Delay)
{
    OS_ERR err;
    OSTimeDly((OS_TICK)Delay,
              (OS_OPT)OS_OPT_TIME_DLY,
              (OS_ERR *)&err);
}

/*
*********************************************************************************************************
*                                      System Initializations
*********************************************************************************************************
*/

void LCD_Init(void)
{
    BSP_LCD_Init();
    BSP_LCD_LayerDefaultInit(LCD_BACKGROUND_LAYER, LCD_FRAME_BUFFER);
    BSP_LCD_LayerDefaultInit(LCD_FOREGROUND_LAYER, LCD_FRAME_BUFFER);
    BSP_LCD_SelectLayer(LCD_FOREGROUND_LAYER);
    BSP_LCD_DisplayOn();
    BSP_LCD_Clear(LCD_COLOR_WHITE);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
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

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (huart->Instance == USART1)
    {
        /* Peripheral clock enable */
        __HAL_RCC_USART1_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**USART1 GPIO Configuration    
        PA9 ------> USART1_RX
        PA10 ------> USART1_TX 
        */
        GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
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

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    /** Configure the main internal regulator output voltage 
  */
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    /** Initializes the CPU, AHB and APB busses clocks 
  */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 180;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /** Activate the Over-Drive mode 
  */
    HAL_PWREx_EnableOverDrive();

    /** Initializes the CPU, AHB and APB busses clocks 
  */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    PeriphClkInitStruct.PLLSAI.PLLSAIN = 216;
    PeriphClkInitStruct.PLLSAI.PLLSAIR = 2;
    PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_2;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
}

