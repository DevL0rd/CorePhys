# Particle Performance Refactor TODO

- [x] Rejected: dirty group refresh no-group fast path benchmarked slower/noisy.
- [ ] Dirty group refresh follow-up: refresh groups only when particle/group state changed enough to require it. Needs explicit group-metric invalidation ownership before code changes.
- [x] Rejected: parallel contact merge benchmarked slower than serial block copy.
- [x] Already handled: body contact query batching exists per particle block with shared shape candidate lists.
- [x] Event diff reserve cleanup: removed per-event reserve calls after whole-buffer reservation; full tests pass, benchmark mixed/noisy.
- [x] Spatial grid cell init cleanup: removed zeroing of occupied-cell output buffer that scatter overwrites before use; full tests pass, benchmark faster in the measured run.
- [x] Rejected: spatial grid next-list init cleanup passed tests but benchmarked slower.
- [x] Rejected: scratch block init cleanup passed tests but benchmarked slower.
- [ ] Deterministic parallel color mixing: keep current serial LiquidFun-equivalent path until an order-preserving independent-contact batch scheduler exists; naive reduction changes color results.
- [ ] Task batching: reduce per-step enqueue/wait churn with a particle phase scheduler; current per-task helper already inlines small ranges, but larger phase batching needs an ownership-level refactor.

Process: finish one item, run focused particle tests, benchmark, then keep or revert based on measured deltas.
