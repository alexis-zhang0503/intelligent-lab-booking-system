# 云端

基于 Supabase（PostgreSQL 数据库 + 用户认证 + Edge Functions），网页端与桌面端共用。

## 目录结构

```
cloud/
├── database/
│   ├── 01_schema.sql ~ 16_*.sql      建表与后续迁移脚本，按编号顺序执行
│   └── admin/                         开通管理员账号的脚本与操作说明
├── functions/
│   └── deepseek-booking-agent/        AI 预约助手云函数（TypeScript / Deno）
│       ├── index.ts                       入口：校验身份 → 读取真实数据 → 调用 DeepSeek → 返回方案
│       ├── booking-agent-tools.mjs        提供给模型调用的工具：查询空位、请用户确认、上报故障
│       ├── booking-request.mjs            解析模型给出的查询参数
│       ├── explicit-booking-request.mjs   识别用户直接说明的预约需求
│       ├── conversation-history.mjs       规范化多轮对话历史
│       └── admin-agent-actions.mjs        管理员助手可准备的操作
└── tests/                             数据库函数测试（SQL 夹具与 Node 脚本）
```

## 数据库

| 编号 | 内容 |
|---|---|
| 01 | 核心表：用户资料、设备、预约、故障；注册时自动建立学生资料的触发器 |
| 02 – 05 | 学生取消预约与账号停用、管理员邀请码、资料修改、故障处理跟踪 |
| 06 – 11 | v2：预约状态、签到签退记录、幂等预约、开放规则、故障响应与修复 |
| 12 – 16 | v3：使用统计、AI 助手会话与方案记录、隐私加固 |

所有写操作都通过数据库函数完成（例如预约 `v2_create_reservations`、签到 `v2_check_in`）。冲突检查、幂等处理和权限判断都在数据库内完成，客户端无法绕过；行级安全策略（RLS）保证用户只能读到自己有权查看的数据。

新建环境时，在 Supabase SQL Editor 中按编号依次执行 `database/` 下的脚本，再在 **Authentication → Providers → Email** 关闭 **Confirm email**（学生使用学号登录，客户端将其映射为内部邮箱）。管理员开通见 [database/admin/README.md](database/admin/README.md)。

## AI 云函数

DeepSeek 密钥只保存在云函数的环境变量 `DEEPSEEK_API_KEY` 中，不会下发到任何客户端。模型只能通过工具读取数据库中的真实空位并生成候选方案；用户确认方案后，才由数据库函数写入预约。云函数同时兼容网页端的 v2 请求格式和桌面端的 v3 请求格式。
