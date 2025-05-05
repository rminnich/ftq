# FTQ Benchmark and SPR
Anonymous \
2025-04-24

## Summary
The FTQ benchmark was observed to exhibit different behavior on Sapphire Rapids (SPR) than older Intel and AMD processors. Whereas older processors executed the main loop at a consistent IPC, the newer CPU core was found to be sensitive to compiler options. In particular, the debug (-O0) build had a high variation in IPC, both run-to-run and within the same run. The behavior on SPR was found to be related to memory-renaming features in the newer CPU.

## Workload Characterization
According to the documentation, FTQ “prob[es] the interference characteristics of a particular computer [...] as experienced by user applications.” In other words, it aims to measure the variation in throughput due to interference from the kernel. The main loop of the benchmark is as such:


```
volatile unsigned long long count = 0;

for (k = 0; k < 32; k++)
	count++;

for (k = 0; k < (32 - 1); k++)
	count--;
```

The debug build was measured to run at 1 IPC on Skylake (SKX) and the optimized build at 0.5 IPC. The difference between the debug and optimized builds lies in the unrolling of the loop over k, which is fully unrolled on the optimized build. On SPR, the optimized build was measured to run at a dramatically faster 3 IPC. However, the debug build, while also running much faster, was found to exhibit variation between 2-2.5 IPC. Topdown counters showed that the variation was correlated with [tma_l1_bound](https://github.com/torvalds/linux/blob/master/tools/perf/pmu-events/arch/x86/skylakex/skx-metrics.json#L1470) (latency-bound without L1 miss).

## Skylake Execution Analysis
### Optimized

```
  404058:       48 8b 44 24 f8          mov    rax,QWORD PTR [rsp-0x8]
  40405d:       48 83 c0 01             add    rax,0x1
  404061:       48 89 44 24 f8          mov    QWORD PTR [rsp-0x8],rax
  404066:       48 8b 44 24 f8          mov    rax,QWORD PTR [rsp-0x8]
  40406b:       48 83 c0 01             add    rax,0x1
  40406f:       48 89 44 24 f8          mov    QWORD PTR [rsp-0x8],rax
  ...

  404226:       48 8b 44 24 f8          mov    rax,QWORD PTR [rsp-0x8]
  40422b:       48 83 e8 01             sub    rax,0x1
  40422f:       48 89 44 24 f8          mov    QWORD PTR [rsp-0x8],rax
  404234:       48 8b 44 24 f8          mov    rax,QWORD PTR [rsp-0x8]
  404239:       48 83 e8 01             sub    rax,0x1
  40423d:       48 89 44 24 f8          mov    QWORD PTR [rsp-0x8],rax
  ...
```

The optimized build is reduced to a repeated sequence of three instructions, a load, add/sub, then store to the same address. The store-to-load forwarding delay on SKX is 5 cycles, and add has 1 cycle of latency. As this code has a data dependency on rax, it can only complete one iteration (3 instructions) per 6 cycles, which matches exactly the observed 0.5 IPC.

### Debug
```
  405058:       c7 45 fc 00 00 00 00    mov    DWORD PTR [rbp-0x4],0x0
  40505f:       eb 10                   jmp    405071 <main_loops+0x71>
  405061:       48 8b 45 c8             mov    rax,QWORD PTR [rbp-0x38]
  405065:       48 83 c0 01             add    rax,0x1
  405069:       48 89 45 c8             mov    QWORD PTR [rbp-0x38],rax
  40506d:       83 45 fc 01             add    DWORD PTR [rbp-0x4],0x1
  405071:       83 7d fc 1f             cmp    DWORD PTR [rbp-0x4],0x1f
  405075:       7e ea                   jle    405061 <main_loops+0x61>
  405077:       c7 45 fc 00 00 00 00    mov    DWORD PTR [rbp-0x4],0x0
  40507e:       eb 10                   jmp    405090 <main_loops+0x90>
  405080:       48 8b 45 c8             mov    rax,QWORD PTR [rbp-0x38]
  405084:       48 83 e8 01             sub    rax,0x1
  405088:       48 89 45 c8             mov    QWORD PTR [rbp-0x38],rax
  40508c:       83 45 fc 01             add    DWORD PTR [rbp-0x4],0x1
  405090:       83 7d fc 1e             cmp    DWORD PTR [rbp-0x4],0x1e
  405094:       7e ea                   jle    405080 <main_loops+0x80>
  405096:       e8 65 ef ff ff          call   404000 <getticks>
  40509b:       48 89 45 e0             mov    QWORD PTR [rbp-0x20],rax
  40509f:       48 8b 45 e0             mov    rax,QWORD PTR [rbp-0x20]
  4050a3:       48 3b 45 d8             cmp    rax,QWORD PTR [rbp-0x28]
  4050a7:       72 af                   jb     405058 <main_loops+0x58>
```

The debug build contains two dependency chains. One is the same load-add-store sequence to the address at rbp-0x38, which has 6 cycles of latency as previously analyzed. The other, parallel, dependency chain on the loop counter at rbp-0x4 also has 6 cycles of latency due to store-to-load forwarding on the memory operand on the add instruction. The compare-branch at the end of the loop is predicted and carries no latency. Overall, this code still requires 6 cycles per iteration, but it has twice as many instructions as the optimized build, hence 1 IPC.

## Golden Cove
Intel’s [Golden Cove](https://chipsandcheese.com/p/popping-the-hood-on-golden-cove) (GLC) core, which is featured on Sapphire, Emerald, and Granite Rapids, supports memory renaming[^1]. Memory renaming is a speculative feature in the CPU frontend that predicts load and stores to the same address and renames them to the same physical register, eliminating the need for store-to-load forwarding. Additionally, GLC supports immediate renaming[^2], which detects and coalesces sequential instructions involving immediates (data values encoded directly in instructions). Applying this knowledge to the optimized code, the transformation to the dependency graph is stark:

| Unrenamed                      | Renamed (3-operand)                   |
|--------------------------------|---------------------------------------|
| mov    rax,QWORD PTR [rsp-0x8] | mov    r1,QWORD PTR [rsp-0x8] ; or rN |
| add    rax,0x1                 | add    r2,r1,0x1                      |
| mov    QWORD PTR [rsp-0x8],rax | mov    QWORD PTR [rsp-0x8],r2         |
| mov    rax,QWORD PTR [rsp-0x8] | nop                                   |
| add    rax,0x1                 | add    r3,r2,0x1  ; or add r3,r1,0x2  |
| mov    QWORD PTR [rsp-0x8],rax | mov    QWORD PTR [rsp-0x8],r3         |
| mov    rax,QWORD PTR [rsp-0x8] | nop                                   |
| add    rax,0x1                 | add    r4,r3,0x1  ; or add r4,r1,0x3  |
| mov    QWORD PTR [rsp-0x8],rax | mov    QWORD PTR [rsp-0x8],r4         |
| ...                            | ...                                   |


The transformed dependency chain has only a single memory load for the first iteration, which may itself be renamed from a prior iteration of the outer loop. The remaining dependency chain is on the repeated add instruction, which has a latency of 1. This add itself could be renamed by the immediate renamer, but this is not observed in practice, likely due to it not combining with memory renaming. This code executed at 3 IPC on SPR.

The same renaming applies to the debug build. However, the number of renaming resources required is greater due to the presence of two dependency chains and the aliasing of the program counter[^3] mapping across loop iterations. While this code could in theory achieve 6 IPC, in practice it does not.


| Unrenamed                       | Renamed (3-operand)                   |
| --------------------------------|---------------------------------------|
| mov    rax,QWORD PTR [rbp-0x38] | mov    r1,rN                          |
| add    rax,0x1                  | add    r2,r1,0x1                      |
| mov    QWORD PTR [rbp-0x38],rax | mov    QWORD PTR [rbp-0x38],rax       |
| add    DWORD PTR [rbp-0x4],0x1  | add    r3,rM,0x1                      |
| cmp    DWORD PTR [rbp-0x4],0x1f | mov    DWORD PTR [rbp-0x4],r3  ; uop  |
| jle    405061 <main_loops+0x61> | cmp    r3,0x1f                        |
|                                 | jle    405061                         |

The reason for the instability in the IPC of the debug build is not immediately clear. However, it can be seen that it is more fragile than the optimized build, due to the aliasing of the program counter (32 registers for 32 iterations). If renaming for either address fails, the loop will again have 6 cycles of latency due to store-to-load forwarding, and the loop IPC will be 1. Public domain discussion suggests that the dependency chain, and thus the training of the renamer, can be affected by register reloads due to interrupt handlers and context switches. It is likely that the renamer is self-recovering in the optimized build due to the lack of aliasing.

## Conclusion
Memory renaming is a speculative feature found in modern CPU cores that breaks assumptions used in the FTQ benchmark. While older cores ran this benchmark at similar IPCs due to serialization on store-to-load forwarding latency, newer cores are able to eliminate this dependency. It is likely that future advancements in speculation will fully break the dependencies in this benchmark, enabling the core to execute at infinite ILP, leading to confusing benchmark results.

[^1]: https://www.realworldtech.com/forum/?threadid=186393&curpostid=186393
[^2]: https://news.ycombinator.com/item?id=42579969
[^3]: Memory renaming is a speculative feature. While the details are proprietary, it is likely to be indexed by program counter and a history buffer.
