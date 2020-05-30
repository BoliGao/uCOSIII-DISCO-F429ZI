/*
*********************************************************************************************************
*                                            LOCAL INCLUDES
*********************************************************************************************************
*/

#include "main.h"

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

/* Task Stack Size */
#define TASK_STK_SIZE 256u

/* Task Priority */
#define APP_TASK_START_PRIO 1u
#define DRAW_BOARD_PRIO 2u
#define BOT_PLAYER_PRIO 3u
#define HUMAN_PLAYER_PRIO 3u

/*
*********************************************************************************************************
*                                           GLOBAL VARIABLES
*********************************************************************************************************
*/

/* Task Control Block */
static OS_TCB AppTaskStartTCB;
static OS_TCB DrawBoardTCB;
static OS_TCB BotPlayerTCB;
static OS_TCB HumanPlayerTCB;

/* Task Stack */
static CPU_STK AppTaskStartStk[TASK_STK_SIZE];
static CPU_STK DrawBoardStk[TASK_STK_SIZE];
static CPU_STK BotPlayerStk[TASK_STK_SIZE];
static CPU_STK HumanPlayerStk[TASK_STK_SIZE];

static TS_StateTypeDef TS_State;

CPU_INT08U board[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0}; //0 is empty, 1 is bot player, 2 is human player

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

/* Task Prototypes */
static void AppTaskStart(void *p_arg);
static void DrawBoard(void *p_arg);
static void BotPlayer(void *p_arg);
static void HumanPlayer(void *p_arg);

/* System Initilization Prototypes */
void SystemClock_Config(void);
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

    OSTaskCreate((OS_TCB *)&AppTaskStartTCB,
                 (CPU_CHAR *)"App Task Start",
                 (OS_TASK_PTR)AppTaskStart,
                 (void *)0,
                 (OS_PRIO)APP_TASK_START_PRIO,
                 (CPU_STK *)&AppTaskStartStk[0],
                 (CPU_STK_SIZE)TASK_STK_SIZE / 10,
                 (CPU_STK_SIZE)TASK_STK_SIZE,
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

    BSP_LED_Init(LED3);
    BSP_LED_Init(LED4);

    LCD_Init();
    Touchscreen_Calibration();

    uint8_t status = 0;
    uint16_t x, y;
    uint8_t state = 0;

    status = BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());

    if (status != TS_OK)
    {
        BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
        BSP_LCD_SetTextColor(LCD_COLOR_RED);
        BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() - 95, (uint8_t *)"ERROR", CENTER_MODE);
        BSP_LCD_DisplayStringAt(0, BSP_LCD_GetYSize() - 80, (uint8_t *)"Touchscreen cannot be initialized", CENTER_MODE);
    }
    else
    {
        OSTaskCreate((OS_TCB *)&DrawBoardTCB,
                     (CPU_CHAR *)"Draw Board Task",
                     (OS_TASK_PTR)DrawBoard,
                     (void *)0,
                     (OS_PRIO)DRAW_BOARD_PRIO,
                     (CPU_STK *)&DrawBoardStk[0],
                     (CPU_STK_SIZE)TASK_STK_SIZE / 10,
                     (CPU_STK_SIZE)TASK_STK_SIZE,
                     (OS_MSG_QTY)5u,
                     (OS_TICK)0u,
                     (void *)0,
                     (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                     (OS_ERR *)&err);

        OSTaskSemPost((OS_TCB *)&DrawBoardTCB, //Notify receive task to send next message
                      (OS_OPT)OS_OPT_POST_NONE,
                      (OS_ERR *)&err);

        OSTaskCreate((OS_TCB *)&BotPlayerTCB,
                     (CPU_CHAR *)"Bot Player Task",
                     (OS_TASK_PTR)BotPlayer,
                     (void *)0,
                     (OS_PRIO)BOT_PLAYER_PRIO,
                     (CPU_STK *)&BotPlayerStk[0],
                     (CPU_STK_SIZE)TASK_STK_SIZE / 10,
                     (CPU_STK_SIZE)TASK_STK_SIZE,
                     (OS_MSG_QTY)5u,
                     (OS_TICK)0u,
                     (void *)0,
                     (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                     (OS_ERR *)&err);

        OSTaskCreate((OS_TCB *)&HumanPlayerTCB,
                     (CPU_CHAR *)"Human Player Task",
                     (OS_TASK_PTR)HumanPlayer,
                     (void *)0,
                     (OS_PRIO)HUMAN_PLAYER_PRIO,
                     (CPU_STK *)&HumanPlayerStk[0],
                     (CPU_STK_SIZE)TASK_STK_SIZE / 10,
                     (CPU_STK_SIZE)TASK_STK_SIZE,
                     (OS_MSG_QTY)5u,
                     (OS_TICK)0u,
                     (void *)0,
                     (OS_OPT)(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                     (OS_ERR *)&err);
    }
}

/*
*********************************************************************************************************
*                                                  TASKS
*********************************************************************************************************
*/

static void DrawBoard(void *p_arg)
{
    OS_ERR err;
    CPU_TS ts;

    while (DEF_TRUE)
    {
        OSTaskSemPend((OS_TICK)0, //Wait for a notification to send next message
                      (OS_OPT)OS_OPT_PEND_BLOCKING,
                      (CPU_TS *)&ts,
                      (OS_ERR *)&err);

        BSP_LCD_Clear(LCD_COLOR_BLACK);
        BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
        BSP_LCD_DrawHLine(0, BSP_LCD_GetYSize() - 240, 240);
        BSP_LCD_DrawHLine(0, BSP_LCD_GetYSize() - 160, 240);
        BSP_LCD_DrawHLine(0, BSP_LCD_GetYSize() - 80, 240);
        BSP_LCD_DrawVLine(BSP_LCD_GetXSize() - 160, 80, 240);
        BSP_LCD_DrawVLine(BSP_LCD_GetXSize() - 80, 80, 240);

        OSTaskSemPost((OS_TCB *)&BotPlayerTCB,
                      (OS_OPT)OS_OPT_POST_NONE,
                      (OS_ERR *)&err);
    }
}

static void BotPlayer(void *p_arg)
{
    OS_ERR err;
    CPU_TS ts;
    CPU_INT08U move;

    while (DEF_TRUE)
    {
        OSTaskSemPend((OS_TICK)0, //Wait for a notification to send next message
                      (OS_OPT)OS_OPT_PEND_BLOCKING,
                      (CPU_TS *)&ts,
                      (OS_ERR *)&err);

        BSP_LCD_SetTextColor(LCD_COLOR_GREEN);

        srand(OSTickCtr);
        move = rand() % 9;

        while (board[move] > 0)
        {
            move = rand() % 9;
        }

        switch (move)
        {
        case 0:
            BSP_LCD_DrawCircle(40, 120, 20);
            break;
        case 1:
            BSP_LCD_DrawCircle(120, 120, 20);
            break;
        case 2:
            BSP_LCD_DrawCircle(200, 120, 20);
            break;
        case 3:
            BSP_LCD_DrawCircle(40, 200, 20);
            break;
        case 4:
            BSP_LCD_DrawCircle(120, 200, 20);
            break;
        case 5:
            BSP_LCD_DrawCircle(200, 200, 20);
            break;
        case 6:
            BSP_LCD_DrawCircle(40, 280, 20);
            break;
        case 7:
            BSP_LCD_DrawCircle(120, 280, 20);
            break;
        case 8:
            BSP_LCD_DrawCircle(200, 280, 20);
            break;
        }
        board[move] = 1;

        OSTaskSemPost((OS_TCB *)&HumanPlayerTCB,
                      (OS_OPT)OS_OPT_POST_NONE,
                      (OS_ERR *)&err);
    }
}

static void HumanPlayer(void *p_arg)
{
    OS_ERR err;
    CPU_TS ts;
    CPU_INT08U move;

    while (DEF_TRUE)
    {
        OSTaskSemPend((OS_TICK)0, //Wait for a notification to send next message
                      (OS_OPT)OS_OPT_PEND_BLOCKING,
                      (CPU_TS *)&ts,
                      (OS_ERR *)&err);

        while (DEF_TRUE)
        {
            BSP_TS_GetState(&TS_State);
            BSP_LCD_SetTextColor(LCD_COLOR_RED);

            if (board[0] == 0 && TS_State.TouchDetected && TS_State.X < 80 && TS_State.Y > 80 && TS_State.Y < 160)
            {
                BSP_LCD_DrawLine(20, 100, 60, 140);
                BSP_LCD_DrawLine(60, 100, 20, 140);
                board[0] = 2;
                OSTaskSemPost((OS_TCB *)&BotPlayerTCB,
                              (OS_OPT)OS_OPT_POST_NONE,
                              (OS_ERR *)&err);
            }
            else if (board[1] == 0 && TS_State.TouchDetected && TS_State.X > 80 && TS_State.X < 160 && TS_State.Y > 80 && TS_State.Y < 160)
            {
                BSP_LCD_DrawLine(100, 100, 140, 140);
                BSP_LCD_DrawLine(140, 100, 100, 140);
                board[1] = 2;
                OSTaskSemPost((OS_TCB *)&BotPlayerTCB,
                              (OS_OPT)OS_OPT_POST_NONE,
                              (OS_ERR *)&err);
            }
            else if (board[2] == 0 && TS_State.TouchDetected && TS_State.X > 160 && TS_State.X < 240 && TS_State.Y > 80 && TS_State.Y < 160)
            {
                BSP_LCD_DrawLine(180, 100, 220, 140);
                BSP_LCD_DrawLine(220, 100, 180, 140);
                board[2] = 2;
                OSTaskSemPost((OS_TCB *)&BotPlayerTCB,
                              (OS_OPT)OS_OPT_POST_NONE,
                              (OS_ERR *)&err);
            }
            else if (board[3] == 0 && TS_State.TouchDetected && TS_State.X < 80 && TS_State.Y > 160 && TS_State.Y < 240)
            {
                BSP_LCD_DrawLine(20, 180, 60, 220);
                BSP_LCD_DrawLine(60, 180, 20, 220);
                board[3] = 2;
                OSTaskSemPost((OS_TCB *)&BotPlayerTCB,
                              (OS_OPT)OS_OPT_POST_NONE,
                              (OS_ERR *)&err);
            }
            else if (board[4] == 0 && TS_State.TouchDetected && TS_State.X > 80 && TS_State.X < 160 && TS_State.Y > 160 && TS_State.Y < 240)
            {
                BSP_LCD_DrawLine(100, 180, 140, 220);
                BSP_LCD_DrawLine(140, 180, 100, 220);
                board[4] = 2;
                OSTaskSemPost((OS_TCB *)&BotPlayerTCB,
                              (OS_OPT)OS_OPT_POST_NONE,
                              (OS_ERR *)&err);
            }
            else if (board[5] == 0 && TS_State.TouchDetected && TS_State.X > 160 && TS_State.X < 240 && TS_State.Y > 160 && TS_State.Y < 240)
            {
                BSP_LCD_DrawLine(180, 180, 220, 220);
                BSP_LCD_DrawLine(220, 180, 180, 220);
                board[5] = 2;
                OSTaskSemPost((OS_TCB *)&BotPlayerTCB,
                              (OS_OPT)OS_OPT_POST_NONE,
                              (OS_ERR *)&err);
            }
            else if (board[6] == 0 && TS_State.TouchDetected && TS_State.X < 80 && TS_State.Y > 80 && TS_State.Y > 240 && TS_State.Y < 320)
            {
                BSP_LCD_DrawLine(20, 260, 60, 300);
                BSP_LCD_DrawLine(60, 260, 20, 300);
                board[6] = 2;
                OSTaskSemPost((OS_TCB *)&BotPlayerTCB,
                              (OS_OPT)OS_OPT_POST_NONE,
                              (OS_ERR *)&err);
            }
            else if (board[7] == 0 && TS_State.TouchDetected && TS_State.X > 80 && TS_State.X < 160 && TS_State.Y > 240 && TS_State.Y < 320)
            {
                BSP_LCD_DrawLine(100, 260, 140, 300);
                BSP_LCD_DrawLine(140, 260, 100, 300);
                board[7] = 2;
                OSTaskSemPost((OS_TCB *)&BotPlayerTCB,
                              (OS_OPT)OS_OPT_POST_NONE,
                              (OS_ERR *)&err);
            }
            else if (board[8] == 0 && TS_State.TouchDetected && TS_State.X > 160 && TS_State.X < 240 && TS_State.Y > 240 && TS_State.Y < 320)
            {
                BSP_LCD_DrawLine(180, 260, 220, 300);
                BSP_LCD_DrawLine(220, 260, 180, 300);
                board[8] = 2;
                OSTaskSemPost((OS_TCB *)&BotPlayerTCB,
                              (OS_OPT)OS_OPT_POST_NONE,
                              (OS_ERR *)&err);
            }

            OSTimeDlyHMSM((CPU_INT16U)0,
                          (CPU_INT16U)0,
                          (CPU_INT16U)0,
                          (CPU_INT32U)100u,
                          (OS_OPT)OS_OPT_TIME_HMSM_STRICT,
                          (OS_ERR *)&err);
        }
    }
}

/*
*********************************************************************************************************
*                                      NON-TASK FUNCTIONS
*********************************************************************************************************
*/

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
