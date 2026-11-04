# 原生 C 3.0.0 验证记录

基于现有项目继续修改，保留最新网页、logo 和后端签到时区修复。桌面实现已移除 WebView2 依赖，重新实现为 Win32 C 控件和 WinHTTP 云端客户端。

已执行：

- GCC C11、Wall/Wextra/Werror 编译便携接口测试；AddressSanitizer 和 UndefinedBehaviorSanitizer 下测试通过。LeakSanitizer 因当前环境 ptrace 限制关闭。
- C 测试覆盖账号解析缺失接口回退、正常登录与不完整响应、401 单次令牌刷新、北京时间开始/结束边界、非本人预约拒绝、签到/签退 RPC 参数、多小时部分成功、重复设备、云端角色覆盖和停用账号、注册、版本比较。
- Zig/Clang C11、Wall/Wextra/Werror 交叉编译 Windows x64 GUI EXE，无编译警告。导入仅含 Windows 系统库及 UCRT，无 WebView2Loader、C++ 桥接或网页资源；图标和清单已嵌入。
- 网页回归测试 50 项通过，Python 站点/原生下载打包测试 6 项通过。
- PGlite 上执行签到数据库 fixture、v2-2-attendance-fix.sql、attendance-db.sql 通过，验证云端函数时段、权限、签到签退状态和记录写入。

未执行：真实 Windows 窗口及交互测试、真实账号登录和签到、远程 GitHub Actions、生产数据库迁移及远程部署。打包内容可供上传，但不代表已经上线；真实签到仍需线上数据库安装正确的接口并完成验收。

功能边界：原生桌面覆盖主要预约/签到/故障/账号流程，提供当日统计和智能建议；未复制网页所有周视图及助手候选一键确认交互。桌面原生 UI 通过新的 EXE 更新，不通过 HTML 自动变更。
