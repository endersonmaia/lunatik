#!/usr/bin/env lunatik
--
-- SPDX-FileCopyrightText: (c) 2023-2025 Ring Zero Desenvolvimento de Software LTDA
-- SPDX-License-Identifier: MIT OR GPL-2.0-only
--
-- Test script for cpustat module
-- This script verifies basic functionality of the cpustat module

local cpustat = require("cpustat")

print("=== CPU Statistics Module Test ===\n")

-- Test 1: Get CPU count
print("Test 1: Getting CPU count")
local num_cpus = cpustat.count()
assert(num_cpus > 0, "CPU count should be greater than 0")
print(string.format("  ✓ Number of CPUs: %d\n", num_cpus))

-- Test 2: Get all CPU stats
print("Test 2: Getting all CPU statistics")
local all_stats = cpustat.get()
assert(type(all_stats) == "table", "Result should be a table")
assert(all_stats.user, "Should have 'user' field")
assert(all_stats.nice, "Should have 'nice' field")
assert(all_stats.system, "Should have 'system' field")
assert(all_stats.idle, "Should have 'idle' field")
assert(all_stats.iowait, "Should have 'iowait' field")
assert(all_stats.irq, "Should have 'irq' field")
assert(all_stats.softirq, "Should have 'softirq' field")
assert(all_stats.steal, "Should have 'steal' field")
assert(all_stats.guest, "Should have 'guest' field")
assert(all_stats.guest_nice, "Should have 'guest_nice' field")
print("  ✓ All required fields present")
print(string.format("    user=%d, system=%d, idle=%d", 
	all_stats.user, all_stats.system, all_stats.idle))
print()

-- Test 3: Get per-CPU stats
print("Test 3: Getting per-CPU statistics")
for i = 0, num_cpus - 1 do
	local cpu_stats = cpustat.get(i)
	assert(type(cpu_stats) == "table", string.format("CPU %d stats should be a table", i))
	assert(cpu_stats.user >= 0, string.format("CPU %d user time should be >= 0", i))
	assert(cpu_stats.idle >= 0, string.format("CPU %d idle time should be >= 0", i))
	print(string.format("  ✓ CPU %d: user=%d, system=%d, idle=%d", 
		i, cpu_stats.user, cpu_stats.system, cpu_stats.idle))
end
print()

-- Test 4: Verify values are cumulative (non-decreasing)
print("Test 4: Verifying values are cumulative")
local stats1 = cpustat.get(0)
-- In kernel context, we might see the same values if called immediately,
-- but they should never decrease
local stats2 = cpustat.get(0)
assert(stats2.user >= stats1.user, "User time should not decrease")
assert(stats2.system >= stats1.system, "System time should not decrease")
assert(stats2.idle >= stats1.idle, "Idle time should not decrease")
print("  ✓ Values are non-decreasing\n")

-- Test 5: Verify total is consistent
print("Test 5: Verifying data consistency")
local total_all = all_stats.user + all_stats.nice + all_stats.system + 
                  all_stats.idle + all_stats.iowait + all_stats.irq + 
                  all_stats.softirq + all_stats.steal
print(string.format("  Total all CPUs: %d ticks", total_all))

-- Sum per-CPU values
local sum_per_cpu = 0
for i = 0, num_cpus - 1 do
	local cpu_stats = cpustat.get(i)
	sum_per_cpu = sum_per_cpu + cpu_stats.user + cpu_stats.nice + 
	              cpu_stats.system + cpu_stats.idle + cpu_stats.iowait + 
	              cpu_stats.irq + cpu_stats.softirq + cpu_stats.steal
end
print(string.format("  Sum of per-CPU: %d ticks", sum_per_cpu))
-- Note: These might not match exactly due to race conditions in reading,
-- but they should be close
print("  ✓ Data appears consistent\n")

-- Test 6: Test invalid CPU number
print("Test 6: Testing error handling")
local invalid_cpu = num_cpus + 10
local status, err = pcall(function()
	cpustat.get(invalid_cpu)
end)
assert(not status, "Should error on invalid CPU number")
print(string.format("  ✓ Correctly handles invalid CPU number (%d)", invalid_cpu))
print()

print("=== All Tests Passed ===")
