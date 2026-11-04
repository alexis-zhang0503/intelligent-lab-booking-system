# Windows 纯 C 桌面客户端（共享网页 UI）

这是 Win32 C11 程序，应用代码为 `src/main.c` 和 `src/site_policy.h`，没有 C++ 桥接层。使用 Microsoft WebView2 的官方 C COM 接口在原生窗口内加载已发布的实验室预约网站。

## 使用

Windows 10/11 x64：完整解压 Windows-x64 文件夹，运行 `LabReservation.exe`。`WebView2Loader.dll` 必须放在 EXE 旁边。如果缺少 Microsoft Edge WebView2 Runtime，程序会显示安装按钮，打开微软官方下载页；安装后重新运行。

请用网页上的同一个账号登录。桌面版和普通浏览器分别保存登录会话，但读取和写入同一 Supabase 数据库。旧 Qt 本地账号、预约 JSON 不会自动成为云端账号或记录。

- “我的”、预约、签到、签退、管理功能和 logo 都直接显示发布网站里的同一套 UI。
- 每次启动或点击“刷新”，加载带新请求标记的站点入口；站点在启动时不使用 HTTP 缓存读取版本清单。
- 发布构建会为 HTML、CSS、JS 和模块依赖加上统一版本。普通网页和桌面内的网页都按相同方式更新。
- 程序保持打开时每分钟及重新获得焦点时检查网页版本，发现更新会提示“更新界面”。为避免中断填写或签到请求，不会强制刷新正在操作的页面。
- 这更新的是网页界面和业务逻辑；未来修改 C 宿主程序本身仍需要重新下载 EXE。
- 要联网。服务不可达时会显示连接失败及重试入口，不会编造离线预约或签到成功记录。

## C 语言要求的准确范围

Windows 宿主、窗口、COM 回调、站点导航控制、启动与重试逻辑使用 C；共享界面和业务前端仍是网站已有的 HTML/CSS/JavaScript。如果课程要求“全部 UI 和业务代码也必须纯 C、禁止 WebView”，本架构不能满足那个额外要求，需要采用原生 C UI 配合统一后端，并为客户端提供独立更新流程。网页 UI 自动一致在本架构中通过直接复用已发布网页实现。

## Windows 源码构建

安装 Visual Studio Build Tools 的 C++ 桌面工具链与 Windows SDK（用于提供 C 编译器和 Win32 头文件；本程序仍按 C 编译）。在 x64 Native Tools Command Prompt 中执行：

```powershell
powershell -ExecutionPolicy Bypass -File desktop-c/build.ps1
```

脚本从微软 NuGet 下载固定版本 WebView2 SDK 并校验 SHA-256，用 `/TC /std:c11` 编译，输出 `desktop-c/dist/`。仅脚本文件需要当前终端允许运行，不要求改机器全局策略。GitHub Actions 的 Build shared-UI C desktop 也会构建并上传 Windows x64 成品。

## 验证和限制

本交付已用 Zig/Clang 交叉编译为 Windows x64 GUI EXE，启用 C11、Wall、Wextra、Werror；图标和应用清单已嵌入，可执行文件格式、导入表和资源已检查。站点导航策略的原生 C 测试通过。

当前环境不是 Windows，尚未实机执行 WebView2、真实账号登录或签到。CI 增加了 Windows 原生构建和窗口启动检查，但本次未在 GitHub 上触发它。真实签到仍取决于 Supabase 是否已应用仓库内的 V2 和签到修复迁移。

Microsoft WebView2 官方文档：https://learn.microsoft.com/en-us/microsoft-edge/webview2/get-started/win32

Loader 与 SDK 来自 Microsoft.Web.WebView2 1.0.2903.40，分发目录附 Microsoft LICENSE.txt 和 NOTICE.txt。
