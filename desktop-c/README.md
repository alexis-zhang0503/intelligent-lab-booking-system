# 实验室预约 · 原生 C 桌面 3.1.0

Windows 10/11 x64。解压 Windows-x64 文件夹后运行 LabReservation.exe，无需安装其他运行环境。图标沿用米色背景、棕色 NJ 标志。

## 界面

Windows 和 macOS 沿用当前网页的浅灰背景、白色圆角卡片、紫色主色和左侧导航。学生入口统一为“预约 / 智能助手 / 我的”；故障记录从“我的”进入。预约页使用 8 列设备平面图、中间走道、右侧时间和实验室状态卡片。“我的”显示个人资料、今日预约、偏好、实际签到/签退时间和使用分钟数。原生控件字体和系统对话框仍随操作系统变化。

## 实现

所有桌面应用源码均为 C11：Win32 原生控件/GDI 绘制界面，WinHTTP 发起 HTTPS 请求，cJSON 1.7.19 解析数据。无 HTML、JavaScript、WebView、Electron、Qt 或 C++ 桥接。Windows 自身系统组件及 C 运行库不属于本项目应用源码；MSVC 构建使用 /MT；交付的交叉编译版使用 Windows 10/11 自带的 UCRT 系统组件。

- 登录/注册使用与网页相同的 Supabase Auth；学生填学号，管理员填邮箱。管理员角色由云端 profiles 决定。
- 40 台设备、日期/小时、多台多小时预约、我的预约、签到/签退、取消预约。
- 故障报告、维修完成、归档、学生账号停用、管理员邀请码、本人资料修改、当日统计、规则推荐和智能分析。
- 界面使用原生按钮、列表、日期控件、下拉框和 MessageBox 确认窗口。智能分析在说明框输入问题；预约仍需自行选择设备、时段并确认。
- 签到按北京时间计算预约小时；云端最终校验身份、状态和时段。提交在工作线程执行，界面不会在网络请求期间阻塞；提交期间禁用重复操作。
- 所有数据读写同一个云端后端，不使用旧 Qt 本地 JSON。登录后每 30 秒刷新，也可手动刷新。登录令牌仅保留在进程内存，退出后需重新登录。
- 多小时预约按小时分别提交，每小时独立幂等编号；遇到冲突保留前面成功的预约并报告部分结果。超时后先刷新核实，不能把网络失败等同于未预约。

## 网页与桌面同步的边界

两端共用账号、预约、签到及管理接口和数据；网页产生的数据会在桌面刷新后出现。原生 C 界面不加载网页，HTML/CSS 的布局修改不会直接成为 Windows 控件布局。原生界面的变化需更新 C 源码、编译和重新下载 EXE。

启动时后台检查 native-desktop.json，不阻塞登录；也可点击“检查版本”。发现较新语义版本后，由用户确认打开固定的官方下载地址下载 ZIP。程序不会静默安装或替换自身。发布前须增加 src/client.h 的 LAB_NATIVE_VERSION。托管地址在同一头文件，下载链接在 main.c；更换域名需同步修改并重新编译。

## 使用提示

预约页选择日期、开始小时、结束小时及设备；紫色表示选中，浅色表示空闲，灰色表示所选时段占用，蓝色表示使用中，红色表示故障。选中不保证可预约，云端提交时会再次校验。

“我的”选中记录后点击“签到 / 签退”，确认框显示设备、日期和小时。只能在预约小时内签到，使用中才可签退。错误会显示云端原因，不会把失败显示成成功。

故障管理：先在预约页选中设备，在说明框填写故障说明，再提交；多选时使用编号最小的一台。管理员可选故障记录维修/归档，云端检查归档等待时间。账号管理可停用所选学生，生成 7 天邀请码，查询所选日期统计。修改本人资料：说明框输入 `姓名|学号或邮箱`，再点“修改本人资料”。

桌面功能沿用主要云端流程，但界面不是网页的逐像素复制。当前统计为所选单日，智能分析只展示建议，未实现网页完整的周视图和助手候选一键确认交互。

## 构建与测试

安装 Visual Studio Build Tools 的 MSVC 和 Windows SDK；在 x64 Native Tools Command Prompt 中运行：

```powershell
powershell -ExecutionPolicy Bypass -File desktop-c/build.ps1
```

`/TC /std:c11 /utf-8` 编译所有翻译单元；不下载浏览器 SDK。输出 desktop-c/dist/，包含 EXE、说明及 cJSON 许可证。GitHub Actions 的 Build native C desktop 会运行 C 测试、编译并检查窗口启动；Pages 部署流程也会构建桌面 ZIP 及版本清单。

可在 Linux/macOS 运行便携接口测试：

```sh
cc -std=c11 -Wall -Wextra -Werror desktop-c/tests/client_test.c desktop-c/src/client.c desktop-c/vendor/cJSON.c -o client-test
./client-test
```

## 验证状态与后端前提

共享 C 测试覆盖认证、时区跨日、预约权限、签到接口、部分预约、版本比较和实际使用时长。CI 在 Windows、macOS Apple 芯片和 Intel 上编译、运行测试并检查启动；同时用隔离示例数据生成登录、预约、我的、智能助手四页原生截图。预览模式不登录真实账号、不访问云端、不提交操作。真实账号端到端登录和签到仍需在对应设备上核对。

现有云端需已经安装项目的账号、V2、故障管理相关迁移，尤其 backend/supabase/v2-2-attendance-fix.sql。新客户端不会自动修改数据库。若提示接口缺失，请管理员按照 docs 中的后端迁移说明核对；不要在已有数据库盲目重新运行 schema.sql。

第三方：vendor/cJSON.c、cJSON.h 来自 DaveGamble/cJSON v1.7.19，MIT 许可证见 vendor/CJSON-LICENSE.txt。


## 原生界面预览

运行 `LabReservation.exe --preview=booking --preview-output=preview.bmp`（Mac 使用应用内启动器并输出 PNG）。支持 `login`、`booking`、`profile`、`agent`。截图仅绘制当前应用窗口；CI 产物名为 `native-ui-windows` 和 `native-ui-macos-<arch>`。
