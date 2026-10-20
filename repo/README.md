# Swarm LLM（sLLM）

> **旧卡不死，只是需要调度。**
>
> P2P 当年对 CDN 做过的事，sLLM 对大模型推理再做一次。

[![License: AGPL v3](https://img.shields.io/badge/License-AGPL%20v3-blue.svg)](LICENSE)

**sLLM 是一个引擎无关的 KV 调度层**：把局域网里吃灰的旧机器——老显卡工作站、无卡旧服务器、甚至一台 Mac mini——拼成一个私有的大模型推理集群。

不换硬件，换调度。

---

## 解决什么问题

中小企业和个人开发者做私有化推理时的死结：

- 新卡买不起，旧机器在机房里吃灰；
- llama.cpp、vLLM 都是好引擎，但都是单点——没有人管「哪台机器算什么、什么时候算、算完的中间结果给谁复用」；
- 引擎是别人的，调度权也在别人手里。

sLLM 补的是中间那层：**跨节点、跨引擎的 KV 调度与复用**。

## 三个核心想法（人话版）

1. **KV 是种子。** 推理最贵的中间产物是 KV Cache。sLLM 把它当 P2P 的种子做内容寻址、登记副本、跨节点复用——算过一次的东西，不算第二次。
2. **取 or 算，先算报价。** 每个请求到达，调度器现场比价：从邻居拉取 KV（T_fetch）和本地重算（T_compute），谁便宜用谁，留 1.2× 安全边际。每次比价记入决策黑匣子，事后实测回填，校准下一次预测。
3. **Prefill 是点播，Decode 是直播。** 两类流量走两条物理隔离的 QUIC 连接：bulk 连接用 CUBIC 开大管道拉 KV 块，stream 连接用 BBRv3 保 token 实时送达，互不抢车道。再配合作息编排——机器白天保稳、夜里扩张，跟着主人的上下班时间干活。

## 架构：三层分离

- **调度层（灵魂 · 永不外包）**：取 or 算成本函数 · 副本管理 · 作息编排 · Tracker 内容寻址目录 · 观测层（metrics + 决策黑匣子）
- **传输层（肌肉）**：TQUIC 双连接分治——设计永不外包，协议实现可选型
- **适配层（薄 · 可换）**：llama.cpp（P0）→ vLLM（P1）；换引擎时上面两层不动

KV 键 = H(前缀 tokens ‖ 模型 ID ‖ 引擎族 ‖ 量化版本)。跨引擎族**不解释、不转换、直接重算**——用一点重算成本，换掉一整个转换层的复杂度。

节点下班、链路抖动、引擎侧门变慢，都有明确的降级路径（详见设计文档降级矩阵一节）。

## 我们不做什么

- **不做推理引擎。** llama.cpp 和 vLLM 已经够好，不重复造轮子。
- **不与任何引擎比峰值吞吐。** 单机极限性能是引擎的战场；sLLM 的战场是集群调度。
- **不做 P2P 文件分发、不做公网穿透。** 借的是 P2P 的调度思想，不是它的旧战场。

## 现状与路线图

| 阶段 | 内容 | 状态 |
|---|---|---|
| 概念设计 | 设计文档 v0.6 + 概念代码骨架（头文件契约） | ✅ 已完成 |
| M0 自举 | 两台机器裸跑 llama.cpp + metrics 出口 + TQUIC 千兆 LAN benchmark | 🔨 进行中 |
| M1 原理通透 | llama.cpp / TQUIC 源码剖析 | ⬜ 未开始 |
| M2 拼集群 | 三台杂牌机 demo：无池 vs 有池，对比数据说话 | ⬜ 未开始 |
| M3 成本闭环 | 真实业务 Agent 接入，出每 token 成本账单 | ⬜ 未开始 |

以上，未开始的就是未开始。这个项目用数据说话，不用幻灯片说话。

## 仓库结构

```
scheduler/   调度层：取or算决策、副本、作息编排
transport/   传输层：ITransport 接口 + TQUIC 实现
adapter/     适配层：IEngineAdapter + llama.cpp 适配
tracker/     KV 内容寻址目录（Tracker）
observe/     metrics 出口 + 决策黑匣子（Decision Journal）
app/         节点主循环入口
```

技术栈：C++17 · [TQUIC](https://github.com/Tencent/tquic)（QUIC 传输）· [llama.cpp](https://github.com/ggml-org/llama.cpp)（P0 引擎）· Prometheus 文本格式（观测）

## 许可证

sLLM 采用双许可（Dual Licensing）：

**开源许可：GNU AGPLv3**（见 [LICENSE](LICENSE)）。
你可以自由下载、使用、修改和再分发 sLLM。前提是：基于 sLLM 的衍生作品——包括修改后通过网络（SaaS / 托管服务）对外提供的服务——必须以相同的 AGPLv3 条款公开源代码。

**商业许可：**
如果你的使用场景不能或不愿履行 AGPLv3 义务，例如：

- 将 sLLM 闭源嵌入自有产品并对外分发；
- 基于 sLLM 对外提供托管 / 云服务而不公开改动；
- 企业合规要求去除 copyleft 义务；

请联系 **[联系方式占位]** 获取商业授权。商业授权可与私有化部署、调优与技术支持服务打包提供。

## License

sLLM is dual-licensed:

- **Open source: GNU AGPLv3** (see [LICENSE](LICENSE)). You may use, modify, and redistribute sLLM freely, provided that derivative works — including modified versions offered to third parties over a network (SaaS / hosted services) — are released under the same AGPLv3 terms.
- **Commercial license:** if AGPLv3 obligations don't fit your use case (closed-source embedding, hosted offerings without source disclosure, or enterprise compliance requirements), contact **[contact placeholder]**. Commercial licensing is available bundled with private deployment, tuning, and support services.

## 参与

项目处于 M0 早期阶段，欢迎围观、提 issue 讨论设计。

向本仓库提交 Pull Request 即表示你同意：你的贡献以 AGPLv3 授权给本项目，并授予项目作者（祝晓光）将你的贡献纳入商业许可版本的权利。

## 关于作者

祝晓光，写了 18 年 P2P 的 C++ 老兵，白天是保险经纪人。
sLLM 最初的用途是给他自己的主业造势——写着写着，发现它能解决真问题，就认真了。

联系方式：**[联系方式占位]**
