/*
 * SPDX-License-Identifier: APSL-2.0
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * https://opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 *
 * CARIBEOS MODIFICATION NOTICE
 * This file was created as part of the experimental CaribeOS RV32IMACSU
 * port of Apple XNU 2050.48.11 on 2026-07-14.
 */

#line 1
#include <machine/linux_syscall.h>
#include <machine/bootinfo.h>
#include <machine/copyio.h>
#include <machine/cpu_data.h>
#include <machine/early_print.h>
#include <machine/linux_exec.h>
#include <machine/linux_initrd.h>
#include <machine/linux_process.h>
#include <machine/linux_signal.h>
#include <machine/pmap.h>
#include <machine/tty.h>
#include <mach/mach_time.h>
#include <kern/clock.h>
#include <kern/processor.h>
#include <kern/sched_prim.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/zalloc.h>
#include <vm/pmap.h>
#include <vm/vm_page.h>
#include <string.h>

#define RISCV32_LINUX_SYS_GETCWD	17U
#define RISCV32_LINUX_SYS_EVENTFD2	19U
#define RISCV32_LINUX_SYS_EPOLL_CREATE1 20U
#define RISCV32_LINUX_SYS_EPOLL_CTL	21U
#define RISCV32_LINUX_SYS_EPOLL_PWAIT	22U
#define RISCV32_LINUX_SYS_DUP		23U
#define RISCV32_LINUX_SYS_DUP3		24U
#define RISCV32_LINUX_SYS_FCNTL	25U
#define RISCV32_LINUX_SYS_INOTIFY_INIT1 26U
#define RISCV32_LINUX_SYS_INOTIFY_ADD_WATCH 27U
#define RISCV32_LINUX_SYS_INOTIFY_RM_WATCH 28U
#define RISCV32_LINUX_SYS_IOCTL	29U
#define RISCV32_LINUX_SYS_MKNODAT	33U
#define RISCV32_LINUX_SYS_MKDIRAT	34U
#define RISCV32_LINUX_SYS_UNLINKAT	35U
#define RISCV32_LINUX_SYS_SYMLINKAT	36U
#define RISCV32_LINUX_SYS_LINKAT	37U
#define RISCV32_LINUX_SYS_RENAMEAT	38U
#define RISCV32_LINUX_SYS_STATFS64	43U
#define RISCV32_LINUX_SYS_FSTATFS64	44U
#define RISCV32_LINUX_SYS_FTRUNCATE64	46U
#define RISCV32_LINUX_SYS_FACCESSAT	48U
#define RISCV32_LINUX_SYS_CHDIR	49U
#define RISCV32_LINUX_SYS_FCHDIR	50U
#define RISCV32_LINUX_SYS_FCHMODAT	53U
#define RISCV32_LINUX_SYS_FCHOWNAT	54U
#define RISCV32_LINUX_SYS_OPENAT	56U
#define RISCV32_LINUX_SYS_CLOSE	57U
#define RISCV32_LINUX_SYS_PIPE2	59U
#define RISCV32_LINUX_SYS_GETDENTS64	61U
#define RISCV32_LINUX_SYS_LLSEEK	62U
#define RISCV32_LINUX_SYS_READ	63U
#define RISCV32_LINUX_SYS_WRITE	64U
#define RISCV32_LINUX_SYS_READV	65U
#define RISCV32_LINUX_SYS_WRITEV	66U
#define RISCV32_LINUX_SYS_PREAD64	67U
#define RISCV32_LINUX_SYS_PSELECT6	72U
#define RISCV32_LINUX_SYS_PPOLL	73U
#define RISCV32_LINUX_SYS_READLINKAT	78U
#define RISCV32_LINUX_SYS_FSTATAT64	79U
#define RISCV32_LINUX_SYS_FSTAT64	80U
#define RISCV32_LINUX_SYS_SYNC		81U
#define RISCV32_LINUX_SYS_FSYNC	82U
#define RISCV32_LINUX_SYS_FDATASYNC	83U
#define RISCV32_LINUX_SYS_TIMERFD_CREATE 85U
#define RISCV32_LINUX_SYS_TIMERFD_SETTIME 86U
#define RISCV32_LINUX_SYS_TIMERFD_GETTIME 87U
#define RISCV32_LINUX_SYS_UTIMENSAT	88U
#define RISCV32_LINUX_SYS_CAPGET	90U
#define RISCV32_LINUX_SYS_CAPSET	91U
#define RISCV32_LINUX_SYS_PERSONALITY	92U
#define RISCV32_LINUX_SYS_EXIT		93U
#define RISCV32_LINUX_SYS_EXIT_GROUP	94U
#define RISCV32_LINUX_SYS_WAITID	95U
#define RISCV32_LINUX_SYS_SET_TID_ADDRESS 96U
#define RISCV32_LINUX_SYS_FUTEX	98U
#define RISCV32_LINUX_SYS_SET_ROBUST_LIST 99U
#define RISCV32_LINUX_SYS_GET_ROBUST_LIST 100U
#define RISCV32_LINUX_SYS_NANOSLEEP	101U
#define RISCV32_LINUX_SYS_GETITIMER	102U
#define RISCV32_LINUX_SYS_SETITIMER	103U
#define RISCV32_LINUX_SYS_CLOCK_GETTIME	113U
#define RISCV32_LINUX_SYS_CLOCK_GETRES	114U
#define RISCV32_LINUX_SYS_CLOCK_NANOSLEEP 115U
#define RISCV32_LINUX_SYS_SCHED_SETAFFINITY 122U
#define RISCV32_LINUX_SYS_SCHED_GETAFFINITY 123U
#define RISCV32_LINUX_SYS_SCHED_YIELD	124U
#define RISCV32_LINUX_SYS_KILL		129U
#define RISCV32_LINUX_SYS_TKILL	130U
#define RISCV32_LINUX_SYS_TGKILL	131U
#define RISCV32_LINUX_SYS_SIGALTSTACK	132U
#define RISCV32_LINUX_SYS_RT_SIGACTION	134U
#define RISCV32_LINUX_SYS_RT_SIGPROCMASK 135U
#define RISCV32_LINUX_SYS_RT_SIGPENDING	136U
#define RISCV32_LINUX_SYS_RT_SIGRETURN	139U
#define RISCV32_LINUX_SYS_SETPRIORITY	140U
#define RISCV32_LINUX_SYS_GETPRIORITY	141U
#define RISCV32_LINUX_SYS_SETREGID	143U
#define RISCV32_LINUX_SYS_SETGID	144U
#define RISCV32_LINUX_SYS_SETREUID	145U
#define RISCV32_LINUX_SYS_SETUID	146U
#define RISCV32_LINUX_SYS_SETRESUID	147U
#define RISCV32_LINUX_SYS_GETRESUID	148U
#define RISCV32_LINUX_SYS_SETRESGID	149U
#define RISCV32_LINUX_SYS_GETRESGID	150U
#define RISCV32_LINUX_SYS_SETFSUID	151U
#define RISCV32_LINUX_SYS_SETFSGID	152U
#define RISCV32_LINUX_SYS_TIMES	153U
#define RISCV32_LINUX_SYS_SETPGID	154U
#define RISCV32_LINUX_SYS_GETPGID	155U
#define RISCV32_LINUX_SYS_GETSID	156U
#define RISCV32_LINUX_SYS_SETSID	157U
#define RISCV32_LINUX_SYS_GETGROUPS	158U
#define RISCV32_LINUX_SYS_SETGROUPS	159U
#define RISCV32_LINUX_SYS_UNAME	160U
#define RISCV32_LINUX_SYS_GETRUSAGE	165U
#define RISCV32_LINUX_SYS_UMASK	166U
#define RISCV32_LINUX_SYS_PRCTL	167U
#define RISCV32_LINUX_SYS_GETCPU	168U
#define RISCV32_LINUX_SYS_GETTIMEOFDAY	169U
#define RISCV32_LINUX_SYS_GETPPID	173U
#define RISCV32_LINUX_SYS_GETPID	172U
#define RISCV32_LINUX_SYS_GETUID	174U
#define RISCV32_LINUX_SYS_GETEUID	175U
#define RISCV32_LINUX_SYS_GETGID	176U
#define RISCV32_LINUX_SYS_GETEGID	177U
#define RISCV32_LINUX_SYS_GETTID	178U
#define RISCV32_LINUX_SYS_SYSINFO	179U
#define RISCV32_LINUX_SYS_SOCKET	198U
#define RISCV32_LINUX_SYS_SOCKETPAIR	199U
#define RISCV32_LINUX_SYS_BIND	200U
#define RISCV32_LINUX_SYS_LISTEN	201U
#define RISCV32_LINUX_SYS_ACCEPT	202U
#define RISCV32_LINUX_SYS_CONNECT	203U
#define RISCV32_LINUX_SYS_GETSOCKNAME	204U
#define RISCV32_LINUX_SYS_GETPEERNAME	205U
#define RISCV32_LINUX_SYS_SENDTO	206U
#define RISCV32_LINUX_SYS_RECVFROM	207U
#define RISCV32_LINUX_SYS_SETSOCKOPT	208U
#define RISCV32_LINUX_SYS_GETSOCKOPT	209U
#define RISCV32_LINUX_SYS_SHUTDOWN	210U
#define RISCV32_LINUX_SYS_SENDMSG	211U
#define RISCV32_LINUX_SYS_RECVMSG	212U
#define RISCV32_LINUX_SYS_BRK		214U
#define RISCV32_LINUX_SYS_MUNMAP	215U
#define RISCV32_LINUX_SYS_MREMAP	216U
#define RISCV32_LINUX_SYS_CLONE	220U
#define RISCV32_LINUX_SYS_EXECVE	221U
#define RISCV32_LINUX_SYS_MMAP		222U
#define RISCV32_LINUX_SYS_MPROTECT	226U
#define RISCV32_LINUX_SYS_MSYNC	227U
#define RISCV32_LINUX_SYS_MLOCK	228U
#define RISCV32_LINUX_SYS_MUNLOCK	229U
#define RISCV32_LINUX_SYS_MLOCKALL	230U
#define RISCV32_LINUX_SYS_MUNLOCKALL	231U
#define RISCV32_LINUX_SYS_MINCORE	232U
#define RISCV32_LINUX_SYS_MADVISE	233U
#define RISCV32_LINUX_SYS_ACCEPT4	242U
#define RISCV32_LINUX_SYS_RECVMMSG	243U
#define RISCV32_LINUX_SYS_WAIT4	260U
#define RISCV32_LINUX_SYS_PRLIMIT64	261U
#define RISCV32_LINUX_SYS_SYNCFS	267U
#define RISCV32_LINUX_SYS_SENDMMSG	269U
#define RISCV32_LINUX_SYS_RENAMEAT2	276U
#define RISCV32_LINUX_SYS_GETRANDOM	278U
#define RISCV32_LINUX_SYS_EXECVEAT	281U
#define RISCV32_LINUX_SYS_MLOCK2	284U
#define RISCV32_LINUX_SYS_MEMBARRIER	283U
#define RISCV32_LINUX_SYS_STATX	291U
#define RISCV32_LINUX_SYS_RSEQ		293U
#define RISCV32_LINUX_SYS_CLOCK_GETTIME64 403U
#define RISCV32_LINUX_SYS_CLOCK_GETRES_TIME64 406U
#define RISCV32_LINUX_SYS_CLOCK_NANOSLEEP_TIME64 407U
#define RISCV32_LINUX_SYS_TIMERFD_GETTIME64 410U
#define RISCV32_LINUX_SYS_TIMERFD_SETTIME64 411U
#define RISCV32_LINUX_SYS_UTIMENSAT_TIME64 412U
#define RISCV32_LINUX_SYS_PSELECT6_TIME64 413U
#define RISCV32_LINUX_SYS_PPOLL_TIME64 414U
#define RISCV32_LINUX_SYS_RECVMMSG_TIME64 417U
#define RISCV32_LINUX_SYS_FUTEX_TIME64	422U
#define RISCV32_LINUX_SYS_EPOLL_PWAIT2	441U
#define RISCV32_LINUX_SYS_FCHMODAT2	452U
#define RISCV32_LINUX_E2BIG		7U
#define RISCV32_LINUX_ENOENT	2U
#define RISCV32_LINUX_ESRCH		3U
#define RISCV32_LINUX_EINTR		4U
#define RISCV32_LINUX_EBADF		9U
#define RISCV32_LINUX_ECHILD	10U
#define RISCV32_LINUX_EAGAIN	11U
#define RISCV32_LINUX_ENOMEM	12U
#define RISCV32_LINUX_EACCES	13U
#define RISCV32_LINUX_ENOEXEC	8U
#define RISCV32_LINUX_ENOSYS	38U
#define RISCV32_LINUX_EFAULT	14U
#define RISCV32_LINUX_EEXIST	17U
#define RISCV32_LINUX_ENOTDIR	20U
#define RISCV32_LINUX_EISDIR	21U
#define RISCV32_LINUX_EINVAL	22U
#define RISCV32_LINUX_EMFILE	24U
#define RISCV32_LINUX_ENOTTY	25U
#define RISCV32_LINUX_ESPIPE	29U
#define RISCV32_LINUX_EROFS	30U
#define RISCV32_LINUX_EPIPE	32U
#define RISCV32_LINUX_ENAMETOOLONG 36U
#define RISCV32_LINUX_ERANGE	34U
#define RISCV32_LINUX_ENOTSOCK	88U
#define RISCV32_LINUX_EAFNOSUPPORT 97U
#define RISCV32_LINUX_AT_SYMLINK_NOFOLLOW 0x100U
#define RISCV32_LINUX_AT_EMPTY_PATH	0x1000U
#define RISCV32_LINUX_AT_FDCWD	0xffffff9cU
#define RISCV32_LINUX_DEV_NULL		0x0103U
#define RISCV32_LINUX_DEV_TTY		0x0500U
#define RISCV32_LINUX_DEV_CONSOLE	0x0501U
#define RISCV32_LINUX_FD_FIRST	3U
#define RISCV32_LINUX_FD_MAX	16U
#define RISCV32_LINUX_OFD_MAX	128U
#define RISCV32_LINUX_OFD_MAGIC	0x434f4644U
#define RISCV32_LINUX_FD_NONE	0U
#define RISCV32_LINUX_FD_INITRD	1U
#define RISCV32_LINUX_FD_CONSOLE	2U
#define RISCV32_LINUX_FD_DIR	3U
#define RISCV32_LINUX_FD_PIPE	4U
#define RISCV32_LINUX_FD_EVENTFD	5U
#define RISCV32_LINUX_FD_EPOLL	6U
#define RISCV32_LINUX_FD_TIMERFD	7U
#define RISCV32_LINUX_FD_INOTIFY	8U
#define RISCV32_LINUX_FD_PROC_VERSION	9U
#define RISCV32_LINUX_FD_PROC_CPUINFO	10U
#define RISCV32_LINUX_FD_PROC_MEMINFO	11U
#define RISCV32_LINUX_FD_PROC_CARIBEOS	12U
#define RISCV32_LINUX_FD_PROC_MOUNTS	13U
#define RISCV32_LINUX_FD_PROC_FILESYSTEMS 14U
#define RISCV32_LINUX_FD_PROC_BOOTLOG	15U
#define RISCV32_LINUX_FD_PROC_IOKIT	16U
#define RISCV32_LINUX_FD_PROC_CARIBE_SBI 17U
#define RISCV32_LINUX_FD_PROC_CARIBE_DEVICES 18U
#define RISCV32_LINUX_FD_PROC_CARIBE_MEMMAP 19U
#define RISCV32_LINUX_FD_PROC_SELF_MAPS 20U
#define RISCV32_LINUX_FD_PROC_SELF_STATUS 21U
#define RISCV32_LINUX_FD_PROC_SELF_AUXV 22U
#define RISCV32_LINUX_FD_PROC_SELF_CMDLINE 23U
#define RISCV32_LINUX_FD_PROC_SELF_ENVIRON 24U
#define RISCV32_LINUX_FD_PROC_UPTIME 25U
#define RISCV32_LINUX_FD_PROC_STAT 26U
#define RISCV32_LINUX_FD_NULL	27U
#define RISCV32_LINUX_FD_PROC_SELF_FDINFO 28U
#define RISCV32_LINUX_FD_PROC_CARIBE_RESOURCES 29U
#define RISCV32_IOKIT_STAGE0_CPP_DEFERRED 0x00000008U
#define RISCV32_IOKIT_CPP_PROBE_LINKED	0x00000100U
#define RISCV32_IOKIT_CPP_PROBE_CTOR	0x00000200U
#define RISCV32_IOKIT_CPP_PROBE_VTABLE	0x00000400U
#define RISCV32_IOKIT_CPP_PROBE_SERVICE_SHAPE 0x00000800U
#define RISCV32_IOKIT_CPP_MICRO_REGISTRY 0x00001000U
#define RISCV32_IOKIT_CPP_MICRO_PUBLISH	0x00002000U
#define RISCV32_IOKIT_CPP_MICRO_START	0x00004000U
#define RISCV32_IOKIT_CPP_MICRO_MATCH	0x00008000U
#define RISCV32_IOKIT_CPP_REAL_STARTIOKIT 0x00010000U
#define RISCV32_IOKIT_FDT_LEGACY_SKIPPED 0x00020000U
#define RISCV32_LINUX_EPOLL_CTL_ADD	1U
#define RISCV32_LINUX_EPOLL_CTL_DEL	2U
#define RISCV32_LINUX_EPOLL_CTL_MOD	3U
#define RISCV32_LINUX_EPOLL_EVENT_SIZE	12U
#define RISCV32_LINUX_EPOLL_MAX_EVENTS	32U
#define RISCV32_LINUX_FUTEX_CMD_MASK	0x7fU
#define RISCV32_LINUX_FUTEX_WAIT	0U
#define RISCV32_LINUX_FUTEX_WAKE	1U
#define RISCV32_LINUX_ROBUST_LIST_HEAD_SIZE 12U
#define RISCV32_LINUX_ITIMER_MAX	2U
#define RISCV32_LINUX_ITIMERVAL_SIZE	16U
#define RISCV32_LINUX_SIGSET_MAX_SIZE	16U
#define RISCV32_LINUX_SIGALTSTACK_SIZE	12U
#define RISCV32_LINUX_SIGACTION_HEAD_SIZE 12U
#define RISCV32_LINUX_SIGACTION_MAX_SIZE \
	(RISCV32_LINUX_SIGACTION_HEAD_SIZE + RISCV32_LINUX_SIGSET_MAX_SIZE)
#define RISCV32_LINUX_SIG_MAX	64U
#define RISCV32_LINUX_SIG_BLOCK	0U
#define RISCV32_LINUX_SIG_UNBLOCK	1U
#define RISCV32_LINUX_SIG_SETMASK	2U
#define RISCV32_LINUX_WNOHANG	1U
#define RISCV32_LINUX_WSTOPPED	2U
#define RISCV32_LINUX_WEXITED	4U
#define RISCV32_LINUX_WCONTINUED 8U
#define RISCV32_LINUX_WNOWAIT	0x01000000U
#define RISCV32_LINUX_P_ALL		0U
#define RISCV32_LINUX_P_PID		1U
#define RISCV32_LINUX_P_PGID	2U
#define RISCV32_LINUX_SIGCHLD	17U
#define RISCV32_LINUX_SIGCONT	18U
#define RISCV32_LINUX_SIGPIPE	13U
#define RISCV32_LINUX_CLD_EXITED	1U
#define RISCV32_LINUX_CLD_KILLED	2U
#define RISCV32_LINUX_CLD_STOPPED	5U
#define RISCV32_LINUX_CLD_CONTINUED 6U
#define RISCV32_LINUX_CLONE_VM	0x00000100U
#define RISCV32_LINUX_CLONE_FS	0x00000200U
#define RISCV32_LINUX_CLONE_FILES	0x00000400U
#define RISCV32_LINUX_CLONE_SIGHAND	0x00000800U
#define RISCV32_LINUX_CLONE_THREAD	0x00010000U
#define RISCV32_LINUX_PR_GET_DUMPABLE	3U
#define RISCV32_LINUX_PR_SET_DUMPABLE	4U
#define RISCV32_LINUX_PR_SET_NAME	15U
#define RISCV32_LINUX_PR_GET_NAME	16U
#define RISCV32_LINUX_TASK_NAME_LEN	16U
#define RISCV32_LINUX_PRIO_PROCESS	0U
#define RISCV32_LINUX_PRIO_PGRP	1U
#define RISCV32_LINUX_PRIO_USER	2U
#define RISCV32_LINUX_CAP_VERSION_3	0x20080522U
#define RISCV32_LINUX_ID_NO_CHANGE	UINT32_MAX
#define RISCV32_LINUX_PERSONALITY_QUERY	UINT32_MAX
#define RISCV32_LINUX_PERSONALITY_LINUX	0U
#define RISCV32_LINUX_NGROUPS_MAX	32U
#define RISCV32_LINUX_RLIMIT_CPU	0U
#define RISCV32_LINUX_RLIMIT_FSIZE	1U
#define RISCV32_LINUX_RLIMIT_DATA	2U
#define RISCV32_LINUX_RLIMIT_STACK	3U
#define RISCV32_LINUX_RLIMIT_CORE	4U
#define RISCV32_LINUX_RLIMIT_NOFILE	7U
#define RISCV32_LINUX_RLIMIT_AS		9U
#define RISCV32_LINUX_RLIMIT_NLIMITS	16U
#define RISCV32_LINUX_DIR_ROOT	1U
#define RISCV32_LINUX_DIR_DEV	2U
#define RISCV32_LINUX_DIR_PROC	3U
#define RISCV32_LINUX_DIR_PROC_SELF	4U
#define RISCV32_LINUX_DIR_ETC	5U
#define RISCV32_LINUX_DIR_CARIBE	6U
#define RISCV32_LINUX_DIR_BIN	7U
#define RISCV32_LINUX_DIR_INITRD	8U
#define RISCV32_LINUX_DIR_PROC_CARIBE	9U
#define RISCV32_LINUX_DIR_DEV_FD	10U
#define RISCV32_LINUX_DIR_PROC_SELF_FD	11U
#define RISCV32_LINUX_DIR_PROC_SELF_FDINFO 12U
#define RISCV32_LINUX_DT_CHR	2U
#define RISCV32_LINUX_DT_DIR	4U
#define RISCV32_LINUX_DT_REG	8U
#define RISCV32_LINUX_DT_LNK	10U
#define RISCV32_LINUX_MODE_IFIFO	0010000U
#define RISCV32_LINUX_MODE_IFCHR	0020000U
#define RISCV32_LINUX_MODE_IFDIR	0040000U
#define RISCV32_LINUX_MODE_IFMT	0170000U
#define RISCV32_LINUX_MODE_IFREG	0100000U
#define RISCV32_LINUX_MODE_IFLNK	0120000U
#define RISCV32_LINUX_MODE_0666	0666U
#define RISCV32_LINUX_MODE_0755	0755U
#define RISCV32_LINUX_PROT_READ	1U
#define RISCV32_LINUX_PROT_WRITE	2U
#define RISCV32_LINUX_PROT_EXEC	4U
#define RISCV32_LINUX_MAP_SHARED	0x01U
#define RISCV32_LINUX_MAP_PRIVATE	0x02U
#define RISCV32_LINUX_MAP_FIXED	0x10U
#define RISCV32_LINUX_MAP_ANONYMOUS	0x20U
#define RISCV32_LINUX_MAP_FIXED_NOREPLACE 0x100000U
#define RISCV32_LINUX_VMA_SOURCE_ANON	1U
#define RISCV32_LINUX_VMA_SOURCE_INITRD	2U
#define RISCV32_LINUX_O_ACCMODE	3U
#define RISCV32_LINUX_O_RDONLY	0U
#define RISCV32_LINUX_O_WRONLY	1U
#define RISCV32_LINUX_O_RDWR		2U
#define RISCV32_LINUX_O_NONBLOCK	04000U
#define RISCV32_LINUX_O_CLOEXEC	02000000U
#define RISCV32_LINUX_F_DUPFD	0U
#define RISCV32_LINUX_F_GETFD	1U
#define RISCV32_LINUX_F_SETFD	2U
#define RISCV32_LINUX_F_GETFL	3U
#define RISCV32_LINUX_F_SETFL	4U
#define RISCV32_LINUX_F_DUPFD_CLOEXEC	1030U
#define RISCV32_LINUX_FD_CLOEXEC	1U
#define RISCV32_LINUX_TCGETS	0x5401U
#define RISCV32_LINUX_TCSETS	0x5402U
#define RISCV32_LINUX_TCSETSW	0x5403U
#define RISCV32_LINUX_TCSETSF	0x5404U
#define RISCV32_LINUX_TCGETA	0x5405U
#define RISCV32_LINUX_TCSETA	0x5406U
#define RISCV32_LINUX_TCSETAW	0x5407U
#define RISCV32_LINUX_TCSETAF	0x5408U
#define RISCV32_LINUX_TIOCGPGRP	0x540fU
#define RISCV32_LINUX_TIOCSPGRP	0x5410U
#define RISCV32_LINUX_TIOCGWINSZ	0x5413U
#define RISCV32_LINUX_TIOCSWINSZ	0x5414U
#define RISCV32_LINUX_FIONREAD	0x541bU
#define RISCV32_LINUX_TIOCNOTTY	0x5422U
#define RISCV32_LINUX_TIOCGSID	0x5429U
#define RISCV32_LINUX_TERMIOS_SIZE	36U
#define RISCV32_LINUX_TERMIO_SIZE	18U
#define RISCV32_LINUX_WINSIZE_SIZE	8U
#define RISCV32_LINUX_SEEK_SET	0U
#define RISCV32_LINUX_SEEK_CUR	1U
#define RISCV32_LINUX_SEEK_END	2U
#define RISCV32_LINUX_STDIN	0U
#define RISCV32_LINUX_STDOUT	1U
#define RISCV32_LINUX_STDERR	2U
#define RISCV32_LINUX_STDIO_COUNT	3U
#define RISCV32_LINUX_IOVEC_SIZE	8U
#define RISCV32_LINUX_IOV_MAX	16U
#define RISCV32_LINUX_POLLFD_SIZE	8U
#define RISCV32_LINUX_POLLFD_MAX	32U
#define RISCV32_LINUX_POLLIN	0x0001U
#define RISCV32_LINUX_POLLOUT	0x0004U
#define RISCV32_LINUX_POLLERR	0x0008U
#define RISCV32_LINUX_POLLHUP	0x0010U
#define RISCV32_LINUX_POLLNVAL	0x0020U
#define RISCV32_LINUX_SELECT_MAX_FDS	1024U
#define RISCV32_LINUX_SELECT_FDSET_BYTES \
	(RISCV32_LINUX_SELECT_MAX_FDS / 8U)
#define RISCV32_LINUX_DIRENT64_BASE_SIZE 19U
#define RISCV32_LINUX_DIRENT64_MAX_SIZE 128U
#define RISCV32_LINUX_PATH_MAX	128U
#define RISCV32_LINUX_STAT64_SIZE	104U
#define RISCV32_LINUX_STATX_SIZE	256U
#define RISCV32_LINUX_STATX_BASIC_STATS	0x07ffU
#define RISCV32_LINUX_STATFS64_SIZE	88U
#define RISCV32_LINUX_HFSPLUS_SUPER_MAGIC 0x482bU
#define RISCV32_LINUX_SYSINFO_SIZE	64U
#define RISCV32_LINUX_UTSNAME_FIELD_LEN 65U
#define RISCV32_LINUX_UTSNAME_FIELD_COUNT 6U
#define RISCV32_LINUX_UTSNAME_SIZE \
	(RISCV32_LINUX_UTSNAME_FIELD_LEN * RISCV32_LINUX_UTSNAME_FIELD_COUNT)
#define RISCV32_LINUX_READ_MAX	4096U
#define RISCV32_LINUX_WRITE_MAX	4096U
#define RISCV32_LINUX_WRITE_CHUNK	128U
#define RISCV32_LINUX_COPY_CHUNK	128U
#define RISCV32_LINUX_PIPE_MAX	8U
#define RISCV32_LINUX_PIPE_BUF	4096U
#define RISCV32_LINUX_GETRANDOM_MAX	256U
#define RISCV32_LINUX_RUSAGE_SIZE	72U
#define RISCV32_LINUX_SIGINFO_SIZE	128U
#define RISCV32_LINUX_CPUSET_WORD_SIZE	4U
#define RISCV32_LINUX_BOOT_RAM_SIZE	(256U * 1024U * 1024U)
#define RISCV32_LINUX_MMAP_BASE	0x60000000U
#define RISCV32_LINUX_MMAP_PAGE_SIZE	4096U
#define RISCV32_LINUX_MMAP_MAX_PAGES	256U
#define RISCV32_LINUX_BRK_BASE		0x61000000U
#define RISCV32_LINUX_BRK_MAX_PAGES	512U
#define RISCV32_LINUX_NSEC_PER_SEC	1000000000ULL
#define RISCV32_LINUX_CLK_TCK	100ULL

typedef struct riscv32_linux_open_file_description {
	uint32_t magic;
	uint32_t pool_id;
	uint32_t kind;
	uint32_t offset;
	uint32_t accmode;
	uint32_t status_flags;
	uint32_t descriptor_flags;
	uint32_t dir_kind;
	uint64_t value;
	riscv32_linux_initrd_file_t file;
	char dir_path[RISCV32_LINUX_PROCESS_PATH_MAX];
} riscv32_linux_fd_t;

typedef struct riscv32_linux_pipe {
	uint32_t used;
	uint32_t id;
	uint32_t read_pos;
	uint32_t size;
	uint32_t readers;
	uint32_t writers;
	uint32_t read_sequence;
	uint32_t write_sequence;
	uint32_t read_blocks;
	uint32_t read_wakeups;
	uint32_t write_blocks;
	uint32_t write_wakeups;
	uint32_t bytes_read;
	uint32_t bytes_written;
	uint8_t data[RISCV32_LINUX_PIPE_BUF];
} riscv32_linux_pipe_t;

typedef riscv32_linux_process_sigaction_t riscv32_linux_sigaction_t;

typedef riscv32_linux_process_mmap_page_t riscv32_linux_mmap_page_t;

static riscv32_linux_pipe_t riscv32_linux_pipes[RISCV32_LINUX_PIPE_MAX];
static simple_lock_data_t riscv32_linux_pipe_locks[RISCV32_LINUX_PIPE_MAX];
static decl_simple_lock_data(, riscv32_linux_pipe_pool_lock)
static volatile uint32_t riscv32_linux_pipe_gate12_read_block_logged;
static volatile uint32_t riscv32_linux_pipe_gate12_write_block_logged;
static volatile uint32_t riscv32_linux_pipe_gate12_eof_logged;
static volatile uint32_t riscv32_linux_pipe_gate12_sigpipe_logged;
static volatile uint32_t riscv32_linux_pipe_gate12_completed;
static riscv32_linux_pipe_stats_t riscv32_linux_pipe_stats;
static riscv32_linux_fd_t
    riscv32_linux_ofd_pool[RISCV32_LINUX_OFD_MAX];
static uint8_t riscv32_linux_ofd_used[RISCV32_LINUX_OFD_MAX];
static volatile uint32_t
    riscv32_linux_ofd_references[RISCV32_LINUX_OFD_MAX];
static simple_lock_data_t riscv32_linux_ofd_locks[RISCV32_LINUX_OFD_MAX];
static decl_simple_lock_data(, riscv32_linux_ofd_pool_lock)
static volatile uint32_t riscv32_linux_ofd_init_state;
static riscv32_linux_ofd_stats_t riscv32_linux_ofd_stats;
static uint8_t riscv32_linux_mmap_pages[RISCV32_LINUX_MMAP_MAX_PAGES]
    [RISCV32_LINUX_MMAP_PAGE_SIZE]
    __attribute__((aligned(RISCV32_LINUX_MMAP_PAGE_SIZE)));
static uint8_t riscv32_linux_brk_pages[RISCV32_LINUX_BRK_MAX_PAGES]
    [RISCV32_LINUX_MMAP_PAGE_SIZE]
    __attribute__((aligned(RISCV32_LINUX_MMAP_PAGE_SIZE)));
static char riscv32_linux_proc_version[384];
static char riscv32_linux_proc_cpuinfo[4096];
static char riscv32_linux_proc_meminfo[256];
static char riscv32_linux_proc_uptime[128];
static char riscv32_linux_proc_stat[1024];
static char riscv32_linux_proc_caribeos[4096];
static char riscv32_linux_proc_bootlog[8192];
static char riscv32_linux_proc_iokit[12288];
static char riscv32_linux_proc_caribe_sbi[4096];
static char riscv32_linux_proc_caribe_devices[4096];
static char riscv32_linux_proc_caribe_memmap[8192];
static char riscv32_linux_proc_caribe_resources[4096];
#define RISCV32_LINUX_RESOURCE_ZONE_MAX 200U
static zone_lifecycle_entry_t
    riscv32_linux_resource_zones[RISCV32_LINUX_RESOURCE_ZONE_MAX];
static char riscv32_linux_proc_self_maps[8192];
static char riscv32_linux_proc_self_status[2048];
static char riscv32_linux_proc_self_auxv[4096];
static char riscv32_linux_proc_self_cmdline[1024];
static char riscv32_linux_proc_self_environ[1024];
static char riscv32_linux_proc_self_fdinfo[256];
static riscv32_linux_exec_stack_args_t riscv32_linux_execve_args_work;
static riscv32_linux_exec_stack_args_t riscv32_linux_execve_script_args_work;

#define RISCV32_LINUX_CURRENT_STATE() \
	riscv32_linux_process_personality_current()
#define riscv32_linux_fds \
	(RISCV32_LINUX_CURRENT_STATE()->fds)
#define riscv32_linux_stdio \
	(RISCV32_LINUX_CURRENT_STATE()->stdio)
#define riscv32_linux_stdio_ready \
	(RISCV32_LINUX_CURRENT_STATE()->stdio_ready)
#define riscv32_linux_sigactions \
	(RISCV32_LINUX_CURRENT_STATE()->sigactions)
#define riscv32_linux_mmap_page_info \
	(RISCV32_LINUX_CURRENT_STATE()->mmap_page_info)
#define riscv32_linux_brk_current \
	(RISCV32_LINUX_CURRENT_STATE()->brk_current)
#define riscv32_linux_brk_mapped_pages \
	(RISCV32_LINUX_CURRENT_STATE()->brk_mapped_pages)
#define riscv32_linux_cwd \
	(RISCV32_LINUX_CURRENT_STATE()->cwd)
#define riscv32_linux_task_name \
	(RISCV32_LINUX_CURRENT_STATE()->task_name)
#define riscv32_linux_priority_encoded \
	(RISCV32_LINUX_CURRENT_STATE()->priority_encoded)
#define riscv32_linux_personality \
	(RISCV32_LINUX_CURRENT_STATE()->personality)
#define riscv32_linux_umask \
	(RISCV32_LINUX_CURRENT_STATE()->umask)
#define riscv32_linux_uid \
	(RISCV32_LINUX_CURRENT_STATE()->uid)
#define riscv32_linux_euid \
	(RISCV32_LINUX_CURRENT_STATE()->euid)
#define riscv32_linux_suid \
	(RISCV32_LINUX_CURRENT_STATE()->suid)
#define riscv32_linux_fsuid \
	(RISCV32_LINUX_CURRENT_STATE()->fsuid)
#define riscv32_linux_gid \
	(RISCV32_LINUX_CURRENT_STATE()->gid)
#define riscv32_linux_egid \
	(RISCV32_LINUX_CURRENT_STATE()->egid)
#define riscv32_linux_sgid \
	(RISCV32_LINUX_CURRENT_STATE()->sgid)
#define riscv32_linux_fsgid \
	(RISCV32_LINUX_CURRENT_STATE()->fsgid)
#define riscv32_linux_pgid \
	(RISCV32_LINUX_CURRENT_STATE()->pgid)
#define riscv32_linux_sid \
	(RISCV32_LINUX_CURRENT_STATE()->sid)
#define riscv32_linux_signal_mask \
	(RISCV32_LINUX_CURRENT_STATE()->signal_mask)
#define riscv32_linux_sigaltstack \
	(RISCV32_LINUX_CURRENT_STATE()->sigaltstack)

static const char *riscv32_linux_proc_version_text(void);
static const char *riscv32_linux_proc_cpuinfo_text(void);
static const char *riscv32_linux_proc_meminfo_text(void);
static const char *riscv32_linux_proc_uptime_text(void);
static const char *riscv32_linux_proc_stat_text(void);
static const char *riscv32_linux_proc_caribeos_text(void);
static const char *riscv32_linux_proc_bootlog_text(void);
static const char *riscv32_linux_proc_iokit_text(void);
static const char *riscv32_linux_proc_caribe_sbi_text(void);
static const char *riscv32_linux_proc_caribe_devices_text(void);
static const char *riscv32_linux_proc_caribe_memmap_text(void);
static const char *riscv32_linux_proc_caribe_resources_text(void);
static const char *riscv32_linux_proc_self_maps_text(void);
static const char *riscv32_linux_proc_self_status_text(void);
static uint32_t riscv32_linux_proc_self_auxv_data(const uint8_t **data);
static uint32_t riscv32_linux_proc_self_cmdline_data(const uint8_t **data);
static uint32_t riscv32_linux_proc_self_environ_data(const uint8_t **data);
static const char *riscv32_linux_proc_mounts_text(void);
static const char *riscv32_linux_proc_filesystems_text(void);
static boolean_t riscv32_linux_proc_data_for_kind(uint32_t kind,
    const uint8_t **data, uint32_t *size);
static void riscv32_linux_copy_cstr(char *dst, uint32_t dst_size,
    const char *src);
static uint32_t riscv32_linux_boot_total_ram_bytes(void);
static uint32_t riscv32_linux_boot_usable_ram_bytes(void);
static int riscv32_linux_mmap_page_index(uint32_t vaddr);

extern uint32_t riscv32_iokit_stage0_flags(void);
extern uint32_t riscv32_iokit_stage0_pe_init_count(void);
extern uint32_t riscv32_iokit_micro_service_count(void);
extern uint32_t riscv32_iokit_micro_started_count(void);
extern uint32_t riscv32_iokit_micro_published_count(void);
extern uint32_t riscv32_iokit_micro_matched_count(void);
extern uint32_t riscv32_iokit_micro_registry_line_count(void);
extern const char *riscv32_iokit_micro_registry_line(uint32_t index);

static uint32_t
riscv32_linux_current_pid(void)
{
	riscv32_linux_process_t *process = riscv32_linux_process_current();

	return process != (riscv32_linux_process_t *)0 ? process->pid : 1U;
}

static uint32_t
riscv32_linux_current_ppid(void)
{
	riscv32_linux_process_t *process = riscv32_linux_process_current();

	return process != (riscv32_linux_process_t *)0 ? process->ppid : 0U;
}

static void
riscv32_linux_return_errno(riscv_saved_state_t *state, uint32_t error)
{
	state->thread.x[RISCV_X_A0] = (uint32_t)(0U - error);
}

static void
riscv32_linux_return_value(riscv_saved_state_t *state, uint32_t value)
{
	state->thread.x[RISCV_X_A0] = value;
}

static void
riscv32_linux_zero(void *ptr, uint32_t size)
{
	uint8_t *bytes = (uint8_t *)ptr;
	uint32_t i;

	for (i = 0; i < size; i++) {
		bytes[i] = 0;
	}
}

static void riscv32_linux_pipe_ref_fd(const riscv32_linux_fd_t *desc);
static void riscv32_linux_pipe_release_fd(const riscv32_linux_fd_t *desc);

static void
riscv32_linux_ofd_init(void)
{
	uint32_t i;

	if (__sync_fetch_and_add(&riscv32_linux_ofd_init_state, 0U) == 2U) {
		return;
	}
	if (__sync_bool_compare_and_swap(&riscv32_linux_ofd_init_state,
	    0U, 1U)) {
		simple_lock_init(&riscv32_linux_ofd_pool_lock, 0);
		simple_lock_init(&riscv32_linux_pipe_pool_lock, 0);
		riscv32_linux_zero(riscv32_linux_pipes,
		    sizeof(riscv32_linux_pipes));
		riscv32_linux_zero(riscv32_linux_ofd_pool,
		    sizeof(riscv32_linux_ofd_pool));
		riscv32_linux_zero(riscv32_linux_ofd_used,
		    sizeof(riscv32_linux_ofd_used));
		for (i = 0U; i < RISCV32_LINUX_OFD_MAX; i++) {
			riscv32_linux_ofd_references[i] = 0U;
			simple_lock_init(&riscv32_linux_ofd_locks[i], 0);
		}
		for (i = 0U; i < RISCV32_LINUX_PIPE_MAX; i++) {
			simple_lock_init(&riscv32_linux_pipe_locks[i], 0);
		}
		riscv32_linux_zero(&riscv32_linux_ofd_stats,
		    sizeof(riscv32_linux_ofd_stats));
		riscv32_linux_ofd_stats.capacity = RISCV32_LINUX_OFD_MAX;
		riscv32_linux_zero(&riscv32_linux_pipe_stats,
		    sizeof(riscv32_linux_pipe_stats));
		riscv32_linux_pipe_stats.capacity = RISCV32_LINUX_PIPE_MAX;
		__sync_synchronize();
		riscv32_linux_ofd_init_state = 2U;
		return;
	}
	while (__sync_fetch_and_add(&riscv32_linux_ofd_init_state, 0U) != 2U) {
		__asm__ volatile("nop");
	}
}

static riscv32_linux_fd_t *
riscv32_linux_ofd_get(uint32_t ofd_id)
{
	uint32_t slot;
	riscv32_linux_fd_t *ofd;

	if (ofd_id == 0U || ofd_id > RISCV32_LINUX_OFD_MAX) {
		return (riscv32_linux_fd_t *)0;
	}
	riscv32_linux_ofd_init();
	slot = ofd_id - 1U;
	simple_lock(&riscv32_linux_ofd_pool_lock);
	if (riscv32_linux_ofd_used[slot] != 1U ||
	    riscv32_linux_ofd_references[slot] == 0U) {
		simple_unlock(&riscv32_linux_ofd_pool_lock);
		return (riscv32_linux_fd_t *)0;
	}
	ofd = &riscv32_linux_ofd_pool[slot];
	if (ofd->magic != RISCV32_LINUX_OFD_MAGIC ||
	    ofd->pool_id != ofd_id || ofd->kind == RISCV32_LINUX_FD_NONE) {
		simple_unlock(&riscv32_linux_ofd_pool_lock);
		panic("riscv32: corrupt Linux open file description id=%u",
		    ofd_id);
	}
	simple_unlock(&riscv32_linux_ofd_pool_lock);
	return ofd;
}

static riscv32_linux_fd_t *
riscv32_linux_ofd_alloc(uint32_t kind, uint32_t accmode,
    const riscv32_linux_initrd_file_t *file, uint32_t *ofd_id_out)
{
	riscv32_linux_fd_t *ofd;
	uint32_t slot;

	if (kind == RISCV32_LINUX_FD_NONE) {
		return (riscv32_linux_fd_t *)0;
	}
	riscv32_linux_ofd_init();
	simple_lock(&riscv32_linux_ofd_pool_lock);
	for (slot = 0U; slot < RISCV32_LINUX_OFD_MAX; slot++) {
		if (riscv32_linux_ofd_used[slot] == 0U) {
			break;
		}
	}
	if (slot == RISCV32_LINUX_OFD_MAX) {
		simple_unlock(&riscv32_linux_ofd_pool_lock);
		return (riscv32_linux_fd_t *)0;
	}
	riscv32_linux_ofd_used[slot] = 2U;
	simple_unlock(&riscv32_linux_ofd_pool_lock);

	ofd = &riscv32_linux_ofd_pool[slot];
	riscv32_linux_zero(ofd, sizeof(*ofd));
	ofd->magic = RISCV32_LINUX_OFD_MAGIC;
	ofd->pool_id = slot + 1U;
	ofd->kind = kind;
	ofd->accmode = accmode;
	ofd->status_flags = accmode;
	if (file != (const riscv32_linux_initrd_file_t *)0) {
		ofd->file = *file;
	}
	riscv32_linux_ofd_references[slot] = 1U;
	__sync_synchronize();
	simple_lock(&riscv32_linux_ofd_pool_lock);
	if (riscv32_linux_ofd_used[slot] != 2U) {
		simple_unlock(&riscv32_linux_ofd_pool_lock);
		panic("riscv32: Linux OFD allocation state corrupt");
	}
	riscv32_linux_ofd_used[slot] = 1U;
	riscv32_linux_ofd_stats.in_use++;
	riscv32_linux_ofd_stats.references++;
	riscv32_linux_ofd_stats.total_allocations++;
	if (riscv32_linux_ofd_stats.in_use >
	    riscv32_linux_ofd_stats.high_watermark) {
		riscv32_linux_ofd_stats.high_watermark =
		    riscv32_linux_ofd_stats.in_use;
	}
	simple_unlock(&riscv32_linux_ofd_pool_lock);
	if (ofd_id_out != (uint32_t *)0) {
		*ofd_id_out = slot + 1U;
	}
	return ofd;
}

static boolean_t
riscv32_linux_ofd_retain(uint32_t ofd_id)
{
	uint32_t slot;

	if (ofd_id == 0U || ofd_id > RISCV32_LINUX_OFD_MAX) {
		return FALSE;
	}
	riscv32_linux_ofd_init();
	slot = ofd_id - 1U;
	simple_lock(&riscv32_linux_ofd_pool_lock);
	if (riscv32_linux_ofd_used[slot] != 1U ||
	    riscv32_linux_ofd_references[slot] == 0U ||
	    riscv32_linux_ofd_references[slot] == UINT32_MAX) {
		simple_unlock(&riscv32_linux_ofd_pool_lock);
		return FALSE;
	}
	riscv32_linux_ofd_references[slot]++;
	riscv32_linux_ofd_stats.references++;
	riscv32_linux_ofd_stats.total_retains++;
	simple_unlock(&riscv32_linux_ofd_pool_lock);
	return TRUE;
}

static void
riscv32_linux_ofd_release(uint32_t ofd_id)
{
	riscv32_linux_fd_t *ofd;
	uint32_t slot;

	if (ofd_id == 0U || ofd_id > RISCV32_LINUX_OFD_MAX) {
		panic("riscv32: invalid Linux OFD release id=%u", ofd_id);
	}
	riscv32_linux_ofd_init();
	slot = ofd_id - 1U;
	simple_lock(&riscv32_linux_ofd_pool_lock);
	if (riscv32_linux_ofd_used[slot] != 1U ||
	    riscv32_linux_ofd_references[slot] == 0U ||
	    riscv32_linux_ofd_stats.references == 0U) {
		simple_unlock(&riscv32_linux_ofd_pool_lock);
		panic("riscv32: Linux OFD reference underflow id=%u", ofd_id);
	}
	riscv32_linux_ofd_references[slot]--;
	riscv32_linux_ofd_stats.references--;
	riscv32_linux_ofd_stats.total_releases++;
	if (riscv32_linux_ofd_references[slot] != 0U) {
		simple_unlock(&riscv32_linux_ofd_pool_lock);
		return;
	}
	riscv32_linux_ofd_used[slot] = 2U;
	ofd = &riscv32_linux_ofd_pool[slot];
	simple_unlock(&riscv32_linux_ofd_pool_lock);

	riscv32_linux_pipe_release_fd(ofd);
	riscv32_linux_zero(ofd, sizeof(*ofd));
	__sync_synchronize();
	simple_lock(&riscv32_linux_ofd_pool_lock);
	if (riscv32_linux_ofd_used[slot] != 2U ||
	    riscv32_linux_ofd_stats.in_use == 0U) {
		simple_unlock(&riscv32_linux_ofd_pool_lock);
		panic("riscv32: Linux OFD free state corrupt");
	}
	riscv32_linux_ofd_used[slot] = 0U;
	riscv32_linux_ofd_stats.in_use--;
	riscv32_linux_ofd_stats.total_frees++;
	simple_unlock(&riscv32_linux_ofd_pool_lock);
}

static void
riscv32_linux_ofd_lock(riscv32_linux_fd_t *ofd)
{
	if (ofd == (riscv32_linux_fd_t *)0 || ofd->pool_id == 0U ||
	    ofd->pool_id > RISCV32_LINUX_OFD_MAX) {
		panic("riscv32: invalid Linux OFD lock");
	}
	simple_lock(&riscv32_linux_ofd_locks[ofd->pool_id - 1U]);
}

static void
riscv32_linux_ofd_unlock(riscv32_linux_fd_t *ofd)
{
	simple_unlock(&riscv32_linux_ofd_locks[ofd->pool_id - 1U]);
}

void
riscv32_linux_syscall_get_ofd_stats(riscv32_linux_ofd_stats_t *stats)
{
	if (stats == (riscv32_linux_ofd_stats_t *)0) {
		return;
	}
	riscv32_linux_ofd_init();
	simple_lock(&riscv32_linux_ofd_pool_lock);
	*stats = riscv32_linux_ofd_stats;
	simple_unlock(&riscv32_linux_ofd_pool_lock);
}

void
riscv32_linux_syscall_get_pipe_stats(riscv32_linux_pipe_stats_t *stats)
{
	if (stats == (riscv32_linux_pipe_stats_t *)0) {
		return;
	}
	riscv32_linux_ofd_init();
	simple_lock(&riscv32_linux_pipe_pool_lock);
	*stats = riscv32_linux_pipe_stats;
	simple_unlock(&riscv32_linux_pipe_pool_lock);
}

static boolean_t
riscv32_linux_fd_mode_readable(const riscv32_linux_fd_t *desc)
{
	return desc != (const riscv32_linux_fd_t *)0 &&
	    (desc->accmode & RISCV32_LINUX_O_ACCMODE) !=
	    RISCV32_LINUX_O_WRONLY;
}

static boolean_t
riscv32_linux_fd_mode_writable(const riscv32_linux_fd_t *desc)
{
	return desc != (const riscv32_linux_fd_t *)0 &&
	    (desc->accmode & RISCV32_LINUX_O_ACCMODE) !=
	    RISCV32_LINUX_O_RDONLY;
}

static riscv32_linux_pipe_t *
riscv32_linux_pipe_get(uint32_t pipe_id)
{
	riscv32_linux_ofd_init();
	if (pipe_id == 0 || pipe_id > RISCV32_LINUX_PIPE_MAX ||
	    riscv32_linux_pipes[pipe_id - 1U].used == 0) {
		return (riscv32_linux_pipe_t *)0;
	}
	return &riscv32_linux_pipes[pipe_id - 1U];
}

static uint32_t
riscv32_linux_pipe_alloc(void)
{
	uint32_t i;

	riscv32_linux_ofd_init();
	simple_lock(&riscv32_linux_pipe_pool_lock);
	for (i = 0; i < RISCV32_LINUX_PIPE_MAX; i++) {
		if (riscv32_linux_pipes[i].used != 0) {
			continue;
		}
		simple_lock(&riscv32_linux_pipe_locks[i]);
		riscv32_linux_zero(&riscv32_linux_pipes[i],
		    sizeof(riscv32_linux_pipes[i]));
		riscv32_linux_pipes[i].used = 1U;
		riscv32_linux_pipes[i].id = i + 1U;
		riscv32_linux_pipe_stats.in_use++;
		riscv32_linux_pipe_stats.total_allocations++;
		if (riscv32_linux_pipe_stats.in_use >
		    riscv32_linux_pipe_stats.high_watermark) {
			riscv32_linux_pipe_stats.high_watermark =
			    riscv32_linux_pipe_stats.in_use;
		}
		simple_unlock(&riscv32_linux_pipe_locks[i]);
		simple_unlock(&riscv32_linux_pipe_pool_lock);
		return i + 1U;
	}
	simple_unlock(&riscv32_linux_pipe_pool_lock);
	return 0;
}

static void
riscv32_linux_pipe_abandon(uint32_t pipe_id)
{
	riscv32_linux_pipe_t *pipe = riscv32_linux_pipe_get(pipe_id);

	if (pipe == (riscv32_linux_pipe_t *)0) {
		return;
	}
	simple_lock(&riscv32_linux_pipe_locks[pipe_id - 1U]);
	if (pipe->readers != 0U || pipe->writers != 0U) {
		simple_unlock(&riscv32_linux_pipe_locks[pipe_id - 1U]);
		panic("riscv32: abandoning referenced Linux pipe id=%u", pipe_id);
	}
	riscv32_linux_zero(pipe, sizeof(*pipe));
	simple_unlock(&riscv32_linux_pipe_locks[pipe_id - 1U]);
	simple_lock(&riscv32_linux_pipe_pool_lock);
	if (riscv32_linux_pipe_stats.in_use == 0U) {
		simple_unlock(&riscv32_linux_pipe_pool_lock);
		panic("riscv32: Linux pipe abandon accounting underflow");
	}
	riscv32_linux_pipe_stats.in_use--;
	riscv32_linux_pipe_stats.total_releases++;
	simple_unlock(&riscv32_linux_pipe_pool_lock);
}

static void
riscv32_linux_pipe_ref_fd(const riscv32_linux_fd_t *desc)
{
	riscv32_linux_pipe_t *pipe;

	if (desc == (const riscv32_linux_fd_t *)0 ||
	    desc->kind != RISCV32_LINUX_FD_PIPE) {
		return;
	}
	pipe = riscv32_linux_pipe_get((uint32_t)desc->value);
	if (pipe == (riscv32_linux_pipe_t *)0) {
		return;
	}
	simple_lock(&riscv32_linux_pipe_locks[pipe->id - 1U]);
	if (riscv32_linux_fd_mode_readable(desc)) {
		pipe->readers++;
	}
	if (riscv32_linux_fd_mode_writable(desc)) {
		pipe->writers++;
	}
	simple_unlock(&riscv32_linux_pipe_locks[pipe->id - 1U]);
}

static void
riscv32_linux_pipe_release_fd(const riscv32_linux_fd_t *desc)
{
	riscv32_linux_pipe_t *pipe;
	uint32_t pipe_id;
	uint32_t readers;
	uint32_t writers;
	uint32_t read_blocks;
	uint32_t read_wakeups;
	uint32_t write_blocks;
	uint32_t write_wakeups;
	uint32_t bytes_read;
	uint32_t bytes_written;
	boolean_t wake_readers = FALSE;
	boolean_t wake_writers = FALSE;
	boolean_t released = FALSE;

	if (desc == (const riscv32_linux_fd_t *)0 ||
	    desc->kind != RISCV32_LINUX_FD_PIPE) {
		return;
	}
	pipe = riscv32_linux_pipe_get((uint32_t)desc->value);
	if (pipe == (riscv32_linux_pipe_t *)0) {
		return;
	}
	pipe_id = pipe->id;
	simple_lock(&riscv32_linux_pipe_locks[pipe_id - 1U]);
	if (riscv32_linux_fd_mode_readable(desc) && pipe->readers != 0) {
		pipe->readers--;
		pipe->write_sequence++;
		wake_writers = TRUE;
	}
	if (riscv32_linux_fd_mode_writable(desc) && pipe->writers != 0) {
		pipe->writers--;
		pipe->read_sequence++;
		wake_readers = TRUE;
	}
	readers = pipe->readers;
	writers = pipe->writers;
	read_blocks = pipe->read_blocks;
	read_wakeups = pipe->read_wakeups;
	write_blocks = pipe->write_blocks;
	write_wakeups = pipe->write_wakeups;
	bytes_read = pipe->bytes_read;
	bytes_written = pipe->bytes_written;
	if (pipe->readers == 0 && pipe->writers == 0) {
		riscv32_linux_zero(pipe, sizeof(*pipe));
		released = TRUE;
	}
	simple_unlock(&riscv32_linux_pipe_locks[pipe_id - 1U]);
	if (released) {
		simple_lock(&riscv32_linux_pipe_pool_lock);
		if (riscv32_linux_pipe_stats.in_use == 0U) {
			simple_unlock(&riscv32_linux_pipe_pool_lock);
			panic("riscv32: Linux pipe release accounting underflow");
		}
		riscv32_linux_pipe_stats.in_use--;
		riscv32_linux_pipe_stats.total_releases++;
		riscv32_linux_pipe_stats.bytes_read += bytes_read;
		riscv32_linux_pipe_stats.bytes_written += bytes_written;
		riscv32_linux_pipe_stats.read_blocks += read_blocks;
		riscv32_linux_pipe_stats.read_wakeups += read_wakeups;
		riscv32_linux_pipe_stats.write_blocks += write_blocks;
		riscv32_linux_pipe_stats.write_wakeups += write_wakeups;
		simple_unlock(&riscv32_linux_pipe_pool_lock);
	}
	if (wake_readers) {
		thread_wakeup((event_t)&pipe->read_sequence);
	}
	if (wake_writers) {
		thread_wakeup((event_t)&pipe->write_sequence);
	}
	if (released && riscv32_boot_arg_has("process-gate-12")) {
		riscv32_early_puts("[process-gate-12] pipe id=");
		riscv32_early_putdec(pipe_id);
		riscv32_early_puts(" released readers/writers=");
		riscv32_early_putdec(readers);
		riscv32_early_puts("/");
		riscv32_early_putdec(writers);
		riscv32_early_puts(" bytes=");
		riscv32_early_putdec(bytes_written);
		riscv32_early_puts("/");
		riscv32_early_putdec(bytes_read);
		riscv32_early_puts(" blocks/wakeups=");
		riscv32_early_putdec(read_blocks);
		riscv32_early_puts("/");
		riscv32_early_putdec(read_wakeups);
		riscv32_early_puts("+");
		riscv32_early_putdec(write_blocks);
		riscv32_early_puts("/");
		riscv32_early_putdec(write_wakeups);
		riscv32_early_puts(" PASS\n");
		if (bytes_written == 12288U && bytes_read == bytes_written &&
		    read_blocks != 0U && read_blocks == read_wakeups &&
		    write_blocks != 0U && write_blocks == write_wakeups) {
			uint32_t completed = __sync_add_and_fetch(
			    &riscv32_linux_pipe_gate12_completed, 1U);

			riscv32_early_puts(
			    "[process-gate-12] blocking pipeline lifecycle index=");
			riscv32_early_putdec(completed);
			riscv32_early_puts(
			    " transfer=12288 EOF=1 no-lost-wakeup=1 resources=0 PASS\n");
		}
	}
}

static void
riscv32_linux_fd_entry_clear(riscv32_linux_process_fd_t *entry)
{
	uint32_t ofd_id;

	if (entry == (riscv32_linux_process_fd_t *)0 ||
	    entry->ofd_id == 0U) {
		return;
	}
	ofd_id = entry->ofd_id;
	riscv32_linux_zero(entry, sizeof(*entry));
	riscv32_linux_ofd_release(ofd_id);
}

boolean_t
riscv32_linux_syscall_inherit_fork_state(
    riscv32_linux_process_t *parent, riscv32_linux_process_t *child)
{
	uint32_t i;

	if (parent == (riscv32_linux_process_t *)0 ||
	    child == (riscv32_linux_process_t *)0 || parent == child ||
	    parent->magic != RISCV32_LINUX_PROCESS_MAGIC ||
	    child->magic != RISCV32_LINUX_PROCESS_MAGIC) {
		return FALSE;
	}
	simple_lock(&parent->lock);
	if (parent->state == RISCV32_LINUX_PROCESS_EXITING ||
	    parent->state == RISCV32_LINUX_PROCESS_ZOMBIE ||
	    parent->state == RISCV32_LINUX_PROCESS_DEAD) {
		simple_unlock(&parent->lock);
		return FALSE;
	}
	for (i = 0; i < RISCV32_LINUX_PROCESS_STDIO_COUNT; i++) {
		if (parent->personality.stdio[i].ofd_id != 0U &&
		    riscv32_linux_ofd_get(
		    parent->personality.stdio[i].ofd_id) ==
		    (riscv32_linux_fd_t *)0) {
			simple_unlock(&parent->lock);
			return FALSE;
		}
	}
	for (i = 0; i < RISCV32_LINUX_PROCESS_FD_MAX; i++) {
		if (parent->personality.fds[i].ofd_id != 0U &&
		    riscv32_linux_ofd_get(parent->personality.fds[i].ofd_id) ==
		    (riscv32_linux_fd_t *)0) {
			simple_unlock(&parent->lock);
			return FALSE;
		}
	}
	simple_lock(&child->lock);
	if (child->state != RISCV32_LINUX_PROCESS_NEW ||
	    child->personality.stdio_ready != 0U) {
		simple_unlock(&child->lock);
		simple_unlock(&parent->lock);
		return FALSE;
	}
	child->personality = parent->personality;
	riscv32_linux_zero(child->personality.pending_signals,
	    sizeof(child->personality.pending_signals));
	simple_unlock(&child->lock);
	simple_unlock(&parent->lock);
	for (i = 0; i < RISCV32_LINUX_PROCESS_STDIO_COUNT; i++) {
		if (child->personality.stdio[i].ofd_id != 0U &&
		    !riscv32_linux_ofd_retain(
		    child->personality.stdio[i].ofd_id)) {
			panic("riscv32: failed to retain inherited stdio OFD");
		}
	}
	for (i = 0; i < RISCV32_LINUX_PROCESS_FD_MAX; i++) {
		if (child->personality.fds[i].ofd_id != 0U &&
		    !riscv32_linux_ofd_retain(
		    child->personality.fds[i].ofd_id)) {
			panic("riscv32: failed to retain inherited fd OFD");
		}
	}
	return TRUE;
}

void
riscv32_linux_syscall_teardown_process(riscv32_linux_process_t *process,
    riscv32_linux_exit_resources_t *resources)
{
	riscv32_linux_personality_state_t *personality;
	uint32_t i;

	if (process == (riscv32_linux_process_t *)0 ||
	    process->magic != RISCV32_LINUX_PROCESS_MAGIC ||
	    resources == (riscv32_linux_exit_resources_t *)0) {
		panic("riscv32: invalid Linux process teardown");
	}
	personality = &process->personality;
	riscv32_linux_zero(resources, sizeof(*resources));
	for (i = 0; i < RISCV32_LINUX_PIPE_MAX; i++) {
		if (riscv32_linux_pipes[i].used != 0U) {
			resources->live_pipes_before++;
		}
	}
	for (i = 0; i < RISCV32_LINUX_PROCESS_STDIO_COUNT; i++) {
		riscv32_linux_fd_t *ofd;

		if (personality->stdio[i].ofd_id == 0U) {
			continue;
		}
		ofd = riscv32_linux_ofd_get(personality->stdio[i].ofd_id);
		if (ofd == (riscv32_linux_fd_t *)0) {
			panic("riscv32: process owns invalid stdio OFD");
		}
		resources->open_fds_before++;
		resources->closed_fds++;
		if (ofd->kind == RISCV32_LINUX_FD_PIPE) {
			resources->pipe_endpoints_closed++;
		}
		riscv32_linux_fd_entry_clear(&personality->stdio[i]);
	}
	for (i = 0; i < RISCV32_LINUX_PROCESS_FD_MAX; i++) {
		riscv32_linux_fd_t *ofd;

		if (personality->fds[i].ofd_id == 0U) {
			continue;
		}
		ofd = riscv32_linux_ofd_get(personality->fds[i].ofd_id);
		if (ofd == (riscv32_linux_fd_t *)0) {
			panic("riscv32: process owns invalid fd OFD");
		}
		resources->open_fds_before++;
		resources->closed_fds++;
		if (ofd->kind == RISCV32_LINUX_FD_PIPE) {
			resources->pipe_endpoints_closed++;
		}
		riscv32_linux_fd_entry_clear(&personality->fds[i]);
	}
	personality->stdio_ready = 0U;
	for (i = 0; i < RISCV32_LINUX_PROCESS_MMAP_PAGES; i++) {
		if (personality->mmap_page_info[i].used != 0U) {
			resources->mmap_pages_released++;
		}
	}
	resources->brk_pages_released = personality->brk_mapped_pages;
	riscv32_linux_zero(personality->mmap_page_info,
	    sizeof(personality->mmap_page_info));
	personality->brk_current = personality->brk_base;
	personality->brk_mapped_pages = 0U;
	riscv32_linux_zero(personality->sigactions,
	    sizeof(personality->sigactions));
	riscv32_linux_zero(personality->signal_mask,
	    sizeof(personality->signal_mask));
	riscv32_linux_zero(personality->sigaltstack,
	    sizeof(personality->sigaltstack));
	for (i = 0; i < RISCV32_LINUX_PIPE_MAX; i++) {
		if (riscv32_linux_pipes[i].used != 0U) {
			resources->live_pipes_after++;
		}
	}
}

static boolean_t
riscv32_linux_pipe_wait(riscv32_linux_process_t *process,
    riscv32_linux_pipe_t *pipe, boolean_t reading, uint32_t expected)
{
	uint32_t *sequence = reading ? &pipe->read_sequence :
	    &pipe->write_sequence;
	boolean_t blocked = FALSE;

	if (process == (riscv32_linux_process_t *)0 ||
	    !riscv32_linux_process_transition(process,
	    RISCV32_LINUX_PROCESS_SLEEPING)) {
		panic("riscv32: pipe process cannot enter sleep");
	}
	assert_wait((event_t)sequence, THREAD_UNINT);
	__sync_synchronize();
	if (__sync_fetch_and_add(sequence, 0U) == expected &&
	    !riscv32_linux_signal_has_deliverable(process)) {
		if (reading) {
			(void)__sync_add_and_fetch(&pipe->read_blocks, 1U);
			if (riscv32_boot_arg_has("process-gate-12") &&
			    __sync_add_and_fetch(
			    &riscv32_linux_pipe_gate12_read_block_logged, 1U) == 1U) {
				riscv32_early_puts("[process-gate-12] pipe reader pid=");
				riscv32_early_putdec(process->pid);
				riscv32_early_puts(
				    " blocking on empty pipe without polling PASS\n");
			}
		} else {
			(void)__sync_add_and_fetch(&pipe->write_blocks, 1U);
			if (riscv32_boot_arg_has("process-gate-12") &&
			    __sync_add_and_fetch(
			    &riscv32_linux_pipe_gate12_write_block_logged, 1U) == 1U) {
				riscv32_early_puts("[process-gate-12] pipe writer pid=");
				riscv32_early_putdec(process->pid);
				riscv32_early_puts(
				    " blocking on full pipe without polling PASS\n");
			}
		}
		thread_block(THREAD_CONTINUE_NULL);
		if (reading) {
			(void)__sync_add_and_fetch(&pipe->read_wakeups, 1U);
		} else {
			(void)__sync_add_and_fetch(&pipe->write_wakeups, 1U);
		}
		blocked = TRUE;
	} else {
		(void)clear_wait(current_thread(), THREAD_AWAKENED);
	}
	if (!riscv32_linux_process_transition(process,
	    RISCV32_LINUX_PROCESS_RUNNABLE) ||
	    !riscv32_linux_process_transition(process,
	    RISCV32_LINUX_PROCESS_RUNNING)) {
		panic("riscv32: pipe process cannot resume");
	}
	return blocked;
}

static boolean_t
riscv32_linux_pipe_read_to_user(const riscv32_linux_fd_t *desc,
    uint32_t user_buf, uint32_t count, uint32_t *copied_out,
    uint32_t *error_out)
{
	riscv32_linux_process_t *process = riscv32_linux_process_current();
	riscv32_linux_pipe_t *pipe;
	uint32_t pipe_id;
	uint32_t expected;
	uint32_t limit;
	uint32_t copied;
	uint32_t chunk;
	uint32_t until_wrap;
	boolean_t wake_writers;

	*copied_out = 0U;
	*error_out = 0U;
	if (!riscv32_linux_fd_mode_readable(desc)) {
		*error_out = RISCV32_LINUX_EBADF;
		return FALSE;
	}
	if (count == 0U) {
		return TRUE;
	}
	limit = count;
	if (limit > RISCV32_LINUX_READ_MAX) {
		limit = RISCV32_LINUX_READ_MAX;
	}
	if (!riscv32_user_range_valid(user_buf, limit)) {
		*error_out = RISCV32_LINUX_EFAULT;
		return FALSE;
	}
	pipe_id = (uint32_t)desc->value;
	for (;;) {
		if (riscv32_linux_signal_has_deliverable(process)) {
			*error_out = RISCV32_LINUX_EINTR;
			return FALSE;
		}
		pipe = riscv32_linux_pipe_get(pipe_id);
		if (pipe == (riscv32_linux_pipe_t *)0) {
			*error_out = RISCV32_LINUX_EBADF;
			return FALSE;
		}
		simple_lock(&riscv32_linux_pipe_locks[pipe_id - 1U]);
		if (pipe->used == 0U || pipe->id != pipe_id) {
			simple_unlock(&riscv32_linux_pipe_locks[pipe_id - 1U]);
			*error_out = RISCV32_LINUX_EBADF;
			return FALSE;
		}
		if (pipe->size != 0U) {
			if (limit > pipe->size) {
				limit = pipe->size;
			}
			for (copied = 0U; copied < limit; copied += chunk) {
				until_wrap = RISCV32_LINUX_PIPE_BUF - pipe->read_pos;
				chunk = limit - copied;
				if (chunk > until_wrap) {
					chunk = until_wrap;
				}
				if (riscv32_copyout(pipe->data + pipe->read_pos,
				    user_buf + copied, chunk) != 0) {
					simple_unlock(
					    &riscv32_linux_pipe_locks[pipe_id - 1U]);
					*error_out = RISCV32_LINUX_EFAULT;
					return FALSE;
				}
				pipe->read_pos = (pipe->read_pos + chunk) %
				    RISCV32_LINUX_PIPE_BUF;
				pipe->size -= chunk;
				pipe->bytes_read += chunk;
				*copied_out = copied + chunk;
			}
			pipe->write_sequence++;
			wake_writers = TRUE;
			simple_unlock(&riscv32_linux_pipe_locks[pipe_id - 1U]);
			if (wake_writers) {
				thread_wakeup((event_t)&pipe->write_sequence);
			}
			return TRUE;
		}
		if (pipe->writers == 0U) {
			simple_unlock(&riscv32_linux_pipe_locks[pipe_id - 1U]);
			if (riscv32_boot_arg_has("process-gate-12") &&
			    __sync_add_and_fetch(
			    &riscv32_linux_pipe_gate12_eof_logged, 1U) == 1U) {
				riscv32_early_puts("[process-gate-12] pipe reader pid=");
				riscv32_early_putdec(process !=
				    (riscv32_linux_process_t *)0 ? process->pid : 0U);
				riscv32_early_puts(
				    " observed EOF after final writer close PASS\n");
			}
			return TRUE;
		}
		if ((desc->status_flags & RISCV32_LINUX_O_NONBLOCK) != 0U) {
			simple_unlock(&riscv32_linux_pipe_locks[pipe_id - 1U]);
			*error_out = RISCV32_LINUX_EAGAIN;
			return FALSE;
		}
		expected = pipe->read_sequence;
		simple_unlock(&riscv32_linux_pipe_locks[pipe_id - 1U]);
		(void)riscv32_linux_pipe_wait(process, pipe, TRUE, expected);
	}
}

static boolean_t
riscv32_linux_pipe_write_from_user(const riscv32_linux_fd_t *desc,
    uint32_t user_buf, uint32_t count, uint32_t *copied_out,
    uint32_t *error_out)
{
	riscv32_linux_process_t *process = riscv32_linux_process_current();
	riscv32_linux_pipe_t *pipe;
	uint32_t pipe_id;
	uint32_t expected;
	uint32_t limit;
	uint32_t copied;
	uint32_t chunk;
	uint32_t write_pos;
	uint32_t until_wrap;
	uint32_t space;

	*copied_out = 0U;
	*error_out = 0U;
	if (!riscv32_linux_fd_mode_writable(desc)) {
		*error_out = RISCV32_LINUX_EBADF;
		return FALSE;
	}
	if (count == 0U) {
		return TRUE;
	}
	limit = count;
	if (limit > RISCV32_LINUX_WRITE_MAX) {
		limit = RISCV32_LINUX_WRITE_MAX;
	}
	if (!riscv32_user_range_valid(user_buf, limit)) {
		*error_out = RISCV32_LINUX_EFAULT;
		return FALSE;
	}
	pipe_id = (uint32_t)desc->value;
	for (;;) {
		if (riscv32_linux_signal_has_deliverable(process)) {
			*error_out = RISCV32_LINUX_EINTR;
			return FALSE;
		}
		pipe = riscv32_linux_pipe_get(pipe_id);
		if (pipe == (riscv32_linux_pipe_t *)0) {
			*error_out = RISCV32_LINUX_EBADF;
			return FALSE;
		}
		simple_lock(&riscv32_linux_pipe_locks[pipe_id - 1U]);
		if (pipe->used == 0U || pipe->id != pipe_id) {
			simple_unlock(&riscv32_linux_pipe_locks[pipe_id - 1U]);
			*error_out = RISCV32_LINUX_EBADF;
			return FALSE;
		}
		if (pipe->readers == 0U) {
			simple_unlock(&riscv32_linux_pipe_locks[pipe_id - 1U]);
			if (riscv32_linux_signal_queue(process,
			    RISCV32_LINUX_SIGPIPE, process !=
			    (riscv32_linux_process_t *)0 ? process->pid : 0U,
			    0U, 0U, 0U) &&
			    riscv32_boot_arg_has("process-gate-12") &&
			    __sync_add_and_fetch(
			    &riscv32_linux_pipe_gate12_sigpipe_logged, 1U) == 1U) {
				riscv32_early_puts(
				    "[process-gate-12] SIGPIPE queued pid=");
				riscv32_early_putdec(process->pid);
				riscv32_early_puts(
				    " after final reader close PASS\n");
			}
			*error_out = RISCV32_LINUX_EPIPE;
			return FALSE;
		}
		space = RISCV32_LINUX_PIPE_BUF - pipe->size;
		if (space >= limit) {
			for (copied = 0U; copied < limit; copied += chunk) {
				write_pos = (pipe->read_pos + pipe->size) %
				    RISCV32_LINUX_PIPE_BUF;
				until_wrap = RISCV32_LINUX_PIPE_BUF - write_pos;
				chunk = limit - copied;
				if (chunk > until_wrap) {
					chunk = until_wrap;
				}
				if (riscv32_copyin(user_buf + copied,
				    pipe->data + write_pos, chunk) != 0) {
					simple_unlock(
					    &riscv32_linux_pipe_locks[pipe_id - 1U]);
					*error_out = RISCV32_LINUX_EFAULT;
					return FALSE;
				}
				pipe->size += chunk;
				pipe->bytes_written += chunk;
				*copied_out = copied + chunk;
			}
			pipe->read_sequence++;
			simple_unlock(&riscv32_linux_pipe_locks[pipe_id - 1U]);
			thread_wakeup((event_t)&pipe->read_sequence);
			return TRUE;
		}
		if ((desc->status_flags & RISCV32_LINUX_O_NONBLOCK) != 0U) {
			simple_unlock(&riscv32_linux_pipe_locks[pipe_id - 1U]);
			*error_out = RISCV32_LINUX_EAGAIN;
			return FALSE;
		}
		expected = pipe->write_sequence;
		simple_unlock(&riscv32_linux_pipe_locks[pipe_id - 1U]);
		(void)riscv32_linux_pipe_wait(process, pipe, FALSE, expected);
	}
}

static void
riscv32_linux_stdio_init(void)
{
	riscv32_linux_fd_t *ofd;
	uint32_t ofd_id;
	uint32_t i;
	static const uint32_t modes[RISCV32_LINUX_STDIO_COUNT] = {
		RISCV32_LINUX_O_RDONLY,
		RISCV32_LINUX_O_WRONLY,
		RISCV32_LINUX_O_WRONLY
	};

	if (riscv32_linux_stdio_ready != 0U) {
		return;
	}
	riscv32_linux_zero(riscv32_linux_stdio, sizeof(riscv32_linux_stdio));
	for (i = 0U; i < RISCV32_LINUX_STDIO_COUNT; i++) {
		ofd = riscv32_linux_ofd_alloc(RISCV32_LINUX_FD_CONSOLE,
		    modes[i], (const riscv32_linux_initrd_file_t *)0, &ofd_id);
		if (ofd == (riscv32_linux_fd_t *)0) {
			panic("riscv32: cannot allocate Linux stdio OFD");
		}
		riscv32_linux_stdio[i].ofd_id = ofd_id;
	}
	__sync_synchronize();
	riscv32_linux_stdio_ready = 1U;
}

static riscv32_linux_process_fd_t *
riscv32_linux_fd_entry(uint32_t fd)
{
	uint32_t index;

	if (fd < RISCV32_LINUX_FD_FIRST) {
		riscv32_linux_stdio_init();
		if (fd >= RISCV32_LINUX_STDIO_COUNT) {
			return (riscv32_linux_process_fd_t *)0;
		}
		return &riscv32_linux_stdio[fd];
	}
	index = fd - RISCV32_LINUX_FD_FIRST;
	if (index >= RISCV32_LINUX_FD_MAX) {
		return (riscv32_linux_process_fd_t *)0;
	}
	return &riscv32_linux_fds[index];
}

static riscv32_linux_fd_t *
riscv32_linux_fd_get(uint32_t fd)
{
	riscv32_linux_process_fd_t *entry;

	if (fd < RISCV32_LINUX_FD_FIRST) {
		return (riscv32_linux_fd_t *)0;
	}
	entry = riscv32_linux_fd_entry(fd);
	if (entry == (riscv32_linux_process_fd_t *)0 || entry->ofd_id == 0U) {
		return (riscv32_linux_fd_t *)0;
	}
	return riscv32_linux_ofd_get(entry->ofd_id);
}

static riscv32_linux_fd_t *
riscv32_linux_fd_ref(uint32_t fd)
{
	riscv32_linux_process_fd_t *entry = riscv32_linux_fd_entry(fd);

	if (entry == (riscv32_linux_process_fd_t *)0 || entry->ofd_id == 0U) {
		return (riscv32_linux_fd_t *)0;
	}
	return riscv32_linux_ofd_get(entry->ofd_id);
}

static boolean_t
riscv32_linux_fd_close_number(uint32_t fd)
{
	riscv32_linux_process_fd_t *entry = riscv32_linux_fd_entry(fd);

	if (entry == (riscv32_linux_process_fd_t *)0 || entry->ofd_id == 0U) {
		return FALSE;
	}
	riscv32_linux_fd_entry_clear(entry);
	return TRUE;
}

static boolean_t
riscv32_linux_fd_alloc(uint32_t kind, uint32_t accmode,
    const riscv32_linux_initrd_file_t *file, uint32_t *fd_out)
{
	riscv32_linux_fd_t *ofd;
	uint32_t ofd_id;
	uint32_t i;

	for (i = 0; i < RISCV32_LINUX_FD_MAX; i++) {
		if (riscv32_linux_fds[i].ofd_id != 0U) {
			continue;
		}
		ofd = riscv32_linux_ofd_alloc(kind, accmode, file, &ofd_id);
		if (ofd == (riscv32_linux_fd_t *)0) {
			return FALSE;
		}
		riscv32_linux_fds[i].ofd_id = ofd_id;
		riscv32_linux_fds[i].descriptor_flags = 0U;
		if (fd_out != (uint32_t *)0) {
			*fd_out = RISCV32_LINUX_FD_FIRST + i;
		}
		return TRUE;
	}
	return FALSE;
}

static boolean_t
riscv32_linux_fd_set_open_flags(uint32_t fd, uint32_t flags)
{
	riscv32_linux_fd_t *desc = riscv32_linux_fd_ref(fd);
	riscv32_linux_process_fd_t *entry = riscv32_linux_fd_entry(fd);

	if (desc == (riscv32_linux_fd_t *)0 ||
	    entry == (riscv32_linux_process_fd_t *)0) {
		return FALSE;
	}
	riscv32_linux_ofd_lock(desc);
	desc->status_flags = desc->accmode |
	    (flags & RISCV32_LINUX_O_NONBLOCK);
	riscv32_linux_ofd_unlock(desc);
	entry->descriptor_flags = (flags & RISCV32_LINUX_O_CLOEXEC) != 0 ?
	    RISCV32_LINUX_FD_CLOEXEC : 0;
	return TRUE;
}

static boolean_t
riscv32_linux_fd_alloc_dir(uint32_t dir_kind, uint32_t accmode,
    uint32_t *fd_out)
{
	riscv32_linux_fd_t *desc;
	uint32_t fd;

	if (!riscv32_linux_fd_alloc(RISCV32_LINUX_FD_DIR, accmode,
	    (const riscv32_linux_initrd_file_t *)0, &fd)) {
		return FALSE;
	}
	desc = riscv32_linux_fd_ref(fd);
	if (desc == (riscv32_linux_fd_t *)0) {
		return FALSE;
	}
	desc->dir_kind = dir_kind;
	switch (dir_kind) {
	case RISCV32_LINUX_DIR_DEV:
		riscv32_linux_copy_cstr(desc->dir_path, sizeof(desc->dir_path),
		    "/dev");
		break;
	case RISCV32_LINUX_DIR_PROC:
		riscv32_linux_copy_cstr(desc->dir_path, sizeof(desc->dir_path),
		    "/proc");
		break;
	case RISCV32_LINUX_DIR_PROC_SELF:
		riscv32_linux_copy_cstr(desc->dir_path, sizeof(desc->dir_path),
		    "/proc/self");
		break;
	case RISCV32_LINUX_DIR_PROC_SELF_FD:
		riscv32_linux_copy_cstr(desc->dir_path, sizeof(desc->dir_path),
		    "/proc/self/fd");
		break;
	case RISCV32_LINUX_DIR_PROC_SELF_FDINFO:
		riscv32_linux_copy_cstr(desc->dir_path, sizeof(desc->dir_path),
		    "/proc/self/fdinfo");
		break;
	case RISCV32_LINUX_DIR_PROC_CARIBE:
		riscv32_linux_copy_cstr(desc->dir_path, sizeof(desc->dir_path),
		    "/proc/caribe");
		break;
	case RISCV32_LINUX_DIR_DEV_FD:
		riscv32_linux_copy_cstr(desc->dir_path, sizeof(desc->dir_path),
		    "/dev/fd");
		break;
	case RISCV32_LINUX_DIR_ETC:
		riscv32_linux_copy_cstr(desc->dir_path, sizeof(desc->dir_path),
		    "/etc");
		break;
	case RISCV32_LINUX_DIR_CARIBE:
		riscv32_linux_copy_cstr(desc->dir_path, sizeof(desc->dir_path),
		    "/caribe");
		break;
	case RISCV32_LINUX_DIR_BIN:
		riscv32_linux_copy_cstr(desc->dir_path, sizeof(desc->dir_path),
		    "/bin");
		break;
	default:
		riscv32_linux_copy_cstr(desc->dir_path, sizeof(desc->dir_path),
		    "/");
		break;
	}
	if (fd_out != (uint32_t *)0) {
		*fd_out = fd;
	}
	return TRUE;
}

static boolean_t
riscv32_linux_fd_alloc_initrd_dir(const char *path, uint32_t accmode,
    uint32_t *fd_out)
{
	riscv32_linux_fd_t *desc;
	uint32_t fd;

	if (!riscv32_linux_fd_alloc_dir(RISCV32_LINUX_DIR_INITRD, accmode,
	    &fd)) {
		return FALSE;
	}
	desc = riscv32_linux_fd_get(fd);
	if (desc == (riscv32_linux_fd_t *)0) {
		return FALSE;
	}
	riscv32_linux_copy_cstr(desc->dir_path, sizeof(desc->dir_path), path);
	desc->value = 8;
	if (fd_out != (uint32_t *)0) {
		*fd_out = fd;
	}
	return TRUE;
}

static boolean_t
riscv32_linux_fd_snapshot(uint32_t fd, riscv32_linux_fd_t *out)
{
	riscv32_linux_fd_t *desc;
	riscv32_linux_process_fd_t *entry;

	riscv32_linux_zero(out, sizeof(*out));
	entry = riscv32_linux_fd_entry(fd);
	if (entry == (riscv32_linux_process_fd_t *)0 || entry->ofd_id == 0U) {
		return FALSE;
	}
	desc = riscv32_linux_ofd_get(entry->ofd_id);
	if (desc == (riscv32_linux_fd_t *)0) {
		return FALSE;
	}
	riscv32_linux_ofd_lock(desc);
	*out = *desc;
	riscv32_linux_ofd_unlock(desc);
	out->descriptor_flags = entry->descriptor_flags;
	return TRUE;
}

static boolean_t
riscv32_linux_fd_install_copy(uint32_t fd,
    const riscv32_linux_fd_t *source)
{
	riscv32_linux_process_fd_t *entry;

	if (source == (const riscv32_linux_fd_t *)0 ||
	    riscv32_linux_ofd_get(source->pool_id) ==
	    (riscv32_linux_fd_t *)0) {
		return FALSE;
	}
	entry = riscv32_linux_fd_entry(fd);
	if (entry == (riscv32_linux_process_fd_t *)0 ||
	    !riscv32_linux_ofd_retain(source->pool_id)) {
		return FALSE;
	}
	riscv32_linux_fd_entry_clear(entry);
	entry->ofd_id = source->pool_id;
	entry->descriptor_flags = source->descriptor_flags &
	    RISCV32_LINUX_FD_CLOEXEC;
	return TRUE;
}

static boolean_t
riscv32_linux_fd_alloc_copy(const riscv32_linux_fd_t *source,
    uint32_t min_fd, uint32_t *fd_out)
{
	uint32_t start;
	uint32_t i;

	start = min_fd;
	if (start < RISCV32_LINUX_FD_FIRST) {
		start = RISCV32_LINUX_FD_FIRST;
	}
	for (i = start - RISCV32_LINUX_FD_FIRST; i < RISCV32_LINUX_FD_MAX; i++) {
		if (riscv32_linux_fds[i].ofd_id != 0U) {
			continue;
		}
		if (!riscv32_linux_ofd_retain(source->pool_id)) {
			return FALSE;
		}
		riscv32_linux_fds[i].ofd_id = source->pool_id;
		riscv32_linux_fds[i].descriptor_flags =
		    source->descriptor_flags & RISCV32_LINUX_FD_CLOEXEC;
		if (fd_out != (uint32_t *)0) {
			*fd_out = RISCV32_LINUX_FD_FIRST + i;
		}
		return TRUE;
	}
	return FALSE;
}

static boolean_t
riscv32_linux_fd_alloc_cloned_description(const riscv32_linux_fd_t *source,
    uint32_t min_fd, uint32_t *fd_out)
{
	riscv32_linux_fd_t *destination;
	uint32_t fd;
	uint32_t magic;
	uint32_t pool_id;

	if (source == (const riscv32_linux_fd_t *)0 ||
	    !riscv32_linux_fd_alloc(source->kind, source->accmode,
	    &source->file, &fd) || fd < min_fd) {
		return FALSE;
	}
	destination = riscv32_linux_fd_get(fd);
	if (destination == (riscv32_linux_fd_t *)0) {
		return FALSE;
	}
	magic = destination->magic;
	pool_id = destination->pool_id;
	*destination = *source;
	destination->magic = magic;
	destination->pool_id = pool_id;
	destination->descriptor_flags = 0U;
	if (destination->kind == RISCV32_LINUX_FD_PIPE) {
		riscv32_linux_pipe_ref_fd(destination);
	}
	if (fd_out != (uint32_t *)0) {
		*fd_out = fd;
	}
	return TRUE;
}

uint32_t
riscv32_linux_syscall_close_cloexec_current(void)
{
	riscv32_linux_personality_state_t *personality =
	    RISCV32_LINUX_CURRENT_STATE();
	uint32_t closed = 0U;
	uint32_t i;

	if (personality->stdio_ready != 0U) {
		for (i = 0U; i < RISCV32_LINUX_PROCESS_STDIO_COUNT; i++) {
			if (personality->stdio[i].ofd_id != 0U &&
			    (personality->stdio[i].descriptor_flags &
			    RISCV32_LINUX_FD_CLOEXEC) != 0U) {
				riscv32_linux_fd_entry_clear(
				    &personality->stdio[i]);
				closed++;
			}
		}
	}
	for (i = 0U; i < RISCV32_LINUX_PROCESS_FD_MAX; i++) {
		if (personality->fds[i].ofd_id != 0U &&
		    (personality->fds[i].descriptor_flags &
		    RISCV32_LINUX_FD_CLOEXEC) != 0U) {
			riscv32_linux_fd_entry_clear(&personality->fds[i]);
			closed++;
		}
	}
	return closed;
}

static boolean_t
riscv32_linux_fd_is_console(uint32_t fd)
{
	riscv32_linux_fd_t source;

	return riscv32_linux_fd_snapshot(fd, &source) &&
	    source.kind == RISCV32_LINUX_FD_CONSOLE;
}

static boolean_t
riscv32_linux_fd_is_output(uint32_t fd)
{
	riscv32_linux_fd_t source;

	if (!riscv32_linux_fd_snapshot(fd, &source)) {
		return FALSE;
	}
	return (source.kind == RISCV32_LINUX_FD_CONSOLE ||
	    source.kind == RISCV32_LINUX_FD_NULL ||
	    source.kind == RISCV32_LINUX_FD_PIPE ||
	    source.kind == RISCV32_LINUX_FD_EVENTFD) &&
	    source.accmode != RISCV32_LINUX_O_RDONLY;
}

static void
riscv32_linux_put_u32le(uint8_t *buf, uint32_t offset, uint32_t value)
{
	buf[offset] = (uint8_t)value;
	buf[offset + 1U] = (uint8_t)(value >> 8);
	buf[offset + 2U] = (uint8_t)(value >> 16);
	buf[offset + 3U] = (uint8_t)(value >> 24);
}

static uint32_t
riscv32_linux_get_u32le(const uint8_t *buf, uint32_t offset)
{
	return (uint32_t)buf[offset] |
	    ((uint32_t)buf[offset + 1U] << 8) |
	    ((uint32_t)buf[offset + 2U] << 16) |
	    ((uint32_t)buf[offset + 3U] << 24);
}

static uint64_t
riscv32_linux_get_u64le(const uint8_t *buf, uint32_t offset)
{
	return (uint64_t)riscv32_linux_get_u32le(buf, offset) |
	    ((uint64_t)riscv32_linux_get_u32le(buf, offset + 4U) << 32);
}

static uint32_t
riscv32_linux_get_u16le(const uint8_t *buf, uint32_t offset)
{
	return (uint32_t)buf[offset] |
	    ((uint32_t)buf[offset + 1U] << 8);
}

static void
riscv32_linux_put_u16le(uint8_t *buf, uint32_t offset, uint32_t value)
{
	buf[offset] = (uint8_t)value;
	buf[offset + 1U] = (uint8_t)(value >> 8);
}

static void
riscv32_linux_put_u64le(uint8_t *buf, uint32_t offset, uint64_t value)
{
	riscv32_linux_put_u32le(buf, offset, (uint32_t)value);
	riscv32_linux_put_u32le(buf, offset + 4U, (uint32_t)(value >> 32));
}

static boolean_t
riscv32_linux_copyout_zero(uint32_t user_addr, uint32_t size)
{
	uint8_t zero[64];
	uint32_t copied;
	uint32_t chunk_len;

	riscv32_linux_zero(zero, sizeof(zero));
	if (!riscv32_user_range_valid(user_addr, size)) {
		return FALSE;
	}
	for (copied = 0; copied < size; copied += chunk_len) {
		chunk_len = size - copied;
		if (chunk_len > sizeof(zero)) {
			chunk_len = sizeof(zero);
		}
		if (riscv32_copyout(zero, user_addr + copied, chunk_len) != 0) {
			return FALSE;
		}
	}
	return TRUE;
}

static void
riscv32_linux_time_now(uint64_t *secs, uint64_t *nsecs)
{
	uint64_t now = mach_absolute_time();

	*secs = now / RISCV32_LINUX_NSEC_PER_SEC;
	*nsecs = now % RISCV32_LINUX_NSEC_PER_SEC;
}

static boolean_t
riscv32_linux_copyout_timespec32(uint32_t user_ts, uint64_t secs,
    uint64_t nsecs)
{
	uint8_t ts[8];

	riscv32_linux_zero(ts, sizeof(ts));
	riscv32_linux_put_u32le(ts, 0, (uint32_t)secs);
	riscv32_linux_put_u32le(ts, 4, (uint32_t)nsecs);
	return riscv32_copyout(ts, user_ts, sizeof(ts)) == 0;
}

static boolean_t
riscv32_linux_copyout_timespec64(uint32_t user_ts, uint64_t secs,
    uint64_t nsecs)
{
	uint8_t ts[16];

	riscv32_linux_zero(ts, sizeof(ts));
	riscv32_linux_put_u64le(ts, 0, secs);
	riscv32_linux_put_u64le(ts, 8, nsecs);
	return riscv32_copyout(ts, user_ts, sizeof(ts)) == 0;
}

static boolean_t
riscv32_linux_copyout_timeval32(uint32_t user_tv, uint64_t secs,
    uint64_t usecs)
{
	uint8_t tv[8];

	riscv32_linux_zero(tv, sizeof(tv));
	riscv32_linux_put_u32le(tv, 0, (uint32_t)secs);
	riscv32_linux_put_u32le(tv, 4, (uint32_t)usecs);
	return riscv32_copyout(tv, user_tv, sizeof(tv)) == 0;
}

static boolean_t
riscv32_linux_validate_timespec(uint32_t user_ts, boolean_t time64)
{
	uint8_t ts[16];
	uint32_t size = time64 ? 16U : 8U;

	if (user_ts == 0) {
		return FALSE;
	}
	return riscv32_copyin(user_ts, ts, size) == 0;
}

static uint32_t
riscv32_linux_copyin_timespec_duration(uint32_t user_ts, boolean_t time64,
    uint64_t *duration_ns)
{
	uint8_t ts[16];
	uint32_t size = time64 ? 16U : 8U;
	uint64_t secs;
	uint64_t nsecs;

	if (user_ts == 0U || duration_ns == (uint64_t *)0 ||
	    riscv32_copyin(user_ts, ts, size) != 0) {
		return RISCV32_LINUX_EFAULT;
	}
	if (time64) {
		secs = riscv32_linux_get_u64le(ts, 0U);
		nsecs = riscv32_linux_get_u64le(ts, 8U);
		if (secs > 0x7fffffffffffffffULL ||
		    nsecs > 0x7fffffffffffffffULL) {
			return RISCV32_LINUX_EINVAL;
		}
	} else {
		secs = riscv32_linux_get_u32le(ts, 0U);
		nsecs = riscv32_linux_get_u32le(ts, 4U);
		if (secs > 0x7fffffffU || nsecs > 0x7fffffffU) {
			return RISCV32_LINUX_EINVAL;
		}
	}
	if (nsecs >= NSEC_PER_SEC ||
	    secs > (UINT64_MAX - nsecs) / NSEC_PER_SEC) {
		return RISCV32_LINUX_EINVAL;
	}
	*duration_ns = secs * NSEC_PER_SEC + nsecs;
	return 0U;
}

static boolean_t
riscv32_linux_copyout_rlimit64(uint32_t user_rlim, uint64_t cur,
    uint64_t max)
{
	uint8_t rlim[16];

	riscv32_linux_zero(rlim, sizeof(rlim));
	riscv32_linux_put_u64le(rlim, 0, cur);
	riscv32_linux_put_u64le(rlim, 8, max);
	return riscv32_copyout(rlim, user_rlim, sizeof(rlim)) == 0;
}

static boolean_t
riscv32_linux_copyout_u32(uint32_t user_addr, uint32_t value)
{
	uint8_t out[4];

	riscv32_linux_put_u32le(out, 0, value);
	return riscv32_copyout(out, user_addr, sizeof(out)) == 0;
}

static boolean_t
riscv32_linux_copyout_sysinfo(uint32_t user_info)
{
	uint8_t info[RISCV32_LINUX_SYSINFO_SIZE];
	uint64_t secs;
	uint64_t nsecs;
	uint32_t total_ram = riscv32_linux_boot_total_ram_bytes();
	uint32_t free_ram = riscv32_linux_boot_usable_ram_bytes();

	if (free_ram > total_ram) {
		free_ram = total_ram / 2U;
	}

	riscv32_linux_time_now(&secs, &nsecs);
	(void)nsecs;
	riscv32_linux_zero(info, sizeof(info));
	riscv32_linux_put_u32le(info, 0, (uint32_t)secs);
	riscv32_linux_put_u32le(info, 16, total_ram);
	riscv32_linux_put_u32le(info, 20, free_ram);
	riscv32_linux_put_u16le(info, 40, 1);
	riscv32_linux_put_u32le(info, 52, 1);
	return riscv32_copyout(info, user_info, sizeof(info)) == 0;
}

static boolean_t
riscv32_linux_streq(const char *a, const char *b)
{
	uint32_t i;

	for (i = 0; a[i] != '\0' || b[i] != '\0'; i++) {
		if (a[i] != b[i]) {
			return FALSE;
		}
	}
	return TRUE;
}

static boolean_t
riscv32_linux_startswith(const char *text, const char *prefix)
{
	uint32_t i;

	for (i = 0; prefix[i] != '\0'; i++) {
		if (text[i] != prefix[i]) {
			return FALSE;
		}
	}
	return TRUE;
}

static uint32_t
riscv32_linux_strlen(const char *s)
{
	uint32_t len = 0;

	while (s[len] != '\0') {
		len++;
	}
	return len;
}

static void
riscv32_linux_copy_cstr(char *dst, uint32_t dst_size, const char *src)
{
	uint32_t i;

	if (dst_size == 0) {
		return;
	}
	if (src == (const char *)0) {
		dst[0] = '\0';
		return;
	}
	for (i = 0; i + 1U < dst_size && src[i] != '\0'; i++) {
		dst[i] = src[i];
	}
	dst[i] = '\0';
}

static void
riscv32_linux_text_append_char(char **cursor, uint32_t *remaining, char value)
{
	if (*remaining <= 1U) {
		return;
	}
	**cursor = value;
	(*cursor)++;
	(*remaining)--;
	**cursor = '\0';
}

static void
riscv32_linux_text_append_cstr(char **cursor, uint32_t *remaining,
    const char *value)
{
	if (value == (const char *)0) {
		value = "";
	}
	for (; *value != '\0'; value++) {
		riscv32_linux_text_append_char(cursor, remaining, *value);
	}
}

static void
riscv32_linux_text_append_dec(char **cursor, uint32_t *remaining,
    uint32_t value)
{
	char digits[10];
	uint32_t count = 0;

	if (value == 0U) {
		riscv32_linux_text_append_char(cursor, remaining, '0');
		return;
	}
	while (value != 0U && count < sizeof(digits)) {
		digits[count++] = (char)('0' + (value % 10U));
		value /= 10U;
	}
	while (count != 0U) {
		riscv32_linux_text_append_char(cursor, remaining,
		    digits[--count]);
	}
}

static void
riscv32_linux_text_append_hex32(char **cursor, uint32_t *remaining,
    uint32_t value)
{
	static const char digits[] = "0123456789abcdef";
	int i;

	riscv32_linux_text_append_cstr(cursor, remaining, "0x");
	for (i = 7; i >= 0; i--) {
		riscv32_linux_text_append_char(cursor, remaining,
		    digits[(value >> (uint32_t)(i * 4)) & 0xfU]);
	}
}

static void
riscv32_linux_text_append_hex8_raw(char **cursor, uint32_t *remaining,
    uint32_t value)
{
	static const char digits[] = "0123456789abcdef";
	int i;

	for (i = 7; i >= 0; i--) {
		riscv32_linux_text_append_char(cursor, remaining,
		    digits[(value >> (uint32_t)(i * 4)) & 0xfU]);
	}
}

static void
riscv32_linux_text_append_hex64(char **cursor, uint32_t *remaining,
    uint64_t value)
{
	static const char digits[] = "0123456789abcdef";
	int i;

	riscv32_linux_text_append_cstr(cursor, remaining, "0x");
	for (i = 15; i >= 0; i--) {
		riscv32_linux_text_append_char(cursor, remaining,
		    digits[(uint32_t)(value >> (uint32_t)(i * 4)) & 0xfU]);
	}
}

static uint32_t
riscv32_linux_u64_to_u32_cap(uint64_t value)
{
	return value > UINT32_MAX ? UINT32_MAX : (uint32_t)value;
}

static uint32_t
riscv32_linux_boot_total_ram_bytes(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	uint64_t total = RISCV32_LINUX_BOOT_RAM_SIZE;

	if (bootinfo != (const riscv32_bootinfo_t *)0) {
		if (bootinfo->memory_total_size != 0) {
			total = bootinfo->memory_total_size;
		} else if (bootinfo->mem_size != 0) {
			total = bootinfo->mem_size;
		}
	}
	return riscv32_linux_u64_to_u32_cap(total);
}

static uint32_t
riscv32_linux_boot_usable_ram_bytes(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	uint64_t usable = (uint64_t)riscv32_linux_boot_total_ram_bytes() / 2U;

	if (bootinfo != (const riscv32_bootinfo_t *)0 &&
	    bootinfo->memory_usable_pages != 0) {
		usable = (uint64_t)bootinfo->memory_usable_pages *
		    RISCV32_LINUX_MMAP_PAGE_SIZE;
	}
	return riscv32_linux_u64_to_u32_cap(usable);
}

static const char *
riscv32_linux_memory_map_source_name(uint32_t source)
{
	switch (source) {
	case 1:
		return "CaribeBootX";
	case 2:
		return "XNU-synthesized";
	default:
		break;
	}
	return "unknown";
}

static const char *
riscv32_linux_efi_memory_type_name(uint32_t type)
{
	switch (type) {
	case kEfiReservedMemoryType:
		return "reserved";
	case kEfiLoaderCode:
		return "loader-code";
	case kEfiLoaderData:
		return "loader-data";
	case kEfiBootServicesCode:
		return "boot-services-code";
	case kEfiBootServicesData:
		return "boot-services-data";
	case kEfiRuntimeServicesCode:
		return "runtime-code";
	case kEfiRuntimeServicesData:
		return "runtime-data";
	case kEfiConventionalMemory:
		return "conventional";
	case kEfiUnusableMemory:
		return "unusable";
	case kEfiACPIReclaimMemory:
		return "acpi-reclaim";
	case kEfiACPIMemoryNVS:
		return "acpi-nvs";
	case kEfiMemoryMappedIO:
		return "mmio";
	case kEfiMemoryMappedIOPortSpace:
		return "mmio-port";
	case kEfiPalCode:
		return "pal-code";
	default:
		break;
	}
	return "unknown";
}

static const char *
riscv32_linux_proc_version_text(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	char *cursor = riscv32_linux_proc_version;
	uint32_t remaining = sizeof(riscv32_linux_proc_version);
	const char *isa = "rv32imac_zicsr_zifencei";

	riscv32_linux_zero(riscv32_linux_proc_version,
	    sizeof(riscv32_linux_proc_version));
	if (bootinfo != (const riscv32_bootinfo_t *)0 &&
	    bootinfo->cpu_isa[0] != '\0') {
		isa = bootinfo->cpu_isa;
	}

	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "Linux version 0.0.1-caribe (XNU-CaribeOS stage0) ");
	riscv32_linux_text_append_cstr(&cursor, &remaining, isa);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	return riscv32_linux_proc_version;
}

static const char *
riscv32_linux_proc_cpuinfo_text(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	char *cursor = riscv32_linux_proc_cpuinfo;
	uint32_t remaining = sizeof(riscv32_linux_proc_cpuinfo);
	uint32_t i;

	riscv32_linux_zero(riscv32_linux_proc_cpuinfo,
	    sizeof(riscv32_linux_proc_cpuinfo));
	if (bootinfo == (const riscv32_bootinfo_t *)0) {
		return "";
	}

	for (i = 0; i < bootinfo->cpu_count && i < RISCV32_FDT_MAX_CPUS; i++) {
		riscv32_linux_text_append_cstr(&cursor, &remaining,
		    "processor\t: ");
		riscv32_linux_text_append_dec(&cursor, &remaining, i);
		riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
		riscv32_linux_text_append_cstr(&cursor, &remaining, "hart\t\t: ");
		riscv32_linux_text_append_dec(&cursor, &remaining,
		    bootinfo->hart_ids[i]);
		riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
		riscv32_linux_text_append_cstr(&cursor, &remaining, "isa\t\t: ");
		riscv32_linux_text_append_cstr(&cursor, &remaining,
		    bootinfo->cpu_isa);
		riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
		riscv32_linux_text_append_cstr(&cursor, &remaining, "mmu\t\t: ");
		riscv32_linux_text_append_cstr(&cursor, &remaining,
		    bootinfo->cpu_mmu_type);
		riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
		riscv32_linux_text_append_cstr(&cursor, &remaining,
		    "compatible\t: ");
		riscv32_linux_text_append_cstr(&cursor, &remaining,
		    bootinfo->cpu_compatible);
		riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
		riscv32_linux_text_append_cstr(&cursor, &remaining,
		    "mvendorid\t: ");
		riscv32_linux_text_append_hex32(&cursor, &remaining,
		    bootinfo->sbi_mvendorid);
		riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
		riscv32_linux_text_append_cstr(&cursor, &remaining,
		    "marchid\t\t: ");
		riscv32_linux_text_append_hex32(&cursor, &remaining,
		    bootinfo->sbi_marchid);
		riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
		riscv32_linux_text_append_cstr(&cursor, &remaining,
		    "mimpid\t\t: ");
		riscv32_linux_text_append_hex32(&cursor, &remaining,
		    bootinfo->sbi_mimpid);
		riscv32_linux_text_append_cstr(&cursor, &remaining, "\n\n");
	}
	return riscv32_linux_proc_cpuinfo;
}

static const char *
riscv32_linux_proc_meminfo_text(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	char *cursor = riscv32_linux_proc_meminfo;
	uint32_t remaining = sizeof(riscv32_linux_proc_meminfo);
	uint32_t total_kb = RISCV32_LINUX_BOOT_RAM_SIZE / 1024U;
	uint32_t free_kb;

	riscv32_linux_zero(riscv32_linux_proc_meminfo,
	    sizeof(riscv32_linux_proc_meminfo));
	if (bootinfo != (const riscv32_bootinfo_t *)0 &&
	    bootinfo->mem_size != 0U) {
		total_kb = bootinfo->mem_size / 1024U;
	}
	free_kb = total_kb / 2U;

	riscv32_linux_text_append_cstr(&cursor, &remaining, "MemTotal:        ");
	riscv32_linux_text_append_dec(&cursor, &remaining, total_kb);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " kB\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining, "MemFree:         ");
	riscv32_linux_text_append_dec(&cursor, &remaining, free_kb);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " kB\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "MemAvailable:    ");
	riscv32_linux_text_append_dec(&cursor, &remaining, free_kb);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " kB\n");
	return riscv32_linux_proc_meminfo;
}

static const char *
riscv32_linux_proc_uptime_text(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	char *cursor = riscv32_linux_proc_uptime;
	uint32_t remaining = sizeof(riscv32_linux_proc_uptime);
	uint64_t secs;
	uint64_t nsecs;
	uint32_t centis;
	uint32_t harts = 1U;
	uint64_t idle_centis64;
	uint32_t idle_secs;
	uint32_t idle_centis;

	riscv32_linux_zero(riscv32_linux_proc_uptime,
	    sizeof(riscv32_linux_proc_uptime));
	riscv32_linux_time_now(&secs, &nsecs);
	centis = (uint32_t)(nsecs / 10000000ULL);
	if (bootinfo != (const riscv32_bootinfo_t *)0 &&
	    bootinfo->cpu_count != 0U) {
		harts = bootinfo->cpu_count;
	}
	idle_centis64 = ((secs * 100ULL) + centis) * harts;
	idle_secs = (uint32_t)(idle_centis64 / 100ULL);
	idle_centis = (uint32_t)(idle_centis64 % 100ULL);

	riscv32_linux_text_append_dec(&cursor, &remaining, (uint32_t)secs);
	riscv32_linux_text_append_char(&cursor, &remaining, '.');
	riscv32_linux_text_append_char(&cursor, &remaining,
	    (char)('0' + (centis / 10U)));
	riscv32_linux_text_append_char(&cursor, &remaining,
	    (char)('0' + (centis % 10U)));
	riscv32_linux_text_append_char(&cursor, &remaining, ' ');
	riscv32_linux_text_append_dec(&cursor, &remaining, idle_secs);
	riscv32_linux_text_append_char(&cursor, &remaining, '.');
	riscv32_linux_text_append_char(&cursor, &remaining,
	    (char)('0' + (idle_centis / 10U)));
	riscv32_linux_text_append_char(&cursor, &remaining,
	    (char)('0' + (idle_centis % 10U)));
	riscv32_linux_text_append_char(&cursor, &remaining, '\n');
	return riscv32_linux_proc_uptime;
}

static const char *
riscv32_linux_proc_stat_text(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	riscv32_linux_process_stats_t process_stats;
	char *cursor = riscv32_linux_proc_stat;
	uint32_t remaining = sizeof(riscv32_linux_proc_stat);
	uint64_t secs;
	uint64_t nsecs;
	uint32_t ticks;
	uint32_t harts = 1U;
	uint32_t idle_ticks;
	uint32_t i;

	riscv32_linux_zero(riscv32_linux_proc_stat,
	    sizeof(riscv32_linux_proc_stat));
	riscv32_linux_time_now(&secs, &nsecs);
	ticks = (uint32_t)((secs * 100ULL) + (nsecs / 10000000ULL));
	if (bootinfo != (const riscv32_bootinfo_t *)0 &&
	    bootinfo->cpu_count != 0U) {
		harts = bootinfo->cpu_count;
	}
	idle_ticks = ticks * harts;

	riscv32_linux_text_append_cstr(&cursor, &remaining, "cpu  0 0 0 ");
	riscv32_linux_text_append_dec(&cursor, &remaining, idle_ticks);
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    " 0 0 0 0 0 0\n");
	for (i = 0; i < harts && i < RISCV32_FDT_MAX_CPUS; i++) {
		riscv32_linux_text_append_cstr(&cursor, &remaining, "cpu");
		riscv32_linux_text_append_dec(&cursor, &remaining, i);
		riscv32_linux_text_append_cstr(&cursor, &remaining, " 0 0 0 ");
		riscv32_linux_text_append_dec(&cursor, &remaining, ticks);
		riscv32_linux_text_append_cstr(&cursor, &remaining,
		    " 0 0 0 0 0 0\n");
	}
	riscv32_linux_text_append_cstr(&cursor, &remaining, "intr 0\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining, "ctxt 0\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining, "btime 0\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining, "processes ");
	riscv32_linux_process_get_stats(&process_stats);
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    process_stats.total_created != 0U ?
	    process_stats.total_created : 1U);
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nprocs_running ");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    process_stats.live_processes != 0U ?
	    process_stats.live_processes : 1U);
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nprocs_blocked 0\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "softirq 0 0 0 0 0 0 0 0 0 0 0\n");
	return riscv32_linux_proc_stat;
}

static const char *
riscv32_linux_sbi_impl_name(uint32_t impl_id)
{
	switch (impl_id) {
	case 1:
		return "OpenSBI";
	default:
		break;
	}
	return "unknown";
}

static void
riscv32_linux_text_append_sbi_exts(char **cursor, uint32_t *remaining,
    uint32_t extensions)
{
	if (extensions == 0U) {
		riscv32_linux_text_append_cstr(cursor, remaining, " none");
		return;
	}
	if (extensions & RISCV32_SBI_EXTBIT_TIME) {
		riscv32_linux_text_append_cstr(cursor, remaining, " time");
	}
	if (extensions & RISCV32_SBI_EXTBIT_HSM) {
		riscv32_linux_text_append_cstr(cursor, remaining, " hsm");
	}
	if (extensions & RISCV32_SBI_EXTBIT_IPI) {
		riscv32_linux_text_append_cstr(cursor, remaining, " ipi");
	}
	if (extensions & RISCV32_SBI_EXTBIT_RFENCE) {
		riscv32_linux_text_append_cstr(cursor, remaining, " rfence");
	}
	if (extensions & RISCV32_SBI_EXTBIT_SRST) {
		riscv32_linux_text_append_cstr(cursor, remaining, " srst");
	}
	if (extensions & RISCV32_SBI_EXTBIT_PMU) {
		riscv32_linux_text_append_cstr(cursor, remaining, " pmu");
	}
	if (extensions & RISCV32_SBI_EXTBIT_DBCN) {
		riscv32_linux_text_append_cstr(cursor, remaining, " dbcn");
	}
}

static void
riscv32_linux_text_append_available(char **cursor, uint32_t *remaining,
    uint32_t extensions, uint32_t bit, const char *name)
{
	riscv32_linux_text_append_cstr(cursor, remaining, name);
	riscv32_linux_text_append_cstr(cursor, remaining,
	    (extensions & bit) != 0 ? "=yes" : "=no");
}

static const char *
riscv32_linux_proc_caribeos_text(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	char *cursor = riscv32_linux_proc_caribeos;
	uint32_t remaining = sizeof(riscv32_linux_proc_caribeos);
	uint32_t i;

	riscv32_linux_zero(riscv32_linux_proc_caribeos,
	    sizeof(riscv32_linux_proc_caribeos));
	if (bootinfo == (const riscv32_bootinfo_t *)0) {
		return "";
	}

	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "bootloader\t: ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    bootinfo->bootloader_present ? "CaribeBootX" : "fallback");
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\ncbx_version\t: ");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->bootloader_version);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\ncbx_args\t: ");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->bootloader_args_paddr);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\ncbx_cmdline\t: ");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->bootloader_cmdline_paddr);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "+");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->bootloader_cmdline_size);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\ncbx_mmap\t: ");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->bootloader_memory_map_paddr);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " count=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->bootloader_memory_map_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " desc=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->bootloader_memory_map_desc_size);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\ncbx_ack\t\t: ");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    RISCV32_CARIBEBOOTX_ACK_MAGIC);
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nopensbi_spec\t: ");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->sbi_spec_version);
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nopensbi_impl\t: ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    riscv32_linux_sbi_impl_name(bootinfo->sbi_impl_id));
	riscv32_linux_text_append_cstr(&cursor, &remaining, " ");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->sbi_impl_id);
	riscv32_linux_text_append_cstr(&cursor, &remaining, ":");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->sbi_impl_version);
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nopensbi_ext\t: ");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->sbi_extensions);
	riscv32_linux_text_append_sbi_exts(&cursor, &remaining,
	    bootinfo->sbi_extensions);
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nsbi_runtime\t: ");
	riscv32_linux_text_append_available(&cursor, &remaining,
	    bootinfo->sbi_extensions, RISCV32_SBI_EXTBIT_SRST, "reset");
	riscv32_linux_text_append_cstr(&cursor, &remaining, " ");
	riscv32_linux_text_append_available(&cursor, &remaining,
	    bootinfo->sbi_extensions, RISCV32_SBI_EXTBIT_DBCN, "debug_console");
	riscv32_linux_text_append_cstr(&cursor, &remaining, " ");
	riscv32_linux_text_append_available(&cursor, &remaining,
	    bootinfo->sbi_extensions, RISCV32_SBI_EXTBIT_IPI, "ipi");
	riscv32_linux_text_append_cstr(&cursor, &remaining, " ");
	riscv32_linux_text_append_available(&cursor, &remaining,
	    bootinfo->sbi_extensions, RISCV32_SBI_EXTBIT_RFENCE, "rfence");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nloader_mem\t: ");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->loader_mem_base);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "+");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->loader_mem_size);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nfdt_mem\t\t: ");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->fdt_mem_base);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "+");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->fdt_mem_size);
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nfdt_model\t: ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    bootinfo->fdt_model[0] != '\0' ?
	    bootinfo->fdt_model : "unknown");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nfdt_compatible\t: ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    bootinfo->fdt_root_compatible[0] != '\0' ?
	    bootinfo->fdt_root_compatible : "unknown");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nfdt_chosen\t: bootargs=");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    bootinfo->fdt_chosen_bootargs[0] != '\0' ?
	    bootinfo->fdt_chosen_bootargs : "<none>");
	riscv32_linux_text_append_cstr(&cursor, &remaining, " stdout=");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    bootinfo->fdt_chosen_stdout_path[0] != '\0' ?
	    bootinfo->fdt_chosen_stdout_path : "<none>");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nfdt_inventory\t: nodes=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->fdt_node_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " mmio=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->fdt_mmio_device_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " irqctl=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->fdt_interrupt_controller_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " reserved=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->fdt_reserved_region_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " reserved_kb=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    ((uint32_t)bootinfo->fdt_reserved_total_size) / 1024U);
	for (i = 0; i < bootinfo->fdt_reserved_region_count &&
	    i < RISCV32_FDT_MAX_RESERVED_REGIONS; i++) {
		riscv32_linux_text_append_cstr(&cursor, &remaining,
		    "\nfdt_reserved");
		riscv32_linux_text_append_dec(&cursor, &remaining, i);
		riscv32_linux_text_append_cstr(&cursor, &remaining, "\t: ");
		riscv32_linux_text_append_hex32(&cursor, &remaining,
		    bootinfo->fdt_reserved_regions[i].base);
		riscv32_linux_text_append_cstr(&cursor, &remaining, "+");
		riscv32_linux_text_append_hex32(&cursor, &remaining,
		    bootinfo->fdt_reserved_regions[i].size);
	}
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nactive_mem\t: ");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->mem_base);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "+");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->mem_size);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\ntotal_mem_kb\t: ");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    ((uint32_t)bootinfo->memory_total_size) / 1024U);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nram_banks\t: ");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->memory_bank_count);
	for (i = 0; i < bootinfo->memory_bank_count &&
	    i < RISCV32_FDT_MAX_MEMORY_REGIONS; i++) {
		riscv32_linux_text_append_cstr(&cursor, &remaining, "\nram");
		riscv32_linux_text_append_dec(&cursor, &remaining, i);
		riscv32_linux_text_append_cstr(&cursor, &remaining, "\t\t: ");
		riscv32_linux_text_append_hex32(&cursor, &remaining,
		    bootinfo->memory_banks[i].base);
		riscv32_linux_text_append_cstr(&cursor, &remaining, "+");
		riscv32_linux_text_append_hex32(&cursor, &remaining,
		    bootinfo->memory_banks[i].size);
	}
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nmem_pages\t: total=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->memory_total_pages);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " usable=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->memory_usable_pages);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " reserved=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->memory_reserved_pages);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	return riscv32_linux_proc_caribeos;
}

static const char *
riscv32_linux_proc_caribe_sbi_text(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	char *cursor = riscv32_linux_proc_caribe_sbi;
	uint32_t remaining = sizeof(riscv32_linux_proc_caribe_sbi);

	riscv32_linux_zero(riscv32_linux_proc_caribe_sbi,
	    sizeof(riscv32_linux_proc_caribe_sbi));
	if (bootinfo == (const riscv32_bootinfo_t *)0) {
		return "";
	}

	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "CaribeOS SBI runtime\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "firmware\t: ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    riscv32_linux_sbi_impl_name(bootinfo->sbi_impl_id));
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nimplementation\t: ");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->sbi_impl_id);
	riscv32_linux_text_append_cstr(&cursor, &remaining, ":");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->sbi_impl_version);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nspec\t\t: ");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->sbi_spec_version);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nextensions\t: ");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->sbi_extensions);
	riscv32_linux_text_append_sbi_exts(&cursor, &remaining,
	    bootinfo->sbi_extensions);
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nfeature_time\t: ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    (bootinfo->sbi_extensions & RISCV32_SBI_EXTBIT_TIME) != 0 ?
	    "yes" : "no");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nfeature_hsm\t: ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    (bootinfo->sbi_extensions & RISCV32_SBI_EXTBIT_HSM) != 0 ?
	    "yes" : "no");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nfeature_ipi\t: ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    (bootinfo->sbi_extensions & RISCV32_SBI_EXTBIT_IPI) != 0 ?
	    "yes" : "no");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nfeature_rfence\t: ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    (bootinfo->sbi_extensions & RISCV32_SBI_EXTBIT_RFENCE) != 0 ?
	    "yes" : "no");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nfeature_srst\t: ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    (bootinfo->sbi_extensions & RISCV32_SBI_EXTBIT_SRST) != 0 ?
	    "yes" : "no");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nfeature_pmu\t: ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    (bootinfo->sbi_extensions & RISCV32_SBI_EXTBIT_PMU) != 0 ?
	    "yes" : "no");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nfeature_dbcn\t: ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    (bootinfo->sbi_extensions & RISCV32_SBI_EXTBIT_DBCN) != 0 ?
	    "yes" : "no");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\ntimebase_hz\t: ");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->timebase_hz);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nmvendorid\t: ");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->sbi_mvendorid);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nmarchid\t\t: ");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->sbi_marchid);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nmimpid\t\t: ");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->sbi_mimpid);
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nnext_layer\t: CaribeBootX S-mode payload\n");
	return riscv32_linux_proc_caribe_sbi;
}

static const char *
riscv32_linux_proc_caribe_devices_text(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	char *cursor = riscv32_linux_proc_caribe_devices;
	uint32_t remaining = sizeof(riscv32_linux_proc_caribe_devices);
	uint32_t i;

	riscv32_linux_zero(riscv32_linux_proc_caribe_devices,
	    sizeof(riscv32_linux_proc_caribe_devices));
	if (bootinfo == (const riscv32_bootinfo_t *)0) {
		return "";
	}

	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "CaribeOS RISC-V device inventory\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining, "model\t\t: ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    bootinfo->fdt_model[0] != '\0' ?
	    bootinfo->fdt_model : "unknown");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\ncompatible\t: ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    bootinfo->fdt_root_compatible[0] != '\0' ?
	    bootinfo->fdt_root_compatible : "unknown");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nfdt_nodes\t: ");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->fdt_node_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nfdt_mmio\t: ");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->fdt_mmio_device_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nfdt_irqctl\t: ");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->fdt_interrupt_controller_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nchosen_stdout\t: ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    bootinfo->fdt_chosen_stdout_path[0] != '\0' ?
	    bootinfo->fdt_chosen_stdout_path : "<none>");
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nuart0\t\t: base=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->uart_base);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " size=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->uart_size);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " irq=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->uart_irq);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nplic0\t\t: base=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->plic_base);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " size=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->plic_size);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\naclint0\t\t: base=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->aclint_base);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " size=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->aclint_size);
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nvirtio_mmio\t: count=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->virtio_count);
	for (i = 0; i < bootinfo->virtio_count &&
	    i < RISCV32_FDT_MAX_VIRTIO_MMIO; i++) {
		riscv32_linux_text_append_cstr(&cursor, &remaining,
		    "\nvirtio");
		riscv32_linux_text_append_dec(&cursor, &remaining, i);
		riscv32_linux_text_append_cstr(&cursor, &remaining, "\t: base=");
		riscv32_linux_text_append_hex32(&cursor, &remaining,
		    bootinfo->virtio[i].base);
		riscv32_linux_text_append_cstr(&cursor, &remaining, " size=");
		riscv32_linux_text_append_hex32(&cursor, &remaining,
		    bootinfo->virtio[i].size);
		riscv32_linux_text_append_cstr(&cursor, &remaining, " irq=");
		riscv32_linux_text_append_dec(&cursor, &remaining,
		    bootinfo->virtio[i].irq);
	}
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nroot_storage\t: virtio-blk HFS+ /kernel.elf + /initrd.img\n");
	return riscv32_linux_proc_caribe_devices;
}

static const char *
riscv32_linux_proc_caribe_memmap_text(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	const boot_args *args = riscv32_boot_args();
	char *cursor = riscv32_linux_proc_caribe_memmap;
	uint32_t remaining = sizeof(riscv32_linux_proc_caribe_memmap);
	uint32_t i;

	riscv32_linux_zero(riscv32_linux_proc_caribe_memmap,
	    sizeof(riscv32_linux_proc_caribe_memmap));
	if (bootinfo == (const riscv32_bootinfo_t *)0) {
		return "";
	}

	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "CaribeOS memory map\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining, "source\t\t: ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    riscv32_linux_memory_map_source_name(bootinfo->memory_map_source));
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nactive\t\t: ");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->mem_base);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "+");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->mem_size);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\ntotal_kb\t: ");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    ((uint32_t)bootinfo->memory_total_size) / 1024U);
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\npages\t\t: total=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->memory_total_pages);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " usable=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->memory_usable_pages);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " reserved=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->memory_reserved_pages);
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nfirst_free\t: ");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->memory_first_free);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nmax_end\t\t: ");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->memory_max_end);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\noverlaps\t: ");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->memory_overlaps);
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\ncbx_mmap\t: ");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->bootloader_memory_map_paddr);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " count=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->bootloader_memory_map_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " desc=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->bootloader_memory_map_desc_size);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nram_banks\t: ");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->memory_bank_count);
	for (i = 0; i < bootinfo->memory_bank_count &&
	    i < RISCV32_FDT_MAX_MEMORY_REGIONS; i++) {
		riscv32_linux_text_append_cstr(&cursor, &remaining, "\nram");
		riscv32_linux_text_append_dec(&cursor, &remaining, i);
		riscv32_linux_text_append_cstr(&cursor, &remaining, "\t\t: ");
		riscv32_linux_text_append_hex32(&cursor, &remaining,
		    bootinfo->memory_banks[i].base);
		riscv32_linux_text_append_cstr(&cursor, &remaining, "+");
		riscv32_linux_text_append_hex32(&cursor, &remaining,
		    bootinfo->memory_banks[i].size);
	}
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nfdt_reserved\t: count=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->fdt_reserved_region_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " bytes=");
	riscv32_linux_text_append_hex64(&cursor, &remaining,
	    bootinfo->fdt_reserved_total_size);
	for (i = 0; i < bootinfo->fdt_reserved_region_count &&
	    i < RISCV32_FDT_MAX_RESERVED_REGIONS; i++) {
		riscv32_linux_text_append_cstr(&cursor, &remaining,
		    "\nfdt_reserved");
		riscv32_linux_text_append_dec(&cursor, &remaining, i);
		riscv32_linux_text_append_cstr(&cursor, &remaining, "\t: ");
		riscv32_linux_text_append_hex32(&cursor, &remaining,
		    bootinfo->fdt_reserved_regions[i].base);
		riscv32_linux_text_append_cstr(&cursor, &remaining, "+");
		riscv32_linux_text_append_hex32(&cursor, &remaining,
		    bootinfo->fdt_reserved_regions[i].size);
	}
	if (args != (const boot_args *)0 && args->MemoryMap != 0 &&
	    args->MemoryMapDescriptorSize >= sizeof(EfiMemoryRange)) {
		const uint8_t *map = (const uint8_t *)(uintptr_t)args->MemoryMap;
		uint32_t desc_size = args->MemoryMapDescriptorSize;
		uint32_t count = args->MemoryMapSize / desc_size;

		riscv32_linux_text_append_cstr(&cursor, &remaining,
		    "\nefi_mmap\t: ptr=");
		riscv32_linux_text_append_hex32(&cursor, &remaining,
		    args->MemoryMap);
		riscv32_linux_text_append_cstr(&cursor, &remaining, " count=");
		riscv32_linux_text_append_dec(&cursor, &remaining, count);
		riscv32_linux_text_append_cstr(&cursor, &remaining, " desc=");
		riscv32_linux_text_append_dec(&cursor, &remaining, desc_size);
		if (count > 24U) {
			count = 24U;
		}
		for (i = 0; i < count; i++) {
			const EfiMemoryRange *range =
			    (const EfiMemoryRange *)(const void *)
			    &map[i * desc_size];
			uint64_t bytes = range->NumberOfPages <<
			    (uint32_t)PAGE_SHIFT;

			riscv32_linux_text_append_cstr(&cursor, &remaining,
			    "\nefi");
			riscv32_linux_text_append_dec(&cursor, &remaining, i);
			riscv32_linux_text_append_cstr(&cursor, &remaining,
			    "\t\t: type=");
			riscv32_linux_text_append_cstr(&cursor, &remaining,
			    riscv32_linux_efi_memory_type_name(range->Type));
			riscv32_linux_text_append_cstr(&cursor, &remaining,
			    " base=");
			riscv32_linux_text_append_hex64(&cursor, &remaining,
			    range->PhysicalStart);
			riscv32_linux_text_append_cstr(&cursor, &remaining,
			    " pages=");
			riscv32_linux_text_append_dec(&cursor, &remaining,
			    riscv32_linux_u64_to_u32_cap(range->NumberOfPages));
			riscv32_linux_text_append_cstr(&cursor, &remaining,
			    " bytes=");
			riscv32_linux_text_append_hex64(&cursor, &remaining,
			    bytes);
			riscv32_linux_text_append_cstr(&cursor, &remaining,
			    " attr=");
			riscv32_linux_text_append_hex64(&cursor, &remaining,
			    range->Attribute);
		}
	} else {
		riscv32_linux_text_append_cstr(&cursor, &remaining,
		    "\nefi_mmap\t: unavailable");
	}
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	return riscv32_linux_proc_caribe_memmap;
}

static void
riscv32_linux_resource_append_u32(char **cursor, uint32_t *remaining,
    const char *name, uint32_t value)
{
	riscv32_linux_text_append_char(cursor, remaining, ' ');
	riscv32_linux_text_append_cstr(cursor, remaining, name);
	riscv32_linux_text_append_char(cursor, remaining, '=');
	riscv32_linux_text_append_dec(cursor, remaining, value);
}

static const char *
riscv32_linux_proc_caribe_resources_text(void)
{
	riscv32_linux_process_stats_t process;
	riscv32_pmap_lifecycle_stats_t pmap_stats;
	riscv32_linux_user_backing_stats_t backing;
	riscv32_linux_exec_arena_stats_t arena;
	riscv32_linux_ofd_stats_t ofd;
	riscv32_linux_pipe_stats_t pipes;
	riscv32_linux_signal_stats_t signals;
	riscv32_tty_stats_t tty;
	char *cursor = riscv32_linux_proc_caribe_resources;
	uint32_t remaining = sizeof(riscv32_linux_proc_caribe_resources);
	uint32_t zone_count;
	uint32_t zone_active = 0U;
	uint32_t zone_bytes = 0U;
	uint32_t signal_frames_active;
	uint32_t stacks_active;
	uint32_t stacks_total;
	unsigned int i;
	int stack_count = 0;
	int collectable = 0;
	int exhaustable = 0;
	int caller_acct = 0;
	vm_size_t current_stack_bytes = 0;
	vm_size_t max_stack_bytes = 0;
	vm_size_t stack_element_bytes = 0;
	vm_size_t stack_allocation_bytes = 0;
	uint64_t cumulative_stack_bytes = 0;

	riscv32_linux_zero(riscv32_linux_proc_caribe_resources,
	    sizeof(riscv32_linux_proc_caribe_resources));
	riscv32_linux_process_get_stats(&process);
	riscv32_pmap_get_lifecycle_stats(&pmap_stats);
	riscv32_linux_exec_get_user_backing_stats(&backing);
	riscv32_linux_exec_get_arena_stats(&arena);
	riscv32_linux_syscall_get_ofd_stats(&ofd);
	riscv32_linux_syscall_get_pipe_stats(&pipes);
	riscv32_linux_signal_get_stats(&signals);
	riscv32_tty_get_stats(&tty);
	stack_fake_zone_info(&stack_count, &current_stack_bytes,
	    &max_stack_bytes, &stack_element_bytes, &stack_allocation_bytes,
	    &cumulative_stack_bytes, &collectable, &exhaustable, &caller_acct);
	stacks_active = stack_count < 0 ? 0U : (uint32_t)stack_count;
	stacks_total = kernel_stack_size == 0U ? 0U :
	    (uint32_t)(current_stack_bytes / kernel_stack_size);
	zone_count = zone_lifecycle_snapshot(riscv32_linux_resource_zones,
	    RISCV32_LINUX_RESOURCE_ZONE_MAX);
	for (i = 0U; i < zone_count; i++) {
		zone_active += riscv32_linux_resource_zones[i].active_count;
		zone_bytes += (uint32_t)
		    riscv32_linux_resource_zones[i].current_size;
	}
	signal_frames_active = signals.frames_delivered >=
	    signals.frames_returned ?
	    signals.frames_delivered - signals.frames_returned : UINT32_MAX;

	riscv32_linux_text_append_cstr(&cursor, &remaining, "version=1");
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "process_capacity", process.table_capacity);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "process_entries", process.table_entries);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "process_live", process.live_processes);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "zombies", process.zombies);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "tasks", (uint32_t)tasks_count);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "threads", (uint32_t)threads_count);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "pmaps", pmap_stats.live_user_pmaps);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "pt_pages", pmap_stats.nonkernel_in_use_pages);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "kernel_pt_pages", pmap_stats.kernel_owned_pt_pages);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "user_pages", backing.in_use_pages);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "exec_arenas", arena.in_use);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "fd_refs", ofd.references);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "ofds", ofd.in_use);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "pipes", pipes.in_use);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "signal_frames", signal_frames_active);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "free_pages", vm_page_free_count);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "wired_pages", vm_page_wire_count);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "kernel_stacks_active", stacks_active);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "kernel_stacks_total", stacks_total);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "reserved_stack_handoffs", riscv32_reserved_stack_handoff_count());
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "zones", zone_count);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "zone_active", zone_active);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "zone_bytes", zone_bytes);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "process_created", process.total_created);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "process_removed", process.total_removed);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "process_exited", process.total_exited);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "process_reaped", process.total_reaped);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "pmap_created", pmap_stats.total_user_pmaps_created);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "pmap_destroyed", pmap_stats.total_user_pmaps_destroyed);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "pt_alloc", pmap_stats.total_pt_allocations);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "pt_free", pmap_stats.total_pt_frees);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "backing_alloc", backing.total_allocations);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "backing_free", backing.total_frees);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "ofd_alloc", ofd.total_allocations);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "ofd_free", ofd.total_frees);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "ofd_retain", ofd.total_retains);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "ofd_release", ofd.total_releases);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "pipe_alloc", pipes.total_allocations);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "pipe_release", pipes.total_releases);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "pipe_read_bytes", pipes.bytes_read);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "pipe_write_bytes", pipes.bytes_written);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "pipe_read_blocks", pipes.read_blocks);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "pipe_read_wakeups", pipes.read_wakeups);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "pipe_write_blocks", pipes.write_blocks);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "pipe_write_wakeups", pipes.write_wakeups);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "signal_queued", signals.signals_queued);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "signal_discarded", signals.signals_discarded);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "signal_delivered", signals.frames_delivered);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "signal_returned", signals.frames_returned);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "signal_failures", signals.frame_failures);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "tty_lines", tty.input_lines);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "tty_overruns", tty.input_overruns);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "tty_reads", tty.read_calls);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "tty_read_blocks", tty.read_blocks);
	riscv32_linux_resource_append_u32(&cursor, &remaining,
	    "tty_read_wakeups", tty.read_wakeups);
	riscv32_linux_text_append_char(&cursor, &remaining, '\n');
	return riscv32_linux_proc_caribe_resources;
}

static const char *
riscv32_linux_proc_bootlog_text(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	char *cursor = riscv32_linux_proc_bootlog;
	uint32_t remaining = sizeof(riscv32_linux_proc_bootlog);

	riscv32_linux_zero(riscv32_linux_proc_bootlog,
	    sizeof(riscv32_linux_proc_bootlog));
	if (bootinfo == (const riscv32_bootinfo_t *)0) {
		return "";
	}

	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "CaribeOS boot report\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "chain           : OpenSBI -> CaribeBootX -> XNU-CaribeOS -> /sbin/init\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "firmware        : ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    riscv32_linux_sbi_impl_name(bootinfo->sbi_impl_id));
	riscv32_linux_text_append_cstr(&cursor, &remaining, " spec=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->sbi_spec_version);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " impl=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->sbi_impl_id);
	riscv32_linux_text_append_cstr(&cursor, &remaining, ":");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->sbi_impl_version);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " ext=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->sbi_extensions);
	riscv32_linux_text_append_sbi_exts(&cursor, &remaining,
	    bootinfo->sbi_extensions);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "bootloader      : ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    bootinfo->bootloader_present ? "CaribeBootX" : "fallback");
	riscv32_linux_text_append_cstr(&cursor, &remaining, " v=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->bootloader_version);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " flags=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->bootloader_flags);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " ack=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    RISCV32_CARIBEBOOTX_ACK_MAGIC);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "cpu             : hart=");
	riscv32_linux_text_append_dec(&cursor, &remaining, bootinfo->hartid);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " harts=");
	riscv32_linux_text_append_dec(&cursor, &remaining, bootinfo->cpu_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " isa=");
	riscv32_linux_text_append_cstr(&cursor, &remaining, bootinfo->cpu_isa);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " mmu=");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    bootinfo->cpu_mmu_type);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " compatible=");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    bootinfo->cpu_compatible);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "memory          : active=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->mem_base);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "+");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->mem_size);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " total_kb=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    ((uint32_t)bootinfo->memory_total_size) / 1024U);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " banks=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->memory_bank_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " usable_pages=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->memory_usable_pages);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "image           : kernel=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->kernel_base);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "+");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->kernel_size);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " initrd=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->initrd_base);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "+");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->initrd_size);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " dtb=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->dtb_paddr);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "+");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->dtb_size);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "fdt             : model=");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    bootinfo->fdt_model[0] != '\0' ?
	    bootinfo->fdt_model : "unknown");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    " compatible=");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    bootinfo->fdt_root_compatible[0] != '\0' ?
	    bootinfo->fdt_root_compatible : "unknown");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    " nodes=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->fdt_node_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    " mmio=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->fdt_mmio_device_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    " reserved=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->fdt_reserved_region_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "devices         : uart=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->uart_base);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " irq=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->uart_irq);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " plic=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->plic_base);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " aclint=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->aclint_base);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " virtio_mmio=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->virtio_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "iokit           : PE_init_iokit=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    riscv32_iokit_stage0_pe_init_count());
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    " state=registry-seed cpp=");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    (riscv32_iokit_stage0_flags() & RISCV32_IOKIT_CPP_MICRO_START) != 0 ?
	    "micro-ioservice" :
	    ((riscv32_iokit_stage0_flags() & RISCV32_IOKIT_CPP_PROBE_VTABLE) != 0 ?
	    "probe-live" : "deferred"));
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    " bsd=deferred\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "rootfs          : initrd newc mounted at /, procfs at /proc, devtmpfs at /dev\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "userspace       : /init -> /sbin/init (/sbin/caribed), utility=/bin/caribectl\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "status          : bootable-stage1-initrd\n");
	return riscv32_linux_proc_bootlog;
}

static const char *
riscv32_linux_proc_iokit_text(void)
{
	const riscv32_bootinfo_t *bootinfo = riscv32_bootinfo();
	char *cursor = riscv32_linux_proc_iokit;
	uint32_t remaining = sizeof(riscv32_linux_proc_iokit);
	uint32_t flags = riscv32_iokit_stage0_flags();
	uint32_t i;

	riscv32_linux_zero(riscv32_linux_proc_iokit,
	    sizeof(riscv32_linux_proc_iokit));
	if (bootinfo == (const riscv32_bootinfo_t *)0) {
		return "";
	}

	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "IOKit-CaribeOS stage0\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "xnu_tree\t: xnu-2050.48.11 iokit/ + libkern/c++ present\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "config\t\t: IOKIT option enabled in MASTER.riscv32\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "bootstrap\t: PE_init_iokit calls=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    riscv32_iokit_stage0_pe_init_count());
	riscv32_linux_text_append_cstr(&cursor, &remaining, " flags=");
	riscv32_linux_text_append_hex32(&cursor, &remaining, flags);
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "\nstate\t\t: registry-seed, micro IOService runtime live\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "cpp_probe\t: linked=");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    (flags & RISCV32_IOKIT_CPP_PROBE_LINKED) != 0 ? "yes" : "no");
	riscv32_linux_text_append_cstr(&cursor, &remaining, " ctor=");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    (flags & RISCV32_IOKIT_CPP_PROBE_CTOR) != 0 ? "yes" : "no");
	riscv32_linux_text_append_cstr(&cursor, &remaining, " vtable=");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    (flags & RISCV32_IOKIT_CPP_PROBE_VTABLE) != 0 ? "yes" : "no");
	riscv32_linux_text_append_cstr(&cursor, &remaining, " service_shape=");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    (flags & RISCV32_IOKIT_CPP_PROBE_SERVICE_SHAPE) != 0 ? "yes" : "no");
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "cpp_runtime\t: ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    (flags & RISCV32_IOKIT_CPP_REAL_STARTIOKIT) != 0 ?
	    "rv32 g++ ABI live, real StartIOKit returned\n" :
	    "rv32 g++ ABI probe live, boot runtime still micro-IOService\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "cpp_link_gate\t: libkern=pass ioregistry=pass iokitbase=pass iomemory=pass platform=pass pe_rv32=pass\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "cpp_devices\t: devtree=pass irq=pass cpu=pass dma=pass nvram=pass mode=offline\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "cpp_startiokit\t: ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    (flags & RISCV32_IOKIT_CPP_REAL_STARTIOKIT) != 0 ?
	    "StartIOKit=real-returned IOService/IOCatalogue/IOWorkLoop crossed\n" :
	    "StartIOKit=pass IOService/IOCatalogue init linked mode=offline\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "cpp_boundaries\t: OSKextInit=weak diagnostics=weak SHA1=weak UUID=weak uio/cache=weak\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "cpp_next\t: ");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    (flags & RISCV32_IOKIT_CPP_REAL_STARTIOKIT) != 0 ?
	    "build FDT->IORegistry population and replace weak crypto/UUID/uio/cache\n" :
	    "wire real StartIOKit/PE/IOService into boot runtime, then retire crypto/UUID stubs\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "ioservice\t: registry=");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    (flags & RISCV32_IOKIT_CPP_MICRO_REGISTRY) != 0 ? "yes" : "no");
	riscv32_linux_text_append_cstr(&cursor, &remaining, " published=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    riscv32_iokit_micro_published_count());
	riscv32_linux_text_append_cstr(&cursor, &remaining, "/");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    riscv32_iokit_micro_service_count());
	riscv32_linux_text_append_cstr(&cursor, &remaining, " matched=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    riscv32_iokit_micro_matched_count());
	riscv32_linux_text_append_cstr(&cursor, &remaining, " started=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    riscv32_iokit_micro_started_count());
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "mach_bridge\t: is_iokit_subsystem=2800 user_client_trap=stub\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "bsd_bridge\t: IOKitBSDInit, IOFindBSDRoot, IOMedia matching deferred\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "platform\t: CaribeRiscVPlatformExpert seed over QEMU virt\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "platform\t: fdt model=");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    bootinfo->fdt_model[0] != '\0' ?
	    bootinfo->fdt_model : "unknown");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    " compatible=");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    bootinfo->fdt_root_compatible[0] != '\0' ?
	    bootinfo->fdt_root_compatible : "unknown");
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "platform\t: fdt chosen bootargs=");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    bootinfo->fdt_chosen_bootargs[0] != '\0' ?
	    bootinfo->fdt_chosen_bootargs : "<none>");
	riscv32_linux_text_append_cstr(&cursor, &remaining, " stdout=");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    bootinfo->fdt_chosen_stdout_path[0] != '\0' ?
	    bootinfo->fdt_chosen_stdout_path : "<none>");
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "platform\t: fdt nodes=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->fdt_node_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " mmio=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->fdt_mmio_device_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " irqctl=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->fdt_interrupt_controller_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " reserved=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->fdt_reserved_region_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "platform\t: fdt_legacy=");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    (flags & RISCV32_IOKIT_FDT_LEGACY_SKIPPED) != 0 ?
	    "skipped OpenSBI/CBX FDT kept in rv32 bootinfo\n" :
	    "apple-device-tree path\n");
	for (i = 0; i < riscv32_iokit_micro_registry_line_count(); i++) {
		const char *line = riscv32_iokit_micro_registry_line(i);

		if (line == (const char *)0) {
			continue;
		}
		riscv32_linux_text_append_cstr(&cursor, &remaining,
		    "ioreg\t\t: ");
		riscv32_linux_text_append_cstr(&cursor, &remaining, line);
		riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	}
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "registry_seed\t: IOPlatformExpertDevice\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "registry_seed\t: IOCPU boot_hart=");
	riscv32_linux_text_append_dec(&cursor, &remaining, bootinfo->hartid);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " harts=");
	riscv32_linux_text_append_dec(&cursor, &remaining, bootinfo->cpu_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " isa=");
	riscv32_linux_text_append_cstr(&cursor, &remaining, bootinfo->cpu_isa);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " mmu=");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    bootinfo->cpu_mmu_type);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	for (i = 0; i < bootinfo->memory_bank_count &&
	    i < RISCV32_FDT_MAX_MEMORY_REGIONS; i++) {
		riscv32_linux_text_append_cstr(&cursor, &remaining,
		    "registry_seed\t: IOMemoryBank");
		riscv32_linux_text_append_dec(&cursor, &remaining, i);
		riscv32_linux_text_append_cstr(&cursor, &remaining, " base=");
		riscv32_linux_text_append_hex32(&cursor, &remaining,
		    bootinfo->memory_banks[i].base);
		riscv32_linux_text_append_cstr(&cursor, &remaining, " size=");
		riscv32_linux_text_append_hex32(&cursor, &remaining,
		    bootinfo->memory_banks[i].size);
		riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	}
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "registry_seed\t: IOUART16550 base=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->uart_base);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " irq=");
	riscv32_linux_text_append_dec(&cursor, &remaining, bootinfo->uart_irq);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "registry_seed\t: IOPLIC base=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->plic_base);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " size=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->plic_size);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "registry_seed\t: IOACLINT base=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->aclint_base);
	riscv32_linux_text_append_cstr(&cursor, &remaining, " size=");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    bootinfo->aclint_size);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "registry_seed\t: IOVirtioMMIO count=");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    bootinfo->virtio_count);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	for (i = 0; i < bootinfo->virtio_count &&
	    i < RISCV32_FDT_MAX_VIRTIO_MMIO; i++) {
		riscv32_linux_text_append_cstr(&cursor, &remaining,
		    "registry_seed\t: IOVirtioMMIO");
		riscv32_linux_text_append_dec(&cursor, &remaining, i);
		riscv32_linux_text_append_cstr(&cursor, &remaining, " base=");
		riscv32_linux_text_append_hex32(&cursor, &remaining,
		    bootinfo->virtio[i].base);
		riscv32_linux_text_append_cstr(&cursor, &remaining, " size=");
		riscv32_linux_text_append_hex32(&cursor, &remaining,
		    bootinfo->virtio[i].size);
		riscv32_linux_text_append_cstr(&cursor, &remaining, " irq=");
		riscv32_linux_text_append_dec(&cursor, &remaining,
		    bootinfo->virtio[i].irq);
		riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	}
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    "next\t\t: retire micro IOService behind OSObject/OSMetaClass + IORegistryEntry/IOService\n");
	return riscv32_linux_proc_iokit;
}

static int
riscv32_linux_mmap_lowest_at_or_after(uint32_t min_vaddr)
{
	uint32_t i;
	uint32_t best_vaddr = UINT32_MAX;
	int best = -1;

	for (i = 0; i < RISCV32_LINUX_MMAP_MAX_PAGES; i++) {
		if (riscv32_linux_mmap_page_info[i].used != 1 ||
		    riscv32_linux_mmap_page_info[i].vaddr < min_vaddr) {
			continue;
		}
		if (riscv32_linux_mmap_page_info[i].vaddr < best_vaddr) {
			best_vaddr = riscv32_linux_mmap_page_info[i].vaddr;
			best = (int)i;
		}
	}
	return best;
}

static boolean_t
riscv32_linux_mmap_pages_merge(const riscv32_linux_mmap_page_t *first,
    const riscv32_linux_mmap_page_t *next, uint32_t expected_offset)
{
	if (first == (const riscv32_linux_mmap_page_t *)0 ||
	    next == (const riscv32_linux_mmap_page_t *)0) {
		return FALSE;
	}
	return next->used == 1 &&
	    next->prot == first->prot &&
	    next->source == first->source &&
	    next->file_offset == expected_offset &&
	    riscv32_linux_streq(next->path, first->path);
}

static void
riscv32_linux_proc_maps_append_perms(char **cursor, uint32_t *remaining,
    uint32_t prot)
{
	riscv32_linux_text_append_char(cursor, remaining,
	    (prot & VM_PROT_READ) != 0 ? 'r' : '-');
	riscv32_linux_text_append_char(cursor, remaining,
	    (prot & VM_PROT_WRITE) != 0 ? 'w' : '-');
	riscv32_linux_text_append_char(cursor, remaining,
	    (prot & VM_PROT_EXECUTE) != 0 ? 'x' : '-');
	riscv32_linux_text_append_char(cursor, remaining, 'p');
}

static void
riscv32_linux_proc_maps_append_line(char **cursor, uint32_t *remaining,
    uint32_t start, uint32_t end, uint32_t prot, uint32_t offset,
    const char *path)
{
	riscv32_linux_text_append_hex8_raw(cursor, remaining, start);
	riscv32_linux_text_append_char(cursor, remaining, '-');
	riscv32_linux_text_append_hex8_raw(cursor, remaining, end);
	riscv32_linux_text_append_char(cursor, remaining, ' ');
	riscv32_linux_proc_maps_append_perms(cursor, remaining, prot);
	riscv32_linux_text_append_char(cursor, remaining, ' ');
	riscv32_linux_text_append_hex8_raw(cursor, remaining, offset);
	riscv32_linux_text_append_cstr(cursor, remaining, " 00:00 0");
	if (path != (const char *)0 && path[0] != '\0') {
		riscv32_linux_text_append_cstr(cursor, remaining, "    ");
		riscv32_linux_text_append_cstr(cursor, remaining, path);
	}
	riscv32_linux_text_append_char(cursor, remaining, '\n');
}

static const char *
riscv32_linux_proc_self_maps_text(void)
{
	char *cursor = riscv32_linux_proc_self_maps;
	uint32_t remaining = sizeof(riscv32_linux_proc_self_maps);
	uint32_t min_vaddr = 0;

	riscv32_linux_zero(riscv32_linux_proc_self_maps,
	    sizeof(riscv32_linux_proc_self_maps));
	for (;;) {
		int slot = riscv32_linux_mmap_lowest_at_or_after(min_vaddr);
		const riscv32_linux_mmap_page_t *first;
		uint32_t start;
		uint32_t end;

		if (slot < 0) {
			break;
		}
		first = &riscv32_linux_mmap_page_info[slot];
		start = first->vaddr;
		end = start + RISCV32_LINUX_MMAP_PAGE_SIZE;
		for (;;) {
			int next_slot = riscv32_linux_mmap_page_index(end);
			uint32_t expected_offset = first->file_offset +
			    (end - start);

			if (next_slot < 0 ||
			    !riscv32_linux_mmap_pages_merge(first,
			    &riscv32_linux_mmap_page_info[next_slot],
			    expected_offset)) {
				break;
			}
			end += RISCV32_LINUX_MMAP_PAGE_SIZE;
		}
		riscv32_linux_proc_maps_append_line(&cursor, &remaining, start,
		    end, first->prot, first->file_offset, first->path);
		min_vaddr = end;
	}
	if (riscv32_linux_brk_mapped_pages != 0) {
		uint32_t heap_end = RISCV32_LINUX_BRK_BASE +
		    riscv32_linux_brk_mapped_pages *
		    RISCV32_LINUX_MMAP_PAGE_SIZE;

		riscv32_linux_proc_maps_append_line(&cursor, &remaining,
		    RISCV32_LINUX_BRK_BASE, heap_end,
		    VM_PROT_READ | VM_PROT_WRITE, 0, "[heap]");
	}
	if (cursor == riscv32_linux_proc_self_maps) {
		riscv32_linux_text_append_cstr(&cursor, &remaining,
		    "# no linux personality mappings recorded yet\n");
	}
	return riscv32_linux_proc_self_maps;
}

static const char *
riscv32_linux_proc_self_status_text(void)
{
	const riscv32_linux_exec_process_info_t *info =
	    riscv32_linux_exec_process_info();
	riscv32_linux_process_t *process = riscv32_linux_process_current();
	uint32_t pid = process != (riscv32_linux_process_t *)0 ?
	    process->pid : 1U;
	uint32_t ppid = process != (riscv32_linux_process_t *)0 ?
	    process->ppid : 0U;
	uint32_t process_threads = process != (riscv32_linux_process_t *)0 ?
	    process->thread_count : 1U;
	const char *process_state = process != (riscv32_linux_process_t *)0 ?
	    riscv32_linux_process_state_name(process->state) : "RUNNING";
	char *cursor = riscv32_linux_proc_self_status;
	uint32_t remaining = sizeof(riscv32_linux_proc_self_status);

	riscv32_linux_zero(riscv32_linux_proc_self_status,
	    sizeof(riscv32_linux_proc_self_status));
	riscv32_linux_text_append_cstr(&cursor, &remaining, "Name:\t");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    riscv32_linux_task_name);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nState:\tR (");
	riscv32_linux_text_append_cstr(&cursor, &remaining, process_state);
	riscv32_linux_text_append_cstr(&cursor, &remaining, ")\nPid:\t");
	riscv32_linux_text_append_dec(&cursor, &remaining, pid);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nPPid:\t");
	riscv32_linux_text_append_dec(&cursor, &remaining, ppid);
	riscv32_linux_text_append_char(&cursor, &remaining, '\n');
	riscv32_linux_text_append_cstr(&cursor, &remaining, "Uid:\t");
	riscv32_linux_text_append_dec(&cursor, &remaining, riscv32_linux_uid);
	riscv32_linux_text_append_char(&cursor, &remaining, '\t');
	riscv32_linux_text_append_dec(&cursor, &remaining, riscv32_linux_euid);
	riscv32_linux_text_append_char(&cursor, &remaining, '\t');
	riscv32_linux_text_append_dec(&cursor, &remaining, riscv32_linux_suid);
	riscv32_linux_text_append_char(&cursor, &remaining, '\t');
	riscv32_linux_text_append_dec(&cursor, &remaining, riscv32_linux_fsuid);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nGid:\t");
	riscv32_linux_text_append_dec(&cursor, &remaining, riscv32_linux_gid);
	riscv32_linux_text_append_char(&cursor, &remaining, '\t');
	riscv32_linux_text_append_dec(&cursor, &remaining, riscv32_linux_egid);
	riscv32_linux_text_append_char(&cursor, &remaining, '\t');
	riscv32_linux_text_append_dec(&cursor, &remaining, riscv32_linux_sgid);
	riscv32_linux_text_append_char(&cursor, &remaining, '\t');
	riscv32_linux_text_append_dec(&cursor, &remaining, riscv32_linux_fsgid);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nExec:\t");
	riscv32_linux_text_append_cstr(&cursor, &remaining, info->path);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nInterp:\t");
	riscv32_linux_text_append_cstr(&cursor, &remaining,
	    info->has_interpreter ? info->interpreter : "<none>");
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nEntry:\t");
	riscv32_linux_text_append_hex32(&cursor, &remaining, info->entry);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nRunEntry:\t");
	riscv32_linux_text_append_hex32(&cursor, &remaining,
	    info->runtime_entry);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nLoadBias:\t");
	riscv32_linux_text_append_hex32(&cursor, &remaining, info->load_bias);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nAtBase:\t");
	riscv32_linux_text_append_hex32(&cursor, &remaining, info->at_base);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nPhdr:\t");
	riscv32_linux_text_append_hex32(&cursor, &remaining, info->phdr_vaddr);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nVmText:\t");
	riscv32_linux_text_append_hex32(&cursor, &remaining, info->low_vaddr);
	riscv32_linux_text_append_char(&cursor, &remaining, '-');
	riscv32_linux_text_append_hex32(&cursor, &remaining, info->high_vaddr);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nStack:\t");
	riscv32_linux_text_append_hex32(&cursor, &remaining, info->stack_base);
	riscv32_linux_text_append_char(&cursor, &remaining, '-');
	riscv32_linux_text_append_hex32(&cursor, &remaining, info->stack_top);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nThreads:\t");
	riscv32_linux_text_append_dec(&cursor, &remaining, process_threads);
	riscv32_linux_text_append_char(&cursor, &remaining, '\n');
	return riscv32_linux_proc_self_status;
}

static uint32_t
riscv32_linux_proc_self_auxv_data(const uint8_t **data)
{
	const riscv32_linux_exec_process_info_t *info =
	    riscv32_linux_exec_process_info();
	uint8_t *out = (uint8_t *)(void *)riscv32_linux_proc_self_auxv;
	uint32_t offset = 0;
	boolean_t saw_null = FALSE;
	uint32_t i;

	riscv32_linux_zero(riscv32_linux_proc_self_auxv,
	    sizeof(riscv32_linux_proc_self_auxv));
	for (i = 0; i < info->auxc && i < RISCV32_LINUX_EXEC_MAX_AUXV; i++) {
		if (offset > sizeof(riscv32_linux_proc_self_auxv) - 8U) {
			break;
		}
		riscv32_linux_put_u32le(out, offset, info->auxv[i].type);
		riscv32_linux_put_u32le(out, offset + 4U,
		    info->auxv[i].value);
		offset += 8U;
		if (info->auxv[i].type == 0U) {
			saw_null = TRUE;
			break;
		}
	}
	if (!saw_null && offset <= sizeof(riscv32_linux_proc_self_auxv) - 8U) {
		riscv32_linux_put_u32le(out, offset, 0);
		riscv32_linux_put_u32le(out, offset + 4U, 0);
		offset += 8U;
	}
	*data = out;
	return offset;
}

static uint32_t
riscv32_linux_proc_append_cstr0(char *out, uint32_t size, uint32_t offset,
    const char *value)
{
	uint32_t i;

	if (value == (const char *)0) {
		value = "";
	}
	for (i = 0; value[i] != '\0' && offset + 1U < size; i++) {
		out[offset++] = value[i];
	}
	if (offset < size) {
		out[offset++] = '\0';
	}
	return offset;
}

static uint32_t
riscv32_linux_proc_self_cmdline_data(const uint8_t **data)
{
	const riscv32_linux_exec_process_info_t *info =
	    riscv32_linux_exec_process_info();
	uint32_t offset = 0;
	uint32_t i;

	riscv32_linux_zero(riscv32_linux_proc_self_cmdline,
	    sizeof(riscv32_linux_proc_self_cmdline));
	for (i = 0; i < info->argc && i < RISCV32_LINUX_EXEC_MAX_ARGC; i++) {
		offset = riscv32_linux_proc_append_cstr0(
		    riscv32_linux_proc_self_cmdline,
		    sizeof(riscv32_linux_proc_self_cmdline), offset,
		    info->argv[i]);
	}
	*data = (const uint8_t *)(const void *)riscv32_linux_proc_self_cmdline;
	return offset;
}

static uint32_t
riscv32_linux_proc_self_environ_data(const uint8_t **data)
{
	const riscv32_linux_exec_process_info_t *info =
	    riscv32_linux_exec_process_info();
	uint32_t offset = 0;
	uint32_t i;

	riscv32_linux_zero(riscv32_linux_proc_self_environ,
	    sizeof(riscv32_linux_proc_self_environ));
	for (i = 0; i < info->envc && i < RISCV32_LINUX_EXEC_MAX_ENVC; i++) {
		offset = riscv32_linux_proc_append_cstr0(
		    riscv32_linux_proc_self_environ,
		    sizeof(riscv32_linux_proc_self_environ), offset,
		    info->envp[i]);
	}
	*data = (const uint8_t *)(const void *)riscv32_linux_proc_self_environ;
	return offset;
}

static boolean_t
riscv32_linux_proc_self_fdinfo_data(uint32_t fd, const uint8_t **data,
    uint32_t *size)
{
	riscv32_linux_fd_t snapshot;
	riscv32_linux_fd_t *desc = &snapshot;
	char *cursor = riscv32_linux_proc_self_fdinfo;
	uint32_t remaining = sizeof(riscv32_linux_proc_self_fdinfo);
	uint32_t pos = 0;

	if (!riscv32_linux_fd_snapshot(fd, &snapshot)) {
		return FALSE;
	}
	if (desc->kind == RISCV32_LINUX_FD_INITRD) {
		pos = desc->offset;
	}
	riscv32_linux_zero(riscv32_linux_proc_self_fdinfo,
	    sizeof(riscv32_linux_proc_self_fdinfo));
	riscv32_linux_text_append_cstr(&cursor, &remaining, "pos:\t");
	riscv32_linux_text_append_dec(&cursor, &remaining, pos);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nflags:\t");
	riscv32_linux_text_append_dec(&cursor, &remaining,
	    desc->status_flags |
	    ((desc->descriptor_flags & RISCV32_LINUX_FD_CLOEXEC) != 0 ?
	    RISCV32_LINUX_O_CLOEXEC : 0));
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\nmnt_id:\t1\nino:\t");
	riscv32_linux_text_append_dec(&cursor, &remaining, 100U + fd);
	riscv32_linux_text_append_cstr(&cursor, &remaining, "\n");
	*data = (const uint8_t *)(const void *)riscv32_linux_proc_self_fdinfo;
	*size = riscv32_linux_strlen(riscv32_linux_proc_self_fdinfo);
	return TRUE;
}

static uint32_t
riscv32_linux_proc_path_kind(const char *path)
{
	if (riscv32_linux_streq(path, "/proc/iokit")) {
		return RISCV32_LINUX_FD_PROC_IOKIT;
	}
	if (riscv32_linux_streq(path, "/proc/sbi") ||
	    riscv32_linux_streq(path, "/proc/caribe/sbi")) {
		return RISCV32_LINUX_FD_PROC_CARIBE_SBI;
	}
	if (riscv32_linux_streq(path, "/proc/caribe/devices")) {
		return RISCV32_LINUX_FD_PROC_CARIBE_DEVICES;
	}
	if (riscv32_linux_streq(path, "/proc/caribe/memmap")) {
		return RISCV32_LINUX_FD_PROC_CARIBE_MEMMAP;
	}
	if (riscv32_linux_streq(path, "/proc/caribe/resources")) {
		return RISCV32_LINUX_FD_PROC_CARIBE_RESOURCES;
	}
	if (riscv32_linux_streq(path, "/proc/self/maps")) {
		return RISCV32_LINUX_FD_PROC_SELF_MAPS;
	}
	if (riscv32_linux_streq(path, "/proc/self/status")) {
		return RISCV32_LINUX_FD_PROC_SELF_STATUS;
	}
	if (riscv32_linux_streq(path, "/proc/self/auxv")) {
		return RISCV32_LINUX_FD_PROC_SELF_AUXV;
	}
	if (riscv32_linux_streq(path, "/proc/self/cmdline")) {
		return RISCV32_LINUX_FD_PROC_SELF_CMDLINE;
	}
	if (riscv32_linux_streq(path, "/proc/self/environ")) {
		return RISCV32_LINUX_FD_PROC_SELF_ENVIRON;
	}
	if (riscv32_linux_streq(path, "/proc/bootlog")) {
		return RISCV32_LINUX_FD_PROC_BOOTLOG;
	}
	if (riscv32_linux_streq(path, "/proc/version")) {
		return RISCV32_LINUX_FD_PROC_VERSION;
	}
	if (riscv32_linux_streq(path, "/proc/cpuinfo")) {
		return RISCV32_LINUX_FD_PROC_CPUINFO;
	}
	if (riscv32_linux_streq(path, "/proc/meminfo")) {
		return RISCV32_LINUX_FD_PROC_MEMINFO;
	}
	if (riscv32_linux_streq(path, "/proc/uptime")) {
		return RISCV32_LINUX_FD_PROC_UPTIME;
	}
	if (riscv32_linux_streq(path, "/proc/stat")) {
		return RISCV32_LINUX_FD_PROC_STAT;
	}
	if (riscv32_linux_streq(path, "/proc/caribeos")) {
		return RISCV32_LINUX_FD_PROC_CARIBEOS;
	}
	if (riscv32_linux_streq(path, "/proc/mounts")) {
		return RISCV32_LINUX_FD_PROC_MOUNTS;
	}
	if (riscv32_linux_streq(path, "/proc/filesystems")) {
		return RISCV32_LINUX_FD_PROC_FILESYSTEMS;
	}
	return RISCV32_LINUX_FD_NONE;
}

static const char *
riscv32_linux_proc_mounts_text(void)
{
	return "initrd / initrd ro,relatime 0 0\n"
	    "proc /proc proc rw,nosuid,nodev,noexec,relatime 0 0\n"
	    "devtmpfs /dev devtmpfs rw,nosuid 0 0\n";
}

static const char *
riscv32_linux_proc_filesystems_text(void)
{
	return "nodev\tproc\n"
	    "nodev\tdevtmpfs\n"
	    "\tinitrd\n";
}

static const char *
riscv32_linux_proc_text_for_kind(uint32_t kind)
{
	switch (kind) {
	case RISCV32_LINUX_FD_PROC_VERSION:
		return riscv32_linux_proc_version_text();
	case RISCV32_LINUX_FD_PROC_CPUINFO:
		return riscv32_linux_proc_cpuinfo_text();
	case RISCV32_LINUX_FD_PROC_MEMINFO:
		return riscv32_linux_proc_meminfo_text();
	case RISCV32_LINUX_FD_PROC_UPTIME:
		return riscv32_linux_proc_uptime_text();
	case RISCV32_LINUX_FD_PROC_STAT:
		return riscv32_linux_proc_stat_text();
	case RISCV32_LINUX_FD_PROC_CARIBEOS:
		return riscv32_linux_proc_caribeos_text();
	case RISCV32_LINUX_FD_PROC_BOOTLOG:
		return riscv32_linux_proc_bootlog_text();
	case RISCV32_LINUX_FD_PROC_IOKIT:
		return riscv32_linux_proc_iokit_text();
	case RISCV32_LINUX_FD_PROC_CARIBE_SBI:
		return riscv32_linux_proc_caribe_sbi_text();
	case RISCV32_LINUX_FD_PROC_CARIBE_DEVICES:
		return riscv32_linux_proc_caribe_devices_text();
	case RISCV32_LINUX_FD_PROC_CARIBE_MEMMAP:
		return riscv32_linux_proc_caribe_memmap_text();
	case RISCV32_LINUX_FD_PROC_CARIBE_RESOURCES:
		return riscv32_linux_proc_caribe_resources_text();
	case RISCV32_LINUX_FD_PROC_SELF_MAPS:
		return riscv32_linux_proc_self_maps_text();
	case RISCV32_LINUX_FD_PROC_SELF_STATUS:
		return riscv32_linux_proc_self_status_text();
	case RISCV32_LINUX_FD_PROC_MOUNTS:
		return riscv32_linux_proc_mounts_text();
	case RISCV32_LINUX_FD_PROC_FILESYSTEMS:
		return riscv32_linux_proc_filesystems_text();
	default:
		break;
	}
	return (const char *)0;
}

static boolean_t
riscv32_linux_proc_data_for_kind(uint32_t kind, const uint8_t **data,
    uint32_t *size)
{
	const char *text;

	switch (kind) {
	case RISCV32_LINUX_FD_PROC_SELF_AUXV:
		*size = riscv32_linux_proc_self_auxv_data(data);
		return TRUE;
	case RISCV32_LINUX_FD_PROC_SELF_CMDLINE:
		*size = riscv32_linux_proc_self_cmdline_data(data);
		return TRUE;
	case RISCV32_LINUX_FD_PROC_SELF_ENVIRON:
		*size = riscv32_linux_proc_self_environ_data(data);
		return TRUE;
	default:
		break;
	}

	text = riscv32_linux_proc_text_for_kind(kind);
	if (text == (const char *)0) {
		return FALSE;
	}
	*data = (const uint8_t *)(const void *)text;
	*size = riscv32_linux_strlen(text);
	return TRUE;
}

static uint32_t
riscv32_linux_proc_ino_for_kind(uint32_t kind)
{
	switch (kind) {
	case RISCV32_LINUX_FD_PROC_VERSION:
		return 8;
	case RISCV32_LINUX_FD_PROC_CPUINFO:
		return 11;
	case RISCV32_LINUX_FD_PROC_MEMINFO:
		return 12;
	case RISCV32_LINUX_FD_PROC_CARIBEOS:
		return 13;
	case RISCV32_LINUX_FD_PROC_BOOTLOG:
		return 21;
	case RISCV32_LINUX_FD_PROC_IOKIT:
		return 22;
	case RISCV32_LINUX_FD_PROC_CARIBE_SBI:
		return 24;
	case RISCV32_LINUX_FD_PROC_CARIBE_DEVICES:
		return 25;
	case RISCV32_LINUX_FD_PROC_CARIBE_MEMMAP:
		return 26;
	case RISCV32_LINUX_FD_PROC_CARIBE_RESOURCES:
		return 39;
	case RISCV32_LINUX_FD_PROC_SELF_MAPS:
		return 27;
	case RISCV32_LINUX_FD_PROC_SELF_STATUS:
		return 28;
	case RISCV32_LINUX_FD_PROC_SELF_AUXV:
		return 29;
	case RISCV32_LINUX_FD_PROC_SELF_CMDLINE:
		return 30;
	case RISCV32_LINUX_FD_PROC_SELF_ENVIRON:
		return 31;
	case RISCV32_LINUX_FD_PROC_UPTIME:
		return 32;
	case RISCV32_LINUX_FD_PROC_STAT:
		return 33;
	case RISCV32_LINUX_FD_PROC_MOUNTS:
		return 19;
	case RISCV32_LINUX_FD_PROC_FILESYSTEMS:
		return 20;
	default:
		break;
	}
	return 0;
}

static uint32_t
riscv32_linux_align8(uint32_t value)
{
	return (value + 7U) & ~7U;
}

static uint32_t
riscv32_linux_page_up(uint32_t value)
{
	return (value + RISCV32_LINUX_MMAP_PAGE_SIZE - 1U) &
	    ~(RISCV32_LINUX_MMAP_PAGE_SIZE - 1U);
}

static vm_prot_t
riscv32_linux_mmap_prot(uint32_t linux_prot)
{
	vm_prot_t prot = VM_PROT_NONE;

	if ((linux_prot & RISCV32_LINUX_PROT_READ) != 0) {
		prot |= VM_PROT_READ;
	}
	if ((linux_prot & RISCV32_LINUX_PROT_WRITE) != 0) {
		prot |= VM_PROT_WRITE;
	}
	if ((linux_prot & RISCV32_LINUX_PROT_EXEC) != 0) {
		prot |= VM_PROT_EXECUTE;
	}
	return prot;
}

static boolean_t
riscv32_linux_enter_static_page(pmap_t pmap, uint32_t vaddr, uint8_t *page,
    vm_prot_t prot)
{
	addr64_t phys;

	if (pmap == PMAP_NULL || page == (uint8_t *)0) {
		return FALSE;
	}
	phys = riscv32_kva_to_phys((vm_offset_t)(uintptr_t)page);
	if (phys == 0) {
		return FALSE;
	}
	pmap_enter(pmap, vaddr, (ppnum_t)(phys >> PAGE_SHIFT), prot, prot, 0,
	    TRUE);
	return TRUE;
}

static boolean_t
riscv32_linux_map_static_page(pmap_t pmap, uint32_t vaddr, uint8_t *page,
    vm_prot_t prot)
{
	riscv32_linux_zero(page, RISCV32_LINUX_MMAP_PAGE_SIZE);
	return riscv32_linux_enter_static_page(pmap, vaddr, page, prot);
}

static uint32_t
riscv32_linux_copyin_path(uint32_t user_path, char *path, uint32_t path_size)
{
	uint32_t i;

	if (path_size == 0) {
		return RISCV32_LINUX_EINVAL;
	}
	for (i = 0; i < path_size; i++) {
		if (riscv32_copyin(user_path + i, &path[i], 1) != 0) {
			path[0] = '\0';
			return RISCV32_LINUX_EFAULT;
		}
		if (path[i] == '\0') {
			return 0;
		}
	}
	path[path_size - 1U] = '\0';
	return RISCV32_LINUX_ENAMETOOLONG;
}

static int
riscv32_linux_copyout_cstr_field(const char *src, uint32_t user_addr)
{
	char field[RISCV32_LINUX_UTSNAME_FIELD_LEN];
	uint32_t i;

	for (i = 0; i < sizeof(field); i++) {
		field[i] = '\0';
	}
	if (src != (const char *)0) {
		for (i = 0; i + 1U < sizeof(field) && src[i] != '\0'; i++) {
			field[i] = src[i];
		}
	}
	return riscv32_copyout(field, user_addr, sizeof(field));
}

static boolean_t
riscv32_linux_write_user_bytes(uint32_t user_buf, uint32_t count,
    uint32_t *copied_out)
{
	char chunk[RISCV32_LINUX_WRITE_CHUNK];
	uint32_t i;
	uint32_t copied;
	uint32_t chunk_len;

	if (copied_out != (uint32_t *)0) {
		*copied_out = 0;
	}
	if (!riscv32_user_range_valid(user_buf, count)) {
		return FALSE;
	}
	for (copied = 0; copied < count; copied += chunk_len) {
		chunk_len = count - copied;
		if (chunk_len > sizeof(chunk)) {
			chunk_len = sizeof(chunk);
		}
		if (riscv32_copyin(user_buf + copied, chunk, chunk_len) != 0) {
			return FALSE;
		}
		for (i = 0; i < chunk_len; i++) {
			riscv32_early_putc(chunk[i]);
		}
		if (copied_out != (uint32_t *)0) {
			*copied_out = copied + chunk_len;
		}
	}
	return TRUE;
}

static boolean_t
riscv32_linux_copyout_phys(uint32_t paddr, uint32_t user_buf, uint32_t count,
    uint32_t *copied_out)
{
	uint32_t copied;
	uint32_t chunk_len;
	vm_offset_t kva;

	if (copied_out != (uint32_t *)0) {
		*copied_out = 0;
	}
	if (!riscv32_user_range_valid(user_buf, count)) {
		return FALSE;
	}
	for (copied = 0; copied < count; copied += chunk_len) {
		if (paddr > UINT32_MAX - copied) {
			return FALSE;
		}
		chunk_len = count - copied;
		if (chunk_len > RISCV32_LINUX_COPY_CHUNK) {
			chunk_len = RISCV32_LINUX_COPY_CHUNK;
		}
		kva = riscv32_phys_to_kva((addr64_t)(paddr + copied));
		if (kva == 0 ||
		    riscv32_copyout((const void *)(uintptr_t)kva,
		    user_buf + copied, chunk_len) != 0) {
			return FALSE;
		}
		if (copied_out != (uint32_t *)0) {
			*copied_out = copied + chunk_len;
		}
	}
	return TRUE;
}

static boolean_t
riscv32_linux_copy_phys_to_kernel(uint32_t paddr, uint8_t *dst,
    uint32_t count)
{
	uint32_t copied;
	uint32_t chunk_len;
	vm_offset_t kva;

	if (dst == (uint8_t *)0) {
		return FALSE;
	}
	for (copied = 0; copied < count; copied += chunk_len) {
		const volatile uint8_t *src;
		uint32_t i;

		if (paddr > UINT32_MAX - copied) {
			return FALSE;
		}
		chunk_len = count - copied;
		if (chunk_len > RISCV32_LINUX_COPY_CHUNK) {
			chunk_len = RISCV32_LINUX_COPY_CHUNK;
		}
		kva = riscv32_phys_to_kva((addr64_t)(paddr + copied));
		if (kva == 0) {
			return FALSE;
		}
		src = (const volatile uint8_t *)(uintptr_t)kva;
		for (i = 0; i < chunk_len; i++) {
			dst[copied + i] = src[i];
		}
	}
	return TRUE;
}

static boolean_t
riscv32_linux_path_is_synthetic_dir(const char *path)
{
	return riscv32_linux_streq(path, "/") ||
	    riscv32_linux_streq(path, ".") ||
	    riscv32_linux_streq(path, "/bin") ||
	    riscv32_linux_streq(path, "/dev") ||
	    riscv32_linux_streq(path, "/dev/fd") ||
	    riscv32_linux_streq(path, "/etc") ||
	    riscv32_linux_streq(path, "/caribe") ||
	    riscv32_linux_streq(path, "/proc") ||
	    riscv32_linux_streq(path, "/proc/self") ||
	    riscv32_linux_streq(path, "/proc/self/fd") ||
	    riscv32_linux_streq(path, "/proc/self/fdinfo") ||
	    riscv32_linux_streq(path, "/proc/caribe");
}

static uint32_t
riscv32_linux_synthetic_dir_kind(const char *path)
{
	if (riscv32_linux_streq(path, "/dev")) {
		return RISCV32_LINUX_DIR_DEV;
	}
	if (riscv32_linux_streq(path, "/dev/fd")) {
		return RISCV32_LINUX_DIR_DEV_FD;
	}
	if (riscv32_linux_streq(path, "/bin")) {
		return RISCV32_LINUX_DIR_BIN;
	}
	if (riscv32_linux_streq(path, "/etc")) {
		return RISCV32_LINUX_DIR_ETC;
	}
	if (riscv32_linux_streq(path, "/caribe")) {
		return RISCV32_LINUX_DIR_CARIBE;
	}
	if (riscv32_linux_streq(path, "/proc")) {
		return RISCV32_LINUX_DIR_PROC;
	}
	if (riscv32_linux_streq(path, "/proc/self")) {
		return RISCV32_LINUX_DIR_PROC_SELF;
	}
	if (riscv32_linux_streq(path, "/proc/self/fd")) {
		return RISCV32_LINUX_DIR_PROC_SELF_FD;
	}
	if (riscv32_linux_streq(path, "/proc/self/fdinfo")) {
		return RISCV32_LINUX_DIR_PROC_SELF_FDINFO;
	}
	if (riscv32_linux_streq(path, "/proc/caribe")) {
		return RISCV32_LINUX_DIR_PROC_CARIBE;
	}
	return RISCV32_LINUX_DIR_ROOT;
}

static uint32_t
riscv32_linux_synthetic_dir_ino(uint32_t dir_kind)
{
	switch (dir_kind) {
	case RISCV32_LINUX_DIR_DEV:
		return 4;
	case RISCV32_LINUX_DIR_PROC:
		return 5;
	case RISCV32_LINUX_DIR_PROC_SELF:
		return 6;
	case RISCV32_LINUX_DIR_PROC_SELF_FD:
		return 36;
	case RISCV32_LINUX_DIR_PROC_SELF_FDINFO:
		return 38;
	case RISCV32_LINUX_DIR_PROC_CARIBE:
		return 23;
	case RISCV32_LINUX_DIR_DEV_FD:
		return 37;
	case RISCV32_LINUX_DIR_ETC:
		return 14;
	case RISCV32_LINUX_DIR_CARIBE:
		return 15;
	case RISCV32_LINUX_DIR_BIN:
		return 21;
	default:
		return 2;
	}
}

static boolean_t
riscv32_linux_mode_is_dir(uint32_t mode)
{
	return (mode & RISCV32_LINUX_MODE_IFMT) == RISCV32_LINUX_MODE_IFDIR;
}

static boolean_t
riscv32_linux_fd_path_number(const char *path, uint32_t *fd_out);
static boolean_t
riscv32_linux_fdinfo_path_number(const char *path, uint32_t *fd_out);

static boolean_t
riscv32_linux_path_exists(const char *path)
{
	riscv32_linux_initrd_file_t file;
	uint32_t fd;

	return riscv32_linux_path_is_synthetic_dir(path) ||
	    riscv32_linux_initrd_dir_exists(path) ||
	    riscv32_linux_streq(path, "/dev/console") ||
	    riscv32_linux_streq(path, "/dev/tty") ||
	    riscv32_linux_streq(path, "/dev/null") ||
	    (riscv32_linux_fd_path_number(path, &fd) &&
	    riscv32_linux_fd_ref(fd) != (riscv32_linux_fd_t *)0) ||
	    (riscv32_linux_fdinfo_path_number(path, &fd) &&
	    riscv32_linux_fd_ref(fd) != (riscv32_linux_fd_t *)0) ||
	    riscv32_linux_proc_path_kind(path) != RISCV32_LINUX_FD_NONE ||
	    riscv32_linux_streq(path, "/proc/self/exe") ||
	    riscv32_linux_initrd_lookup(path, &file);
}

static boolean_t
riscv32_linux_path_is_dir_existing(const char *path)
{
	riscv32_linux_initrd_file_t file;

	if (riscv32_linux_path_is_synthetic_dir(path) ||
	    riscv32_linux_initrd_dir_exists(path)) {
		return TRUE;
	}
	if (riscv32_linux_initrd_lookup(path, &file)) {
		return riscv32_linux_mode_is_dir(file.mode);
	}
	return FALSE;
}

static boolean_t
riscv32_linux_parse_u32_dec(const char *text, uint32_t *value_out)
{
	uint32_t value = 0;
	uint32_t i;

	if (text == (const char *)0 || text[0] == '\0') {
		return FALSE;
	}
	for (i = 0; text[i] != '\0'; i++) {
		uint32_t digit;

		if (text[i] < '0' || text[i] > '9') {
			return FALSE;
		}
		digit = (uint32_t)(text[i] - '0');
		if (value > (UINT32_MAX - digit) / 10U) {
			return FALSE;
		}
		value = value * 10U + digit;
	}
	*value_out = value;
	return TRUE;
}

static boolean_t
riscv32_linux_fd_path_number(const char *path, uint32_t *fd_out)
{
	const char *suffix = (const char *)0;

	if (riscv32_linux_startswith(path, "/proc/self/fd/")) {
		suffix = path + 14;
	} else if (riscv32_linux_startswith(path, "/dev/fd/")) {
		suffix = path + 8;
	}
	return suffix != (const char *)0 &&
	    riscv32_linux_parse_u32_dec(suffix, fd_out);
}

static boolean_t
riscv32_linux_fdinfo_path_number(const char *path, uint32_t *fd_out)
{
	const char *prefix = "/proc/self/fdinfo/";

	if (!riscv32_linux_startswith(path, prefix)) {
		return FALSE;
	}
	return riscv32_linux_parse_u32_dec(
	    path + riscv32_linux_strlen(prefix), fd_out);
}

static void
riscv32_linux_append_dec_cstr(char *dst, uint32_t dst_size, uint32_t value)
{
	char digits[10];
	uint32_t count = 0;
	uint32_t len;

	if (dst_size == 0) {
		return;
	}
	len = riscv32_linux_strlen(dst);
	if (len + 1U >= dst_size) {
		return;
	}
	if (value == 0) {
		dst[len] = '0';
		dst[len + 1U] = '\0';
		return;
	}
	while (value != 0 && count < sizeof(digits)) {
		digits[count++] = (char)('0' + (value % 10U));
		value /= 10U;
	}
	while (count != 0 && len + 1U < dst_size) {
		dst[len++] = digits[--count];
		dst[len] = '\0';
	}
}

static const char *
riscv32_linux_proc_path_for_kind(uint32_t kind)
{
	switch (kind) {
	case RISCV32_LINUX_FD_PROC_VERSION:
		return "/proc/version";
	case RISCV32_LINUX_FD_PROC_CPUINFO:
		return "/proc/cpuinfo";
	case RISCV32_LINUX_FD_PROC_MEMINFO:
		return "/proc/meminfo";
	case RISCV32_LINUX_FD_PROC_CARIBEOS:
		return "/proc/caribeos";
	case RISCV32_LINUX_FD_PROC_MOUNTS:
		return "/proc/mounts";
	case RISCV32_LINUX_FD_PROC_FILESYSTEMS:
		return "/proc/filesystems";
	case RISCV32_LINUX_FD_PROC_BOOTLOG:
		return "/proc/bootlog";
	case RISCV32_LINUX_FD_PROC_IOKIT:
		return "/proc/iokit";
	case RISCV32_LINUX_FD_PROC_CARIBE_SBI:
		return "/proc/caribe/sbi";
	case RISCV32_LINUX_FD_PROC_CARIBE_DEVICES:
		return "/proc/caribe/devices";
	case RISCV32_LINUX_FD_PROC_CARIBE_MEMMAP:
		return "/proc/caribe/memmap";
	case RISCV32_LINUX_FD_PROC_CARIBE_RESOURCES:
		return "/proc/caribe/resources";
	case RISCV32_LINUX_FD_PROC_SELF_MAPS:
		return "/proc/self/maps";
	case RISCV32_LINUX_FD_PROC_SELF_STATUS:
		return "/proc/self/status";
	case RISCV32_LINUX_FD_PROC_SELF_AUXV:
		return "/proc/self/auxv";
	case RISCV32_LINUX_FD_PROC_SELF_CMDLINE:
		return "/proc/self/cmdline";
	case RISCV32_LINUX_FD_PROC_SELF_ENVIRON:
		return "/proc/self/environ";
	case RISCV32_LINUX_FD_PROC_UPTIME:
		return "/proc/uptime";
	case RISCV32_LINUX_FD_PROC_STAT:
		return "/proc/stat";
	default:
		break;
	}
	return (const char *)0;
}

static boolean_t
riscv32_linux_fd_link_target(uint32_t fd, char *target,
    uint32_t target_size)
{
	riscv32_linux_fd_t *desc = riscv32_linux_fd_ref(fd);
	const char *path;

	if (desc == (riscv32_linux_fd_t *)0 || target_size == 0) {
		return FALSE;
	}
	switch (desc->kind) {
	case RISCV32_LINUX_FD_CONSOLE:
		riscv32_linux_copy_cstr(target, target_size, "/dev/tty");
		return TRUE;
	case RISCV32_LINUX_FD_NULL:
		riscv32_linux_copy_cstr(target, target_size, "/dev/null");
		return TRUE;
	case RISCV32_LINUX_FD_DIR:
	case RISCV32_LINUX_FD_INITRD:
		riscv32_linux_copy_cstr(target, target_size, desc->dir_path);
		return TRUE;
	case RISCV32_LINUX_FD_PIPE:
		riscv32_linux_copy_cstr(target, target_size, "pipe:[");
		riscv32_linux_append_dec_cstr(target, target_size,
		    (uint32_t)desc->value);
		{
			uint32_t len = riscv32_linux_strlen(target);

			if (len + 1U < target_size) {
				target[len] = ']';
				target[len + 1U] = '\0';
			}
		}
		return TRUE;
	case RISCV32_LINUX_FD_EVENTFD:
		riscv32_linux_copy_cstr(target, target_size,
		    "anon_inode:[eventfd]");
		return TRUE;
	case RISCV32_LINUX_FD_EPOLL:
		riscv32_linux_copy_cstr(target, target_size,
		    "anon_inode:[eventpoll]");
		return TRUE;
	case RISCV32_LINUX_FD_TIMERFD:
		riscv32_linux_copy_cstr(target, target_size,
		    "anon_inode:[timerfd]");
		return TRUE;
	case RISCV32_LINUX_FD_INOTIFY:
		riscv32_linux_copy_cstr(target, target_size,
		    "anon_inode:[inotify]");
		return TRUE;
	case RISCV32_LINUX_FD_PROC_SELF_FDINFO:
		riscv32_linux_copy_cstr(target, target_size,
		    "/proc/self/fdinfo/");
		riscv32_linux_append_dec_cstr(target, target_size,
		    (uint32_t)desc->value);
		return TRUE;
	default:
		break;
	}
	path = riscv32_linux_proc_path_for_kind(desc->kind);
	if (path != (const char *)0) {
		riscv32_linux_copy_cstr(target, target_size, path);
		return TRUE;
	}
	return FALSE;
}

static void
riscv32_linux_resolve_path(const char *path, char *out, uint32_t out_size)
{
	if (out_size == 0) {
		return;
	}
	if (path == (const char *)0 || path[0] == '\0') {
		out[0] = '\0';
		return;
	}
	if (path[0] == '/') {
		riscv32_linux_copy_cstr(out, out_size, path);
		return;
	}
	if (riscv32_linux_streq(path, ".")) {
		riscv32_linux_copy_cstr(out, out_size, riscv32_linux_cwd);
		return;
	}
	if (riscv32_linux_streq(riscv32_linux_cwd, "/")) {
		uint32_t i = 0;

		out[i++] = '/';
		while (i + 1U < out_size && *path != '\0') {
			out[i++] = *path++;
		}
		out[i] = '\0';
		return;
	}
	riscv32_linux_copy_cstr(out, out_size, riscv32_linux_cwd);
	{
		uint32_t len = riscv32_linux_strlen(out);

		if (len + 1U < out_size) {
			out[len++] = '/';
			out[len] = '\0';
		}
		while (len + 1U < out_size && *path != '\0') {
			out[len++] = *path++;
		}
		out[len] = '\0';
	}
}

static void
riscv32_linux_resolve_path_at(uint32_t dirfd, const char *path, char *out,
    uint32_t out_size)
{
	riscv32_linux_fd_t *desc;
	uint32_t base_len;

	if (out_size == 0) {
		return;
	}
	if (path == (const char *)0 || path[0] == '\0' ||
	    path[0] == '/' || dirfd == RISCV32_LINUX_AT_FDCWD) {
		riscv32_linux_resolve_path(path, out, out_size);
		return;
	}
	desc = riscv32_linux_fd_ref(dirfd);
	if (desc == (riscv32_linux_fd_t *)0 ||
	    desc->kind != RISCV32_LINUX_FD_DIR) {
		riscv32_linux_resolve_path(path, out, out_size);
		return;
	}
	riscv32_linux_copy_cstr(out, out_size, desc->dir_path);
	base_len = riscv32_linux_strlen(out);
	if (base_len == 0 || out[base_len - 1U] != '/') {
		if (base_len + 1U >= out_size) {
			return;
		}
		out[base_len++] = '/';
		out[base_len] = '\0';
	}
	riscv32_linux_copy_cstr(out + base_len, out_size - base_len, path);
}

static boolean_t
riscv32_linux_copyin_path_arg(riscv_saved_state_t *state,
    uint32_t user_path, char *path)
{
	uint32_t error;

	error = riscv32_linux_copyin_path(user_path, path,
	    RISCV32_LINUX_PATH_MAX);
	if (error != 0) {
		riscv32_linux_return_errno(state, error);
		return FALSE;
	}
	return TRUE;
}

static boolean_t
riscv32_linux_return_readonly_for_path(riscv_saved_state_t *state,
    uint32_t user_path, boolean_t creating)
{
	char path[RISCV32_LINUX_PATH_MAX];

	if (!riscv32_linux_copyin_path_arg(state, user_path, path)) {
		return TRUE;
	}
	if (creating && riscv32_linux_path_exists(path)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EEXIST);
		return TRUE;
	}
	if (!creating && !riscv32_linux_path_exists(path)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOENT);
		return TRUE;
	}
	riscv32_linux_return_errno(state, RISCV32_LINUX_EROFS);
	return TRUE;
}

static boolean_t
riscv32_linux_copyout_link_target(uint32_t user_buf, uint32_t buf_size,
    const char *target, uint32_t *copied_out)
{
	uint32_t len;

	if (buf_size == 0) {
		return FALSE;
	}
	len = riscv32_linux_strlen(target);
	if (len > buf_size) {
		len = buf_size;
	}
	if (riscv32_copyout(target, user_buf, len) != 0) {
		return FALSE;
	}
	if (copied_out != (uint32_t *)0) {
		*copied_out = len;
	}
	return TRUE;
}

static boolean_t
riscv32_linux_sys_openat(riscv_saved_state_t *state)
{
	uint32_t dirfd = state->thread.x[RISCV_X_A0];
	uint32_t user_path = state->thread.x[RISCV_X_A1];
	uint32_t flags = state->thread.x[RISCV_X_A2];
	uint32_t accmode = flags & RISCV32_LINUX_O_ACCMODE;
	char raw_path[RISCV32_LINUX_PATH_MAX];
	char path[RISCV32_LINUX_PATH_MAX];
	riscv32_linux_initrd_file_t file;
	riscv32_linux_fd_t *desc;
	riscv32_linux_fd_t source;
	uint32_t proc_kind;
	uint32_t source_fd;
	uint32_t fdinfo_fd;
	uint32_t error;
	uint32_t fd;

	error = riscv32_linux_copyin_path(user_path, raw_path,
	    sizeof(raw_path));
	if (error != 0) {
		riscv32_linux_return_errno(state, error);
		return TRUE;
	}
	riscv32_linux_resolve_path_at(dirfd, raw_path, path, sizeof(path));

	if (riscv32_linux_streq(path, "/dev/console") ||
	    riscv32_linux_streq(path, "/dev/tty")) {
		if (!riscv32_linux_fd_alloc(RISCV32_LINUX_FD_CONSOLE,
		    accmode, (const riscv32_linux_initrd_file_t *)0, &fd)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EMFILE);
			return TRUE;
		}
		riscv32_linux_fd_set_open_flags(fd, flags);
		riscv32_linux_return_value(state, fd);
		return TRUE;
	}
	if (riscv32_linux_streq(path, "/dev/null")) {
		if (!riscv32_linux_fd_alloc(RISCV32_LINUX_FD_NULL,
		    accmode, (const riscv32_linux_initrd_file_t *)0, &fd)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EMFILE);
			return TRUE;
		}
		riscv32_linux_fd_set_open_flags(fd, flags);
		riscv32_linux_return_value(state, fd);
		return TRUE;
	}
	if (riscv32_linux_fd_path_number(path, &source_fd)) {
		if (!riscv32_linux_fd_snapshot(source_fd, &source)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_ENOENT);
			return TRUE;
		}
		if (!riscv32_linux_fd_alloc_cloned_description(&source,
		    RISCV32_LINUX_FD_FIRST, &fd)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EMFILE);
			return TRUE;
		}
		riscv32_linux_fd_set_open_flags(fd, flags);
		riscv32_linux_return_value(state, fd);
		return TRUE;
	}
	if (riscv32_linux_fdinfo_path_number(path, &fdinfo_fd)) {
		if (accmode != RISCV32_LINUX_O_RDONLY) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EACCES);
			return TRUE;
		}
		if (riscv32_linux_fd_ref(fdinfo_fd) ==
		    (riscv32_linux_fd_t *)0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_ENOENT);
			return TRUE;
		}
		if (!riscv32_linux_fd_alloc(
		    RISCV32_LINUX_FD_PROC_SELF_FDINFO, accmode,
		    (const riscv32_linux_initrd_file_t *)0, &fd)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EMFILE);
			return TRUE;
		}
		desc = riscv32_linux_fd_get(fd);
		if (desc != (riscv32_linux_fd_t *)0) {
			desc->value = fdinfo_fd;
			riscv32_linux_copy_cstr(desc->dir_path,
			    sizeof(desc->dir_path), path);
		}
		riscv32_linux_fd_set_open_flags(fd, flags);
		riscv32_linux_return_value(state, fd);
		return TRUE;
	}
	proc_kind = riscv32_linux_proc_path_kind(path);
	if (proc_kind != RISCV32_LINUX_FD_NONE) {
		if (accmode != RISCV32_LINUX_O_RDONLY) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EACCES);
			return TRUE;
		}
		if (!riscv32_linux_fd_alloc(proc_kind, accmode,
		    (const riscv32_linux_initrd_file_t *)0, &fd)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EMFILE);
			return TRUE;
		}
		riscv32_linux_fd_set_open_flags(fd, flags);
		riscv32_linux_return_value(state, fd);
		return TRUE;
	}
	if (riscv32_linux_path_is_synthetic_dir(path)) {
		uint32_t dir_kind = riscv32_linux_synthetic_dir_kind(path);

		if (accmode != RISCV32_LINUX_O_RDONLY) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EACCES);
			return TRUE;
		}
		if (!riscv32_linux_fd_alloc_dir(dir_kind, accmode, &fd)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EMFILE);
			return TRUE;
		}
		riscv32_linux_fd_set_open_flags(fd, flags);
		riscv32_linux_return_value(state, fd);
		return TRUE;
	}
	if (riscv32_linux_initrd_dir_exists(path)) {
		if (accmode != RISCV32_LINUX_O_RDONLY) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EACCES);
			return TRUE;
		}
		if (!riscv32_linux_fd_alloc_initrd_dir(path, accmode, &fd)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EMFILE);
			return TRUE;
		}
		riscv32_linux_fd_set_open_flags(fd, flags);
		riscv32_linux_return_value(state, fd);
		return TRUE;
	}
	if (accmode != RISCV32_LINUX_O_RDONLY) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EACCES);
		return TRUE;
	}
	if (!riscv32_linux_initrd_lookup(path, &file)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOENT);
		return TRUE;
	}
	if (riscv32_linux_mode_is_dir(file.mode)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EISDIR);
		return TRUE;
	}
	if (!riscv32_linux_fd_alloc(RISCV32_LINUX_FD_INITRD, accmode, &file,
	    &fd)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EMFILE);
		return TRUE;
	}
	desc = riscv32_linux_fd_get(fd);
	if (desc != (riscv32_linux_fd_t *)0) {
		riscv32_linux_copy_cstr(desc->dir_path,
		    sizeof(desc->dir_path), path);
	}
	riscv32_linux_fd_set_open_flags(fd, flags);

	riscv32_linux_return_value(state, fd);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_faccessat(riscv_saved_state_t *state)
{
	uint32_t dirfd = state->thread.x[RISCV_X_A0];
	uint32_t user_path = state->thread.x[RISCV_X_A1];
	char raw_path[RISCV32_LINUX_PATH_MAX];
	char path[RISCV32_LINUX_PATH_MAX];
	uint32_t error;

	error = riscv32_linux_copyin_path(user_path, raw_path,
	    sizeof(raw_path));
	if (error != 0) {
		riscv32_linux_return_errno(state, error);
		return TRUE;
	}
	riscv32_linux_resolve_path_at(dirfd, raw_path, path, sizeof(path));
	if (!riscv32_linux_path_exists(path)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOENT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_readlinkat(riscv_saved_state_t *state)
{
	uint32_t dirfd = state->thread.x[RISCV_X_A0];
	uint32_t user_path = state->thread.x[RISCV_X_A1];
	uint32_t user_buf = state->thread.x[RISCV_X_A2];
	uint32_t buf_size = state->thread.x[RISCV_X_A3];
	char raw_path[RISCV32_LINUX_PATH_MAX];
	char path[RISCV32_LINUX_PATH_MAX];
	char target[RISCV32_LINUX_PATH_MAX];
	uint32_t copied;
	uint32_t error;
	uint32_t fd;

	error = riscv32_linux_copyin_path(user_path, raw_path,
	    sizeof(raw_path));
	if (error != 0) {
		riscv32_linux_return_errno(state, error);
		return TRUE;
	}
	riscv32_linux_resolve_path_at(dirfd, raw_path, path, sizeof(path));
	if (buf_size == 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (riscv32_linux_streq(path, "/proc/self/exe")) {
		if (!riscv32_linux_copyout_link_target(user_buf, buf_size,
		    "/kernel.elf", &copied)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, copied);
		return TRUE;
	}
	if (riscv32_linux_fd_path_number(path, &fd) &&
	    riscv32_linux_fd_link_target(fd, target, sizeof(target))) {
		if (!riscv32_linux_copyout_link_target(user_buf, buf_size,
		    target, &copied)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, copied);
		return TRUE;
	}

	riscv32_linux_return_errno(state, RISCV32_LINUX_ENOENT);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_readonly_create_path(riscv_saved_state_t *state,
    uint32_t user_path)
{
	return riscv32_linux_return_readonly_for_path(state, user_path, TRUE);
}

static boolean_t
riscv32_linux_sys_readonly_existing_path(riscv_saved_state_t *state,
    uint32_t user_path)
{
	return riscv32_linux_return_readonly_for_path(state, user_path, FALSE);
}

static boolean_t
riscv32_linux_sys_symlinkat(riscv_saved_state_t *state)
{
	uint32_t user_old = state->thread.x[RISCV_X_A0];
	uint32_t user_new = state->thread.x[RISCV_X_A2];
	char old_path[RISCV32_LINUX_PATH_MAX];

	if (!riscv32_linux_copyin_path_arg(state, user_old, old_path)) {
		return TRUE;
	}
	return riscv32_linux_sys_readonly_create_path(state, user_new);
}

static boolean_t
riscv32_linux_sys_linkat(riscv_saved_state_t *state)
{
	uint32_t user_old = state->thread.x[RISCV_X_A1];
	uint32_t user_new = state->thread.x[RISCV_X_A3];
	char old_path[RISCV32_LINUX_PATH_MAX];
	char new_path[RISCV32_LINUX_PATH_MAX];

	if (!riscv32_linux_copyin_path_arg(state, user_old, old_path) ||
	    !riscv32_linux_copyin_path_arg(state, user_new, new_path)) {
		return TRUE;
	}
	if (!riscv32_linux_path_exists(old_path)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOENT);
		return TRUE;
	}
	if (riscv32_linux_path_exists(new_path)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EEXIST);
		return TRUE;
	}
	riscv32_linux_return_errno(state, RISCV32_LINUX_EROFS);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_renameat(riscv_saved_state_t *state)
{
	uint32_t user_old = state->thread.x[RISCV_X_A1];
	uint32_t user_new = state->thread.x[RISCV_X_A3];
	char old_path[RISCV32_LINUX_PATH_MAX];
	char new_path[RISCV32_LINUX_PATH_MAX];

	if (!riscv32_linux_copyin_path_arg(state, user_old, old_path) ||
	    !riscv32_linux_copyin_path_arg(state, user_new, new_path)) {
		return TRUE;
	}
	if (!riscv32_linux_path_exists(old_path)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOENT);
		return TRUE;
	}
	riscv32_linux_return_errno(state, RISCV32_LINUX_EROFS);
	return TRUE;
}

static boolean_t
riscv32_linux_validate_timespec_pair(uint32_t user_times, boolean_t time64)
{
	uint8_t times[32];
	uint32_t size = time64 ? 32U : 16U;

	if (user_times == 0) {
		return TRUE;
	}
	return riscv32_copyin(user_times, times, size) == 0;
}

static boolean_t
riscv32_linux_sys_utimensat(riscv_saved_state_t *state, boolean_t time64)
{
	uint32_t dirfd = state->thread.x[RISCV_X_A0];
	uint32_t user_path = state->thread.x[RISCV_X_A1];
	uint32_t user_times = state->thread.x[RISCV_X_A2];
	char path[RISCV32_LINUX_PATH_MAX];
	riscv32_linux_fd_t source;

	if (!riscv32_linux_validate_timespec_pair(user_times, time64)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	if (user_path == 0) {
		if (!riscv32_linux_fd_snapshot(dirfd, &source)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
			return TRUE;
		}
		riscv32_linux_return_errno(state, RISCV32_LINUX_EROFS);
		return TRUE;
	}
	if (!riscv32_linux_copyin_path_arg(state, user_path, path)) {
		return TRUE;
	}
	if (!riscv32_linux_path_exists(path)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOENT);
		return TRUE;
	}
	riscv32_linux_return_errno(state, RISCV32_LINUX_EROFS);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_getcwd(riscv_saved_state_t *state)
{
	uint32_t user_buf = state->thread.x[RISCV_X_A0];
	uint32_t size = state->thread.x[RISCV_X_A1];
	uint32_t len = riscv32_linux_strlen(riscv32_linux_cwd) + 1U;

	if (size < len) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ERANGE);
		return TRUE;
	}
	if (riscv32_copyout(riscv32_linux_cwd, user_buf, len) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, len);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_chdir(riscv_saved_state_t *state)
{
	uint32_t user_path = state->thread.x[RISCV_X_A0];
	char raw[RISCV32_LINUX_PATH_MAX];
	char path[RISCV32_LINUX_PATH_MAX];
	uint32_t error;

	error = riscv32_linux_copyin_path(user_path, raw, sizeof(raw));
	if (error != 0) {
		riscv32_linux_return_errno(state, error);
		return TRUE;
	}
	riscv32_linux_resolve_path(raw, path, sizeof(path));
	if (!riscv32_linux_path_exists(path)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOENT);
		return TRUE;
	}
	if (!riscv32_linux_path_is_dir_existing(path)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOTDIR);
		return TRUE;
	}
	riscv32_linux_copy_cstr(riscv32_linux_cwd, sizeof(riscv32_linux_cwd),
	    path);
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_fchdir(riscv_saved_state_t *state)
{
	uint32_t fd = state->thread.x[RISCV_X_A0];
	riscv32_linux_fd_t *desc = riscv32_linux_fd_ref(fd);

	if (desc == (riscv32_linux_fd_t *)0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	if (desc->kind != RISCV32_LINUX_FD_DIR) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOTDIR);
		return TRUE;
	}
	riscv32_linux_copy_cstr(riscv32_linux_cwd, sizeof(riscv32_linux_cwd),
	    desc->dir_path);
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_read(riscv_saved_state_t *state)
{
	uint32_t fd = state->thread.x[RISCV_X_A0];
	uint32_t user_buf = state->thread.x[RISCV_X_A1];
	uint32_t count = state->thread.x[RISCV_X_A2];
	riscv32_linux_fd_t *desc;
	const uint8_t *proc_data;
	uint32_t proc_size;
	uint32_t limit;
	uint32_t copied;
	uint32_t error;
	uint32_t paddr;

	desc = riscv32_linux_fd_ref(fd);
	if (desc == (riscv32_linux_fd_t *)0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	if (desc->kind == RISCV32_LINUX_FD_CONSOLE) {
		limit = count;
		if (limit > RISCV32_LINUX_READ_MAX) {
			limit = RISCV32_LINUX_READ_MAX;
		}
		error = riscv32_tty_read_user(riscv32_linux_process_current(),
		    user_buf, limit,
		    (desc->status_flags & RISCV32_LINUX_O_NONBLOCK) != 0U,
		    &copied);
		if (error != 0U) {
			riscv32_linux_return_errno(state, error);
			return TRUE;
		}
		riscv32_linux_return_value(state, copied);
		return TRUE;
	}
	if (desc->kind == RISCV32_LINUX_FD_PIPE) {
		if (!riscv32_linux_pipe_read_to_user(desc, user_buf, count,
		    &copied, &error)) {
			riscv32_linux_return_errno(state, error);
			return TRUE;
		}
		riscv32_linux_return_value(state, copied);
		return TRUE;
	}
	if (desc->kind == RISCV32_LINUX_FD_NULL) {
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (desc->kind == RISCV32_LINUX_FD_TIMERFD) {
		uint8_t expirations[8];

		if (count < sizeof(expirations)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
			return TRUE;
		}
		riscv32_linux_return_errno(state, RISCV32_LINUX_EAGAIN);
		return TRUE;
	}
	if (desc->kind == RISCV32_LINUX_FD_INOTIFY) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EAGAIN);
		return TRUE;
	}
	if (desc->kind == RISCV32_LINUX_FD_EVENTFD) {
		uint8_t value[8];

		if (count < sizeof(value)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
			return TRUE;
		}
		if (desc->value == 0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EAGAIN);
			return TRUE;
		}
		riscv32_linux_zero(value, sizeof(value));
		riscv32_linux_put_u64le(value, 0, desc->value);
		if (riscv32_copyout(value, user_buf, sizeof(value)) != 0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		desc->value = 0;
		riscv32_linux_return_value(state, sizeof(value));
		return TRUE;
	}
	if (desc->kind == RISCV32_LINUX_FD_DIR) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EISDIR);
		return TRUE;
	}
	if (desc->kind == RISCV32_LINUX_FD_PROC_SELF_FDINFO) {
		if (!riscv32_linux_proc_self_fdinfo_data(
		    (uint32_t)desc->value, &proc_data, &proc_size)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_ENOENT);
			return TRUE;
		}
		if (desc->offset >= proc_size) {
			riscv32_linux_return_value(state, 0);
			return TRUE;
		}
		limit = proc_size - desc->offset;
		if (limit > count) {
			limit = count;
		}
		if (limit > RISCV32_LINUX_READ_MAX) {
			limit = RISCV32_LINUX_READ_MAX;
		}
		if (riscv32_copyout(proc_data + desc->offset, user_buf,
		    limit) != 0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		desc->offset += limit;
		riscv32_linux_return_value(state, limit);
		return TRUE;
	}
	if (riscv32_linux_proc_data_for_kind(desc->kind, &proc_data,
	    &proc_size)) {
		if (desc->offset >= proc_size) {
			riscv32_linux_return_value(state, 0);
			return TRUE;
		}
		limit = proc_size - desc->offset;
		if (limit > count) {
			limit = count;
		}
		if (limit > RISCV32_LINUX_READ_MAX) {
			limit = RISCV32_LINUX_READ_MAX;
		}
		if (riscv32_copyout(proc_data + desc->offset, user_buf,
		    limit) != 0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		desc->offset += limit;
		riscv32_linux_return_value(state, limit);
		return TRUE;
	}
	if (desc->kind != RISCV32_LINUX_FD_INITRD) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	if (desc->offset >= desc->file.size) {
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}

	limit = desc->file.size - desc->offset;
	if (limit > count) {
		limit = count;
	}
	if (limit > RISCV32_LINUX_READ_MAX) {
		limit = RISCV32_LINUX_READ_MAX;
	}
	if (desc->file.data_paddr > UINT32_MAX - desc->offset) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	paddr = desc->file.data_paddr + desc->offset;
	if (!riscv32_linux_copyout_phys(paddr, user_buf, limit, &copied)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	desc->offset += copied;
	riscv32_linux_return_value(state, copied);
	return TRUE;
}

static uint32_t
riscv32_linux_read_desc_to_user(riscv32_linux_fd_t *desc, uint32_t user_buf,
    uint32_t count, uint32_t *copied_out)
{
	const uint8_t *proc_data;
	uint32_t proc_size;
	uint32_t limit;
	uint32_t copied;
	uint32_t paddr;
	uint32_t error;
	uint8_t value[8];

	*copied_out = 0;
	if (desc == (riscv32_linux_fd_t *)0) {
		return RISCV32_LINUX_EBADF;
	}
	if (count == 0) {
		return 0;
	}
	if (desc->kind == RISCV32_LINUX_FD_CONSOLE) {
		return riscv32_tty_read_user(riscv32_linux_process_current(),
		    user_buf, count,
		    (desc->status_flags & RISCV32_LINUX_O_NONBLOCK) != 0U,
		    copied_out);
	}
	if (desc->kind == RISCV32_LINUX_FD_NULL) {
		return 0;
	}
	if (desc->kind == RISCV32_LINUX_FD_PIPE) {
		if (!riscv32_linux_pipe_read_to_user(desc, user_buf, count,
		    copied_out, &error)) {
			return error;
		}
		return 0;
	}
	if (desc->kind == RISCV32_LINUX_FD_TIMERFD ||
	    desc->kind == RISCV32_LINUX_FD_INOTIFY) {
		return RISCV32_LINUX_EAGAIN;
	}
	if (desc->kind == RISCV32_LINUX_FD_EVENTFD) {
		if (count < sizeof(value)) {
			return RISCV32_LINUX_EINVAL;
		}
		if (desc->value == 0) {
			return RISCV32_LINUX_EAGAIN;
		}
		riscv32_linux_zero(value, sizeof(value));
		riscv32_linux_put_u64le(value, 0, desc->value);
		if (riscv32_copyout(value, user_buf, sizeof(value)) != 0) {
			return RISCV32_LINUX_EFAULT;
		}
		desc->value = 0;
		*copied_out = sizeof(value);
		return 0;
	}
	if (desc->kind == RISCV32_LINUX_FD_DIR) {
		return RISCV32_LINUX_EISDIR;
	}
	if (desc->kind == RISCV32_LINUX_FD_PROC_SELF_FDINFO) {
		if (!riscv32_linux_proc_self_fdinfo_data(
		    (uint32_t)desc->value, &proc_data, &proc_size)) {
			return RISCV32_LINUX_ENOENT;
		}
		if (desc->offset >= proc_size) {
			return 0;
		}
		limit = proc_size - desc->offset;
		if (limit > count) {
			limit = count;
		}
		if (limit > RISCV32_LINUX_READ_MAX) {
			limit = RISCV32_LINUX_READ_MAX;
		}
		if (riscv32_copyout(proc_data + desc->offset, user_buf,
		    limit) != 0) {
			return RISCV32_LINUX_EFAULT;
		}
		desc->offset += limit;
		*copied_out = limit;
		return 0;
	}
	if (riscv32_linux_proc_data_for_kind(desc->kind, &proc_data,
	    &proc_size)) {
		if (desc->offset >= proc_size) {
			return 0;
		}
		limit = proc_size - desc->offset;
		if (limit > count) {
			limit = count;
		}
		if (limit > RISCV32_LINUX_READ_MAX) {
			limit = RISCV32_LINUX_READ_MAX;
		}
		if (riscv32_copyout(proc_data + desc->offset, user_buf,
		    limit) != 0) {
			return RISCV32_LINUX_EFAULT;
		}
		desc->offset += limit;
		*copied_out = limit;
		return 0;
	}
	if (desc->kind != RISCV32_LINUX_FD_INITRD) {
		return RISCV32_LINUX_EBADF;
	}
	if (desc->offset >= desc->file.size) {
		return 0;
	}
	limit = desc->file.size - desc->offset;
	if (limit > count) {
		limit = count;
	}
	if (limit > RISCV32_LINUX_READ_MAX) {
		limit = RISCV32_LINUX_READ_MAX;
	}
	if (desc->file.data_paddr > UINT32_MAX - desc->offset) {
		return RISCV32_LINUX_EFAULT;
	}
	paddr = desc->file.data_paddr + desc->offset;
	if (!riscv32_linux_copyout_phys(paddr, user_buf, limit, &copied)) {
		return RISCV32_LINUX_EFAULT;
	}
	desc->offset += copied;
	*copied_out = copied;
	return 0;
}

static boolean_t
riscv32_linux_sys_readv(riscv_saved_state_t *state)
{
	uint32_t fd = state->thread.x[RISCV_X_A0];
	uint32_t user_iov = state->thread.x[RISCV_X_A1];
	uint32_t iovcnt = state->thread.x[RISCV_X_A2];
	riscv32_linux_fd_t *desc;
	uint32_t iov[2];
	uint32_t i;
	uint32_t len;
	uint32_t total = 0;
	uint32_t copied;
	uint32_t error;

	if (iovcnt > RISCV32_LINUX_IOV_MAX) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (!riscv32_user_range_valid(user_iov,
	    iovcnt * RISCV32_LINUX_IOVEC_SIZE)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	desc = riscv32_linux_fd_ref(fd);
	if (desc == (riscv32_linux_fd_t *)0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	for (i = 0; i < iovcnt && total < RISCV32_LINUX_READ_MAX; i++) {
		if (riscv32_copyin(user_iov + i * RISCV32_LINUX_IOVEC_SIZE,
		    iov, sizeof(iov)) != 0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		len = iov[1];
		if (len > RISCV32_LINUX_READ_MAX - total) {
			len = RISCV32_LINUX_READ_MAX - total;
		}
		error = riscv32_linux_read_desc_to_user(desc, iov[0], len,
		    &copied);
		if (error != 0) {
			if (total != 0) {
				break;
			}
			riscv32_linux_return_errno(state, error);
			return TRUE;
		}
		total += copied;
		if (copied < len) {
			break;
		}
	}
	riscv32_linux_return_value(state, total);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_pread64(riscv_saved_state_t *state)
{
	uint32_t fd = state->thread.x[RISCV_X_A0];
	uint32_t user_buf = state->thread.x[RISCV_X_A1];
	uint32_t count = state->thread.x[RISCV_X_A2];
	uint32_t offset_low = state->thread.x[RISCV_X_A3];
	uint32_t offset_high = state->thread.x[RISCV_X_A4];
	riscv32_linux_fd_t *desc;
	riscv32_linux_fd_t copy;
	const uint8_t *proc_data;
	uint32_t proc_size;
	uint32_t copied;
	uint32_t error;

	desc = riscv32_linux_fd_ref(fd);
	if (desc == (riscv32_linux_fd_t *)0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	if (offset_high != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (desc->kind == RISCV32_LINUX_FD_DIR) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EISDIR);
		return TRUE;
	}
	if (desc->kind != RISCV32_LINUX_FD_INITRD &&
	    desc->kind != RISCV32_LINUX_FD_PROC_SELF_FDINFO &&
	    !riscv32_linux_proc_data_for_kind(desc->kind, &proc_data,
	    &proc_size)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ESPIPE);
		return TRUE;
	}
	copy = *desc;
	copy.offset = offset_low;
	error = riscv32_linux_read_desc_to_user(&copy, user_buf, count,
	    &copied);
	if (error != 0) {
		riscv32_linux_return_errno(state, error);
		return TRUE;
	}
	riscv32_linux_return_value(state, copied);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_close(riscv_saved_state_t *state)
{
	uint32_t fd = state->thread.x[RISCV_X_A0];

	if (!riscv32_linux_fd_close_number(fd)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_pipe2(riscv_saved_state_t *state)
{
	uint32_t user_pipefd = state->thread.x[RISCV_X_A0];
	uint32_t flags = state->thread.x[RISCV_X_A1];
	uint32_t read_fd;
	uint32_t write_fd;
	uint32_t pipe_id;
	riscv32_linux_fd_t *read_desc;
	riscv32_linux_fd_t *write_desc;
	uint8_t out[8];

	if ((flags & ~(RISCV32_LINUX_O_CLOEXEC |
	    RISCV32_LINUX_O_NONBLOCK)) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	pipe_id = riscv32_linux_pipe_alloc();
	if (pipe_id == 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EMFILE);
		return TRUE;
	}
	if (!riscv32_linux_fd_alloc(RISCV32_LINUX_FD_PIPE,
	    RISCV32_LINUX_O_RDONLY, (const riscv32_linux_initrd_file_t *)0,
	    &read_fd)) {
		riscv32_linux_pipe_abandon(pipe_id);
		riscv32_linux_return_errno(state, RISCV32_LINUX_EMFILE);
		return TRUE;
	}
	if (!riscv32_linux_fd_alloc(RISCV32_LINUX_FD_PIPE,
	    RISCV32_LINUX_O_WRONLY, (const riscv32_linux_initrd_file_t *)0,
	    &write_fd)) {
		(void)riscv32_linux_fd_close_number(read_fd);
		riscv32_linux_pipe_abandon(pipe_id);
		riscv32_linux_return_errno(state, RISCV32_LINUX_EMFILE);
		return TRUE;
	}
	read_desc = riscv32_linux_fd_get(read_fd);
	write_desc = riscv32_linux_fd_get(write_fd);
	if (read_desc == (riscv32_linux_fd_t *)0 ||
	    write_desc == (riscv32_linux_fd_t *)0) {
		(void)riscv32_linux_fd_close_number(read_fd);
		(void)riscv32_linux_fd_close_number(write_fd);
		riscv32_linux_pipe_abandon(pipe_id);
		riscv32_linux_return_errno(state, RISCV32_LINUX_EMFILE);
		return TRUE;
	}
	read_desc->value = pipe_id;
	write_desc->value = pipe_id;
	riscv32_linux_pipe_ref_fd(read_desc);
	riscv32_linux_pipe_ref_fd(write_desc);
	(void)riscv32_linux_fd_set_open_flags(read_fd, flags);
	(void)riscv32_linux_fd_set_open_flags(write_fd, flags);
	riscv32_linux_zero(out, sizeof(out));
	riscv32_linux_put_u32le(out, 0, read_fd);
	riscv32_linux_put_u32le(out, 4, write_fd);
	if (riscv32_copyout(out, user_pipefd, sizeof(out)) != 0) {
		(void)riscv32_linux_fd_close_number(read_fd);
		(void)riscv32_linux_fd_close_number(write_fd);
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	if (riscv32_boot_arg_has("process-gate-12")) {
		riscv32_early_puts("[process-gate-12] pipe id=");
		riscv32_early_putdec(pipe_id);
		riscv32_early_puts(" read-fd=");
		riscv32_early_putdec(read_fd);
		riscv32_early_puts(" write-fd=");
		riscv32_early_putdec(write_fd);
		riscv32_early_puts(" capacity=4096 created PASS\n");
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_eventfd2(riscv_saved_state_t *state)
{
	uint32_t initval = state->thread.x[RISCV_X_A0];
	uint32_t flags = state->thread.x[RISCV_X_A1];
	uint32_t fd;
	riscv32_linux_fd_t *desc;

	if ((flags & ~(RISCV32_LINUX_O_CLOEXEC |
	    RISCV32_LINUX_O_NONBLOCK)) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (!riscv32_linux_fd_alloc(RISCV32_LINUX_FD_EVENTFD,
	    RISCV32_LINUX_O_RDWR, (const riscv32_linux_initrd_file_t *)0,
	    &fd)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EMFILE);
		return TRUE;
	}
	desc = riscv32_linux_fd_get(fd);
	if (desc != (riscv32_linux_fd_t *)0) {
		desc->value = initval;
	}
	riscv32_linux_return_value(state, fd);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_epoll_create1(riscv_saved_state_t *state)
{
	uint32_t flags = state->thread.x[RISCV_X_A0];
	uint32_t fd;

	if ((flags & ~RISCV32_LINUX_O_CLOEXEC) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (!riscv32_linux_fd_alloc(RISCV32_LINUX_FD_EPOLL,
	    RISCV32_LINUX_O_RDWR, (const riscv32_linux_initrd_file_t *)0,
	    &fd)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EMFILE);
		return TRUE;
	}
	riscv32_linux_return_value(state, fd);
	return TRUE;
}

static boolean_t
riscv32_linux_fd_kind_is(uint32_t fd, uint32_t kind)
{
	riscv32_linux_fd_t *desc = riscv32_linux_fd_get(fd);

	return desc != (riscv32_linux_fd_t *)0 && desc->kind == kind;
}

static boolean_t
riscv32_linux_sys_epoll_ctl(riscv_saved_state_t *state)
{
	uint32_t epfd = state->thread.x[RISCV_X_A0];
	uint32_t op = state->thread.x[RISCV_X_A1];
	uint32_t fd = state->thread.x[RISCV_X_A2];
	uint32_t user_event = state->thread.x[RISCV_X_A3];
	riscv32_linux_fd_t source;
	uint8_t event[RISCV32_LINUX_EPOLL_EVENT_SIZE];

	if (!riscv32_linux_fd_kind_is(epfd, RISCV32_LINUX_FD_EPOLL)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	if (!riscv32_linux_fd_snapshot(fd, &source)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	if (op != RISCV32_LINUX_EPOLL_CTL_ADD &&
	    op != RISCV32_LINUX_EPOLL_CTL_DEL &&
	    op != RISCV32_LINUX_EPOLL_CTL_MOD) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (op != RISCV32_LINUX_EPOLL_CTL_DEL &&
	    riscv32_copyin(user_event, event, sizeof(event)) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_epoll_pwait(riscv_saved_state_t *state,
    boolean_t timeout_is_timespec)
{
	uint32_t epfd = state->thread.x[RISCV_X_A0];
	uint32_t user_events = state->thread.x[RISCV_X_A1];
	uint32_t maxevents = state->thread.x[RISCV_X_A2];
	uint32_t user_timeout = state->thread.x[RISCV_X_A3];
	uint32_t event_bytes;

	if (!riscv32_linux_fd_kind_is(epfd, RISCV32_LINUX_FD_EPOLL)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	if (maxevents == 0 || maxevents > RISCV32_LINUX_EPOLL_MAX_EVENTS) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	event_bytes = maxevents * RISCV32_LINUX_EPOLL_EVENT_SIZE;
	if (!riscv32_user_range_valid(user_events, event_bytes)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	if (timeout_is_timespec && user_timeout != 0 &&
	    !riscv32_linux_validate_timespec(user_timeout, TRUE)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_timerfd_create(riscv_saved_state_t *state)
{
	uint32_t flags = state->thread.x[RISCV_X_A1];
	uint32_t fd;

	if ((flags & ~(RISCV32_LINUX_O_CLOEXEC |
	    RISCV32_LINUX_O_NONBLOCK)) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (!riscv32_linux_fd_alloc(RISCV32_LINUX_FD_TIMERFD,
	    RISCV32_LINUX_O_RDONLY, (const riscv32_linux_initrd_file_t *)0,
	    &fd)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EMFILE);
		return TRUE;
	}
	riscv32_linux_return_value(state, fd);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_timerfd_gettime(riscv_saved_state_t *state,
    boolean_t time64)
{
	uint32_t fd = state->thread.x[RISCV_X_A0];
	uint32_t user_curr = state->thread.x[RISCV_X_A1];
	uint32_t size = time64 ? 32U : 16U;

	if (!riscv32_linux_fd_kind_is(fd, RISCV32_LINUX_FD_TIMERFD)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	if (!riscv32_linux_copyout_zero(user_curr, size)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_timerfd_settime(riscv_saved_state_t *state,
    boolean_t time64)
{
	uint32_t fd = state->thread.x[RISCV_X_A0];
	uint32_t user_new = state->thread.x[RISCV_X_A2];
	uint32_t user_old = state->thread.x[RISCV_X_A3];
	uint32_t size = time64 ? 32U : 16U;

	if (!riscv32_linux_fd_kind_is(fd, RISCV32_LINUX_FD_TIMERFD)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	if (!riscv32_linux_validate_timespec_pair(user_new, time64)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	if (user_old != 0 && !riscv32_linux_copyout_zero(user_old, size)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_inotify_init1(riscv_saved_state_t *state)
{
	uint32_t flags = state->thread.x[RISCV_X_A0];
	uint32_t fd;

	if ((flags & ~(RISCV32_LINUX_O_CLOEXEC |
	    RISCV32_LINUX_O_NONBLOCK)) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (!riscv32_linux_fd_alloc(RISCV32_LINUX_FD_INOTIFY,
	    RISCV32_LINUX_O_RDONLY, (const riscv32_linux_initrd_file_t *)0,
	    &fd)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EMFILE);
		return TRUE;
	}
	riscv32_linux_return_value(state, fd);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_inotify_add_watch(riscv_saved_state_t *state)
{
	uint32_t fd = state->thread.x[RISCV_X_A0];
	uint32_t user_path = state->thread.x[RISCV_X_A1];
	char path[RISCV32_LINUX_PATH_MAX];

	if (!riscv32_linux_fd_kind_is(fd, RISCV32_LINUX_FD_INOTIFY)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	if (!riscv32_linux_copyin_path_arg(state, user_path, path)) {
		return TRUE;
	}
	if (!riscv32_linux_path_exists(path)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOENT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 1);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_inotify_rm_watch(riscv_saved_state_t *state)
{
	uint32_t fd = state->thread.x[RISCV_X_A0];
	uint32_t wd = state->thread.x[RISCV_X_A1];

	if (!riscv32_linux_fd_kind_is(fd, RISCV32_LINUX_FD_INOTIFY)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	if (wd != 1) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_dup(riscv_saved_state_t *state)
{
	uint32_t oldfd = state->thread.x[RISCV_X_A0];
	riscv32_linux_fd_t source;
	uint32_t newfd;

	if (!riscv32_linux_fd_snapshot(oldfd, &source)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	source.descriptor_flags = 0U;
	if (!riscv32_linux_fd_alloc_copy(&source, RISCV32_LINUX_FD_FIRST,
	    &newfd)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EMFILE);
		return TRUE;
	}
	riscv32_linux_return_value(state, newfd);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_dup3(riscv_saved_state_t *state)
{
	uint32_t oldfd = state->thread.x[RISCV_X_A0];
	uint32_t newfd = state->thread.x[RISCV_X_A1];
	uint32_t flags = state->thread.x[RISCV_X_A2];
	riscv32_linux_fd_t source;

	if (oldfd == newfd ||
	    (flags & ~RISCV32_LINUX_O_CLOEXEC) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (!riscv32_linux_fd_snapshot(oldfd, &source)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	source.descriptor_flags = (flags & RISCV32_LINUX_O_CLOEXEC) != 0 ?
	    RISCV32_LINUX_FD_CLOEXEC : 0U;
	if (!riscv32_linux_fd_install_copy(newfd, &source)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	riscv32_linux_return_value(state, newfd);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_fcntl(riscv_saved_state_t *state)
{
	uint32_t fd = state->thread.x[RISCV_X_A0];
	uint32_t cmd = state->thread.x[RISCV_X_A1];
	uint32_t arg = state->thread.x[RISCV_X_A2];
	riscv32_linux_fd_t *desc;
	riscv32_linux_fd_t source;
	riscv32_linux_process_fd_t *entry;
	uint32_t newfd;

	desc = riscv32_linux_fd_ref(fd);
	if (desc == (riscv32_linux_fd_t *)0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	if (!riscv32_linux_fd_snapshot(fd, &source)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	entry = riscv32_linux_fd_entry(fd);
	switch (cmd) {
	case RISCV32_LINUX_F_DUPFD:
	case RISCV32_LINUX_F_DUPFD_CLOEXEC:
		source.descriptor_flags =
		    cmd == RISCV32_LINUX_F_DUPFD_CLOEXEC ?
		    RISCV32_LINUX_FD_CLOEXEC : 0;
		if (!riscv32_linux_fd_alloc_copy(&source, arg, &newfd)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EMFILE);
			return TRUE;
		}
		riscv32_linux_return_value(state, newfd);
		return TRUE;
	case RISCV32_LINUX_F_GETFD:
		riscv32_linux_return_value(state, source.descriptor_flags);
		return TRUE;
	case RISCV32_LINUX_F_SETFD:
		entry->descriptor_flags = arg & RISCV32_LINUX_FD_CLOEXEC;
		riscv32_linux_return_value(state, 0);
		return TRUE;
	case RISCV32_LINUX_F_GETFL:
		riscv32_linux_return_value(state, source.status_flags);
		return TRUE;
	case RISCV32_LINUX_F_SETFL:
		riscv32_linux_ofd_lock(desc);
		desc->status_flags = source.accmode |
		    (arg & RISCV32_LINUX_O_NONBLOCK);
		riscv32_linux_ofd_unlock(desc);
		riscv32_linux_return_value(state, 0);
		return TRUE;
	default:
		break;
	}
	riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_fd_noop(riscv_saved_state_t *state)
{
	uint32_t fd = state->thread.x[RISCV_X_A0];
	riscv32_linux_fd_t source;

	if (!riscv32_linux_fd_snapshot(fd, &source)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_socket_unsupported(riscv_saved_state_t *state)
{
	riscv32_linux_return_errno(state, RISCV32_LINUX_EAFNOSUPPORT);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_socket_fd_notsock(riscv_saved_state_t *state)
{
	uint32_t fd = state->thread.x[RISCV_X_A0];
	riscv32_linux_fd_t source;

	if (!riscv32_linux_fd_snapshot(fd, &source)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	riscv32_linux_return_errno(state, RISCV32_LINUX_ENOTSOCK);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_llseek(riscv_saved_state_t *state)
{
	uint32_t fd = state->thread.x[RISCV_X_A0];
	uint32_t offset_high = state->thread.x[RISCV_X_A1];
	uint32_t offset_low = state->thread.x[RISCV_X_A2];
	uint32_t user_result = state->thread.x[RISCV_X_A3];
	uint32_t whence = state->thread.x[RISCV_X_A4];
	riscv32_linux_fd_t *desc;
	uint8_t result[8];
	uint32_t base;
	uint32_t new_offset;

	desc = riscv32_linux_fd_get(fd);
	if (desc == (riscv32_linux_fd_t *)0) {
		if (riscv32_linux_fd_is_console(fd)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_ESPIPE);
		} else {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		}
		return TRUE;
	}
	if (desc->kind != RISCV32_LINUX_FD_INITRD) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ESPIPE);
		return TRUE;
	}
	if (offset_high != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}

	switch (whence) {
	case RISCV32_LINUX_SEEK_SET:
		base = 0;
		break;
	case RISCV32_LINUX_SEEK_CUR:
		base = desc->offset;
		break;
	case RISCV32_LINUX_SEEK_END:
		base = desc->file.size;
		break;
	default:
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (offset_low > UINT32_MAX - base) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	new_offset = base + offset_low;
	riscv32_linux_zero(result, sizeof(result));
	riscv32_linux_put_u32le(result, 0, new_offset);
	if (riscv32_copyout(result, user_result, sizeof(result)) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	desc->offset = new_offset;
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_dir_entry(uint32_t dir_kind, uint32_t index,
    const char **name, uint32_t *ino, uint32_t *type)
{
	if (dir_kind == RISCV32_LINUX_DIR_DEV_FD ||
	    dir_kind == RISCV32_LINUX_DIR_PROC_SELF_FD ||
	    dir_kind == RISCV32_LINUX_DIR_PROC_SELF_FDINFO) {
		static char fd_name[12];
		uint32_t parent_ino;
		uint32_t self_ino;
		uint32_t fd;
		uint32_t ordinal = 0;
		uint32_t max_fd = RISCV32_LINUX_FD_FIRST +
		    RISCV32_LINUX_FD_MAX;

		self_ino = riscv32_linux_synthetic_dir_ino(dir_kind);
		parent_ino = dir_kind == RISCV32_LINUX_DIR_DEV_FD ? 4U : 6U;
		if (index == 0) {
			*name = ".";
			*ino = self_ino;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		}
		if (index == 1) {
			*name = "..";
			*ino = parent_ino;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		}
		for (fd = 0; fd < max_fd; fd++) {
			if (riscv32_linux_fd_ref(fd) ==
			    (riscv32_linux_fd_t *)0) {
				continue;
			}
			if (ordinal == index - 2U) {
				fd_name[0] = '\0';
				riscv32_linux_append_dec_cstr(fd_name,
				    sizeof(fd_name), fd);
				*name = fd_name;
				*ino = dir_kind ==
				    RISCV32_LINUX_DIR_PROC_SELF_FDINFO ?
				    200U + fd : 100U + fd;
				*type = dir_kind ==
				    RISCV32_LINUX_DIR_PROC_SELF_FDINFO ?
				    RISCV32_LINUX_DT_REG :
				    RISCV32_LINUX_DT_LNK;
				return TRUE;
			}
			ordinal++;
		}
		return FALSE;
	}
	if (dir_kind == RISCV32_LINUX_DIR_ROOT) {
		switch (index) {
		case 0:
			*name = ".";
			*ino = 2;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 1:
			*name = "..";
			*ino = 2;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 2:
			*name = "bin";
			*ino = 21;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 3:
			*name = "dev";
			*ino = 4;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 4:
			*name = "proc";
			*ino = 5;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 5:
			*name = "etc";
			*ino = 14;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 6:
			*name = "caribe";
			*ino = 15;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		default:
			return FALSE;
		}
	}
	if (dir_kind == RISCV32_LINUX_DIR_BIN) {
		switch (index) {
		case 0:
			*name = ".";
			*ino = 21;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 1:
			*name = "..";
			*ino = 2;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 2:
			*name = "caribectl";
			*ino = 22;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		default:
			return FALSE;
		}
	}
	if (dir_kind == RISCV32_LINUX_DIR_ETC) {
		switch (index) {
		case 0:
			*name = ".";
			*ino = 14;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 1:
			*name = "..";
			*ino = 2;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 2:
			*name = "os-release";
			*ino = 16;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		case 3:
			*name = "caribe-release";
			*ino = 17;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		default:
			return FALSE;
		}
	}
	if (dir_kind == RISCV32_LINUX_DIR_CARIBE) {
		switch (index) {
		case 0:
			*name = ".";
			*ino = 15;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 1:
			*name = "..";
			*ino = 2;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 2:
			*name = "stage1.manifest";
			*ino = 18;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		default:
			return FALSE;
		}
	}
	if (dir_kind == RISCV32_LINUX_DIR_DEV) {
		switch (index) {
		case 0:
			*name = ".";
			*ino = 4;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 1:
			*name = "..";
			*ino = 2;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 2:
			*name = "console";
			*ino = 3;
			*type = RISCV32_LINUX_DT_CHR;
			return TRUE;
		case 3:
			*name = "tty";
			*ino = 34;
			*type = RISCV32_LINUX_DT_CHR;
			return TRUE;
		case 4:
			*name = "null";
			*ino = 35;
			*type = RISCV32_LINUX_DT_CHR;
			return TRUE;
		case 5:
			*name = "fd";
			*ino = 37;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		default:
			return FALSE;
		}
	}
	if (dir_kind == RISCV32_LINUX_DIR_PROC) {
		switch (index) {
		case 0:
			*name = ".";
			*ino = 5;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 1:
			*name = "..";
			*ino = 2;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 2:
			*name = "self";
			*ino = 6;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 3:
			*name = "version";
			*ino = 8;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		case 4:
			*name = "cpuinfo";
			*ino = 11;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		case 5:
			*name = "meminfo";
			*ino = 12;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		case 6:
			*name = "caribeos";
			*ino = 13;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		case 7:
			*name = "mounts";
			*ino = 19;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		case 8:
			*name = "filesystems";
			*ino = 20;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		case 9:
			*name = "bootlog";
			*ino = 21;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		case 10:
			*name = "iokit";
			*ino = 22;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		case 11:
			*name = "caribe";
			*ino = 23;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 12:
			*name = "uptime";
			*ino = 32;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		case 13:
			*name = "stat";
			*ino = 33;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		default:
			return FALSE;
		}
	}
	if (dir_kind == RISCV32_LINUX_DIR_PROC_CARIBE) {
		switch (index) {
		case 0:
			*name = ".";
			*ino = 23;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 1:
			*name = "..";
			*ino = 5;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 2:
			*name = "sbi";
			*ino = 24;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		case 3:
			*name = "devices";
			*ino = 25;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		case 4:
			*name = "memmap";
			*ino = 26;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		case 5:
			*name = "resources";
			*ino = 39;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		default:
			return FALSE;
		}
	}
	if (dir_kind == RISCV32_LINUX_DIR_PROC_SELF) {
		switch (index) {
		case 0:
			*name = ".";
			*ino = 6;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 1:
			*name = "..";
			*ino = 5;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 2:
			*name = "exe";
			*ino = 7;
			*type = RISCV32_LINUX_DT_LNK;
			return TRUE;
		case 3:
			*name = "maps";
			*ino = 27;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		case 4:
			*name = "status";
			*ino = 28;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		case 5:
			*name = "auxv";
			*ino = 29;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		case 6:
			*name = "cmdline";
			*ino = 30;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		case 7:
			*name = "environ";
			*ino = 31;
			*type = RISCV32_LINUX_DT_REG;
			return TRUE;
		case 8:
			*name = "fd";
			*ino = 36;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		case 9:
			*name = "fdinfo";
			*ino = 38;
			*type = RISCV32_LINUX_DT_DIR;
			return TRUE;
		default:
			return FALSE;
		}
	}
	return FALSE;
}

static uint32_t
riscv32_linux_dir_static_count(uint32_t dir_kind)
{
	const char *name;
	uint32_t ino;
	uint32_t type;
	uint32_t count = 0;

	while (riscv32_linux_dir_entry(dir_kind, count, &name, &ino, &type)) {
		count++;
	}
	return count;
}

static boolean_t
riscv32_linux_dir_static_has_name(uint32_t dir_kind, const char *candidate)
{
	const char *name;
	uint32_t ino;
	uint32_t type;
	uint32_t index;

	for (index = 2; riscv32_linux_dir_entry(dir_kind, index, &name, &ino,
	    &type); index++) {
		if (riscv32_linux_streq(name, candidate)) {
			return TRUE;
		}
	}
	return FALSE;
}

static uint32_t
riscv32_linux_dirent_type_for_mode(uint32_t mode)
{
	switch (mode & RISCV32_LINUX_MODE_IFMT) {
	case RISCV32_LINUX_MODE_IFDIR:
		return RISCV32_LINUX_DT_DIR;
	case RISCV32_LINUX_MODE_IFLNK:
		return RISCV32_LINUX_DT_LNK;
	default:
		return RISCV32_LINUX_DT_REG;
	}
}

static boolean_t
riscv32_linux_initrd_dir_entry(const riscv32_linux_fd_t *desc,
    uint32_t dynamic_index, const char **name, uint32_t *ino, uint32_t *type,
    char *name_buf, uint32_t name_buf_size)
{
	riscv32_linux_initrd_dirent_t dirent;
	uint32_t scan_index;
	uint32_t accepted = 0;
	uint32_t dir_kind;

	if (desc == (const riscv32_linux_fd_t *)0 ||
	    name_buf == (char *)0 || name_buf_size == 0) {
		return FALSE;
	}
	dir_kind = desc->dir_kind;
	for (scan_index = 0; riscv32_linux_initrd_dir_child(desc->dir_path,
	    scan_index, &dirent); scan_index++) {
		if (dir_kind != RISCV32_LINUX_DIR_INITRD &&
		    riscv32_linux_dir_static_has_name(dir_kind, dirent.name)) {
			continue;
		}
		if (accepted == dynamic_index) {
			riscv32_linux_copy_cstr(name_buf, name_buf_size,
			    dirent.name);
			*name = name_buf;
			*ino = dirent.ino;
			*type = riscv32_linux_dirent_type_for_mode(dirent.mode);
			return TRUE;
		}
		accepted++;
	}
	return FALSE;
}

static uint32_t
riscv32_linux_dirent64_reclen(const char *name)
{
	return riscv32_linux_align8(RISCV32_LINUX_DIRENT64_BASE_SIZE +
	    riscv32_linux_strlen(name) + 1U);
}

static boolean_t
riscv32_linux_copyout_dirent64(uint32_t user_dirent, uint32_t ino,
    uint32_t off, uint32_t type, const char *name, uint32_t reclen)
{
	uint8_t record[RISCV32_LINUX_DIRENT64_MAX_SIZE];
	uint32_t i;

	if (reclen > sizeof(record)) {
		return FALSE;
	}
	riscv32_linux_zero(record, sizeof(record));
	riscv32_linux_put_u64le(record, 0, ino);
	riscv32_linux_put_u64le(record, 8, off);
	riscv32_linux_put_u16le(record, 16, reclen);
	record[18] = (uint8_t)type;
	for (i = 0; name[i] != '\0'; i++) {
		record[19U + i] = name[i];
	}
	return riscv32_copyout(record, user_dirent, reclen) == 0;
}

static boolean_t
riscv32_linux_sys_getdents64(riscv_saved_state_t *state)
{
	uint32_t fd = state->thread.x[RISCV_X_A0];
	uint32_t user_dirent = state->thread.x[RISCV_X_A1];
	uint32_t count = state->thread.x[RISCV_X_A2];
	riscv32_linux_fd_t *desc;
	const char *name;
	char name_buf[RISCV32_LINUX_INITRD_NAME_MAX];
	uint32_t ino;
	uint32_t type;
	uint32_t reclen;
	uint32_t total = 0;

	desc = riscv32_linux_fd_get(fd);
	if (desc == (riscv32_linux_fd_t *)0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	if (desc->kind != RISCV32_LINUX_FD_DIR) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOTDIR);
		return TRUE;
	}
	for (;;) {
		uint32_t static_count = desc->dir_kind ==
		    RISCV32_LINUX_DIR_INITRD ? 2U :
		    riscv32_linux_dir_static_count(desc->dir_kind);
		boolean_t have_entry;

		have_entry = riscv32_linux_dir_entry(desc->dir_kind,
		    desc->offset, &name, &ino, &type);
		if (!have_entry) {
			if (desc->dir_kind == RISCV32_LINUX_DIR_INITRD) {
				if (desc->offset == 0) {
					name = ".";
					ino = (uint32_t)desc->value;
					type = RISCV32_LINUX_DT_DIR;
					have_entry = TRUE;
				} else if (desc->offset == 1) {
					name = "..";
					ino = 2;
					type = RISCV32_LINUX_DT_DIR;
					have_entry = TRUE;
				}
			}
		}
		if (!have_entry && desc->offset >= static_count) {
			have_entry = riscv32_linux_initrd_dir_entry(desc,
			    desc->offset - static_count, &name, &ino, &type,
			    name_buf, sizeof(name_buf));
		}
		if (!have_entry) {
			break;
		}
		reclen = riscv32_linux_dirent64_reclen(name);
		if (reclen > count - total) {
			if (total == 0) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EINVAL);
				return TRUE;
			}
			break;
		}
		if (!riscv32_linux_copyout_dirent64(user_dirent + total, ino,
		    desc->offset + 1U, type, name, reclen)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		total += reclen;
		desc->offset++;
	}
	riscv32_linux_return_value(state, total);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_write(riscv_saved_state_t *state)
{
	uint32_t fd = state->thread.x[RISCV_X_A0];
	uint32_t user_buf = state->thread.x[RISCV_X_A1];
	uint32_t count = state->thread.x[RISCV_X_A2];
	riscv32_linux_fd_t *desc;
	uint32_t limit;
	uint32_t copied;
	uint32_t error;

	if (!riscv32_linux_fd_is_output(fd)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	limit = count;
	if (limit > RISCV32_LINUX_WRITE_MAX) {
		limit = RISCV32_LINUX_WRITE_MAX;
	}
	desc = riscv32_linux_fd_ref(fd);
	if (desc != (riscv32_linux_fd_t *)0 &&
	    desc->kind == RISCV32_LINUX_FD_EVENTFD) {
		uint8_t value_buf[8];
		uint64_t value;

		if (count < sizeof(value_buf)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
			return TRUE;
		}
		if (riscv32_copyin(user_buf, value_buf, sizeof(value_buf)) != 0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		value = riscv32_linux_get_u64le(value_buf, 0);
		if (value == UINT64_MAX) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
			return TRUE;
		}
		desc->value += value;
		riscv32_linux_return_value(state, sizeof(value_buf));
		return TRUE;
	}
	if (desc != (riscv32_linux_fd_t *)0 &&
	    desc->kind == RISCV32_LINUX_FD_PIPE) {
		if (!riscv32_linux_pipe_write_from_user(desc, user_buf, limit,
		    &copied, &error)) {
			riscv32_linux_return_errno(state, error);
			return TRUE;
		}
		riscv32_linux_return_value(state, copied);
		return TRUE;
	}
	if (desc != (riscv32_linux_fd_t *)0 &&
	    desc->kind == RISCV32_LINUX_FD_NULL) {
		riscv32_linux_return_value(state, limit);
		return TRUE;
	}
	if (!riscv32_linux_write_user_bytes(user_buf, limit, (uint32_t *)0)) {
		riscv32_early_puts("[linux-syscall] write copyin EFAULT va=0x");
		riscv32_early_puthex32(user_buf);
		riscv32_early_puts(" pmap=0x");
		riscv32_early_puthex32((uint32_t)(uintptr_t)riscv32_pmap_current());
		riscv32_early_puts(" pa=0x");
		riscv32_early_puthex32((uint32_t)pmap_extract(
		    riscv32_pmap_current(), user_buf));
		riscv32_early_puts("\n");
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, limit);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_writev(riscv_saved_state_t *state)
{
	uint32_t fd = state->thread.x[RISCV_X_A0];
	uint32_t user_iov = state->thread.x[RISCV_X_A1];
	uint32_t iovcnt = state->thread.x[RISCV_X_A2];
	uint32_t i;
	uint32_t iov[2];
	uint32_t len;
	uint32_t total;
	uint32_t copied;
	uint32_t error;
	riscv32_linux_fd_t *desc;

	if (!riscv32_linux_fd_is_output(fd)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	if (iovcnt > RISCV32_LINUX_IOV_MAX) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (!riscv32_user_range_valid(user_iov,
	    iovcnt * RISCV32_LINUX_IOVEC_SIZE)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}

	desc = riscv32_linux_fd_ref(fd);
	total = 0;
	if (desc != (riscv32_linux_fd_t *)0 &&
	    desc->kind == RISCV32_LINUX_FD_PIPE) {
		for (i = 0; i < iovcnt && total < RISCV32_LINUX_WRITE_MAX; i++) {
			if (riscv32_copyin(user_iov +
			    i * RISCV32_LINUX_IOVEC_SIZE, iov,
			    sizeof(iov)) != 0) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
			len = iov[1];
			if (len > RISCV32_LINUX_WRITE_MAX - total) {
				len = RISCV32_LINUX_WRITE_MAX - total;
			}
			if (!riscv32_linux_pipe_write_from_user(desc, iov[0],
			    len, &copied, &error)) {
				if (total != 0) {
					break;
				}
				riscv32_linux_return_errno(state, error);
				return TRUE;
			}
			total += copied;
			if (copied < len) {
				break;
			}
		}
		riscv32_linux_return_value(state, total);
		return TRUE;
	}
	for (i = 0; i < iovcnt && total < RISCV32_LINUX_WRITE_MAX; i++) {
		if (riscv32_copyin(user_iov + i * RISCV32_LINUX_IOVEC_SIZE,
		    iov, sizeof(iov)) != 0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		len = iov[1];
		if (len > RISCV32_LINUX_WRITE_MAX - total) {
			len = RISCV32_LINUX_WRITE_MAX - total;
		}
		if (!riscv32_linux_write_user_bytes(iov[0], len, &copied)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		total += copied;
	}
	riscv32_linux_return_value(state, total);
	return TRUE;
}

static boolean_t
riscv32_linux_copyout_termios(uint32_t user_termios)
{
	uint8_t termios[RISCV32_LINUX_TERMIOS_SIZE];

	riscv32_tty_get_termios(termios);
	return riscv32_copyout(termios, user_termios, sizeof(termios)) == 0;
}

static boolean_t
riscv32_linux_copyout_termio(uint32_t user_termio)
{
	uint8_t termio[RISCV32_LINUX_TERMIO_SIZE];
	static const uint8_t cc[8] = { 3, 28, 127, 21, 4, 0, 1, 0 };
	uint32_t i;

	riscv32_linux_zero(termio, sizeof(termio));
	riscv32_linux_put_u16le(termio, 0, 0x0502U);
	riscv32_linux_put_u16le(termio, 2, 0x0005U);
	riscv32_linux_put_u16le(termio, 4, 0x00bfU);
	riscv32_linux_put_u16le(termio, 6, 0x8a3bU);
	for (i = 0; i < sizeof(cc); i++) {
		termio[10U + i] = cc[i];
	}
	return riscv32_copyout(termio, user_termio, sizeof(termio)) == 0;
}

static boolean_t
riscv32_linux_copyout_winsize(uint32_t user_winsize)
{
	uint8_t winsize[RISCV32_LINUX_WINSIZE_SIZE];

	riscv32_linux_zero(winsize, sizeof(winsize));
	riscv32_linux_put_u16le(winsize, 0, 24);
	riscv32_linux_put_u16le(winsize, 2, 80);
	return riscv32_copyout(winsize, user_winsize, sizeof(winsize)) == 0;
}

static boolean_t
riscv32_linux_sys_ioctl(riscv_saved_state_t *state)
{
	uint32_t fd = state->thread.x[RISCV_X_A0];
	uint32_t request = state->thread.x[RISCV_X_A1];
	uint32_t arg = state->thread.x[RISCV_X_A2];
	riscv32_linux_fd_t source;
	riscv32_linux_process_t *process;
	uint32_t pgid;
	uint32_t sid;
	uint8_t termios[RISCV32_LINUX_TERMIOS_SIZE];

	if (!riscv32_linux_fd_snapshot(fd, &source)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	if (source.kind != RISCV32_LINUX_FD_CONSOLE) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOTTY);
		return TRUE;
	}
	switch (request) {
	case RISCV32_LINUX_TCGETS:
		if (!riscv32_linux_copyout_termios(arg)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	case RISCV32_LINUX_TCSETS:
	case RISCV32_LINUX_TCSETSW:
	case RISCV32_LINUX_TCSETSF:
		if (riscv32_copyin(arg, termios, sizeof(termios)) != 0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_tty_set_termios(termios,
		    request == RISCV32_LINUX_TCSETSF);
		riscv32_linux_return_value(state, 0);
		return TRUE;
	case RISCV32_LINUX_TCGETA:
		if (!riscv32_linux_copyout_termio(arg)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	case RISCV32_LINUX_TCSETA:
	case RISCV32_LINUX_TCSETAW:
	case RISCV32_LINUX_TCSETAF:
		if (!riscv32_user_range_valid(arg, RISCV32_LINUX_TERMIO_SIZE)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	case RISCV32_LINUX_TIOCGWINSZ:
		if (!riscv32_linux_copyout_winsize(arg)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	case RISCV32_LINUX_TIOCSWINSZ:
		if (!riscv32_user_range_valid(arg, RISCV32_LINUX_WINSIZE_SIZE)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	case RISCV32_LINUX_TIOCGPGRP:
		if (!riscv32_linux_copyout_u32(arg,
		    riscv32_tty_get_foreground_pgrp((uint32_t *)0))) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	case RISCV32_LINUX_TIOCGSID:
		(void)riscv32_tty_get_foreground_pgrp(&sid);
		if (!riscv32_linux_copyout_u32(arg, sid)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	case RISCV32_LINUX_TIOCSPGRP:
		if (riscv32_copyin(arg, &pgid, sizeof(pgid)) != 0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		if (pgid == 0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
			return TRUE;
		}
		process = riscv32_linux_process_current();
		if (process == (riscv32_linux_process_t *)0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_ESRCH);
			return TRUE;
		}
		simple_lock(&process->lock);
		sid = process->personality.sid;
		simple_unlock(&process->lock);
		if (!riscv32_linux_process_group_exists(pgid, sid) ||
		    !riscv32_tty_set_foreground_pgrp(pgid, sid)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_ESRCH);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	case RISCV32_LINUX_FIONREAD:
		if (!riscv32_linux_copyout_u32(arg,
		    riscv32_tty_readable_bytes())) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	case RISCV32_LINUX_TIOCNOTTY:
		riscv32_linux_return_value(state, 0);
		return TRUE;
	default:
		break;
	}
	riscv32_linux_return_errno(state, RISCV32_LINUX_ENOTTY);
	return TRUE;
}

static boolean_t
riscv32_linux_copyout_stat64(uint32_t user_stat, uint32_t mode,
    uint32_t rdev, uint32_t size)
{
	uint8_t st[RISCV32_LINUX_STAT64_SIZE];

	riscv32_linux_zero(st, sizeof(st));
	riscv32_linux_put_u64le(st, 0, 1);
	riscv32_linux_put_u64le(st, 8, rdev != 0 ? 3 : 2);
	riscv32_linux_put_u32le(st, 16, mode);
	riscv32_linux_put_u32le(st, 20, 1);
	riscv32_linux_put_u32le(st, 24, 0);
	riscv32_linux_put_u32le(st, 28, 0);
	riscv32_linux_put_u64le(st, 32, rdev);
	riscv32_linux_put_u64le(st, 48, size);
	riscv32_linux_put_u32le(st, 56, 4096);
	riscv32_linux_put_u64le(st, 64, 0);

	return riscv32_copyout(st, user_stat, sizeof(st)) == 0;
}

static boolean_t
riscv32_linux_copyout_console_stat64(uint32_t user_stat)
{
	return riscv32_linux_copyout_stat64(user_stat,
	    RISCV32_LINUX_MODE_IFCHR | RISCV32_LINUX_MODE_0666,
	    RISCV32_LINUX_DEV_CONSOLE, 0);
}

static boolean_t
riscv32_linux_copyout_tty_stat64(uint32_t user_stat)
{
	return riscv32_linux_copyout_stat64(user_stat,
	    RISCV32_LINUX_MODE_IFCHR | RISCV32_LINUX_MODE_0666,
	    RISCV32_LINUX_DEV_TTY, 0);
}

static boolean_t
riscv32_linux_copyout_null_stat64(uint32_t user_stat)
{
	return riscv32_linux_copyout_stat64(user_stat,
	    RISCV32_LINUX_MODE_IFCHR | RISCV32_LINUX_MODE_0666,
	    RISCV32_LINUX_DEV_NULL, 0);
}

static boolean_t
riscv32_linux_copyout_pipe_stat64(uint32_t user_stat)
{
	return riscv32_linux_copyout_stat64(user_stat,
	    RISCV32_LINUX_MODE_IFIFO | RISCV32_LINUX_MODE_0666, 0, 0);
}

static boolean_t
riscv32_linux_copyout_anon_stat64(uint32_t user_stat)
{
	return riscv32_linux_copyout_stat64(user_stat,
	    RISCV32_LINUX_MODE_IFREG | RISCV32_LINUX_MODE_0666, 0, 0);
}

static boolean_t
riscv32_linux_copyout_proc_data_stat64(uint32_t user_stat, uint32_t size)
{
	return riscv32_linux_copyout_stat64(user_stat,
	    RISCV32_LINUX_MODE_IFREG | RISCV32_LINUX_MODE_0666, 0,
	    size);
}

static boolean_t
riscv32_linux_copyout_dir_stat64(uint32_t user_stat)
{
	return riscv32_linux_copyout_stat64(user_stat,
	    RISCV32_LINUX_MODE_IFDIR | RISCV32_LINUX_MODE_0755, 0, 0);
}

static boolean_t
riscv32_linux_copyout_file_stat64(uint32_t user_stat,
    const riscv32_linux_initrd_file_t *file)
{
	uint32_t mode;

	mode = file->mode;
	if ((mode & RISCV32_LINUX_MODE_IFMT) == 0) {
		mode |= RISCV32_LINUX_MODE_IFREG;
	}
	return riscv32_linux_copyout_stat64(user_stat, mode, 0, file->size);
}

static boolean_t
riscv32_linux_copyout_statx(uint32_t user_statx, uint32_t mode,
    uint32_t rdev, uint32_t size, uint32_t ino)
{
	uint8_t stx[RISCV32_LINUX_STATX_SIZE];

	riscv32_linux_zero(stx, sizeof(stx));
	riscv32_linux_put_u32le(stx, 0, RISCV32_LINUX_STATX_BASIC_STATS);
	riscv32_linux_put_u32le(stx, 4, 4096);
	riscv32_linux_put_u64le(stx, 8, 0);
	riscv32_linux_put_u32le(stx, 16, 1);
	riscv32_linux_put_u32le(stx, 20, 0);
	riscv32_linux_put_u32le(stx, 24, 0);
	riscv32_linux_put_u16le(stx, 28, mode);
	riscv32_linux_put_u64le(stx, 32, ino);
	riscv32_linux_put_u64le(stx, 40, size);
	riscv32_linux_put_u64le(stx, 48, 0);
	riscv32_linux_put_u64le(stx, 56, RISCV32_LINUX_STATX_BASIC_STATS);
	if (rdev != 0) {
		riscv32_linux_put_u32le(stx, 128, rdev >> 8);
		riscv32_linux_put_u32le(stx, 132, rdev & 0xffU);
	}
	riscv32_linux_put_u32le(stx, 136, 0);
	riscv32_linux_put_u32le(stx, 140, 1);
	riscv32_linux_put_u64le(stx, 144, 1);
	return riscv32_copyout(stx, user_statx, sizeof(stx)) == 0;
}

static boolean_t
riscv32_linux_copyout_console_statx(uint32_t user_statx)
{
	return riscv32_linux_copyout_statx(user_statx,
	    RISCV32_LINUX_MODE_IFCHR | RISCV32_LINUX_MODE_0666,
	    RISCV32_LINUX_DEV_CONSOLE, 0, 3);
}

static boolean_t
riscv32_linux_copyout_tty_statx(uint32_t user_statx)
{
	return riscv32_linux_copyout_statx(user_statx,
	    RISCV32_LINUX_MODE_IFCHR | RISCV32_LINUX_MODE_0666,
	    RISCV32_LINUX_DEV_TTY, 0, 34);
}

static boolean_t
riscv32_linux_copyout_null_statx(uint32_t user_statx)
{
	return riscv32_linux_copyout_statx(user_statx,
	    RISCV32_LINUX_MODE_IFCHR | RISCV32_LINUX_MODE_0666,
	    RISCV32_LINUX_DEV_NULL, 0, 35);
}

static boolean_t
riscv32_linux_copyout_pipe_statx(uint32_t user_statx)
{
	return riscv32_linux_copyout_statx(user_statx,
	    RISCV32_LINUX_MODE_IFIFO | RISCV32_LINUX_MODE_0666, 0, 0, 9);
}

static boolean_t
riscv32_linux_copyout_anon_statx(uint32_t user_statx)
{
	return riscv32_linux_copyout_statx(user_statx,
	    RISCV32_LINUX_MODE_IFREG | RISCV32_LINUX_MODE_0666, 0, 0, 10);
}

static boolean_t
riscv32_linux_copyout_proc_data_statx(uint32_t user_statx, uint32_t kind,
    uint32_t size)
{
	return riscv32_linux_copyout_statx(user_statx,
	    RISCV32_LINUX_MODE_IFREG | RISCV32_LINUX_MODE_0666, 0,
	    size, riscv32_linux_proc_ino_for_kind(kind));
}

static boolean_t
riscv32_linux_copyout_dir_statx(uint32_t user_statx, uint32_t ino)
{
	return riscv32_linux_copyout_statx(user_statx,
	    RISCV32_LINUX_MODE_IFDIR | RISCV32_LINUX_MODE_0755, 0, 0, ino);
}

static boolean_t
riscv32_linux_copyout_file_statx(uint32_t user_statx,
    const riscv32_linux_initrd_file_t *file)
{
	uint32_t mode = file->mode;

	if ((mode & RISCV32_LINUX_MODE_IFMT) == 0) {
		mode |= RISCV32_LINUX_MODE_IFREG;
	}
	return riscv32_linux_copyout_statx(user_statx, mode, 0, file->size, 8);
}

static boolean_t
riscv32_linux_copyout_fd_target_stat64(uint32_t user_stat,
    const riscv32_linux_fd_t *desc)
{
	const uint8_t *proc_data;
	uint32_t proc_size;

	if (desc->kind == RISCV32_LINUX_FD_CONSOLE) {
		return riscv32_linux_copyout_console_stat64(user_stat);
	}
	if (desc->kind == RISCV32_LINUX_FD_NULL) {
		return riscv32_linux_copyout_null_stat64(user_stat);
	}
	if (desc->kind == RISCV32_LINUX_FD_DIR) {
		return riscv32_linux_copyout_dir_stat64(user_stat);
	}
	if (desc->kind == RISCV32_LINUX_FD_INITRD) {
		return riscv32_linux_copyout_file_stat64(user_stat,
		    &desc->file);
	}
	if (desc->kind == RISCV32_LINUX_FD_PIPE) {
		return riscv32_linux_copyout_pipe_stat64(user_stat);
	}
	if (desc->kind == RISCV32_LINUX_FD_EVENTFD ||
	    desc->kind == RISCV32_LINUX_FD_EPOLL ||
	    desc->kind == RISCV32_LINUX_FD_TIMERFD ||
	    desc->kind == RISCV32_LINUX_FD_INOTIFY) {
		return riscv32_linux_copyout_anon_stat64(user_stat);
	}
	if (desc->kind == RISCV32_LINUX_FD_PROC_SELF_FDINFO) {
		if (!riscv32_linux_proc_self_fdinfo_data(
		    (uint32_t)desc->value, &proc_data, &proc_size)) {
			return FALSE;
		}
		return riscv32_linux_copyout_proc_data_stat64(user_stat,
		    proc_size);
	}
	if (riscv32_linux_proc_data_for_kind(desc->kind, &proc_data,
	    &proc_size)) {
		return riscv32_linux_copyout_proc_data_stat64(user_stat,
		    proc_size);
	}
	return FALSE;
}

static boolean_t
riscv32_linux_copyout_fd_target_statx(uint32_t user_statx,
    const riscv32_linux_fd_t *desc)
{
	const uint8_t *proc_data;
	uint32_t proc_size;
	uint32_t ino;

	if (desc->kind == RISCV32_LINUX_FD_CONSOLE) {
		return riscv32_linux_copyout_console_statx(user_statx);
	}
	if (desc->kind == RISCV32_LINUX_FD_NULL) {
		return riscv32_linux_copyout_null_statx(user_statx);
	}
	if (desc->kind == RISCV32_LINUX_FD_DIR) {
		ino = desc->dir_kind == RISCV32_LINUX_DIR_INITRD ?
		    (uint32_t)desc->value :
		    riscv32_linux_synthetic_dir_ino(desc->dir_kind);
		return riscv32_linux_copyout_dir_statx(user_statx, ino);
	}
	if (desc->kind == RISCV32_LINUX_FD_INITRD) {
		return riscv32_linux_copyout_file_statx(user_statx,
		    &desc->file);
	}
	if (desc->kind == RISCV32_LINUX_FD_PIPE) {
		return riscv32_linux_copyout_pipe_statx(user_statx);
	}
	if (desc->kind == RISCV32_LINUX_FD_EVENTFD ||
	    desc->kind == RISCV32_LINUX_FD_EPOLL ||
	    desc->kind == RISCV32_LINUX_FD_TIMERFD ||
	    desc->kind == RISCV32_LINUX_FD_INOTIFY) {
		return riscv32_linux_copyout_anon_statx(user_statx);
	}
	if (desc->kind == RISCV32_LINUX_FD_PROC_SELF_FDINFO) {
		if (!riscv32_linux_proc_self_fdinfo_data(
		    (uint32_t)desc->value, &proc_data, &proc_size)) {
			return FALSE;
		}
		return riscv32_linux_copyout_statx(user_statx,
		    RISCV32_LINUX_MODE_IFREG | RISCV32_LINUX_MODE_0666,
		    0, proc_size, 200U + (uint32_t)desc->value);
	}
	if (riscv32_linux_proc_data_for_kind(desc->kind, &proc_data,
	    &proc_size)) {
		return riscv32_linux_copyout_proc_data_statx(user_statx,
		    desc->kind, proc_size);
	}
	return FALSE;
}

static boolean_t
riscv32_linux_copyout_fd_link_stat64(uint32_t user_stat, uint32_t fd,
    const char *target)
{
	(void)fd;
	return riscv32_linux_copyout_stat64(user_stat,
	    RISCV32_LINUX_MODE_IFLNK | RISCV32_LINUX_MODE_0755, 0,
	    riscv32_linux_strlen(target));
}

static boolean_t
riscv32_linux_copyout_fd_link_statx(uint32_t user_statx, uint32_t fd,
    const char *target)
{
	return riscv32_linux_copyout_statx(user_statx,
	    RISCV32_LINUX_MODE_IFLNK | RISCV32_LINUX_MODE_0755, 0,
	    riscv32_linux_strlen(target), 100U + fd);
}

static boolean_t
riscv32_linux_copyout_statfs64(uint32_t user_statfs)
{
	uint8_t stfs[RISCV32_LINUX_STATFS64_SIZE];
	uint64_t blocks = RISCV32_LINUX_BOOT_RAM_SIZE /
	    RISCV32_LINUX_MMAP_PAGE_SIZE;

	riscv32_linux_zero(stfs, sizeof(stfs));
	riscv32_linux_put_u32le(stfs, 0, RISCV32_LINUX_HFSPLUS_SUPER_MAGIC);
	riscv32_linux_put_u32le(stfs, 4, RISCV32_LINUX_MMAP_PAGE_SIZE);
	riscv32_linux_put_u64le(stfs, 8, blocks);
	riscv32_linux_put_u64le(stfs, 16, blocks / 2U);
	riscv32_linux_put_u64le(stfs, 24, blocks / 2U);
	riscv32_linux_put_u64le(stfs, 32, 1024);
	riscv32_linux_put_u64le(stfs, 40, 512);
	riscv32_linux_put_u32le(stfs, 56, RISCV32_LINUX_PATH_MAX - 1U);
	riscv32_linux_put_u32le(stfs, 60, RISCV32_LINUX_MMAP_PAGE_SIZE);
	return riscv32_copyout(stfs, user_statfs, sizeof(stfs)) == 0;
}

static boolean_t
riscv32_linux_sys_fstat64(riscv_saved_state_t *state)
{
	uint32_t fd = state->thread.x[RISCV_X_A0];
	uint32_t user_stat = state->thread.x[RISCV_X_A1];
	riscv32_linux_fd_t *desc;
	const uint8_t *proc_data;
	uint32_t proc_size;

	if (riscv32_linux_fd_is_console(fd)) {
		if (!riscv32_linux_copyout_console_stat64(user_stat)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	desc = riscv32_linux_fd_ref(fd);
	if (desc == (riscv32_linux_fd_t *)0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	if (desc->kind == RISCV32_LINUX_FD_NULL) {
		if (!riscv32_linux_copyout_null_stat64(user_stat)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (desc->kind == RISCV32_LINUX_FD_PIPE) {
		if (!riscv32_linux_copyout_pipe_stat64(user_stat)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (desc->kind == RISCV32_LINUX_FD_EVENTFD ||
	    desc->kind == RISCV32_LINUX_FD_EPOLL ||
	    desc->kind == RISCV32_LINUX_FD_TIMERFD ||
	    desc->kind == RISCV32_LINUX_FD_INOTIFY) {
		if (!riscv32_linux_copyout_anon_stat64(user_stat)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (desc->kind == RISCV32_LINUX_FD_DIR) {
		if (!riscv32_linux_copyout_dir_stat64(user_stat)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (riscv32_linux_proc_data_for_kind(desc->kind, &proc_data,
	    &proc_size)) {
		if (!riscv32_linux_copyout_proc_data_stat64(user_stat,
		    proc_size)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (desc->kind != RISCV32_LINUX_FD_INITRD) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	if (!riscv32_linux_copyout_file_stat64(user_stat, &desc->file)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_statfs64(riscv_saved_state_t *state)
{
	uint32_t user_path = state->thread.x[RISCV_X_A0];
	uint32_t user_statfs = state->thread.x[RISCV_X_A1];
	char path[RISCV32_LINUX_PATH_MAX];
	int error;

	error = riscv32_linux_copyin_path(user_path, path, sizeof(path));
	if (error != 0) {
		riscv32_linux_return_errno(state, (uint32_t)error);
		return TRUE;
	}
	if (!riscv32_linux_path_exists(path)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOENT);
		return TRUE;
	}
	if (!riscv32_linux_copyout_statfs64(user_statfs)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_fstatfs64(riscv_saved_state_t *state)
{
	uint32_t fd = state->thread.x[RISCV_X_A0];
	uint32_t user_statfs = state->thread.x[RISCV_X_A1];
	riscv32_linux_fd_t source;

	if (!riscv32_linux_fd_snapshot(fd, &source)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	if (!riscv32_linux_copyout_statfs64(user_statfs)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_fstatat64(riscv_saved_state_t *state)
{
	uint32_t dirfd = state->thread.x[RISCV_X_A0];
	uint32_t user_path = state->thread.x[RISCV_X_A1];
	uint32_t user_stat = state->thread.x[RISCV_X_A2];
	uint32_t flags = state->thread.x[RISCV_X_A3];
	char raw_path[RISCV32_LINUX_PATH_MAX];
	char path[RISCV32_LINUX_PATH_MAX];
	char target[RISCV32_LINUX_PATH_MAX];
	riscv32_linux_initrd_file_t file;
	riscv32_linux_fd_t *desc;
	const uint8_t *proc_data;
	uint32_t fd;
	uint32_t fdinfo_fd;
	uint32_t proc_kind;
	uint32_t proc_size;
	uint32_t error;

	error = riscv32_linux_copyin_path(user_path, raw_path,
	    sizeof(raw_path));
	if (error != 0) {
		riscv32_early_puts("[linux-syscall] execve path copyin FAIL error=");
		riscv32_early_putdec(error);
		riscv32_early_puts(" va=0x");
		riscv32_early_puthex32(user_path);
		riscv32_early_puts("\n");
		riscv32_linux_return_errno(state, error);
		return TRUE;
	}

	if (raw_path[0] == '\0' &&
	    (flags & RISCV32_LINUX_AT_EMPTY_PATH) != 0) {
		if (riscv32_linux_fd_is_console(dirfd)) {
			if (!riscv32_linux_copyout_console_stat64(user_stat)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
			riscv32_linux_return_value(state, 0);
			return TRUE;
		}
		desc = riscv32_linux_fd_ref(dirfd);
		if (desc == (riscv32_linux_fd_t *)0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
			return TRUE;
		}
		if (desc->kind == RISCV32_LINUX_FD_NULL) {
			if (!riscv32_linux_copyout_null_stat64(user_stat)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
			riscv32_linux_return_value(state, 0);
			return TRUE;
		}
		if (desc->kind == RISCV32_LINUX_FD_DIR) {
			if (!riscv32_linux_copyout_dir_stat64(user_stat)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
			riscv32_linux_return_value(state, 0);
			return TRUE;
		}
		if (desc->kind == RISCV32_LINUX_FD_INITRD) {
			if (!riscv32_linux_copyout_file_stat64(user_stat,
			    &desc->file)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
			riscv32_linux_return_value(state, 0);
			return TRUE;
		}
		if (desc->kind == RISCV32_LINUX_FD_PIPE) {
			if (!riscv32_linux_copyout_pipe_stat64(user_stat)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
			riscv32_linux_return_value(state, 0);
			return TRUE;
		}
		if (desc->kind == RISCV32_LINUX_FD_EVENTFD ||
		    desc->kind == RISCV32_LINUX_FD_EPOLL ||
		    desc->kind == RISCV32_LINUX_FD_TIMERFD ||
		    desc->kind == RISCV32_LINUX_FD_INOTIFY) {
			if (!riscv32_linux_copyout_anon_stat64(user_stat)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
			riscv32_linux_return_value(state, 0);
			return TRUE;
		}
		if (desc->kind == RISCV32_LINUX_FD_PROC_SELF_FDINFO) {
			if (!riscv32_linux_proc_self_fdinfo_data(
			    (uint32_t)desc->value, &proc_data, &proc_size) ||
			    !riscv32_linux_copyout_proc_data_stat64(user_stat,
			    proc_size)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
			riscv32_linux_return_value(state, 0);
			return TRUE;
		}
		if (riscv32_linux_proc_data_for_kind(desc->kind, &proc_data,
		    &proc_size)) {
			if (!riscv32_linux_copyout_proc_data_stat64(user_stat,
			    proc_size)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
			riscv32_linux_return_value(state, 0);
			return TRUE;
		}
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	riscv32_linux_resolve_path_at(dirfd, raw_path, path, sizeof(path));
	if (riscv32_linux_fdinfo_path_number(path, &fdinfo_fd)) {
		if (!riscv32_linux_proc_self_fdinfo_data(fdinfo_fd,
		    &proc_data, &proc_size)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_ENOENT);
			return TRUE;
		}
		if (!riscv32_linux_copyout_proc_data_stat64(user_stat,
		    proc_size)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (riscv32_linux_fd_path_number(path, &fd)) {
		desc = riscv32_linux_fd_ref(fd);
		if (desc == (riscv32_linux_fd_t *)0 ||
		    !riscv32_linux_fd_link_target(fd, target,
		    sizeof(target))) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_ENOENT);
			return TRUE;
		}
		if ((flags & RISCV32_LINUX_AT_SYMLINK_NOFOLLOW) != 0) {
			if (!riscv32_linux_copyout_fd_link_stat64(user_stat,
			    fd, target)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
		} else if (!riscv32_linux_copyout_fd_target_stat64(
		    user_stat, desc)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (riscv32_linux_path_is_synthetic_dir(path)) {
		if (!riscv32_linux_copyout_dir_stat64(user_stat)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (riscv32_linux_streq(path, "/dev/console")) {
		if (!riscv32_linux_copyout_console_stat64(user_stat)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (riscv32_linux_streq(path, "/dev/tty")) {
		if (!riscv32_linux_copyout_tty_stat64(user_stat)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (riscv32_linux_streq(path, "/dev/null")) {
		if (!riscv32_linux_copyout_null_stat64(user_stat)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	proc_kind = riscv32_linux_proc_path_kind(path);
	if (proc_kind != RISCV32_LINUX_FD_NONE) {
		if (!riscv32_linux_proc_data_for_kind(proc_kind, &proc_data,
		    &proc_size) ||
		    !riscv32_linux_copyout_proc_data_stat64(user_stat,
		    proc_size)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (riscv32_linux_initrd_lookup(path, &file)) {
		if (riscv32_linux_mode_is_dir(file.mode)) {
			if (!riscv32_linux_copyout_dir_stat64(user_stat)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
		} else if (!riscv32_linux_copyout_file_stat64(user_stat,
		    &file)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}

	riscv32_linux_return_errno(state, RISCV32_LINUX_ENOENT);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_statx(riscv_saved_state_t *state)
{
	uint32_t dirfd = state->thread.x[RISCV_X_A0];
	uint32_t user_path = state->thread.x[RISCV_X_A1];
	uint32_t flags = state->thread.x[RISCV_X_A2];
	uint32_t user_statx = state->thread.x[RISCV_X_A4];
	char raw_path[RISCV32_LINUX_PATH_MAX];
	char path[RISCV32_LINUX_PATH_MAX];
	char target[RISCV32_LINUX_PATH_MAX];
	riscv32_linux_initrd_file_t file;
	riscv32_linux_fd_t *desc;
	const uint8_t *proc_data;
	uint32_t dir_kind;
	uint32_t fd;
	uint32_t fdinfo_fd;
	uint32_t proc_kind;
	uint32_t proc_size;
	uint32_t error;

	error = riscv32_linux_copyin_path(user_path, raw_path,
	    sizeof(raw_path));
	if (error != 0) {
		riscv32_linux_return_errno(state, error);
		return TRUE;
	}
	if (raw_path[0] == '\0' &&
	    (flags & RISCV32_LINUX_AT_EMPTY_PATH) != 0) {
		if (riscv32_linux_fd_is_console(dirfd)) {
			if (!riscv32_linux_copyout_console_statx(user_statx)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
			riscv32_linux_return_value(state, 0);
			return TRUE;
		}
		desc = riscv32_linux_fd_ref(dirfd);
		if (desc == (riscv32_linux_fd_t *)0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
			return TRUE;
		}
		if (desc->kind == RISCV32_LINUX_FD_NULL) {
			if (!riscv32_linux_copyout_null_statx(user_statx)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
			riscv32_linux_return_value(state, 0);
			return TRUE;
		}
		if (desc->kind == RISCV32_LINUX_FD_DIR) {
			uint32_t ino = desc->dir_kind == RISCV32_LINUX_DIR_INITRD ?
			    (uint32_t)desc->value :
			    riscv32_linux_synthetic_dir_ino(desc->dir_kind);

			if (!riscv32_linux_copyout_dir_statx(user_statx,
			    ino)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
			riscv32_linux_return_value(state, 0);
			return TRUE;
		}
		if (desc->kind == RISCV32_LINUX_FD_INITRD) {
			if (!riscv32_linux_copyout_file_statx(user_statx,
			    &desc->file)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
			riscv32_linux_return_value(state, 0);
			return TRUE;
		}
		if (desc->kind == RISCV32_LINUX_FD_PIPE) {
			if (!riscv32_linux_copyout_pipe_statx(user_statx)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
			riscv32_linux_return_value(state, 0);
			return TRUE;
		}
		if (desc->kind == RISCV32_LINUX_FD_EVENTFD ||
		    desc->kind == RISCV32_LINUX_FD_EPOLL ||
		    desc->kind == RISCV32_LINUX_FD_TIMERFD ||
		    desc->kind == RISCV32_LINUX_FD_INOTIFY) {
			if (!riscv32_linux_copyout_anon_statx(user_statx)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
			riscv32_linux_return_value(state, 0);
			return TRUE;
		}
		if (desc->kind == RISCV32_LINUX_FD_PROC_SELF_FDINFO) {
			if (!riscv32_linux_proc_self_fdinfo_data(
			    (uint32_t)desc->value, &proc_data, &proc_size) ||
			    !riscv32_linux_copyout_statx(user_statx,
			    RISCV32_LINUX_MODE_IFREG |
			    RISCV32_LINUX_MODE_0666, 0, proc_size,
			    200U + (uint32_t)desc->value)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
			riscv32_linux_return_value(state, 0);
			return TRUE;
		}
		proc_kind = desc->kind;
		if (riscv32_linux_proc_data_for_kind(proc_kind, &proc_data,
		    &proc_size)) {
			if (!riscv32_linux_copyout_proc_data_statx(user_statx,
			    proc_kind, proc_size)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
			riscv32_linux_return_value(state, 0);
			return TRUE;
		}
		riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
		return TRUE;
	}
	riscv32_linux_resolve_path_at(dirfd, raw_path, path, sizeof(path));
	if (riscv32_linux_fdinfo_path_number(path, &fdinfo_fd)) {
		if (!riscv32_linux_proc_self_fdinfo_data(fdinfo_fd,
		    &proc_data, &proc_size)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_ENOENT);
			return TRUE;
		}
		if (!riscv32_linux_copyout_statx(user_statx,
		    RISCV32_LINUX_MODE_IFREG | RISCV32_LINUX_MODE_0666, 0,
		    proc_size, 200U + fdinfo_fd)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (riscv32_linux_fd_path_number(path, &fd)) {
		desc = riscv32_linux_fd_ref(fd);
		if (desc == (riscv32_linux_fd_t *)0 ||
		    !riscv32_linux_fd_link_target(fd, target,
		    sizeof(target))) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_ENOENT);
			return TRUE;
		}
		if ((flags & RISCV32_LINUX_AT_SYMLINK_NOFOLLOW) != 0) {
			if (!riscv32_linux_copyout_fd_link_statx(user_statx,
			    fd, target)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
		} else if (!riscv32_linux_copyout_fd_target_statx(user_statx,
		    desc)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (riscv32_linux_path_is_synthetic_dir(path)) {
		dir_kind = riscv32_linux_synthetic_dir_kind(path);
		if (!riscv32_linux_copyout_dir_statx(user_statx,
		    riscv32_linux_synthetic_dir_ino(dir_kind))) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (riscv32_linux_streq(path, "/dev/console")) {
		if (!riscv32_linux_copyout_console_statx(user_statx)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (riscv32_linux_streq(path, "/dev/tty")) {
		if (!riscv32_linux_copyout_tty_statx(user_statx)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (riscv32_linux_streq(path, "/dev/null")) {
		if (!riscv32_linux_copyout_null_statx(user_statx)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (riscv32_linux_streq(path, "/proc/self/exe")) {
		if (!riscv32_linux_copyout_statx(user_statx,
		    RISCV32_LINUX_MODE_IFLNK | RISCV32_LINUX_MODE_0755, 0,
		    11, 7)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (riscv32_linux_initrd_dir_exists(path)) {
		if (!riscv32_linux_copyout_dir_statx(user_statx, 8)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	proc_kind = riscv32_linux_proc_path_kind(path);
	if (proc_kind != RISCV32_LINUX_FD_NONE) {
		if (!riscv32_linux_proc_data_for_kind(proc_kind, &proc_data,
		    &proc_size) ||
		    !riscv32_linux_copyout_proc_data_statx(user_statx,
		    proc_kind, proc_size)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (riscv32_linux_initrd_lookup(path, &file)) {
		if (riscv32_linux_mode_is_dir(file.mode)) {
			if (!riscv32_linux_copyout_dir_statx(user_statx, 8)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
		} else if (!riscv32_linux_copyout_file_statx(user_statx,
		    &file)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}

	riscv32_linux_return_errno(state, RISCV32_LINUX_ENOENT);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_clock_gettime(riscv_saved_state_t *state, boolean_t time64)
{
	uint32_t user_ts = state->thread.x[RISCV_X_A1];
	uint64_t secs;
	uint64_t nsecs;
	boolean_t ok;

	riscv32_linux_time_now(&secs, &nsecs);
	if (time64) {
		ok = riscv32_linux_copyout_timespec64(user_ts, secs, nsecs);
	} else {
		ok = riscv32_linux_copyout_timespec32(user_ts, secs, nsecs);
	}
	if (!ok) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_clock_getres(riscv_saved_state_t *state, boolean_t time64)
{
	uint32_t user_ts = state->thread.x[RISCV_X_A1];
	boolean_t ok;

	if (user_ts == 0) {
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (time64) {
		ok = riscv32_linux_copyout_timespec64(user_ts, 0, 1);
	} else {
		ok = riscv32_linux_copyout_timespec32(user_ts, 0, 1);
	}
	if (!ok) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_getitimer(riscv_saved_state_t *state)
{
	uint32_t which = state->thread.x[RISCV_X_A0];
	uint32_t user_value = state->thread.x[RISCV_X_A1];

	if (which > RISCV32_LINUX_ITIMER_MAX) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (!riscv32_linux_copyout_zero(user_value,
	    RISCV32_LINUX_ITIMERVAL_SIZE)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_setitimer(riscv_saved_state_t *state)
{
	uint32_t which = state->thread.x[RISCV_X_A0];
	uint32_t user_new = state->thread.x[RISCV_X_A1];
	uint32_t user_old = state->thread.x[RISCV_X_A2];
	uint8_t value[RISCV32_LINUX_ITIMERVAL_SIZE];

	if (which > RISCV32_LINUX_ITIMER_MAX) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (user_new != 0 &&
	    riscv32_copyin(user_new, value, sizeof(value)) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	if (user_old != 0 &&
	    !riscv32_linux_copyout_zero(user_old,
	    RISCV32_LINUX_ITIMERVAL_SIZE)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_gettimeofday(riscv_saved_state_t *state)
{
	uint32_t user_tv = state->thread.x[RISCV_X_A0];
	uint32_t user_tz = state->thread.x[RISCV_X_A1];
	uint8_t tz[8];
	uint64_t secs;
	uint64_t nsecs;

	riscv32_linux_time_now(&secs, &nsecs);
	if (user_tv != 0 &&
	    !riscv32_linux_copyout_timeval32(user_tv, secs, nsecs / 1000ULL)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	if (user_tz != 0) {
		riscv32_linux_zero(tz, sizeof(tz));
		if (riscv32_copyout(tz, user_tz, sizeof(tz)) != 0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_nanosleep(riscv_saved_state_t *state, boolean_t time64)
{
	uint32_t user_req = state->thread.x[RISCV_X_A0];
	uint32_t user_rem = state->thread.x[RISCV_X_A1];
	uint32_t size = time64 ? 16U : 8U;
	riscv32_linux_process_t *process = riscv32_linux_process_current();
	uint64_t duration_ns;
	uint64_t interval;
	uint64_t deadline;
	uint32_t error;
	boolean_t interrupted = FALSE;

	error = riscv32_linux_copyin_timespec_duration(user_req, time64,
	    &duration_ns);
	if (error != 0U) {
		riscv32_linux_return_errno(state, error);
		return TRUE;
	}
	if (user_rem != 0 && !riscv32_linux_copyout_zero(user_rem, size)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	if (duration_ns != 0U) {
		if (process == (riscv32_linux_process_t *)0 ||
		    !riscv32_linux_process_transition(process,
		    RISCV32_LINUX_PROCESS_SLEEPING)) {
			panic("riscv32: nanosleep cannot enter process sleep");
		}
		nanoseconds_to_absolutetime(duration_ns, &interval);
		clock_absolutetime_interval_to_deadline(interval, &deadline);
		assert_wait_deadline((event_t)current_thread(), THREAD_UNINT,
		    deadline);
		__sync_synchronize();
		if (riscv32_linux_signal_has_deliverable(process)) {
			(void)clear_wait(current_thread(), THREAD_AWAKENED);
			interrupted = TRUE;
		} else {
			thread_block(THREAD_CONTINUE_NULL);
			interrupted =
			    riscv32_linux_signal_has_deliverable(process);
		}
		if (!riscv32_linux_process_transition(process,
		    RISCV32_LINUX_PROCESS_RUNNABLE) ||
		    !riscv32_linux_process_transition(process,
		    RISCV32_LINUX_PROCESS_RUNNING)) {
			panic("riscv32: nanosleep cannot resume process");
		}
	}
	if (interrupted) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINTR);
	} else {
		riscv32_linux_return_value(state, 0);
	}
	return TRUE;
}

static boolean_t
riscv32_linux_sys_clock_nanosleep(riscv_saved_state_t *state,
    boolean_t time64)
{
	uint32_t user_req = state->thread.x[RISCV_X_A2];
	uint32_t user_rem = state->thread.x[RISCV_X_A3];
	uint32_t size = time64 ? 16U : 8U;

	if (!riscv32_linux_validate_timespec(user_req, time64)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	if (user_rem != 0 && !riscv32_linux_copyout_zero(user_rem, size)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static uint32_t riscv32_linux_poll_revents(uint32_t fd, uint32_t events);

static boolean_t
riscv32_linux_fdset_isset(const uint8_t *set, uint32_t fd)
{
	return (set[fd / 8U] & (uint8_t)(1U << (fd % 8U))) != 0U;
}

static void
riscv32_linux_fdset_set(uint8_t *set, uint32_t fd)
{
	set[fd / 8U] |= (uint8_t)(1U << (fd % 8U));
}

static uint32_t
riscv32_linux_pselect_copyin_set(uint32_t user_set, uint8_t *set,
    uint32_t set_bytes)
{
	riscv32_linux_zero(set, RISCV32_LINUX_SELECT_FDSET_BYTES);
	if (user_set != 0U && set_bytes != 0U &&
	    riscv32_copyin(user_set, set, set_bytes) != 0) {
		return RISCV32_LINUX_EFAULT;
	}
	return 0U;
}

static uint32_t
riscv32_linux_pselect_copyout_sets(uint32_t user_read, uint32_t user_write,
    uint32_t user_except, const uint8_t *read_set, const uint8_t *write_set,
    const uint8_t *except_set, uint32_t set_bytes)
{
	if (set_bytes == 0U) {
		return 0U;
	}
	if ((user_read != 0U &&
	    riscv32_copyout(read_set, user_read, set_bytes) != 0) ||
	    (user_write != 0U &&
	    riscv32_copyout(write_set, user_write, set_bytes) != 0) ||
	    (user_except != 0U &&
	    riscv32_copyout(except_set, user_except, set_bytes) != 0)) {
		return RISCV32_LINUX_EFAULT;
	}
	return 0U;
}

static uint32_t
riscv32_linux_pselect_install_mask(riscv32_linux_process_t *process,
    uint32_t user_sigarg, uint8_t *old_mask, boolean_t *installed_out)
{
	uint8_t sigarg[8];
	uint8_t new_mask[RISCV32_LINUX_SIGSET_MAX_SIZE];
	uint32_t user_mask;
	uint32_t sigset_size;

	*installed_out = FALSE;
	if (user_sigarg == 0U) {
		return 0U;
	}
	if (riscv32_copyin(user_sigarg, sigarg, sizeof(sigarg)) != 0) {
		return RISCV32_LINUX_EFAULT;
	}
	user_mask = riscv32_linux_get_u32le(sigarg, 0U);
	sigset_size = riscv32_linux_get_u32le(sigarg, 4U);
	if (user_mask == 0U) {
		return 0U;
	}
	if (sigset_size == 0U ||
	    sigset_size > RISCV32_LINUX_SIGSET_MAX_SIZE) {
		return RISCV32_LINUX_EINVAL;
	}
	riscv32_linux_zero(new_mask, sizeof(new_mask));
	if (riscv32_copyin(user_mask, new_mask, sigset_size) != 0) {
		return RISCV32_LINUX_EFAULT;
	}

	simple_lock(&process->lock);
	memcpy(old_mask, process->personality.signal_mask,
	    RISCV32_LINUX_SIGSET_MAX_SIZE);
	memcpy(process->personality.signal_mask, new_mask,
	    RISCV32_LINUX_SIGSET_MAX_SIZE);
	process->personality.signal_mask[(9U - 1U) / 8U] &=
	    (uint8_t)~(1U << ((9U - 1U) % 8U));
	process->personality.signal_mask[(19U - 1U) / 8U] &=
	    (uint8_t)~(1U << ((19U - 1U) % 8U));
	simple_unlock(&process->lock);
	*installed_out = TRUE;
	return 0U;
}

static void
riscv32_linux_pselect_restore_mask(riscv32_linux_process_t *process,
    const uint8_t *old_mask, boolean_t installed)
{
	if (!installed) {
		return;
	}
	simple_lock(&process->lock);
	memcpy(process->personality.signal_mask, old_mask,
	    RISCV32_LINUX_SIGSET_MAX_SIZE);
	simple_unlock(&process->lock);
}

static uint32_t
riscv32_linux_pselect_scan(uint32_t nfds, const uint8_t *read_set,
    const uint8_t *write_set, const uint8_t *except_set,
    uint8_t *ready_read, uint8_t *ready_write, uint8_t *ready_except,
    uint32_t *ready_out, boolean_t *wait_console_out)
{
	riscv32_linux_fd_t *desc;
	uint32_t revents;
	uint32_t events;
	uint32_t fd;
	boolean_t read_requested;
	boolean_t write_requested;
	boolean_t except_requested;
	boolean_t fd_ready;

	riscv32_linux_zero(ready_read, RISCV32_LINUX_SELECT_FDSET_BYTES);
	riscv32_linux_zero(ready_write, RISCV32_LINUX_SELECT_FDSET_BYTES);
	riscv32_linux_zero(ready_except, RISCV32_LINUX_SELECT_FDSET_BYTES);
	*ready_out = 0U;
	*wait_console_out = FALSE;
	for (fd = 0U; fd < nfds; fd++) {
		read_requested = riscv32_linux_fdset_isset(read_set, fd);
		write_requested = riscv32_linux_fdset_isset(write_set, fd);
		except_requested = riscv32_linux_fdset_isset(except_set, fd);
		if (!read_requested && !write_requested && !except_requested) {
			continue;
		}
		desc = riscv32_linux_fd_ref(fd);
		if (desc == (riscv32_linux_fd_t *)0) {
			return RISCV32_LINUX_EBADF;
		}
		events = (read_requested ? RISCV32_LINUX_POLLIN : 0U) |
		    (write_requested ? RISCV32_LINUX_POLLOUT : 0U);
		revents = riscv32_linux_poll_revents(fd, events);
		fd_ready = FALSE;
		if (read_requested && (revents & (RISCV32_LINUX_POLLIN |
		    RISCV32_LINUX_POLLHUP | RISCV32_LINUX_POLLERR)) != 0U) {
			riscv32_linux_fdset_set(ready_read, fd);
			fd_ready = TRUE;
		}
		if (write_requested && (revents & (RISCV32_LINUX_POLLOUT |
		    RISCV32_LINUX_POLLERR)) != 0U) {
			riscv32_linux_fdset_set(ready_write, fd);
			fd_ready = TRUE;
		}
		if (fd_ready) {
			(*ready_out)++;
		}
		if (read_requested && desc->kind == RISCV32_LINUX_FD_CONSOLE &&
		    (revents & RISCV32_LINUX_POLLIN) == 0U) {
			*wait_console_out = TRUE;
		}
	}
	return 0U;
}

static uint32_t
riscv32_linux_pselect_wait_signal(riscv32_linux_process_t *process,
    boolean_t has_deadline, uint64_t deadline, boolean_t *timed_out_out)
{
	*timed_out_out = FALSE;
	for (;;) {
		if (riscv32_linux_signal_has_deliverable(process)) {
			return RISCV32_LINUX_EINTR;
		}
		if (has_deadline && mach_absolute_time() >= deadline) {
			*timed_out_out = TRUE;
			return 0U;
		}
		if (!riscv32_linux_process_transition(process,
		    RISCV32_LINUX_PROCESS_SLEEPING)) {
			panic("riscv32: pselect cannot enter process sleep");
		}
		if (has_deadline) {
			assert_wait_deadline((event_t)current_thread(), THREAD_UNINT,
			    deadline);
		} else {
			assert_wait((event_t)current_thread(), THREAD_UNINT);
		}
		__sync_synchronize();
		if (!riscv32_linux_signal_has_deliverable(process) &&
		    (!has_deadline || mach_absolute_time() < deadline)) {
			thread_block(THREAD_CONTINUE_NULL);
		} else {
			(void)clear_wait(current_thread(), THREAD_AWAKENED);
		}
		if (!riscv32_linux_process_transition(process,
		    RISCV32_LINUX_PROCESS_RUNNABLE) ||
		    !riscv32_linux_process_transition(process,
		    RISCV32_LINUX_PROCESS_RUNNING)) {
			panic("riscv32: pselect cannot resume process");
		}
	}
}

static boolean_t
riscv32_linux_sys_pselect6(riscv_saved_state_t *state, boolean_t time64)
{
	uint32_t nfds = state->thread.x[RISCV_X_A0];
	uint32_t user_read = state->thread.x[RISCV_X_A1];
	uint32_t user_write = state->thread.x[RISCV_X_A2];
	uint32_t user_except = state->thread.x[RISCV_X_A3];
	uint32_t user_timeout = state->thread.x[RISCV_X_A4];
	uint32_t user_sigarg = state->thread.x[RISCV_X_A5];
	uint8_t read_set[RISCV32_LINUX_SELECT_FDSET_BYTES];
	uint8_t write_set[RISCV32_LINUX_SELECT_FDSET_BYTES];
	uint8_t except_set[RISCV32_LINUX_SELECT_FDSET_BYTES];
	uint8_t ready_read[RISCV32_LINUX_SELECT_FDSET_BYTES];
	uint8_t ready_write[RISCV32_LINUX_SELECT_FDSET_BYTES];
	uint8_t ready_except[RISCV32_LINUX_SELECT_FDSET_BYTES];
	uint8_t old_mask[RISCV32_LINUX_SIGSET_MAX_SIZE];
	riscv32_linux_process_t *process = riscv32_linux_process_current();
	uint32_t set_bytes;
	uint32_t ready = 0U;
	uint32_t error = 0U;
	uint32_t result = 0U;
	uint64_t duration_ns = 0U;
	uint64_t interval;
	uint64_t deadline = 0U;
	boolean_t has_deadline = FALSE;
	boolean_t mask_installed = FALSE;
	boolean_t wait_console;
	boolean_t timed_out;
	boolean_t gate11 = riscv32_boot_arg_has("process-gate-11");
	static volatile uint32_t gate11_blocks;
	static volatile uint32_t gate11_wakeups;

	if (nfds > RISCV32_LINUX_SELECT_MAX_FDS ||
	    process == (riscv32_linux_process_t *)0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	set_bytes = (nfds + 7U) / 8U;
	error = riscv32_linux_pselect_copyin_set(user_read, read_set,
	    set_bytes);
	if (error == 0U) {
		error = riscv32_linux_pselect_copyin_set(user_write, write_set,
		    set_bytes);
	}
	if (error == 0U) {
		error = riscv32_linux_pselect_copyin_set(user_except, except_set,
		    set_bytes);
	}
	if (error != 0U) {
		riscv32_linux_return_errno(state, error);
		return TRUE;
	}
	if (user_timeout != 0U) {
		error = riscv32_linux_copyin_timespec_duration(user_timeout,
		    time64, &duration_ns);
		if (error != 0U) {
			riscv32_linux_return_errno(state, error);
			return TRUE;
		}
		has_deadline = TRUE;
		nanoseconds_to_absolutetime(duration_ns, &interval);
		clock_absolutetime_interval_to_deadline(interval, &deadline);
	}
	error = riscv32_linux_pselect_install_mask(process, user_sigarg,
	    old_mask, &mask_installed);
	if (error != 0U) {
		riscv32_linux_return_errno(state, error);
		return TRUE;
	}

	for (;;) {
		error = riscv32_linux_pselect_scan(nfds, read_set, write_set,
		    except_set, ready_read, ready_write, ready_except, &ready,
		    &wait_console);
		if (error != 0U) {
			break;
		}
		if (ready != 0U) {
			error = riscv32_linux_pselect_copyout_sets(user_read,
			    user_write, user_except, ready_read, ready_write,
			    ready_except, set_bytes);
			result = ready;
			break;
		}
		if (riscv32_linux_signal_has_deliverable(process)) {
			error = RISCV32_LINUX_EINTR;
			break;
		}
		if (has_deadline && mach_absolute_time() >= deadline) {
			error = riscv32_linux_pselect_copyout_sets(user_read,
			    user_write, user_except, ready_read, ready_write,
			    ready_except, set_bytes);
			result = 0U;
			break;
		}
		if (wait_console) {
			if (gate11 && __sync_add_and_fetch(&gate11_blocks, 1U) ==
			    1U) {
				riscv32_early_puts("[process-gate-11] pselect6 "
				    "blocking on TTY without polling PASS\n");
			}
			error = riscv32_tty_wait_readable(process, has_deadline,
			    deadline, &timed_out);
			if (error != 0U) {
				break;
			}
			if (timed_out) {
				error = riscv32_linux_pselect_copyout_sets(user_read,
				    user_write, user_except, ready_read, ready_write,
				    ready_except, set_bytes);
				result = 0U;
				break;
			}
			if (gate11 &&
			    __sync_add_and_fetch(&gate11_wakeups, 1U) == 1U) {
				riscv32_early_puts("[process-gate-11] pselect6 "
				    "woke for TTY readiness PASS\n");
			}
			continue;
		}
		error = riscv32_linux_pselect_wait_signal(process, has_deadline,
		    deadline, &timed_out);
		if (error != 0U) {
			break;
		}
		if (timed_out) {
			error = riscv32_linux_pselect_copyout_sets(user_read,
			    user_write, user_except, ready_read, ready_write,
			    ready_except, set_bytes);
			result = 0U;
			break;
		}
	}

	riscv32_linux_pselect_restore_mask(process, old_mask, mask_installed);
	if (error != 0U) {
		riscv32_linux_return_errno(state, error);
	} else {
		riscv32_linux_return_value(state, result);
	}
	return TRUE;
}

static uint32_t
riscv32_linux_poll_revents(uint32_t fd, uint32_t events)
{
	riscv32_linux_fd_t *desc;
	riscv32_linux_pipe_t *pipe;
	uint32_t revents = 0;

	if ((fd & 0x80000000U) != 0) {
		return 0;
	}
	desc = riscv32_linux_fd_ref(fd);
	if (desc == (riscv32_linux_fd_t *)0) {
		return RISCV32_LINUX_POLLNVAL;
	}
	if (desc->kind == RISCV32_LINUX_FD_PIPE) {
		pipe = riscv32_linux_pipe_get((uint32_t)desc->value);
		if (pipe == (riscv32_linux_pipe_t *)0) {
			return RISCV32_LINUX_POLLNVAL;
		}
		if (riscv32_linux_fd_mode_readable(desc)) {
			if ((events & RISCV32_LINUX_POLLIN) != 0 &&
			    pipe->size != 0) {
				revents |= RISCV32_LINUX_POLLIN;
			}
			if (pipe->writers == 0) {
				revents |= RISCV32_LINUX_POLLHUP;
			}
		}
		if (riscv32_linux_fd_mode_writable(desc)) {
			if (pipe->readers == 0) {
				revents |= RISCV32_LINUX_POLLERR;
			} else if ((events & RISCV32_LINUX_POLLOUT) != 0 &&
			    pipe->size < RISCV32_LINUX_PIPE_BUF) {
				revents |= RISCV32_LINUX_POLLOUT;
			}
		}
		return revents;
	}
	if (desc->kind == RISCV32_LINUX_FD_CONSOLE) {
		if (riscv32_linux_fd_mode_readable(desc) &&
		    (events & RISCV32_LINUX_POLLIN) != 0U &&
		    riscv32_tty_poll_readable()) {
			revents |= RISCV32_LINUX_POLLIN;
		}
		if (riscv32_linux_fd_mode_writable(desc) &&
		    (events & RISCV32_LINUX_POLLOUT) != 0U) {
			revents |= RISCV32_LINUX_POLLOUT;
		}
		return revents;
	}
	if (riscv32_linux_fd_mode_readable(desc) &&
	    (events & RISCV32_LINUX_POLLIN) != 0 &&
	    desc->kind != RISCV32_LINUX_FD_CONSOLE &&
	    desc->kind != RISCV32_LINUX_FD_TIMERFD &&
	    desc->kind != RISCV32_LINUX_FD_INOTIFY) {
		if (desc->kind != RISCV32_LINUX_FD_EVENTFD ||
		    desc->value != 0) {
			revents |= RISCV32_LINUX_POLLIN;
		}
	}
	if (riscv32_linux_fd_mode_writable(desc) &&
	    (events & RISCV32_LINUX_POLLOUT) != 0) {
		revents |= RISCV32_LINUX_POLLOUT;
	}
	return revents;
}

static boolean_t
riscv32_linux_sys_ppoll(riscv_saved_state_t *state, boolean_t time64)
{
	uint32_t user_fds = state->thread.x[RISCV_X_A0];
	uint32_t nfds = state->thread.x[RISCV_X_A1];
	uint32_t user_timeout = state->thread.x[RISCV_X_A2];
	uint8_t pollfd[RISCV32_LINUX_POLLFD_SIZE];
	uint8_t revents_buf[2];
	uint32_t fd;
	uint32_t events;
	uint32_t revents;
	uint32_t ready;
	uint32_t i;

	if (nfds > RISCV32_LINUX_POLLFD_MAX) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (user_timeout != 0 &&
	    !riscv32_linux_validate_timespec(user_timeout, time64)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	ready = 0;
	for (i = 0; i < nfds; i++) {
		if (riscv32_copyin(user_fds +
		    i * RISCV32_LINUX_POLLFD_SIZE, pollfd,
		    sizeof(pollfd)) != 0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		fd = riscv32_linux_get_u32le(pollfd, 0);
		events = riscv32_linux_get_u16le(pollfd, 4);
		revents = riscv32_linux_poll_revents(fd, events);
		riscv32_linux_zero(revents_buf, sizeof(revents_buf));
		riscv32_linux_put_u16le(revents_buf, 0, revents);
		if (revents != 0) {
			ready++;
		}
		if (riscv32_copyout(revents_buf, user_fds +
		    i * RISCV32_LINUX_POLLFD_SIZE + 6U,
		    sizeof(revents_buf)) != 0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
	}
	riscv32_linux_return_value(state, ready);
	return TRUE;
}

static int
riscv32_linux_mmap_page_index(uint32_t vaddr)
{
	uint32_t i;

	for (i = 0; i < RISCV32_LINUX_MMAP_MAX_PAGES; i++) {
		if (riscv32_linux_mmap_page_info[i].used != 0 &&
		    riscv32_linux_mmap_page_info[i].vaddr == vaddr) {
			return (int)i;
		}
	}
	return -1;
}

static int
riscv32_linux_mmap_free_slot(void)
{
	uint32_t i;

	for (i = 0; i < RISCV32_LINUX_MMAP_MAX_PAGES; i++) {
		if (riscv32_linux_mmap_page_info[i].used == 0) {
			return (int)i;
		}
	}
	return -1;
}

static boolean_t
riscv32_linux_mmap_range_mapped(uint32_t addr, uint32_t len)
{
	uint32_t end = addr + len;
	uint32_t vaddr;

	for (vaddr = addr; vaddr < end; vaddr += RISCV32_LINUX_MMAP_PAGE_SIZE) {
		if (riscv32_linux_mmap_page_index(vaddr) >= 0) {
			return TRUE;
		}
	}
	return FALSE;
}

static boolean_t
riscv32_linux_mmap_range_resident(uint32_t addr, uint32_t len)
{
	uint32_t end = addr + len;
	uint32_t vaddr;

	for (vaddr = addr; vaddr < end; vaddr += RISCV32_LINUX_MMAP_PAGE_SIZE) {
		if (riscv32_linux_mmap_page_index(vaddr) >= 0) {
			continue;
		}
		if (vaddr >= RISCV32_LINUX_BRK_BASE &&
		    vaddr < RISCV32_LINUX_BRK_BASE +
		    riscv32_linux_brk_mapped_pages *
		    RISCV32_LINUX_MMAP_PAGE_SIZE) {
			continue;
		}
		return FALSE;
	}
	return TRUE;
}

static boolean_t
riscv32_linux_mmap_find_free_vaddr(uint32_t page_count, uint32_t *vaddr_out)
{
	uint32_t first;

	if (page_count == 0 || page_count > RISCV32_LINUX_MMAP_MAX_PAGES ||
	    vaddr_out == (uint32_t *)0) {
		return FALSE;
	}
	for (first = 0; first + page_count <= RISCV32_LINUX_MMAP_MAX_PAGES;
	    first++) {
		uint32_t addr = RISCV32_LINUX_MMAP_BASE +
		    first * RISCV32_LINUX_MMAP_PAGE_SIZE;
		uint32_t len = page_count * RISCV32_LINUX_MMAP_PAGE_SIZE;

		if (!riscv32_linux_mmap_range_mapped(addr, len)) {
			*vaddr_out = addr;
			return TRUE;
		}
	}
	return FALSE;
}

static void
riscv32_linux_mmap_record_page(uint32_t slot, uint32_t vaddr,
    vm_prot_t prot, uint32_t source, uint32_t file_offset, const char *path)
{
	riscv32_linux_mmap_page_t *page =
	    &riscv32_linux_mmap_page_info[slot];

	riscv32_linux_zero(page, sizeof(*page));
	page->used = 1;
	page->vaddr = vaddr;
	page->prot = prot;
	page->source = source;
	page->file_offset = file_offset;
	if (path != (const char *)0 && path[0] != '\0') {
		riscv32_linux_copy_cstr(page->path, sizeof(page->path), path);
	} else {
		riscv32_linux_copy_cstr(page->path, sizeof(page->path),
		    "[anon]");
	}
}

static void
riscv32_linux_mmap_unmap_range(pmap_t pmap, uint32_t addr, uint32_t len)
{
	uint32_t end = addr + len;
	uint32_t vaddr;

	if (pmap != PMAP_NULL) {
		pmap_remove(pmap, addr, end);
	}
	for (vaddr = addr; vaddr < end; vaddr += RISCV32_LINUX_MMAP_PAGE_SIZE) {
		int slot = riscv32_linux_mmap_page_index(vaddr);

		if (slot >= 0) {
			riscv32_linux_zero(&riscv32_linux_mmap_page_info[slot],
			    sizeof(riscv32_linux_mmap_page_info[slot]));
		}
	}
}

static boolean_t
riscv32_linux_mmap_prepare_page(uint8_t *page,
    const riscv32_linux_initrd_file_t *file, uint32_t file_offset)
{
	uint32_t copy_len;

	riscv32_linux_zero(page, RISCV32_LINUX_MMAP_PAGE_SIZE);
	if (file == (const riscv32_linux_initrd_file_t *)0 ||
	    file_offset >= file->size) {
		return TRUE;
	}
	copy_len = file->size - file_offset;
	if (copy_len > RISCV32_LINUX_MMAP_PAGE_SIZE) {
		copy_len = RISCV32_LINUX_MMAP_PAGE_SIZE;
	}
	if (file->data_paddr > UINT32_MAX - file_offset) {
		return FALSE;
	}
	return riscv32_linux_copy_phys_to_kernel(file->data_paddr + file_offset,
	    page, copy_len);
}

static boolean_t
riscv32_linux_mmap_load_range(pmap_t pmap, uint32_t vaddr,
    uint32_t page_count, vm_prot_t prot,
    const riscv32_linux_initrd_file_t *file, uint32_t file_offset,
    const char *path)
{
	int slots[RISCV32_LINUX_MMAP_MAX_PAGES];
	uint32_t page;
	uint32_t source = file == (const riscv32_linux_initrd_file_t *)0 ?
	    RISCV32_LINUX_VMA_SOURCE_ANON : RISCV32_LINUX_VMA_SOURCE_INITRD;

	for (page = 0; page < RISCV32_LINUX_MMAP_MAX_PAGES; page++) {
		slots[page] = -1;
	}
	for (page = 0; page < page_count; page++) {
		int slot = riscv32_linux_mmap_free_slot();

		if (slot < 0) {
			goto fail;
		}
		slots[page] = slot;
		riscv32_linux_mmap_page_info[slot].used = 2;
	}
	for (page = 0; page < page_count; page++) {
		uint32_t page_vaddr = vaddr +
		    page * RISCV32_LINUX_MMAP_PAGE_SIZE;
		uint32_t page_file_offset = file_offset +
		    page * RISCV32_LINUX_MMAP_PAGE_SIZE;
		int slot = slots[page];

		if (!riscv32_linux_mmap_prepare_page(
		    riscv32_linux_mmap_pages[slot], file, page_file_offset) ||
		    !riscv32_linux_enter_static_page(pmap, page_vaddr,
		    riscv32_linux_mmap_pages[slot], prot)) {
			goto fail;
		}
		riscv32_linux_mmap_record_page((uint32_t)slot, page_vaddr,
		    prot, source, page_file_offset, path);
	}
	return TRUE;

fail:
	for (page = 0; page < page_count; page++) {
		if (slots[page] >= 0) {
			riscv32_linux_zero(
			    &riscv32_linux_mmap_page_info[slots[page]],
			    sizeof(riscv32_linux_mmap_page_info[slots[page]]));
		}
	}
	if (pmap != PMAP_NULL) {
		pmap_remove(pmap, vaddr,
		    vaddr + page_count * RISCV32_LINUX_MMAP_PAGE_SIZE);
	}
	return FALSE;
}

static boolean_t
riscv32_linux_sys_brk(riscv_saved_state_t *state)
{
	uint32_t requested = state->thread.x[RISCV_X_A0];
	uint32_t max_brk = RISCV32_LINUX_BRK_BASE +
	    RISCV32_LINUX_BRK_MAX_PAGES * RISCV32_LINUX_MMAP_PAGE_SIZE;
	uint32_t needed_size;
	uint32_t needed_pages;
	uint32_t page;
	pmap_t pmap;

	if (requested == 0) {
		riscv32_linux_return_value(state, riscv32_linux_brk_current);
		return TRUE;
	}
	if (requested < RISCV32_LINUX_BRK_BASE || requested > max_brk) {
		riscv32_linux_return_value(state, riscv32_linux_brk_current);
		return TRUE;
	}

	needed_size = requested - RISCV32_LINUX_BRK_BASE;
	needed_pages = needed_size == 0 ? 0 :
	    riscv32_linux_page_up(needed_size) / RISCV32_LINUX_MMAP_PAGE_SIZE;
	if (needed_pages > RISCV32_LINUX_BRK_MAX_PAGES) {
		riscv32_linux_return_value(state, riscv32_linux_brk_current);
		return TRUE;
	}
	pmap = riscv32_pmap_current();
	if (pmap == PMAP_NULL) {
		riscv32_linux_return_value(state, riscv32_linux_brk_current);
		return TRUE;
	}
	for (page = riscv32_linux_brk_mapped_pages; page < needed_pages; page++) {
		uint32_t vaddr = RISCV32_LINUX_BRK_BASE +
		    page * RISCV32_LINUX_MMAP_PAGE_SIZE;

		if (!riscv32_linux_map_static_page(pmap, vaddr,
		    riscv32_linux_brk_pages[page], VM_PROT_READ | VM_PROT_WRITE)) {
			riscv32_linux_return_value(state,
			    riscv32_linux_brk_current);
			return TRUE;
		}
		riscv32_linux_brk_mapped_pages++;
	}
	riscv32_linux_brk_current = requested;
	riscv32_linux_return_value(state, riscv32_linux_brk_current);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_mmap(riscv_saved_state_t *state)
{
	uint32_t req_addr = state->thread.x[RISCV_X_A0];
	uint32_t len = state->thread.x[RISCV_X_A1];
	uint32_t linux_prot = state->thread.x[RISCV_X_A2];
	uint32_t flags = state->thread.x[RISCV_X_A3];
	uint32_t fd = state->thread.x[RISCV_X_A4];
	uint32_t file_offset = state->thread.x[RISCV_X_A5];
	uint32_t map_size;
	uint32_t page_count;
	uint32_t vaddr;
	vm_prot_t prot;
	pmap_t pmap;
	const riscv32_linux_initrd_file_t *file =
	    (const riscv32_linux_initrd_file_t *)0;
	const char *path = (const char *)0;
	boolean_t fixed;
	riscv32_linux_fd_t *desc;

	if (len == 0 || len > UINT32_MAX - (RISCV32_LINUX_MMAP_PAGE_SIZE - 1U)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if ((flags & RISCV32_LINUX_MAP_PRIVATE) == 0 ||
	    (flags & RISCV32_LINUX_MAP_SHARED) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOSYS);
		return TRUE;
	}
	if ((flags & RISCV32_LINUX_MAP_ANONYMOUS) == 0) {
		if ((file_offset & (RISCV32_LINUX_MMAP_PAGE_SIZE - 1U)) != 0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
			return TRUE;
		}
		desc = riscv32_linux_fd_ref(fd);
		if (desc == (riscv32_linux_fd_t *)0 ||
		    desc->kind != RISCV32_LINUX_FD_INITRD) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
			return TRUE;
		}
		file = &desc->file;
		path = desc->dir_path;
	}

	map_size = riscv32_linux_page_up(len);
	page_count = map_size / RISCV32_LINUX_MMAP_PAGE_SIZE;
	if (page_count == 0 || page_count > RISCV32_LINUX_MMAP_MAX_PAGES) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOMEM);
		return TRUE;
	}
	fixed = (flags & (RISCV32_LINUX_MAP_FIXED |
	    RISCV32_LINUX_MAP_FIXED_NOREPLACE)) != 0;
	if (fixed) {
		if (req_addr == 0 ||
		    (req_addr & (RISCV32_LINUX_MMAP_PAGE_SIZE - 1U)) != 0 ||
		    !riscv32_user_range_valid(req_addr, map_size)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
			return TRUE;
		}
		vaddr = req_addr;
	} else {
		if (!riscv32_linux_mmap_find_free_vaddr(page_count, &vaddr) ||
		    !riscv32_user_range_valid(vaddr, map_size)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_ENOMEM);
			return TRUE;
		}
	}

	prot = riscv32_linux_mmap_prot(linux_prot);
	if (prot == VM_PROT_NONE) {
		prot = VM_PROT_READ;
	}
	pmap = riscv32_pmap_current();
	if (pmap == PMAP_NULL) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOMEM);
		return TRUE;
	}
	if (fixed) {
		if ((flags & RISCV32_LINUX_MAP_FIXED_NOREPLACE) != 0 &&
		    riscv32_linux_mmap_range_mapped(vaddr, map_size)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EEXIST);
			return TRUE;
		}
		riscv32_linux_mmap_unmap_range(pmap, vaddr, map_size);
	}
	if (!riscv32_linux_mmap_load_range(pmap, vaddr, page_count, prot,
	    file, file_offset, path)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOMEM);
		return TRUE;
	}
	riscv32_linux_return_value(state, vaddr);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_munmap(riscv_saved_state_t *state)
{
	uint32_t addr = state->thread.x[RISCV_X_A0];
	uint32_t len = state->thread.x[RISCV_X_A1];
	uint32_t map_size;
	pmap_t pmap;

	if (len == 0 ||
	    (addr & (RISCV32_LINUX_MMAP_PAGE_SIZE - 1U)) != 0 ||
	    len > UINT32_MAX - (RISCV32_LINUX_MMAP_PAGE_SIZE - 1U)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	map_size = riscv32_linux_page_up(len);
	if (!riscv32_user_range_valid(addr, map_size)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	pmap = riscv32_pmap_current();
	riscv32_linux_mmap_unmap_range(pmap, addr, map_size);
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_mprotect(riscv_saved_state_t *state)
{
	uint32_t addr = state->thread.x[RISCV_X_A0];
	uint32_t len = state->thread.x[RISCV_X_A1];
	uint32_t linux_prot = state->thread.x[RISCV_X_A2];
	uint32_t map_size;
	uint32_t vaddr;
	vm_prot_t prot;
	pmap_t pmap;

	if (len == 0 ||
	    (addr & (RISCV32_LINUX_MMAP_PAGE_SIZE - 1U)) != 0 ||
	    len > UINT32_MAX - (RISCV32_LINUX_MMAP_PAGE_SIZE - 1U)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	map_size = riscv32_linux_page_up(len);
	if (!riscv32_user_range_valid(addr, map_size)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (!riscv32_linux_mmap_range_resident(addr, map_size)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOMEM);
		return TRUE;
	}
	prot = riscv32_linux_mmap_prot(linux_prot);
	pmap = riscv32_pmap_current();
	if (pmap == PMAP_NULL) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOMEM);
		return TRUE;
	}
	pmap_protect(pmap, addr, addr + map_size, prot);
	for (vaddr = addr; vaddr < addr + map_size;
	    vaddr += RISCV32_LINUX_MMAP_PAGE_SIZE) {
		int slot = riscv32_linux_mmap_page_index(vaddr);

		if (slot >= 0) {
			riscv32_linux_mmap_page_info[slot].prot = prot;
		}
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_memory_range_noop(riscv_saved_state_t *state)
{
	uint32_t addr = state->thread.x[RISCV_X_A0];
	uint32_t len = state->thread.x[RISCV_X_A1];

	if (len != 0 && !riscv32_user_range_valid(addr, len)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_mremap(riscv_saved_state_t *state)
{
	uint32_t old_addr = state->thread.x[RISCV_X_A0];
	uint32_t old_size = state->thread.x[RISCV_X_A1];
	uint32_t new_size = state->thread.x[RISCV_X_A2];

	if (old_size != 0 && !riscv32_user_range_valid(old_addr, old_size)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	if (new_size <= old_size) {
		riscv32_linux_return_value(state, old_addr);
		return TRUE;
	}
	riscv32_linux_return_errno(state, RISCV32_LINUX_ENOMEM);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_mincore(riscv_saved_state_t *state)
{
	uint32_t addr = state->thread.x[RISCV_X_A0];
	uint32_t len = state->thread.x[RISCV_X_A1];
	uint32_t user_vec = state->thread.x[RISCV_X_A2];
	uint8_t resident[64];
	uint32_t pages;
	uint32_t map_size;
	uint32_t i;

	if (len == 0) {
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if ((addr & (RISCV32_LINUX_MMAP_PAGE_SIZE - 1U)) != 0 ||
	    len > UINT32_MAX - (RISCV32_LINUX_MMAP_PAGE_SIZE - 1U)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	map_size = riscv32_linux_page_up(len);
	if (!riscv32_user_range_valid(addr, map_size)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	pages = map_size / RISCV32_LINUX_MMAP_PAGE_SIZE;
	if (pages > sizeof(resident)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (!riscv32_linux_mmap_range_resident(addr, map_size)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOMEM);
		return TRUE;
	}
	for (i = 0; i < pages; i++) {
		resident[i] = 1U;
	}
	if (riscv32_copyout(resident, user_vec, pages) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_getrandom(riscv_saved_state_t *state)
{
	uint32_t user_buf = state->thread.x[RISCV_X_A0];
	uint32_t count = state->thread.x[RISCV_X_A1];
	uint8_t chunk[64];
	uint32_t limit = count;
	uint32_t copied;
	uint32_t chunk_len;
	uint32_t i;
	uint32_t seed;

	if (limit > RISCV32_LINUX_GETRANDOM_MAX) {
		limit = RISCV32_LINUX_GETRANDOM_MAX;
	}
	if (!riscv32_user_range_valid(user_buf, limit)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	seed = (uint32_t)mach_absolute_time() ^ user_buf ^ limit ^
	    0x43415249U;
	for (copied = 0; copied < limit; copied += chunk_len) {
		chunk_len = limit - copied;
		if (chunk_len > sizeof(chunk)) {
			chunk_len = sizeof(chunk);
		}
		for (i = 0; i < chunk_len; i++) {
			seed = seed * 1664525U + 1013904223U;
			chunk[i] = (uint8_t)(seed >> 24);
		}
		if (riscv32_copyout(chunk, user_buf + copied, chunk_len) != 0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
	}
	riscv32_linux_return_value(state, limit);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_prlimit64(riscv_saved_state_t *state)
{
	uint32_t pid = state->thread.x[RISCV_X_A0];
	uint32_t resource = state->thread.x[RISCV_X_A1];
	uint32_t new_limit = state->thread.x[RISCV_X_A2];
	uint32_t old_limit = state->thread.x[RISCV_X_A3];
	uint64_t cur = UINT32_MAX;
	uint64_t max = UINT32_MAX;

	if (pid > 1) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ESRCH);
		return TRUE;
	}
	if (resource >= RISCV32_LINUX_RLIMIT_NLIMITS) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	(void)new_limit;
	switch (resource) {
	case RISCV32_LINUX_RLIMIT_CPU:
	case RISCV32_LINUX_RLIMIT_FSIZE:
	case RISCV32_LINUX_RLIMIT_DATA:
	case RISCV32_LINUX_RLIMIT_AS:
		cur = UINT32_MAX;
		max = UINT32_MAX;
		break;
	case RISCV32_LINUX_RLIMIT_STACK:
		cur = 8U * 1024U * 1024U;
		max = 8U * 1024U * 1024U;
		break;
	case RISCV32_LINUX_RLIMIT_CORE:
		cur = 0;
		max = 0;
		break;
	case RISCV32_LINUX_RLIMIT_NOFILE:
		cur = RISCV32_LINUX_FD_FIRST + RISCV32_LINUX_FD_MAX;
		max = RISCV32_LINUX_FD_FIRST + RISCV32_LINUX_FD_MAX;
		break;
	default:
		cur = UINT32_MAX;
		max = UINT32_MAX;
		break;
	}
	if (old_limit != 0 &&
	    !riscv32_linux_copyout_rlimit64(old_limit, cur, max)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_sched_getaffinity(riscv_saved_state_t *state)
{
	uint32_t pid = state->thread.x[RISCV_X_A0];
	uint32_t len = state->thread.x[RISCV_X_A1];
	uint32_t user_mask = state->thread.x[RISCV_X_A2];
	uint8_t mask[RISCV32_LINUX_CPUSET_WORD_SIZE];

	if (pid > 1) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ESRCH);
		return TRUE;
	}
	if (len < RISCV32_LINUX_CPUSET_WORD_SIZE ||
	    (len & (RISCV32_LINUX_CPUSET_WORD_SIZE - 1U)) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	riscv32_linux_zero(mask, sizeof(mask));
	mask[0] = 1U;
	if (riscv32_copyout(mask, user_mask, sizeof(mask)) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, sizeof(mask));
	return TRUE;
}

static boolean_t
riscv32_linux_sys_sched_setaffinity(riscv_saved_state_t *state)
{
	uint32_t pid = state->thread.x[RISCV_X_A0];
	uint32_t len = state->thread.x[RISCV_X_A1];
	uint32_t user_mask = state->thread.x[RISCV_X_A2];
	uint32_t mask_word;

	if (pid > 1) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ESRCH);
		return TRUE;
	}
	if (len == 0 ||
	    (len & (RISCV32_LINUX_CPUSET_WORD_SIZE - 1U)) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (riscv32_copyin(user_mask, &mask_word, sizeof(mask_word)) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	if ((mask_word & 1U) == 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_setpriority(riscv_saved_state_t *state)
{
	uint32_t which = state->thread.x[RISCV_X_A0];
	uint32_t who = state->thread.x[RISCV_X_A1];
	int nice_value = (int)state->thread.x[RISCV_X_A2];

	if (which > RISCV32_LINUX_PRIO_USER) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (who > 1) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ESRCH);
		return TRUE;
	}
	if (nice_value < -20 || nice_value > 19) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	riscv32_linux_priority_encoded = (uint32_t)(20 - nice_value);
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_getpriority(riscv_saved_state_t *state)
{
	uint32_t which = state->thread.x[RISCV_X_A0];
	uint32_t who = state->thread.x[RISCV_X_A1];

	if (which > RISCV32_LINUX_PRIO_USER) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (who > 1) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ESRCH);
		return TRUE;
	}
	riscv32_linux_return_value(state, riscv32_linux_priority_encoded);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_sysinfo(riscv_saved_state_t *state)
{
	uint32_t user_info = state->thread.x[RISCV_X_A0];

	if (!riscv32_linux_copyout_sysinfo(user_info)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_personality(riscv_saved_state_t *state)
{
	uint32_t requested = state->thread.x[RISCV_X_A0];
	uint32_t old = riscv32_linux_personality;

	if (requested != RISCV32_LINUX_PERSONALITY_QUERY) {
		riscv32_linux_personality = requested;
	}
	riscv32_linux_return_value(state, old);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_capget(riscv_saved_state_t *state)
{
	uint32_t user_header = state->thread.x[RISCV_X_A0];
	uint32_t user_data = state->thread.x[RISCV_X_A1];
	uint8_t header[8];
	uint32_t pid;

	if (riscv32_copyin(user_header, header, sizeof(header)) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	pid = riscv32_linux_get_u32le(header, 4);
	if (pid > 1) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ESRCH);
		return TRUE;
	}
	riscv32_linux_put_u32le(header, 0, RISCV32_LINUX_CAP_VERSION_3);
	riscv32_linux_put_u32le(header, 4, 0);
	if (riscv32_copyout(header, user_header, sizeof(header)) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	if (user_data != 0 && !riscv32_linux_copyout_zero(user_data, 24U)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_capset(riscv_saved_state_t *state)
{
	uint32_t user_header = state->thread.x[RISCV_X_A0];
	uint32_t user_data = state->thread.x[RISCV_X_A1];
	uint8_t header[8];
	uint8_t data[24];
	uint32_t pid;

	if (riscv32_copyin(user_header, header, sizeof(header)) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	pid = riscv32_linux_get_u32le(header, 4);
	if (pid > 1) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ESRCH);
		return TRUE;
	}
	if (user_data == 0 ||
	    riscv32_copyin(user_data, data, sizeof(data)) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_put_u32le(header, 0, RISCV32_LINUX_CAP_VERSION_3);
	riscv32_linux_put_u32le(header, 4, 0);
	if (riscv32_copyout(header, user_header, sizeof(header)) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static void
riscv32_linux_set_id_if_changed(uint32_t *id, uint32_t value)
{
	if (value != RISCV32_LINUX_ID_NO_CHANGE) {
		*id = value;
	}
}

static boolean_t
riscv32_linux_sys_setuid(riscv_saved_state_t *state)
{
	uint32_t uid = state->thread.x[RISCV_X_A0];

	riscv32_linux_uid = uid;
	riscv32_linux_euid = uid;
	riscv32_linux_suid = uid;
	riscv32_linux_fsuid = uid;
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_setgid(riscv_saved_state_t *state)
{
	uint32_t gid = state->thread.x[RISCV_X_A0];

	riscv32_linux_gid = gid;
	riscv32_linux_egid = gid;
	riscv32_linux_sgid = gid;
	riscv32_linux_fsgid = gid;
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_setreuid(riscv_saved_state_t *state)
{
	riscv32_linux_set_id_if_changed(&riscv32_linux_uid,
	    state->thread.x[RISCV_X_A0]);
	riscv32_linux_set_id_if_changed(&riscv32_linux_euid,
	    state->thread.x[RISCV_X_A1]);
	riscv32_linux_suid = riscv32_linux_euid;
	riscv32_linux_fsuid = riscv32_linux_euid;
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_setregid(riscv_saved_state_t *state)
{
	riscv32_linux_set_id_if_changed(&riscv32_linux_gid,
	    state->thread.x[RISCV_X_A0]);
	riscv32_linux_set_id_if_changed(&riscv32_linux_egid,
	    state->thread.x[RISCV_X_A1]);
	riscv32_linux_sgid = riscv32_linux_egid;
	riscv32_linux_fsgid = riscv32_linux_egid;
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_setresuid(riscv_saved_state_t *state)
{
	riscv32_linux_set_id_if_changed(&riscv32_linux_uid,
	    state->thread.x[RISCV_X_A0]);
	riscv32_linux_set_id_if_changed(&riscv32_linux_euid,
	    state->thread.x[RISCV_X_A1]);
	riscv32_linux_set_id_if_changed(&riscv32_linux_suid,
	    state->thread.x[RISCV_X_A2]);
	riscv32_linux_fsuid = riscv32_linux_euid;
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_setresgid(riscv_saved_state_t *state)
{
	riscv32_linux_set_id_if_changed(&riscv32_linux_gid,
	    state->thread.x[RISCV_X_A0]);
	riscv32_linux_set_id_if_changed(&riscv32_linux_egid,
	    state->thread.x[RISCV_X_A1]);
	riscv32_linux_set_id_if_changed(&riscv32_linux_sgid,
	    state->thread.x[RISCV_X_A2]);
	riscv32_linux_fsgid = riscv32_linux_egid;
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_getresuid(riscv_saved_state_t *state)
{
	if (!riscv32_linux_copyout_u32(state->thread.x[RISCV_X_A0],
	    riscv32_linux_uid) ||
	    !riscv32_linux_copyout_u32(state->thread.x[RISCV_X_A1],
	    riscv32_linux_euid) ||
	    !riscv32_linux_copyout_u32(state->thread.x[RISCV_X_A2],
	    riscv32_linux_suid)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_getresgid(riscv_saved_state_t *state)
{
	if (!riscv32_linux_copyout_u32(state->thread.x[RISCV_X_A0],
	    riscv32_linux_gid) ||
	    !riscv32_linux_copyout_u32(state->thread.x[RISCV_X_A1],
	    riscv32_linux_egid) ||
	    !riscv32_linux_copyout_u32(state->thread.x[RISCV_X_A2],
	    riscv32_linux_sgid)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_setfsuid(riscv_saved_state_t *state)
{
	uint32_t old = riscv32_linux_fsuid;

	riscv32_linux_fsuid = state->thread.x[RISCV_X_A0];
	riscv32_linux_return_value(state, old);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_setfsgid(riscv_saved_state_t *state)
{
	uint32_t old = riscv32_linux_fsgid;

	riscv32_linux_fsgid = state->thread.x[RISCV_X_A0];
	riscv32_linux_return_value(state, old);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_setgroups(riscv_saved_state_t *state)
{
	uint32_t count = state->thread.x[RISCV_X_A0];
	uint32_t user_groups = state->thread.x[RISCV_X_A1];
	uint32_t first_group;

	if (count > RISCV32_LINUX_NGROUPS_MAX) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (count != 0 &&
	    riscv32_copyin(user_groups, &first_group, sizeof(first_group)) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_pid_exists(uint32_t pid)
{
	riscv32_linux_process_t *process;

	if (pid == 0) {
		pid = riscv32_linux_current_pid();
	}
	process = riscv32_linux_process_lookup(pid);
	if (process != (riscv32_linux_process_t *)0) {
		riscv32_linux_process_release(process);
		return TRUE;
	}
	return FALSE;
}

static uint32_t
riscv32_linux_pid_pgid(uint32_t pid)
{
	riscv32_linux_process_t *process;
	uint32_t pgid;

	if (pid == 0) {
		pid = riscv32_linux_current_pid();
	}
	process = riscv32_linux_process_lookup(pid);
	if (process != (riscv32_linux_process_t *)0) {
		simple_lock(&process->lock);
		pgid = process->personality.pgid;
		simple_unlock(&process->lock);
		riscv32_linux_process_release(process);
		return pgid;
	}
	if (pid == riscv32_linux_current_pid()) {
		return riscv32_linux_pgid;
	}
	return 0U;
}

static boolean_t
riscv32_linux_sys_setpgid(riscv_saved_state_t *state)
{
	uint32_t pid = state->thread.x[RISCV_X_A0];
	uint32_t pgid = state->thread.x[RISCV_X_A1];
	riscv32_linux_process_t *process;
	uint32_t current_pid = riscv32_linux_current_pid();

	if (pid == 0) {
		pid = current_pid;
	}
	if (pgid == 0) {
		pgid = pid;
	}
	if (pgid == 0) {
		pgid = riscv32_linux_pgid;
	}
	if (!riscv32_linux_pid_exists(pid)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ESRCH);
		return TRUE;
	}
	process = riscv32_linux_process_lookup(pid);
	if (process != (riscv32_linux_process_t *)0) {
		simple_lock(&process->lock);
		process->personality.pgid = pgid;
		simple_unlock(&process->lock);
		riscv32_linux_process_release(process);
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_setsid(riscv_saved_state_t *state)
{
	uint32_t pid = riscv32_linux_current_pid();

	riscv32_linux_sid = pid;
	riscv32_linux_pgid = pid;
	riscv32_linux_return_value(state, riscv32_linux_sid);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_getpgid(riscv_saved_state_t *state)
{
	uint32_t pid = state->thread.x[RISCV_X_A0];
	uint32_t pgid;

	if (pid == 0) {
		pid = riscv32_linux_current_pid();
	}
	if (!riscv32_linux_pid_exists(pid)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ESRCH);
		return TRUE;
	}
	pgid = riscv32_linux_pid_pgid(pid);
	riscv32_linux_return_value(state, pgid != 0 ? pgid :
	    riscv32_linux_pgid);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_getsid(riscv_saved_state_t *state)
{
	uint32_t pid = state->thread.x[RISCV_X_A0];

	if (pid == 0) {
		pid = riscv32_linux_current_pid();
	}
	if (!riscv32_linux_pid_exists(pid)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ESRCH);
		return TRUE;
	}
	riscv32_linux_return_value(state, riscv32_linux_sid);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_wait_no_children(riscv_saved_state_t *state)
{
	riscv32_linux_return_errno(state, RISCV32_LINUX_ECHILD);
	return TRUE;
}

static boolean_t
riscv32_linux_wait4_selector(riscv32_linux_process_t *parent,
    uint32_t requested, riscv32_linux_wait_selector_t *selector_out,
    uint32_t *id_out)
{
	int32_t signed_requested = (int32_t)requested;

	if (parent == (riscv32_linux_process_t *)0 ||
	    selector_out == (riscv32_linux_wait_selector_t *)0 ||
	    id_out == (uint32_t *)0) {
		return FALSE;
	}
	if (signed_requested > 0) {
		*selector_out = RISCV32_LINUX_WAIT_PID;
		*id_out = requested;
		return TRUE;
	}
	if (signed_requested == -1) {
		*selector_out = RISCV32_LINUX_WAIT_ANY;
		*id_out = 0U;
		return TRUE;
	}
	if (signed_requested == 0) {
		*selector_out = RISCV32_LINUX_WAIT_PGID;
		*id_out = parent->personality.pgid;
		return TRUE;
	}
	if (requested == 0x80000000U) {
		return FALSE;
	}
	*selector_out = RISCV32_LINUX_WAIT_PGID;
	*id_out = (uint32_t)(-signed_requested);
	return TRUE;
}

static boolean_t
riscv32_linux_waitid_selector(riscv32_linux_process_t *parent,
    uint32_t idtype, uint32_t id,
    riscv32_linux_wait_selector_t *selector_out, uint32_t *id_out)
{
	if (parent == (riscv32_linux_process_t *)0 ||
	    selector_out == (riscv32_linux_wait_selector_t *)0 ||
	    id_out == (uint32_t *)0) {
		return FALSE;
	}
	switch (idtype) {
	case RISCV32_LINUX_P_ALL:
		*selector_out = RISCV32_LINUX_WAIT_ANY;
		*id_out = 0U;
		return TRUE;
	case RISCV32_LINUX_P_PID:
		if (id == 0U) {
			return FALSE;
		}
		*selector_out = RISCV32_LINUX_WAIT_PID;
		*id_out = id;
		return TRUE;
	case RISCV32_LINUX_P_PGID:
		*selector_out = RISCV32_LINUX_WAIT_PGID;
		*id_out = id != 0U ? id : parent->personality.pgid;
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

static boolean_t
riscv32_linux_copyout_process_siginfo(uint32_t user_info,
    const riscv32_linux_process_t *child,
    riscv32_linux_wait_event_t event)
{
	uint8_t info[RISCV32_LINUX_SIGINFO_SIZE];
	uint32_t code;
	uint32_t status;

	if (user_info == 0) {
		return TRUE;
	}
	riscv32_linux_zero(info, sizeof(info));
	if (child != (const riscv32_linux_process_t *)0) {
		if (event == RISCV32_LINUX_WAIT_EVENT_STOPPED) {
			code = RISCV32_LINUX_CLD_STOPPED;
			status = child->stop_signal;
		} else if (event == RISCV32_LINUX_WAIT_EVENT_CONTINUED) {
			code = RISCV32_LINUX_CLD_CONTINUED;
			status = RISCV32_LINUX_SIGCONT;
		} else {
			code = child->termination_signal != 0U ?
			    RISCV32_LINUX_CLD_KILLED : RISCV32_LINUX_CLD_EXITED;
			status = child->termination_signal != 0U ?
			    child->termination_signal : child->exit_status & 0xffU;
		}
		riscv32_linux_put_u32le(info, 0, RISCV32_LINUX_SIGCHLD);
		riscv32_linux_put_u32le(info, 8, code);
		riscv32_linux_put_u32le(info, 12, child->pid);
		riscv32_linux_put_u32le(info, 16, child->personality.uid);
		riscv32_linux_put_u32le(info, 20, status);
	}
	return riscv32_copyout(info, user_info, sizeof(info)) == 0;
}

static uint32_t
riscv32_linux_process_wait_status(const riscv32_linux_process_t *child,
    riscv32_linux_wait_event_t event)
{
	if (event == RISCV32_LINUX_WAIT_EVENT_STOPPED) {
		return (child->stop_signal << 8) | 0x7fU;
	}
	if (event == RISCV32_LINUX_WAIT_EVENT_CONTINUED) {
		return 0xffffU;
	}
	if (child->termination_signal != 0U) {
		return child->termination_signal & 0x7fU;
	}
	return (child->exit_status & 0xffU) << 8;
}

static void
riscv32_linux_log_gate13_wait_event(riscv32_linux_process_t *parent,
    uint32_t pid, riscv32_linux_wait_event_t event, const char *api)
{
	if (!riscv32_boot_arg_has("process-gate-13") || parent->pid != 2U ||
	    (event != RISCV32_LINUX_WAIT_EVENT_STOPPED &&
	    event != RISCV32_LINUX_WAIT_EVENT_CONTINUED)) {
		return;
	}
	riscv32_early_puts("[process-gate-13] ");
	riscv32_early_puts(api);
	riscv32_early_puts(" observed child=");
	riscv32_early_putdec(pid);
	if (event == RISCV32_LINUX_WAIT_EVENT_STOPPED) {
		riscv32_early_puts(" code=CLD_STOPPED status=20 PASS\n");
	} else {
		riscv32_early_puts(" code=CLD_CONTINUED status=18 PASS\n");
	}
}

static void
riscv32_linux_clone_wait_child_entered(riscv32_linux_process_t *parent,
    uint32_t child_pid)
{
	riscv32_linux_process_t *child;
	uint32_t flags;

	child = riscv32_linux_process_lookup(child_pid);
	if (child == (riscv32_linux_process_t *)0) {
		panic("riscv32: clone child %u disappeared before start",
		    child_pid);
	}
	for (;;) {
		flags = __sync_fetch_and_add(&child->flags, 0U);
		if ((flags & RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED) != 0U) {
			break;
		}
		if (!riscv32_linux_process_transition(parent,
		    RISCV32_LINUX_PROCESS_SLEEPING)) {
			panic("riscv32: clone parent %u cannot await child start",
			    parent->pid);
		}
		assert_wait((event_t)&child->flags, THREAD_UNINT);
		__sync_synchronize();
		flags = __sync_fetch_and_add(&child->flags, 0U);
		if ((flags & RISCV32_LINUX_PROCESS_FLAG_USER_ENTERED) == 0U) {
			thread_block(THREAD_CONTINUE_NULL);
		} else {
			(void)clear_wait(current_thread(), THREAD_AWAKENED);
		}
		if (!riscv32_linux_process_transition(parent,
		    RISCV32_LINUX_PROCESS_RUNNABLE) ||
		    !riscv32_linux_process_transition(parent,
		    RISCV32_LINUX_PROCESS_RUNNING)) {
			panic("riscv32: clone parent %u cannot resume after child start",
			    parent->pid);
		}
	}
	if (riscv32_boot_arg_has("process-gate-11") &&
	    !riscv32_boot_arg_has("process-gate-14") && parent->pid == 2U) {
		riscv32_early_puts("[process-gate-11] child U-mode rendezvous pid=");
		riscv32_early_putdec(child_pid);
		riscv32_early_puts(" PASS\n");
	}
	riscv32_linux_process_release(child);
}

static boolean_t
riscv32_linux_sys_clone(riscv_saved_state_t *state)
{
	uint32_t flags = state->thread.x[RISCV_X_A0];
	uint32_t signal = flags & 0xffU;
	uint32_t thread_flags = flags & (RISCV32_LINUX_CLONE_VM |
	    RISCV32_LINUX_CLONE_FS | RISCV32_LINUX_CLONE_FILES |
	    RISCV32_LINUX_CLONE_SIGHAND | RISCV32_LINUX_CLONE_THREAD);
	riscv32_linux_process_t *parent = riscv32_linux_process_current();
	uint32_t pid;

	if (thread_flags != 0 || (signal != 0 &&
	    signal != RISCV32_LINUX_SIGCHLD)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOSYS);
		return TRUE;
	}
	if (riscv32_boot_arg_has("process-gate-3")) {
		if (parent != (riscv32_linux_process_t *)0 &&
		    (parent->flags &
		    RISCV32_LINUX_PROCESS_FLAG_GATE3_PARENT) != 0U) {
			if (signal != RISCV32_LINUX_SIGCHLD ||
			    state->thread.x[RISCV_X_A1] != 0U ||
			    state->thread.x[RISCV_X_A2] != 0U ||
			    state->thread.x[RISCV_X_A3] != 0U ||
			    state->thread.x[RISCV_X_A4] != 0U ||
			    !riscv32_linux_exec_gate3_clone_current(state, &pid)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EAGAIN);
				return TRUE;
			}
			riscv32_early_puts(
			    "[linux-clone] Gate 3 real child task/thread pid=");
			riscv32_early_putdec(pid);
			riscv32_early_puts(" parent=");
			riscv32_early_putdec(parent->pid);
			riscv32_early_puts(" PASS\n");
			riscv32_linux_return_value(state, pid);
			return TRUE;
		}
	}
	if (parent == (riscv32_linux_process_t *)0 ||
	    signal != RISCV32_LINUX_SIGCHLD ||
	    state->thread.x[RISCV_X_A1] != 0U ||
	    !riscv32_linux_exec_fork_current(state, &pid)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EAGAIN);
		return TRUE;
	}
	if ((parent->flags & (RISCV32_LINUX_PROCESS_FLAG_GATE4_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_GATE5_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_GATE6_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_GATE7_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_GATE8_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_GATE9_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_GATE10_PARENT)) == 0U &&
	    !(riscv32_linux_gate14_stress_enabled() &&
	    (parent->pid == 2U || (parent->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE14_DRIVER) != 0U))) {
		riscv32_early_puts("[linux-clone] real task/thread child pid=");
		riscv32_early_putdec(pid);
		riscv32_early_puts(" parent=");
		riscv32_early_putdec(parent->pid);
		riscv32_early_puts(riscv32_boot_arg_has("process-gate-14") &&
		    parent->pid == 2U ? " affinity=cpu1\n" : " affinity=cpu0\n");
	}
	if (riscv32_boot_arg_has("process-gate-11") &&
	    !riscv32_boot_arg_has("process-gate-14") && parent->pid == 2U) {
		riscv32_early_puts(
		    "[process-gate-11] fork child-first rendezvous armed PASS\n");
	}
	riscv32_linux_clone_wait_child_entered(parent, pid);
	riscv32_linux_return_value(state, pid);
	return TRUE;
}

static boolean_t
riscv32_linux_wait_block_sequence(riscv32_linux_process_t *parent,
    event_t event, uint32_t *sequence, uint32_t expected)
{
	boolean_t blocked = FALSE;

	if (!riscv32_linux_process_transition(parent,
	    RISCV32_LINUX_PROCESS_SLEEPING)) {
		panic("riscv32: PID %u cannot enter wait sleep", parent->pid);
	}
	assert_wait(event, THREAD_UNINT);
	__sync_synchronize();
	if (__sync_fetch_and_add(sequence, 0U) == expected &&
	    !riscv32_linux_signal_has_deliverable(parent)) {
		(void)__sync_add_and_fetch(&parent->waiter_count, 1U);
		riscv32_linux_process_note_wait_block(parent);
		thread_block(THREAD_CONTINUE_NULL);
		riscv32_linux_process_note_wait_resume(parent);
		(void)__sync_sub_and_fetch(&parent->waiter_count, 1U);
		blocked = TRUE;
	} else {
		(void)clear_wait(current_thread(), THREAD_AWAKENED);
	}
	if (!riscv32_linux_process_transition(parent,
	    RISCV32_LINUX_PROCESS_RUNNABLE) ||
	    !riscv32_linux_process_transition(parent,
	    RISCV32_LINUX_PROCESS_RUNNING)) {
		panic("riscv32: PID %u cannot resume from wait", parent->pid);
	}
	return blocked;
}

static boolean_t
riscv32_linux_sys_wait4_real(riscv_saved_state_t *state,
    riscv32_linux_process_t *parent,
    riscv32_linux_wait_selector_t selector, uint32_t id,
    uint32_t user_status, uint32_t options, uint32_t user_usage)
{
	riscv32_linux_process_t *child;
	riscv32_linux_wait_event_t event;
	boolean_t has_child;
	uint32_t expected;
	uint32_t event_mask = RISCV32_LINUX_WAIT_EVENT_MASK_EXITED;
	uint32_t status;
	uint32_t pid;
	uint32_t child_last_cpu;
	uint32_t child_entry_count;
	uint32_t child_pmap_release;
	uint32_t child_xnu_release;
	uint32_t child_gate14_cycle;
	uint32_t child_gate14_stage;
	boolean_t announced = FALSE;
	boolean_t gate14_child;
	boolean_t verbose_wait =
	    (parent->flags & (RISCV32_LINUX_PROCESS_FLAG_GATE4_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_GATE6_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_GATE7_PARENT)) == 0U ||
	    parent->child_create_count == 1U ||
	    (parent->child_create_count % 100U) == 0U;
	if (riscv32_linux_gate14_stress_enabled() &&
	    (parent->pid == 2U || (parent->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE14_DRIVER) != 0U)) {
		verbose_wait = riscv32_linux_gate14_should_log_cycle(
		    parent->child_create_count);
		if ((parent->flags &
		    RISCV32_LINUX_PROCESS_FLAG_GATE14_DRIVER) != 0U) {
			/* CPU1 emits the sampled exec record while this wait begins. */
			verbose_wait = FALSE;
		}
	}

	if ((options & RISCV32_LINUX_WSTOPPED) != 0U) {
		event_mask |= RISCV32_LINUX_WAIT_EVENT_MASK_STOPPED;
	}
	if ((options & RISCV32_LINUX_WCONTINUED) != 0U) {
		event_mask |= RISCV32_LINUX_WAIT_EVENT_MASK_CONTINUED;
	}

	for (;;) {
		if (riscv32_linux_signal_has_deliverable(parent)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EINTR);
			return TRUE;
		}
		if (!riscv32_linux_process_wait_select_event(parent, selector, id,
		    event_mask, TRUE, &child, &event, &has_child, &expected)) {
			panic("riscv32: invalid real wait selector");
		}
		if (child != (riscv32_linux_process_t *)0) {
			break;
		}
		if (!has_child) {
			return riscv32_linux_sys_wait_no_children(state);
		}
		if ((options & RISCV32_LINUX_WNOHANG) != 0U) {
			riscv32_linux_process_note_wait_nohang();
			riscv32_linux_return_value(state, 0U);
			return TRUE;
		}
		if (!announced) {
			if (verbose_wait) {
				riscv32_early_puts("[linux-wait] parent=");
				riscv32_early_putdec(parent->pid);
				riscv32_early_puts(" blocking selector=");
				riscv32_early_putdec((uint32_t)selector);
				riscv32_early_puts(" id=");
				riscv32_early_putdec(id);
				riscv32_early_puts(" without polling\n");
			}
			announced = TRUE;
		}
		(void)riscv32_linux_wait_block_sequence(parent,
		    parent->wait_channel, &parent->wait_sequence, expected);
	}

	while (event == RISCV32_LINUX_WAIT_EVENT_EXITED &&
	    __sync_fetch_and_add(&child->pmap_release_sequence, 0U) == 0U) {
		if (riscv32_linux_signal_has_deliverable(parent)) {
			riscv32_linux_process_wait_unclaim(parent, child);
			riscv32_linux_process_release(child);
			riscv32_linux_return_errno(state, RISCV32_LINUX_EINTR);
			return TRUE;
		}
		expected = 0U;
		(void)riscv32_linux_wait_block_sequence(parent,
		    (event_t)&child->pmap_release_sequence,
		    &child->pmap_release_sequence, expected);
	}
	while (event == RISCV32_LINUX_WAIT_EVENT_EXITED &&
	    __sync_fetch_and_add(&child->xnu_release_sequence, 0U) == 0U) {
		if (riscv32_linux_signal_has_deliverable(parent)) {
			riscv32_linux_process_wait_unclaim(parent, child);
			riscv32_linux_process_release(child);
			riscv32_linux_return_errno(state, RISCV32_LINUX_EINTR);
			return TRUE;
		}
		expected = 0U;
		(void)riscv32_linux_wait_block_sequence(parent,
		    (event_t)&child->xnu_release_sequence,
		    &child->xnu_release_sequence, expected);
	}
	status = event == RISCV32_LINUX_WAIT_EVENT_STOPPED ?
	    child->stop_signal : (event == RISCV32_LINUX_WAIT_EVENT_CONTINUED ?
	    RISCV32_LINUX_SIGCONT : child->exit_status);
	pid = child->pid;
	gate14_child = (child->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE14_CHILD) != 0U;
	child_gate14_cycle = child->gate14_cycle_index;
	child_gate14_stage = __sync_fetch_and_add(
	    &child->gate14_lifecycle_stage, 0U);
	child_last_cpu = __sync_fetch_and_add(&child->last_user_cpu, 0U);
	child_entry_count = __sync_fetch_and_add(&child->user_entry_count, 0U);
	if (user_status != 0U &&
	    !riscv32_linux_copyout_u32(user_status,
	    riscv32_linux_process_wait_status(child, event))) {
		riscv32_linux_process_wait_unclaim(parent, child);
		riscv32_linux_process_release(child);
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	if (user_usage != 0U &&
	    !riscv32_linux_copyout_zero(user_usage,
	    RISCV32_LINUX_RUSAGE_SIZE)) {
		riscv32_linux_process_wait_unclaim(parent, child);
		riscv32_linux_process_release(child);
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	if (event != RISCV32_LINUX_WAIT_EVENT_EXITED) {
		if (!riscv32_linux_process_finish_wait_event(parent, child,
		    event)) {
			riscv32_linux_process_wait_unclaim(parent, child);
			riscv32_linux_process_release(child);
			riscv32_linux_return_errno(state, RISCV32_LINUX_ECHILD);
			return TRUE;
		}
		riscv32_linux_process_release(child);
		riscv32_linux_log_gate13_wait_event(parent, pid, event, "wait4");
		riscv32_linux_return_value(state, pid);
		return TRUE;
	}
	riscv32_linux_exec_release_process_resources(child);
	child_pmap_release = __sync_fetch_and_add(
	    &child->pmap_release_sequence, 0U);
	child_xnu_release = __sync_fetch_and_add(
	    &child->xnu_release_sequence, 0U);
	if (!riscv32_linux_process_finish_reap(parent, child, &status)) {
		riscv32_linux_process_wait_unclaim(parent, child);
		riscv32_linux_process_release(child);
		riscv32_linux_return_errno(state, RISCV32_LINUX_ECHILD);
		return TRUE;
	}
	riscv32_linux_process_release(child);
	if (gate14_child) {
		if (get_cpu_number() != 0 || parent->affinity_cpu != 0U ||
		    child_last_cpu != 1U || child_entry_count == 0U ||
		    child_pmap_release != 1U || child_xnu_release != 1U ||
		    status != 0U ||
		    !riscv32_linux_process_current_cpu_allowed(parent)) {
			panic("riscv32: Gate 14 cross-CPU wait4 invariant cpu=%d child-cpu=%u entries=%u pmap=%u xnu=%u status=%u",
			    get_cpu_number(), child_last_cpu, child_entry_count,
			    child_pmap_release, child_xnu_release, status);
		}
		riscv32_linux_gate14_note_reap(child_gate14_cycle,
		    child_gate14_stage, child_last_cpu, child_entry_count,
		    child_pmap_release, status);
		if (riscv32_linux_gate14_should_log_cycle(child_gate14_cycle)) {
			if ((parent->flags &
			    RISCV32_LINUX_PROCESS_FLAG_GATE14_DRIVER) != 0U) {
				riscv32_early_puts("[process-gate-14] driver pid=");
				riscv32_early_putdec(parent->pid);
				riscv32_early_puts(" cpu=0 wait4 reaped child=");
			} else {
				riscv32_early_puts("[process-gate-14] Bash pid=2 cpu=0 wait4 reaped child=");
			}
			riscv32_early_putdec(pid);
			riscv32_early_puts(" child-last-cpu=1 pmap-release=1 status=0 PASS\n");
		}
	}
	if (verbose_wait) {
		riscv32_early_puts("[linux-wait] parent=");
		riscv32_early_putdec(parent->pid);
		riscv32_early_puts(" woke and reaped child=");
		riscv32_early_putdec(pid);
		riscv32_early_puts(" status=");
		riscv32_early_putdec(status);
		riscv32_early_puts(" PASS\n");
	}
	riscv32_linux_return_value(state, pid);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_wait4(riscv_saved_state_t *state)
{
	uint32_t requested = state->thread.x[RISCV_X_A0];
	uint32_t user_status = state->thread.x[RISCV_X_A1];
	uint32_t options = state->thread.x[RISCV_X_A2];
	uint32_t user_usage = state->thread.x[RISCV_X_A3];
	riscv32_linux_process_t *parent = riscv32_linux_process_current();
	riscv32_linux_wait_selector_t selector;
	uint32_t id;

	if ((options & ~(RISCV32_LINUX_WNOHANG | RISCV32_LINUX_WSTOPPED |
	    RISCV32_LINUX_WCONTINUED)) != 0U ||
	    parent == (riscv32_linux_process_t *)0 ||
	    !riscv32_linux_wait4_selector(parent, requested, &selector, &id)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	return riscv32_linux_sys_wait4_real(state, parent, selector, id,
	    user_status, options, user_usage);
}

static boolean_t
riscv32_linux_sys_waitid(riscv_saved_state_t *state)
{
	uint32_t idtype = state->thread.x[RISCV_X_A0];
	uint32_t id = state->thread.x[RISCV_X_A1];
	uint32_t user_info = state->thread.x[RISCV_X_A2];
	uint32_t options = state->thread.x[RISCV_X_A3];
	uint32_t user_usage = state->thread.x[RISCV_X_A4];
	uint32_t supported = RISCV32_LINUX_WNOHANG |
	    RISCV32_LINUX_WSTOPPED | RISCV32_LINUX_WEXITED |
	    RISCV32_LINUX_WCONTINUED | RISCV32_LINUX_WNOWAIT;
	riscv32_linux_process_t *parent = riscv32_linux_process_current();
	riscv32_linux_process_t *child;
	riscv32_linux_wait_selector_t selector;
	riscv32_linux_wait_event_t event;
	boolean_t has_child;
	uint32_t expected;
	uint32_t event_mask = 0U;
	uint32_t status;
	uint32_t pid;
	uint32_t child_last_cpu;
	uint32_t child_entry_count;
	uint32_t child_pmap_release;
	uint32_t child_xnu_release;
	uint32_t child_gate14_cycle;
	uint32_t child_gate14_stage;
	boolean_t announced = FALSE;
	boolean_t gate14_child;
	boolean_t verbose_wait = TRUE;
	static volatile uint32_t gate11_options_logged;

	if (parent == (riscv32_linux_process_t *)0 ||
	    !riscv32_linux_waitid_selector(parent, idtype, id, &selector, &id) ||
	    (options & ~supported) != 0U ||
	    (options & (RISCV32_LINUX_WSTOPPED | RISCV32_LINUX_WEXITED |
	    RISCV32_LINUX_WCONTINUED)) == 0U) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (riscv32_linux_gate14_stress_enabled() &&
	    (parent->pid == 2U || (parent->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE14_DRIVER) != 0U)) {
		verbose_wait = riscv32_linux_gate14_should_log_cycle(
		    parent->child_create_count);
	}
	if (riscv32_boot_arg_has("process-gate-11") && parent->pid == 2U &&
	    __sync_add_and_fetch(&gate11_options_logged, 1U) == 1U) {
		riscv32_early_puts("[process-gate-11] waitid job-control options=0x");
		riscv32_early_puthex32(options);
		riscv32_early_puts(" accepted PASS\n");
	}
	if ((options & RISCV32_LINUX_WEXITED) != 0U) {
		event_mask |= RISCV32_LINUX_WAIT_EVENT_MASK_EXITED;
	}
	if ((options & RISCV32_LINUX_WSTOPPED) != 0U) {
		event_mask |= RISCV32_LINUX_WAIT_EVENT_MASK_STOPPED;
	}
	if ((options & RISCV32_LINUX_WCONTINUED) != 0U) {
		event_mask |= RISCV32_LINUX_WAIT_EVENT_MASK_CONTINUED;
	}
	for (;;) {
		if (riscv32_linux_signal_has_deliverable(parent)) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EINTR);
			return TRUE;
		}
		if (!riscv32_linux_process_wait_select_event(parent, selector, id,
		    event_mask, TRUE, &child, &event, &has_child, &expected)) {
			panic("riscv32: invalid waitid selector");
		}
		if (child != (riscv32_linux_process_t *)0) {
			break;
		}
		if (!has_child) {
			return riscv32_linux_sys_wait_no_children(state);
		}
		if ((options & RISCV32_LINUX_WNOHANG) != 0U) {
			if (!riscv32_linux_copyout_process_siginfo(user_info,
			    (const riscv32_linux_process_t *)0,
			    RISCV32_LINUX_WAIT_EVENT_NONE)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
			riscv32_linux_process_note_wait_nohang();
			riscv32_linux_return_value(state, 0U);
			return TRUE;
		}
		if (!announced) {
			if (verbose_wait) {
				riscv32_early_puts("[linux-waitid] parent=");
				riscv32_early_putdec(parent->pid);
				riscv32_early_puts(" blocking selector=");
				riscv32_early_putdec((uint32_t)selector);
				riscv32_early_puts(" id=");
				riscv32_early_putdec(id);
				riscv32_early_puts(" without polling\n");
			}
			announced = TRUE;
		}
		(void)riscv32_linux_wait_block_sequence(parent,
		    parent->wait_channel, &parent->wait_sequence, expected);
	}
	pid = child->pid;
	gate14_child = (child->flags &
	    RISCV32_LINUX_PROCESS_FLAG_GATE14_CHILD) != 0U;
	child_gate14_cycle = child->gate14_cycle_index;
	child_gate14_stage = __sync_fetch_and_add(
	    &child->gate14_lifecycle_stage, 0U);
	child_last_cpu = __sync_fetch_and_add(&child->last_user_cpu, 0U);
	child_entry_count = __sync_fetch_and_add(&child->user_entry_count, 0U);
	if (!riscv32_linux_copyout_process_siginfo(user_info, child, event) ||
	    (user_usage != 0U && !riscv32_linux_copyout_zero(user_usage,
	    RISCV32_LINUX_RUSAGE_SIZE))) {
		riscv32_linux_process_wait_unclaim(parent, child);
		riscv32_linux_process_release(child);
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	if ((options & RISCV32_LINUX_WNOWAIT) != 0U) {
		riscv32_linux_process_note_waitid_observation();
		riscv32_linux_process_wait_unclaim(parent, child);
		riscv32_linux_process_release(child);
		riscv32_linux_return_value(state, 0U);
		return TRUE;
	}
	if (event != RISCV32_LINUX_WAIT_EVENT_EXITED) {
		if (!riscv32_linux_process_finish_wait_event(parent, child,
		    event)) {
			riscv32_linux_process_wait_unclaim(parent, child);
			riscv32_linux_process_release(child);
			return riscv32_linux_sys_wait_no_children(state);
		}
		riscv32_linux_process_release(child);
		riscv32_linux_log_gate13_wait_event(parent, pid, event, "waitid");
		riscv32_linux_return_value(state, 0U);
		return TRUE;
	}
	while (__sync_fetch_and_add(&child->pmap_release_sequence, 0U) == 0U) {
		if (riscv32_linux_signal_has_deliverable(parent)) {
			riscv32_linux_process_wait_unclaim(parent, child);
			riscv32_linux_process_release(child);
			riscv32_linux_return_errno(state, RISCV32_LINUX_EINTR);
			return TRUE;
		}
		expected = 0U;
		(void)riscv32_linux_wait_block_sequence(parent,
		    (event_t)&child->pmap_release_sequence,
		    &child->pmap_release_sequence, expected);
	}
	while (__sync_fetch_and_add(&child->xnu_release_sequence, 0U) == 0U) {
		if (riscv32_linux_signal_has_deliverable(parent)) {
			riscv32_linux_process_wait_unclaim(parent, child);
			riscv32_linux_process_release(child);
			riscv32_linux_return_errno(state, RISCV32_LINUX_EINTR);
			return TRUE;
		}
		expected = 0U;
		(void)riscv32_linux_wait_block_sequence(parent,
		    (event_t)&child->xnu_release_sequence,
		    &child->xnu_release_sequence, expected);
	}
	riscv32_linux_exec_release_process_resources(child);
	child_pmap_release = __sync_fetch_and_add(
	    &child->pmap_release_sequence, 0U);
	child_xnu_release = __sync_fetch_and_add(
	    &child->xnu_release_sequence, 0U);
	if (!riscv32_linux_process_finish_reap(parent, child, &status)) {
		riscv32_linux_process_wait_unclaim(parent, child);
		riscv32_linux_process_release(child);
		return riscv32_linux_sys_wait_no_children(state);
	}
	riscv32_linux_process_release(child);
	if (gate14_child) {
		if (get_cpu_number() != 0 || parent->affinity_cpu != 0U ||
		    child_last_cpu != 1U || child_entry_count == 0U ||
		    child_pmap_release != 1U || child_xnu_release != 1U ||
		    status != 0U ||
		    !riscv32_linux_process_current_cpu_allowed(parent)) {
			panic("riscv32: Gate 14 cross-CPU wait invariant cpu=%d child-cpu=%u entries=%u pmap=%u xnu=%u status=%u",
			    get_cpu_number(), child_last_cpu, child_entry_count,
			    child_pmap_release, child_xnu_release, status);
		}
		riscv32_linux_gate14_note_reap(child_gate14_cycle,
		    child_gate14_stage, child_last_cpu, child_entry_count,
		    child_pmap_release, status);
		if (riscv32_linux_gate14_should_log_cycle(child_gate14_cycle)) {
			if ((parent->flags &
			    RISCV32_LINUX_PROCESS_FLAG_GATE14_DRIVER) != 0U) {
				riscv32_early_puts("[process-gate-14] driver pid=");
				riscv32_early_putdec(parent->pid);
				riscv32_early_puts(" cpu=0 woke/reaped child=");
			} else {
				riscv32_early_puts("[process-gate-14] Bash pid=2 cpu=0 woke/reaped child=");
			}
			riscv32_early_putdec(pid);
			riscv32_early_puts(" child-last-cpu=1 pmap-release=1 status=0 PASS\n");
		}
	}
	if ((parent->flags & RISCV32_LINUX_PROCESS_FLAG_GATE7_PARENT) != 0U) {
		riscv32_early_puts("[linux-waitid] parent=");
		riscv32_early_putdec(parent->pid);
		riscv32_early_puts(" reaped child=");
		riscv32_early_putdec(pid);
		riscv32_early_puts(" status=");
		riscv32_early_putdec(status);
		riscv32_early_puts(" PASS\n");
	}
	riscv32_linux_return_value(state, 0U);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_getrusage(riscv_saved_state_t *state)
{
	uint32_t user_usage = state->thread.x[RISCV_X_A1];

	if (user_usage != 0 &&
	    !riscv32_linux_copyout_zero(user_usage, RISCV32_LINUX_RUSAGE_SIZE)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_times(riscv_saved_state_t *state)
{
	uint32_t user_tms = state->thread.x[RISCV_X_A0];
	uint64_t secs;
	uint64_t nsecs;
	uint64_t ticks64;
	uint32_t ticks;

	riscv32_linux_time_now(&secs, &nsecs);
	ticks64 = secs * RISCV32_LINUX_CLK_TCK +
	    nsecs / (RISCV32_LINUX_NSEC_PER_SEC / RISCV32_LINUX_CLK_TCK);
	ticks = ticks64 > 0xffffffffULL ? 0xffffffffU : (uint32_t)ticks64;
	if (user_tms != 0 && !riscv32_linux_copyout_zero(user_tms, 16U)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, ticks);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_prctl(riscv_saved_state_t *state)
{
	uint32_t option = state->thread.x[RISCV_X_A0];
	uint32_t arg2 = state->thread.x[RISCV_X_A1];
	uint32_t i;
	char c;

	switch (option) {
	case RISCV32_LINUX_PR_GET_DUMPABLE:
		riscv32_linux_return_value(state, 1);
		return TRUE;
	case RISCV32_LINUX_PR_SET_DUMPABLE:
		riscv32_linux_return_value(state, 0);
		return TRUE;
	case RISCV32_LINUX_PR_GET_NAME:
		if (riscv32_copyout(riscv32_linux_task_name, arg2,
		    RISCV32_LINUX_TASK_NAME_LEN) != 0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	case RISCV32_LINUX_PR_SET_NAME:
		for (i = 0; i < RISCV32_LINUX_TASK_NAME_LEN; i++) {
			riscv32_linux_task_name[i] = '\0';
		}
		for (i = 0; i + 1U < RISCV32_LINUX_TASK_NAME_LEN; i++) {
			if (riscv32_copyin(arg2 + i, &c, 1) != 0) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
			riscv32_linux_task_name[i] = c;
			if (c == '\0') {
				break;
			}
		}
		riscv32_linux_return_value(state, 0);
		return TRUE;
	default:
		break;
	}

	riscv32_linux_return_errno(state, RISCV32_LINUX_ENOSYS);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_getcpu(riscv_saved_state_t *state)
{
	uint32_t user_cpu = state->thread.x[RISCV_X_A0];
	uint32_t user_node = state->thread.x[RISCV_X_A1];
	uint32_t cpu = (uint32_t)get_cpu_number();
	uint32_t zero = 0;
	riscv32_linux_process_t *process = riscv32_linux_process_current();

	if (user_cpu != 0 && riscv32_copyout(&cpu, user_cpu, sizeof(cpu)) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	if (user_node != 0 &&
	    riscv32_copyout(&zero, user_node, sizeof(zero)) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	if (process != (riscv32_linux_process_t *)0 &&
	    (process->flags & RISCV32_LINUX_PROCESS_FLAG_GATE14_CHILD) != 0U) {
		if (cpu != 1U || process->affinity_cpu != 1U ||
		    !riscv32_linux_process_current_cpu_allowed(process)) {
			panic("riscv32: Gate 14 getcpu affinity mismatch");
		}
		if (riscv32_linux_gate14_note_getcpu(process) &&
		    riscv32_linux_gate14_should_log_process(process)) {
			riscv32_early_puts("[process-gate-14] getcpu pid=");
			riscv32_early_putdec(process->pid);
			riscv32_early_puts(" cpu=1 node=0 affinity=1 PASS\n");
		}
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_sigaltstack(riscv_saved_state_t *state)
{
	uint32_t user_new = state->thread.x[RISCV_X_A0];
	uint32_t user_old = state->thread.x[RISCV_X_A1];
	riscv32_linux_process_t *process = riscv32_linux_process_current();
	uint8_t old_stack[RISCV32_LINUX_SIGALTSTACK_SIZE];
	uint8_t new_stack[RISCV32_LINUX_SIGALTSTACK_SIZE];

	if (process == (riscv32_linux_process_t *)0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	simple_lock(&process->lock);
	memcpy(old_stack, process->personality.sigaltstack, sizeof(old_stack));
	simple_unlock(&process->lock);
	if (user_old != 0 &&
	    riscv32_copyout(old_stack, user_old,
	    RISCV32_LINUX_SIGALTSTACK_SIZE) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	if (user_new != 0) {
		if (riscv32_copyin(user_new, new_stack,
		    RISCV32_LINUX_SIGALTSTACK_SIZE) != 0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		simple_lock(&process->lock);
		memcpy(process->personality.sigaltstack, new_stack,
		    sizeof(new_stack));
		simple_unlock(&process->lock);
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_rt_sigaction(riscv_saved_state_t *state)
{
	uint32_t sig = state->thread.x[RISCV_X_A0];
	uint32_t user_act = state->thread.x[RISCV_X_A1];
	uint32_t user_oldact = state->thread.x[RISCV_X_A2];
	uint32_t sigset_size = state->thread.x[RISCV_X_A3];
	uint32_t size;
	riscv32_linux_process_t *process = riscv32_linux_process_current();
	uint8_t old_action[RISCV32_LINUX_SIGACTION_MAX_SIZE];
	uint8_t new_action[RISCV32_LINUX_SIGACTION_MAX_SIZE];
	uint32_t old_installed;

	if (sig == 0 || sig > RISCV32_LINUX_SIG_MAX ||
	    sig == 9U || sig == 19U ||
	    sigset_size > RISCV32_LINUX_SIGSET_MAX_SIZE ||
	    process == (riscv32_linux_process_t *)0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	size = RISCV32_LINUX_SIGACTION_HEAD_SIZE + sigset_size;
	simple_lock(&process->lock);
	old_installed = process->personality.sigactions[sig].installed;
	memcpy(old_action, process->personality.sigactions[sig].bytes,
	    sizeof(old_action));
	simple_unlock(&process->lock);
	if (user_oldact != 0) {
		if (old_installed == 0) {
			if (!riscv32_linux_copyout_zero(user_oldact, size)) {
				riscv32_linux_return_errno(state,
				    RISCV32_LINUX_EFAULT);
				return TRUE;
			}
		} else if (riscv32_copyout(old_action, user_oldact, size) != 0) {
			riscv32_linux_return_errno(state,
			    RISCV32_LINUX_EFAULT);
			return TRUE;
		}
	}
	if (user_act != 0) {
		riscv32_linux_zero(new_action, sizeof(new_action));
		if (riscv32_copyin(user_act, new_action, size) != 0) {
			riscv32_linux_return_errno(state,
			    RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		simple_lock(&process->lock);
		memcpy(process->personality.sigactions[sig].bytes, new_action,
		    sizeof(new_action));
		process->personality.sigactions[sig].installed = 1U;
		simple_unlock(&process->lock);
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_rt_sigprocmask(riscv_saved_state_t *state)
{
	uint32_t how = state->thread.x[RISCV_X_A0];
	uint32_t user_set = state->thread.x[RISCV_X_A1];
	uint32_t user_oldset = state->thread.x[RISCV_X_A2];
	uint32_t sigset_size = state->thread.x[RISCV_X_A3];
	uint8_t newset[RISCV32_LINUX_SIGSET_MAX_SIZE];
	uint8_t oldset[RISCV32_LINUX_SIGSET_MAX_SIZE];
	riscv32_linux_process_t *process = riscv32_linux_process_current();
	uint32_t i;

	if (sigset_size > RISCV32_LINUX_SIGSET_MAX_SIZE ||
	    process == (riscv32_linux_process_t *)0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	simple_lock(&process->lock);
	memcpy(oldset, process->personality.signal_mask, sizeof(oldset));
	simple_unlock(&process->lock);
	if (user_oldset != 0 &&
	    riscv32_copyout(oldset, user_oldset,
	    sigset_size) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	if (user_set == 0) {
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (how != RISCV32_LINUX_SIG_BLOCK &&
	    how != RISCV32_LINUX_SIG_UNBLOCK &&
	    how != RISCV32_LINUX_SIG_SETMASK) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	riscv32_linux_zero(newset, sizeof(newset));
	if (riscv32_copyin(user_set, newset, sigset_size) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	simple_lock(&process->lock);
	for (i = 0; i < sigset_size; i++) {
		switch (how) {
		case RISCV32_LINUX_SIG_BLOCK:
			process->personality.signal_mask[i] |= newset[i];
			break;
		case RISCV32_LINUX_SIG_UNBLOCK:
			process->personality.signal_mask[i] &= (uint8_t)~newset[i];
			break;
		case RISCV32_LINUX_SIG_SETMASK:
			process->personality.signal_mask[i] = newset[i];
			break;
		default:
			break;
		}
	}
	process->personality.signal_mask[(9U - 1U) / 8U] &=
	    (uint8_t)~(1U << ((9U - 1U) % 8U));
	process->personality.signal_mask[(19U - 1U) / 8U] &=
	    (uint8_t)~(1U << ((19U - 1U) % 8U));
	simple_unlock(&process->lock);
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_rt_sigpending(riscv_saved_state_t *state)
{
	uint32_t user_set = state->thread.x[RISCV_X_A0];
	uint32_t sigset_size = state->thread.x[RISCV_X_A1];
	riscv32_linux_process_t *process = riscv32_linux_process_current();
	uint8_t pending[RISCV32_LINUX_SIGSET_MAX_SIZE];
	uint32_t i;

	if (sigset_size > RISCV32_LINUX_SIGSET_MAX_SIZE ||
	    process == (riscv32_linux_process_t *)0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	riscv32_linux_zero(pending, sizeof(pending));
	simple_lock(&process->lock);
	for (i = 0U; i < sigset_size; i++) {
		pending[i] = process->personality.pending_signals[i] &
		    process->personality.signal_mask[i];
	}
	simple_unlock(&process->lock);
	if (riscv32_copyout(pending, user_set, sigset_size) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_signal_send(riscv_saved_state_t *state, uint32_t pid,
    uint32_t sig, boolean_t process_semantics)
{
	riscv32_linux_process_t *sender = riscv32_linux_process_current();
	riscv32_linux_process_t *target;
	riscv32_linux_process_t *targets[RISCV32_LINUX_PROCESS_MAX];
	int32_t signed_pid = (int32_t)pid;
	uint32_t sender_pid;
	uint32_t sender_uid;
	uint32_t sender_pgid;
	uint32_t sender_sid;
	uint32_t pgid;
	uint32_t count;
	uint32_t delivered;
	uint32_t i;

	if (sig > RISCV32_LINUX_SIG_MAX) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EINVAL);
		return TRUE;
	}
	if (sender == (riscv32_linux_process_t *)0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ESRCH);
		return TRUE;
	}
	simple_lock(&sender->lock);
	sender_pid = sender->pid;
	sender_uid = sender->personality.uid;
	sender_pgid = sender->personality.pgid;
	sender_sid = sender->personality.sid;
	simple_unlock(&sender->lock);

	if (process_semantics && signed_pid <= 0) {
		if (signed_pid == -1 || pid == 0x80000000U) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_ENOSYS);
			return TRUE;
		}
		pgid = signed_pid == 0 ? sender_pgid :
		    (uint32_t)(-signed_pid);
		count = riscv32_linux_process_collect_group(pgid, sender_sid,
		    targets, RISCV32_LINUX_PROCESS_MAX);
		if (count == 0U) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_ESRCH);
			return TRUE;
		}
		delivered = 0U;
		for (i = 0U; i < count; i++) {
			if (sig == 0U || riscv32_linux_signal_queue(targets[i], sig,
			    sender_pid, sender_uid, 0U, 0U)) {
				delivered++;
			}
			riscv32_linux_process_release(targets[i]);
		}
		if (delivered == 0U) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_ENOSYS);
			return TRUE;
		}
		if (riscv32_boot_arg_has("process-gate-13") &&
		    sig == RISCV32_LINUX_SIGCONT) {
			riscv32_early_puts(
			    "[process-gate-13] kill process-group pgid=");
			riscv32_early_putdec(pgid);
			riscv32_early_puts(" signal=18 targets=");
			riscv32_early_putdec(delivered);
			riscv32_early_puts(" PASS\n");
		}
		riscv32_linux_return_value(state, 0U);
		return TRUE;
	}
	if (!process_semantics && signed_pid <= 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ESRCH);
		return TRUE;
	}
	target = riscv32_linux_process_lookup(pid);
	if (target == (riscv32_linux_process_t *)0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ESRCH);
		return TRUE;
	}
	if (sig == 0) {
		riscv32_linux_process_release(target);
		riscv32_linux_return_value(state, 0);
		return TRUE;
	}
	if (!riscv32_linux_signal_queue(target, sig, sender_pid, sender_uid,
	    0U, 0U)) {
		riscv32_linux_process_release(target);
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOSYS);
		return TRUE;
	}
	if ((target->flags & (RISCV32_LINUX_PROCESS_FLAG_GATE9_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_GATE10_PARENT)) != 0U) {
		riscv32_early_puts("[linux-signal] queued signal=");
		riscv32_early_putdec(sig);
		riscv32_early_puts(" sender=");
		riscv32_early_putdec(sender_pid);
		riscv32_early_puts(" target=");
		riscv32_early_putdec(target->pid);
		riscv32_early_puts(" PASS\n");
	}
	riscv32_linux_process_release(target);
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_futex(riscv_saved_state_t *state)
{
	uint32_t user_uaddr = state->thread.x[RISCV_X_A0];
	uint32_t op = state->thread.x[RISCV_X_A1] & RISCV32_LINUX_FUTEX_CMD_MASK;
	uint32_t expected = state->thread.x[RISCV_X_A2];
	uint32_t current;

	switch (op) {
	case RISCV32_LINUX_FUTEX_WAKE:
		riscv32_linux_return_value(state, 0);
		return TRUE;
	case RISCV32_LINUX_FUTEX_WAIT:
		if (riscv32_copyin(user_uaddr, &current, sizeof(current)) != 0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
			return TRUE;
		}
		(void)expected;
		riscv32_linux_return_errno(state, RISCV32_LINUX_EAGAIN);
		return TRUE;
	default:
		break;
	}

	riscv32_linux_return_errno(state, RISCV32_LINUX_ENOSYS);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_get_robust_list(riscv_saved_state_t *state)
{
	uint32_t pid = state->thread.x[RISCV_X_A0];
	uint32_t user_head_ptr = state->thread.x[RISCV_X_A1];
	uint32_t user_len_ptr = state->thread.x[RISCV_X_A2];

	if (pid > 1) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_ESRCH);
		return TRUE;
	}
	if (!riscv32_linux_copyout_u32(user_head_ptr, 0) ||
	    !riscv32_linux_copyout_u32(user_len_ptr,
	    RISCV32_LINUX_ROBUST_LIST_HEAD_SIZE)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static boolean_t
riscv32_linux_sys_uname(riscv_saved_state_t *state)
{
	uint32_t user_uts = state->thread.x[RISCV_X_A0];

	if (!riscv32_user_range_valid(user_uts, RISCV32_LINUX_UTSNAME_SIZE)) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}
	if (riscv32_linux_copyout_cstr_field("Linux", user_uts) != 0 ||
	    riscv32_linux_copyout_cstr_field("caribeos",
	    user_uts + RISCV32_LINUX_UTSNAME_FIELD_LEN) != 0 ||
	    riscv32_linux_copyout_cstr_field("0.0.0-caribe",
	    user_uts + 2U * RISCV32_LINUX_UTSNAME_FIELD_LEN) != 0 ||
	    riscv32_linux_copyout_cstr_field("#1 CaribeOS RV32IMACSU",
	    user_uts + 3U * RISCV32_LINUX_UTSNAME_FIELD_LEN) != 0 ||
	    riscv32_linux_copyout_cstr_field("riscv32",
	    user_uts + 4U * RISCV32_LINUX_UTSNAME_FIELD_LEN) != 0 ||
	    riscv32_linux_copyout_cstr_field("(none)",
	    user_uts + 5U * RISCV32_LINUX_UTSNAME_FIELD_LEN) != 0) {
		riscv32_linux_return_errno(state, RISCV32_LINUX_EFAULT);
		return TRUE;
	}

	riscv32_linux_return_value(state, 0);
	return TRUE;
}

static uint32_t
riscv32_linux_copyin_string_vector(uint32_t user_vector,
    char strings[][RISCV32_LINUX_EXEC_ARG_STRLEN], uint32_t max_strings,
    uint32_t *count_out)
{
	uint32_t i;

	*count_out = 0;
	if (user_vector == 0) {
		return 0;
	}
	for (i = 0; i < max_strings; i++) {
		uint32_t user_string;
		uint32_t error;

		if (riscv32_copyin(user_vector + i * sizeof(uint32_t),
		    &user_string, sizeof(user_string)) != 0) {
			return RISCV32_LINUX_EFAULT;
		}
		if (user_string == 0) {
			return 0;
		}
		error = riscv32_linux_copyin_path(user_string, strings[i],
		    RISCV32_LINUX_EXEC_ARG_STRLEN);
		if (error != 0) {
			return error;
		}
		*count_out = i + 1U;
	}

	{
		uint32_t next_string;

		if (riscv32_copyin(user_vector + max_strings * sizeof(uint32_t),
		    &next_string, sizeof(next_string)) != 0) {
			return RISCV32_LINUX_EFAULT;
		}
		if (next_string != 0) {
			return RISCV32_LINUX_E2BIG;
		}
	}
	return 0;
}

static boolean_t
riscv32_linux_copy_range_cstr(char *dst, uint32_t dst_size,
    const uint8_t *src, uint32_t start, uint32_t end)
{
	uint32_t i = 0;

	if (dst_size == 0 || end < start) {
		return FALSE;
	}
	while (start < end && i + 1U < dst_size) {
		dst[i++] = (char)src[start++];
	}
	if (start != end) {
		return FALSE;
	}
	dst[i] = '\0';
	return TRUE;
}

static boolean_t
riscv32_linux_parse_shebang(const riscv32_linux_initrd_file_t *file,
    char *interp, uint32_t interp_size, char *interp_arg,
    uint32_t interp_arg_size)
{
	uint8_t line[RISCV32_LINUX_EXEC_ARG_STRLEN];
	uint32_t size;
	uint32_t i;
	uint32_t start;
	uint32_t end;

	if (file == (const riscv32_linux_initrd_file_t *)0 ||
	    interp == (char *)0 || interp_size == 0 ||
	    interp_arg == (char *)0 || interp_arg_size == 0) {
		return FALSE;
	}
	interp[0] = '\0';
	interp_arg[0] = '\0';
	size = file->size;
	if (size >= sizeof(line)) {
		size = sizeof(line) - 1U;
	}
	if (size < 3U ||
	    !riscv32_linux_copy_phys_to_kernel(file->data_paddr, line, size)) {
		return FALSE;
	}
	line[size] = '\0';
	if (line[0] != '#' || line[1] != '!') {
		return FALSE;
	}
	i = 2U;
	while (i < size && (line[i] == ' ' || line[i] == '\t')) {
		i++;
	}
	start = i;
	while (i < size && line[i] != ' ' && line[i] != '\t' &&
	    line[i] != '\r' && line[i] != '\n' && line[i] != '\0') {
		i++;
	}
	end = i;
	if (end == start ||
	    !riscv32_linux_copy_range_cstr(interp, interp_size, line,
	    start, end)) {
		return FALSE;
	}
	while (i < size && (line[i] == ' ' || line[i] == '\t')) {
		i++;
	}
	start = i;
	while (i < size && line[i] != '\r' && line[i] != '\n' &&
	    line[i] != '\0') {
		i++;
	}
	end = i;
	while (end > start && (line[end - 1U] == ' ' ||
	    line[end - 1U] == '\t')) {
		end--;
	}
	if (end > start) {
		return riscv32_linux_copy_range_cstr(interp_arg,
		    interp_arg_size, line, start, end);
	}
	return TRUE;
}

static boolean_t
riscv32_linux_exec_args_push(riscv32_linux_exec_stack_args_t *args,
    const char *value)
{
	if (args->argc >= RISCV32_LINUX_EXEC_MAX_ARGC) {
		return FALSE;
	}
	riscv32_linux_copy_cstr(args->argv[args->argc],
	    sizeof(args->argv[args->argc]), value);
	args->argc++;
	return TRUE;
}

static void
riscv32_linux_exec_args_copy_env(riscv32_linux_exec_stack_args_t *dst,
    const riscv32_linux_exec_stack_args_t *src)
{
	uint32_t i;

	dst->envc = src->envc;
	if (dst->envc > RISCV32_LINUX_EXEC_MAX_ENVC) {
		dst->envc = RISCV32_LINUX_EXEC_MAX_ENVC;
	}
	for (i = 0; i < dst->envc; i++) {
		riscv32_linux_copy_cstr(dst->envp[i],
		    sizeof(dst->envp[i]), src->envp[i]);
	}
}

static uint32_t
riscv32_linux_prepare_shebang_args(const char *script_path,
    const char *interp, const char *interp_arg,
    const riscv32_linux_exec_stack_args_t *orig,
    riscv32_linux_exec_stack_args_t *script_args)
{
	uint32_t i;

	riscv32_linux_zero(script_args, sizeof(*script_args));
	if (!riscv32_linux_exec_args_push(script_args, interp)) {
		return RISCV32_LINUX_E2BIG;
	}
	if (interp_arg[0] != '\0' &&
	    !riscv32_linux_exec_args_push(script_args, interp_arg)) {
		return RISCV32_LINUX_E2BIG;
	}
	if (!riscv32_linux_exec_args_push(script_args, script_path)) {
		return RISCV32_LINUX_E2BIG;
	}
	for (i = 1U; i < orig->argc; i++) {
		if (!riscv32_linux_exec_args_push(script_args,
		    orig->argv[i])) {
			return RISCV32_LINUX_E2BIG;
		}
	}
	riscv32_linux_exec_args_copy_env(script_args, orig);
	return 0;
}

static uint32_t
riscv32_linux_execve_resolved(riscv_saved_state_t *state, const char *path,
    uint32_t user_argv, uint32_t user_envp, boolean_t *loaded_out)
{
	riscv32_linux_exec_stack_args_t *args =
	    &riscv32_linux_execve_args_work;
	riscv32_linux_exec_stack_args_t *script_args =
	    &riscv32_linux_execve_script_args_work;
	riscv32_linux_initrd_file_t file;
	char interp[RISCV32_LINUX_EXEC_INTERP_MAX];
	char interp_arg[RISCV32_LINUX_EXEC_ARG_STRLEN];
	char interp_path[RISCV32_LINUX_PATH_MAX];
	uint32_t error;
	riscv32_linux_process_t *process;

	*loaded_out = FALSE;
	process = riscv32_linux_process_current();
	riscv32_linux_zero(args, sizeof(*args));
	error = riscv32_linux_copyin_string_vector(user_argv, args->argv,
	    RISCV32_LINUX_EXEC_MAX_ARGC, &args->argc);
	if (error != 0) {
		return error;
	}
	error = riscv32_linux_copyin_string_vector(user_envp, args->envp,
	    RISCV32_LINUX_EXEC_MAX_ENVC, &args->envc);
	if (error != 0) {
		return error;
	}
	if (riscv32_boot_arg_has("process-gate-11") &&
	    process != (riscv32_linux_process_t *)0 && process->pid >= 3U &&
	    !(riscv32_linux_gate14_stress_enabled() &&
	    (process->flags & RISCV32_LINUX_PROCESS_FLAG_GATE14_CHILD) != 0U)) {
		riscv32_early_puts(
		    "[process-gate-11] exec argv/env collected argc=");
		riscv32_early_putdec(args->argc);
		riscv32_early_puts(" envc=");
		riscv32_early_putdec(args->envc);
		riscv32_early_puts(" PASS\n");
	}
	if (!riscv32_linux_initrd_lookup(path, &file)) {
		return RISCV32_LINUX_ENOENT;
	}
	if (riscv32_linux_parse_shebang(&file, interp, sizeof(interp),
	    interp_arg, sizeof(interp_arg))) {
		riscv32_linux_resolve_path(interp, interp_path,
		    sizeof(interp_path));
		if (!riscv32_linux_initrd_lookup(interp_path, &file)) {
			return RISCV32_LINUX_ENOENT;
		}
		error = riscv32_linux_prepare_shebang_args(path, interp_path,
		    interp_arg, args, script_args);
		if (error != 0) {
			return error;
		}
		riscv32_early_puts("[linux-exec] shebang path=");
		riscv32_early_puts(path);
		riscv32_early_puts(" interp=");
		riscv32_early_puts(interp_path);
		if (interp_arg[0] != '\0') {
			riscv32_early_puts(" arg=");
			riscv32_early_puts(interp_arg);
		}
		riscv32_early_puts("\n");
		error = riscv32_linux_exec_replace_from_initrd(interp_path,
		    script_args, state);
		if (error != 0U) {
			return error;
		}
		(void)riscv32_linux_syscall_close_cloexec_current();
		*loaded_out = TRUE;
		return 0;
	}
	error = riscv32_linux_exec_replace_from_initrd(path, args, state);
	if (error != 0U) {
		return error;
	}
	(void)riscv32_linux_syscall_close_cloexec_current();
	*loaded_out = TRUE;
	return 0;
}

static boolean_t
riscv32_linux_sys_execve(riscv_saved_state_t *state)
{
	uint32_t user_path = state->thread.x[RISCV_X_A0];
	uint32_t user_argv = state->thread.x[RISCV_X_A1];
	uint32_t user_envp = state->thread.x[RISCV_X_A2];
	char raw_path[RISCV32_LINUX_PATH_MAX];
	char path[RISCV32_LINUX_PATH_MAX];
	uint32_t error;
	boolean_t loaded;
	error = riscv32_linux_copyin_path(user_path, raw_path,
	    sizeof(raw_path));
	if (error != 0) {
		riscv32_linux_return_errno(state, error);
		return TRUE;
	}
	riscv32_linux_resolve_path(raw_path, path, sizeof(path));
	error = riscv32_linux_execve_resolved(state, path, user_argv,
	    user_envp, &loaded);
	if (error != 0) {
		riscv32_linux_return_errno(state, error);
		return TRUE;
	}
	return TRUE;
}

static boolean_t
riscv32_linux_sys_execveat(riscv_saved_state_t *state)
{
	uint32_t dirfd = state->thread.x[RISCV_X_A0];
	uint32_t user_path = state->thread.x[RISCV_X_A1];
	uint32_t user_argv = state->thread.x[RISCV_X_A2];
	uint32_t user_envp = state->thread.x[RISCV_X_A3];
	uint32_t flags = state->thread.x[RISCV_X_A4];
	char raw_path[RISCV32_LINUX_PATH_MAX];
	char path[RISCV32_LINUX_PATH_MAX];
	riscv32_linux_fd_t *desc;
	uint32_t error;
	boolean_t loaded;

	error = riscv32_linux_copyin_path(user_path, raw_path,
	    sizeof(raw_path));
	if (error != 0) {
		riscv32_linux_return_errno(state, error);
		return TRUE;
	}
	if (raw_path[0] == '\0' &&
	    (flags & RISCV32_LINUX_AT_EMPTY_PATH) != 0) {
		desc = riscv32_linux_fd_ref(dirfd);
		if (desc == (riscv32_linux_fd_t *)0) {
			riscv32_linux_return_errno(state, RISCV32_LINUX_EBADF);
			return TRUE;
		}
		if (desc->kind != RISCV32_LINUX_FD_INITRD ||
		    desc->dir_path[0] == '\0') {
			riscv32_linux_return_errno(state, RISCV32_LINUX_ENOEXEC);
			return TRUE;
		}
		riscv32_linux_copy_cstr(path, sizeof(path), desc->dir_path);
		riscv32_early_puts("[linux-exec] execveat AT_EMPTY_PATH fd=");
		riscv32_early_putdec(dirfd);
		riscv32_early_puts(" path=");
		riscv32_early_puts(path);
		riscv32_early_puts("\n");
	} else {
		riscv32_linux_resolve_path_at(dirfd, raw_path, path,
		    sizeof(path));
	}
	error = riscv32_linux_execve_resolved(state, path, user_argv,
	    user_envp, &loaded);
	if (error != 0) {
		riscv32_linux_return_errno(state, error);
		return TRUE;
	}
	return TRUE;
}

void
riscv32_linux_terminate_current(uint32_t status, uint32_t signal)
{
	riscv32_linux_process_t *process = riscv32_linux_process_current();
	riscv32_linux_exit_resources_t *resources;
	task_t task;
	thread_t thread;
	pmap_t pmap;
	kern_return_t kr;
	boolean_t orphan;
	boolean_t verbose_exit;
	cpu_set active_mask;
	uint32_t exit_cpu;

	status &= 0xffU;

	if (process == (riscv32_linux_process_t *)0 ||
	    process->main_thread != current_thread() ||
	    process->task != current_task() || process->pmap == PMAP_NULL ||
	    signal > RISCV32_LINUX_PROCESS_SIG_MAX) {
		panic("riscv32: exit without attached Linux process");
	}
	verbose_exit = signal != 0U || (process->flags &
	    (RISCV32_LINUX_PROCESS_FLAG_GATE4_CHILD |
	    RISCV32_LINUX_PROCESS_FLAG_GATE6_CHILD |
	    RISCV32_LINUX_PROCESS_FLAG_GATE7_CHILD |
	    RISCV32_LINUX_PROCESS_FLAG_GATE8_CHILD |
	    RISCV32_LINUX_PROCESS_FLAG_GATE9_CHILD)) == 0U ||
	    (process->flags & RISCV32_LINUX_PROCESS_FLAG_GATE7_CHILD) != 0U ||
	    (process->flags & RISCV32_LINUX_PROCESS_FLAG_GATE8_CHILD) != 0U ||
	    (process->flags & RISCV32_LINUX_PROCESS_FLAG_GATE9_CHILD) != 0U ||
	    process->pid == 4U || (process->pid % 100U) == 3U;
	if ((process->flags & RISCV32_LINUX_PROCESS_FLAG_GATE14_CHILD) != 0U) {
		exit_cpu = (uint32_t)get_cpu_number();
		active_mask = riscv32_pmap_active_cpu_mask(process->pmap);
		if (exit_cpu != 1U || process->affinity_cpu != 1U ||
		    (active_mask & (cpu_set)(1U << 1)) == 0U ||
		    !riscv32_linux_process_current_cpu_allowed(process)) {
			panic("riscv32: Gate 14 exit affinity/pmap mismatch cpu=%u mask=0x%x",
			    exit_cpu, active_mask);
		}
		riscv32_linux_gate14_note_exit(process, status);
		if (riscv32_linux_gate14_should_log_process(process)) {
			riscv32_early_puts("[process-gate-14] child pid=");
			riscv32_early_putdec(process->pid);
			riscv32_early_puts(" exit cpu=1 active-pmap-mask=0x");
			riscv32_early_puthex32((uint32_t)active_mask);
			riscv32_early_puts(" status=");
			riscv32_early_putdec(status);
			riscv32_early_puts(" PASS\n");
		} else if (riscv32_linux_gate14_stress_enabled()) {
			verbose_exit = FALSE;
		}
	}
	if (verbose_exit) {
		if (signal != 0U) {
			riscv32_early_puts("[linux] exit signal=");
			riscv32_early_putdec(signal);
		} else {
			riscv32_early_puts("[linux] exit status=");
			riscv32_early_putdec(status);
		}
		riscv32_early_puts("\n");
	}
	orphan = process->parent == (riscv32_linux_process_t *)0;
	if (!(signal != 0U ?
	    riscv32_linux_process_begin_signal_exit(process, signal) :
	    riscv32_linux_process_begin_exit(process, status))) {
		panic("riscv32: PID %u cannot enter EXITING", process->pid);
	}
	resources = &process->exit_resources;
	riscv32_linux_syscall_teardown_process(process, resources);
	if (!riscv32_linux_process_detach_xnu(process, &task, &thread,
	    &pmap)) {
		panic("riscv32: PID %u cannot detach XNU resources", process->pid);
	}
	riscv32_pmap_arm_destroy_notification(pmap,
	    &process->pmap_release_sequence);
	if (!orphan) {
		if (process->xnu_release_sequence != 0U ||
		    thread->machine.deallocate_sequence !=
		    (uint32_t *)0) {
			panic("riscv32: PID %u duplicate XNU teardown notification",
			    process->pid);
		}
		thread->machine.deallocate_sequence =
		    &process->xnu_release_sequence;
		__sync_synchronize();
	}
	set_bsdtask_info(task, (void *)0);
	kr = task_terminate_internal(task);
	if (kr != KERN_SUCCESS) {
		panic("riscv32: PID %u task termination failed kr=%u",
		    process->pid, (uint32_t)kr);
	}
	if (!riscv32_linux_process_publish_exit(process)) {
		panic("riscv32: PID %u exit publication failed", process->pid);
	}
	if (verbose_exit) {
		riscv32_early_puts("[linux-process] pid=");
		riscv32_early_putdec(process->pid);
		riscv32_early_puts(" state=");
		riscv32_early_puts(riscv32_linux_process_state_name(process->state));
		riscv32_early_puts(" closed-fds=");
		riscv32_early_putdec(resources->closed_fds);
		riscv32_early_puts(" mmap-pages=");
		riscv32_early_putdec(resources->mmap_pages_released);
		riscv32_early_puts(" brk-pages=");
		riscv32_early_putdec(resources->brk_pages_released);
		riscv32_early_puts(" pmap-pages=");
		riscv32_early_putdec(resources->pmap_owned_pt_pages);
		riscv32_early_puts(" backing-pages=");
		riscv32_early_putdec(resources->user_backing_pages);
		riscv32_early_puts("\n");
	}

	/* Drop the creator references.  Active thread/task references remain. */
	thread_deallocate(thread);
	task_deallocate(task);
	if (orphan) {
		riscv32_linux_process_release_lifecycle(process);
	}
	thread_terminate_self();
	panic("riscv32: thread_terminate_self returned for PID %u",
	    process->pid);
	__builtin_unreachable();
}

static void __attribute__((noreturn))
riscv32_linux_sys_exit(riscv_saved_state_t *state)
{
	riscv32_linux_terminate_current(
	    state->thread.x[RISCV_X_A0] & 0xffU, 0U);
}

boolean_t
riscv32_linux_syscall_dispatch(riscv_saved_state_t *state,
    uint32_t syscall_number)
{
	if (state == (riscv_saved_state_t *)0) {
		return FALSE;
	}

	switch (syscall_number) {
	case RISCV32_LINUX_SYS_GETCWD:
		return riscv32_linux_sys_getcwd(state);
	case RISCV32_LINUX_SYS_EVENTFD2:
		return riscv32_linux_sys_eventfd2(state);
	case RISCV32_LINUX_SYS_EPOLL_CREATE1:
		return riscv32_linux_sys_epoll_create1(state);
	case RISCV32_LINUX_SYS_EPOLL_CTL:
		return riscv32_linux_sys_epoll_ctl(state);
	case RISCV32_LINUX_SYS_EPOLL_PWAIT:
		return riscv32_linux_sys_epoll_pwait(state, FALSE);
	case RISCV32_LINUX_SYS_DUP:
		return riscv32_linux_sys_dup(state);
	case RISCV32_LINUX_SYS_DUP3:
		return riscv32_linux_sys_dup3(state);
	case RISCV32_LINUX_SYS_FCNTL:
		return riscv32_linux_sys_fcntl(state);
	case RISCV32_LINUX_SYS_INOTIFY_INIT1:
		return riscv32_linux_sys_inotify_init1(state);
	case RISCV32_LINUX_SYS_INOTIFY_ADD_WATCH:
		return riscv32_linux_sys_inotify_add_watch(state);
	case RISCV32_LINUX_SYS_INOTIFY_RM_WATCH:
		return riscv32_linux_sys_inotify_rm_watch(state);
	case RISCV32_LINUX_SYS_IOCTL:
		return riscv32_linux_sys_ioctl(state);
	case RISCV32_LINUX_SYS_MKNODAT:
	case RISCV32_LINUX_SYS_MKDIRAT:
		return riscv32_linux_sys_readonly_create_path(state,
		    state->thread.x[RISCV_X_A1]);
	case RISCV32_LINUX_SYS_UNLINKAT:
		return riscv32_linux_sys_readonly_existing_path(state,
		    state->thread.x[RISCV_X_A1]);
	case RISCV32_LINUX_SYS_SYMLINKAT:
		return riscv32_linux_sys_symlinkat(state);
	case RISCV32_LINUX_SYS_LINKAT:
		return riscv32_linux_sys_linkat(state);
	case RISCV32_LINUX_SYS_RENAMEAT:
		return riscv32_linux_sys_renameat(state);
	case RISCV32_LINUX_SYS_STATFS64:
		return riscv32_linux_sys_statfs64(state);
	case RISCV32_LINUX_SYS_FSTATFS64:
		return riscv32_linux_sys_fstatfs64(state);
	case RISCV32_LINUX_SYS_FTRUNCATE64:
		return riscv32_linux_sys_fd_noop(state);
	case RISCV32_LINUX_SYS_FACCESSAT:
		return riscv32_linux_sys_faccessat(state);
	case RISCV32_LINUX_SYS_CHDIR:
		return riscv32_linux_sys_chdir(state);
	case RISCV32_LINUX_SYS_FCHDIR:
		return riscv32_linux_sys_fchdir(state);
	case RISCV32_LINUX_SYS_FCHMODAT:
	case RISCV32_LINUX_SYS_FCHOWNAT:
		return riscv32_linux_sys_readonly_existing_path(state,
		    state->thread.x[RISCV_X_A1]);
	case RISCV32_LINUX_SYS_OPENAT:
		return riscv32_linux_sys_openat(state);
	case RISCV32_LINUX_SYS_CLOSE:
		return riscv32_linux_sys_close(state);
	case RISCV32_LINUX_SYS_PIPE2:
		return riscv32_linux_sys_pipe2(state);
	case RISCV32_LINUX_SYS_GETDENTS64:
		return riscv32_linux_sys_getdents64(state);
	case RISCV32_LINUX_SYS_LLSEEK:
		return riscv32_linux_sys_llseek(state);
	case RISCV32_LINUX_SYS_READ:
		return riscv32_linux_sys_read(state);
	case RISCV32_LINUX_SYS_WRITE:
		return riscv32_linux_sys_write(state);
	case RISCV32_LINUX_SYS_READV:
		return riscv32_linux_sys_readv(state);
	case RISCV32_LINUX_SYS_WRITEV:
		return riscv32_linux_sys_writev(state);
	case RISCV32_LINUX_SYS_PREAD64:
		return riscv32_linux_sys_pread64(state);
	case RISCV32_LINUX_SYS_PSELECT6:
		return riscv32_linux_sys_pselect6(state, FALSE);
	case RISCV32_LINUX_SYS_PPOLL:
		return riscv32_linux_sys_ppoll(state, FALSE);
	case RISCV32_LINUX_SYS_READLINKAT:
		return riscv32_linux_sys_readlinkat(state);
	case RISCV32_LINUX_SYS_FSTATAT64:
		return riscv32_linux_sys_fstatat64(state);
	case RISCV32_LINUX_SYS_FSTAT64:
		return riscv32_linux_sys_fstat64(state);
	case RISCV32_LINUX_SYS_SYNC:
		riscv32_linux_return_value(state, 0);
		return TRUE;
	case RISCV32_LINUX_SYS_FSYNC:
	case RISCV32_LINUX_SYS_FDATASYNC:
		return riscv32_linux_sys_fd_noop(state);
	case RISCV32_LINUX_SYS_TIMERFD_CREATE:
		return riscv32_linux_sys_timerfd_create(state);
	case RISCV32_LINUX_SYS_TIMERFD_SETTIME:
		return riscv32_linux_sys_timerfd_settime(state, FALSE);
	case RISCV32_LINUX_SYS_TIMERFD_GETTIME:
		return riscv32_linux_sys_timerfd_gettime(state, FALSE);
	case RISCV32_LINUX_SYS_UTIMENSAT:
		return riscv32_linux_sys_utimensat(state, FALSE);
	case RISCV32_LINUX_SYS_CAPGET:
		return riscv32_linux_sys_capget(state);
	case RISCV32_LINUX_SYS_CAPSET:
		return riscv32_linux_sys_capset(state);
	case RISCV32_LINUX_SYS_PERSONALITY:
		return riscv32_linux_sys_personality(state);
	case RISCV32_LINUX_SYS_EXIT:
	case RISCV32_LINUX_SYS_EXIT_GROUP:
		riscv32_linux_sys_exit(state);
		return TRUE;
	case RISCV32_LINUX_SYS_WAITID:
		return riscv32_linux_sys_waitid(state);
	case RISCV32_LINUX_SYS_SET_TID_ADDRESS:
		riscv32_linux_return_value(state, riscv32_linux_current_pid());
		return TRUE;
	case RISCV32_LINUX_SYS_FUTEX:
		return riscv32_linux_sys_futex(state);
	case RISCV32_LINUX_SYS_SET_ROBUST_LIST:
		riscv32_linux_return_value(state, 0);
		return TRUE;
	case RISCV32_LINUX_SYS_GET_ROBUST_LIST:
		return riscv32_linux_sys_get_robust_list(state);
	case RISCV32_LINUX_SYS_NANOSLEEP:
		return riscv32_linux_sys_nanosleep(state, FALSE);
	case RISCV32_LINUX_SYS_GETITIMER:
		return riscv32_linux_sys_getitimer(state);
	case RISCV32_LINUX_SYS_SETITIMER:
		return riscv32_linux_sys_setitimer(state);
	case RISCV32_LINUX_SYS_CLOCK_GETTIME:
		return riscv32_linux_sys_clock_gettime(state, FALSE);
	case RISCV32_LINUX_SYS_CLOCK_GETRES:
		return riscv32_linux_sys_clock_getres(state, FALSE);
	case RISCV32_LINUX_SYS_CLOCK_NANOSLEEP:
		return riscv32_linux_sys_clock_nanosleep(state, FALSE);
	case RISCV32_LINUX_SYS_SCHED_SETAFFINITY:
		return riscv32_linux_sys_sched_setaffinity(state);
	case RISCV32_LINUX_SYS_SCHED_GETAFFINITY:
		return riscv32_linux_sys_sched_getaffinity(state);
	case RISCV32_LINUX_SYS_SCHED_YIELD:
		thread_block_reason(THREAD_CONTINUE_NULL, NULL, AST_YIELD);
		riscv32_linux_return_value(state, 0);
		return TRUE;
	case RISCV32_LINUX_SYS_KILL:
		return riscv32_linux_sys_signal_send(state,
		    state->thread.x[RISCV_X_A0], state->thread.x[RISCV_X_A1], TRUE);
	case RISCV32_LINUX_SYS_TKILL:
		return riscv32_linux_sys_signal_send(state,
		    state->thread.x[RISCV_X_A0], state->thread.x[RISCV_X_A1], FALSE);
	case RISCV32_LINUX_SYS_TGKILL:
		return riscv32_linux_sys_signal_send(state,
		    state->thread.x[RISCV_X_A1], state->thread.x[RISCV_X_A2], FALSE);
	case RISCV32_LINUX_SYS_SIGALTSTACK:
		return riscv32_linux_sys_sigaltstack(state);
	case RISCV32_LINUX_SYS_RT_SIGACTION:
		return riscv32_linux_sys_rt_sigaction(state);
	case RISCV32_LINUX_SYS_RT_SIGPROCMASK:
		return riscv32_linux_sys_rt_sigprocmask(state);
	case RISCV32_LINUX_SYS_RT_SIGPENDING:
		return riscv32_linux_sys_rt_sigpending(state);
	case RISCV32_LINUX_SYS_RT_SIGRETURN:
		return riscv32_linux_signal_rt_sigreturn(state);
	case RISCV32_LINUX_SYS_SETPRIORITY:
		return riscv32_linux_sys_setpriority(state);
	case RISCV32_LINUX_SYS_GETPRIORITY:
		return riscv32_linux_sys_getpriority(state);
	case RISCV32_LINUX_SYS_SETREGID:
		return riscv32_linux_sys_setregid(state);
	case RISCV32_LINUX_SYS_SETGID:
		return riscv32_linux_sys_setgid(state);
	case RISCV32_LINUX_SYS_SETREUID:
		return riscv32_linux_sys_setreuid(state);
	case RISCV32_LINUX_SYS_SETUID:
		return riscv32_linux_sys_setuid(state);
	case RISCV32_LINUX_SYS_SETRESUID:
		return riscv32_linux_sys_setresuid(state);
	case RISCV32_LINUX_SYS_GETRESUID:
		return riscv32_linux_sys_getresuid(state);
	case RISCV32_LINUX_SYS_SETRESGID:
		return riscv32_linux_sys_setresgid(state);
	case RISCV32_LINUX_SYS_GETRESGID:
		return riscv32_linux_sys_getresgid(state);
	case RISCV32_LINUX_SYS_SETFSUID:
		return riscv32_linux_sys_setfsuid(state);
	case RISCV32_LINUX_SYS_SETFSGID:
		return riscv32_linux_sys_setfsgid(state);
	case RISCV32_LINUX_SYS_TIMES:
		return riscv32_linux_sys_times(state);
	case RISCV32_LINUX_SYS_SETPGID:
		return riscv32_linux_sys_setpgid(state);
	case RISCV32_LINUX_SYS_GETPGID:
		return riscv32_linux_sys_getpgid(state);
	case RISCV32_LINUX_SYS_GETSID:
		return riscv32_linux_sys_getsid(state);
	case RISCV32_LINUX_SYS_SETSID:
		return riscv32_linux_sys_setsid(state);
	case RISCV32_LINUX_SYS_GETGROUPS:
		riscv32_linux_return_value(state, 0);
		return TRUE;
	case RISCV32_LINUX_SYS_SETGROUPS:
		return riscv32_linux_sys_setgroups(state);
	case RISCV32_LINUX_SYS_UNAME:
		return riscv32_linux_sys_uname(state);
	case RISCV32_LINUX_SYS_GETRUSAGE:
		return riscv32_linux_sys_getrusage(state);
	case RISCV32_LINUX_SYS_UMASK:
	{
		uint32_t old_mask = riscv32_linux_umask;

		riscv32_linux_umask = state->thread.x[RISCV_X_A0] & 0777U;
		riscv32_linux_return_value(state, old_mask);
		return TRUE;
	}
	case RISCV32_LINUX_SYS_PRCTL:
		return riscv32_linux_sys_prctl(state);
	case RISCV32_LINUX_SYS_GETCPU:
		return riscv32_linux_sys_getcpu(state);
	case RISCV32_LINUX_SYS_GETTIMEOFDAY:
		return riscv32_linux_sys_gettimeofday(state);
	case RISCV32_LINUX_SYS_GETPPID:
		riscv32_linux_return_value(state, riscv32_linux_current_ppid());
		return TRUE;
	case RISCV32_LINUX_SYS_GETPID:
	case RISCV32_LINUX_SYS_GETTID:
		riscv32_linux_return_value(state, riscv32_linux_current_pid());
		return TRUE;
	case RISCV32_LINUX_SYS_GETUID:
		riscv32_linux_return_value(state, riscv32_linux_uid);
		return TRUE;
	case RISCV32_LINUX_SYS_GETEUID:
		riscv32_linux_return_value(state, riscv32_linux_euid);
		return TRUE;
	case RISCV32_LINUX_SYS_GETGID:
		riscv32_linux_return_value(state, riscv32_linux_gid);
		return TRUE;
	case RISCV32_LINUX_SYS_GETEGID:
		riscv32_linux_return_value(state, riscv32_linux_egid);
		return TRUE;
	case RISCV32_LINUX_SYS_SYSINFO:
		return riscv32_linux_sys_sysinfo(state);
	case RISCV32_LINUX_SYS_SOCKET:
	case RISCV32_LINUX_SYS_SOCKETPAIR:
		return riscv32_linux_sys_socket_unsupported(state);
	case RISCV32_LINUX_SYS_BIND:
	case RISCV32_LINUX_SYS_LISTEN:
	case RISCV32_LINUX_SYS_ACCEPT:
	case RISCV32_LINUX_SYS_CONNECT:
	case RISCV32_LINUX_SYS_GETSOCKNAME:
	case RISCV32_LINUX_SYS_GETPEERNAME:
	case RISCV32_LINUX_SYS_SENDTO:
	case RISCV32_LINUX_SYS_RECVFROM:
	case RISCV32_LINUX_SYS_SETSOCKOPT:
	case RISCV32_LINUX_SYS_GETSOCKOPT:
	case RISCV32_LINUX_SYS_SHUTDOWN:
	case RISCV32_LINUX_SYS_SENDMSG:
	case RISCV32_LINUX_SYS_RECVMSG:
		return riscv32_linux_sys_socket_fd_notsock(state);
	case RISCV32_LINUX_SYS_BRK:
		return riscv32_linux_sys_brk(state);
	case RISCV32_LINUX_SYS_MUNMAP:
		return riscv32_linux_sys_munmap(state);
	case RISCV32_LINUX_SYS_MREMAP:
		return riscv32_linux_sys_mremap(state);
	case RISCV32_LINUX_SYS_CLONE:
		return riscv32_linux_sys_clone(state);
	case RISCV32_LINUX_SYS_EXECVE:
		return riscv32_linux_sys_execve(state);
	case RISCV32_LINUX_SYS_MMAP:
		return riscv32_linux_sys_mmap(state);
	case RISCV32_LINUX_SYS_MPROTECT:
		return riscv32_linux_sys_mprotect(state);
	case RISCV32_LINUX_SYS_MSYNC:
	case RISCV32_LINUX_SYS_MLOCK:
	case RISCV32_LINUX_SYS_MUNLOCK:
		return riscv32_linux_sys_memory_range_noop(state);
	case RISCV32_LINUX_SYS_MLOCKALL:
	case RISCV32_LINUX_SYS_MUNLOCKALL:
		riscv32_linux_return_value(state, 0);
		return TRUE;
	case RISCV32_LINUX_SYS_MINCORE:
		return riscv32_linux_sys_mincore(state);
	case RISCV32_LINUX_SYS_MADVISE:
		return riscv32_linux_sys_memory_range_noop(state);
	case RISCV32_LINUX_SYS_ACCEPT4:
	case RISCV32_LINUX_SYS_RECVMMSG:
		return riscv32_linux_sys_socket_fd_notsock(state);
	case RISCV32_LINUX_SYS_WAIT4:
		return riscv32_linux_sys_wait4(state);
	case RISCV32_LINUX_SYS_PRLIMIT64:
		return riscv32_linux_sys_prlimit64(state);
	case RISCV32_LINUX_SYS_SYNCFS:
		return riscv32_linux_sys_fd_noop(state);
	case RISCV32_LINUX_SYS_SENDMMSG:
		return riscv32_linux_sys_socket_fd_notsock(state);
	case RISCV32_LINUX_SYS_RENAMEAT2:
		return riscv32_linux_sys_renameat(state);
	case RISCV32_LINUX_SYS_GETRANDOM:
		return riscv32_linux_sys_getrandom(state);
	case RISCV32_LINUX_SYS_EXECVEAT:
		return riscv32_linux_sys_execveat(state);
	case RISCV32_LINUX_SYS_MLOCK2:
		return riscv32_linux_sys_memory_range_noop(state);
	case RISCV32_LINUX_SYS_MEMBARRIER:
		riscv32_linux_return_value(state, 0);
		return TRUE;
	case RISCV32_LINUX_SYS_STATX:
		return riscv32_linux_sys_statx(state);
	case RISCV32_LINUX_SYS_RSEQ:
		riscv32_linux_return_errno(state, RISCV32_LINUX_ENOSYS);
		return TRUE;
	case RISCV32_LINUX_SYS_CLOCK_GETTIME64:
		return riscv32_linux_sys_clock_gettime(state, TRUE);
	case RISCV32_LINUX_SYS_CLOCK_GETRES_TIME64:
		return riscv32_linux_sys_clock_getres(state, TRUE);
	case RISCV32_LINUX_SYS_CLOCK_NANOSLEEP_TIME64:
		return riscv32_linux_sys_clock_nanosleep(state, TRUE);
	case RISCV32_LINUX_SYS_TIMERFD_GETTIME64:
		return riscv32_linux_sys_timerfd_gettime(state, TRUE);
	case RISCV32_LINUX_SYS_TIMERFD_SETTIME64:
		return riscv32_linux_sys_timerfd_settime(state, TRUE);
	case RISCV32_LINUX_SYS_UTIMENSAT_TIME64:
		return riscv32_linux_sys_utimensat(state, TRUE);
	case RISCV32_LINUX_SYS_PSELECT6_TIME64:
		return riscv32_linux_sys_pselect6(state, TRUE);
	case RISCV32_LINUX_SYS_PPOLL_TIME64:
		return riscv32_linux_sys_ppoll(state, TRUE);
	case RISCV32_LINUX_SYS_RECVMMSG_TIME64:
		return riscv32_linux_sys_socket_fd_notsock(state);
	case RISCV32_LINUX_SYS_FUTEX_TIME64:
		return riscv32_linux_sys_futex(state);
	case RISCV32_LINUX_SYS_EPOLL_PWAIT2:
		return riscv32_linux_sys_epoll_pwait(state, TRUE);
	case RISCV32_LINUX_SYS_FCHMODAT2:
		return riscv32_linux_sys_readonly_existing_path(state,
		    state->thread.x[RISCV_X_A1]);
	default:
		break;
	}

	riscv32_linux_return_errno(state, RISCV32_LINUX_ENOSYS);
	return TRUE;
}
