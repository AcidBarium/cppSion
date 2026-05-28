# cppSion — 可配置的随机 C++ 程序生成器

cppSion 致力于生成可编译、可运行且可控的随机 C++ 程序，用于编译器测试、性能基准和程序行为研究。目标类似 Csmith，但更强调可配置性和可拓展性。

## 核心特性
- **可复现随机性**：显式 seed 控制，跨平台一致的 Mersenne Twister 随机源。
- **配置驱动**：命令行参数可调节行数/函数数、复杂度预算、六种特征权重（计算/分支/内存/IO/递归/模板）。
- **语句种类丰富**：变量声明/赋值、if-else、while 循环、switch-case、复合赋值、IO（`cout` / `fin >>`）、函数调用、三元表达式、指针 `new`/`delete`、模板函数。
- **表达式丰富**：算术（`+-*/%`）、位运算（`&|^~`）、逻辑比较（`&&||<==`）、一元（`-~`）、后置 `++`/`--`。
- **语义保障**：符号/类型系统确保变量和函数引用均合法，作用域管理正确。
- **UB 防护**：有界循环、除数非零保证、安全移位操作、禁止对指针自增自减、模板参数避免 `bool` 推导。
- **IO 模式**：从 `input.txt` 文件读取输入，自动生成配套输入文件。
- **多重输出**：C++17 源码 + 可选 AST JSON，附带生成统计信息。

## 目录结构
- `src/main.cpp`：CLI 入口，驱动配置解析、生成、输出、输入文件写入。
- `src/util/random.*`：可播种随机工具，支持权重选择、布尔分布。
- `src/config/`：配置结构与命令行解析（含权重范围校验）。
- `src/semantic/`：类型系统、符号表、作用域管理。
- `src/ast/`：AST 定义与 C++17 pretty-printer，AST JSON 简易导出。
- `src/generator/`：生成上下文（预算/统计）、完整表达式/语句/函数/类型生成管线。
- `CMakeLists.txt`：构建配置，C++17，开启常用警告。

## 快速开始
1. 配置并构建：
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   cmake --build .
   ```
2. 运行生成器：
   ```bash
   ./build/cppsion --seed 42 --lines 200 --functions 8 --compute-weight 0.7 --out generated.cpp
   ```
3. 编译并运行生成的代码：
   ```bash
   g++ -std=c++17 generated.cpp -o generated
   # 若使用了 IO 模式，确保 input.txt 或 companion _input.txt 存在
   ./generated
   ```
4. 输出说明：
   - `generated.cpp`：生成的 C++17 源码。
   - `generated_input.txt`：IO 模式配套输入文件（自动生成）。
   - stderr 上打印 `cppSion stats`（函数数、语句数、分支/循环/表达式数等）。

## 命令行参数
- `--seed <u64>`：设置 RNG 种子（0 表示使用硬件随机源）。
- `--out <path>`：输出文件路径；缺省为 stdout。
- `--emit-ast`：额外输出 AST JSON。
- `--lines <int>`：目标行数（默认 100）。
- `--functions <int>`：函数数量（默认 4，实际生成 `N` 个 helper + `main`）。
- `--complexity <int>`：语句预算上限（默认 500）。
- `--max-depth <int>`：最大嵌套深度（默认 5）。
- `--max-expr <int>`：表达式节点预算上限（默认 200）。
- `--compute-weight <double>`：计算密集路径权重 `[0, 1]`（默认 0.5）。
- `--branch-weight <double>`：分支密集路径权重 `[0, 1]`（默认 0.3）。
- `--memory-weight <double>`：内存密集路径权重 `[0, 1]`（默认 0.2）。
- `--io-weight <double>`：IO 密集路径权重 `[0, 1]`（默认 0.1）。
- `--recursion-weight <double>`：递归密集路径权重 `[0, 1]`（默认 0.1）。
- `--template-weight <double>`：模板使用权重 `[0, 1]`（默认 0.1）。
- `-h/--help`：打印帮助。

## 示例
```bash
# 生成 10 个函数、300 行、计算密集的程序
./build/cppsion --seed 123 --lines 300 --functions 10 \
    --compute-weight 0.7 --out sample.cpp

# IO 密集模式：生成读取 input.txt 的程序
./build/cppsion --seed 0 --lines 50 --functions 4 \
    --io-weight 1.0 --out io_sample.cpp
# 此时会自动生成 io_sample_input.txt
```

## 实现状态
- ✅ 基础 RNG、配置解析（含权重校验）、AST 定义与打印、符号表/作用域/类型系统
- ✅ 六种特征权重：compute / branch / memory / IO / recursion / template
- ✅ 语句：变量声明（含指针 `new`）、赋值（含解引用）、if-else、while、switch-case、复合赋值、IO（`cout`/`fin`）、return
- ✅ 表达式：字面量（int/double/bool/string）、变量引用、二元运算、一元运算、三元、函数调用、`new`/`delete`
- ✅ 模板函数（`template<typename T>`）
- ✅ 函数间调用（main 自动调用所有 helper）
- ✅ UB 防护：安全除数、无移位溢出、无指针自增自减、模板避免 `bool` 推导、有界循环、预算控制
- ✅ IO 模式：文件输入 + 自动生成配套输入文件
- 📌 规划中：结构体/类支持、多线程可选特性、差分编译器测试管线、CI

## 许可证
- 见 [LICENSE](LICENSE)。
