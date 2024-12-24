# HoloCubicX🥑

本项目基于`稚晖君`大佬的`Holocubic`和`遛马小哥`的`HoloCubic_AIO`开发，对AIO固件代码进行重构，优化系统框架和app功能。

固件代码完全开源，共各位参考学习，若使用本项目二次开源或部分参考，请适当注明参考来源。
硬件较原版有修改：核心`ESP32-pico`改为`ESP32-s3`，陀螺仪`MPU6050`改为`QMI8658A`，请注意驱动和引脚变化。

* Holocubic的项目链接 https://github.com/peng-zhihui/HoloCubic
* HoloCubic_AIO的链接 https://github.com/ClimbSnail/HoloCubic_AIO
* 本项目链接 https://github.com/Forza-cbw/HoloCubicX

<img src="https://github.com/Forza-cbw/HoloCubicX/blob/master/src/resource/%E6%95%88%E6%9E%9C%E5%9B%BE.jpg" width="200" />

### 重构清单
1. 系统框架优化：优化自启动逻辑；规范函数/变量名称；增加调试配置；System统一异步刷新屏幕。
2. 操作逻辑优化：将切换APP的手势从“长按”改为“大幅低头”，避免误操作。
3. 消息处理优化：统一send_to()行为，消息全进队列；区分同/异步消息，同步消息在当前调用栈下立即调用事件处理函数。
4. 显示优化：屏幕亮度可配置；新增屏保功能。
5. RGB优化：亮度渐变步长可配置。
4. Wifi优化：增加关闭AP的接口；区分Wifi-STA、Wifi-AP模式；立即生效对Wifi配置的修改。
4. Weather优化：重构代码；异步化屏幕刷新；修复ntp对时bug；优化显示界面；提示数据状态（最新/过期/更新）。
5. 2048优化：优化移动计算逻辑，精炼代码；重写渲染逻辑，解决渲染缓冲区与实际数据不一致问题。
6. Tomato优化：增加“低头”减少时长的功能；优化增减定时时长的速度。
7. Server优化：主动连接WIFI、退出关闭AP；APP名称改为宏定义；使用同步消息读写配置信息。
8. **............ 更多优化敬请期待 ............**

### 功能特点
1. 聚合多种APP，内置天气、时钟、相册、特效动画、视频播放、web设置等等。 
2. 开机无论是否插接tf卡、陀螺仪是否焊接正常、是否连接wifi，都不影响系统启动和屏幕显示。 
3. 程序相对模块化，低耦合。 
4. 提供web界面进行配网以及其他设置选项。 
5. 提供web端的文件上传到SD卡（包括删除），无需拔插SD来更新图片。
6. `遛马小哥`开源上位机源码，可进行刷机等操作。 https://github.com/ClimbSnail/HoloCubic_AIO_Tool

### **烧录地址**
本固件适用于esp32-s3-devkitc-1版本的HoloCubic，其他版本需要移植。注意烧录地址与pico版本有所差异。

|   地址  |  二进制文件   |
|-------|-------|
|  **0x0000***  |  bootloader.bin  |
|  0x8000  |  partitions.bin  |
|  0xe000  |  boot_app0.bin  |
|  0x10000  |  firmware.bin  |

### **调试配置**
esp32-s3内置JTAG，使用前需要安装驱动：https://dl.espressif.com/dl/idf-driver/idf-driver-esp32-usb-jtag-2021-07-15.zip 。