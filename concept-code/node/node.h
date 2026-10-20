// ============================================================================
// sLLM 概念设计骨架 · node/node.h
// 契约来源：设计文档 v0.6 §15.3（节点主循环）、§7（闲时动作）
// 并发模型（总则 §3）：单线程异步 + 定时器优先
// ============================================================================
#pragma once

#include <memory>

#include "adapter/iengine.h"
#include "node/profile.h"
#include "observe/journal.h"
#include "scheduler/decision.h"
#include "tracker/tracker.h"
#include "transport/itransport.h"

namespace sllm {

// 节点主循环（§15.3）：上报画像 → 领角色 → 干活 → 传感器回流
//
// for (;;) {
//     tracker.heartbeat(self.profile());              // 1. 五维画像
//     Role role = tracker.assign(self.id());          // 2. 领角色
//     switch (role) {                                  // 3. 干活
//     case Role::kPrefill: run_prefill();  break;     //    产 KV → 注册回池
//     case Role::kDecode:  run_decode();   break;     //    token 走 stream
//     case Role::kPool:    serve_kv();     break;     //    bulk 供块
//     case Role::kIdle:    idle_task();    break;     //    闲时预热（P1）
//     }
//     transport.stats(...);                           // 4. 传感器回流 → 成本输入
// }
class Node {
public:
    Node(NodeProfile profile,
         std::unique_ptr<IEngineAdapter> engine,
         Tracker& tracker,
         ITransport& transport,
         Scheduler& scheduler,
         DecisionJournal& journal,
         Metrics& metrics);

    // 事件循环入口（单线程异步；确需多线程先请示，总则 §3）
    int run();

    const NodeProfile& profile() const { return profile_; }

private:
    void run_prefill();   // Prefill 产出 → tracker.register_block（§6.5 第 6 步）
    void run_decode();    // token → transport.send_token（stream 连接）
    void serve_kv();      // bulk 连接供块
    void idle_task();     // P1：闲时预热，MVP 空转

    NodeProfile profile_;
    std::unique_ptr<IEngineAdapter> engine_;
    Tracker& tracker_;
    ITransport& transport_;
    Scheduler& scheduler_;
    DecisionJournal& journal_;
    Metrics& metrics_;
};

}  // namespace sllm
