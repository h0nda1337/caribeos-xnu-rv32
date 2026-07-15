#include <kern/debug.h>
#include <kern/sched_prim.h>
#include <kern/thread.h>
#include <machine/bootinfo.h>
#include <machine/copyio.h>
#include <machine/early_print.h>
#include <machine/linux_process.h>
#include <machine/linux_signal.h>
#include <machine/linux_syscall.h>
#include <machine/pmap.h>
#include <machine/user_exception.h>
#include <string.h>

#define RISCV32_LINUX_SIGNAL_SIGINFO_SIZE	128U
#define RISCV32_LINUX_SIGNAL_UCONTEXT_SIZE	816U
#define RISCV32_LINUX_SIGNAL_FRAME_SIZE		944U
#define RISCV32_LINUX_SIGNAL_UCONTEXT_OFFSET	128U
#define RISCV32_LINUX_SIGNAL_UC_STACK_OFFSET	8U
#define RISCV32_LINUX_SIGNAL_UC_SIGMASK_OFFSET	20U
#define RISCV32_LINUX_SIGNAL_UC_MCONTEXT_OFFSET	160U
#define RISCV32_LINUX_SIGNAL_GREG_COUNT		32U
#define RISCV32_LINUX_SIGNAL_ACTION_MASK_OFFSET	12U
#define RISCV32_LINUX_SIGNAL_SIGSET_ABI_SIZE	8U
#define RISCV32_LINUX_SIGNAL_MAX		64U
#define RISCV32_LINUX_SIGNAL_SIGINT		2U
#define RISCV32_LINUX_SIGNAL_SIGQUIT		3U
#define RISCV32_LINUX_SIGNAL_SIGKILL		9U
#define RISCV32_LINUX_SIGNAL_SIGUSR1		10U
#define RISCV32_LINUX_SIGNAL_SIGPIPE		13U
#define RISCV32_LINUX_SIGNAL_SIGCONT		18U
#define RISCV32_LINUX_SIGNAL_SIGSTOP		19U
#define RISCV32_LINUX_SIGNAL_SIGTSTP		20U
#define RISCV32_LINUX_SIGNAL_SIGTTIN		21U
#define RISCV32_LINUX_SIGNAL_SIGTTOU		22U
#define RISCV32_LINUX_SIGNAL_SA_SIGINFO		0x00000004U
#define RISCV32_LINUX_SIGNAL_SA_RESTORER	0x04000000U
#define RISCV32_LINUX_SIGNAL_SA_ONSTACK		0x08000000U
#define RISCV32_LINUX_SIGNAL_SA_NODEFER		0x40000000U
#define RISCV32_LINUX_SIGNAL_SS_ONSTACK		1U
#define RISCV32_LINUX_SIGNAL_SS_DISABLE		2U
#define RISCV32_LINUX_SIGNAL_USER_TOP		0x80000000U
#define RISCV32_LINUX_SIGNAL_EFAULT		14U
#define RISCV32_LINUX_SIGNAL_EINVAL		22U
#define RISCV32_LINUX_SIGNAL_CLD_EXITED		1U

static volatile uint32_t riscv32_linux_signal_frames_delivered;
static volatile uint32_t riscv32_linux_signal_frames_returned;
static volatile uint32_t riscv32_linux_signal_default_ignored;
static volatile uint32_t riscv32_linux_signal_frame_failures;
static volatile uint32_t riscv32_linux_signals_queued;
static volatile uint32_t riscv32_linux_signals_discarded;
static volatile uint32_t riscv32_linux_signal_target_wakeups;

static boolean_t
riscv32_linux_signal_default_terminates(uint32_t signal)
{
	switch (signal) {
	case 1U:  /* SIGHUP */
	case RISCV32_LINUX_SIGNAL_SIGINT:
	case RISCV32_LINUX_SIGNAL_SIGQUIT:
	case 4U:  /* SIGILL */
	case 5U:  /* SIGTRAP */
	case 6U:  /* SIGABRT */
	case 7U:  /* SIGBUS */
	case 8U:  /* SIGFPE */
	case RISCV32_LINUX_SIGNAL_SIGKILL:
	case RISCV32_LINUX_SIGNAL_SIGUSR1:
	case 11U: /* SIGSEGV */
	case 12U: /* SIGUSR2 */
	case RISCV32_LINUX_SIGNAL_SIGPIPE:
	case 14U: /* SIGALRM */
	case 15U: /* SIGTERM */
	case 16U: /* SIGSTKFLT */
	case 24U: /* SIGXCPU */
	case 25U: /* SIGXFSZ */
	case 26U: /* SIGVTALRM */
	case 27U: /* SIGPROF */
	case 29U: /* SIGIO */
	case 30U: /* SIGPWR */
	case 31U: /* SIGSYS */
		return TRUE;
	default:
		return signal >= 32U && signal <= RISCV32_LINUX_SIGNAL_MAX;
	}
}

static boolean_t
riscv32_linux_signal_default_ignores(uint32_t signal)
{
	return signal == RISCV32_LINUX_SIGCHLD ||
	    signal == RISCV32_LINUX_SIGNAL_SIGCONT ||
	    signal == 23U || /* SIGURG */
	    signal == 28U;   /* SIGWINCH */
}

static boolean_t
riscv32_linux_signal_default_stops(uint32_t signal)
{
	return signal == RISCV32_LINUX_SIGNAL_SIGSTOP ||
	    signal == RISCV32_LINUX_SIGNAL_SIGTSTP ||
	    signal == RISCV32_LINUX_SIGNAL_SIGTTIN ||
	    signal == RISCV32_LINUX_SIGNAL_SIGTTOU;
}

static void
riscv32_linux_signal_clear_pending_locked(riscv32_linux_process_t *process,
    uint32_t signal)
{
	uint32_t index = (signal - 1U) / 8U;
	uint32_t bit = 1U << ((signal - 1U) % 8U);

	process->personality.pending_signals[index] &= (uint8_t)~bit;
	memset(&process->pending_signal_info[signal], 0,
	    sizeof(process->pending_signal_info[signal]));
}

static uint32_t
riscv32_linux_signal_get_u32(const uint8_t *bytes, uint32_t offset)
{
	return (uint32_t)bytes[offset] |
	    ((uint32_t)bytes[offset + 1U] << 8) |
	    ((uint32_t)bytes[offset + 2U] << 16) |
	    ((uint32_t)bytes[offset + 3U] << 24);
}

static void
riscv32_linux_signal_put_u32(uint8_t *bytes, uint32_t offset, uint32_t value)
{
	bytes[offset] = (uint8_t)value;
	bytes[offset + 1U] = (uint8_t)(value >> 8);
	bytes[offset + 2U] = (uint8_t)(value >> 16);
	bytes[offset + 3U] = (uint8_t)(value >> 24);
}

static void
riscv32_linux_signal_return_errno(riscv_saved_state_t *state, uint32_t error)
{
	state->thread.x[RISCV_X_A0] = 0U - error;
	state->thread.x[RISCV_X_A1] = error;
}

static boolean_t
riscv32_linux_signal_user_pc_valid(pmap_t pmap, uint32_t pc)
{
	return pc != 0U && pc < RISCV32_LINUX_SIGNAL_USER_TOP &&
	    (pc & 1U) == 0U &&
	    riscv32_pmap_user_range_accessible(pmap, pc, 2U, FALSE);
}

static boolean_t
riscv32_linux_signal_user_sp_valid(pmap_t pmap, uint32_t sp)
{
	return sp > 1U && sp < RISCV32_LINUX_SIGNAL_USER_TOP &&
	    riscv32_pmap_user_range_accessible(pmap, sp - 1U, 1U, FALSE);
}

static uint32_t
riscv32_linux_signal_handler_locked(riscv32_linux_process_t *process,
    uint32_t signal)
{
	if (process->personality.sigactions[signal].installed == 0U) {
		return 0U;
	}
	return riscv32_linux_signal_get_u32(
	    process->personality.sigactions[signal].bytes, 0U);
}

boolean_t
riscv32_linux_signal_has_deliverable(riscv32_linux_process_t *process)
{
	uint32_t signal;
	boolean_t deliverable = FALSE;

	if (process == (riscv32_linux_process_t *)0 ||
	    process->magic != RISCV32_LINUX_PROCESS_MAGIC) {
		return FALSE;
	}
	simple_lock(&process->lock);
	for (signal = 1U; signal <= RISCV32_LINUX_SIGNAL_MAX; signal++) {
		uint32_t index = (signal - 1U) / 8U;
		uint32_t bit = 1U << ((signal - 1U) % 8U);
		uint32_t handler;

		if ((process->personality.pending_signals[index] & bit) == 0U ||
		    (process->personality.signal_mask[index] & bit) != 0U) {
			continue;
		}
		handler = riscv32_linux_signal_handler_locked(process, signal);
		if ((handler != 0U && handler != 1U) ||
		    (handler == 0U &&
		    (riscv32_linux_signal_default_terminates(signal) ||
		    riscv32_linux_signal_default_stops(signal)))) {
			deliverable = TRUE;
			break;
		}
	}
	simple_unlock(&process->lock);
	return deliverable;
}

boolean_t
riscv32_linux_signal_queue(riscv32_linux_process_t *process,
    uint32_t signal, uint32_t sender_pid, uint32_t sender_uid,
    uint32_t code, uint32_t status)
{
	thread_t target_thread = THREAD_NULL;
	event_t wait_channel = (event_t)0;
	uint32_t index;
	uint32_t bit;
	uint32_t handler;
	boolean_t wake_target = FALSE;
	boolean_t discard = FALSE;
	boolean_t resumed = FALSE;

	if (process == (riscv32_linux_process_t *)0 ||
	    process->magic != RISCV32_LINUX_PROCESS_MAGIC || signal == 0U ||
	    signal > RISCV32_LINUX_SIGNAL_MAX) {
		return FALSE;
	}
	if ((signal == RISCV32_LINUX_SIGNAL_SIGCONT ||
	    signal == RISCV32_LINUX_SIGNAL_SIGKILL) &&
	    !riscv32_linux_process_continue(process,
	    signal == RISCV32_LINUX_SIGNAL_SIGCONT, &resumed)) {
		return FALSE;
	}
	index = (signal - 1U) / 8U;
	bit = 1U << ((signal - 1U) % 8U);
	simple_lock(&process->lock);
	if (process->state == RISCV32_LINUX_PROCESS_EXITING ||
	    process->state == RISCV32_LINUX_PROCESS_ZOMBIE ||
	    process->state == RISCV32_LINUX_PROCESS_DEAD) {
		simple_unlock(&process->lock);
		return FALSE;
	}
	if (signal == RISCV32_LINUX_SIGNAL_SIGCONT) {
		riscv32_linux_signal_clear_pending_locked(process,
		    RISCV32_LINUX_SIGNAL_SIGSTOP);
		riscv32_linux_signal_clear_pending_locked(process,
		    RISCV32_LINUX_SIGNAL_SIGTSTP);
		riscv32_linux_signal_clear_pending_locked(process,
		    RISCV32_LINUX_SIGNAL_SIGTTIN);
		riscv32_linux_signal_clear_pending_locked(process,
		    RISCV32_LINUX_SIGNAL_SIGTTOU);
	} else if (riscv32_linux_signal_default_stops(signal)) {
		riscv32_linux_signal_clear_pending_locked(process,
		    RISCV32_LINUX_SIGNAL_SIGCONT);
	}
	handler = riscv32_linux_signal_handler_locked(process, signal);
	if (handler == 1U || (handler == 0U &&
	    riscv32_linux_signal_default_ignores(signal))) {
		process->personality.pending_signals[index] &= (uint8_t)~bit;
		memset(&process->pending_signal_info[signal], 0,
		    sizeof(process->pending_signal_info[signal]));
		discard = TRUE;
	} else if (handler == 0U &&
	    !riscv32_linux_signal_default_terminates(signal) &&
	    !riscv32_linux_signal_default_stops(signal)) {
		simple_unlock(&process->lock);
		return FALSE;
	} else {
		process->pending_signal_info[signal].sender_pid = sender_pid;
		process->pending_signal_info[signal].sender_uid = sender_uid;
		process->pending_signal_info[signal].code = code;
		process->pending_signal_info[signal].status = status;
		process->personality.pending_signals[index] |= (uint8_t)bit;
		process->signal_queue_count++;
		if (process->state == RISCV32_LINUX_PROCESS_SLEEPING &&
		    process->main_thread != THREAD_NULL) {
			target_thread = process->main_thread;
			wait_channel = process->wait_channel;
			process->signal_wakeup_count++;
			wake_target = TRUE;
		}
	}
	simple_unlock(&process->lock);
	if (resumed && signal == RISCV32_LINUX_SIGNAL_SIGCONT &&
	    riscv32_boot_arg_has("process-gate-13")) {
		riscv32_early_puts(
		    "[process-gate-13] SIGCONT resumed pid=");
		riscv32_early_putdec(process->pid);
		riscv32_early_puts(" pgid=");
		riscv32_early_putdec(process->personality.pgid);
		riscv32_early_puts(" PASS\n");
	}
	if (discard) {
		(void)__sync_add_and_fetch(&riscv32_linux_signals_discarded, 1U);
		return TRUE;
	}
	(void)__sync_add_and_fetch(&riscv32_linux_signals_queued, 1U);
	if (wake_target) {
		(void)__sync_add_and_fetch(
		    &riscv32_linux_signal_target_wakeups, 1U);
		thread_wakeup(wait_channel);
		thread_wakeup((event_t)target_thread);
		(void)clear_wait(target_thread, THREAD_AWAKENED);
	}
	return TRUE;
}

void
riscv32_linux_signal_get_stats(riscv32_linux_signal_stats_t *stats)
{
	if (stats == (riscv32_linux_signal_stats_t *)0) {
		return;
	}
	stats->frames_delivered = __sync_fetch_and_add(
	    &riscv32_linux_signal_frames_delivered, 0U);
	stats->frames_returned = __sync_fetch_and_add(
	    &riscv32_linux_signal_frames_returned, 0U);
	stats->default_ignored = __sync_fetch_and_add(
	    &riscv32_linux_signal_default_ignored, 0U);
	stats->frame_failures = __sync_fetch_and_add(
	    &riscv32_linux_signal_frame_failures, 0U);
	stats->signals_queued = __sync_fetch_and_add(
	    &riscv32_linux_signals_queued, 0U);
	stats->signals_discarded = __sync_fetch_and_add(
	    &riscv32_linux_signals_discarded, 0U);
	stats->target_wakeups = __sync_fetch_and_add(
	    &riscv32_linux_signal_target_wakeups, 0U);
}

static boolean_t
riscv32_linux_signal_stop_current(riscv32_linux_process_t *process,
    uint32_t signal)
{
	riscv32_linux_process_state_t state;
	uint32_t expected;
	boolean_t should_block;

	if (!riscv32_linux_process_publish_stop(process, signal)) {
		return FALSE;
	}
	if (riscv32_boot_arg_has("process-gate-13") &&
	    signal == RISCV32_LINUX_SIGNAL_SIGTSTP) {
		riscv32_early_puts(
		    "[process-gate-13] default SIGTSTP stopped pid=");
		riscv32_early_putdec(process->pid);
		riscv32_early_puts(" pgid=");
		riscv32_early_putdec(process->personality.pgid);
		riscv32_early_puts(" signal=20 PASS\n");
	}
	for (;;) {
		simple_lock(&process->lock);
		state = process->state;
		expected = process->stop_sequence;
		simple_unlock(&process->lock);
		if (state != RISCV32_LINUX_PROCESS_STOPPED) {
			break;
		}
		assert_wait((event_t)&process->stop_sequence, THREAD_UNINT);
		__sync_synchronize();
		simple_lock(&process->lock);
		should_block = process->state ==
		    RISCV32_LINUX_PROCESS_STOPPED &&
		    process->stop_sequence == expected;
		simple_unlock(&process->lock);
		if (should_block) {
			thread_block(THREAD_CONTINUE_NULL);
		} else {
			(void)clear_wait(current_thread(), THREAD_AWAKENED);
		}
	}
	if (state == RISCV32_LINUX_PROCESS_RUNNABLE &&
	    !riscv32_linux_process_transition(process,
	    RISCV32_LINUX_PROCESS_RUNNING)) {
		panic("riscv32: PID %u cannot resume after SIGCONT", process->pid);
	}
	return TRUE;
}

boolean_t
riscv32_linux_signal_deliver_pending(riscv_saved_state_t *state)
{
	riscv32_linux_process_t *process = riscv32_linux_process_current();
	uint8_t frame[RISCV32_LINUX_SIGNAL_FRAME_SIZE]
	    __attribute__((aligned(16)));
	uint8_t old_mask[RISCV32_LINUX_PROCESS_SIGSET_SIZE];
	uint8_t action_mask[RISCV32_LINUX_SIGNAL_SIGSET_ABI_SIZE];
	uint8_t saved_altstack[RISCV32_LINUX_PROCESS_SIGALTSTACK_SIZE];
	riscv32_linux_pending_signal_info_t signal_info;
	uint32_t signal = 0U;
	uint32_t signal_index = 0U;
	uint32_t signal_bit = 0U;
	uint32_t handler;
	uint32_t flags;
	uint32_t restorer;
	uint32_t frame_top;
	uint32_t frame_sp;
	uint32_t current_sp;
	uint32_t alt_sp;
	uint32_t alt_flags;
	uint32_t alt_size;
	uint32_t alt_top;
	uint32_t info_pid;
	uint32_t info_uid;
	uint32_t info_status;
	uint32_t info_code;
	uint32_t discarded = 0U;
	uint32_t i;
	boolean_t default_terminate = FALSE;
	boolean_t default_stop = FALSE;

	if (state == (riscv_saved_state_t *)0 ||
	    process == (riscv32_linux_process_t *)0 ||
	    process->pmap == PMAP_NULL) {
		return FALSE;
	}
	simple_lock(&process->lock);
	if (process->signal_frame_active != 0U) {
		simple_unlock(&process->lock);
		return FALSE;
	}
	for (i = 1U; i <= RISCV32_LINUX_SIGNAL_MAX; i++) {
		uint32_t candidate_index = (i - 1U) / 8U;
		uint32_t candidate_bit = 1U << ((i - 1U) % 8U);

		if ((process->personality.pending_signals[candidate_index] &
		    candidate_bit) == 0U ||
		    (process->personality.signal_mask[candidate_index] &
		    candidate_bit) != 0U) {
			continue;
		}
		handler = riscv32_linux_signal_handler_locked(process, i);
		if (handler == 1U || (handler == 0U &&
		    !riscv32_linux_signal_default_terminates(i) &&
		    !riscv32_linux_signal_default_stops(i))) {
			process->personality.pending_signals[candidate_index] &=
			    (uint8_t)~candidate_bit;
			memset(&process->pending_signal_info[i], 0,
			    sizeof(process->pending_signal_info[i]));
			discarded++;
			continue;
		}
		default_terminate = handler == 0U &&
		    riscv32_linux_signal_default_terminates(i);
		default_stop = handler == 0U &&
		    riscv32_linux_signal_default_stops(i);
		signal = i;
		signal_index = candidate_index;
		signal_bit = candidate_bit;
		break;
	}
	if (signal == 0U) {
		simple_unlock(&process->lock);
		if (discarded != 0U) {
			(void)__sync_add_and_fetch(
			    &riscv32_linux_signal_default_ignored, discarded);
			(void)__sync_add_and_fetch(
			    &riscv32_linux_signals_discarded, discarded);
		}
		return FALSE;
	}
	flags = riscv32_linux_signal_get_u32(
	    process->personality.sigactions[signal].bytes, 4U);
	restorer = riscv32_linux_signal_get_u32(
	    process->personality.sigactions[signal].bytes, 8U);
	memcpy(old_mask, process->personality.signal_mask, sizeof(old_mask));
	memcpy(action_mask,
	    &process->personality.sigactions[signal].bytes[
	    RISCV32_LINUX_SIGNAL_ACTION_MASK_OFFSET], sizeof(action_mask));
	memcpy(saved_altstack, process->personality.sigaltstack,
	    sizeof(saved_altstack));
	signal_info = process->pending_signal_info[signal];
	info_pid = signal_info.sender_pid;
	info_uid = signal_info.sender_uid;
	info_status = signal_info.status;
	info_code = signal_info.code;
	process->personality.pending_signals[signal_index] &=
	    (uint8_t)~signal_bit;
	memset(&process->pending_signal_info[signal], 0,
	    sizeof(process->pending_signal_info[signal]));
	if (default_terminate) {
		process->last_signal = signal;
		simple_unlock(&process->lock);
		if (discarded != 0U) {
			(void)__sync_add_and_fetch(
			    &riscv32_linux_signal_default_ignored, discarded);
			(void)__sync_add_and_fetch(
			    &riscv32_linux_signals_discarded, discarded);
		}
		if (riscv32_boot_arg_has("process-gate-12") &&
		    signal == RISCV32_LINUX_SIGNAL_SIGPIPE) {
			riscv32_early_puts(
			    "[process-gate-12] default SIGPIPE terminating pid=");
			riscv32_early_putdec(process->pid);
			riscv32_early_puts(" at syscall return PASS\n");
		} else if (riscv32_boot_arg_has("process-gate-13") &&
		    signal == RISCV32_LINUX_SIGNAL_SIGINT) {
			riscv32_early_puts(
			    "[process-gate-13] default SIGINT terminating pid=");
			riscv32_early_putdec(process->pid);
			riscv32_early_puts(" at syscall return PASS\n");
		} else if (riscv32_boot_arg_has("process-gate-13") &&
		    signal == RISCV32_LINUX_SIGNAL_SIGQUIT) {
			riscv32_early_puts(
			    "[process-gate-13] default SIGQUIT terminating pid=");
			riscv32_early_putdec(process->pid);
			riscv32_early_puts(" at syscall return PASS\n");
		}
		riscv32_linux_terminate_current(0U, signal);
	}
	if (default_stop) {
		process->last_signal = signal;
		simple_unlock(&process->lock);
		if (discarded != 0U) {
			(void)__sync_add_and_fetch(
			    &riscv32_linux_signal_default_ignored, discarded);
			(void)__sync_add_and_fetch(
			    &riscv32_linux_signals_discarded, discarded);
		}
		if (!riscv32_linux_signal_stop_current(process, signal)) {
			panic("riscv32: PID %u cannot stop for signal %u",
			    process->pid, signal);
		}
		return riscv32_linux_signal_deliver_pending(state);
	}
	for (i = 0U; i < sizeof(action_mask); i++) {
		process->personality.signal_mask[i] |= action_mask[i];
	}
	if ((flags & RISCV32_LINUX_SIGNAL_SA_NODEFER) == 0U) {
		process->personality.signal_mask[signal_index] |=
		    (uint8_t)signal_bit;
	}
	process->personality.signal_mask[(9U - 1U) / 8U] &=
	    (uint8_t)~(1U << ((9U - 1U) % 8U));
	process->personality.signal_mask[(19U - 1U) / 8U] &=
	    (uint8_t)~(1U << ((19U - 1U) % 8U));
	process->signal_frame_active = 1U;
	process->signal_frame_sp = 0U;
	process->last_signal = signal;
	simple_unlock(&process->lock);
	if (discarded != 0U) {
		(void)__sync_add_and_fetch(
		    &riscv32_linux_signal_default_ignored, discarded);
		(void)__sync_add_and_fetch(
		    &riscv32_linux_signals_discarded, discarded);
	}

	alt_sp = riscv32_linux_signal_get_u32(saved_altstack, 0U);
	alt_flags = riscv32_linux_signal_get_u32(saved_altstack, 4U);
	alt_size = riscv32_linux_signal_get_u32(saved_altstack, 8U);
	alt_top = alt_sp + alt_size;
	current_sp = state->thread.x[RISCV_X_SP];
	frame_top = current_sp;
	if ((flags & RISCV32_LINUX_SIGNAL_SA_ONSTACK) != 0U &&
	    alt_sp != 0U && alt_size >= RISCV32_LINUX_SIGNAL_FRAME_SIZE &&
	    (alt_flags & RISCV32_LINUX_SIGNAL_SS_DISABLE) == 0U &&
	    alt_top > alt_sp && alt_top < RISCV32_LINUX_SIGNAL_USER_TOP &&
	    !(current_sp >= alt_sp && current_sp < alt_top)) {
		frame_top = alt_top;
	}
	if (frame_top < RISCV32_LINUX_SIGNAL_FRAME_SIZE) {
		goto frame_failure;
	}
	frame_sp = (frame_top - RISCV32_LINUX_SIGNAL_FRAME_SIZE) & ~0xfU;
	if (frame_sp >= RISCV32_LINUX_SIGNAL_USER_TOP ||
	    !riscv32_pmap_user_range_accessible(process->pmap, frame_sp,
	    RISCV32_LINUX_SIGNAL_FRAME_SIZE, TRUE) ||
	    !riscv32_linux_signal_user_pc_valid(process->pmap, handler) ||
	    (flags & RISCV32_LINUX_SIGNAL_SA_RESTORER) == 0U ||
	    !riscv32_linux_signal_user_pc_valid(process->pmap, restorer)) {
		goto frame_failure;
	}

	memset(frame, 0, sizeof(frame));
	riscv32_linux_signal_put_u32(frame, 0U, signal);
	riscv32_linux_signal_put_u32(frame, 8U,
	    signal == RISCV32_LINUX_SIGCHLD && info_code == 0U ?
	    RISCV32_LINUX_SIGNAL_CLD_EXITED : info_code);
	riscv32_linux_signal_put_u32(frame, 12U, info_pid);
	riscv32_linux_signal_put_u32(frame, 16U, info_uid);
	riscv32_linux_signal_put_u32(frame, 20U, info_status);
	memcpy(&frame[RISCV32_LINUX_SIGNAL_UCONTEXT_OFFSET +
	    RISCV32_LINUX_SIGNAL_UC_STACK_OFFSET],
	    saved_altstack, sizeof(saved_altstack));
	memcpy(&frame[RISCV32_LINUX_SIGNAL_UCONTEXT_OFFSET +
	    RISCV32_LINUX_SIGNAL_UC_SIGMASK_OFFSET], old_mask,
	    sizeof(old_mask));
	riscv32_linux_signal_put_u32(frame,
	    RISCV32_LINUX_SIGNAL_UCONTEXT_OFFSET +
	    RISCV32_LINUX_SIGNAL_UC_MCONTEXT_OFFSET,
	    state->exception.sepc);
	for (i = 1U; i < RISCV32_LINUX_SIGNAL_GREG_COUNT; i++) {
		riscv32_linux_signal_put_u32(frame,
		    RISCV32_LINUX_SIGNAL_UCONTEXT_OFFSET +
		    RISCV32_LINUX_SIGNAL_UC_MCONTEXT_OFFSET + i * 4U,
		    state->thread.x[i]);
	}
	if (riscv32_copyout(frame, frame_sp, sizeof(frame)) != 0) {
		goto frame_failure;
	}

	simple_lock(&process->lock);
	if (process->signal_frame_active != 1U ||
	    process->signal_frame_sp != 0U) {
		simple_unlock(&process->lock);
		panic("riscv32: signal frame publication raced pid=%u", process->pid);
	}
	process->signal_frame_sp = frame_sp;
	process->signal_delivery_count++;
	simple_unlock(&process->lock);
	state->thread.x[RISCV_X_RA] = restorer;
	state->thread.x[RISCV_X_SP] = frame_sp;
	state->thread.x[RISCV_X_A0] = signal;
	state->thread.x[RISCV_X_A1] = frame_sp;
	state->thread.x[RISCV_X_A2] = frame_sp +
	    RISCV32_LINUX_SIGNAL_UCONTEXT_OFFSET;
	state->thread.pc = handler;
	state->exception.sepc = handler;
	(void)__sync_add_and_fetch(&riscv32_linux_signal_frames_delivered, 1U);
	if ((process->flags & RISCV32_LINUX_PROCESS_FLAG_GATE8_PARENT) != 0U &&
	    signal == RISCV32_LINUX_SIGCHLD) {
		riscv32_early_puts("[linux-signal] SIGCHLD frame pid=");
		riscv32_early_putdec(process->pid);
		riscv32_early_puts(" child=");
		riscv32_early_putdec(info_pid);
		riscv32_early_puts(" status=");
		riscv32_early_putdec(info_status);
		riscv32_early_puts(" sp=0x");
		riscv32_early_puthex32(frame_sp);
		riscv32_early_puts(" handler=0x");
		riscv32_early_puthex32(handler);
		riscv32_early_puts(" PASS\n");
	} else if ((process->flags &
	    (RISCV32_LINUX_PROCESS_FLAG_GATE9_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_GATE10_PARENT)) != 0U) {
		riscv32_early_puts("[linux-signal] signal=");
		riscv32_early_putdec(signal);
		riscv32_early_puts(" frame pid=");
		riscv32_early_putdec(process->pid);
		riscv32_early_puts(" sender=");
		riscv32_early_putdec(info_pid);
		riscv32_early_puts(" sp=0x");
		riscv32_early_puthex32(frame_sp);
		riscv32_early_puts(" handler=0x");
		riscv32_early_puthex32(handler);
		riscv32_early_puts(" PASS\n");
	}
	return TRUE;

frame_failure:
	simple_lock(&process->lock);
	memcpy(process->personality.signal_mask, old_mask, sizeof(old_mask));
	process->personality.pending_signals[signal_index] |=
	    (uint8_t)signal_bit;
	process->pending_signal_info[signal] = signal_info;
	process->signal_frame_active = 0U;
	process->signal_frame_sp = 0U;
	simple_unlock(&process->lock);
	(void)__sync_add_and_fetch(&riscv32_linux_signal_frame_failures, 1U);
	riscv32_early_puts("[linux-signal] signal=");
	riscv32_early_putdec(signal);
	riscv32_early_puts(" frame setup failed pid=");
	riscv32_early_putdec(process->pid);
	riscv32_early_puts(" sp=0x");
	riscv32_early_puthex32(current_sp);
	riscv32_early_puts("\n");
	return FALSE;
}

boolean_t
riscv32_linux_signal_rt_sigreturn(riscv_saved_state_t *state)
{
	riscv32_linux_process_t *process = riscv32_linux_process_current();
	uint32_t gregs[RISCV32_LINUX_SIGNAL_GREG_COUNT];
	uint8_t restored_mask[RISCV32_LINUX_PROCESS_SIGSET_SIZE];
	uint8_t restored_altstack[RISCV32_LINUX_PROCESS_SIGALTSTACK_SIZE];
	uint32_t frame_sp;
	uint32_t restored_pc;
	uint32_t restored_sp;
	uint32_t restored_alt_sp;
	uint32_t restored_alt_flags;
	uint32_t restored_alt_size;
	uint32_t delivered_signal;
	uint32_t i;

	if (state == (riscv_saved_state_t *)0 ||
	    process == (riscv32_linux_process_t *)0 ||
	    process->pmap == PMAP_NULL) {
		return FALSE;
	}
	frame_sp = state->thread.x[RISCV_X_SP];
	simple_lock(&process->lock);
	if (process->signal_frame_active != 1U ||
	    process->signal_frame_sp != frame_sp ||
	    process->last_signal == 0U) {
		simple_unlock(&process->lock);
		riscv32_linux_signal_return_errno(state,
		    RISCV32_LINUX_SIGNAL_EINVAL);
		return TRUE;
	}
	delivered_signal = process->last_signal;
	simple_unlock(&process->lock);
	if (frame_sp >= RISCV32_LINUX_SIGNAL_USER_TOP ||
	    frame_sp + RISCV32_LINUX_SIGNAL_FRAME_SIZE < frame_sp ||
	    riscv32_copyin(frame_sp + RISCV32_LINUX_SIGNAL_UCONTEXT_OFFSET +
	    RISCV32_LINUX_SIGNAL_UC_MCONTEXT_OFFSET, gregs,
	    sizeof(gregs)) != 0 ||
	    riscv32_copyin(frame_sp + RISCV32_LINUX_SIGNAL_UCONTEXT_OFFSET +
	    RISCV32_LINUX_SIGNAL_UC_SIGMASK_OFFSET, restored_mask,
	    sizeof(restored_mask)) != 0 ||
	    riscv32_copyin(frame_sp + RISCV32_LINUX_SIGNAL_UCONTEXT_OFFSET +
	    RISCV32_LINUX_SIGNAL_UC_STACK_OFFSET, restored_altstack,
	    sizeof(restored_altstack)) != 0) {
		riscv32_linux_signal_return_errno(state,
		    RISCV32_LINUX_SIGNAL_EFAULT);
		return TRUE;
	}
	restored_pc = gregs[0];
	restored_sp = gregs[RISCV_X_SP];
	restored_alt_sp = riscv32_linux_signal_get_u32(restored_altstack, 0U);
	restored_alt_flags = riscv32_linux_signal_get_u32(restored_altstack, 4U);
	restored_alt_size = riscv32_linux_signal_get_u32(restored_altstack, 8U);
	if (!riscv32_linux_signal_user_pc_valid(process->pmap, restored_pc) ||
	    !riscv32_linux_signal_user_sp_valid(process->pmap, restored_sp) ||
	    (restored_alt_flags & ~(RISCV32_LINUX_SIGNAL_SS_ONSTACK |
	    RISCV32_LINUX_SIGNAL_SS_DISABLE)) != 0U ||
	    ((restored_alt_flags & RISCV32_LINUX_SIGNAL_SS_DISABLE) == 0U &&
	    (restored_alt_sp != 0U || restored_alt_size != 0U) &&
	    (restored_alt_sp == 0U ||
	    restored_alt_size < RISCV32_LINUX_SIGNAL_FRAME_SIZE ||
	    restored_alt_sp + restored_alt_size < restored_alt_sp ||
	    !riscv32_pmap_user_range_accessible(process->pmap,
	    restored_alt_sp, restored_alt_size, TRUE)))) {
		riscv32_linux_signal_return_errno(state,
		    RISCV32_LINUX_SIGNAL_EINVAL);
		return TRUE;
	}

	simple_lock(&process->lock);
	if (process->signal_frame_active != 1U ||
	    process->signal_frame_sp != frame_sp) {
		simple_unlock(&process->lock);
		panic("riscv32: rt_sigreturn frame ownership raced pid=%u",
		    process->pid);
	}
	memcpy(process->personality.signal_mask, restored_mask,
	    sizeof(restored_mask));
	process->personality.signal_mask[(9U - 1U) / 8U] &=
	    (uint8_t)~(1U << ((9U - 1U) % 8U));
	process->personality.signal_mask[(19U - 1U) / 8U] &=
	    (uint8_t)~(1U << ((19U - 1U) % 8U));
	memcpy(process->personality.sigaltstack, restored_altstack,
	    sizeof(restored_altstack));
	riscv32_linux_signal_put_u32(process->personality.sigaltstack, 4U,
	    restored_alt_flags & ~RISCV32_LINUX_SIGNAL_SS_ONSTACK);
	process->signal_frame_active = 0U;
	process->signal_frame_sp = 0U;
	process->signal_return_count++;
	simple_unlock(&process->lock);
	state->thread.x[0] = 0U;
	for (i = 1U; i < RISCV32_LINUX_SIGNAL_GREG_COUNT; i++) {
		state->thread.x[i] = gregs[i];
	}
	state->thread.pc = restored_pc;
	state->exception.sepc = restored_pc;
	(void)__sync_add_and_fetch(&riscv32_linux_signal_frames_returned, 1U);
	if ((process->flags & RISCV32_LINUX_PROCESS_FLAG_GATE8_PARENT) != 0U &&
	    delivered_signal == RISCV32_LINUX_SIGCHLD) {
		riscv32_early_puts(
		    "[linux-signal] rt_sigreturn restored pc=0x");
		riscv32_early_puthex32(restored_pc);
		riscv32_early_puts(" sp=0x");
		riscv32_early_puthex32(restored_sp);
		riscv32_early_puts(" cpu=0 PASS\n");
	} else if ((process->flags &
	    (RISCV32_LINUX_PROCESS_FLAG_GATE9_PARENT |
	    RISCV32_LINUX_PROCESS_FLAG_GATE10_PARENT)) != 0U) {
		riscv32_early_puts("[linux-signal] signal=");
		riscv32_early_putdec(delivered_signal);
		riscv32_early_puts(" rt_sigreturn pc=0x");
		riscv32_early_puthex32(restored_pc);
		riscv32_early_puts(" sp=0x");
		riscv32_early_puthex32(restored_sp);
		riscv32_early_puts(" cpu=0 PASS\n");
	}
	return TRUE;
}

boolean_t
riscv32_linux_signal_handle_user_exception(riscv_saved_state_t *state)
{
	uint32_t signal = riscv32_user_exception_pending_signal();

	if (signal == 0) {
		return FALSE;
	}

	riscv32_early_puts("[linux] fatal user signal=");
	riscv32_early_putdec(signal);
	if (state != (riscv_saved_state_t *)0) {
		riscv32_early_puts(" sepc=0x");
		riscv32_early_puthex32(state->exception.sepc);
		riscv32_early_puts(" stval=0x");
		riscv32_early_puthex32(state->exception.stval);
	}
	riscv32_early_puts("\n");
	riscv32_user_exception_clear_pending();

	for (;;) {
		__asm__ volatile("wfi");
	}
}
