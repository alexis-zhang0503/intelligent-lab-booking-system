# 智能实验室预约系统

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

## 本地预览

在项目根目录运行 python -m http.server 4173，然后访问 http://localhost:4173。

## GitHub Pages

推送 main 分支后，GitHub Actions 会部署页面。首次部署后在仓库 Settings → Pages 查看公开链接。
