# 智能实验室预约系统

面向实验室设备的预约管理系统。学生可以注册、预约设备、签到签退、上报故障，也可以用 AI 助手按一句话的需求找空位并预约；管理员可以处理故障、停用学生账号、查看使用统计。

系统由三部分组成：**网页端**和 **Windows 桌面端**是两个客户端，它们连接同一个**云端**（Supabase 数据库与 AI 云函数），共用同一套数据和业务规则。

```
        ┌──────────┐        ┌───────────────┐
        │  web/    │        │  desktop/     │
        │  网页端   │        │  桌面端（C11） │
        └────┬─────┘        └──────┬────────┘
             │        HTTPS        │
             └─────────┬───────────┘
                 ┌─────┴─────┐
                 │  cloud/   │  数据库：表、权限、业务函数
                 │  云端      │  AI 云函数：调用 DeepSeek
                 └───────────┘
```

## 主要功能

- 40 台设备，开放时间 08:00–17:00，按 1 小时一个时段预约；
- 一次可预约多台设备、多个时段，同一设备同一时段不能被重复预约；
- 在预约时段内签到、签退，系统记录实际使用时长；
- 学生上报设备故障，管理员完成维修、归档故障记录；
- 管理员停用学生账号、生成管理员邀请码、查看使用统计、导出完整记录；
- AI 预约助手：用自然语言描述需求，助手根据真实空位给出方案，用户确认后才会预约。

## 目录结构

| 目录 | 内容 |
|---|---|
| [`web/`](web/README.md) | 网页端：HTML + CSS + JavaScript（ES 模块），通过 GitHub Pages 发布 |
| [`desktop/`](desktop/README.md) | Windows 桌面端：C11 + Win32 + WinHTTP |
| [`cloud/`](cloud/README.md) | 云端：数据库脚本（SQL）与 AI 云函数（TypeScript） |
| `.github/workflows/` | 持续集成：发布网页、编译桌面端、运行测试 |

## 运行

- **网页端**：直接访问 https://alexis-zhang0503.github.io/intelligent-lab-booking-system/ ；
  本地运行方式见 [web/README.md](web/README.md)。
- **桌面端**：从 GitHub Actions 的 *Build native C desktop* 下载编译好的 `LabReservation.exe`，
  或按 [desktop/README.md](desktop/README.md) 自行编译。

两个客户端都连接已部署好的云端，学生账号可在登录页直接注册。

## 持续集成

| 工作流 | 触发 | 作用 |
|---|---|---|
| Deploy GitHub Pages | 推送到 main | 用 `web/scripts/build_site.py` 构建 `web/` 并发布 |
| Build native C desktop | 推送到 main | 编译桌面端、运行 C 测试、检查程序能正常启动 |
| Test attendance | 每次推送 | 运行网页端测试，并在 PostgreSQL 中验证签到相关的数据库函数 |
