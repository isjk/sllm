// ============================================================================
// sLLM 概念设计骨架 · tracker/key.h
// 契约来源：设计文档 v0.6 §2（D1 引擎族命名空间）、§4（内容寻址）
// 宪法第 3 条：键 = H(前缀 tokens ‖ 模型 ID ‖ 引擎族 ‖ 量化版本)
// ============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sllm {

// 内容寻址哈希（160bit，实现选型：SHA-1/BLAKE3 均可，实现细节放开）
struct Hash {
    std::array<uint8_t, 20> bytes{};
    bool operator==(const Hash&) const = default;
    std::string hex() const;  // info-hash 的可读形态（§4.1）
};

// 引擎族（D1 的核心维度之一）：跨族不解释、不转换、直接重算（T_转换 = ∞）
enum class EngineFamily : uint8_t {
    kLlamaCpp,   // P0
    kVLLM,       // P1（vLLM KV Connector）
    // 未来引擎在此追加 —— 只冲击适配层（宪法第 1 条）
};

// KV 块键（D1）。四个分量缺一不可：
// - 量化版本必进键：Q4_K_M 的 KV 喂给 Q8 会出错（内容寻址的正确性前提）
// - 引擎版本粒度由实现定（llama.cpp 升级可能改变数值）
struct Key {
    Hash h;  // = H(prefix_tokens ‖ model_id ‖ engine_family ‖ quant_ver)
};

// 算键的唯一入口 —— 任何模块不得自行拼哈希（防键空间分裂）
Key make_key(const std::vector<int32_t>& prefix_tokens,
             const std::string& model_id,
             EngineFamily engine_family,
             const std::string& quant_ver);

// 前缀链算键：prompt 按 KV 块切分后逐块算键（§6.5 第 2 步）
// 返回第 i 块的键；增量接口，供最长前缀命中查询逐块推进
Key make_prefix_key(const Key& prev_block_key,
                    const std::vector<int32_t>& block_tokens,
                    const std::string& model_id,
                    EngineFamily engine_family,
                    const std::string& quant_ver);

}  // namespace sllm
