# C 语言在线桌面端

新版入口为 desktop-c/main.c，使用 Windows C API；旧 qt-client 是保留的本机演示，不是新版入口。

桌面 UI 直接加载 https://alexis-zhang0503.github.io/intelligent-lab-booking-system/ ，不复制 HTML/CSS，不维护第二套签到逻辑。启动器先用 HTTPS HEAD 检查网站可访问，再用 Microsoft Edge --app 打开独立应用窗口，账号和数据使用线上网页。网页发布后的更新在再次打开或刷新时加载；启动参数增加时间戳以重新请求入口，但这不等于校验每一个静态资源的版本。登录状态由 Edge 管理。

## Windows 编译

安装 Visual Studio Build Tools 的 C++ 桌面开发工作负载和 Windows SDK（其中包含 C 编译器），在 x64 Native Tools Command Prompt 中进入 desktop-c，执行：

```bat
cl /nologo /W4 /utf-8 main.c /Fe:LabBooking.exe /link /SUBSYSTEM:WINDOWS winhttp.lib shell32.lib
```

运行 LabBooking.exe，需已安装 Microsoft Edge。没有网络或站点不可访问时提供重试/取消。此程序是 C 启动器加 Edge 网页应用窗口，不是内嵌 WebView，也不把网页源码变成 C；如果课程要求全部界面和业务逻辑由 C 实现，本方案不满足该要求。

## 验证范围

线上网页入口已在浏览器确认可加载。尚未在 Windows 编译和运行此 C 启动器，也未验证真实账号的签到/签退。需在 Windows 编译后核对：启动窗口、登录、预约记录一致、手动签到/签退、网页更新后重新打开、断网重试和导出文件。旧 Qt EXE 不会自动变成此版本，必须编译并运行新的 LabBooking.exe。
