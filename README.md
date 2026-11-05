# 智能实验室预约系统（Web）

面向学生与管理员的实验室预约、设备管理和运营总览网页。主站由 GitHub Pages 部署，数据库与权限由 Supabase 提供。

## 功能

- 学生预约、取消预约、签到/签退与故障上报；
- 管理员查看运营总览、维护设备与故障记录、管理用户；
- 智能助手提供可用设备与时段建议，并在预约成功后给出确认反馈；
- 管理员导出完整预约与故障处理记录；
- 中英文界面切换、响应式布局和减少动态效果支持。

## 网页端结构

| 路径 | 作用 |
| --- | --- |
| `index.html` | GitHub Pages 入口，仅保留页面骨架与资源加载顺序。 |
| `web/app.js` | 单页应用的业务渲染、交互与语言切换。 |
| `web/core/` | Supabase、认证、预约时段和签到等业务规则。 |
| `web/ui/` | 独立的界面动效。 |
| `web/styles/` | 基础样式、界面层与助手样式，按 `index.html` 的顺序叠加。 |
| `web/assets/` | 网页独用的本地视觉资源。 |
| `design-system/tokens.css` | 全站共用设计 token。 |
| `backend/supabase/` | 网页使用的数据库 schema、迁移与 Edge Function。 |

根目录的 `favicon.svg` 仍保留为共享品牌资源；`desktop-c/` 与 `qt-client/` 为独立客户端，不属于网页端代码，整理网页时不应修改。

## 本地预览与检查

在仓库根目录启动静态服务：

```powershell
python -m http.server 4173
```

打开 <http://localhost:4173>。提交网页改动前可运行：

```powershell
node --test (rg --files tests -g "*.test.mjs" -g "*.test.cjs")
python -X utf8 -m unittest tests/test_shared_site_build.py
```

## 数据与部署

数据库初始化、管理员开通与 Supabase 安全约束见 [backend/supabase/README.md](backend/supabase/README.md) 和 [管理员开通说明](backend/supabase/ADMIN_ACCOUNT_RUNBOOK.md)。生产环境迁移、Edge Function 与 DeepSeek 密钥配置并不会随前端推送自动执行；发布前请核对 [部署步骤](docs/v2-deployment.md)。

推送 `main` 后，GitHub Actions 会运行 `scripts/build_site.py`，为本地网页资源写入同一版本号并发布到 GitHub Pages。构建产物不包含测试、后端源码或桌面客户端目录。

## 相关文档

- [网页架构](docs/v2-architecture.md)
- [网页 API 与数据契约](docs/v2-api.md)
- [智能助手架构](docs/agent-v3-architecture.md)
- [界面设计 token](docs/ui-design-tokens.md)
