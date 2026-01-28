1.  **`nlohmann::json` 的特性**：这是一个非常易用但“重”的库。它的底层基于 `std::map` 或 `std::vector` 实现，并且大量使用堆内存（Heap Allocation）。做一个 Deep Copy (`json masked_ast = root;`) 会触发整棵树的递归复制，涉及大量的 `new` 操作。
2.  **高并发场景**：如果 QPS 是 10万，那么每秒就要做 10万次整树拷贝。这会给内存分配器（Allocator）带来巨大压力，导致 CPU 花在 `malloc/free` 上的时间比处理业务逻辑还多。
3.  **完全不必要**：仔细观察逻辑，`root` 变量在 `Analyze` 函数结束后就被销毁了。我们并不需要在函数外部保留一份“未被 Mask 的原始 JSON AST”。我们只需要：
    *   提取出的参数列表 (`params`)。
    *   Mask 后的指纹字符串 (`fingerprint`)。

### 优化方案：原地修改 (In-Place Mutation)

我们可以直接在 `root` 上进行遍历、提取参数，并顺手把它修改成 Mask 状态。这样就**完全消除了这次 Deep Copy**。

以下是修改后的 `parser.cpp` 代码逻辑：

#### 修改 `src/parser.cpp`

```cpp
bool SQLParser::Analyze(int req_id, const std::string& sql, ParsedQuery& out_result) {
    // 1. 调用 C 库解析 SQL
    PgQueryParseResult result = pg_query_parse(sql.c_str());

    if (result.error) {
        pg_query_free_parse_result(result);
        return false;
    }

    try {
        // 2. 第一次解析为 JSON (这是不可避免的开销，除非换 SAX 解析)
        // 注意：这里 move 语义通常对 json 构造不起作用，因为 parse 返回的是右值，编译器会有 RVO 优化
        json root = json::parse(result.parse_tree);
        pg_query_free_parse_result(result);

        out_result.request_id = req_id;
        out_result.original_sql = sql; // string copy, 必要
        out_result.arrive_time = std::chrono::high_resolution_clock::now();

        // 3. 【优化点】直接在 root 上操作，不再创建 masked_ast 副本
        // traverse 过程中，既提取参数到 out_result.params，又直接修改 root 节点为占位符
        TraverseAndMask(root, out_result.params);
        
        // 4. 生成结构指纹
        // 此时 root 已经被修改为 Masked 状态了，直接 dump 即可
        // 使用 -1 参数可以生成压缩的无空格字符串，减少哈希计算量
        out_result.fingerprint = root.dump(-1); 

        return true;
    } catch (const std::exception& e) {
        if (result.parse_tree) pg_query_free_parse_result(result);
        return false;
    }
}
```

### 进一步的极致优化建议 (针对 Paper 里的 "Optimization" 章节)

如果你在实验中发现 `Parser` 依然是瓶颈（通常 `libpg_query` + `json parse` 会占大头），可以考虑以下更底层的优化（作为 Future Work 或高级优化点）：

1.  **避免完整的 JSON Parse (SAX Parsing)**：
    *   `libpg_query` 返回的是 JSON 字符串。
    *   使用 `nlohmann::json::sax_parse` 或者更快的 `simdjson` 库。
    *   **思路**：实现一个 SAX 事件处理器。当遇到 `key == "A_Const"` 时，记录参数并写入固定字符串 `"?"` 到指纹流中；其他情况直接透传字符串。
    *   **收益**：可以完全跳过构建 JSON DOM 树的过程，实现 **Zero-Allocation**（除了输出字符串外）。

2.  **字符串指纹 vs 哈希指纹**：
    *   目前 `fingerprint` 是一个很长的字符串。
    *   在 Map 查找时，比较长字符串很慢。
    *   **优化**：在 `Analyze` 结束前，对 `root.dump()` 的结果直接计算 `XXHash` 或 `MurmurHash`，存一个 `uint64_t fingerprint_hash`。Map 的 Key 改用 `uint64_t`。

**当前阶段建议**：
先采纳上面的 **"原地修改 (In-Place)"** 方案。它只需要改动几行代码，就能消除最大的那个不必要的拷贝开销，对于验证 Prototype 性能已经足够高效了。如果后续 Profiling 显示瓶颈在 JSON 解析本身，再考虑 SAX 解析。