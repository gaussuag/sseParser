# Roadmap: SSE Parser

**Project:** C++17 Header-Only SSE Parser Library  
**Version:** 1.0.0  
**Created:** 2025-03-17

---

## Overview

| Metric | Value |
|--------|-------|
| Total Phases | 5 |
| v1 Requirements | 19 (14 P0, 3 P1, 2 P2) |
| Estimated Duration | 2-3 weeks |

---

## Phases

### Phase 1: Foundation

**Goal:** Core data structures and error handling

**Requirements:**
- ERR-01: 错误码系统（无异常）
- ERR-02: 错误信息字符串
- MSG-01: Message 结构体定义
- BUF-01: 环形缓冲区实现
- ROB-01: 缓冲区溢出处理
- PER-01/02: 零分配设计

**Success Criteria:**
1. RingBuffer 单测通过（边界条件、满/空状态）
2. Message 结构体可编译，内存布局合理
3. 错误码可正确返回和转换
4. 代码通过编译器 C++17 检查

**测试交付:**
- `tests/test_ring_buffer.cpp` - 环形缓冲区单元测试（15+测试用例）
- `tests/test_error_codes.cpp` - 错误码系统测试
- `tests/test_message.cpp` - Message结构体验证
- 目标：90%行覆盖率

**Estimated Plans:** 3

---

### Phase 2: Core Parsing

**Goal:** Parse SSE fields and handle line endings

**Requirements:**
- PAR-01: 字段解析（field: value）
- PAR-02: 所有换行符样式（LF/CRLF/CR）
- PAR-03: 前导空格处理
- PAR-04: 注释行跳过
- VAL-01: retry 字段验证
- EXT-01: UTF-8 BOM 检测（P2）

**Success Criteria:**
1. 可正确解析单字段消息
2. 通过换行符样式测试矩阵（9种组合）
3. 正确处理前导空格（spec示例）
4. 注释行被跳过
5. 无效 retry 返回错误码

**测试交付:**
- `tests/test_field_parser.cpp` - 字段解析测试（12+用例）
- `tests/test_line_endings.cpp` - 换行符样式矩阵测试（9种组合）
- 包含Spec合规性验证测试
- 目标：85%行覆盖率

**Estimated Plans:** 4

---

### Phase 3: Message Assembly

**Goal:** Build complete messages and handle multi-line data

**Requirements:**
- DAT-01: 多行 data 累积
- DAT-02: 空消息处理
- EXT-02: Last-Event-ID 跟踪（P2）

**Success Criteria:**
1. 多行 data 用 `\n` 正确连接
2. 空消息触发回调（data为空字符串）
3. 消息边界正确检测
4. ID 字段正确跟踪

**测试交付:**
- `tests/test_message_builder.cpp` - 消息组装测试（8+用例）
- 包含多行数据累积验证
- 目标：90%行覆盖率

**Estimated Plans:** 3

---

### Phase 4: Public API

**Goal:** Facade class and callback integration

**Requirements:**
- API-01: parse(const char*, size_t)
- API-02: parse(string_view)
- API-03: 回调接口
- API-04: flush() 方法
- API-05: reset() 方法
- IFC-01: 函数指针回调支持（P1）

**Success Criteria:**
1. 可正确处理分块输入（模拟网络分包）
2. 回调在消息完整时触发
3. flush() 可处理不完整末尾消息
4. reset() 正确清空状态
5. string_view 接口零拷贝

**测试交付:**
- `tests/test_sse_parser.cpp` - 完整解析器API测试（12+用例）
- 分块输入模拟测试
- 回调类型测试（std::function + 函数指针）
- 目标：85%行覆盖率

**Estimated Plans:** 4

---

### Phase 5: Polish

**Goal:** Edge cases, tests, and documentation

**Requirements:**
- 所有 P0/P1/P2 需求验收
- 错误恢复行为
- 性能基准测试

**Success Criteria:**
1. 100% P0 需求测试覆盖
2. 通过 fuzzing 测试（随机输入不崩溃）
3. 性能达到 >100MB/s 解析速度
4. 完整 API 文档（使用示例）
5. 嵌入式编译器验证（GCC/Clang/MSVC）

**测试交付:**
- `tests/test_integration.cpp` - 集成测试（LLM场景模拟）
- `tests/test_fuzzing.cpp` - 模糊测试（1000+随机输入）
- `tests/test_performance.cpp` - 性能基准测试（>100MB/s目标）
- `tests/test_edge_cases.cpp` - 边界条件测试
- 需求追溯矩阵验证
- 完整测试文档

**Estimated Plans:** 3

---

## Phase Summary

| # | Phase | Goal | Requirements | Plans | Status |
|---|---|-------|------|--------------|-------|--------|
| 1 | Foundation | Core types & buffer | 6 | 10 | ◆ In Progress (5/10 complete) |
| 2 | Core Parsing | Field & line parsing | 6 | 4 | ○ Pending |
| 3 | Message Assembly | Multi-line handling | 3 | 3 | ○ Pending |
| 4 | Public API | Facade & callbacks | 6 | 4 | ○ Pending |
| 5 | Polish | Tests & docs | - | 3 | ○ Pending |

**Total:** 5 phases | 19 requirements | 24 plans

---

## Dependency Graph

```
Phase 1 (Foundation)
    ↓
Phase 2 (Core Parsing) → requires RingBuffer, Error codes
    ↓
Phase 3 (Message Assembly) → requires State machine
    ↓
Phase 4 (Public API) → requires all above
    ↓
Phase 5 (Polish) → requires working implementation
```

Phases are strictly sequential due to dependencies.

---

## Risk Mitigation

| Risk | Phase | Mitigation |
|------|-------|------------|
| 换行符处理复杂 | 2 | 优先完成，充分测试 |
| 缓冲区溢出 | 1 | 早期设计，边界测试 |
| 回调异常 | 4 | 文档约定，或捕获转换 |
| 嵌入式编译失败 | 5 | 多编译器CI验证 |

---

## Notes

- **Execution mode:** YOLO (auto-approve plans)
- **Granularity:** Standard (balanced phase size)
- **Parallelization:** Sequential (phases have dependencies)
- **Research:** Completed — see `.planning/research/`
- **Testing:** Google Test (gtest) — see `.planning/TESTING.md`
  - Phase 1-4: 单元测试 + 覆盖率目标
  - Phase 5: 集成测试 + Fuzzing + 性能测试
  - 总体目标: 100% P0需求覆盖

---

*Roadmap created: 2025-03-17*  
*Last updated: 2026-03-17 - Plan 01-05 complete: Main header with version info and unified namespace (53 tests passing)*
