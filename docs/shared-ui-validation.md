# 本交付验证记录

源代码基于 origin/main 的 ede7c7c，改动集中在新增 C 客户端、共享 UI 版本加载及发布流程。未操作线上数据库，未推送远程仓库。

已完成：

- 新宿主源代码按 C11 编译，使用官方 WebView2 SDK 的 C COM vtable 接口，无 C++ 源码或桥接依赖。
- 使用 Zig/Clang 交叉编译 Windows x64 GUI EXE，Wall/Wextra/Werror 检查通过。
- Windows PE 格式、系统 DLL 导入、图标资源、应用清单检查通过；图标由网页当前 favicon 的嵌入图像生成。
- 原生 C 站点导航策略测试通过。
- 50 项网页和签到回归测试通过。
- 5 项共享站点发布测试通过：版本清单、入口版本、完整本地资源、模块依赖版本、发布目录排除原生/数据库源码、错误版本和输出路径保护。
- 在隔离的 PGlite（PostgreSQL 兼容引擎）执行签到修复迁移和数据库测试，通过。
- GitHub 工作流 YAML 语法检查及 git diff --check 通过。

未完成：

- 尚未在真实 Windows 10/11 电脑运行这个 EXE，未实测 WebView2 窗口、下载文件、真实账号登录或签到。
- Windows MSVC 原生构建和窗口启动检查已写入 GitHub Actions，本次未触发线上运行。
- 未核实线上 Supabase 是否已执行签到迁移。客户端共享同一网页之后，数据库函数缺失或服务器校验错误仍需在云端处理。

复现命令：

```sh
node --test tests/*.test.cjs tests/attendance.test.mjs tests/time-range-and-live-use.test.mjs
python -m unittest discover -s tests -p test_shared_site_build.py
cc -std=c11 -Wall -Wextra -Werror desktop-c/tests/site_policy_test.c -o site-policy-check
./site-policy-check
python scripts/build_site.py --out _site --version local-shared-ui-2
```
