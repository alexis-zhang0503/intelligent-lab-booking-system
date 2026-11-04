# macOS 原生 C 客户端 3.0.0

下载同一 v3.0.0 Release 中的 LabReservation-macOS-arm64.zip（Apple 芯片）或 LabReservation-macOS-x64.zip（Intel），解压后将 LabReservation.app 拖到应用程序。支持 macOS 13 及以上；构建所用 GTK 依赖可能要求更高的系统版本，以实际构建记录为准。

应用源码使用 C11、GTK 3 C API、系统 libcurl，复用 desktop-c/src/client.c 的认证、北京时间、预约和签到规则。没有 HTML、WebView 或 C++ 应用层。GTK 与所需动态库打包在应用中，用户不需要安装 Homebrew。界面为米色/棕色、40 台设备、四个页面，与 Windows 原生端功能对应；不是网页像素级复制。

功能：登录/注册、设备多选和多小时预约、本人预约签到/签退与取消、故障报告/维修/归档、账号管理、邀请码、统计、资料修改、时段推荐、智能分析。每 30 秒刷新云端，启动时检查版本；更新后需下载新版应用。

构建：在对应架构 Mac 上 brew install gtk+3 pkg-config dylibbundler，然后 bash desktop-c/macos/build.sh。CI 在 Apple 芯片和 Intel runner 分别运行共享 C 测试、编译、动态库路径检查、临时目录启动测试及 ad-hoc 签名校验，再上传现有 v3.0.0 Release。附件来自新增提交，不移动原 v3.0.0 标签；完整源码请查看 main 中 desktop-c/macos。

应用仅有 ad-hoc 签名，未经过 Apple Developer ID 签名或公证，首次打开可能被 Gatekeeper 拦截。确认下载来源后，在系统设置 → 隐私与安全性中允许打开。未使用真实生产账号测试登录或签到，也未修改数据库。云端权限与接口仍由 Supabase 验证。
