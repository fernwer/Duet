# Duet: A Proxy-Kernel Collaborative Architecture for Agentic SQL

**Duet** is a proxy-kernel cooperative execution framework for agentic SQL workloads. It addresses the mismatch between intent-level agent reasoning and statement-level database execution by using the **Reasoning Window** as the unit for workload shaping, adaptive multi-query optimization (MQO), and snapshot-aware execution.

This repository contains the source code for the prototype implementation of Duet.

## 📂 Repository Structure

The repository is organized into two main components, reflecting the proxy-kernel architecture:

```text
Duet/
├── Proxy/          # Proxy-side workload shaping layer
└── Kernel/         # PostgreSQL kernel extension
```

## 🧩 Component Details

### 1. Proxy/ — Proxy-Side Workload Shaping

This directory contains the implementation of the **Duet Proxy**, which acts as the gateway between agent-generated SQL requests and the database kernel. It performs lightweight pre-execution processing before queries enter the kernel execution path.

**Key features:**

* **Reasoning-window construction:** Groups temporally close and structurally related SQL requests into bounded reasoning windows.
* **Syntactic validation:** Filters malformed SQL before expensive kernel-side optimization.
* **Literal normalization:** Replaces literal values with placeholders to expose reusable query templates.
* **Fingerprinting and hint extraction:** Generates template fingerprints and structural hints, such as referenced relations, predicate columns, and parameter lists.
* **Window metadata propagation:** Passes normalized templates, parameters, hints, and optional snapshot identifiers to the kernel layer.

### 2. Kernel/ — PostgreSQL Kernel Extension

This directory contains the source code for the **Duet Kernel Extension**, implemented on top of PostgreSQL. It extends the standard query processing pipeline to support hint-guided MQO and snapshot-aware cooperative execution.

**Key features:**

* **Adaptive MQO strategy selection:** Uses proxy-provided hints and optimizer information to choose among parameterized plan reuse, predicate coalescing, shared scans, and independent execution.
* **Parameterized plan reuse:** Reuses normalized query templates across multiple parameter bindings within a reasoning window.
* **Predicate coalescing:** Merges compatible predicates, such as structurally similar lookups, into a shared access path when beneficial.
* **Shared-scan execution:** Reuses scan work across compatible queries in the same reasoning window.
* **Snapshot pinning:** Enforces same-snapshot visibility for queries jointly consumed within a reasoning window.
* **Sandboxed execution:** Provides rollback-based isolation for bounded speculative mutations.
* **Fallback execution:** Reverts to standard PostgreSQL execution when cooperative optimization is not beneficial or cannot be safely applied.

## 🚀 Usage

The prototype is organized as a research artifact. The proxy and kernel extension are developed as separate components and should be built independently.

### Build the Proxy

```bash
cd Proxy
mkdir -p build
cd build
cmake ..
make -j
```

### Build the Kernel Extension

```bash
cd Kernel
make
sudo make install
```

Then enable the extension inside PostgreSQL:

```sql
CREATE EXTENSION duet;
```

> Note: The exact build and deployment steps may depend on the local PostgreSQL installation path and extension configuration.

## 📌 Notes

* Duet does not require changes to the agent-side SQL interface.
* Agents continue to issue ordinary SQL requests.
* Reasoning windows are reconstructed below the client interface by the proxy.
* Kernel-side cooperative execution is applied only when runtime checks and optimizer estimates indicate benefit.
* If cooperative optimization is not applicable, Duet falls back to standard PostgreSQL execution.

## 📄 License

This repository is released for research and evaluation purposes.
