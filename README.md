# 智能实验室预约系统

课程项目的最终交付版本，包含 GitHub Pages 网页端与 Windows 原生桌面端；两端连接同一套 Supabase 数据库、权限规则和预约接口。

## 功能

- 学生公开注册；首个管理员由项目维护者开通，后续管理员使用现有管理员生成的一次性邀请码注册；
- 40 台设备、08:00–17:00 共 9 个一小时时段；
- 学生多设备、多时段预约与取消，冲突时段禁止重复预约；
- 学生故障上报，管理员标记、恢复设备并注销学生账号；
- 管理员查看用户、当前使用、每日及一周使用统计；
- 选做的输入校验、累计实验时长；
- 下载题目要求的 count.txt 与 error.txt。

## 项目结构

- 根目录与 `web/`：用于 GitHub Pages 展示与移动端访问的网页客户端；
- [desktop-c](desktop-c/README.md)：最终 Windows Win32/C11 原生桌面客户端；
- [backend/supabase](backend/supabase/README.md)：供两个客户端共享的 Supabase 数据库、权限与原子预约契约。

旧 Qt 客户端和旧 WebView2 外壳已移除，本仓库仅保留 `desktop-c/` 中的最终桌面端实现。

管理员开通步骤见 [backend/supabase/ADMIN_ACCOUNT_RUNBOOK.md](backend/supabase/ADMIN_ACCOUNT_RUNBOOK.md)。

## 数据范围

Web 与桌面客户端均已接入 Supabase：请先执行 `backend/supabase/schema.sql`、`backend/supabase/cloud-migration.sql` 和 `backend/supabase/admin-invite-migration.sql`，再在 **Authentication → Providers → Email** 关闭 **Confirm email**。之后学生注册、管理员权限、设备状态、预约冲突和实时刷新均以共享数据库为准。

## V2.0 智能预约升级

- 新增的 [V2 Schema 审计](docs/v2-schema-audit.md) 记录了当前表、RLS、索引、未知项和迁移前置条件；
- [V2 迁移](backend/supabase/v2-0-migration.sql) 是增量 SQL：复用既有 `reservations`，新增受控状态、签到使用记录、开放规则、维护时段、Agent 审计和调度/指标 RPC；
- 网页改用 V2 预约、取消、手动签到/签退和实时候选方案 API，并提供 DeepSeek 智能助手与只读管理员分析入口；
- 部署前请依次阅读 [架构](docs/v2-architecture.md)、[API](docs/v2-api.md) 与 [部署步骤](docs/v2-deployment.md)。生产 Supabase 迁移、Edge Function 部署和 DeepSeek 密钥配置均未自动执行。

## 本地预览

在项目根目录运行 python -m http.server 4173，然后访问 http://localhost:4173。

## GitHub Pages

推送 main 分支后，GitHub Actions 会部署页面。首次部署后在仓库 Settings → Pages 查看公开链接。

## 签到修复

网页与原生 C 桌面端均通过同一套 Supabase 签到接口工作。只推送前端文件不会更新 Supabase 函数，必须按后端说明执行增量迁移。

## Windows 原生 C11 桌面端

[`desktop-c`](desktop-c/README.md) 是 C11 Win32 客户端，使用 WinHTTP 调用与网页端相同的 Supabase 认证、预约、签到、故障处理和统计接口，不依赖 Qt、WebView2 或浏览器运行时。请在 Visual Studio 的 x64 Native Tools Command Prompt 中执行 `powershell -ExecutionPolicy Bypass -File desktop-c/build.ps1`；构建脚本会运行客户端测试并在 `desktop-c/dist/` 输出 `LabReservation.exe`。

完整构建、依赖和验证说明见 [desktop-c/README.md](desktop-c/README.md)。
