# 智能实验室预约系统

包含 GitHub Pages Web 客户端与 Windows Qt/C++ 客户端的课程作业项目。

## 功能

- 学生公开注册；管理员由项目维护者在 Supabase 后台预置并开通；
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

Web 本地演示模式不需要服务器，但仅保存当前浏览器的学生数据，不能在不同设备或浏览器间同步。配置 `backend/supabase/schema.sql` 所需的 Supabase 项目后，数据库唯一约束与原子预约函数才提供跨设备冲突控制。请勿在本地演示模式使用真实密码。

## 本地预览

在项目根目录运行 python -m http.server 4173，然后访问 http://localhost:4173。

## GitHub Pages

推送 main 分支后，GitHub Actions 会部署页面。首次部署后在仓库 Settings → Pages 查看公开链接。
