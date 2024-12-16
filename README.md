# HoloCubicX

本项目基于`稚晖君`大佬的`Holocubic`和`遛马小哥`的`HoloCubic_AIO`开发。

固件代码完全开源，共各位参考学习，若使用本项目二次开源或部分参考，请适当注明参考来源。
硬件较原版有修改：核心`ESP32-pico`改为`ESP32-s3`，陀螺仪`MPU6050`改为`QMI8658A`，请注意驱动和引脚变化。

* Holocubic的项目链接 https://github.com/peng-zhihui/HoloCubic
* HoloCubic_AIO的链接 https://github.com/ClimbSnail/HoloCubic_AIO
* 本项目链接 https://gitlab.forza0310.cn/root/holocubicx



### 主要特点
1. 以本人对软件工程的粗浅理解，对原项目代码进行重构。
2. 优化手势操作逻辑：将切换APP的手势从“长按”改为“大幅低头”，避免误操作。
2. 聚合多种APP，内置天气、时钟、相册、特效动画、视频播放、web设置等等。
2. 开机无论是否插接tf卡、陀螺仪是否焊接正常、是否连接wifi（一定要2.4G的wifi），都不影响系统启动和屏幕显示。
3. 程序相对模块化，低耦合。
4. 提供web界面进行配网以及其他设置选项。
5. 提供web端连入除了支持ip访问，也支持域名直接访问 http://holocubic （部分浏览器可能支持不好）
6. 提供web端的文件上传到SD卡（包括删除），无需拔插SD来更新图片。
7. `遛马小哥`开源上位机源码，可进行刷机等操作。 https://github.com/ClimbSnail/HoloCubic_AIO_Tool

