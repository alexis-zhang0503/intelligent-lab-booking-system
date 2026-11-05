# Diorama 风格 Design System

## Semantic tokens

`web/styles/diorama-ui.css` 定义以下 token，并为系统深色偏好提供等价值：

- 表面：`--color-background`、`--color-surface`、`--color-surface-raised`、`--color-surface-hover`
- 结构：`--color-border-subtle`、`--color-border-strong`
- 文字：`--color-text-primary`、`--color-text-secondary`、`--color-text-tertiary`
- 状态：`--color-accent`、`--color-success`、`--color-warning`、`--color-danger` 及对应 soft token
- 几何：4px 基础间距、8/12/16/20/24/32 的尺度，以及 8/12/16/20 圆角

现有 `--ink`、`--line`、`--blue` 等变量被映射到语义 token，确保旧组件仍然可用。

## 字体保留

本次不替换项目既有的中英文字体、品牌标题字体或数据编号字体；`web/styles/diorama-font-preservation.css` 只撤销本轮工作区样式可能造成的字体覆盖。Diorama 的借鉴范围限于布局、间距、层级和组件状态。

## Primitives 与状态

| Primitive | Default | Hover / focus | Selected / error / disabled |
|---|---|---|---|
| Button | 38px、高度一致、8px 圆角 | 边框或背景轻微变化；焦点使用 accent 外环 | 主按钮禁用降低不透明度；危险操作复用 danger token |
| Input / Select / Textarea | surface + subtle border | border 强化；focus 使用 3px accent soft ring | 原有 `aria-invalid` 继续触发错误样式 |
| Panel / Card | surface、16px 圆角、无夸张阴影 | border 加强 | 状态仅由内容语义色呈现，不增加装饰条 |
| Device tile | resource 状态与编号同屏 | 可预约设备显示可选反馈 | selected、occupied、faulty、disabled 有独立且可读状态 |
| Badge | 小型语义背景 | 无必要动效 | 成功/故障 token 对应预约和维修状态 |
| Agent recommendation | 排名、设备、时段与采用操作 | border 强化 | 采用后仍走既有确认/服务端验证 |

禁止使用 `transition: all`、随机渐变、粗左边框或无意义虚线。所有动效只作用于背景、边框、阴影或 transform，并遵守 `prefers-reduced-motion`。

## 响应式

- ≥1180px：完整资源导航 + 工作画布 + 右侧上下文。
- 941–1180px：较窄导航和 270px 上下文。
- 641–940px：顶部可横向滚动导航；预约、设备、AI 分为单列。
- ≤640px：两列指标、单列时段/上下文，表格保持横向可滚动。
