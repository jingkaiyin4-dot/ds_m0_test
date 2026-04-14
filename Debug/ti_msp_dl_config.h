/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)



#define CPUCLK_FREQ                                                     80000000



/* Defines for Motor */
#define Motor_INST                                                         TIMA1
#define Motor_INST_IRQHandler                                   TIMA1_IRQHandler
#define Motor_INST_INT_IRQN                                     (TIMA1_INT_IRQn)
#define Motor_INST_CLK_FREQ                                             80000000
/* GPIO defines for channel 0 */
#define GPIO_Motor_C0_PORT                                                 GPIOB
#define GPIO_Motor_C0_PIN                                          DL_GPIO_PIN_4
#define GPIO_Motor_C0_IOMUX                                      (IOMUX_PINCM17)
#define GPIO_Motor_C0_IOMUX_FUNC                     IOMUX_PINCM17_PF_TIMA1_CCP0
#define GPIO_Motor_C0_IDX                                    DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_Motor_C1_PORT                                                 GPIOB
#define GPIO_Motor_C1_PIN                                          DL_GPIO_PIN_1
#define GPIO_Motor_C1_IOMUX                                      (IOMUX_PINCM13)
#define GPIO_Motor_C1_IOMUX_FUNC                     IOMUX_PINCM13_PF_TIMA1_CCP1
#define GPIO_Motor_C1_IDX                                    DL_TIMER_CC_1_INDEX



/* Defines for TIMER_0 */
#define TIMER_0_INST                                                     (TIMA0)
#define TIMER_0_INST_IRQHandler                                 TIMA0_IRQHandler
#define TIMER_0_INST_INT_IRQN                                   (TIMA0_INT_IRQn)
#define TIMER_0_INST_LOAD_VALUE                                           (999U)




/* Defines for I2C_OLED */
#define I2C_OLED_INST                                                       I2C1
#define I2C_OLED_INST_IRQHandler                                 I2C1_IRQHandler
#define I2C_OLED_INST_INT_IRQN                                     I2C1_INT_IRQn
#define I2C_OLED_BUS_SPEED_HZ                                             400000
#define GPIO_I2C_OLED_SDA_PORT                                             GPIOA
#define GPIO_I2C_OLED_SDA_PIN                                     DL_GPIO_PIN_10
#define GPIO_I2C_OLED_IOMUX_SDA                                  (IOMUX_PINCM21)
#define GPIO_I2C_OLED_IOMUX_SDA_FUNC                   IOMUX_PINCM21_PF_I2C1_SDA
#define GPIO_I2C_OLED_SCL_PORT                                             GPIOA
#define GPIO_I2C_OLED_SCL_PIN                                     DL_GPIO_PIN_11
#define GPIO_I2C_OLED_IOMUX_SCL                                  (IOMUX_PINCM22)
#define GPIO_I2C_OLED_IOMUX_SCL_FUNC                   IOMUX_PINCM22_PF_I2C1_SCL

/* Defines for I2C_mpu */
#define I2C_mpu_INST                                                        I2C0
#define I2C_mpu_INST_IRQHandler                                  I2C0_IRQHandler
#define I2C_mpu_INST_INT_IRQN                                      I2C0_INT_IRQn
#define I2C_mpu_BUS_SPEED_HZ                                              400000
#define GPIO_I2C_mpu_SDA_PORT                                              GPIOA
#define GPIO_I2C_mpu_SDA_PIN                                      DL_GPIO_PIN_28
#define GPIO_I2C_mpu_IOMUX_SDA                                    (IOMUX_PINCM3)
#define GPIO_I2C_mpu_IOMUX_SDA_FUNC                     IOMUX_PINCM3_PF_I2C0_SDA
#define GPIO_I2C_mpu_SCL_PORT                                              GPIOA
#define GPIO_I2C_mpu_SCL_PIN                                      DL_GPIO_PIN_31
#define GPIO_I2C_mpu_IOMUX_SCL                                    (IOMUX_PINCM6)
#define GPIO_I2C_mpu_IOMUX_SCL_FUNC                     IOMUX_PINCM6_PF_I2C0_SCL


/* Defines for UART_BNO08 */
#define UART_BNO08_INST                                                    UART2
#define UART_BNO08_INST_FREQUENCY                                       40000000
#define UART_BNO08_INST_IRQHandler                              UART2_IRQHandler
#define UART_BNO08_INST_INT_IRQN                                  UART2_INT_IRQn
#define GPIO_UART_BNO08_RX_PORT                                            GPIOA
#define GPIO_UART_BNO08_TX_PORT                                            GPIOA
#define GPIO_UART_BNO08_RX_PIN                                    DL_GPIO_PIN_22
#define GPIO_UART_BNO08_TX_PIN                                    DL_GPIO_PIN_23
#define GPIO_UART_BNO08_IOMUX_RX                                 (IOMUX_PINCM47)
#define GPIO_UART_BNO08_IOMUX_TX                                 (IOMUX_PINCM53)
#define GPIO_UART_BNO08_IOMUX_RX_FUNC                  IOMUX_PINCM47_PF_UART2_RX
#define GPIO_UART_BNO08_IOMUX_TX_FUNC                  IOMUX_PINCM53_PF_UART2_TX
#define UART_BNO08_BAUD_RATE                                            (115200)
#define UART_BNO08_IBRD_40_MHZ_115200_BAUD                                  (21)
#define UART_BNO08_FBRD_40_MHZ_115200_BAUD                                  (45)
/* Defines for UART_CAM */
#define UART_CAM_INST                                                      UART1
#define UART_CAM_INST_FREQUENCY                                         40000000
#define UART_CAM_INST_IRQHandler                                UART1_IRQHandler
#define UART_CAM_INST_INT_IRQN                                    UART1_INT_IRQn
#define GPIO_UART_CAM_RX_PORT                                              GPIOA
#define GPIO_UART_CAM_TX_PORT                                              GPIOA
#define GPIO_UART_CAM_RX_PIN                                       DL_GPIO_PIN_9
#define GPIO_UART_CAM_TX_PIN                                       DL_GPIO_PIN_8
#define GPIO_UART_CAM_IOMUX_RX                                   (IOMUX_PINCM20)
#define GPIO_UART_CAM_IOMUX_TX                                   (IOMUX_PINCM19)
#define GPIO_UART_CAM_IOMUX_RX_FUNC                    IOMUX_PINCM20_PF_UART1_RX
#define GPIO_UART_CAM_IOMUX_TX_FUNC                    IOMUX_PINCM19_PF_UART1_TX
#define UART_CAM_BAUD_RATE                                                (9600)
#define UART_CAM_IBRD_40_MHZ_9600_BAUD                                     (260)
#define UART_CAM_FBRD_40_MHZ_9600_BAUD                                      (27)
/* Defines for UART_bj1 */
#define UART_bj1_INST                                                      UART3
#define UART_bj1_INST_FREQUENCY                                         80000000
#define UART_bj1_INST_IRQHandler                                UART3_IRQHandler
#define UART_bj1_INST_INT_IRQN                                    UART3_INT_IRQn
#define GPIO_UART_bj1_RX_PORT                                              GPIOA
#define GPIO_UART_bj1_TX_PORT                                              GPIOA
#define GPIO_UART_bj1_RX_PIN                                      DL_GPIO_PIN_25
#define GPIO_UART_bj1_TX_PIN                                      DL_GPIO_PIN_26
#define GPIO_UART_bj1_IOMUX_RX                                   (IOMUX_PINCM55)
#define GPIO_UART_bj1_IOMUX_TX                                   (IOMUX_PINCM59)
#define GPIO_UART_bj1_IOMUX_RX_FUNC                    IOMUX_PINCM55_PF_UART3_RX
#define GPIO_UART_bj1_IOMUX_TX_FUNC                    IOMUX_PINCM59_PF_UART3_TX
#define UART_bj1_BAUD_RATE                                              (115200)
#define UART_bj1_IBRD_80_MHZ_115200_BAUD                                    (43)
#define UART_bj1_FBRD_80_MHZ_115200_BAUD                                    (26)
/* Defines for UART_bj2 */
#define UART_bj2_INST                                                      UART0
#define UART_bj2_INST_FREQUENCY                                         40000000
#define UART_bj2_INST_IRQHandler                                UART0_IRQHandler
#define UART_bj2_INST_INT_IRQN                                    UART0_INT_IRQn
#define GPIO_UART_bj2_RX_PORT                                              GPIOA
#define GPIO_UART_bj2_TX_PORT                                              GPIOA
#define GPIO_UART_bj2_RX_PIN                                       DL_GPIO_PIN_1
#define GPIO_UART_bj2_TX_PIN                                       DL_GPIO_PIN_0
#define GPIO_UART_bj2_IOMUX_RX                                    (IOMUX_PINCM2)
#define GPIO_UART_bj2_IOMUX_TX                                    (IOMUX_PINCM1)
#define GPIO_UART_bj2_IOMUX_RX_FUNC                     IOMUX_PINCM2_PF_UART0_RX
#define GPIO_UART_bj2_IOMUX_TX_FUNC                     IOMUX_PINCM1_PF_UART0_TX
#define UART_bj2_BAUD_RATE                                              (115200)
#define UART_bj2_IBRD_40_MHZ_115200_BAUD                                    (21)
#define UART_bj2_FBRD_40_MHZ_115200_BAUD                                    (45)





/* Port definition for Pin Group GPIO_MPU6050 */
#define GPIO_MPU6050_PORT                                                (GPIOB)

/* Defines for PIN_INT: GPIOB.14 with pinCMx 31 on package pin 2 */
// groups represented: ["Encoder","GPIO_MPU6050"]
// pins affected: ["PIN_LA","PIN_RA","PIN_INT"]
#define GPIO_MULTIPLE_GPIOB_INT_IRQN                            (GPIOB_INT_IRQn)
#define GPIO_MULTIPLE_GPIOB_INT_IIDX            (DL_INTERRUPT_GROUP1_IIDX_GPIOB)
#define GPIO_MPU6050_PIN_INT_IIDX                           (DL_GPIO_IIDX_DIO14)
#define GPIO_MPU6050_PIN_INT_PIN                                (DL_GPIO_PIN_14)
#define GPIO_MPU6050_PIN_INT_IOMUX                               (IOMUX_PINCM31)
/* Port definition for Pin Group Encoder */
#define Encoder_PORT                                                     (GPIOB)

/* Defines for PIN_LB: GPIOB.5 with pinCMx 18 on package pin 53 */
#define Encoder_PIN_LB_PIN                                       (DL_GPIO_PIN_5)
#define Encoder_PIN_LB_IOMUX                                     (IOMUX_PINCM18)
/* Defines for PIN_LA: GPIOB.3 with pinCMx 16 on package pin 51 */
#define Encoder_PIN_LA_IIDX                                  (DL_GPIO_IIDX_DIO3)
#define Encoder_PIN_LA_PIN                                       (DL_GPIO_PIN_3)
#define Encoder_PIN_LA_IOMUX                                     (IOMUX_PINCM16)
/* Defines for PIN_RA: GPIOB.6 with pinCMx 23 on package pin 58 */
#define Encoder_PIN_RA_IIDX                                  (DL_GPIO_IIDX_DIO6)
#define Encoder_PIN_RA_PIN                                       (DL_GPIO_PIN_6)
#define Encoder_PIN_RA_IOMUX                                     (IOMUX_PINCM23)
/* Defines for PIN_RB: GPIOB.7 with pinCMx 24 on package pin 59 */
#define Encoder_PIN_RB_PIN                                       (DL_GPIO_PIN_7)
#define Encoder_PIN_RB_IOMUX                                     (IOMUX_PINCM24)
/* Defines for MOTOR_LA: GPIOA.30 with pinCMx 5 on package pin 37 */
#define MOTOR_MOTOR_LA_PORT                                              (GPIOA)
#define MOTOR_MOTOR_LA_PIN                                      (DL_GPIO_PIN_30)
#define MOTOR_MOTOR_LA_IOMUX                                      (IOMUX_PINCM5)
/* Defines for MOTOR_LB: GPIOA.29 with pinCMx 4 on package pin 36 */
#define MOTOR_MOTOR_LB_PORT                                              (GPIOA)
#define MOTOR_MOTOR_LB_PIN                                      (DL_GPIO_PIN_29)
#define MOTOR_MOTOR_LB_IOMUX                                      (IOMUX_PINCM4)
/* Defines for MOTOR_RA: GPIOB.0 with pinCMx 12 on package pin 47 */
#define MOTOR_MOTOR_RA_PORT                                              (GPIOB)
#define MOTOR_MOTOR_RA_PIN                                       (DL_GPIO_PIN_0)
#define MOTOR_MOTOR_RA_IOMUX                                     (IOMUX_PINCM12)
/* Defines for MOTOR_RB: GPIOB.2 with pinCMx 15 on package pin 50 */
#define MOTOR_MOTOR_RB_PORT                                              (GPIOB)
#define MOTOR_MOTOR_RB_PIN                                       (DL_GPIO_PIN_2)
#define MOTOR_MOTOR_RB_IOMUX                                     (IOMUX_PINCM15)

/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_Motor_init(void);
void SYSCFG_DL_TIMER_0_init(void);
void SYSCFG_DL_I2C_OLED_init(void);
void SYSCFG_DL_I2C_mpu_init(void);
void SYSCFG_DL_UART_BNO08_init(void);
void SYSCFG_DL_UART_CAM_init(void);
void SYSCFG_DL_UART_bj1_init(void);
void SYSCFG_DL_UART_bj2_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
