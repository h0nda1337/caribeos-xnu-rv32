# Ciclo de vida de procesos Linux RV32

Estado: implementacion verificada hasta Gate 15, 2026-07-14, en UP y con dos
harts. PID 1 y Bash permanecen en CPU0; Gate 14 liga un hijo a CPU1.

## Creacion inicial

PID 1 se construye en `riscv32_linux_process_bootstrap_init()`:

```text
task_create_internal
  -> task->map->pmap Sv32
riscv32_linux_process_create(pid=1)
thread_create
riscv32_linux_process_attach_xnu
  -> task BSD info = process
  -> thread.machine.user_pmap = process pmap
preparar machine.iss
ligar thread a CPU0
NEW -> RUNNABLE
thread_resume
```

La entrada inicial pasa por el scheduler XNU y el retorno RV32 a U-mode; no
reutiliza el bootstrap thread del kernel.

## Fork

`riscv32_linux_exec_fork_current()` mantiene fallibles antes de crear el
thread siempre que es posible:

```text
crear task y pmap destino
crear process child y enlazarlo al parent
copiar personality y retener OFD
clonar paginas U-mode al pmap destino
crear thread
ligar process/task/thread/pmap
copiar registros; child a0 = 0
ligar a CPU0
NEW -> RUNNABLE
thread_resume
parent recibe PID
```

Si falla antes de `thread_create`, rollback devuelve backing, pmap, task, slot
y referencias. Despues de publicar un thread, un fallo se considera violacion
de invariantes y produce panic en vez de dejar un proceso medio visible.

## Exec

Exec conserva la estructura del proceso y el PID. La ruta transaccional crea
una imagen candidata, valida ELF/interpreter/stack, construye un `vm_map` y
pmap nuevo y solo entonces hace commit:

```text
preparar arena e imagen candidata
validar ELF, PT_INTERP, argv/envp y stack
crear mappings candidatos
verificar precondiciones del proceso
intercambiar task->map y process->pmap
actualizar thread.machine.user_pmap y registros
aplicar CLOEXEC y reset de estado de exec
liberar pmap/map/arena anterior
```

Un fallo previo al commit conserva la imagen vieja. Gate 6 verifica 1,000
commits fork-exec-exit-wait y un rollback deliberado.

## Exit

`exit` y `exit_group` comparten la ruta actual de proceso de un thread:

```text
RUNNING -> EXITING
cerrar fd y soltar OFD/pipe endpoints
registrar mmap/brk y contadores de teardown
desligar task/thread/pmap del process
armar notificacion de destruccion de pmap
retirar task BSD info
task_terminate_internal
publicar ZOMBIE o DEAD
marcar SIGCHLD pendiente y despertar al padre
soltar creator refs de task/thread
thread_terminate_self
```

No se usa `wfi`, no se estaciona CPU0 y no se reinicia el kernel. La
destruccion del pmap puede terminar despues de publicar el zombie; por eso
`pmap_release_sequence` separa identidad terminada de recursos VM ya
destruidos.

## Protocolo wait sin lost wakeup

El waiter no hace polling. El protocolo usa una secuencia monotona:

```text
lock tabla
  buscar hijo matching y zombie no claimed
  guardar parent.wait_sequence
unlock tabla

si no hay zombie y existe hijo:
  process RUNNING -> SLEEPING
  assert_wait(parent.wait_channel)
  barrera
  releer wait_sequence
  si no cambio: thread_block
  si cambio: clear_wait
  process SLEEPING -> RUNNABLE -> RUNNING
  repetir seleccion
```

Exit enlaza el zombie y aumenta `wait_sequence` bajo el lock de tabla antes de
`thread_wakeup`. Asi, o el waiter observa el cambio y cancela la espera, o ya
esta armado y recibe el wakeup.

Tras seleccionar un zombie, `wait_claimed` serializa la recoleccion. El reap
destructivo espera `pmap_release_sequence`, libera backing/arena, desenlaza y
retira el proceso. `WNOWAIT` copia `siginfo`, deshace el claim y conserva el
zombie.

Gate 7 registra y exige un numero no cero e identico de bloques y resumes XNU.
La evidencia UP y SMP mas reciente fue `21/21`.

## Reparenting

Cuando un proceso distinto de PID 1 publica exit con hijos:

1. Localiza PID 1 bajo el lock de tabla.
2. Retiene PID 1 una vez por hijo.
3. Cambia `parent` y `ppid` de cada hijo.
4. Mueve las listas de hijos y zombies.
5. Marca SIGCHLD pendiente en PID 1 si adopto zombies.
6. Despierta el wait channel de PID 1.
7. Suelta del padre saliente una referencia por hijo.

Gate 7 deja un hijo vivo al salir el padre, comprueba `ppid=1`, espera su exit
con status 55 y lo recolecta desde PID 1.

## Invariantes de teardown

* Un proceso `ZOMBIE` no tiene task, thread ni pmap ligados.
* No se libera backing antes de la notificacion de destruccion del pmap.
* Un proceso visible en tabla siempre conserva la referencia de tabla.
* Un hijo solo puede ser reaped por su padre actual.
* Un zombie claimed no puede ser seleccionado por otro waiter.
* Tras el ultimo reap del gate, procesos, tasks, threads, pmaps, OFD, backing y
  page tables regresan a su baseline.

## Senales y bloqueo

Exit publica la metadata SIGCHLD y el pending bit bajo `process->lock` antes
de despertar al padre. El wait channel de hijos y el evento del thread son
distintos: el primero anuncia un cambio de lifecycle; el segundo permite que
una senal con handler interrumpa un syscall bloqueado.

Una SIGCHLD con disposicion default no produce `EINTR`. El waiter vuelve a
seleccionar su hijo y lo recoge normalmente. Una senal no bloqueada con
handler de usuario si cancela `nanosleep`, `wait4` o `waitid`, devuelve
`EINTR`, y su frame se entrega en la salida comun hacia U-mode. El contrato
detallado esta en `docs/RV32_SIGNAL_ABI.md`.

## TTY y read bloqueante

La consola NS16550 publica entrada en un ring protegido por lock y una
secuencia monotona. `read(0)` usa `assert_wait` y `thread_block`, por lo que el
proceso pasa realmente a `SLEEPING` y no consume CPU mientras espera. IRQ RX,
cambios de termios y senales despiertan o cancelan el wait sin perder eventos.

Gate 10 verifica modo canonico, echo, backspace, reads parciales, `FIONREAD`,
`O_NONBLOCK`, `VMIN`, `VTIME` y `EINTR`. La descripcion completa esta en
`docs/RV32_TTY.md`.

## Bash, pipelines y job control

Gate 11 mantiene `/sbin/init` como PID 1, crea Bash como PID 2 y deja ambos
bajo el scheduler XNU. PID 1 bloquea en `waitid`; Bash bloquea en `pselect6`
cuando no hay entrada. Builtins modifican el estado persistente del shell y un
comando externo sigue el ciclo fork/exec/exit/wait antes de devolver el prompt.

Gate 12 convierte pipes en objetos bloqueantes con secuencias independientes
para readers y writers. El ultimo close publica EOF; escribir sin readers
encola SIGPIPE y la accion default termina al proceso. Gate 13 agrega
foreground pgrp, `SIGTSTP`, `SIGCONT`, `SIGINT`, `SIGQUIT` y observacion
`WUNTRACED`/`WCONTINUED`. Un stop conserva task, thread y pmap; solo exit entra
al teardown destructivo.

## Afinidad CPU1

Gate 14 crea el hijo desde Bash en CPU0 y lo liga al processor de CPU1 antes de
hacerlo runnable. El hijo entra a U-mode, hace exec, `getcpu`, mmap y exit en
CPU1. Su pmap solo mantiene activo el bit `0x2`; al destruirlo se completa el
shootdown antes de liberar backing. Exit despierta el wait de Bash en CPU0.
La suite de 10,000 ciclos repite este recorrido sin migrar PID 1 ni Bash.

## Ownership del kernel stack

`machine_stack_handoff()` debe transferir ownership exclusivo del stack. Si el
stack activo coincide con `old->reserved_stack`, intercambia tambien los
reserved stacks de los dos threads, como exige el scheduler XNU. Omitir ese
intercambio permitia que dos threads creyeran poseer el mismo stack y que una
reutilizacion sobrescribiera el contexto de Bash.

La ruta RV32 valida stack adjunto, `kstackptr`, saved RA/SP y continuation en
cada switch/load. Los handoffs reservados se cuentan atomicamente y se exponen
en `/proc/caribe/resources`. Gate 15 SMP avanzo de 2 a 1,976 handoffs durante
la matriz sin panic ni alias de stack.

## Gate 15

Despues de warmup se ejecutan 1,000 fork/exit/wait, 1,000
fork/exec/exit/wait, 1,000 pipes, 1,000 frames de senal, 100 pipelines, diez
lotes de diez hijos y 1,000 lineas TTY. El snapshot final debe igualar al
baseline para procesos live, tasks, threads, pmaps, page tables, paginas,
fd/OFD, pipes, frames, stacks y zones; `zombies=0`.
