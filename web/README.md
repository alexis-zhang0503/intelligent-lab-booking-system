# 网页端

用 HTML、CSS 和原生 JavaScript（ES 模块）编写，通过 Supabase 官方 JS 库连接云端，由 GitHub Pages 发布。

## 目录结构

```
web/
├── index.html              页面入口：页面骨架，引入样式与启动脚本
├── app.js                  页面主逻辑：登录注册、预约、签到签退、故障、管理员功能、AI 助手
├── version.json            当前版本号；页面定期检查，发现新版本时提示刷新
├── js/
│   ├── core/               与界面无关的业务规则
│   │   ├── bootstrap.js        启动：读取版本号后加载 app.js
│   │   ├── supabaseClient.js   创建云端连接
│   │   ├── authRules.js        注册校验与登录错误提示
│   │   ├── hourlyRules.js      时段规则：08:00–17:00、40 台设备
│   │   └── attendanceRules.js  签到签退规则
│   ├── ui/introMotion.js   登录页动画
│   └── vendor/supabase.js  第三方库：Supabase 客户端
├── css/                    样式；tokens.css 定义颜色、字号等设计变量
├── assets/                 图标与插图
├── scripts/build_site.py   发布构建：复制 web/ 并给所有本地资源加上同一个版本号
└── tests/                  Node 单元测试与构建脚本测试
```

## 本地运行

```
cd web
python -m http.server 4173
```

然后用浏览器打开 http://localhost:4173 。不能直接双击 `index.html`，浏览器会拦截本地文件之间的模块加载。动画库 GSAP 从 CDN 加载，需要联网。

## 测试

在仓库根目录执行：

```
node --test web/tests/*.test.cjs web/tests/*.test.mjs
python -m unittest discover -s web/tests -p test_shared_site_build.py
```
