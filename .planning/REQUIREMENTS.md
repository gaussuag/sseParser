# Requirements: SSE Parser

**Defined:** 2025-03-17  
**Core Value:** Parse any valid SSE stream correctly regardless of network fragmentation

---

## v1 Requirements

### P0 - Critical

#### Error Handling (ERR)

- [ ] **ERR-01**: 使用错误码报告所有错误（无异常）
  - 实现 `SseError` 枚举类包含：success, incomplete_message, buffer_overflow, invalid_retry
  - 返回 `std::error_code` 或自定义结果类型

- [ ] **ERR-02**: 提供错误信息字符串转换
  - 实现 `error_message(SseError)` 返回可读描述

#### Message Types (MSG)

- [ ] **MSG-01**: 定义 `Message` 结构体包含所有标准SSE字段
  - `std::string event` （默认值为"message"）
  - `std::string data` 
  - `std::optional<std::string> id`
  - `std::optional<int> retry`

#### Buffer Management (BUF)

- [ ] **BUF-01**: 实现环形缓冲区存储内部数据
  - 模板参数配置大小（默认4096字节）
  - 满时拒绝写入并返回错误
  - FIFO顺序提供行读取

#### Protocol Parsing (PAR)

- [ ] **PAR-01**: 解析标准SSE格式 `field: value`
  - 识别字段名（event, data, id, retry）
  - 忽略未知字段（不报错）
  - 正确处理无冒号的情况（空值）

- [ ] **PAR-02**: 处理所有标准换行符样式
  - `\n\n` (LF) — Unix
  - `\r\n\r\n` (CRLF) — Windows
  - `\r\r` (CR) — Classic Mac
  - 支持混合样式

- [ ] **PAR-03**: 正确处理值前导空格
  - 冒号后的第一个空格被移除
  - 后续空格保留

- [ ] **PAR-04**: 跳过注释行
  - 以 `:` 开头的行视为注释
  - 注释行触发空消息（可选）

#### Data Handling (DAT)

- [ ] **DAT-01**: 累积多行 data 字段
  - 多行数据用 `\n` 连接
  - 不在末尾添加额外换行

- [ ] **DAT-02**: 处理空消息
  - 单独的 `\n\n` 产生空 data 的消息
  - 仍需触发回调

#### Public API (API)

- [ ] **API-01**: 提供 `parse(const char*, size_t)` 接口
  - 接受原始字节块
  - 返回错误码
  - 内部缓冲不完整的行

- [ ] **API-02**: 提供 `parse(std::string_view)` 接口
  - C++17 string_view 支持
  - 零拷贝输入

- [ ] **API-03**: 提供回调接口传递完整消息
  - 使用 `std::function<void(const Message&)>` 或函数指针
  - 消息完整时同步调用

- [ ] **API-04**: 提供 `flush()` 方法
  - 处理缓冲区中不完整的最后消息
  - 返回错误码

- [ ] **API-05**: 提供 `reset()` 方法
  - 清空所有内部状态
  - 用于连接重连场景

#### Performance (PER)

- [ ] **PER-01**: 热路径零动态分配
  - 仅使用栈和预分配缓冲区
  - 不调用 malloc/new

- [ ] **PER-02**: 支持编译器优化
  - 允许内联关键路径
  - 避免虚函数

---

### P1 - High Priority

#### Field Validation (VAL)

- [ ] **VAL-01**: 验证 retry 字段为有效整数
  - 检测溢出
  - 拒绝非数字值
  - 错误码：invalid_retry

#### Robustness (ROB)

- [ ] **ROB-01**: 处理缓冲区满的情况
  - 返回 buffer_overflow 错误
  - 不丢失已解析的完整消息

#### Interface (IFC)

- [ ] **IFC-01**: 支持函数指针回调（替代 std::function）
  - 零开销抽象选项
  - 嵌入式友好

---

### P2 - Medium Priority

#### Protocol Extras (EXT)

- [ ] **EXT-01**: 检测并跳过 UTF-8 BOM
  - 仅处理流开始时的 BOM
  - `\xEF\xBB\xBF`

- [ ] **EXT-02**: 跟踪 Last-Event-ID
  - 自动更新最后收到的 id
  - 提供 accessor 方法

---

## v2 Requirements

延后到未来版本，不在v1路线图中：

#### Advanced Features (ADV)

- **ADV-01**: 流式数据输出（不累积完整data）
- **ADV-02**: 配置最大消息大小限制
- **ADV-03**: 统计信息（消息计数、字节数）
- **ADV-04**: 可选的线程安全包装器

#### Protocol Extensions (PRO)

- **PRO-01**: 支持自定义字段处理器
- **PRO-02**: 服务器端事件生成支持

---

## Out of Scope

| 功能 | 原因 |
|------|------|
| HTTP 客户端实现 | 职责分离：仅解析协议 |
| JSON 解析 | 应用层处理 data 内容 |
| 压缩/解压缩 (gzip等) | 网络层处理编码 |
| WebSocket 支持 | 不同协议 |
| 分块传输编码 | HTTP 层处理 |
| 服务器端 SSE 生成 | 仅客户端解析 |
| 自动重连逻辑 | 网络层职责 |
| 线程安全保证 | 用户处理同步 |

---

## Traceability

| 需求ID | 阶段 | 状态 |
|--------|------|------|
| ERR-01 | Phase 1 | Pending |
| ERR-02 | Phase 1 | Pending |
| MSG-01 | Phase 1 | Pending |
| BUF-01 | Phase 1 | Pending |
| PAR-01 | Phase 2 | Pending |
| PAR-02 | Phase 2 | Pending |
| PAR-03 | Phase 2 | Pending |
| PAR-04 | Phase 2 | Pending |
| DAT-01 | Phase 3 | Pending |
| DAT-02 | Phase 3 | Pending |
| API-01 | Phase 4 | Pending |
| API-02 | Phase 4 | Pending |
| API-03 | Phase 4 | Pending |
| API-04 | Phase 4 | Pending |
| API-05 | Phase 4 | Pending |
| PER-01 | Phase 1-4 | Pending |
| PER-02 | Phase 1-4 | Pending |
| VAL-01 | Phase 2 | Pending |
| ROB-01 | Phase 1 | Pending |
| IFC-01 | Phase 4 | Pending |
| EXT-01 | Phase 2 | Pending |
| EXT-02 | Phase 3 | Pending |

**覆盖率:**
- P0 需求: 14 个（全部映射）
- P1 需求: 3 个（全部映射）
- P2 需求: 2 个（全部映射）
- 未映射: 0 ✓

---

*需求定义: 2025-03-17*  
*最后更新: 2025-03-17 初始定义*
