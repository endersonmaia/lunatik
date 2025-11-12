# CPU Statistics Module (cpustat)

This Lunatik module provides access to Linux kernel CPU statistics, allowing Lua scripts running in kernel space to monitor CPU usage and performance metrics.

## Features

- Query CPU statistics for individual CPUs or all CPUs combined
- Access detailed time counters for different CPU states
- Monitor user time, system time, idle time, I/O wait, IRQ handling, and more
- Compatible with kernel monitoring and performance analysis tools

## Functions

### `cpustat.get([cpu])`

Retrieves CPU statistics for a specific CPU or all CPUs.

**Parameters:**
- `cpu` (optional): CPU number (0-based). If omitted or -1, returns aggregated statistics for all CPUs.

**Returns:**
A table containing the following fields (all values are in clock ticks):
- `user`: Time spent in user mode
- `nice`: Time spent in user mode with low priority (nice)
- `system`: Time spent in system mode
- `idle`: Time spent in idle task
- `iowait`: Time spent waiting for I/O to complete
- `irq`: Time spent servicing hardware interrupts
- `softirq`: Time spent servicing software interrupts
- `steal`: Time stolen by other operating systems (virtualized environments)
- `guest`: Time spent running a virtual CPU for guest OS
- `guest_nice`: Time spent running a niced guest

**Example:**
```lua
local cpustat = require("cpustat")

-- Get stats for CPU 0
local cpu0 = cpustat.get(0)
print("CPU 0 user time:", cpu0.user)
print("CPU 0 idle time:", cpu0.idle)

-- Get combined stats for all CPUs
local all_cpus = cpustat.get()
print("Total system time:", all_cpus.system)
```

### `cpustat.count()`

Returns the number of CPUs available in the system.

**Returns:**
Integer representing the number of CPUs (nr_cpu_ids)

**Example:**
```lua
local cpustat = require("cpustat")
local num_cpus = cpustat.count()
print("System has", num_cpus, "CPUs")
```

## Understanding the Values

The values returned by `cpustat.get()` are cumulative counters in clock ticks (usually at USER_HZ frequency, typically 100 Hz). To calculate CPU usage percentages:

1. Take two samples at different times
2. Calculate the difference for each field
3. Calculate the total difference (sum of all fields)
4. Divide individual differences by total difference and multiply by 100

**Example:**
```lua
local function calc_usage(prev, curr)
    local total_prev = prev.user + prev.nice + prev.system + prev.idle + 
                       prev.iowait + prev.irq + prev.softirq + prev.steal
    local total_curr = curr.user + curr.nice + curr.system + curr.idle + 
                       curr.iowait + curr.irq + curr.softirq + curr.steal
    
    local total_diff = total_curr - total_prev
    if total_diff == 0 then return 0 end
    
    local busy = (curr.user - prev.user) + (curr.system - prev.system)
    return (busy * 100.0) / total_diff
end

local prev = cpustat.get()
linux.schedule(1000)  -- Wait 1 second
local curr = cpustat.get()
local usage = calc_usage(prev, curr)
print(string.format("CPU usage: %.2f%%", usage))
```

## Examples

### Basic CPU Monitoring
See `examples/cpustat_monitor.lua` for a simple example that:
- Displays the number of CPUs
- Calculates overall CPU usage
- Shows per-CPU statistics

### Real-time Monitoring
See `examples/cpustat_realtime.lua` for a more advanced example that:
- Continuously monitors CPU usage
- Displays both overall and per-CPU metrics
- Shows detailed breakdowns (user, system, iowait, etc.)

## Building and Loading

The cpustat module is built as part of the Lunatik build process:

```bash
make
sudo make install
```

Load the module:
```bash
sudo modprobe luacpustat
```

## Use Cases

- System performance monitoring
- Load balancing analysis
- Resource usage tracking
- CPU hotspot detection
- Benchmarking and profiling
- Creating custom monitoring tools
- Integration with kernel-based analytics

## Technical Details

The module accesses the kernel's `struct kernel_cpustat` via the `kcpustat_cpu()` macro. It reads per-CPU statistics from the kernel's accounting system, which tracks time spent in various states.

The values correspond to:
- `CPUTIME_USER`: Normal user mode execution
- `CPUTIME_NICE`: User mode with nice priority
- `CPUTIME_SYSTEM`: Kernel mode execution
- `CPUTIME_IDLE`: Idle time
- `CPUTIME_IOWAIT`: Waiting for I/O
- `CPUTIME_IRQ`: Hardware interrupt handling
- `CPUTIME_SOFTIRQ`: Software interrupt handling
- `CPUTIME_STEAL`: Time stolen in virtualized environments
- `CPUTIME_GUEST`: Virtual CPU time
- `CPUTIME_GUEST_NICE`: Virtual CPU time (niced)

## Notes

- Clock tick values need to be converted to percentages by calculating differences over time
- The module requires CONFIG_LUNATIK_CPUSTAT=m in the kernel configuration
- Values are cumulative since boot and will wrap around on very long uptimes
- Guest time is already included in user time (don't double-count)
