# 签到修复与上传说明

本次基于 main 的 bdc84cebd3f2938f007df58f92722a4dcbce9dc8 修改。未推送 GitHub，也未更改线上 Supabase 数据库。

## 问题与修复

1. 原网页对当天所有已确认预约都显示签到按钮，包括尚未开始和已经结束的时段。现在按北京时间只在预约开始（含）至结束（不含）显示签到按钮，使用中显示签退，并每 15 秒更新按钮状态。
2. 原后端把不含时区的预约时间与 now() 比较，结果依赖数据库会话时区。新增增量 SQL 固定将预约时间解释为 Asia/Shanghai；原历史迁移不变。
3. 签到和签退增加防重复提交、异常捕获及明确错误提示。签到写入成功后，即使刷新失败也显示“已保存”，避免误以为写入失败。
4. 桌面端原来只有本地预约，没有签到实现。本次增加本地签到、签退、状态显示、时间戳持久化、幂等校验及账号归属检查；旧 JSON 中没有时间戳的记录可继续读取。桌面端时间统一为北京时间。已经签到的预约不能取消。

尚未获得原报错文本和真实账号，因此无法确认线上报错具体属于“预约未到时间”“时区不一致”或“云端缺少迁移”等哪一种。本次修复了代码中已确认的问题，并为缺少迁移的情况提供单独提示。

## 上传源码

将压缩包中 `intelligent-lab-booking-system/` 目录内的文件复制到原仓库根目录（包括 `.github/`），保留既有文件，提交并推送 main。GitHub Actions 将发布网页，并运行新增测试。

不要将压缩包本身或 `git-history/` 目录上传到网页仓库。Git 历史包用于本地恢复仓库。

## 必须执行的云端步骤

如果已经完整执行过 V2 迁移，在 Supabase SQL Editor 执行：

`backend/supabase/v2-2-attendance-fix.sql`

它替换签到和签退函数，不删除预约或使用记录，并通知 API 刷新函数缓存。

如果提示 `reservation_usage_records`、`reservation_status_events` 或 V2 函数不存在，说明原 V2 迁移未完成。先依照 `docs/v2-deployment.md` 核对原数据库并完成 V2 迁移，再执行本次 SQL。不要在已有生产数据上盲目重复运行初始 schema.sql。

只上传 HTML/JS 不能修复云端 SQL 函数。此次没有在真实云端执行 SQL。

## 桌面端构建与数据范围

Qt 桌面端仍然是原项目的本地演示客户端，本次没有将它改造成云端客户端。它的账号、预约和签到不会自动与网页互通。若你的实际桌面程序使用的不是仓库内 `qt-client/` 源码，需要用实际源码再次核对。

在已安装 Qt 6.4+、CMake 3.21+ 和 C++17 编译器的终端运行：

```sh
cmake -S qt-client -B build -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Windows 发行版还需用对应 Qt 安装中的 windeployqt 部署依赖。本交付包含源码，不包含已经编译的 Windows EXE。

## 验证结果

- 网页 7 项 Node 回归测试通过；另在 America/New_York 时区运行，7 项同样通过。
- 主脚本 ES 模块语法检查和 git diff --check 通过。
- 使用 PGlite（PostgreSQL 兼容引擎）执行测试专用空数据库结构、实际增量 SQL 和数据库测试脚本，通过。覆盖过早/过期签到、未签到签退、其他账号/匿名操作、UTC 会话中的北京时间签到、重复签到/签退及事件去重。
- Qt 回归测试已增加时间边界、持久化和账号归属检查；当前环境缺少 Qt/CMake，未执行桌面端编译和测试。新增 GitHub Actions 包含 Qt 构建、CTest 及独立 PostgreSQL 16 服务测试，上传后应确认全部通过。
- 未使用真实用户账号进行端到端签到，也未部署此版本。

测试：`node --test tests/attendance.test.mjs`。`tests/attendance-fixture.sql` 仅供空的测试数据库，不要在生产数据库运行。
