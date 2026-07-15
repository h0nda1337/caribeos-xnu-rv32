# RV32 Linux signal ABI

Estado verificado: Gates 8, 9, 12, 13 y 15 pasan en QEMU `virt` RV32, tanto UP
como con dos harts. PID 1 y Bash siguen en CPU0; el hijo ligado de Gate 14
tambien conserva estado de senales por proceso al ejecutar en CPU1.

## Ownership y locking

El estado de senales vive en `riscv32_linux_process_t`:

* `personality.sigactions[65]`
* `personality.signal_mask[16]`
* `personality.pending_signals[16]`
* `personality.sigaltstack[12]`
* `pending_signal_info[65]`
* `signal_frame_active` y `signal_frame_sp`
* contadores de queue, wakeup, delivery y return

Todos esos campos se leen o modifican bajo `process->lock`. La tabla de
procesos resuelve y retiene el destino antes de llamar
`riscv32_linux_signal_queue()`. El queue publica metadata y pending bit antes
de despertar el wait channel del proceso o el evento de su thread principal.
El wakeup no implica delivery: la seleccion final ocurre al volver de un
syscall/trap hacia U-mode.

La mascara y pending set reservan 128 bits por compatibilidad con el estado
existente. El ABI visible implementado acepta hasta la senal 64 y usa el
`kernel_sigset_t` RV32 de 8 bytes en `rt_sigaction`.

## rt_sigaction RV32

La cabecera que cruza copyin/copyout es ILP32:

```text
offset  size  field
0       4     handler
4       4     flags
8       4     restorer
12      8     mask
```

El kernel exige un restorer ejecutable cuando construye un frame. SIGKILL y
SIGSTOP no pueden recibir handlers. Se implementan `SA_SIGINFO`,
`SA_RESTORER`, `SA_ONSTACK` y `SA_NODEFER`. `SA_RESETHAND` general todavia no
esta completo.

## Frame de usuario

El frame mide 944 bytes y se alinea a 16 bytes:

```text
rt_sigframe = siginfo_t[128] + ucontext[816]

siginfo offsets usados:
  0   signo
  8   code
  12  sender pid
  16  sender uid
  20  child exit status

ucontext offsets desde el inicio de ucontext:
  8    stack_t (sp, flags, size; 12 bytes)
  20   saved signal mask (16 bytes conservados internamente)
  160  mcontext general registers

mcontext:
  gregs[0]     saved PC
  gregs[1..31] saved x1..x31
```

Al entrar al handler:

```text
ra = sa_restorer
sp = frame_sp
a0 = signal
a1 = &siginfo
a2 = &ucontext
pc = handler
```

Con `SA_ONSTACK`, el frame se crea en el extremo superior del altstack si el
thread no estaba ya dentro de ese rango. El rango completo debe ser writable
y caber debajo de `0x80000000`.

## rt_sigreturn

El syscall 139 toma el frame publicado para el proceso, no un puntero elegido
por el usuario. Copyin y validacion ocurren antes de modificar estado kernel.
Se validan:

* ownership y direccion exacta del frame activo
* PC no nulo, alineado y ejecutable en el pmap de usuario
* SP dentro de un mapping de usuario
* flags y rango de altstack sin overflow
* mascara restaurada, eliminando siempre SIGKILL y SIGSTOP

Se restauran PC, SP, x1..x31, mascara y altstack. No se acepta ni se restaura
`sstatus` desde el frame, por lo que userspace no puede seleccionar bits de
privilegio. `SS_ONSTACK` se limpia al completar el retorno.

Solo puede existir un frame activo por proceso en esta fase. Las senales
anidadas se posponen hasta que termine `rt_sigreturn`.

## EINTR y waits

`riscv32_linux_signal_has_deliverable()` distingue pending de deliverable. Una
SIGCHLD default puede despertar un wait por cambio de hijo sin convertir el
wait en `EINTR`. Un handler de usuario pendiente si interrumpe:

* `nanosleep`
* `wait4`
* `waitid`
* reads TTY y `pselect6`

El chequeo se repite despues de armar el wait para cerrar la carrera entre
publicar pending y dormir. Gate 10 verifica `read(0)` bloqueado -> senal ->
`EINTR` -> frame -> `rt_sigreturn`.

## Acciones default y job control

La implementacion actual cubre las acciones default necesarias para Bash:

* SIGPIPE termina al writer cuando el ultimo reader cerro.
* SIGINT y SIGQUIT terminan el foreground process group seleccionado por TTY.
* SIGTSTP cambia el proceso a `STOPPED` sin liberar task/thread/pmap.
* SIGCONT publica `CLD_CONTINUED`, vuelve el proceso runnable y despierta wait.
* SIGCHLD default no interrumpe wait; la metadata de hijo queda observable.

`kill()` acepta PID y process group para el modelo single-session actual.
`waitid` distingue `CLD_EXITED`, `CLD_STOPPED` y `CLD_CONTINUED`. Gate 13
prueba un stop/continue/interrupt y un SIGQUIT independiente, con status Bash
130 y 131 respectivamente.

## Evidencia automatizada

Gate 8 verifica SIGCHLD bloqueada, `rt_sigpending`, handler con siginfo y
ucontext, `rt_sigreturn`, wait y teardown. Gate 9 verifica tres entregas de
SIGUSR1: despues de `kill(self)`, durante un deadline sleep y durante un wait.
Las ultimas dos deben despertar el target y devolver `EINTR`.

```powershell
.\testproject.ps1 -Mode process-gate-8-only -Seconds 100
.\testproject.ps1 -Mode process-gate-8-only-smp -Seconds 110
.\testproject.ps1 -Mode process-gate-9-only -Seconds 100
.\testproject.ps1 -Mode process-gate-9-only-smp -Seconds 110
```

Gate 9 exige exactamente tres queues, tres frames, tres returns y dos target
wakeups. Tambien exige que los tres frames usen el altstack, que los returns
recuperen el SP normal y que todos los contadores de proceso, task, thread,
pmap, fd y memoria regresen al baseline.

Gate 15 entrega 1,000 SIGUSR1 con `SA_SIGINFO|SA_ONSTACK`, exige 1,000 handlers
y compara `signal_delivered == signal_returned`, `signal_frames=0` y
`signal_failures=0` al final en UP y SMP.

## Limites actuales

* Las acciones default implementadas son el subconjunto usado por pipes, TTY y
  job control; no existe aun la tabla POSIX completa.
* SIGKILL/SIGSTOP y thread-directed IDs necesitan mas cobertura, aunque
  `tkill`/`tgkill` soportan el caso de un thread por proceso.
* No hay realtime signal queue con multiples instancias de la misma senal.
* No hay frames anidados, FPU state ni extension vectorial.
* Hay una sola sesion/TTY; no hay PTY ni multiples sesiones concurrentes.
* Los handlers ordinarios se ejecutan en CPU0; no se ha habilitado migracion
  arbitraria de procesos con frames activos.

Referencias de ABI:

* Linux RISC-V `arch/riscv/kernel/signal.c`
* Linux RISC-V UAPI `asm/ucontext.h` y `asm/ptrace.h`
* musl `src/internal/ksigaction.h` y `src/signal/sigaction.c`
