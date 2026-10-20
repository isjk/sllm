// ============================================================================
// sLLM 概念设计骨架 · tracker/tracker.h
// 契约来源：设计文档 v0.6 §5（Tracker 集中目录）、§7（时间感知副本）、§15.4
// 定位：集中目录，不是数据面
// ============================================================================
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "node/profile.h"
#include "tracker/key.h"

namespace sllm {

// 副本条目：一块 KV 的一份拷贝及其所在节点的画像快照
struct Replica {
    std::string node_id;
    EngineFamily engine_family;  // D1：调度层按此过滤，跨族视为未命中
    NodeProfile profile;         // 查询时点上报名片的快照
};

// 最长前缀命中结果（内容寻址的核心查询，§15.4）
struct Hit {
    uint32_t k{0};               // 命中的前缀块数
    std::vector<Replica> replicas;  // 第 k 块断点处的可用副本
};

class Tracker {
public:
    // ---- 节点面（§15.3 节点主循环第 1、2 步）----
    // 心跳上报五维画像
    virtual void heartbeat(const NodeProfile& profile) = 0;
    // 角色指派（调度决策 = 角色指派 × 引擎路由）
    virtual Role assign(const std::string& node_id) = 0;

    // ---- 目录面 ----
    // 注册：KV 块产出后上报 键 + 节点（§6.5 第 6 步）
    virtual void register_block(const Key& key, const std::string& node_id) = 0;
    // 查询：最长前缀命中（§6.5 第 3 步）
    virtual Hit lookup(const Key& key) const = 0;

    // ---- 副本策略（时间感知，§7 收复）----
    // 白天保稳（少副本，省写放大）；夜里扩张（为次日白天的 churn 上保险）
    virtual int replica_target(const Key& hot_key, int hour_of_day) const = 0;

    // ---- 淘汰（降级矩阵 §8.5：池节点磁盘满 → LRU + 作息权重强制回收）----
    virtual void evict(uint64_t pressure_bytes) = 0;

    virtual ~Tracker() = default;
};

// 降级路径（§8.5）：Tracker 挂 → 节点自治降级（本地缓存直取 / 直接重算）+ 重选主
// 实现注意：lookup 超时必须有界，调度层不得因目录不可用而阻塞

}  // namespace sllm
