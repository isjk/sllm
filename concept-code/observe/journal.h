// ============================================================================
// sLLM 概念设计骨架 · observe/journal.h + metrics 出口
// 契约来源：设计文档 v0.6 §5 观测层（"先长眼睛，再长肌肉"）
// 总则 §2：没有观测埋点的代码不予验收
// ============================================================================
#pragma once

#include <cstdint>
#include <string>

#include "tracker/key.h"

namespace sllm {

// 决策黑匣子记录（§5）：每次"取 or 算"一行结构化 JSON
// 答辩绝杀："每一次调度决策的输入输出和事后校验，我都能给你看。"
struct DecisionRecord {
    uint64_t ts_ms{0};
    std::string key_hex;          // 输入：键快照
    uint32_t hit_k{0};            // 输入：命中前缀长度
    uint32_t replica_count{0};    // 输入：可用副本数
    double t_fetch_pred_ms{0};    // 输入：预测的取
    double t_compute_pred_ms{0};  // 输入：预测的算
    std::string decision;         // 输出：FETCH / COMPUTE / MIXED

    // 事后回填（§6.5 第 6 步）：回放"预测 vs 实际"，校准盈亏函数
    double t_actual_ms{-1};       // -1 = 尚未回填
};

class DecisionJournal {
public:
    virtual void log(const DecisionRecord& rec) = 0;   // 决策时追加一行
    virtual void backfill(uint64_t ts_ms, double t_actual_ms) = 0;  // 事后回填
    virtual ~DecisionJournal() = default;
};

// metrics 出口（§5）：/metrics HTTP endpoint，Prometheus 文本格式，零第三方依赖
// 极简指标清单（≤15 个，只埋调度器决策真正需要的点）：
//   kv_hit_rate / decision_count{plan=} / fetch_eta_pred vs fetch_eta_actual
//   transfer_bytes{tc=} / transfer_latency_ms / ttft_ms / decode_tps
//   node_load{node=} / queue_depth / node_online{node=} / cache_pool_bytes
class Metrics {
public:
    virtual void inc_decision(const std::string& plan) = 0;
    virtual void observe_eta(const std::string& kind, double pred, double actual) = 0;
    virtual void observe_transfer(const std::string& tc, uint64_t bytes, double ms) = 0;
    virtual void observe_ttft(double ms) = 0;
    virtual void set_node_online(const std::string& node, bool online) = 0;
    virtual void set_cache_pool_bytes(uint64_t bytes) = 0;

    virtual std::string render_prometheus() const = 0;  // 文本格式出口
    virtual ~Metrics() = default;
};

}  // namespace sllm
