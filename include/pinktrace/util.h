/* vim: set cino= fo=croql sw=8 ts=8 sts=0 noet cin fdm=syntax : */

/*
 * Copyright (c) 2010 Ali Polatel <alip@exherbo.org>
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

#ifndef PINKTRACE_GUARD_UTIL_H
#define PINKTRACE_GUARD_UTIL_H 1

/**
 * \file
 * Pink's trace utility functions
 **/

#include <stdbool.h>
#include <sys/types.h>

/**
 * The index arguments should be smaller than this define.
 *
 * \see pink_util_get_arg
 * \see pink_decode_simple
 * \see pink_decode_string
 * \see pink_decode_string_persistent
 * \see pink_decode_socket_fd
 * \see pink_decode_socket_address
 * \see pink_encode_simple
 * \see pink_encode_simple_safe
 **/
#define PINK_MAX_INDEX 6

/**
 * On FreeBSD this reads a single int of data at the given offset in the traced
 * process's instruction space and places it in res, aka PT_READ_I.
 *
 * On Linux this reads a word at the given offset in the child's USER area, and
 * places it in res, aka PTRACE_PEEKUSER.
 *
 * \note Mostly for internal use, use higher level functions where possible.
 *
 * \param pid Process ID of the child whose USER area is to be read.
 * \param off Offset
 * \param res Result (may be NULL, e.g. to test if the given offset is readable)
 *
 * \return true on success, false on failure and sets errno accordingly.
 **/
bool
pink_util_peek(pid_t pid, long off, long *res);

/**
 * On FreeBSD this reads a single int of data at the given offset in the traced
 * process's data space and places it in res, aka PT_READ_D.
 *
 * On Linux this reads a word at the given offset in the child's memory, and
 * places it in res, aka PTRACE_PEEKDATA.
 *
 * \note Mostly for internal use, use higher level functions where possible.
 *
 * \param pid Process ID of the child whose USER area is to be read.
 * \param off Offset
 * \param res Result (may be NULL, e.g. to test if the given offset is readable)
 *
 * \return true on success, false on failure and sets errno accordingly.
 **/
bool
pink_util_peekdata(pid_t pid, long off, long *res);

/**
 * On FreeBSD this copies the given single int val to the given offset in the
 * traced process's instruction space, aka PT_WRITE_I.
 *
 * On Linux this copies the word val to the given offset in the child's USER
 * area, aka PTRACE_POKEUSER.
 *
 * \note Mostly for internal use, use higher level functions where possible.
 *
 * \param pid Process ID of the child.
 * \param off Offset
 * \param val Word
 *
 * \return true on success, false on failure and sets errno accordingly.
 **/
bool
pink_util_poke(pid_t pid, long off, long val);

/**
 * On FreeBSD this copies the given single int val to the given offset in the
 * traced process's data space, aka PT_WRITE_D.
 *
 * On Linux this copies the word val to location addr in the child's memory,
 * aka PTRACE_PEEKDATA.
 *
 * \note Mostly for internal use, use higher level functions where possible.
 *
 * \param pid Process ID of the child.
 * \param off Offset
 * \param val Word
 *
 * \return true on success, false on failure and sets errno accordingly.
 **/
bool
pink_util_pokedata(pid_t pid, long off, long val);

/**
 * Copy the child's general purpose registers to the given location.
 *
 * \param pid Process ID of the child.
 * \param regs Pointer to the structure of registers. On FreeBSD this is
 * "struct reg" defined in <machine/reg.h>, on Linux this is "struct pt_regs"
 * defined in <asm/ptrace.h>.
 *
 * \return true on success, false on failure and sets errno accordingly.
 **/
bool
pink_util_get_regs(pid_t pid, void *regs);

/**
 * Set the child's general purpose registers.
 *
 * \param pid Process ID of the child.
 * \param regs Same as pink_util_get_regs()
 *
 * \return true on success, false on failure and sets errno accordingly.
 **/
bool
pink_util_set_regs(pid_t pid, const void *regs);

/**
 * Move len bytes of data of process pid, at address addr, to our address space
 * dest.
 *
 * \note Mostly for internal use, use higher level functions where possible.
 *
 * \param pid Process ID of the child.
 * \param addr Address where the data is to be moved from.
 * \param dest Pointer to store the data.
 * \param len Number of bytes of data to move.
 *
 * \return true on success, false on failure and sets errno accordingly.
 **/
bool
pink_util_moven(pid_t pid, long addr, char *dest, size_t len);

/**
 * Convenience macro to read an object
 *
 * \note Mostly for internal use, use higher level functions where possible.
 *
 * \see pink_util_moven
 **/
#define pink_util_move(pid, addr, objp) \
	pink_util_moven((pid), (addr), (char *)(objp), sizeof *(objp))

/**
 * Like pink_util_moven() but make the additional effort of looking for a
 * terminating zero-byte.
 *
 * \note On FreeBSD this function is equivalent to pink_util_moven().
 * \note Mostly for internal use, use higher level functions where possible.
 **/
bool
pink_util_movestr(pid_t pid, long addr, char *dest, size_t len);

/**
 * Like pink_util_movestr() but allocates the string itself.
 *
 * \note Mostly for internal use, use higher level functions where possible.
 *
 * \return The string on success and NULL on failure and sets errno
 * accordingly.
 **/
char *
pink_util_movestr_persistent(pid_t pid, long addr);

/**
 * Copy len bytes of data to process pid, at address addr, from our address space
 * src.
 *
 * \note Mostly for internal use, use higher level functions where possible.
 *
 * \param pid Process ID of the child being traced
 * \param addr Address where the data is to be copied to
 * \param src Pointer to the data to be moved
 * \param len Length of data
 *
 * \return true on success, false on failure and sets errno accordingly.
 **/
bool
pink_util_putn(pid_t pid, long addr, const char *src, size_t len);

/**
 * Convenience macro to write an object
 *
 * \note Mostly for internal use, use higher level functions where possible.
 *
 * \see pink_util_putn
 **/
#define pink_util_put(pid, addr, objp) \
	pink_util_putn((pid), (addr), (const char *)(objp), sizeof *(objp))

#if defined(PINKTRACE_LINUX) || defined(DOXYGEN)
/**
 * Like pink_util_putn() but make the additional effort not to overwrite
 * unreadable addresses. Use this e.g. to write strings safely.
 *
 * \note Availability: Linux
 * \note Mostly for internal use, use higher level functions where possible.
 *
 * \param pid Process ID of the child being traced
 * \param addr Address where the data is to be copied to
 * \param src Pointer to the data to be moved
 * \param len Length of data
 *
 * \return true on success, false on failure and sets errno accordingly.
 **/
bool
pink_util_putn_safe(pid_t pid, long addr, const char *src, size_t len);

/**
 * Convenience macro to write an object safely
 *
 * \note Availability: Linux
 * \note Mostly for internal use, use higher level functions where possible.
 *
 * \see pink_util_putn_safe
 **/
#define pink_util_put_safe(pid, addr, objp) \
	pink_util_putn_safe((pid), (addr), (const char *)(objp), sizeof *(objp))

#endif /* defined(PINKTRACE_LINUX)... */

/**
 * Gets the last system call called by child with the given process ID.
 *
 * \param pid Process ID of the child whose system call is to be returned.
 * \param bitness Bitness of the child
 * \param res Pointer to store the result.
 *
 * \return true on success, false on failure and sets errno accordingly.
 **/
bool
pink_util_get_syscall(pid_t pid, pink_bitness_t bitness, long *res);

/**
 * Sets the system call to the given value.
 *
 * \param pid Process ID of the child whose system call is to be set.
 * \param bitness Bitness of the child
 * \param scno System call to set.
 *
 * \return true on success, false on failure and sets errno accordingly.
 **/
bool
pink_util_set_syscall(pid_t pid, pink_bitness_t bitness, long scno);

/**
 * Gets the return value of the last system call called by child with the given
 * process ID.
 *
 * \param pid Process ID of the child whose system call return value is to be
 * returned.
 * \param res Pointer to store the result.
 *
 * \return true on success, false on failure and sets errno accordingly.
 **/
pink_nonnull(2)
bool
pink_util_get_return(pid_t pid, long *res);

/**
 * Sets the return value of the last system call called by child with the given
 * process ID.
 *
 * \param pid Process ID of the child whose system call return value is to be
 * modified.
 * \param ret Return value to set.
 *
 * \return true on success, false on failure and sets errno accordingly.
 **/
bool
pink_util_set_return(pid_t pid, long ret);

/**
 * Get the given argument and place it in res.
 *
 * \param pid Process ID of the child whose argument is to be received.
 * \param bitness Bitness of the child
 * \param ind The index of the argument (0-5, see #PINK_MAX_INDEX)
 * \param res Pointer to store the argument
 *
 * \return true on success, false on failure and sets errno accordingly.
 **/
pink_nonnull(4)
bool
pink_util_get_arg(pid_t pid, pink_bitness_t bitness, unsigned ind, long *res);

#endif /* !PINKTRACE_GUARD_UTIL_H */
