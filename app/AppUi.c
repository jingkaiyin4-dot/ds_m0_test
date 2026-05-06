#include "AppUi.h"

#include "../Debug/ti_msp_dl_config.h"
#include "../Drivers/BNO08X_UART_RVC/bno08x_uart_rvc.h"
#include "../Drivers/MPU6050/mpu6050.h"
#include "../Drivers/TOF400F/tof400f.h"
#include "../PID/PID.h"
#include "../encoder/Encoder.h"
#include "../main.h"

#include <stdio.h>

#define APP_UI_KEY_DEBOUNCE_MS 30U
#define APP_UI_KEY_LONG_PRESS_MS 600U
#define APP_UI_RENDER_INTERVAL_MS 80U
#define APP_UI_TUNE_STEP_MEDIAN 0.05f
#define APP_UI_TUNE_STEP_GAIN 0.05f

typedef enum {
    APP_UI_PAGE_RUN = 0,
    APP_UI_PAGE_IMU,
    APP_UI_PAGE_MOTOR,
    APP_UI_PAGE_TUNE,
    APP_UI_PAGE_ACTION,
    APP_UI_PAGE_COUNT,
} AppUiPage;

typedef enum {
    APP_UI_TUNE_MEDIAN = 0,
    APP_UI_TUNE_BAL_KP,
    APP_UI_TUNE_BAL_KD,
    APP_UI_TUNE_COUNT,
} AppUiTuneItem;

typedef enum {
    APP_UI_ACTION_TOGGLE_IMU = 0,
    APP_UI_ACTION_TOGGLE_PID,
    APP_UI_ACTION_RESET_DRIVE,
    APP_UI_ACTION_TOF_QUERY,
    APP_UI_ACTION_COUNT,
} AppUiAction;

typedef struct {
    uint8_t pressed;
    uint8_t clicked;
    uint8_t longClicked;
    uint8_t lastSamplePressed;
    uint8_t longReported;
    uint32_t stableSinceMs;
    uint32_t pressStartMs;
} AppUiButton;

typedef struct {
    AppUiPage page;
    uint8_t tuneEditing;
    uint8_t tuneIndex;
    uint8_t actionIndex;
    uint32_t lastRenderMs;
    uint8_t forceRender;
} AppUiState;

static AppUiState g_appUi;
static AppUiButton g_keyMode;
static AppUiButton g_keyUp;
static AppUiButton g_keyDown;

static uint16_t g_oledThemeFg;
static uint16_t g_oledThemeBg;

static uint8_t AppUi_ReadKey(GPIO_Regs *port, uint32_t pin)
{
    return (DL_GPIO_readPins(port, pin) == 0U) ? 1U : 0U;
}

static void AppUi_UpdateButton(AppUiButton *button, uint8_t rawPressed, uint32_t nowMs)
{
    button->clicked = 0U;
    button->longClicked = 0U;

    if (rawPressed != button->lastSamplePressed) {
        button->lastSamplePressed = rawPressed;
        button->stableSinceMs = nowMs;
    }

    if ((nowMs - button->stableSinceMs) >= APP_UI_KEY_DEBOUNCE_MS) {
        if (button->pressed != rawPressed) {
            button->pressed = rawPressed;
            if (rawPressed != 0U) {
                button->pressStartMs = nowMs;
                button->longReported = 0U;
                /* 修改：在按键按下的瞬间立刻触发 clicked 事件，提升灵敏度 */
                button->clicked = 1U;
            }
        }
    }

    if ((button->pressed != 0U) && (button->longReported == 0U) &&
        ((nowMs - button->pressStartMs) >= APP_UI_KEY_LONG_PRESS_MS)) {
        button->longClicked = 1U;
        button->longReported = 1U;
    }
}

static void AppUi_SelectPrevPage(void)
{
    if (g_appUi.page == APP_UI_PAGE_RUN) {
        g_appUi.page = (AppUiPage)(APP_UI_PAGE_COUNT - 1U);
    } else {
        g_appUi.page = (AppUiPage)(g_appUi.page - 1U);
    }
    g_appUi.tuneEditing = 0U;
    g_appUi.forceRender = 1U;
}

static void AppUi_SelectNextPage(void)
{
    g_appUi.page = (AppUiPage)((g_appUi.page + 1U) % APP_UI_PAGE_COUNT);
    g_appUi.tuneEditing = 0U;
    g_appUi.forceRender = 1U;
}

static void AppUi_AdjustTuneValue(int8_t direction)
{
    if (direction == 0) {
        return;
    }

    switch ((AppUiTuneItem)g_appUi.tuneIndex) {
        case APP_UI_TUNE_MEDIAN:
            g_driveController.balance.mechanicalMedian +=
                (float)direction * APP_UI_TUNE_STEP_MEDIAN;
            break;
        case APP_UI_TUNE_BAL_KP:
            g_driveController.balance.pid.kp +=
                (float)direction * APP_UI_TUNE_STEP_GAIN;
            break;
        case APP_UI_TUNE_BAL_KD:
            g_driveController.balance.pid.kd +=
                (float)direction * APP_UI_TUNE_STEP_GAIN;
            break;
        default:
            break;
    }

    g_appUi.forceRender = 1U;
}

static void AppUi_RunAction(void)
{
    switch ((AppUiAction)g_appUi.actionIndex) {
        case APP_UI_ACTION_TOGGLE_IMU:
            App_SetImuSource((uint8_t)(App_GetImuSource() == 0U));
            g_appUi.forceRender = 1U;
            break;
        case APP_UI_ACTION_TOGGLE_PID:
            if (App_IsDrivePidActive() != 0U) {
                App_StopDrivePidTest();
            } else {
                App_StartDrivePidTest();
            }
            g_appUi.forceRender = 1U;
            break;
        case APP_UI_ACTION_RESET_DRIVE:
            App_ResetDriveState();
            g_appUi.forceRender = 1U;
            break;
        case APP_UI_ACTION_TOF_QUERY:
            if (App_IsTofUartEnabled() != 0U) {
                TOF400F_Query();
            }
            g_appUi.forceRender = 1U;
            break;
        default:
            break;
    }
}

static void AppUi_HandleButtons(uint32_t nowMs)
{
    AppUi_UpdateButton(&g_keyMode,
                       AppUi_ReadKey(GPIO_KEY_PIN_KEY_MODE_PORT,
                                     GPIO_KEY_PIN_KEY_MODE_PIN),
                       nowMs);
    AppUi_UpdateButton(&g_keyUp,
                       AppUi_ReadKey(GPIO_KEY_PIN_KEY_UP_PORT,
                                     GPIO_KEY_PIN_KEY_UP_PIN),
                       nowMs);
    AppUi_UpdateButton(&g_keyDown,
                       AppUi_ReadKey(GPIO_KEY_PIN_KEY_DOWN_PORT,
                                     GPIO_KEY_PIN_KEY_DOWN_PIN),
                       nowMs);

    switch (g_appUi.page) {
        case APP_UI_PAGE_TUNE:
            if (g_keyMode.clicked != 0U) {
                g_appUi.tuneEditing ^= 1U;
                g_appUi.forceRender = 1U;
            }
            if (g_keyUp.longClicked != 0U) {
                g_appUi.tuneIndex = (uint8_t)((g_appUi.tuneIndex + APP_UI_TUNE_COUNT - 1U) %
                                              APP_UI_TUNE_COUNT);
                g_appUi.forceRender = 1U;
            }
            if (g_keyDown.longClicked != 0U) {
                g_appUi.tuneIndex = (uint8_t)((g_appUi.tuneIndex + 1U) % APP_UI_TUNE_COUNT);
                g_appUi.forceRender = 1U;
            }
            if (g_appUi.tuneEditing != 0U) {
                if (g_keyUp.clicked != 0U) {
                    AppUi_AdjustTuneValue(1);
                }
                if (g_keyDown.clicked != 0U) {
                    AppUi_AdjustTuneValue(-1);
                }
            } else {
                if (g_keyUp.clicked != 0U) {
                    g_appUi.tuneIndex = (uint8_t)((g_appUi.tuneIndex + APP_UI_TUNE_COUNT - 1U) %
                                                  APP_UI_TUNE_COUNT);
                    g_appUi.forceRender = 1U;
                }
                if (g_keyDown.clicked != 0U) {
                    g_appUi.tuneIndex = (uint8_t)((g_appUi.tuneIndex + 1U) % APP_UI_TUNE_COUNT);
                    g_appUi.forceRender = 1U;
                }
            }
            break;

        case APP_UI_PAGE_ACTION:
            if (g_keyUp.clicked != 0U) {
                g_appUi.actionIndex = (uint8_t)((g_appUi.actionIndex + APP_UI_ACTION_COUNT - 1U) %
                                                APP_UI_ACTION_COUNT);
                g_appUi.forceRender = 1U;
            }
            if (g_keyDown.clicked != 0U) {
                g_appUi.actionIndex = (uint8_t)((g_appUi.actionIndex + 1U) % APP_UI_ACTION_COUNT);
                g_appUi.forceRender = 1U;
            }
            if (g_keyMode.clicked != 0U) {
                AppUi_RunAction();
            }
            break;

        default:
            if (g_keyUp.longClicked != 0U) {
                AppUi_SelectPrevPage();
            } else if (g_keyUp.clicked != 0U) {
                AppUi_SelectPrevPage();
            }
            if (g_keyDown.longClicked != 0U) {
                AppUi_SelectNextPage();
            } else if (g_keyDown.clicked != 0U) {
                AppUi_SelectNextPage();
            }
            if (g_keyMode.longClicked != 0U) {
                AppUi_SelectNextPage();
            } else if (g_keyMode.clicked != 0U) {
                AppUi_SelectNextPage();
            }
            break;
    }
}

static void AppUi_ShowLine(uint8_t y, const char *text)
{
    OLED_ShowString(0U, y, (uint8_t *)text, 8);
}

static void AppUi_RenderRunPage(void)
{
    char line[32];

    AppUi_ShowLine(0U, "[Run]");
    snprintf(line, sizeof(line), "IMU:%s", (App_GetImuSource() != 0U) ? "BNO08X" : "MPU6050");
    AppUi_ShowLine(2U, line);
    snprintf(line, sizeof(line), "PID:%s", (App_IsDrivePidActive() != 0U) ? "ON" : "OFF");
    AppUi_ShowLine(3U, line);
    snprintf(line, sizeof(line), "SRC:%s", (App_IsTofUartEnabled() != 0U) ? "TOF" : "CAM");
    AppUi_ShowLine(4U, line);
    snprintf(line, sizeof(line), "MID:%+.2f", g_driveController.balance.mechanicalMedian);
    AppUi_ShowLine(5U, line);
    snprintf(line, sizeof(line), "PIT:%+.2f", g_driveController.balance.imu.pitch);
    AppUi_ShowLine(6U, line);
    AppUi_ShowLine(13U, "U/D:Pg M:Nxt");
}

static void AppUi_RenderImuPage(void)
{
    char line[32];
    float pitchValue;
    float rollValue;
    float yawValue;
    float gyroXValue;

    if (App_GetImuSource() != 0U) {
        pitchValue = bno08x_data.pitch;
        rollValue = bno08x_data.roll;
        yawValue = bno08x_data.yaw;
        gyroXValue = g_driveController.balance.imu.gyroX;
    } else {
        pitchValue = mpu6050_euler.pitch;
        rollValue = mpu6050_euler.roll;
        yawValue = mpu6050_euler.yaw;
        gyroXValue = mpu6050_gyro.x_dps;
    }

    AppUi_ShowLine(0U, "[IMU]");
    snprintf(line, sizeof(line), "P:%+7.2f", pitchValue);
    AppUi_ShowLine(2U, line);
    snprintf(line, sizeof(line), "R:%+7.2f", rollValue);
    AppUi_ShowLine(3U, line);
    snprintf(line, sizeof(line), "Y:%+7.2f", yawValue);
    AppUi_ShowLine(4U, line);
    snprintf(line, sizeof(line), "GX:%+6.2f", gyroXValue);
    AppUi_ShowLine(5U, line);
    snprintf(line, sizeof(line), "RD:%u", (unsigned int)MPU6050_IsReady());
    AppUi_ShowLine(6U, line);
    AppUi_ShowLine(13U, "U/D:Pg HoldM");
}

static void AppUi_RenderMotorPage(void)
{
    char line[32];

    AppUi_ShowLine(0U, "[Motor]");
    snprintf(line, sizeof(line), "LS:%+6.2f", (double)g_driveController.left.speedRps);
    AppUi_ShowLine(2U, line);
    snprintf(line, sizeof(line), "RS:%+6.2f", (double)g_driveController.right.speedRps);
    AppUi_ShowLine(3U, line);
    snprintf(line, sizeof(line), "LD:%+6.1f", (double)g_driveController.left.duty);
    AppUi_ShowLine(4U, line);
    snprintf(line, sizeof(line), "RD:%+6.1f", (double)g_driveController.right.duty);
    AppUi_ShowLine(5U, line);
    snprintf(line, sizeof(line), "BO:%+6.1f", (double)g_driveController.balance.pid.output);
    AppUi_ShowLine(6U, line);
    AppUi_ShowLine(13U, "U/D:Pg HoldM");
}

static void AppUi_RenderTunePage(void)
{
    char line[32];
    const char *modeText = (g_appUi.tuneEditing != 0U) ? "EDIT" : "SEL";

    AppUi_ShowLine(0U, "[Tune]");
    snprintf(line, sizeof(line), "%c MID:%+6.2f",
             (g_appUi.tuneIndex == APP_UI_TUNE_MEDIAN) ? '>' : ' ',
             g_driveController.balance.mechanicalMedian);
    AppUi_ShowLine(2U, line);
    snprintf(line, sizeof(line), "%c KP :%+6.2f",
             (g_appUi.tuneIndex == APP_UI_TUNE_BAL_KP) ? '>' : ' ',
             g_driveController.balance.pid.kp);
    AppUi_ShowLine(3U, line);
    snprintf(line, sizeof(line), "%c KD :%+6.2f",
             (g_appUi.tuneIndex == APP_UI_TUNE_BAL_KD) ? '>' : ' ',
             g_driveController.balance.pid.kd);
    AppUi_ShowLine(4U, line);
    snprintf(line, sizeof(line), "MODE:%s", modeText);
    AppUi_ShowLine(6U, line);
    AppUi_ShowLine(13U, "M:Edit U/D:Adj");
}

static void AppUi_RenderActionPage(void)
{
    static const char *const kActionNames[APP_UI_ACTION_COUNT] = {
        "IMU SRC",
        "PID ON/OFF",
        "RESETDRV",
        "TOF QUERY",
    };
    uint8_t i;

    AppUi_ShowLine(0U, "[Action]");
    for (i = 0U; i < APP_UI_ACTION_COUNT; i++) {
        char line[32];

        snprintf(line, sizeof(line), "%c %s",
                 (g_appUi.actionIndex == i) ? '>' : ' ',
                 kActionNames[i]);
        AppUi_ShowLine((uint8_t)(2U + i), line);
    }
    AppUi_ShowLine(13U, "M:Run U/D:Sel");
}

static void AppUi_Render(void)
{
    uint16_t newFg;
    uint16_t newBg;

    if (App_GetImuSource() != 0U) {
        newFg = OLED_COLOR_YELLOW;
        newBg = OLED_COLOR_BLUE;
    } else {
        newFg = OLED_COLOR_WHITE;
        newBg = OLED_COLOR_BLACK;
    }

    if ((newFg != g_oledThemeFg) || (newBg != g_oledThemeBg)) {
        g_oledThemeFg = newFg;
        g_oledThemeBg = newBg;
        OLED_SetTheme(newFg, newBg);
        OLED_Clear();
    }

    switch (g_appUi.page) {
        case APP_UI_PAGE_RUN:
            AppUi_RenderRunPage();
            break;
        case APP_UI_PAGE_IMU:
            AppUi_RenderImuPage();
            break;
        case APP_UI_PAGE_MOTOR:
            AppUi_RenderMotorPage();
            break;
        case APP_UI_PAGE_TUNE:
            AppUi_RenderTunePage();
            break;
        case APP_UI_PAGE_ACTION:
            AppUi_RenderActionPage();
            break;
        default:
            break;
    }
}

void AppUi_Init(void)
{
    g_appUi.page = APP_UI_PAGE_RUN;
    g_appUi.tuneEditing = 0U;
    g_appUi.tuneIndex = APP_UI_TUNE_MEDIAN;
    g_appUi.actionIndex = APP_UI_ACTION_TOGGLE_IMU;
    g_appUi.lastRenderMs = 0xFFFFFFFFU;
    g_appUi.forceRender = 1U;
}

void AppUi_Update(uint32_t nowMs)
{
    AppUi_HandleButtons(nowMs);

    if (g_appUi.forceRender != 0U) {
        AppUi_Render();
        g_appUi.lastRenderMs = nowMs;
        g_appUi.forceRender = 0U;
    } else if ((nowMs - g_appUi.lastRenderMs) >= APP_UI_RENDER_INTERVAL_MS) {
        AppUi_Render();
        g_appUi.lastRenderMs = nowMs;
    }
}
