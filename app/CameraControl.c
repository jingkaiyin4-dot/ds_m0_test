#include "CameraControl.h"

#include "ti_msp_dl_config.h"
#include "Motor.h"

#include <stdbool.h>

#define CAMERA_CONTROL_LEFT_DUTY (50)

static volatile uint8_t g_pendingCommand;
static volatile bool g_hasPendingCommand;
static uint8_t g_lastCommand = '-';
static int16_t g_leftMotorDuty;

void CameraControl_Init(void)
{
    uint8_t dummy[4];

    g_pendingCommand = 0U;
    g_hasPendingCommand = false;
    g_lastCommand = '-';
    g_leftMotorDuty = 0;

    Motor_StopAll();
    DL_UART_drainRXFIFO(UART_CAM_INST, dummy, 4);
    NVIC_ClearPendingIRQ(UART_CAM_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_CAM_INST_INT_IRQN);
}

void CameraControl_Process(void)
{
    uint8_t command;

    if (!g_hasPendingCommand) {
        return;
    }

    __disable_irq();
    command = g_pendingCommand;
    g_hasPendingCommand = false;
    __enable_irq();

    if (command == '1') {
        g_leftMotorDuty = CAMERA_CONTROL_LEFT_DUTY;
        Motor_SetDuty(MOTOR_LEFT, g_leftMotorDuty);
    } else if (command == '2') {
        g_leftMotorDuty = 0;
        Motor_SetDuty(MOTOR_LEFT, 0);
    } else {
        return;
    }

    g_lastCommand = command;
}

uint8_t CameraControl_GetLastCommand(void)
{
    return g_lastCommand;
}

int16_t CameraControl_GetLeftMotorDuty(void)
{
    return g_leftMotorDuty;
}

void UART_CAM_INST_IRQHandler(void)
{
    while (DL_UART_Main_isRXFIFOEmpty(UART_CAM_INST) == false) {
        g_pendingCommand = DL_UART_Main_receiveData(UART_CAM_INST);
        g_hasPendingCommand = true;
    }
}
