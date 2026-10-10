/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
#include "Button.h"        /* 按键宏 + 函数声明 */
#include "FreeRTOS.h"
#include "task.h"          /* xTaskNotifyFromISR */

/** @addtogroup STM32F10x_StdPeriph_Template
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
//void SVC_Handler(void)
//{
//}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
//void PendSV_Handler(void)
//{
//}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
//void SysTick_Handler(void)
//{
//}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/******************************************************************************/
/*                 EXTI 外部中断处理函数（按键）                               */
/******************************************************************************/

/*
 * 消抖时间戳 — volatile 防止编译器优化掉对它的读取
 * 中断里写、主循环里读，不加 volatile 编译器可能把值缓存到寄存器
 */
static volatile TickType_t xLastLeftTick  = 0;
static volatile TickType_t xLastRightTick = 0;

/**
  * @brief  EXTI Line 0 中断 — PB0 左键
  *         流程：确认中断源 → 消抖 → 发任务通知 → 清中断标志
  */
void EXTI0_IRQHandler(void)
{
    /* 第 1 步：确认确实是 EXTI0 触发的（防止误入） */
    if (EXTI_GetITStatus(EXTI_Line0) != RESET)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        TaskHandle_t handle = Button_GetDisplayTaskHandle();

        /* 第 2 步：双重确认引脚确实是低电平（真的按下了） */
        if (handle != NULL && Button_IsLeftPressed())
        {
            TickType_t now = xTaskGetTickCountFromISR();

            /* 第 3 步：消抖 — 20ms 内的重复触发直接丢弃 */
            if ((now - xLastLeftTick) >= pdMS_TO_TICKS(200))
            {
                xLastLeftTick = now;

                /* 第 4 步：发任务通知，bit0 置 1，唤醒 Display 任务 */
                xTaskNotifyFromISR(handle, BTN_LEFT, eSetBits,
                                   &xHigherPriorityTaskWoken);
            }
        }

        /* 第 5 步：清除中断挂起位（必须！否则中断反复触发） */
        EXTI_ClearITPendingBit(EXTI_Line0);

        /* 第 6 步：如果被唤醒的任务优先级更高，立即切换 */
        if (xHigherPriorityTaskWoken == pdTRUE)
        {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

/**
  * @brief  EXTI Line 1 中断 — PB1 右键
  *         与 EXTI0 完全对称：Line0→Line1, LEFT→RIGHT
  */
void EXTI1_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line1) != RESET)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        TaskHandle_t handle = Button_GetDisplayTaskHandle();

        if (handle != NULL && Button_IsRightPressed())
        {
            TickType_t now = xTaskGetTickCountFromISR();

            if ((now - xLastRightTick) >= pdMS_TO_TICKS(200))
            {
                xLastRightTick = now;

                xTaskNotifyFromISR(handle, BTN_RIGHT, eSetBits,
                                   &xHigherPriorityTaskWoken);
            }
        }

        EXTI_ClearITPendingBit(EXTI_Line1);

        if (xHigherPriorityTaskWoken == pdTRUE)
        {
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

/**
  * @}
  */


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
