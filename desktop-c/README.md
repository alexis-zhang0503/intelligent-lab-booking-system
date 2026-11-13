# Windows C11 桌面端

这是智能实验室预约系统的最终 Windows 桌面客户端。应用使用 Win32 窗口、WinHTTP 和 C11 实现，直接调用与网页端相同的 Supabase 认证、预约、签到、故障处理和统计接口；不依赖 Qt、WebView2 或浏览器运行时。

## 源码结构

按答辩分工组织：`src/` 下的三个编号文件是要讲解的核心逻辑，每个文件开头都有讲解提纲和可能被问到的问题；其余代码都放在 `src/other/`。

| 文件 | 讲解人 | 内容 |
|---|---|---|
| `src/1_cloud_data.c` | 第 1 人 | 和云端通信、令牌续期、分页加载数据、登录 |
| `src/2_booking.c` | 第 2 人 | 提交预约、北京时间换算、签到签退规则 |
| `src/3_ai_agent.c` | 第 3 人 | AI 智能助手：打包请求、对话状态、两步确认 |
| `src/client.h` | 共用 | 三个模块的声明，按讲解人分段 |

`src/other/`（答辩不讲）：

- `main.c`：Win32 界面、页面交互与异步任务调度（与三个模块相关的位置标有【答辩·第N人】）；
- `win_http.c`、`win_http.h`：基于 WinHTTP 的 HTTPS 请求实现；
- `client_common.c`：读取 JSON、复制字符串、清空内存等基础工具；
- `client_extras.c`：版本比较、管理员统计、导出 CSV、报表排版；
- `ui_theme.h`：桌面界面主题；

其他：

- `vendor/cJSON.*`：JSON 解析库；
- `tests/client_test.c`：无真实账号依赖的客户端协议与边界测试。

## 构建与测试

在安装了 Visual Studio C++ 工具和 Windows SDK 的 **x64 Native Tools Command Prompt for Visual Studio** 中，于仓库根目录执行：

```powershell
powershell -ExecutionPolicy Bypass -File desktop-c/build.ps1
```

脚本会从 `assets/lab-logo.ico.b64` 生成构建时使用的图标，编译并运行 `ClientCheck.exe`，再生成 `LabReservation.exe`。构建产物位于 `desktop-c/dist/`，不应提交到仓库。GitHub Actions 的 **Build native C desktop** 工作流会在 Windows 环境执行相同构建并上传产物。

## 后端前提

桌面端调用仓库内 `backend/supabase/` 所定义的数据库表、RLS、RPC 和认证流程。部署前请先按根目录 README 所链接的后端说明完成迁移与管理员账号开通。客户端测试使用模拟传输层，不代替真实 Supabase 环境中的端到端验证。
