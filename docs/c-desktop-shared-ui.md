# 桌面端与网页端统一界面

新版入口是 desktop-c，不再给学生分发 qt-client 的离线演示版。qt-client 保留作历史参考。

两种入口共用部署站点：

https://alexis-zhang0503.github.io/intelligent-lab-booking-system/

C11 Windows 窗口通过 WebView2 打开同一站点，两端的“我的”、签到按钮、时段判断、Supabase RPC 和 UI 都来自同一套网站源码。修改网页并成功部署之后，桌面端下次启动会请求当前发布版本；已打开的页面收到版本提示后点击更新。

## 上传

把交付包 intelligent-lab-booking-system 目录内的文件覆盖到原仓库根目录，包括 .github。保留目录结构；不要把压缩包当作一个文件上传来代替源码。提交 main 后，Deploy GitHub Pages 自动生成版本和站点资产，再发布 _site；Build shared-UI C desktop 生成原生 C 桌面程序。

如果使用其他静态网站托管，上传交付包 static-site 内的文件即可。C 宿主默认地址是上述 GitHub Pages 地址；迁移域名时需修改 desktop-c/src/site_policy.h 中的固定站点地址并重新编译 C 程序。

## 签到数据库

此架构统一客户端行为，不能代替数据库迁移。已部署完整 V2 的项目需执行 backend/supabase/v2-2-attendance-fix.sql。缺少 V2 表或函数时，先按 docs/v2-deployment.md 检查既有数据库。不得在生产库运行 tests 中的空库测试脚本。

本次提供源码、Windows 交叉编译成品和静态发布目录；没有操作线上数据库或推送 GitHub。
