// ============================================================================
// sLLM 概念设计骨架 · transport/itransport.h
// 契约来源：设计文档 v0.6 §6（TQUIC + 双连接分治）
// 宪法第 4 条：两类流量物理隔离为两条独立 QUIC 连接（与实现无关的架构决定）
// 总则红线：本接口是宪法条款的实现体，模型不得修改
// ============================================================================
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>

#include "node/profile.h"
#include "tracker/key.h"

namespace sllm {

// 连接种类（§6）：同一份接口，两种心法
enum class ConnectionKind : uint8_t {
    kBulk,    // KV 块：吞吐优先，CUBIC，Jumbo Frame，低频 ACK（点播心法）
    kStream,  // token 流：时延优先，BBRv3/Copa，高频 ACK（直播心法）
};

// 链路实时状态（§5 画像第四维、§6"神经末梢"）：
// 从 QUIC 实现（P0 = TQUIC）的统计接口直读，直送"取 or 算"成本函数
struct LinkStats {
    double rtt_ms{0};
    double cwnd_utilization{0};        // 拥塞窗口占用率
    double bw_avail_bytes_per_sec{0};  // 可用带宽估计
    double loss_rate{0};               // FEC 条件触发的判据（丢包 >1% 才评估，§12）
};

// 取块结果：成功返回字节视图；失败返回错误（expected 风格，总则 §3）
struct FetchResult {
    bool ok{false};
    std::string error;                 // ok=false 时非空
    uint64_t bytes_fetched{0};
    double elapsed_ms{0};              // 实测耗时 —— 回填决策黑匣子用（§5 观测层）
};

// ITransport —— "设计永不外包，协议实现可选型"的物理载体（§2 右注解）
// P0 实现：TQUIC（Rust 核心 + C API，静态链接进单 binary）
// 实现注意：两个 ConnectionKind 必须落到两条独立 QUIC 连接上——
//           拥塞窗口 per-connection，共用一条连接即违反宪法第 4 条。
class ITransport {
public:
    virtual ~ITransport() = default;

    // ---- 生命周期（churn 承接，§6）----
    // 0-RTT 重连：管"9 点开机 18 点关机"；连接迁移由 QUIC Connection ID 白拿。
    // allow_0rtt 仅允许用于幂等拉取；写入池操作必须传 false（0-RTT 安全边界，图 3）
    virtual bool connect(const std::string& node_id,
                         ConnectionKind kind,
                         bool allow_0rtt) = 0;

    // ---- 数据面 ----
    // bulk：拉取一段 KV 块（同族才取——调用前由调度层按 D1 过滤，此处不重复检查）
    virtual FetchResult fetch_kv(const std::string& node_id,
                                 const Key& key,
                                 std::span<uint8_t> out) = 0;

    // stream：发送 token（直播心法，小包高频）
    virtual bool send_token(const std::string& node_id,
                            std::span<const uint8_t> token_bytes) = 0;

    // ---- 传感器面（神经末梢）----
    virtual LinkStats stats(const std::string& node_id,
                            ConnectionKind kind) const = 0;

    // ---- MyCC 成本钩子（§6 LeCC→MyCC 注入点）----
    // 不重写 CC：在 CC 输出的 pacing rate 上叠加 cost-aware 限速。
    // factor ∈ (0, 1]，由调度层按成本信号给出；1.0 = 不限速。
    virtual void set_pacing_limit(ConnectionKind kind, double factor) = 0;
};

// 实现工厂：返回 TQUIC 实现（P0）。替换实现只允许动这里。
std::unique_ptr<ITransport> make_tquic_transport();

}  // namespace sllm
