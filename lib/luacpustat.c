/*
* SPDX-FileCopyrightText: (c) 2023-2025 Ring Zero Desenvolvimento de Software LTDA
* SPDX-License-Identifier: MIT OR GPL-2.0-only
*/

/***
* Kernel CPU statistics primitives.
* This library provides access to per-CPU statistics from the Linux kernel,
* including time spent in user mode, system mode, idle, iowait, IRQ handling, etc.
* It allows Lua scripts to collect and monitor CPU usage information.
* @module cpustat
*/

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kernel_stat.h>
#include <linux/string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <lunatik.h>

static const lunatik_reg_t cpu_usage_stat[] = {
	{"USER", CPUTIME_USER},
	{"NICE", CPUTIME_NICE},
	{"SYSTEM", CPUTIME_SYSTEM},
	{"SOFTIRQ", CPUTIME_SOFTIRQ},
	{"IRQ", CPUTIME_IRQ},
	{"IDLE", CPUTIME_IDLE},
	{"IOWAIT", CPUTIME_IOWAIT},
	{"STEAL", CPUTIME_STEAL},
	{"GUEST", CPUTIME_GUEST},
	{"GUEST_NICE", CPUTIME_GUEST_NICE},
#ifdef CONFIG_SCHED_CORE
	{"FORCEIDLE", CPUTIME_FORCEIDLE},
#endif
	{NULL, 0}
};

typedef struct luacpustat_s {
	struct kernel_cpustat *cpustat;
	lunatik_object_t *runtime;
} luacpustat_t;

static int luacpustat_get(lua_State *L);
static int luacpustat_count(lua_State *L);

/***
* Retrieves CPU statistics for a specific CPU or all CPUs.
* Returns a table containing various CPU time counters in clock ticks.
* Each field represents the accumulated time (in USER_HZ or clock ticks)
* the CPU has spent in different states.
*
* @function get
* @tparam[opt] integer cpu The CPU number to query (0-based). If omitted or -1,
*   returns statistics for all CPUs combined.
* @treturn table A table with the following fields:
*   @tfield integer user Time spent in user mode
*   @tfield integer nice Time spent in user mode with low priority (nice)
*   @tfield integer system Time spent in system mode
*   @tfield integer idle Time spent in idle task
*   @tfield integer iowait Time spent waiting for I/O to complete
*   @tfield integer irq Time spent servicing interrupts
*   @tfield integer softirq Time spent servicing softirqs
*   @tfield integer steal Time spent in other operating systems (virtualized environment)
*   @tfield integer guest Time spent running a virtual CPU for guest operating systems
*   @tfield integer guest_nice Time spent running a niced guest
* @usage
* local cpustat = require("cpustat")
*
* -- Get statistics for CPU 0
* local cpu0_stats = cpustat.get(0)
* print("CPU 0 user time:", cpu0_stats.user)
* print("CPU 0 idle time:", cpu0_stats.idle)
*
* -- Get combined statistics for all CPUs
* local all_stats = cpustat.get()
* print("Total system time:", all_stats.system)
* @within cpustat
*/
static int luacpustat_get(lua_State *L)
{
	int cpu = luaL_optinteger(L, 1, -1);
	u64 stats[NR_STATS];
	struct kernel_cpustat kcs;
	int i, table;
	int stat_idx;

	/* Initialize stats array */
	memset(stats, 0, sizeof(stats));

	if (cpu >= 0) {
		/* Get stats for specific CPU */
		if (cpu >= nr_cpu_ids) {
			return luaL_error(L, "invalid CPU number: %d (max: %d)", cpu, nr_cpu_ids - 1);
		}

		/* Copy the per-CPU structure */
		kcs = kcpustat_cpu(cpu);
		for (stat_idx = 0; cpu_usage_stat[stat_idx].name != NULL; stat_idx++) {
			enum cpu_usage_stat cputime_idx = cpu_usage_stat[stat_idx].value;
			stats[cputime_idx] = kcs.cpustat[cputime_idx];
		}
	} else {
		/* Aggregate stats for all CPUs */
		for_each_possible_cpu(i) {
			kcs = kcpustat_cpu(i);
			for (stat_idx = 0; cpu_usage_stat[stat_idx].name != NULL; stat_idx++) {
				enum cpu_usage_stat cputime_idx = cpu_usage_stat[stat_idx].value;
				stats[cputime_idx] += kcs.cpustat[cputime_idx];
			}
		}
	}

	/* Create result table and populate it using cpu_usage_stat array */
	lua_createtable(L, 0, NR_STATS);
	table = lua_gettop(L);

	for (stat_idx = 0; cpu_usage_stat[stat_idx].name != NULL; stat_idx++) {
		const char *src = cpu_usage_stat[stat_idx].name;
		char field_name[32];
		char *dst = field_name;

		/* Convert constant name to lowercase field name */
		while (*src && (dst - field_name) < sizeof(field_name) - 1) {
			*dst++ = (*src >= 'A' && *src <= 'Z') ? (*src + 32) : *src;
			src++;
		}
		*dst = '\0';

		lua_pushinteger(L, (lua_Integer)stats[cpu_usage_stat[stat_idx].value]);
		lua_setfield(L, table, field_name);
	}

	return 1; /* table */
}

/***
* Returns the number of CPUs available in the system.
* @function count
* @treturn integer The number of CPUs (nr_cpu_ids)
* @usage
* local cpustat = require("cpustat")
* local num_cpus = cpustat.count()
* print("Number of CPUs:", num_cpus)
* @within cpustat
*/
static int luacpustat_count(lua_State *L)
{
	lua_pushinteger(L, (lua_Integer)nr_cpu_ids);
	return 1;
}

static const luaL_Reg luacpustat_lib[] = {
	{"get", luacpustat_get},
	{"count", luacpustat_count},
	{NULL, NULL}
};

static const lunatik_namespace_t luacpustat_flags[] = {
	{"stat", cpu_usage_stat},
	{NULL, NULL}
};

LUNATIK_NEWLIB(cpustat, luacpustat_lib, NULL, luacpustat_flags);

static int __init luacpustat_init(void)
{
	return 0;
}

static void __exit luacpustat_exit(void)
{
}

module_init(luacpustat_init);
module_exit(luacpustat_exit);
MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("Enderson Maia");

