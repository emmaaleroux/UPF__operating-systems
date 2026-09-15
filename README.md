# OPERATING SYSTEMS


> **Course:** Operating Systems (OS) — 2nd Year, B.Sc. in Computer Engineering    
> **Institution:** Universitat Pompeu Fabra (UPF), Barcelona  

---

This repository contains the complete codebase and laboratories for the **Operating Systems** course at UPF. 
The primary focus of the course was building lower-level runtime utilities in C, managing concurrent systems, handling I/O buffering performance, implementing POSIX process/thread execution models, and analyzing user-level context switching on a RISC-V architecture.

--- 

## Key Features 

* **File I/O Performance & Circular Buffers (Lab 1):** Low-level memory buffer processing comparing text vs. raw binary file parsing speed using circular and linear secondary buffers to handle variable-length stream splitting.
* **Custom UNIX Command Shell (Lab 2):** Process management system implemented in C supporting `SINGLE`, `PIPED`, and `CONCURRENT` background execution modes using low-level POSIX system calls (`fork`, `execvp`, `pipe`, `dup2`, and `waitpid`).
* **Multithreaded Image Processing (Lab 3):** Parallelized histogram calculation engine for PGM binary images using POSIX threads, thread-safe offset chunking, and fine-grained mutex synchronization to eliminate race conditions.
* **Producer-Consumer Synchronization (Lab 4):** Concurrent multi-producer / multi-consumer queue architecture with bounded buffers, managed using POSIX semaphores/condition variables, broadcast waking, and graceful teardown handling.
* **User-Level Threading & RISC-V Context Switching (Lab 5):** Analysis and deployment of a cooperative user-space threading engine running on top of the **egos-2000** RISC-V emulator. Explores Thread Control Blocks (TCBs), register saving, and assembly context switches.

---

## Built with

* C
* Bash scripts
* Visual Studio Code
* GitHub

---

## Authorship & Academic Context

* **Authors:** [Emma Leroux] ([@emmaaleroux](https://github.com/emmaaleroux)), [Guillem Arévalo] ([@Guillem-Are](https://github.com/Guillem-Are))
* **Course:** Operating Systems, Universitat Pompeu Fabra (UPF)
