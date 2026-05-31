# BrickGame — 球打砖块

项目简介
- 这是一个基于 raylib 的单机/网络砖块打击游戏示例，包含本地玩法、网络演示与性能采集（Tracy）。

主要功能
- 本地单人游戏（键盘控制）
- 网络对战/演示（使用 ENet）
- 关卡加载（levels/levels.json）
- 性能分析集成（Tracy，可选）

依赖与环境
- Windows + MinGW-w64（g++ 支持 C++17）
- raylib 5.5（win64）— 请安装并记录 include/lib 路径
- ENet（用于网络演示）
- 可选：Tracy 用于性能采样

快速构建（在项目根目录执行）
- 使用 VS Code 任务（已配置）：选择 `Build BrickGame (raylib)` 或 `Build BrickGame Network`。
- 也可在 PowerShell 中手动运行（示例，需按本地路径调整 include/lib）：

```powershell
g++ -std=c++17 -g -Wall -Wextra -static-libgcc -static-libstdc++ -IC:/raylib-5.5/raylib-5.5_win64_mingw-w64/include \
    -I"C:/Users/wanghan/安装的软件/enet-1.3.18/enet-1.3.18/include" \
    BrickGame/main.cpp BrickGame/Ball.cpp BrickGame/Paddle.cpp BrickGame/Brick.cpp \
    -LC:/raylib-5.5/raylib-5.5_win64_mingw-w64/lib -L"C:/Users/wanghan/安装的软件/enet-1.3.18/enet-1.3.18" \
    -l:enet64.lib -lraylib -lopengl32 -lgdi32 -lwinmm -lws2_32 -o BrickGame/build/BrickGame.exe
```

运行
- 运行可执行文件：`BrickGame/build/BrickGame.exe`
-- 网络功能：使用 `Build BrickGame Network` 任务编译（需安装 ENet），主机使用 `--host` 参数启动，客户端使用 `--client <server_ip>` 连接。

项目结构（重点）
- BrickGame/: 源代码与资源
  - main.cpp / main_network.cpp
  - Ball.cpp/.h, Paddle.cpp/.h, Brick.cpp/.h 等游戏对象
  - tracy/: Tracy profiler 源码（可选）
- build/: 编译输出
- levels/: 关卡数据（levels.json）

注释规范建议
- 在关键类/函数处补充注释，说明：是什么（作用）、为什么（设计意图）、怎么用（接口/参数）。例如在 `Ball.h`、`Paddle.h`、`Brick.h`、`NetworkManager.h` 开始添加说明。

整理建议
- 删除或移动临时/调试用文件到单独目录，移除无用依赖与冗余脚本。

故障排查
- 若遇到链接错误，确认 raylib/ENet 的 lib 路径与库名是否正确。
- 若运行缺少 DLL，请确保 MinGW 运行时与 raylib 依赖一并可用或使用静态链接选项。

下一步
- 如果你同意，我将按计划为关键源码文件添加规范注释（从 `Ball.*`, `Paddle.*`, `Brick.*`, `NetworkManager.*` 开始）。

作者/许可
- 本仓库为练习与示例用途。请根据需要添加许可声明。
