#include "CameraControl.h"

#include "UART1.h"
#include "clock.h"
#include "ti_msp_dl_config.h"
#include "ZdtStepper.h"

#include <stdbool.h>

#define CAMERA_STEPPER_ADDR_1 (1U)
#define CAMERA_STEPPER_ADDR_2 (2U)
#define CAMERA_STEPPER_ACC    (25U)
#define CAMERA_STEPPER_VEL    (1500.0f)

static volatile uint8_t g_pendingCommand;
static volatile bool g_hasPendingCommand;
static volatile uint8_t g_lastRawByte;
static uint8_t g_lastCommand = '-';
static uint8_t g_stepperRunning;

static void CameraControl_OnRxByte(uint8_t rxByte);

static void CameraControl_StartSteppers(void)
{
    ZdtStepper_EnControl(ZDT_STEPPER_BJ1, CAMERA_STEPPER_ADDR_1, true, false);
    mspm0_delay_ms(2);
    ZdtStepper_EnControl(ZDT_STEPPER_BJ2, CAMERA_STEPPER_ADDR_2, true, false);
    mspm0_delay_ms(10);
    ZdtStepper_GimbalSetVelocity(CAMERA_STEPPER_VEL, CAMERA_STEPPER_VEL);

    g_stepperRunning = 1U;
}

static void CameraControl_StopSteppers(void)
{
    ZdtStepper_GimbalEmergencyStop();
    g_stepperRunning = 0U;
}

void CameraControl_Start(void)
{
    CameraControl_StartSteppers();
    g_lastCommand = '1';
}

void CameraControl_Stop(void)
{
    CameraControl_StopSteppers();
    g_lastCommand = '2';
}

void CameraControl_Init(void)
{
    g_pendingCommand = 0U;
    g_hasPendingCommand = false;
    g_lastRawByte = 0U;
    g_lastCommand = '-';
    g_stepperRunning = 0U;

    ZdtStepper_Init();
    CameraControl_StopSteppers();
    UART_CAM_ServiceInit();
    UART_CAM_RegisterHandler(UART_CAM_MODE_CAMERA_CONTROL, CameraControl_OnRxByte);
}

void CameraControl_EnableUart(void)
{
    g_pendingCommand = 0U;
    g_hasPendingCommand = false;
    g_lastRawByte = 0U;
    UART_CAM_SelectMode(UART_CAM_MODE_CAMERA_CONTROL);
}

void CameraControl_DisableUart(void)
{
    if (UART_CAM_GetMode() == UART_CAM_MODE_CAMERA_CONTROL) {
        UART_CAM_SelectMode(UART_CAM_MODE_NONE);
    }
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
        CameraControl_Start();
    } else if (command == '2') {
        CameraControl_Stop();
    } else {
        return;
    }
}

uint8_t CameraControl_GetLastCommand(void)
{
    return g_lastCommand;
}

uint8_t CameraControl_GetPendingCommand(void)
{
    return g_pendingCommand;
}

uint8_t CameraControl_IsStepperRunning(void)
{
    return g_stepperRunning;
}

uint8_t CameraControl_GetLastRawByte(void)
{
    return g_lastRawByte;
}

int16_t CameraControl_GetLeftMotorDuty(void)
{
    return (g_stepperRunning != 0U) ? 1 : 0;
}

static void CameraControl_OnRxByte(uint8_t rxByte)
{
    g_lastRawByte = rxByte;

    /* Ignore CR/LF from serial tools and only latch valid commands. */
    if ((rxByte == '1') || (rxByte == '2')) {
        g_pendingCommand = rxByte;
        g_hasPendingCommand = true;
    }
}
