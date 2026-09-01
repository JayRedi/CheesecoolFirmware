# 固件更新流程

Application 链接在 `0x2000`；`application.bin` 绝不能作为从零地址开始的镜像写入。

软件触发 DFU 已移除。先按既有官方/非软件 Bootloader-entry procedure 使 CheeseCool DFU 设备
`1a86:8035` 枚举，然后执行：

```text
pio run -e ch32x035f8u6_evt_r0 -t upload
```

`scripts/upload_dfu.py` 会确认 DFU device 已经存在，然后调用：

```text
dfu-util -a 0 -d 1a86:8035 -D application.bin -R
```

它不会检测 HID Application 后发送命令 `0x08` 或 `0x0D`，也不会请求 reset。无 Application 时，
Bootloader 的 `DFU_ENTER_IF_NO_APP=1` 保留为防砖回退；不要通过刻意破坏有效 Application 来测试该路径。
