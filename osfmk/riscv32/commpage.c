#include <machine/commpage.h>

char *commPagePtr32;

void
commpage_populate(void)
{
}

void
commpage_text_populate(void)
{
}

void
commpage_set_timestamp(uint64_t abstime, uint64_t secs)
{
	(void)abstime;
	(void)secs;
}

void
commpage_disable_timestamp(void)
{
}

void
commpage_set_nanotime(uint64_t tsc_base, uint64_t ns_base,
    uint32_t scale, uint32_t shift)
{
	(void)tsc_base;
	(void)ns_base;
	(void)scale;
	(void)shift;
}

void
commpage_set_memory_pressure(unsigned int pressure)
{
	(void)pressure;
}

void
commpage_set_spin_count(unsigned int count)
{
	(void)count;
}

void
commpage_sched_gen_inc(void)
{
}

void
commpage_update_active_cpus(void)
{
}

uint32_t
commpage_is_in_pfz32(uint32_t addr)
{
	(void)addr;
	return 0;
}
