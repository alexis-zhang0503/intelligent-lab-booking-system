# Implementation Plan: 智能实验室预约系统

## Overview

构建可部署到 GitHub Pages 的纯前端预约网站。系统将以浏览器本地存储保存演示数据，并让用户下载题目要求的 count.txt 与 error.txt，不伪装为可跨设备同步的后端服务。

## Architecture Decisions

- 使用无构建步骤的 HTML、CSS、JavaScript，便于课程验收和 GitHub Pages 部署。
- 管理员和学生均通过注册获得角色；登录状态、预约和故障记录存储在当前浏览器。
- 预约记录的唯一性由日期、设备编号与小时的组合保证。
- 采用深色、数据工作台式界面；桌面侧栏在窄屏转换为横向导航，40 台设备在手机上重排。

## Task List

### Phase 1: Foundation

- [ ] Task 1: 创建静态项目、GitHub Pages 工作流和使用说明。
- [ ] Task 2: 实现用户注册、登录、本地状态与角色导航。

### Checkpoint: Foundation

- [ ] JavaScript 语法检查通过。
- [ ] 学生和管理员均可注册、退出与重新登录。

### Phase 2: Core Features

- [ ] Task 3: 实现 40 设备、9 时段的多选预约与取消，并阻止时段冲突。
- [ ] Task 4: 实现学生故障上报、管理员设备维护和用户查询。
- [ ] Task 5: 实现周统计、累计时长、count.txt 和 error.txt 下载。

### Checkpoint: Core Features

- [ ] 两种角色关键流程均可完成。
- [ ] 导出的两个文本文件含实际界面数据。

### Phase 3: Responsive QA and Publish

- [ ] Task 6: 在 375、768、1024、1440 像素宽度检查布局、键盘焦点和触控操作。
- [ ] Task 7: 自动审查、修复问题、推送仓库并验证 GitHub Pages 部署。

### Checkpoint: Complete

- [ ] 无横向溢出；手机可使用座位图、预约与导航。
- [ ] GitHub Actions Pages 工作流成功。

## Risks and Mitigations

| Risk | Mitigation |
|---|---|
| 纯前端不支持真实多人共享 | README 和界面明确说明数据只在当前浏览器保存。 |
| GitHub Pages 无法直接写入文本文件 | 通过浏览器下载题目要求的两个文本文件。 |
| 40 格座位图在手机上过密 | 以两列座位组重排，保持按钮可触及和状态可辨。 |
