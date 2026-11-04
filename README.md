# 智能实验室预约系统

当前桌面端入口为 `desktop-c/`：C11 + Win32 原生界面 + WinHTTP，直接调用与网页相同的云端接口，无 HTML/WebView。旧 `qt-client/` 仅保留作历史离线演示。


包含 GitHub Pages Web 客户端与 Windows Qt/C++ 客户端的课程作业项目。

## 功能

- 学生公开注册；首个管理员由项目维护者开通，后续管理员使用现有管理员生成的一次性邀请码注册；
- 40 台设备、08:00–17:00 共 9 个一小时时段；
- 学生多设备、多时段预约与取消，冲突时段禁止重复预约；
- 学生故障上报，管理员标记、恢复设备并注销学生账号；
- 管理员查看用户、当前使用、每日及一周使用统计；
- 选做的输入校验、累计实验时长；
- 下载题目要求的 count.txt 与 error.txt。

## 两个客户端

- 根目录 Web 客户端：用于 GitHub Pages 展示与移动端访问；
- [qt-client](qt-client/README.md)：Qt 6 Widgets/C++ Windows 图形界面；
- [backend/supabase](backend/supabase/README.md)：供两个客户端共享的 Supabase 数据库、权限与原子预约契约。

管理员开通步骤见 [backend/supabase/ADMIN_ACCOUNT_RUNBOOK.md](backend/supabase/ADMIN_ACCOUNT_RUNBOOK.md)。

## 数据范围

Web 客户端已接入 Supabase：请先执行 `backend/supabase/schema.sql`、`backend/supabase/cloud-migration.sql` 和 `backend/supabase/admin-invite-migration.sql`，再在 **Authentication → Providers → Email** 关闭 **Confirm email**。之后学生注册、管理员权限、设备状态、预约冲突和实时刷新均以共享数据库为准。Qt 客户端目前保留离线本地演示模式，尚未接入云端。

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

网页签到时间、异常提示和 Qt 本地签到实现已更新，部署所需的数据库步骤及验证范围见 [签到修复与上传说明](docs/attendance-fix.md)。只推送前端文件不会更新 Supabase 函数，必须按说明执行增量迁移。

## 推荐桌面客户端：纯 C，共享网页界面

[desktop-c](desktop-c/README.md) 使用 C11、Win32 原生控件和 WinHTTP；桌面 UI 与业务逻辑不依赖网页代码。账号和数据共用云端，登录后每 30 秒刷新；原生 UI 变化需发布新的 EXE，启动时检查版本并提示下载。`qt-client` 仅保留历史离线演示。

上传和静态托管步骤见 [原生 C 部署说明](docs/c-desktop-native.md)。
