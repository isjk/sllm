// ============================================================================
// sLLM 概念设计骨架 · adapter/iengine.h
// 契约来源：设计文档 v0.6 §2（适配层薄、可换）、§10 可辩护性备忘
// 宪法第 1 条：引擎 API 演进只允许冲击这一层
// ============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "tracker/key.h"

namespace sllm {

// IEngineAdapter —— 调度层不直接 include 任何引擎头文件（总则 §3）
// P0 实现：llama.cpp（老卡/弱卡/Mac；侧门 KV 序列化）
// P1 实现：vLLM KV Connector（强卡；与 Mooncake 同接口叙事）
class IEngineAdapter {
public:
    virtual ~IEngineAdapter() = default;

    // 本引擎的族与版本（D1 键分量来源，§2）
    virtual EngineFamily family() const = 0;
    virtual std::string version() const = 0;
    virtual std::vector<std::string> supported_quants() const = 0;

    // ---- 成本函数的"算"侧输入 ----
    // 预估本地 Prefill 耗时（不含排队；排队由调度层的 queue_depth 负责）
    virtual double est_prefill_ms(uint32_t prompt_tokens) const = 0;

    // ---- KV 侧门（§8.5 高危项：llama_state_seq_* 序列化慢，issue #8915）----
    // 产池：Prefill 完成后导出 KV 块字节（不透明 blob，D1）
    virtual bool export_kv_block(uint32_t block_index, std::vector<uint8_t>& out) = 0;
    // 取池：注入前 k 块（混合决策"取前 k 块 + 算余下"的落点）
    virtual bool import_kv_prefix(const std::vector<uint8_t>& blob, uint32_t k) = 0;

    // 降级路径（§8.5）：侧门性能不达标 → 本节点退化为"只产不取"，
    // 即 export 保留、import 返回 false —— 池化价值打七折，架构不崩
};

// P0 工厂
std::unique_ptr<IEngineAdapter> make_llamacpp_adapter(const std::string& model_path,
                                                      const std::string& quant);

}  // namespace sllm
