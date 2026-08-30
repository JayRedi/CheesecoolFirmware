# 固件更新流程

正常 Application 链接在 `0x2000`；因此 `application.bin` 绝不能作为从零地址开始的镜像发送给 WCH ROM ISP。

`scripts/upload_dfu.py` 是正常 Application 环境的 PlatformIO 自定义上传器：

1. 检查已验证的 CheeseCool DFU 设备 `1a86:8035`。
2. 如果不存在，检查 Application HID 设备 `1a86:fe01`，并发送协议命令 8（`CMD_ENTER_DFU`）。
3. 以 200 ms 间隔轮询 DFU 设备，最长 5 秒。
4. 执行 `dfu-util -a 0 -d 1a86:8035 -D application.bin -R`。

首次安装仍需要通过 WCH ROM ISP 写入现有 factory image。Bootloader 和诊断环境均未启用自动上传器。
