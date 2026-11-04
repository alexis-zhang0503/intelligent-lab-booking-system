# Qt/C++ Windows 客户端

使用 Qt 6 Widgets / C++17 / CMake 的实验室预约桌面客户端。本次更新将原来的英文单页界面调整为与网页端一致的中文导航和浅蓝白色卡片风格，并补齐独立的“我的”页面。

## 页面与功能

- 学生：预约、时长、故障、我的。
- 管理员：总览、设备、用户、故障、我的。
- “我的”：个人资料、今日预约、常用设备、常用时段、未来预约、完整预约历史（包括已取消）、实际签到/签退时间及实际使用时长。
- 预约历史中选中一条记录后，按状态启用签到、签退或取消。签到限预约时段内，时间统一按北京时间；已签到记录可以签退；只允许取消未开始且未签到的预约。
- 时长页面分别统计未取消的预约小时数与已经完成签到签退的实际使用时长。
- 保留 40 台设备、08:00–17:00 一小时时段、多设备预约、故障上报、管理员维护、学生账号管理和 count.txt / error.txt 导出。
- 定时刷新保留仍有效的设备选择、预约记录选择和滚动位置；退出登录后停止旧页面定时器。

## Windows 编译

安装 Qt 6.4 或更新版本（Core / Gui / Widgets）、与该 Qt 套件匹配的编译器，以及 CMake 3.21 或更新版本。请使用 Qt 套件的命令行环境，在项目根目录运行：

```powershell
cmake -S qt-client -B qt-client/build -DBUILD_TESTING=ON
cmake --build qt-client/build --config Release
ctest --test-dir qt-client/build -C Release --output-on-failure
```

若找不到 Qt，请在配置命令中添加 `-DCMAKE_PREFIX_PATH="你的Qt套件目录"`，例如 Qt 安装目录下对应 MSVC 或 MinGW 的套件目录。编译器必须与 Qt 套件匹配。项目已为 MSVC 启用 UTF-8 源码编译。

Windows 多配置生成器通常将程序输出到 `qt-client/build/Release/IntelligentLabReservationQt.exe`；单配置生成器通常输出到 `qt-client/build/IntelligentLabReservationQt.exe`。分发到没有安装 Qt 的电脑前，使用对应 Qt 套件的 `windeployqt` 为 EXE 收集运行依赖。

## 本机数据与账号

桌面端沿用 Windows 用户应用数据目录中的本地 JSON 数据，原有数据格式不变。公开注册只创建学生账号。首次运行且用户表为空时，维护者可以设置 `LAB_DEMO_ADMIN_PASSWORD` 环境变量来创建 `admin` 演示管理员账号；程序中没有硬编码管理员密码。

**桌面端仍是本地演示客户端，不会同步网页端的 Supabase 账号或预约。** UI 对齐不等于云端数据接入。网页端的多时段预约、AI 助手、管理员邀请码等服务不在本次桌面 UI 更新范围内；桌面端仍按每次一个小时预约。

## 验证范围

- `LabRepositoryCheck`：现有本地业务规则与签到签退边界测试。
- `LabWindowCheck`：新增无屏幕 GUI 冒烟测试，覆盖登录、导航、“我的”历史包含已取消记录、实际时长、按钮状态、取消后保留历史、刷新保持选中、退出并重新登录后只有一个刷新定时器。
- 两个测试均已接入 CTest；现有 GitHub Actions 的 CMake / CTest 步骤会包含它们。
- 本次修改环境没有 Qt / CMake / C++ 编译器，因此未执行桌面程序编译、GUI 测试或截图验收，发布包不包含已编译 EXE。网页端现有 37 项业务回归测试已通过。

详见 `../docs/desktop-ui-alignment.md`。原签到数据库部署说明仍见 `../docs/attendance-fix.md`。
