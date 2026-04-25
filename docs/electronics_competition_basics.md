# 电赛项目基础教学文档

## 1. 项目总览

这个工程运行在 `MSPM0G3507` 上，核心目标是把多个传感器、执行器和显示模块组织成一个可用于电赛的小系统。

当前系统主要包含这些模块：

- IMU 姿态模块
  - `BNO08X`，通过 `UART` 接收现成姿态角
  - `MPU6050`，通过 `I2C` 读取原始数据并在本地解算
- 距离/相机模块
  - `TOF400F`，通过 `UART` 查询距离
  - `CameraControl`，通过 `UART` 接收上位机或串口工具控制命令
- 步进云台模块
  - 两个总线步进电机，分别走两路 UART
- 底盘模块
  - 两个 TB6612 驱动的减速编码电机
  - 两路增量编码器反馈
  - 双环 PID 控制
- 显示模块
  - 240x320 ST7789 SPI 彩屏
- 人机交互
  - 板载按键 `PB21`

这套系统不是所有模块都强耦合在一起，而是“多个独立功能块共存”。理解项目时，最重要的是先看清每个模块的职责，再看模块之间怎样传数据。

## 2. 目录结构和职责

### 2.1 `main.c`

`main.c` 是系统集成层。它负责：

- 初始化各个模块
- 轮询按键
- 决定当前显示谁的数据
- 定时触发 TOF 查询、步进位置查询
- 把结果显示到屏幕上

它不应该写太多底层细节，而是负责“调用谁、什么时候调用、显示什么”。

### 2.2 `Drivers/`

这里主要放具体硬件驱动。

- `Drivers/MPU6050/`
  - `mspm0_i2c.c`：I2C 底层读写
  - `mpu6050.c`：MPU6050 寄存器配置和姿态解算
- `Drivers/TOF400F/`
  - TOF 查询、CRC 校验、距离解析
- `Drivers/OLED_Hardware_SPI/`
  - 现在已经是 `ST7789` 的兼容显示层
- `Drivers/BNO08X_UART_RVC/`
  - 从 BNO08X 的 UART RVC 数据帧中提取姿态角

### 2.3 `app/`

这里放业务层和系统层的小模块。

- `CameraControl.c`
  - 串口收到命令后，控制步进云台开始/停止
- `ZdtStepper.c`
  - 两个步进电机的 UART 协议收发
- `DriveBase.c`
  - 新加的统一底盘状态层
- `AppTypes.h`
  - 新加的公共结构体类型
- `AppBus.h`
  - 新加的总线抽象数据结构

### 2.4 `motor/` 和 `encoder/`

这两个目录共同构成“减速编码电机底盘”的底层。

- `motor/`
  - 负责 PWM 和方向控制
- `encoder/`
  - 负责计数编码器脉冲

### 2.5 `PID/`

负责底盘双环 PID 控制。

## 3. 各模块工作原理

## 3.1 按键模块

### 文件

- `main.c`
- `app/AppTypes.h`

### 结构体

```c
typedef struct {
    uint8_t pressed;
    uint8_t clicked;
    uint32_t stable_since_ms;
} AppButtonState;
```

### 含义

- `pressed`：当前稳定状态是否按下
- `clicked`：本轮是否刚刚产生一次点击事件
- `stable_since_ms`：最近一次状态变化发生的时间

### 核心函数

#### `App_ButtonUpdate(AppButtonState *button, uint8_t rawPressed, uint32_t now)`

作用：

- 把 GPIO 原始电平变成“稳定按下/点击事件”

内部逻辑：

1. 先读取按键当前原始电平
2. 如果和上次采样不同，记录变化时间
3. 如果保持稳定超过消抖时间，才更新 `pressed`
4. 如果是从松开变成按下，置 `clicked = 1`

### 为什么这样写

优点是：

- 非阻塞
- 不用中断
- 不影响 PID 主循环
- 后面加长按、双击很方便

## 3.2 总线设备抽象

### 文件

- `app/AppTypes.h`
- `app/AppBus.h`

### 结构体

```c
typedef enum {
    APP_BUS_NONE = 0,
    APP_BUS_UART,
    APP_BUS_I2C,
} AppBusType;

typedef struct {
    AppBusType type;
    const char *name;
    uint8_t address;
    uint32_t speed_hz;
} AppBusDevice;
```

这个结构体的目的，不是直接替代所有底层驱动，而是先统一“设备描述方式”。

例如：

- `BNO08X` 是 `UART` 设备
- `MPU6050` 是 `I2C` 设备
- `TOF400F` 是 `UART` 设备

这样做的好处是：

- 上层逻辑可以用统一数据结构描述设备
- 以后要做统一设备表、统一调试打印、统一配置管理会很方便

### `AppBusTransfer`

```c
typedef struct {
    AppBusDevice device;
    uint8_t reg;
    uint8_t *buffer;
    uint16_t length;
} AppBusTransfer;
```

这个结构体是为以后统一读写接口做准备的。

## 3.3 底盘减速编码电机模块

这部分是这次重点。

### 3.3.1 电机驱动层 `motor/`

#### 文件

- `motor/Motor.c`
- `motor/Motor.h`

#### 核心职责

- 输出 PWM
- 设置电机方向
- 保存当前占空比

#### 关键函数

##### `Motor_SetDuty(MotorChannel channel, int16_t duty)`

作用：

- 设置左/右电机占空比和方向

流程：

1. 限幅到 `-100 ~ 100`
2. 通过正负号决定方向
3. 取绝对值作为 PWM 幅值
4. 调 `Motor_SetDirection()` 设置 `TB6612` 方向脚
5. 调 `Motor_SetPwmCompare()` 输出 PWM
6. 记录当前 `g_motorDuty[channel]`

##### `Motor_GetDuty(MotorChannel channel)`

作用：

- 获取当前电机占空比

##### `Motor_FillDriveState(MotorChannel channel, DriveMotorState *state)`

作用：

- 用一个统一结构体同时填充
  - 当前占空比
  - 本周期编码器增量
  - 编码器累计值

这是“统一状态结构”的关键入口。

### 3.3.2 编码器层 `encoder/`

#### 文件

- `encoder/Encoder.c`
- `encoder/Encoder.h`

#### 核心职责

- 响应 GPIO 中断
- 统计左右轮编码器脉冲

#### 关键函数

##### `GROUP1_IRQHandler(void)`

作用：

- 处理编码器 A 相跳变中断

逻辑：

1. 看左轮/右轮哪一路 A 相产生中断
2. 读取对应 B 相电平
3. 判断当前是正转还是反转
4. 调 `Encoder_Update()` 加一或减一
5. 清除中断标志

##### `Encoder_GetDelta(EncoderChannel channel)`

作用：

- 读取“从上次调用到现在”的增量

特性：

- 读完会清零
- 适合 PID 周期调用

##### `Encoder_GetTotal(EncoderChannel channel)`

作用：

- 获取从开机到现在的累计值

用途：

- 用于显示总圈数
- 用于路径累计
- 用于调试运动距离

### 3.3.3 统一底盘状态层 `DriveBase`

#### 文件

- `app/DriveBase.h`
- `app/DriveBase.c`

这是这次新加的模块。

#### 结构体

```c
typedef struct {
    const char *name;
    MotorChannel motor;
    EncoderChannel encoder;
    DriveMotorState state;
} DriveWheel;

typedef struct {
    DriveWheel left;
    DriveWheel right;
} DriveBase;
```

### 它解决什么问题

以前：

- 电机状态在 `motor/`
- 编码器状态在 `encoder/`
- PID 又直接分别调这两个模块

现在：

- 用 `DriveWheel` 把“一个轮子”的输出和反馈统一起来
- 用 `DriveBase` 把左右轮统一起来

这就是解耦的核心：

- 上层不必关心“这个数据是从 motor 取还是 encoder 取”
- 只关心“左轮状态、右轮状态”

#### 关键函数

##### `DriveBase_Init(DriveBase *driveBase)`

作用：

- 初始化左右轮元信息

##### `DriveBase_UpdateState(DriveBase *driveBase)`

作用：

- 调 `Motor_FillDriveState()` 更新左右轮状态

## 3.4 PID 双环控制模块

### 文件

- `PID/PID.c`
- `PID/PID.h`

### 结构体

#### `PIDController`

表示一个 PID 控制器。

- `kp/ki/kd`
- `target`
- `actual`
- `integral`
- `prevError`
- `output`
- `integralLimit`
- `outputLimit`

#### `WheelController`

表示一个轮子的控制器，由两个 PID 组成：

- 位置环 PID
- 速度环 PID

#### `DriveController`

表示整个底盘控制器：

- 左轮控制器
- 右轮控制器
- 新加的 `DriveBase driveBase`
- 编码器分辨率、减速比、采样周期等参数

### 关键函数

#### `PIDController_Update(PIDController *pid, float actual)`

作用：

- 输入实际值
- 根据目标值算出输出

内部步骤：

1. 算误差 `error`
2. 算微分 `derivative`
3. 累加积分 `integral`
4. 做积分限幅
5. 计算 `kp*e + ki*i + kd*d`
6. 做输出限幅
7. 保存 `prevError`

#### `WheelController_Update(...)`

现在已经不再直接从编码器模块取数据，而是从 `DriveBase` 的轮状态里拿：

```c
int32_t delta = driveWheel->state.encoder_delta;
```

流程：

1. 用编码器增量算电机转速
2. 再除以减速比，得到轮子转速
3. 更新位置累计
4. 位置环输出一个速度目标
5. 速度环输出一个占空比
6. 调 `Motor_SetDuty()` 控制电机

这就是典型的“双环 PID”：

- 外环：位置环
- 内环：速度环

#### `DriveController_Init()`

作用：

- 初始化 PID 参数
- 初始化底盘状态结构 `DriveBase_Init(&g_driveController.driveBase)`

#### `double_pid()`

作用：

- 每个控制周期更新一次左右轮 PID

现在流程是：

1. `DriveBase_UpdateState(...)`
2. `WheelController_Update(left, ...)`
3. `WheelController_Update(right, ...)`

## 3.5 步进云台模块

### 文件

- `app/ZdtStepper.c`
- `app/CameraControl.c`

### 职责划分

#### `ZdtStepper.c`

负责：

- 两个步进电机 UART 协议发送
- 查询位置等参数
- 接收回包

#### `CameraControl.c`

负责：

- 接收来自串口的简单命令
- 根据命令调用 `ZdtStepper` 开始或停止云台

### 关键思路

`CameraControl` 不直接管 UART 底层细节，而是：

- 串口来了命令
- 把命令翻译成“启动/停止云台”
- 然后调用 `ZdtStepper` 的高层函数

这也是一种解耦。

## 3.6 IMU 模块

### BNO08X

- 总线：UART
- 优点：直接给姿态角，使用简单
- 缺点：灵活性较低

### MPU6050

- 总线：I2C
- 优点：原始数据可控
- 缺点：需要本地解算

### 切换方式

现在工程里通过按键在显示层切换：

- `BNO08X`
- `MPU6050`

但底层两个模块都可以初始化，显示上选择谁主要由 `imu_use_bno08x` 决定。

## 3.7 TOF / Camera 共用串口的思路

工程中有一个很典型的“资源复用”案例：

- `TOF400F`
- `CameraControl`

它们共用一套 UART 服务层，通过模式切换来决定当前这路串口归谁用。

这背后的设计思路是：

- 物理资源有限
- 与其硬编码死，不如做一个小服务层统一分发

## 4. 数据流怎么走

## 4.1 底盘双环 PID 数据流

```text
编码器中断 -> Encoder 累计增量
           -> DriveBase_UpdateState 统一收集左右轮状态
           -> WheelController_Update
           -> 位置环 PID
           -> 速度环 PID
           -> Motor_SetDuty
           -> TB6612 输出到减速电机
```

## 4.2 云台步进数据流

```text
Camera 串口命令 / 主程序切换
    -> CameraControl 或 main
    -> ZdtStepper 发送控制命令
    -> 步进电机动作
    -> 可选位置查询回读
    -> 屏幕显示 Turn
```

## 4.3 IMU 数据流

### BNO08X

```text
BNO08X UART帧 -> 驱动解析 -> bno08x_data -> main显示
```

### MPU6050

```text
MPU6050寄存器 -> I2C读取 -> 姿态解算 -> mpu6050_euler -> main显示
```

## 5. 你现在最应该理解的几个工程思想

### 5.1 驱动层和功能层分离

驱动层做的事：

- 和硬件寄存器、引脚、总线直接打交道

功能层做的事：

- 组织驱动
- 实现业务逻辑

比如：

- `Motor.c` 是驱动层
- `DriveBase.c` 是中间状态层
- `PID.c` 是控制层
- `main.c` 是系统集成层

### 5.2 统一结构体的意义

这次加的这些结构体，不是为了“写得高级”，而是为了解决：

- 数据散在各个模块，不容易管理
- 上层代码耦合太深
- 后面一改底层，上层就一起改

统一结构体的目的，是把“一个对象的状态”收拢起来。

例如：

- 一个按键 -> `AppButtonState`
- 一个总线设备 -> `AppBusDevice`
- 一个轮子 -> `DriveWheel`
- 一个底盘 -> `DriveBase`

### 5.3 为什么先统一数据，再统一接口

这是嵌入式项目里很实用的思路。

如果你一开始就想把所有驱动全部改成统一 `read/write/open/close` 接口，风险会很大。

更稳的做法是：

1. 先统一结构体
2. 让上层先习惯统一的数据组织
3. 再逐步把底层接口收敛

## 6. 后续建议

如果后面继续优化，我建议按这个顺序：

1. 给 `DriveBase` 增加目标速度、目标位置接口
2. 把底盘 PID 的目标设定也统一到 `DriveBase`
3. 再把 `TOF/BNO/MPU/Camera` 逐步接到统一总线适配层
4. 最后再考虑做更完整的任务调度和状态机

## 7. 结语

这个工程已经不只是“点亮模块”的阶段了，而是在逐步变成一个小型系统。

学习这类电赛工程时，不要只盯着某个函数里写了什么，更要看：

- 模块职责怎么分
- 数据从哪里来，到哪里去
- 为什么这个结构体要这样设计
- 为什么这一层要存在

只要把“驱动层 -> 状态层 -> 控制层 -> 集成层”这条线理解透了，你后面自己扩功能会轻松很多。
