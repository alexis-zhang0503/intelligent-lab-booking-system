# 最终 Windows 桌面端说明

最终交付的桌面端仅为 `desktop-c/`。它是基于 Win32/C11 的原生应用，使用 WinHTTP 与网页端共用 Supabase 后端；旧 Qt 客户端和旧 WebView2 外壳均不属于最终交付内容。

构建命令、源码职责和测试边界见 [`../desktop-c/README.md`](../desktop-c/README.md)。网页端部署、数据库迁移和管理员账号开通仍以根目录 README 链接的后端文档为准。
