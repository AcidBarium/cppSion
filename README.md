# cppSion — 可配置的随机 C++ 程序生成器

cppSion 致力于生成可编译、可运行且可控的随机 C++ 程序，用于编译器测试、性能基准和程序行为研究。目标类似 Csmith，但更强调可配置性和可拓展性。

## 核心特性
- **可复现随机性**：显式 seed 控制，跨平台一致的 Mersenne Twister 随机源。
- **配置驱动**：命令行参数可调节行数/函数数、复杂度预算、特征权重（计算/分支/内存/IO/递归/模板）。
- **语义保障**：基础符号/类型系统确保生成的变量与类型已定义且匹配（持续扩展中）。
- **AST 与安全性**：分层 AST（Program/Function/Stmt/Expr/Type）与初步 UB 防护（有界循环、包装算术、已初始化变量）。
- **多重输出**：默认输出 C++17 源码，可选导出 AST JSON，附带生成统计信息。

## 目录结构
- `src/main.cpp`：CLI 入口，驱动配置解析、生成、输出。
- `src/util/random.*`：可播种随机工具，支持权重选择、布尔分布。
- `src/config/`：配置结构与命令行解析。
- `src/semantic/`：类型系统、符号表、作用域管理（持续扩展）。
- `src/ast/`：AST 定义与 C++17 pretty-printer，AST JSON 简易导出。
- `src/generator/`：生成上下文、统计与示例 ProgramGenerator（待扩展为完整 stmt/expr/function/type 生成管线）。
- `CMakeLists.txt`：构建配置，C++17，开启常用警告。

## 快速开始
1. 配置并构建（Windows 示例，使用 CMake ≥ 3.16）：
   ```pwsh
   mkdir build
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   cmake --build . --config Release
   ```
2. 运行生成器：
   ```pwsh
   ./cppsion --seed 42 --lines 200 --functions 8 --complexity 500 --compute-weight 0.7 --out generated.cpp --emit-ast
   ```
3. 输出说明：
   - `generated.cpp`：生成的 C++17 源码（若未指定 `--out` 则写到 stdout）。
   - `generated.cpp.json`：AST JSON（仅在 `--emit-ast` 时生成；未指定 out 时写到 stdout）。
   - stderr 上会打印 `cppSion stats`，包含函数数、语句数、分支/循环计数、表达式数、最大深度、内存操作计数等。

## 命令行参数
- `--seed <u64>`：设置 RNG 种子（0 默认用硬件随机源）。
- `--out <path>`：输出文件路径；缺省为 stdout。
- `--emit-ast`：额外输出 AST JSON（文件或 stdout）。
- `--lines <int>`：目标行数（提示生成规模）。
- `--functions <int>`：最大函数数量。
- `--complexity <int>`：语句预算上限。
- `--max-depth <int>`：最大嵌套深度。
- `--max-expr <int>`：表达式节点预算上限。
- `--compute-weight <double>`：计算密集权重。
- `--branch-weight <double>`：分支密集权重。
- `--memory-weight <double>`：内存密集权重。
- `--io-weight <double>`：IO 密集权重。
- `--recursion-weight <double>`：递归密集权重。
- `--template-weight <double>`：模板使用权重。
- `-h/--help`：打印帮助。

## 示例
```pwsh
./cppsion --seed 123 --lines 300 --functions 10 --complexity 800 \
          --compute-weight 0.5 --branch-weight 0.2 --memory-weight 0.1 \
          --io-weight 0.1 --recursion-weight 0.05 --template-weight 0.05 \
          --out sample.cpp --emit-ast
```

## 当前状态与计划
- ✅ 基础 RNG、配置解析、AST/打印、符号表雏形、生成统计。
- 🔄 正在扩展：语义/类型检查、语句/表达式/函数/类型生成器的细分模块，更多 UB 规避（安全移位、索引、有界递归/循环），复杂度预算全链路落实。
- 📌 规划中：更丰富的容器/结构体/模板支持，多线程可选特性，差分编译器测试管线，CI 编译烟囱测试，AST JSON schema 稳定化与可视化工具。

## 许可证
- 见 [LICENSE](LICENSE)。
