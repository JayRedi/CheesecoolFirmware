% DFU-PREFIX(1) dfu-util 0.11
% See AUTHORS file in source
% September 2021

# 名称
dfu-prefix - 添加、检查或删除特殊固件文件前缀

# 用法
**dfu-prefix** [ **\-s** *address* | **\-L** ] **\--add** *DFU_FILE*\
**dfu-prefix** [ **\-T** | **\-L** ] **\--check** *DFU_FILE*\
**dfu-prefix** [ **\-T** | **\-L** ] **\--delete** *DFU_FILE*\
**dfu-prefix** **\--help**\
**dfu-prefix** **\--version**

# 描述
**dfu-prefix** 可用于添加、检查或删除部分硬件制造商使用的前缀。
支持 TI 的 Stellaris 格式和 NXP 的 LPC 格式。

请注意，标准 DFU 固件文件没有前缀概念；`dfu-util` 之类的 DFU Host 工具会将
前缀作为普通固件 payload 的一部分传送给设备。

# 选项
-s, \--stellaris-address *address*
:（与 \--add 一起使用）向文件添加 TI Stellaris 地址前缀

-T, \--stellaris
:（与 \--delete 或 \--check 一起使用）处理文件中的 TI Stellaris 地址前缀

-L, \--lpc-prefix
:（与 \--add、\--delete 或 \--check 一起使用）使用 NXP LPC DFU 前缀格式

-h, \--help
: 显示帮助信息。

-V, \--version
: 显示软件版本。

# 示例
**dfu-prefix** \--stellaris-address 0x0100 \--add firmware.dfu
: 添加加载地址为 0x0100 的 Stellaris 前缀

**dfu-prefix** \--stellaris \--check firmware.dfu
: 检查文件 firmware.dfu 是否包含 Stellaris 前缀

**dfu-prefix** \--lpc-prefix \--delete firmware.dfu
: 从文件 firmware.dfu 中删除 LPC 前缀

# 退出值
**0**
: 成功（即使缺少前缀也返回成功）

**-64**
: 用法错误

# 错误报告
https://sourceforge.net/p/dfu-util/tickets/

# 版权
许可证 GPLv2：GNU GPL version 2

# 另请参阅
**dfu-suffix**(1), **dfu-util**(1)
