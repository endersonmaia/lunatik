--
-- SPDX-FileCopyrightText: (c) 2023-2025 Ring Zero Desenvolvimento de Software LTDA
-- SPDX-License-Identifier: MIT OR GPL-2.0-only
--
-- Simple CPU usage monitoring example
-- This script demonstrates how to use the cpustat module to monitor
-- CPU usage in real-time from within the kernel

local cpustat = require("cpustat")
local linux = require("linux")
local thread = require("thread")

-- Configuration
local SAMPLE_INTERVAL_MS = 1000  -- Sample every 1 second
local NUM_SAMPLES = 10            -- Take 10 samples

-- Helper function to calculate CPU usage percentage
local function calc_cpu_usage(prev, curr)
	-- Calculate total time difference
	local prev_total = prev.user + prev.nice + prev.system + prev.idle + 
	                   prev.iowait + prev.irq + prev.softirq + prev.steal
	local curr_total = curr.user + curr.nice + curr.system + curr.idle + 
	                   curr.iowait + curr.irq + curr.softirq + curr.steal
	
	local total_diff = curr_total - prev_total
	if total_diff == 0 then
		return {
			user = 0, nice = 0, system = 0, idle = 0,
			iowait = 0, irq = 0, softirq = 0, steal = 0,
			total = 0
		}
	end
	
	-- Calculate individual percentages
	local usage = {
		user = ((curr.user - prev.user) * 100.0) / total_diff,
		nice = ((curr.nice - prev.nice) * 100.0) / total_diff,
		system = ((curr.system - prev.system) * 100.0) / total_diff,
		idle = ((curr.idle - prev.idle) * 100.0) / total_diff,
		iowait = ((curr.iowait - prev.iowait) * 100.0) / total_diff,
		irq = ((curr.irq - prev.irq) * 100.0) / total_diff,
		softirq = ((curr.softirq - prev.softirq) * 100.0) / total_diff,
		steal = ((curr.steal - prev.steal) * 100.0) / total_diff,
	}
	
	-- Calculate total busy time (100 - idle)
	usage.total = 100.0 - usage.idle
	
	return usage
end

-- Print header
local num_cpus = cpustat.count()
print(string.format("=== CPU Usage Monitor ==="))
print(string.format("Number of CPUs: %d", num_cpus))
print(string.format("Sample interval: %d ms", SAMPLE_INTERVAL_MS))
print(string.format("Number of samples: %d", NUM_SAMPLES))
print("")

-- Main monitoring loop
local sample_count = 0
local prev_all = cpustat.get()
local prev_per_cpu = {}

-- Initialize per-CPU previous stats
for i = 0, num_cpus - 1 do
	prev_per_cpu[i] = cpustat.get(i)
end

while sample_count < NUM_SAMPLES and not thread.shouldstop() do
	-- Wait for sample interval
	linux.schedule(SAMPLE_INTERVAL_MS)
	
	-- Get current stats
	local curr_all = cpustat.get()
	
	-- Calculate overall system usage
	local usage_all = calc_cpu_usage(prev_all, curr_all)
	
	print(string.format("Sample #%d", sample_count + 1))
	print(string.format("  Overall: %.2f%% (user: %.2f%%, system: %.2f%%, iowait: %.2f%%)",
		usage_all.total, usage_all.user, usage_all.system, usage_all.iowait))
	
	-- Calculate and display per-CPU usage
	for i = 0, num_cpus - 1 do
		local curr_cpu = cpustat.get(i)
		local usage_cpu = calc_cpu_usage(prev_per_cpu[i], curr_cpu)
		
		print(string.format("  CPU%d: %.2f%% (user: %.2f%%, system: %.2f%%, idle: %.2f%%)",
			i, usage_cpu.total, usage_cpu.user, usage_cpu.system, usage_cpu.idle))
		
		prev_per_cpu[i] = curr_cpu
	end
	
	print("")
	
	-- Update previous stats
	prev_all = curr_all
	sample_count = sample_count + 1
end

print("Monitoring complete.")
