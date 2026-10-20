// ============================================================================
// sLLM 概念设计骨架 · scheduler/decision.h
// 契约来源：设计文档 v0.6 §5（成本函数输入清单）、§15.2（系统心脏）
// 本文件是宪法的实现体：函数签名与决策语义冻结，实现细节放开
// ============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "adapter/iengine.h"
#include "node/profile.h"
#include "observe/journal.h"
#include "tracker/tracker.h"
#include "transport/itransport.h"

namespace sllm {

// 决策结果（§5 输出）：FETCH / COMPUTE / 混合（取前 k 块 + 算余下）
enum class Plan : uint8_t { kFetch, kCompute, kMixed };

struct Decision {
    Plan plan;
    uint32_t k{0};              // 命中前缀块数（kMixed 时 = 取的块数）
    std::string source_node;    // kFetch/kMixed 时的副本节点
    double t_fetch_pred_ms{0};
    double t_compute_pred_ms{0};
};

// 安全边际：取必须比算便宜到值得承担 churn 风险（§15.2 MARGIN）
inline constexpr double kDecisionMargin = 1.2;

struct Request {
    std::vector<int32_t> prefix_tokens;
    std::string model_id;
    uint64_t ttl_ms{0};         // 供给稳定性判据：别向快下班的节点取
};

// ============================================================================
// 系统心脏："取 or 算"（§15.2 伪代码的正式签名）
//
// 概念级参考实现（模型可按此结构实现，逻辑顺序不得改变）：
//
// Decision decide_fetch_or_compute(const Request& req, const Node& self) {
//     Key key = make_key(req.prefix_tokens, req.model_id,
//                        self.engine_family(), self.quant_ver());
//     Hit hit = tracker.lookup(key);                 // 最长命中前缀 k + 副本列表
//
//     // 路径一：算（含本地排队）
//     double t_compute = engine.est_prefill_ms(req.prefix_tokens.size())
//                      + queue_wait(self);           // node_load / queue_depth
//
//     // 路径二：取（D1：跨族 T_转换 = ∞，自然淘汰）
//     double t_fetch = INF;
//     for (const Replica& r : hit.replicas) {
//         if (r.engine_family != self.engine_family()) continue;   // D1
//         if (!schedule.online(r.node, horizon = req.ttl)) continue;  // 作息维度
//         LinkStats link = transport.stats(r.node_id, kBulk);      // 神经末梢
//         double t = link.rtt_ms
//                  + bytes(hit.k) / link.bw_avail_bytes_per_sec
//                  + t_decompress(hit.k);          // KV 压缩档位（P1，MVP = 0）
//         t_fetch = std::min(t_fetch, t);
//     }
//
//     Plan plan = (t_fetch < t_compute * kDecisionMargin) ? kFetch : kCompute;
//     journal.log({...});          // 黑匣子：输入快照 + 决策 + 预测 ETA
//     // 事后：实测耗时回填 journal，回放"预测 vs 实际"校准盈亏
// }
// ============================================================================
class Scheduler {
public:
    Scheduler(Tracker& tracker, ITransport& transport,
              DecisionJournal& journal, Metrics& metrics);

    // 唯一决策入口。依赖全部经由构造注入——调度层不感知引擎实现（宪法第 1 条）
    Decision decide_fetch_or_compute(const Request& req,
                                     const IEngineAdapter& self_engine,
                                     const NodeProfile& self_profile);

private:
    Tracker& tracker_;
    ITransport& transport_;
    DecisionJournal& journal_;
    Metrics& metrics_;
};

}  // namespace sllm
