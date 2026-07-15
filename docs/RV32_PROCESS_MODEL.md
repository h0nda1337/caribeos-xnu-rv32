# Modelo de procesos Linux RV32

Estado: Gates 1-15 verificados en QEMU `virt` RV32 el 2026-07-14. El modelo
soporta Bash 5.3.5 persistente e interactivo, pipelines, job control y un hijo
U-mode ligado a CPU1. No pretende ser todavia un `struct proc` Darwin completo
ni un subsistema Linux multiproceso sin limites.

## Relacion con XNU

Cada `riscv32_linux_process_t` vivo representa una identidad Linux y esta
ligado a objetos XNU reales:

```text
riscv32_linux_process_t
  -> pid/ppid y estado Linux
  -> task_t
     -> vm_map_t
        -> pmap Sv32 propio
  -> main_thread (thread_t)
     -> machine.iss (registros RV32 de usuario)
     -> machine.user_pmap
  -> personality (fd, cwd, umask, credenciales, senales, VM)
```

`riscv32_linux_process_current()` parte de `current_thread()`, consulta el
puntero BSD asociado al task y valida `magic` e insercion en la tabla. No usa
un PID global. La relacion se instala en
`riscv32_linux_process_attach_xnu()` y se retira antes de terminar el task.

La implementacion actual admite un thread principal por proceso. El modelo ya
guarda `thread_count` y `live_thread_count`, pero `CLONE_THREAD` y pthreads no
estan implementados.

## Identidad y estado

La tabla estatica tiene 16 slots. El PID se asigna desde 2, evita colisiones y
reinicia el cursor tras overflow. PID 1 se crea explicitamente como proceso,
task, thread y pmap de usuario.

Estados permitidos:

```text
NEW -> RUNNABLE -> RUNNING
RUNNING/RUNNABLE -> SLEEPING -> RUNNABLE -> RUNNING
RUNNING/RUNNABLE/SLEEPING/STOPPED -> EXITING
EXITING -> ZOMBIE -> DEAD
EXITING -> DEAD                    (sin padre)
```

Las transiciones se validan. `ZOMBIE` conserva PID, PPID, status, relacion de
parentesco y datos de diagnostico, pero ya no conserva `task_t`, `thread_t` ni
`pmap`. `DEAD` no puede permanecer en la tabla.

## Estado por proceso

`riscv32_linux_personality_state_t` contiene:

* Tabla de fd y stdio por proceso.
* `cwd`, nombre, prioridad, personality y `umask`.
* UID/GID real, efectivo, saved y filesystem.
* PGID y SID.
* Acciones, mascara, pending set y altstack de senales.
* Cursores y metadata de `brk` y `mmap`.

El estado de exec incluye path, argv/env/auxv derivados, arena de imagen y
generacion. El hijo de fork hereda una copia del personality, salvo pending
signals, que comienza vacio.

## Descriptores

La tabla de fd del proceso contiene IDs de open-file descriptions (OFD) y
flags de descriptor. El pool global de OFD contiene tipo, offset, status flags,
objeto subyacente y referencias. Por tanto:

* Fork copia la tabla y retiene cada OFD.
* Padre e hijo comparten offset cuando comparten OFD.
* `close()` elimina solo la entrada local y libera una referencia.
* `FD_CLOEXEC` pertenece a la entrada, no al OFD.
* Pipes conservan readers/writers mientras exista algun endpoint referenciado.

## Parentesco, zombies y wait

Cada padre mantiene `first_child` y `first_zombie`. Un hijo aparece una sola
vez en la lista de hijos; cuando publica exit tambien se enlaza en la lista de
zombies. `wait_claimed` impide que dos waiters recolecten el mismo zombie.

Estan implementados:

* `wait4(pid > 0)`, `wait4(-1)`, PID/PGID y `WNOHANG`.
* `waitid(P_PID)`, `waitid(P_ALL)`, `P_PGID`, `WEXITED` y `WNOWAIT`.
* `ECHILD`, status Linux y `siginfo` minimo de `CLD_EXITED`.
* Reparenting de hijos vivos o zombies a PID 1.
* Wakeup por canal XNU, sin sondeo de estado.

El pending bit de SIGCHLD se marca al publicar un zombie y se limpia cuando ya
no quedan zombies. Gates 8-9 entregan frames RV32 reales y `rt_sigreturn`;
Gates 12-13 agregan acciones default, grupos de proceso y eventos
`CLD_STOPPED`/`CLD_CONTINUED` sin cambiar el ownership del zombie.

## Referencias y ownership

Un proceso nuevo comienza con dos referencias:

1. Referencia de tabla.
2. Referencia de ciclo de vida.

Cada hijo retiene a su padre mientras la relacion existe. Lookup y seleccion
de wait toman referencias transitorias. Un reap correcto:

1. Desenlaza al hijo de `first_zombie` y `first_child`.
2. Retira su slot y cambia a `DEAD`.
3. Suelta las referencias de tabla y ciclo de vida.
4. El caller suelta la referencia transitoria de seleccion.

Al reparentar, cada hijo retiene primero a PID 1 y luego suelta exactamente una
referencia del padre saliente. El slot solo se borra cuando el contador llega a
cero y el proceso ya no esta en la tabla.

## Locks

* `riscv32_linux_process_table_lock`: tabla, slots, listas padre-hijo/zombie,
  claims y estadisticas globales.
* `process->lock`: estado y campos propios ligados a task/thread/pmap/exec.
* `riscv32_linux_ofd_pool_lock`: asignacion y referencias de OFD.
* Lock por OFD: offset y estado mutable de una open-file description.
* `riscv32_linux_pipe_pool_lock` y lock por pipe: allocation, endpoints, ring,
  secuencias de reader/writer y contadores de bloqueo.
* `riscv32_tty_lock`: termios, disciplina de linea, rings RX/TX, foreground
  pgrp y secuencia de readers; se toma con IRQ local deshabilitada.
* Lock del pool de backing: asignacion, owner PID y free stack de paginas.

La ruta que necesita ambos locks toma tabla antes que `process->lock`.
Transiciones ordinarias sueltan el lock del proceso antes de contabilizar bajo
el lock global. La version actual depende tambien de que cada proceso tenga un
solo thread de usuario; antes de pthreads habra que hacer atomica la copia de
fd y sus retains frente a closes concurrentes.

## Afinidad y SMP

PID 1, Bash y el userspace ordinario se ligan a `master_processor` (CPU0).
CPU1 permanece online para idle, timers, IPI, RFENCE y threads de kernel.
Gate 14 permite unicamente al hijo de prueba explicitamente ligado ejecutar en
CPU1; su pmap publica `active-mask=0x2`, hace exec y exit alli, y despierta el
wait de Bash en CPU0. No hay migracion arbitraria de userspace.

## Observabilidad

`/proc/caribe/resources` publica contadores live y acumulativos de procesos,
tasks, threads, pmaps, page tables, backing, arenas de exec, fd/OFD, pipes,
frames de senal, TTY, paginas fisicas, kernel stacks y zones. Tambien publica
`reserved_stack_handoffs`, que permite auditar el intercambio de stacks
reservados entre kernel threads sin logs ligados a un PID concreto.

Gate 15 toma un snapshot despues de warmup y otro al final. Los contadores live
deben ser identicos; los acumulativos deben ser monotonos y sus pares
alloc/free o create/destroy deben balancear exactamente.

## Limites actuales

* Capacidad simultanea estatica: 16 procesos, 16 fd generales y 512 IDs de
  pagina owned por proceso; la suite reutiliza slots despues de cada reap.
* Un thread por proceso.
* No hay COW. Fork usa copia ansiosa.
* `MAP_SHARED` general aun no tiene semantica de sharing durante fork.
* Signals realtime no tienen cola de multiples instancias y no hay frames
  anidados ni estado FPU/vectorial.
* Hay un solo TTY global y job control de una sesion; no hay PTY.
* La ejecucion CPU1 es por afinidad explicita de Gate 14, no scheduling Linux
  general ni migracion.
* No hay filesystem escribible, red, namespaces, ptrace ni pthreads completos.
