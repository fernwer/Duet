***

# Duet: Bridging the Gap between Multi-Agent Systems and Data Kernels via Tiered Cooperative Multi-Query Optimization DUET: A Proxy-Kernel Collaborative Architecture for Agentic SQL

**Duet** is a specialized database extension framework designed to bridge the structural mismatch between the cognitive dynamics of AI Agents and traditional database execution models. By elevating the **Reasoning Window** to a first-class execution unit, Duet synthesizes opportunistic windowing, proxy-driven adaptive hints, and snapshot-pinned consistency into a cohesive runtime environment.

This repository contains the source code for the prototype implementation and the dataset artifacts used for evaluation.

## 📂 Repository Structure

The repository is organized into three main components, reflecting the dual-layer architecture and the experimental workloads:

```text
Duet/
├── Proxy/          # The Smart Proxy Layer
├── Kernel/         # The Database Extension
└── SQL Traces/     # Workload Datasets (Prompts & Generated SQL)
```

## 🧩 Component Details

### 1. Proxy/ (The Smart Proxy Layer)
This directory contains the implementation of the **Duet Proxy**, which acts as the intelligent gateway between AI Agents and the Database Kernel. It is responsible for intercepting, analyzing, and restructuring raw query streams.

*   **Key Features Implemented:**
    *   **Intent-Aware Batching:** Implements the *Opportunistic Windowing* mechanism to reconstruct logical reasoning steps from bursty traffic.
    *   **Normalization & Parameterization:** Strips lexical volatility from raw SQL to enable effective caching.
    *   **Fingerprinting & Hinting:** Generates structural fingerprints (AST hashes) and selectivity signals to guide kernel-side optimization.
    *   **Consistency Management:** Manages the acquisition and propagation of pinned MVCC snapshots ($T_{window}$).

### 2. Kernel/ (The Adaptive Database Engine)
This directory contains the source code for the **Duet Kernel** (based on PostgreSQL). It extends the standard query processing pipeline to support hint-driven execution and inter-query data sharing.

*   **Key Features Implemented:**
    *   **Adaptive Planner:** A modified query optimizer that uses proxy hints to switch between *Parametric Plan Reuse* and *Shared Operator Injection*.
    *   **Cooperative Execution Runtime:** Implements the *Global Shared Scan* operator and *Cooperative Scan Node* for I/O consolidation.
    *   **Snapshot Pinning Enforcement:** Modifications to the storage engine to enforce visibility checks based on the propagated $T_{window}$.
    *   **Sandboxed SubTransactions:** The implementation of the lightweight dry-run mechanism for safe counterfactual reasoning.

### 3. SQL Traces/ (Workload Artifacts)
This directory archives the datasets and workload traces generated for the characterization and evaluation of Agentic SQL.

*   **Contents:**
    *   **Prompts:** The specific prompt templates used to drive LLMs (GPT-4, Llama 3, etc.) in our experiments.
    *   **Generated SQL Logs:** The raw SQL query streams produced by agents executing against the TPC-H benchmarks.

