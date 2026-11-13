# Windows 桌面端

用 C11 和 Win32 API 编写的原生桌面程序，通过 WinHTTP 连接云端，不依赖浏览器或其他运行时，功能与网页端一致。

## 架构

代码分为三层，依赖方向自上而下：

```
ui/        界面层：窗口、控件、后台线程调度（Win32）
  │
core/      业务层：通信、预约、签到、AI 助手、统计导出（纯 C，不依赖平台）
  │  ← 通过 LabTransport 函数指针注入网络发送实现
platform/  平台层：用 WinHTTP 发送 HTTPS 请求
```

业务层不调用任何 Windows API，网络发送由界面层在启动时注入；测试时换成模拟实现，即可在不联网的情况下覆盖全部业务逻辑。

## 目录结构

```
desktop/
├── src/
│   ├── core/
│   │   ├── client.h          业务层接口：数据结构与全部函数声明
│   │   ├── cloud_api.c       云端访问：请求管线、令牌续期、分页加载数据、登录注册
│   │   ├── booking.c         预约与签到：提交预约、北京时间换算、签到签退规则
│   │   ├── ai_agent.c        AI 预约助手：请求组装、对话状态、两步确认
│   │   ├── common.c          基础工具：读取 JSON、安全复制字符串、版本比较
│   │   └── reports.c         统计与导出：使用统计、报表排版、导出 CSV
│   ├── platform/
│   │   └── win_http.c / .h   WinHTTP 实现的 LabTransport
│   └── ui/
│       ├── main.c            窗口界面、按钮响应、后台线程
│       └── ui_theme.h        界面主题与显示格式
├── tests/client_test.c       业务层测试（模拟传输层，不需要真实账号）
├── vendor/                   第三方库 cJSON
├── assets/                   程序图标（Base64，编译时还原）
├── app.rc / app.manifest     Windows 资源
└── build.ps1                 编译脚本
```

## 编译

需要 Visual Studio（含 C++ 工具与 Windows SDK）。在 **x64 Native Tools Command Prompt for VS** 中，于仓库根目录执行：

```
powershell -ExecutionPolicy Bypass -File desktop/build.ps1
```

脚本先编译并运行 `tests/client_test.c`，测试通过后在 `desktop/dist/` 生成 `LabReservation.exe`。GitHub Actions 的 *Build native C desktop* 工作流执行同样的构建并上传产物。
