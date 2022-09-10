
STATIC_INLINE uae_u32 get_word_prefetch (struct regstruct *regs, int o)
{
    uae_u32 v = regs->irc;
    regs->irc = get_word (m68k_getpc(regs) + o);
    return v;
}
STATIC_INLINE uae_u32 get_long_prefetch (struct regstruct *regs, int o)
{
    uae_u32 v = get_word_prefetch (regs, o) << 16;
    v |= get_word_prefetch (regs, o + 2);
    return v;
}

#ifdef CPUEMU_6
#if 0
STATIC_INLINE uae_u32 mem_access_delay_word_read (uaecptr addr)
{
    if (addr < 0x200000 || (addr >= 0xc00000 && addr < 0xe00000)) {
	return wait_cpu_cycle_read (addr, 1);
    } else if (!(addr >= 0xa00000 && addr < 0xc00000)) {
	do_cycles_ce (4 * CYCLE_UNIT / 2);
    }
    return get_word (addr);
}
STATIC_INLINE uae_u32 mem_access_delay_byte_read (uaecptr addr)
{
    if (addr < 0x200000 || (addr >= 0xc00000 && addr < 0xe00000)) {
	return wait_cpu_cycle_read (addr, 0);
    } else if (!(addr >= 0xa00000 && addr < 0xc00000)) {
	do_cycles_ce (4 * CYCLE_UNIT / 2);
    }
    return get_byte (addr);
}
#endif
#ifdef CPUEMU_20

STATIC_INLINE void checkcycles_ce020 (void)
{
	if (regs.ce020memcycles > 0)
		do_cycles_ce (regs.ce020memcycles);
	regs.ce020memcycles = 0;
}

STATIC_INLINE uae_u32 mem_access_delay_long_read_ce020 (uaecptr addr)
{
	checkcycles_ce020 ();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		if ((addr & 3) != 0) {
			uae_u32 v;
			v  = wait_cpu_cycle_read_ce020 (addr + 0, 1) << 16;
			v |= wait_cpu_cycle_read_ce020 (addr + 2, 1) <<  0;
			return v;
		} else {
			return wait_cpu_cycle_read_ce020 (addr, -1);
		}
	case CE_MEMBANK_FAST:
		if ((addr & 3) != 0)
			do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE);
		else
			do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE);
		break;
	case CE_MEMBANK_FAST16BIT:
		do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE);
		break;
	}
	return get_long (addr);
}

STATIC_INLINE uae_u32 mem_access_delay_longi_read_ce020 (uaecptr addr)
{
	checkcycles_ce020 ();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		if ((addr & 3) != 0) {
			uae_u32 v;
			v  = wait_cpu_cycle_read_ce020 (addr + 0, 1) << 16;
			v |= wait_cpu_cycle_read_ce020 (addr + 2, 1) <<  0;
			return v;
		} else {
			return wait_cpu_cycle_read_ce020 (addr, -1);
		}
	case CE_MEMBANK_FAST:
		if ((addr & 3) != 0)
			do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE);
		else
			do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE);
		break;
	case CE_MEMBANK_FAST16BIT:
		do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE);
		break;
	}
	return get_longi (addr);
}

STATIC_INLINE uae_u32 mem_access_delay_word_read_ce020 (uaecptr addr)
{
	checkcycles_ce020 ();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		if ((addr & 3) == 3) {
			uae_u16 v;
			v  = wait_cpu_cycle_read_ce020 (addr + 0, 0) << 8;
			v |= wait_cpu_cycle_read_ce020 (addr + 1, 0) << 0;
			return v;
		} else {
			return wait_cpu_cycle_read_ce020 (addr, 1);
		}
	case CE_MEMBANK_FAST:
	case CE_MEMBANK_FAST16BIT:
		if ((addr & 3) == 3)
			do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE);
		else
			do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE);
		break;
	}
	return get_word (addr);
}

STATIC_INLINE uae_u32 mem_access_delay_wordi_read_ce020 (uaecptr addr)
{
	checkcycles_ce020 ();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		return wait_cpu_cycle_read_ce020 (addr, 1);
	case CE_MEMBANK_FAST:
	case CE_MEMBANK_FAST16BIT:
		do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE);
		break;
	}
	return get_wordi (addr);
}

STATIC_INLINE uae_u32 mem_access_delay_byte_read_ce020 (uaecptr addr)
{
	checkcycles_ce020 ();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		return wait_cpu_cycle_read_ce020 (addr, 0);
	case CE_MEMBANK_FAST:
	case CE_MEMBANK_FAST16BIT:
		do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE);
		break;

	}
	return get_byte (addr);
}

STATIC_INLINE void mem_access_delay_byte_write_ce020 (uaecptr addr, uae_u32 v)
{
	checkcycles_ce020 ();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		wait_cpu_cycle_write_ce020 (addr, 0, v);
		return;
	case CE_MEMBANK_FAST:
	case CE_MEMBANK_FAST16BIT:
		do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE);
		break;
	}
	put_byte (addr, v);
}

STATIC_INLINE void mem_access_delay_word_write_ce020 (uaecptr addr, uae_u32 v)
{
	checkcycles_ce020 ();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		if ((addr & 3) == 3) {
			wait_cpu_cycle_write_ce020 (addr + 0, 0, (v >> 8) & 0xff);
			wait_cpu_cycle_write_ce020 (addr + 1, 0, (v >> 0) & 0xff);
		} else {
			wait_cpu_cycle_write_ce020 (addr + 0, 1, v);
		}
		return;
		break;
	case CE_MEMBANK_FAST:
	case CE_MEMBANK_FAST16BIT:
		if ((addr & 3) == 3)
			do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE);
		else
			do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE);
		break;
	}
	put_word (addr, v);
}

STATIC_INLINE void mem_access_delay_long_write_ce020 (uaecptr addr, uae_u32 v)
{
	checkcycles_ce020 ();
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		if ((addr & 3) == 3) {
			wait_cpu_cycle_write_ce020 (addr + 0, 1, (v >> 16) & 0xffff);
			wait_cpu_cycle_write_ce020 (addr + 2, 1, (v >>  0) & 0xffff);
		} else {
			wait_cpu_cycle_write_ce020 (addr + 0, -1, v);
		}
		return;
		break;
	case CE_MEMBANK_FAST:
		if ((addr & 3) != 0)
			do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE);
		else
			do_cycles_ce020_mem (1 * CPU020_MEM_CYCLE);
		break;
	case CE_MEMBANK_FAST16BIT:
		do_cycles_ce020_mem (2 * CPU020_MEM_CYCLE);
		break;
	}
	put_long (addr, v);
}

STATIC_INLINE uae_u32 get_long_ce020 (uaecptr addr)
{
	return mem_access_delay_long_read_ce020 (addr);
}
STATIC_INLINE uae_u32 get_word_ce020 (uaecptr addr)
{
	return mem_access_delay_word_read_ce020 (addr);
}
STATIC_INLINE uae_u32 get_byte_ce020 (uaecptr addr)
{
	return mem_access_delay_byte_read_ce020 (addr);
}

STATIC_INLINE void put_long_ce020 (uaecptr addr, uae_u32 v)
{
	mem_access_delay_long_write_ce020 (addr, v);
}
STATIC_INLINE void put_word_ce020 (uaecptr addr, uae_u16 v)
{
	mem_access_delay_word_write_ce020 (addr, v);
}
STATIC_INLINE void put_byte_ce020 (uaecptr addr, uae_u8 v)
{
	mem_access_delay_byte_write_ce020 (addr, v);
}

extern void fill_cache0x0 (uae_u32);

STATIC_INLINE uae_u32 get_word_ce020_prefetch (struct regstruct *regs, int o)
{
	uae_u32 pc = m68k_getpc (regs) + o;

	for (;;) {
		if (pc == regs->prefetch020addr) {
			uae_u32 v = regs->prefetch020data >> 16;
			return v;
		}
		if (pc == regs->prefetch020addr + 2) {
			uae_u32 v = regs->prefetch020data & 0xffff;
			fill_cache0x0 (pc + 2);
			return v;
		}
		fill_cache0x0 (pc);
	}
}

STATIC_INLINE uae_u32 get_long_ce020_prefetch (struct regstruct *regs, int o)
{
	uae_u32 v;
	v = get_word_ce020_prefetch (regs, o) << 16;
	v |= get_word_ce020_prefetch (regs, o + 2);
	return v;
}

STATIC_INLINE uae_u32 next_iword_020ce (void)
{
	uae_u32 r = get_word_ce020_prefetch (&regs, 0);
	m68k_incpc (regs, 2);
	return r;
}
STATIC_INLINE uae_u32 next_ilong_020ce (void)
{
	uae_u32 r = get_long_ce020_prefetch (&regs, 0);
	m68k_incpc (regs, 4);
	return r;
}
#endif

#ifdef CPUEMU_12

STATIC_INLINE void ipl_fetch (void)
{
	regs.ipl = regs.ipl_pin;
}

STATIC_INLINE uae_u32 mem_access_delay_word_read (uaecptr addr)
{
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		return wait_cpu_cycle_read (addr, 1);
	case CE_MEMBANK_FAST:
	case CE_MEMBANK_FAST16BIT:
		do_cycles_ce000 (4);
		break;
	}
	return get_word (addr);
}
STATIC_INLINE uae_u32 mem_access_delay_wordi_read (uaecptr addr)
{
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		return wait_cpu_cycle_read (addr, 1);
	case CE_MEMBANK_FAST:
		do_cycles_ce000 (4);
		break;
	}
	return get_wordi (addr);
}

STATIC_INLINE uae_u32 mem_access_delay_byte_read (uaecptr addr)
{
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		return wait_cpu_cycle_read (addr, 0);
	case CE_MEMBANK_FAST:
		do_cycles_ce000 (4);
		break;

    }
    return get_byte (addr);
}
STATIC_INLINE void mem_access_delay_byte_write (uaecptr addr, uae_u32 v)
{
    if (addr < 0x200000 || (addr >= 0xc00000 && addr < 0xe00000)) {
	wait_cpu_cycle_write (addr, 0, v);
	return;
    } else if (!(addr >= 0xa00000 && addr < 0xc00000)) {
	do_cycles_ce (4 * CYCLE_UNIT / 2);
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		wait_cpu_cycle_write (addr, 0, v);
		return;
	case CE_MEMBANK_FAST:
		do_cycles_ce000 (4);
		break;
    }
    put_byte (addr, v);
}
STATIC_INLINE void mem_access_delay_word_write (uaecptr addr, uae_u32 v)
{
    if (addr < 0x200000 || (addr >= 0xc00000 && addr < 0xe00000)) {
	wait_cpu_cycle_write (addr, 1, v);
	return;
    } else if (!(addr >= 0xa00000 && addr < 0xc00000)) {
	do_cycles_ce (4 * CYCLE_UNIT / 2);
	switch (ce_banktype[addr >> 16])
	{
	case CE_MEMBANK_CHIP:
		wait_cpu_cycle_write (addr, 1, v);
		return;
		break;
	case CE_MEMBANK_FAST:
		do_cycles_ce000 (4);
		break;
    }
    put_word (addr, v);
}

STATIC_INLINE uae_u32 get_word_ce (uaecptr addr)
{
    return mem_access_delay_word_read (addr);
}
STATIC_INLINE uae_u32 get_wordi_ce (uaecptr addr)
{
	return mem_access_delay_wordi_read (addr);
}

STATIC_INLINE uae_u32 get_byte_ce (uaecptr addr)
{
    return mem_access_delay_byte_read (addr);
}
/* notes */ 
STATIC_INLINE uae_u32 get_word_ce_prefetch (struct regstruct *regs, int o)
{
    uae_u32 v = regs->irc;
    regs->irc = get_word_ce (m68k_getpc(regs) + o);
}
#if 0
STATIC_INLINE uae_u32 get_word_ce_prefetch (int o)
{
	uae_u32 v = regs->irc;
	regs.irc = get_wordi_ce (m68k_getpc (regs) + o);
    return v;
}
#endif 
STATIC_INLINE void put_word_ce (uaecptr addr, uae_u16 v)
{
    mem_access_delay_word_write (addr, v);
}

STATIC_INLINE void put_byte_ce (uaecptr addr, uae_u8 v)
{
    mem_access_delay_byte_write (addr, v);
}

STATIC_INLINE void m68k_do_rts_ce (struct regstruct *regs)
{
    uaecptr pc;
    pc  = get_word_ce (m68k_areg (regs, 7)) << 16;
    pc |= get_word_ce (m68k_areg (regs, 7) + 2);
    m68k_areg (regs, 7) += 4;
    if (pc & 1)
	exception3 (0x4e75, m68k_getpc (regs), pc);
    else
	m68k_setpc (regs, pc);
}


STATIC_INLINE void m68k_do_bsr_ce (uaecptr oldpc, uae_s32 offset)
{
    m68k_areg (regs, 7) -= 4;
    put_word_ce (m68k_areg (regs, 7), oldpc >> 16);
    put_word_ce (m68k_areg (regs, 7) + 2, oldpc);
    m68k_incpc (regs, offset);
}

STATIC_INLINE void m68k_do_jsr_ce (struct regstruct *regs, uaecptr oldpc, uaecptr dest)
{
    m68k_areg (regs, 7) -= 4;
    put_word_ce (m68k_areg (regs, 7), oldpc >> 16);
    put_word_ce (m68k_areg (regs, 7) + 2, oldpc);
    m68k_setpc (regs, dest);
}

STATIC_INLINE void m68k_do_jsr_ce (uaecptr oldpc, uaecptr dest)
{
	m68k_areg (regs, 7) -= 4;
	put_word_ce (m68k_areg (regs, 7), oldpc >> 16);
	put_word_ce (m68k_areg (regs, 7) + 2, oldpc);
	m68k_setpc (dest);
}
#endif


