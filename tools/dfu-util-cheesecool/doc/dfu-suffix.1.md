% DFU-SUFFIX(1) dfu-util 0.11
% See AUTHORS file in source
% September 2021

# 名称
dfu-suffix - 添加、检查或删除 DFU 固件文件后缀

# 用法
**dfu-suffix** [*options*] **\--add** *DFU_FILE*\
**dfu-suffix** **\--check** *DFU_FILE*\
**dfu-suffix** **\--delete** *DFU_FILE*\
**dfu-suffix** **\--help**\
**dfu-suffix** **\--version**

# 描述
**dfu-suffix** 可用于添加、检查或删除 DFU 固件文件后缀，建议使用它来安全匹配
固件文件与设备。

请注意，DFU 标准建议使用后缀，但并不强制要求。`dfu-util` 之类的 DFU Host 工具
会识别后缀并用它检查设备是否匹配，但不会将后缀传送给设备。

# 选项
-v, \--vid *vendorID*
: 指定 USB vendor ID（十六进制）

-p, \--pid *productID*
: 指定 USB product ID（十六进制）

-d, \--did *deviceID*
: 指定 USB device ID（十六进制）

-S, \--spec *version*
: 指定 DFU specification version（十六进制）

-h, \--help
: 显示帮助信息。

-V, \--version
: 显示软件版本。

# 示例
**dfu-suffix** \--vid 0123 \--add firmware.dfu
: 添加匹配 vendor 0x0123 和 product ID 0x4567 的后缀。
由于未指定 product 和 device ID，二者将使用通配值 0xFFFF。

**dfu-suffix** \--check firmware.dfu
: 检查文件 firmware.dfu 是否包含有效 DFU 后缀

**dfu-suffix** \--delete firmware.dfu
: 从文件 firmware.dfu 中删除有效 DFU 后缀

# 退出值
**0**
: 成功（即使缺少后缀也返回成功）

**-64**
: 用法错误

# 限制
**dfu-suffix** 无法区分损坏的 DFU 后缀（例如 checksum 不匹配）和不存在的后缀，
因此只能删除有效后缀。

# 错误报告
https://sourceforge.net/p/dfu-util/tickets/

# 版权
许可证 GPLv2：GNU GPL version 2

# 另请参阅
**dfu-prefix**(1), **dfu-util**(1)
