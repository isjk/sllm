# sLLM 概念设计代码骨架（v0.6 配套）

概念设计的最后一块拼图：§15 伪代码的可编译形态。**本骨架是契约，不是实现**——头文件冻结接口与决策语义，函数体由 vibe coding 任务卡逐模块填充。

## 文件 → 设计文档章节 → 任务卡锚点

| 文件 | 对应章节 | 内容 | 冻结级别 |
| --- | --- | --- | --- |
| `tracker/key.h` | §2 D1 / §4 内容寻址 | 键 = H(前缀‖模型‖引擎族‖量化)、算键唯一入口 | **签名冻结** |
| `node/profile.h` | §5 五维画像 / §7 作息 | NodeProfile 五维、EngineMount、ScheduleProfile、Role | **结构冻结** |
| `transport/itransport.h` | §6 双连接分治 | ITransport：bulk/stream 两心法、LinkStats 传感器、MyCC pacing 钩子、0-RTT 边界 | **接口冻结（宪法第 4 条实现体）** |
| `tracker/tracker.h` | §5 目录 / §7 副本 / §8.5 降级 | Tracker：心跳、角色指派、register/lookup、时间感知副本、淘汰 | **接口冻结** |
| `adapter/iengine.h` | §2 适配层 | IEngineAdapter：族/版本/量化清单、est_prefill、KV 侧门导出注入、"只产不取"降级 | **接口冻结** |
| `observe/journal.h` | §5 观测层 | DecisionJournal（记录+回填）、Metrics（≤15 指标、Prometheus 文本出口） | **接口冻结** |
| `scheduler/decision.h` | §5 成本函数 / §15.2 | Scheduler::decide_fetch_or_compute——系统心脏，含概念级参考实现注释 | **语义冻结：逻辑顺序不得改变** |
| `node/node.h` | §15.3 节点主循环 | Node：心跳→领角色→干活→传感器回流，单线程异步 | **结构冻结** |

## 与提示词总则的配合

1. **任务卡的"设计契约"栏**：直接粘贴本骨架对应头文件（而非整本设计文档）；
2. **"接口要求"栏**：骨架即要求——模型补实现，不改签名；
3. **验收标准**：以"实现与骨架语义零偏差"为第一项；
4. 骨架中所有 `// §x` 注释是答辩线索：代码评审时能逐处指回设计文档。

## 下一步

- **开发计划总表**：M0 任务拆解（metrics 出口 → 黑匣子 → llama.cpp 裸跑接入 → TQUIC benchmark），每个任务一张任务卡；
- M0 提示词包：基于本骨架的 `observe/` 与 `adapter/` 先行（先长眼睛，再长肌肉）。

> 红线提醒：骨架是 P0 全集。vLLM Connector、KV 压缩、闲时预热、FEC——都在停车场里，不在骨架里。
