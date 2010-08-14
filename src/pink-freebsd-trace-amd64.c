/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

/*
 * Copyright (c) 2010 Ali Polatel <alip@exherbo.org>
 * Copyright (c) 1997 Sean Eric Fagan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>

#include <pinktrace/internal.h>
#include <pinktrace/pink.h>

struct bitness_types {
	const char *type;
	pink_bitness_t bitness;
} bitness_types[] = {
	{"FreeBSD ELF64", PINK_BITNESS_64},
	{"FreeBSD ELF32", PINK_BITNESS_32},
	{NULL, -1}
};

pink_bitness_t
pink_bitness_get(pid_t pid)
{
	char progt[32];
	size_t len = sizeof(progt);
	int mib[4];
	struct bitness_types *walk;

	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_SV_NAME;
	mib[3] = pid;

	if (sysctl(mib, 4, progt, &len, NULL, 0) < 0)
		return PINK_BITNESS_UNKNOWN;

	for (walk = bitness_types; walk->type; walk++) {
		if (!strcmp(walk->type, progt))
			return walk->bitness;
	}

	return PINK_BITNESS_UNKNOWN;
}

bool
pink_util_get_syscall(pid_t pid, pink_bitness_t bitness, long *res)
{
	long parm_offset;
	struct reg r;

	assert(bitness == PINK_BITNESS_32 || bitness == PINK_BITNESS_64);

	if (!pink_util_get_regs(pid, &r))
		return false;

	/*
	 * FreeBSD has two special kinds of system call redirections --
	 * SYS_syscall, and SYS___syscall.  The former is the old syscall()
	 * routine, basicly; the latter is for quad-aligned arguments.
	 */
	*res = r.r_rax;
	switch (*res) {
	case SYS_syscall:
	case SYS___syscall:
		if (bitness == PINK_BITNESS_32) {
			parm_offset = r.r_rsp + sizeof(int);
			if (!pink_util_peekdata(pid, parm_offset, res))
				return false;
		}
		else
			*res = r.r_rdi;
		return true;
	default:
		return true;
	}
}

bool
pink_util_set_syscall(pid_t pid, pink_unused pink_bitness_t bitness, long scno)
{
	struct reg r;

	if (!pink_util_get_regs(pid, &r))
		return false;

	r.r_rax = scno;

	if (!pink_util_set_regs(pid, &r))
		return false;

	return true;
}

bool
pink_util_get_return(pid_t pid, long *res)
{
	bool errorp;
	struct reg r;

	if (!pink_util_get_regs(pid, &r))
		return false;

	if (res) {
		errorp = !!(r.r_rflags & PSL_C);
		*res = errorp ? -r.r_rax : r.r_rax;
	}

	return true;
}

bool
pink_util_set_return(pid_t pid, long ret)
{
	struct reg r;

	if (!pink_util_get_regs(pid, &r))
		return false;

	if (ret < 0) {
		r.r_rax = -ret;
		r.r_rflags |= PSL_C;
	}
	else
		r.r_rax = ret;

	if (!pink_util_set_regs(pid, &r))
		return false;

	return true;
}

bool
pink_util_get_arg(pid_t pid, pink_bitness_t bitness, unsigned ind, long *res)
{
	unsigned parm_offset;
	struct reg r;

	assert(bitness == PINK_BITNESS_32 || bitness == PINK_BITNESS_64);
	assert(ind < PINK_MAX_INDEX);

	if (!pink_util_get_regs(pid, &r))
		return false;

	/*
	 * FreeBSD has two special kinds of system call redirctions --
	 * SYS_syscall, and SYS___syscall.  The former is the old syscall()
	 * routine, basicly; the latter is for quad-aligned arguments.
	 */
	parm_offset = r.r_rsp + (bitness == PINK_BITNESS_32 ? sizeof(int) : sizeof(register_t));
	switch (r.r_rax) {
	case SYS_syscall:
		if (bitness == PINK_BITNESS_64)
			++ind;
		else
			parm_offset += sizeof(int);
		break;
	case SYS___syscall:
		if (bitness == PINK_BITNESS_64)
			++ind;
		else
			parm_offset += sizeof(quad_t);
		break;
	default:
		break;
	}

	if (bitness == PINK_BITNESS_32) {
		parm_offset += ind * sizeof(int);
		if (!pink_util_peekdata(pid, parm_offset, res))
			return false;
		return true;
	}

	switch (ind) {
	case 0:
		*res = r.r_rdi;
		break;
	case 1:
		*res = r.r_rsi;
		break;
	case 2:
		*res = r.r_rdx;
		break;
	case 3:
		*res = r.r_rcx;
		break;
	case 4:
		*res = r.r_r8;
		break;
	case 5:
		*res = r.r_r9;
		break;
	case 6:
		/* system call redirection */
		if (!pink_util_peekdata(pid, parm_offset, res))
			return false;
		break;
	default:
		abort();
	}

	return true;
}

bool
pink_decode_simple(pid_t pid, pink_bitness_t bitness, unsigned ind, void *dest, size_t len)
{
	long addr;

	assert(ind < PINK_MAX_INDEX);

	return pink_util_get_arg(pid, bitness, ind, &addr) && pink_util_moven(pid, addr, dest, len);
}

bool
pink_decode_string(pid_t pid, pink_bitness_t bitness, unsigned ind, char *dest, size_t len)
{
	long addr;

	assert(ind < PINK_MAX_INDEX);

	return pink_util_get_arg(pid, bitness, ind, &addr) && pink_util_movestr(pid, addr, dest, len);
}

char *
pink_decode_string_persistent(pid_t pid, pink_bitness_t bitness, unsigned ind)
{
	long addr;

	assert(ind < PINK_MAX_INDEX);

	if (!pink_util_get_arg(pid, bitness, ind, &addr))
		return NULL;

	return pink_util_movestr_persistent(pid, addr);
}

bool
pink_decode_socket_address(pid_t pid, pink_bitness_t bitness, unsigned ind,
	long *fd_r, pink_socket_address_t *addr_r)
{
	long addr;
	long addrlen;

	if (fd_r && !pink_util_get_arg(pid, bitness, 0, fd_r))
		return false;

	if (!pink_util_get_arg(pid, bitness, ind, &addr) || !pink_util_get_arg(pid, bitness, ind + 1, &addrlen))
		return false;

	return pink_internal_decode_socket_address(pid, addr, addrlen, addr_r);
}

bool
pink_encode_simple(pid_t pid, pink_bitness_t bitness, unsigned ind, const void *src, size_t len)
{
	long addr;

	assert(ind < PINK_MAX_INDEX);

	return pink_util_get_arg(pid, bitness, ind, &addr) && pink_util_putn(pid, addr, src, len);
}