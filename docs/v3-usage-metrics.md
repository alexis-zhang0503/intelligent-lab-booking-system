# V3.0 设备分钟使用率

## 审计结论

源代码审计确认，旧 `v2_admin_metrics` 存在两个口径问题：

- `actual_check_in_usage_rate` 以有 `checked_in_at` 的预约条数计算，不能反映实际使用分钟数；
- 容量查询以当前 `devices.status` 过滤整个日期范围，不能证明历史日期当时的设备状态。

`reservation_usage_records.checked_in_at` 和 `checked_out_at` 都是 `timestamptz`。`v2-2-attendance-fix.sql` 已将签到/签退窗口解释为 `Asia/Shanghai`。V3 保留旧 API 的原有含义，新增独立的 `v3_admin_usage_metrics`，不会静默改变 V2 响应。

## 指标口径

- 有效预约分钟：每条 `reserved`、`confirmed`、`in_use`、`completed` 或 `no_show` 预约占 60 device-minutes；`cancelled` 不计入该分子，但计入取消率分母。
- 最终实际使用分钟：仅完整的签到/签退记录参与，`max(checked_in_at, slot_start)` 到 `min(checked_out_at, slot_end)` 的非负分钟数。`slot_start` 与 `slot_end` 统一由 `Asia/Shanghai` 转为绝对时刻。
- 暂估使用分钟：仅有签到、尚未签退时截至查询时刻的截断分钟数。它单独返回，绝不计入 `actual_usage_rate`。
- No-show：它表示已占用预约资源但未产生完成的使用记录，因此计入有效预约分钟和 no-show 率，不计入最终实际使用分钟。
- 手动签到：仅是系统记录的使用信号，不等同于物理设备验证。

`data_completeness` 返回已签到未签退、缺少有效签到、时间戳异常、完整使用记录、完整使用记录覆盖率。越界签到/签退会按预约时段截断；无交集、签退早于/等于签到等情况被标记为异常。

## 容量与历史限制

V3 以“设备 × 日期 × 开放小时”为容量单元：开放规则确定候选时段，任一覆盖该时段的设备维护或全实验室维护仅扣除一次。新增 `device_status_history` 与 `devices.status` 变更触发器，从迁移后的下一完整 `Asia/Shanghai` 自然日开始记录容量基线及后续状态变化。

迁移前设备状态没有历史事件，V3 不会用当前状态回推过去。若查询范围包含缺少设备状态历史的时段：

- `capacity_data_complete` 为 `false`；
- `capacity_device_minutes`、预约率、实际使用率和差异率为 `null`；
- 仍返回观察到的预约/使用分钟和明确的 `capacity_data_limitations`。

因此历史数据不足时，管理员 Agent 必须说明限制，不能判断实验室利用效率偏低。

## 迁移与回滚

1. 先在独立 Supabase staging 项目依次执行既有 schema、cloud、V2.0 和 V2.2 迁移。
2. 执行 `backend/supabase/v3-0-usage-metrics-migration.sql`。它只新增表、索引、触发器与函数，不删除或改写历史预约。
3. 部署 Web 客户端和 Edge Function。管理员页面会调用 V3 接口；未运行迁移的项目会得到明确的 RPC 缺失错误，而不会回退到语义不同的 V2 指标。
4. 仅在 staging 验证完毕且获得单独授权后，才可对生产项目执行 SQL 或部署。

如需停止 V3 读取，应先把前端/Edge Function 回退到已发布版本，再在维护窗口撤销 V3 函数执行权限并停用状态变更触发器。不要删除 `device_status_history`：它是迁移后形成的审计证据。

## 验证

本地可运行 `node --test tests/usage-metrics-contract.test.cjs` 验证设备分钟公式、边界截断、暂估隔离、维护去重、历史容量不足和 SQL 安全契约。完整 PostgreSQL/RLS 集成验证需要 disposable Supabase 或 PostgreSQL staging 环境；本工作区没有可用的 PostgreSQL、Docker 或 PGlite 运行时，因此未执行该类测试。
