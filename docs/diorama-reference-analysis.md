# Diorama 参考分析与预约系统映射

## 已实际查阅的来源

- Codrops《Building Digital Products with Design Systems and Agents》：文章展示了浅色/深色 To-do 工作区、三栏任务/日历/助手、Today 卡片、日历事件、Agent 生成任务及组件文档。
- `bydiorama/ui` 公开仓库与 `CONVENTIONS.md`：确认其设计系统以 token 为先、采用有限变体和明确状态；规范要求语义 token、显式动画属性、同心圆角与稳定的部件定位。
- Paper 链接可打开但未返回可读画布内容。因此本次没有声称读取其画布，视觉结论只来自 Codrops 的真实截图和公开规范。

## 参考界面拆解

| Diorama 要素 | 观察 | 本系统的适配 |
|---|---|---|
| Three-column workspace | 左侧任务/资源入口，中间为工作内容，右侧为时间/Agent 上下文 | 桌面端为资源导航、预约/管理画布、预约或 Agent 上下文栏；窄屏折叠为单列 |
| Today / Calendar card | 低对比背景、可扫读标题、事件状态先于装饰 | 设备平面图、时段选择、故障/占用/已选状态均由颜色、文字及 disabled 状态共同表达 |
| Agent chat | 聊天作为正式工作区，有上下文和建议结果，而非浮层 | AI 页面改为对话面 + 真实预约/运营上下文栏；原有 Edge Function、确认步骤不变 |
| Light / dark themes | 同一层级与几何，颜色由 token 切换 | `web/styles/diorama-ui.css` 提供 light 与系统 dark token；不在业务组件散落主题颜色 |
| Component documentation | 把状态、token 与可访问性一起定义 | 设计文档记录状态、几何和响应式规则；原有 DOM ID/业务事件保持 |

## Typography、颜色和层级

保留项目既有中英文字体与品牌标题字体；本次不替换字族。页面标题是导航，不是营销口号；只通过字号、权重和层级控制信息密度。背景、表面、升起表面、悬停表面形成四级低对比层级；强调色仅用于主要提交、焦点与已选状态。故障、维护和成功只使用语义状态色。

## Calendar / AI 交互原则

设备平面图相当于资源日历：可预约、已占用、维护、已选四种状态始终可辨。候选方案来自现有实时 RPC；展示为排名卡，不显示内部评分。AI 建议不会等于预约成功，仍沿用现有确认和服务端冲突复核。管理员 AI 仅保留只读分析边界。

## 不直接照搬的部分

不会复制 Diorama 的 React 组件、图标或 Paper 资产；本项目是 Vanilla JS，故只采用公开设计原则和布局关系。课程项目的设备选择、手动签到签退、故障闭环、权限与 Supabase/RLS 都是实验室业务特有内容，不能被 To-do 交互替代。
