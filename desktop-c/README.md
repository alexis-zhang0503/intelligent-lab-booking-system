# Windows C11 桌面端

这是智能实验室预约系统的最终 Windows 桌面客户端。应用使用 Win32 窗口、WinHTTP 和 C11 实现，直接调用与网页端相同的 Supabase 认证、预约、签到、故障处理和统计接口；不依赖 Qt、WebView2 或浏览器运行时。

## 源码结构

- `src/main.c`：Win32 界面、页面交互与异步任务调度；
- `src/client.c`、`src/client.h`：认证、预约、签到、故障、统计与智能助手的 API 客户端；
- `src/win_http.c`：基于 WinHTTP 的 HTTPS 请求实现；
- `src/ui_theme.h`：桌面界面主题；
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
