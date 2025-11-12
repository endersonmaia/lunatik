--
-- SPDX-FileCopyrightText: (c) 2023-2025 Ring Zero Desenvolvimento de Software LTDA
-- SPDX-License-Identifier: MIT OR GPL-2.0-only
--

local cpustat = require("cpustat")
local linux = require("linux")

-- Helper function to calculate percentages
local function calc_usage(prev, curr)
	local total_prev = prev.user + prev.nice + prev.system + prev.idle + 
	                   prev.iowait + prev.irq + prev.softirq + prev.steal
	local total_curr = curr.user + curr.nice + curr.system + curr.idle + 
	                   curr.iowait + curr.irq + curr.softirq + curr.steal
	
	local total_diff = total_curr - total_prev
	if total_diff == 0 then
		return {user = 0, system = 0, idle = 0, iowait = 0}
	end
	
	return {
		user = ((curr.user - prev.user) * 100) / total_diff,
		system = ((curr.system - prev.system) * 100) / total_diff,
		idle = ((curr.idle - prev.idle) * 100) / total_diff,
		iowait = ((curr.iowait - prev.iowait) * 100) / total_diff,
	}
end

-- Get number of CPUs
local num_cpus = cpustat.count()
print(string.format("System has %d CPUs", num_cpus))

-- Get initial stats for all CPUs
local prev_stats = cpustat.get()

-- Wait 1 second
linux.schedule(1000)

-- Get current stats
local curr_stats = cpustat.get()

-- Calculate and display usage
local usage = calc_usage(prev_stats, curr_stats)
print(string.format("Overall CPU Usage:"))
print(string.format("  User:   %.2f%%", usage.user))
print(string.format("  System: %.2f%%", usage.system))
print(string.format("  Idle:   %.2f%%", usage.idle))
print(string.format("  IOWait: %.2f%%", usage.iowait))

-- Display per-CPU stats
print("\nPer-CPU Statistics (absolute values):")
for i = 0, num_cpus - 1 do
	local stats = cpustat.get(i)
	print(string.format("CPU %d: user=%d nice=%d system=%d idle=%d iowait=%d", 
		i, stats.user, stats.nice, stats.system, stats.idle, stats.iowait))
end
