# 原生 C 桌面与网页发布

源码根目录包含网页、backend、desktop-c、历史 qt-client 和 .github。把交付包 intelligent-lab-booking-system 内的文件合并到原仓库根目录（包括 .github），提交 main；不要把源码套入额外一级目录，也不要只上传 ZIP 代替源码。若覆盖旧版本，删除 desktop-c/src/site_policy.h、desktop-c/tests/site_policy_test.c 以及旧 vendor-sdk；发布目录不要保留 WebView2Loader.dll。新增 vendor/cJSON 文件必须提交。

GitHub Pages 使用 Actions 作为部署源。Deploy GitHub Pages 先在 Windows 上按 C 编译原生程序并测试，再生成网页、downloads/LabReservation-Windows-x64.zip 和 native-desktop.json，最后发布 _site。Build native C desktop 还单独提供 Windows 成品和启动检查。工作流尚未在远程运行；需要仓库启用 Actions 和 Pages 权限。

交付包的 static-site 是本地预构建的网页，可直接用于静态托管；包含这次原生 EXE 下载 ZIP。GitHub 文件上传应上传源码目录内的文件，而非 static-site 作为整个仓库。若站点换域名，修改 desktop-c/src/client.h、main.c 中的固定域名和下载地址再重编译；Supabase 服务配置也在 client.h。

网页仍独立使用 HTML/CSS/JS；桌面端用 C 原生 UI，只交换 JSON 数据，不渲染 HTML。网页 UI 改动不会自动转换成原生控件：更新原生界面时须同步修改 C、增加 LAB_NATIVE_VERSION，再提交发布。启动后台版本检查只提示下载，不自动安装。账号和业务数据始终共用云端，桌面端每 30 秒同步。

签到依赖当前云端函数。项目已包含 backend/supabase/v2-2-attendance-fix.sql，应在现有 V2 基础上由管理员执行并验证，不能只更新网页和 EXE 就认为数据库修复已生效。本交付未操作线上数据库、未推送或部署远程服务。勿在现有生产库盲目重建 schema.sql。

本地交叉编译和自动测试不能替代真实 Windows 验收：请使用预约时段内的真实测试预约，在“我的预约”选中记录，确认签到，刷新网页验证使用中，再签退验证已完成。尚未完成这项实机验收。
