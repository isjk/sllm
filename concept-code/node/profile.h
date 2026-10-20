// ============================================================================
// sLLM 概念设计骨架 · node/profile.h
// 契约来源：设计文档 v0.6 §5（五维能力画像）、§7（作息编排）
// "画像的精度就是调度的上限"
// ============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "tracker/key.h"

namespace sllm {

// 引擎挂载描述（画像第五维）：引擎族 + 版本 + 支持的量化清单 —— D1 的查询键由此来
struct EngineMount {
    EngineFamily family;
    std::string version;                 // 引擎版本（键粒度由实现定）
    std::vector<std::string> quants;     // 如 {"Q4_K_M", "Q8_0"}
};

// 作息画像（画像第三维）：在线时段的学习结果（§7）
// MVP 阶段允许手工配置；node_online 指标是它的验证回路
struct ScheduleProfile {
    // 简化表达：一天 24 小时的在线概率分布（0..1），由作息学习模块维护
    std::array<float, 24> online_prob{};
    // 供调度判断：该节点在未来 horizon 秒内持续在线的置信度
    float online_confidence(uint64_t horizon_sec) const;
};

// 五维能力画像（§5）：调度决策 = 角色指派 × 引擎路由 的联合优化输入
struct NodeProfile {
    std::string node_id;

    // 1. 算力
    double compute_tflops{0};            // 实测档位，非标称
    double prefill_tokens_per_sec{0};    // est_prefill 的依据

    // 2. 显存/内存容量
    uint64_t vram_bytes{0};
    uint64_t ram_bytes{0};
    uint64_t kv_pool_quota_bytes{0};     // 划拨给 KV 池的配额

    // 3. 在线时段
    ScheduleProfile schedule;

    // 4. 链路（由传输层统计接口直读回填，§6"神经末梢"）
    double rtt_ms{0};
    double cwnd_utilization{0};          // 拥塞窗口占用率
    double bw_avail_bytes_per_sec{0};    // 可用带宽估计

    // 5. 引擎挂载
    std::vector<EngineMount> engines;
};

// 节点角色（§15.3）：调度器指派，不是节点自选
enum class Role : uint8_t {
    kPrefill,   // 产 KV → 注册回池
    kDecode,    // token 走 stream 连接
    kPool,      // bulk 连接供块
    kIdle,      // 闲时预热（P1）
};

}  // namespace sllm
