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

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <lunatik.h>

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
	struct kernel_cpustat *kcpustat;
	u64 user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice;
	int nrec = 10; /* number of fields */
	int table;

	if (cpu >= 0) {
		/* Get stats for specific CPU */
		if (cpu >= nr_cpu_ids) {
			return luaL_error(L, "invalid CPU number: %d (max: %d)", cpu, nr_cpu_ids - 1);
		}

		kcpustat = &kcpustat_cpu(cpu);
		user = kcpustat->cpustat[CPUTIME_USER];
		nice = kcpustat->cpustat[CPUTIME_NICE];
		system = kcpustat->cpustat[CPUTIME_SYSTEM];
		idle = kcpustat->cpustat[CPUTIME_IDLE];
		iowait = kcpustat->cpustat[CPUTIME_IOWAIT];
		irq = kcpustat->cpustat[CPUTIME_IRQ];
		softirq = kcpustat->cpustat[CPUTIME_SOFTIRQ];
		steal = kcpustat->cpustat[CPUTIME_STEAL];
		guest = kcpustat->cpustat[CPUTIME_GUEST];
		guest_nice = kcpustat->cpustat[CPUTIME_GUEST_NICE];
	} else {
		/* Aggregate stats for all CPUs */
		int i;
		user = nice = system = idle = iowait = irq = softirq = steal = guest = guest_nice = 0;

		for_each_possible_cpu(i) {
			kcpustat = &kcpustat_cpu(i);
			user += kcpustat->cpustat[CPUTIME_USER];
			nice += kcpustat->cpustat[CPUTIME_NICE];
			system += kcpustat->cpustat[CPUTIME_SYSTEM];
			idle += kcpustat->cpustat[CPUTIME_IDLE];
			iowait += kcpustat->cpustat[CPUTIME_IOWAIT];
			irq += kcpustat->cpustat[CPUTIME_IRQ];
			softirq += kcpustat->cpustat[CPUTIME_SOFTIRQ];
			steal += kcpustat->cpustat[CPUTIME_STEAL];
			guest += kcpustat->cpustat[CPUTIME_GUEST];
			guest_nice += kcpustat->cpustat[CPUTIME_GUEST_NICE];
		}
	}

	/* Create result table */
	lua_createtable(L, 0, nrec);
	table = lua_gettop(L);

	lua_pushinteger(L, (lua_Integer)user);
	lua_setfield(L, table, "user");

	lua_pushinteger(L, (lua_Integer)nice);
	lua_setfield(L, table, "nice");

	lua_pushinteger(L, (lua_Integer)system);
	lua_setfield(L, table, "system");

	lua_pushinteger(L, (lua_Integer)idle);
	lua_setfield(L, table, "idle");

	lua_pushinteger(L, (lua_Integer)iowait);
	lua_setfield(L, table, "iowait");

	lua_pushinteger(L, (lua_Integer)irq);
	lua_setfield(L, table, "irq");

	lua_pushinteger(L, (lua_Integer)softirq);
	lua_setfield(L, table, "softirq");

	lua_pushinteger(L, (lua_Integer)steal);
	lua_setfield(L, table, "steal");

	lua_pushinteger(L, (lua_Integer)guest);
	lua_setfield(L, table, "guest");

	lua_pushinteger(L, (lua_Integer)guest_nice);
	lua_setfield(L, table, "guest_nice");

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

LUNATIK_NEWLIB(cpustat, luacpustat_lib, NULL, NULL);

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

