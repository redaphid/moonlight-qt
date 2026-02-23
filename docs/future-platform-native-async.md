# Future Enhancement: Platform-native async primitives for zero-latency event handling

## Summary

Further optimize streaming latency by replacing Qt condition variables with platform-native async primitives for sub-millisecond wake times.

## Current State

After implementing event-driven architecture with Qt's `QWaitCondition` and `QMutex`, latency is already significantly improved. However, there's still overhead from Qt's cross-platform abstraction layer.

## Proposed Enhancement

Use OS-native primitives for critical latency paths:

### macOS (Grand Central Dispatch)
```cpp
dispatch_semaphore_t frameSemaphore = dispatch_semaphore_create(0);

// Producer: Signal when frame arrives
dispatch_semaphore_signal(frameSemaphore);

// Consumer: Wait with zero latency
dispatch_semaphore_wait(frameSemaphore, DISPATCH_TIME_FOREVER);
```

**Benefit:** ~10-50µs wake latency (vs ~100-500µs with Qt)

### Linux (eventfd + epoll)
```cpp
int event_fd = eventfd(0, EFD_NONBLOCK);
epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event_fd, &event);

// Wait for events
epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
```

**Benefit:** Kernel-level efficiency, ~10-100µs latency

### Windows (Event Objects)
```cpp
HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

// Wait
WaitForSingleObject(hEvent, INFINITE);

// Signal
SetEvent(hEvent);
```

**Benefit:** Native Windows scheduler integration

## Performance Impact

| Mechanism | Wake Latency | CPU Overhead | Cross-platform |
|-----------|--------------|--------------|----------------|
| Qt QWaitCondition (current) | ~100-500µs | Low | ✅ Yes |
| GCD/epoll/Events (proposed) | ~10-100µs | Minimal | ❌ Platform-specific |

**Expected improvement:** 50-400µs reduction in frame-to-frame latency

## Implementation Complexity

- **Moderate**: Need platform-specific code paths
- **Requires:** Conditional compilation for each platform
- **Testing:** Need to test on macOS, Linux, and Windows
- **Maintenance:** Three codepaths to maintain

## Recommendation

Implement **after** basic event-driven architecture is proven stable. This is a micro-optimization that requires significantly more complexity.

## Related Work

- PR #4: Initial latency optimizations (timeout reductions)
- Branch `add-performance-tests`: Synthetic benchmarks
- Upcoming: Event-driven decoder/VSync/input (condition variables)

## Technical Details

### Critical Paths to Optimize

1. **Decoder thread wake** (`ffmpeg.cpp:1792`)
   - Currently: QWaitCondition (~200µs)
   - Native: dispatch_semaphore_wait (~20µs)

2. **VSync signaling** (`pacer.cpp:117`)
   - Currently: QWaitCondition (~200µs)
   - Native: eventfd write/read (~30µs)

3. **Frame queue notifications**
   - Lock-free SPSC queue + semaphore = <10µs

### Benchmarking Plan

```cpp
// Measure wake latency
auto start = std::chrono::high_resolution_clock::now();
// Signal from thread A
// Wake on thread B
auto end = std::chrono::high_resolution_clock::now();
auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
```

Should see <100µs consistently with native primitives.

## References

- [GCD Semaphores](https://developer.apple.com/documentation/dispatch/1452955-dispatch_semaphore_wait)
- [Linux eventfd](https://man7.org/linux/man-pages/man2/eventfd.2.html)
- [Windows Events](https://docs.microsoft.com/en-us/windows/win32/sync/event-objects)
- [Lock-free Programming](https://www.1024cores.net/home/lock-free-algorithms/queues)

## Priority

**Low-Medium** - Implement only after:
1. ✅ Basic timeout reductions (PR #4)
2. ⏳ Event-driven architecture with Qt primitives
3. ⏳ Comprehensive real-world benchmarks proving baseline

**This is a polish optimization, not a fundamental architecture change.**

---

To propose this upstream, create an issue at: https://github.com/moonlight-stream/moonlight-qt/issues
