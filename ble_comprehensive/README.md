# BLE Comprehensive Example

> 综合 BLE 示例：集成 BLE NUS、BLE OTA (DFU)、高速 UART (1 Mbps)、SPI Master、I2C Master、ADC、外部中断、Flash 存储（NVS + Settings）、设备电源管理 (PM) 以及 Direct Test Mode (DTM)。

## 目录

- [BLE Comprehensive Example](#ble-comprehensive-example)
  - [目录](#目录)
  - [概述](#概述)
  - [支持的平台](#支持的平台)
  - [NCS 版本兼容性](#ncs-版本兼容性)
  - [快速开始](#快速开始)
    - [编译](#编译)
    - [烧录](#烧录)
    - [生成 DFU 升级包](#生成-dfu-升级包)
  - [功能测试](#功能测试)
    - [1. BLE NUS 服务](#1-ble-nus-服务)
    - [2. BLE OTA DFU](#2-ble-ota-dfu)
    - [3. SPI Master](#3-spi-master)
    - [4. I2C Master](#4-i2c-master)
    - [5. ADC 采样](#5-adc-采样)
    - [6. 外部中断](#6-外部中断)
    - [7. Flash 存储 (NVS + Settings)](#7-flash-存储-nvs--settings)
    - [8. 设备电源管理 (PM)](#8-设备电源管理-pm)
    - [9. 高速 UART (1 Mbps)](#9-高速-uart-1-mbps)
    - [10. Direct Test Mode (DTM)](#10-direct-test-mode-dtm)
  - [架构说明](#架构说明)
    - [线程分配](#线程分配)
    - [构建产物](#构建产物)
  - [SDK 补丁](#sdk-补丁)

---

## 概述

本示例将多个 nRF Connect SDK (NCS) 常用外设和协议栈功能集成到一个项目中，便于开发者参考各模块的 API 用法和配置模式：

| 模块 | 说明 | 源文件 |
|------|------|--------|
| **BLE NUS** | Nordic UART Service，手机与设备间的无线串口透传 | `src/main.c` |
| **BLE OTA DFU** | 通过 BLE SMP 协议进行设备固件升级（MCUboot + MCUmgr） | sysbuild 配置 |
| **高速 UART** | 1 Mbps 异步 UART 回环测试，用于验证高速串口可靠性 | `src/uart_thread.c` |
| **SPI Master** | SPI 主设备通信示例，演示 `spi_transceive_dt` / `spi_write_dt` API | `src/spi_thread.c` |
| **I2C Master** | I2C 主设备通信示例，模拟 EEPROM 读取 | `src/i2c_thread.c` |
| **ADC** | 多通道 ADC 采样（电池电压 + 外部模拟输入） | `src/adc_thread.c` |
| **外部中断** | GPIO 外部中断触发 I2C 操作 | `src/io_int_thread.c` |
| **Flash 存储** | NVS API（键值存储）+ Settings API（层次化配置） | `src/nvs_thread.c`、`src/settings_thread.c` |
| **设备 PM** | 动态开关外设电源，适用于低功耗场景 | `src/main.c` |
| **DTM** | BLE Direct Test Mode，支持射频一致性测试（nRF54L15 专属） | `src/dtm/` |

所有模块运行在独立线程中，通过按钮触发特定功能，支持运行时观察日志输出。

---

## 支持的平台

| 硬件平台 | PCA | Board Target | 外部 Flash |
|----------|-----|-------------|-----------|
| nRF52840 DK | PCA10056 | `nrf52840dk/nrf52840` | MX25R64 (QSPI) |
| nRF5340 DK | PCA10095 | `nrf5340dk/nrf5340/cpuapp` | MX25R64 (QSPI) |
| nRF7002 DK | PCA10143 | `nrf7002dk/nrf5340/cpuapp` | MX25R64 (SPI) |
| nRF54L15 DK | PCA10156 | `nrf54l15dk/nrf54l15/cpuapp` | MX25R64 (QSPI) |

> **说明：** 所有平台均使用外部 SPI Flash (MX25R64) 存放 OTA 固件升级的 secondary slot (slot1)。nRF54L15 还额外支持 **DTM-in-app** 功能（按住 Button 2 复位进入 DTM 模式）。

---

## NCS 版本兼容性

本示例经过以下 NCS 版本的测试：

| NCS 版本 | Git 分支/标签 |
|----------|--------------|
| v1.5.x ~ v1.9.x | `v1.5_v1.9` |
| v2.0.x | `v2.0` |
| v2.2.x ~ v2.9.x | 对应 tag |
| v3.0.0 | `v3.0.0` |
| v3.3.0 | `v3.3.0` |
| **v3.4.0** | `v3.4.0` |



---

## 快速开始

### 编译

```bash
# nRF54L15 DK（含 DTM-in-app 支持）
west build -b nrf54l15dk/nrf54l15/cpuapp --sysbuild -p

# nRF5340 DK
west build -b nrf5340dk/nrf5340/cpuapp --sysbuild -p

# nRF52840 DK
west build -b nrf52840dk/nrf52840 --sysbuild -p

# nRF7002 DK
west build -b nrf7002dk/nrf5340/cpuapp --sysbuild -p
```

> **Sysbuild 说明：** `--sysbuild` 参数启用多镜像构建系统，会同时编译：
> - MCUboot（引导加载程序，应用核）
> - ble_comprehensive（主应用，应用核）
> - IPC Radio / b0n（网络核固件 + 安全引导，仅 nRF5340/nRF54L15/nRF7002DK）

### 烧录

```bash
west flash
```

此命令会烧录 sysbuild 生成的所有镜像。如果只需烧录单个镜像，可使用 `--domain` 参数。

### 生成 DFU 升级包

编译后，OTA 升级所需的 zip 包已自动生成：

```text
build/dfu_application.zip
```

将此文件拷贝到手机，通过 **nRF Connect for Mobile** 或 **nRF Device Manager** 执行 OTA 升级。

---

## 功能测试

### 1. BLE NUS 服务

**验证手机与设备之间的 BLE 无线串口透传。**

1. USB 连接开发板，设备管理器中会分配 COM 端口。
2. 打开串口调试助手，波特率设为 **115200**。
3. 复位开发板，设备开始广播 `comprehensive`。
4. 打开手机上的 **nRF Connect for Mobile**，搜索并连接 `comprehensive`。
5. 在 Unknown Service 中找到 NUS RX characteristic，点击 **Enable CCCDs**（启用通知）。
6. 在 NUS RX characteristic 中写入 `0123456789`，点击 **Write**。
   - 串口端应看到 `0123456789` 输出。
7. 在串口端输入任意文本（如 `Hello`），手机端应收到 NUS TX 通知。

---

### 2. BLE OTA DFU

**通过 BLE SMP 协议（MCUmgr）进行固件无线升级。**

支持两种升级模式：
- **单镜像升级：** 仅升级应用核固件。
- **多镜像升级（nRF53 系列）：** 同时升级应用核 + 网络核固件（通过 `sysbuild.conf` 和 `Kconfig.sysbuild` 配置，`MCUBOOT_MODE_OVERWRITE_ONLY` 模式）。

**测试步骤：**

1. 修改代码（如改动日志中的版本号 `v3.0`），重新编译生成新的 `dfu_application.zip`。
2. 将 `build/dfu_application.zip` 拷贝到手机。
3. 打开 **nRF Connect for Mobile** 或 **nRF Device Manager**，连接设备。
4. 点击右上角 **DFU** 按钮，选择 `dfu_application.zip`。
5. 等待升级完成（设备会自动复位）。
6. nRF Connect 会自动重连设备以确认新镜像。如果新镜像无法正常启动，MCUboot 会在下次复位后回滚到旧镜像。

> **签名密钥：** 构建默认使用 MCUboot 的开发签名密钥。**生产环境必须替换为自定义密钥：**
>
> ```bash
> # 生成 ECDSA P-256 密钥对
> python3 bootloader/mcuboot/scripts/imgtool.py keygen -t ecdsa-p256 -k my_mcuboot_private.pem
> ```
>
> 然后在 `sysbuild.conf` 中指定：
> ```kconfig
> SB_CONFIG_BOOT_SIGNATURE_KEY_FILE="my_mcuboot_private.pem"
> ```

---

### 3. SPI Master

**演示 Zephyr SPI API (`spi_transceive_dt` / `spi_write_dt`) 与 SPI 从设备通信。**

SPI Master 引脚定义见各板子的 DTS overlay 文件（`boards/<board>.overlay`）。

为方便测试，`resources` 目录下提供了预编译的 SPI 从设备固件。SPI 从设备引脚定义如下：

```text
APP_SPIS_SCK_PIN  26
APP_SPIS_MISO_PIN 30
APP_SPIS_MOSI_PIN 29
APP_SPIS_CS_PIN   31
```

**测试步骤：**

1. 准备一个 SPI 从设备（推荐使用另一个 nRF52832/nRF52840 开发板烧录 `nRF5_SDK/examples/peripheral/spis` 中的 SPI slave 固件）。
2. 将 SPI Master 的 SCK / MOSI / MISO / CS 引脚与 SPI 从设备的对应引脚连接。
3. 复位开发板，按 **Button 2** 触发 SPI 通信。
4. 日志输出如下：

```
[00:04:38.533,844] <inf> spi_thread: spi master thread
[00:04:38.534,155] <inf> spi_thread: Received SPI dev0 data:
                           4e 6f 72 64 69 63 00         N o r d i c .
[00:04:38.534,454] <inf> spi_thread: SPI dev1 write success
```

---

### 4. I2C Master

**演示 Zephyr I2C API (`i2c_write_dt` / `i2c_read_dt`) 模拟 EEPROM 读取。**

I2C Master 引脚定义见各板子的 DTS overlay 文件。

为方便测试，`resources` 目录下提供了预编译的 I2C 从设备固件。I2C 从设备（TWIS）引脚定义如下：

```text
SCL_S  31
SDA_S  30
```

**测试步骤：**

1. 准备一个 I2C 从设备（推荐使用另一个 nRF52832/nRF52840 开发板烧录 `nRF5_SDK/examples/peripheral/twi_master_with_twis_slave` 中的 TWIS slave 固件）。
2. 将 I2C Master 的 SCL / SDA 引脚与 I2C 从设备的对应引脚连接。
3. 复位开发板，按 **Button 4** 触发 I2C EEPROM 读取。
4. 日志输出如下：

```
[00:01:55.881,248] <inf> i2c_thread: i2c master thread
[00:01:55.881,849] <inf> i2c_thread: EEPROM:
                           f8 f7 66 ff 1e b9 25 a1  f4 20 f8 f7 61 ff 28 46
[00:01:55.882,450] <inf> i2c_thread: EEPROM:
                           00 f0 60 f8 10 b1 11 20  bd e8 f0 9f 66 61 4f f0
```

---

### 5. ADC 采样

**演示 Zephyr ADC API (`adc_read_dt` / `adc_sequence_init_dt`) 多通道采样。**

- **通道 0：** 外部模拟输入引脚（各板子不同，见 DTS overlay）
- **通道 1：** VDD（电池/供电电压）

**测试步骤：**

1. 无需额外硬件，上电即自动运行。
2. 每 5 秒采样一次。改变通道 0 对应引脚上的电压可观察 ADC 读数变化。
3. 日志输出如下：

```
[00:04:30.853,300] <inf> adc_thread: ADC thread
- adc@d5000, channel 0: 2 = 4 mV
- adc@d5000, channel 1: 1023 = 2247 mV
```

---

### 6. 外部中断

**演示 Zephyr GPIO 中断 API，Button 4 配置为 GPIO 外部中断源。**

按下 Button 4 会：
1. 触发 GPIO 中断 ISR。
2. ISR 释放 `sem_i2c_op` 信号量。
3. I2C 线程获得信号量后执行 EEPROM 读取。

日志输出：

```
[00:00:22.533,525] <inf> extint_thread: External interrupt occurs on pin 0x10 at 0x1f589ms
```

紧接着 I2C 线程开始读取 EEPROM。

---

### 7. Flash 存储 (NVS + Settings)

**演示两种 Flash 存储 API 的用法：**

| API 层 | 说明 | 特点 |
|--------|------|------|
| **NVS API** | Non-Volatile Storage，底层键值存储 | 16-bit 整数 ID 索引，环形缓冲区 + CRC 校验 |
| **Settings API** | 层次化配置存储 | 字符串键名，支持动态注册 handler，底层可切换 NVS/ZMS |

**存储后端选择（自动）：**

| SoC / Flash 类型 | 自动选择的后端 |
|-----------------|---------------|
| nRF52840 / nRF5340 (Flash) | NVS |
| nRF54L15 (RRAM) | ZMS |

**测试步骤：**

无需额外硬件。设备每次复位后：
- NVS：从 Flash 读取上次存储的 key 值和 reboot counter，递增后写回。
- Settings：从 Flash 加载 `alpha/key` 和 `alpha/boot_cnt` 键值，递增 `boot_cnt` 后写回。

日志输出（Settings）：

```
[00:00:00.843,753] <inf> settings_thread: settings subsys initialization: OK.
[00:00:00.843,829] <inf> settings_thread: set handler name=boot_cnt, len=4
[00:00:00.843,854] <inf> settings_thread: *** Reboot counter in Settings: 3 ****
[00:00:00.843,977] <inf> settings_thread: Key value in Settings:
                           30 31 32 33 34 35 36 37          01234567
[00:00:00.843,996] <inf> settings_thread: save new reboot counter by Settings API
```

---

### 8. 设备电源管理 (PM)

**演示使用 `pm_device_action_run()` 动态开关外设以降低功耗。**

- 按 **Button 1** 切换设备电源状态。
- **SUSPEND（低功耗）：** 关闭 UART、SPI、I2C。
- **RESUME（正常工作）：** 重新启用所有外设。

日志输出：

```
[00:02:23.346,708] <inf> main: button1 isr
[00:02:23.346,728] <inf> main: Turning off UART/SPI/I2C to save power
[00:02:23.356,858] <inf> main: Entered low power

[00:03:29.875,444] <inf> main: button1 isr
[00:03:29.875,458] <inf> main: Turning on UART/SPI/I2C
[00:03:29.875,492] <inf> main: Entered active state
```

> **注意：** 如果日志后端为 UART，进入低功耗模式后日志输出会暂停（UART 被关闭）。

---

### 9. 高速 UART (1 Mbps)

**演示 Zephyr 异步 UART API (`uart_tx` / `uart_callback_set` / `uart_rx_buf_rsp`) 实现 1 Mbps 高速串口通信。**

支持三种 UART 工作模式：
- **Polling:** 阻塞式轮询，适合调试。
- **Interrupt-driven:** 中断驱动，每字节中断。
- **Async (DMA)：** EasyDMA 传输，适合高速通信。本示例使用此模式。

**测试步骤：**

1. 确保 BLE 连接已断开，日志终端已关闭（否则会影响 UART 通信带宽）。
2. 使用串口助手（如 "Serial Debug Assistant"）打开设备对应的 1 Mbps UART 端口：
   - nRF52840 / nRF5340：UART1 (1000000 baud)
   - nRF54L15：UART20 (1000000 baud)
3. 通过串口助手发送文件到设备，设备会**原样回传**同样的数据。
4. 验证发送与接收的文件内容一致（回环比对测试）。

日志输出：

```
[00:01:50.627,425] <inf> uart_thread: UART_RX_RDY 255
[00:01:50.627,541] <inf> uart_thread: uart received:
                          44 65 61 72 20 61 6c 6c  2c 0d 0a 20 0d 0a 41 73
                          ...
[00:01:50.630,096] <inf> uart_thread: UART_TX_DONE 255
```

> **注意：** 关闭日志终端以释放 UART 带宽。1000000 baud = ~125 KB/s 理论带宽，异步模式可持续满速收发。

---

### 10. Direct Test Mode (DTM)

**BLE RF PHY 一致性测试，支持与R&S CMW500, Anritsu MT8852 等认证测试仪配合使用。**

> **支持平台：** 仅 **nRF54L15 DK**。`CONFIG_DTM_IN_APP=y` 在 `boards/nrf54l15dk_nrf54l15_cpuapp.conf` 中启用。

**工作原理：**

DTM 在 `PRE_KERNEL_1` 阶段（早于 `main()` 函数）启动。通过读取 GPIO 状态决定运行模式：

| 复位时 Button 2 状态 | 运行模式 |
|---------------------|---------|
| **按住不放** | DTM 模式 |
| **未按下** | 正常应用程序 |

**DTM 规格：**

| 参数 | 值 |
|------|-----|
| 传输协议 | BLE 2-wire UART (DTM standard) |
| UART 接口 | UART20 |
| 波特率 | 19200 |
| 数据格式 | 8N1 |
| 流控 | 无 |

**测试步骤：**

1. 按住板上的 **Button 2**（P1.08）。
2. 按一下 RESET 按钮，保持 Button 2 按住约 100ms 以上。
3. 松开 Button 2。设备进入 DTM 模式，串口输出：

```
DTM mode enabled by Button2
Starting Direct Test Mode sample
```

4. 使用 **nRF Connect for Desktop → Direct Test Mode** 应用连接设备（选择对应 COM 口）。
5. 选择测试参数（频率、包类型、包长度等）并开始测试。

**支持的命令：**

- `LE_TRANSMITTER_TEST` / `LE_RECEIVER_TEST` / `LE_TEST_END`
- 支持的 PHY：1M / 2M / Coded S2 / Coded S8
- 支持的包类型：PRBS9 / PRBS15 / 0xF0 / 0xAA 等标准 BLE DTM 测试包
- TX 输出功率控制（vendor specific）
- Front-End Module (FEM) 控制（如适用）

**技术实现：**

DTM 绕过 Zephyr BLE 协议栈，直接操作 nRF54L15 的 RADIO 外设寄存器（`hal/nrf_radio.h`），配合 nrfx TIMER 和 DPPI 实现精确的射频时序控制。详见 `src/dtm/dtm.c`。

---

## 架构说明

### 线程分配

```
优先级    线程              功能
────────────────────────────────────────────
  3       nvs_thread         NVS Flash 存储
  4       settings_thread    Settings Flash 存储
  5       uart_thread        1 Mbps 高速 UART 收发
  7       spi_thread         SPI Master 通信
  7       io_thread          外部中断监听
  8       i2c_thread         I2C Master 通信
  9       adc_thread         ADC 采样
  -       main               主线程（日志 + 按钮处理 + BLE）
 10       application_work_q LED 闪烁工作队列
```


### 构建产物

Sysbuild 构建完成后，`build/` 目录下的关键文件：

| 文件 | 说明 |
|------|------|
| `dfu_application.zip` | 应用核 OTA 升级包 |
| `ble_comprehensive/zephyr/zephyr.signed.bin` | 应用核签名固件（单个） |
| `ble_comprehensive/zephyr/zephyr.hex` | 应用核 Intel HEX 文件 |
| `mcuboot/zephyr/zephyr.hex` | MCUboot 引导程序 |
| `ipc_radio/zephyr/zephyr.hex` | 网络核 IPC Radio 固件（nRF53/nRF54L） |

---

## SDK 补丁

本例子大部分功能直接就可以跑，不需要打补丁，但有一部分功能，由于本版本SDK还不支持，需要打补丁，补丁放在`sdk_change/` 目录。比如，`sdk_change/ncs_v3.4.x_dtm_in_app/` 目录下的 `mpsl_init.c` 补丁修改了 `nrf/subsys/mpsl/init/mpsl_init.c`：

```c
// 在 mpsl_radio_isr_wrapper 中添加 DTM 分流逻辑：
if (b_dtm_mode) {
    radio_handler(NULL);  // DTM 直接处理 RADIO 中断
    return 0;             // 不传递给 MPSL
}
// 否则走正常 MPSL 路径
MPSL_IRQ_RADIO_Handler();
```

**只有你使能了nRF54L15的DTM in application功能时，这个补丁才需要应用**， 否则你可以不管这个补丁！

其他 SDK 版本的补丁见 `sdk_change/` 目录下对应的子目录。

---
