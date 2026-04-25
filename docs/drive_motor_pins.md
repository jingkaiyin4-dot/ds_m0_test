# 减速编码电机驱动引脚说明

## 1. 说明

本文档记录当前工程中两个减速编码电机的实际引脚分配，方便后续：

- 画原理图
- 画 PCB
- 接线排查
- 软件调试

本文档以当前代码实际使用关系为准，不只看 SysConfig 里的命名。

## 2. 电机驱动引脚分配

### 右减速电机

- PWM: `PB4`
- 方向 1: `PA30`
- 方向 2: `PA29`

对应驱动模块连接建议：

- `PWMA -> PB4`
- `AIN1 -> PA30`
- `AIN2 -> PA29`

### 左减速电机

- PWM: `PB1`
- 方向 1: `PB0`
- 方向 2: `PB2`

对应驱动模块连接建议：

- `PWMB -> PB1`
- `BIN1 -> PB0`
- `BIN2 -> PB2`

## 3. 编码器引脚分配

### 右减速电机编码器

- 编码器 A: `PB3`
- 编码器 B: `PB5`

### 左减速电机编码器

- 编码器 A: `PB6`
- 编码器 B: `PB7`

## 4. 最终接线表

### 右减速电机

- `PWMA -> PB4`
- `AIN1 -> PA30`
- `AIN2 -> PA29`
- 编码器 `A -> PB3`
- 编码器 `B -> PB5`
- 电机两根线 `MA/MB -> AO1/AO2`

### 左减速电机

- `PWMB -> PB1`
- `BIN1 -> PB0`
- `BIN2 -> PB2`
- 编码器 `A -> PB6`
- 编码器 `B -> PB7`
- 电机两根线 `MA/MB -> BO2/BO1`

说明：

- `MA/MB` 是电机本体两根线
- 如果后续发现正反方向和预期不一致，可以通过软件修正
- 编码器 A/B 正负方向也可以通过软件继续校准

## 5. 软件中的对应关系

### `motor/Motor.c`

软件当前约定：

- `MOTOR_RIGHT` 使用 `PB4 + PA30 + PA29`
- `MOTOR_LEFT` 使用 `PB1 + PB0 + PB2`

### `encoder/Encoder.c`

软件当前约定：

- 右编码器: `PB3/PB5`
- 左编码器: `PB6/PB7`

代码中有一层重新映射说明：

```c
#define ENCODER_RIGHT_A_PIN Encoder_PIN_LA_PIN
#define ENCODER_RIGHT_B_PIN Encoder_PIN_LB_PIN
#define ENCODER_LEFT_A_PIN  Encoder_PIN_RA_PIN
#define ENCODER_LEFT_B_PIN  Encoder_PIN_RB_PIN
```

也就是说，最终以代码中的左右轮解释为准，不要只看 `LA/LB/RA/RB` 这些名字直觉判断。

## 6. 使用建议

### 画板时建议丝印直接写清楚

右轮：

- `R_PWM PB4`
- `R_IN1 PA30`
- `R_IN2 PA29`
- `R_ENC_A PB3`
- `R_ENC_B PB5`

左轮：

- `L_PWM PB1`
- `L_IN1 PB0`
- `L_IN2 PB2`
- `L_ENC_A PB6`
- `L_ENC_B PB7`

### 电源建议

必须保证以下地共地：

- 主控板 `GND`
- 电机驱动模块 `GND`
- 编码器 `GND`
- 电机电源负极

否则 PWM、方向控制、编码器采样都可能异常。

## 7. 当前状态结论

根据当前调试结果：

- 左右减速电机方向接线可用
- 左右编码器均可工作
- 当前剩余问题主要在软件方向符号和 PID 校准，不是引脚映射错误
