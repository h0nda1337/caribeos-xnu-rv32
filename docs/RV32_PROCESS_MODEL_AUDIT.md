# Auditoria del modelo de procesos RV32

Estado auditado: Tranche 196, 2026-07-13.

Actualizacion posterior: Gates 1-15 quedaron implementados y verificados el
2026-07-14, incluyendo Bash interactivo, pipes bloqueantes, job control,
userspace ligado a CPU1 y estabilidad UP/SMP. Esta auditoria se conserva como
fotografia pre-implementacion; el contrato vigente esta en
`RV32_PROCESS_MODEL.md`, `RV32_PROCESS_LIFECYCLE.md`, `RV32_FORK_VM.md`,
`RV32_SIGNAL_ABI.md`, `RV32_TTY.md`, `RV32_BASH_INTERACTIVE.md` y
`RV32_PROCESS_STRESS.md`.

Esta auditoria describe el modelo que existe antes de introducir procesos Linux
reales. El codigo del arbol es la fuente de verdad. En particular, no se debe
confundir la ejecucion real de ELF en U-mode con la existencia de un proceso:
el primer caso ya funciona, el segundo todavia no existia al iniciar esta fase.

## Clasificacion

Se usan estas categorias:

* `ya ligado a task/thread`: el ownership ya sigue una entidad XNU real.
* `global de stage0`: una sola instancia estatica compartida por todo userspace.
* `sintetico`: emula semantica sin crear el objeto de kernel correspondiente.
* `reutilizable`: mecanismo util para el modelo nuevo, aunque necesite una API.
* `debe reemplazarse`: impide procesos concurrentes o un ciclo de vida correcto.
* `puede posponerse`: no bloquea los primeros gates si se documenta el limite.

## Ciclo de vida actual

La ruta de arranque auditada es:

```text
kernel_bootstrap_thread (kernel_task)
  -> bsd_init() debil
  -> riscv32_linux_exec_umode_smoke()
  -> pmap_create() sin task propietario
  -> carga ELF y stack desde pools globales
  -> pmap_switch(user_pmap)
  -> riscv32_return_to_user(&riscv32_linux_umode_state)
  -> traps sobre el mismo kernel stack del bootstrap thread
  -> execve reemplaza los pools globales en el mismo pmap
  -> exit/exit_group imprime el status y ejecuta wfi para siempre
```

Por tanto, Bash corre en U-mode real, pero su contexto XNU sigue siendo el
`kernel_bootstrap_thread` de `kernel_task`. No hay un `task_t` de usuario, el
pmap no pertenece a `task->map`, y PID 1 solo es una convencion del personality.

## Inventario auditado

| Elemento | Ubicacion | Estado actual | Clasificacion | Accion |
| --- | --- | --- | --- | --- |
| Creacion inicial U-mode | `osfmk/riscv32/full_stage0_stubs.c:241`, `linux_exec.c:726` | `bsd_init()` llama directamente al loader y este hace `sret` | `debe reemplazarse` | Crear task/thread PID 1 y dejar que el scheduler lo despache |
| Pmap inicial | `linux_exec.c:743` | `pmap_create()` suelto, no incluido en un `vm_map_t` de task | `debe reemplazarse` | Usar `task_create_internal()` y `task->map->pmap` |
| Thread inicial | `osfmk/kern/startup.c:421` | El thread bootstrap del kernel entra a U-mode | `debe reemplazarse` | Crear un `thread_t` no-kernel y fijarlo a CPU0 |
| `current_thread()` | `osfmk/riscv32/cpu_data.c`, `riscv32_thread.c` | Es por CPU y se actualiza en switch/load context | `ya ligado a task/thread` | Reutilizar como llave para encontrar el proceso |
| Copyio | `osfmk/riscv32/copyio.c:21` | Prefiere pmap activo, luego `thread->machine.user_pmap`, luego `thread->map->pmap` | `reutilizable` | Al crear cada thread, asignar el pmap de su task |
| Registros iniciales | `linux_exec.c:763` | `riscv32_linux_umode_state` global | `global de stage0` | Construirlos en `thread->machine.iss` |
| Trap frame | `osfmk/riscv32/trap_entry.s` | Frame de 176 bytes en el kernel stack actual | `ya ligado a task/thread` | Mantener; sincronizar con `machine.iss` al bloquear desde syscall |
| Retorno a U-mode | `osfmk/riscv32/user_return.s` | `riscv32_return_to_user()` restaura un saved state y hace `sret` | `reutilizable` | Implementar `thread_bootstrap_return()` RV32 fuerte |
| Retorno XNU generico | `full_stage0_stubs.c:354` | `thread_bootstrap_return` y `thread_exception_return` estacionan | `debe reemplazarse` | Entrada inicial fuerte; no usar estado obsoleto al volver de traps |
| Loader ELF | `linux_exec.c` | Valida ELF32, carga segmentos, interpreter y stack | `reutilizable` | Separar plan, backing y mapeo por proceso |
| `execve`/`execveat` | `linux_syscall.c`, `linux_exec.c:779` | Reusa el pmap y los pools globales; no es transaccional | `debe reemplazarse` | Preparar imagen nueva y hacer commit/rollback por proceso |
| Informacion de exec | `linux_exec.c` | Un `riscv32_linux_current_process` global | `global de stage0` | Mover argv/env/auxv/path al proceso |
| Paginas ELF | `linux_exec.c:29` | 512 paginas globales para imagen | `global de stage0` | Introducir objetos/backings con owner y liberacion |
| Paginas interpreter | `linux_exec.c:32` | 512 paginas globales | `global de stage0` | Igual que imagen ELF |
| User stack | `linux_exec.c:35` | 32 paginas globales, 128 KiB | `global de stage0` | Stack por proceso y contador de ownership |
| Tabla fd | `linux_syscall.c:455` | 3 entradas stdio y 16 descriptores globales | `global de stage0` | Tabla por proceso; mas adelante separar open-file descriptions |
| Pipes | `linux_syscall.c:457` | Pool global de 8 objetos de 4096 bytes | `global de stage0`, `reutilizable` | Mantener objetos globales con refs; fd ownership por proceso |
| I/O de pipes | `linux_syscall.c` | Almacenamiento real, pero no hay espera bloqueante | `debe reemplazarse` | Wait channels y wakeups; eliminar respuestas por polling |
| CWD | `linux_syscall.c:473` | Buffer global de path | `global de stage0` | Campo por proceso |
| Umask | `linux_syscall.c:478` | Escalar global | `global de stage0` | Campo por proceso |
| Credenciales | `linux_syscall.c:479` | UID/GID efectivos/guardados/fs globales | `global de stage0` | Campos por proceso |
| Senales | `linux_syscall.c:459`, `:490` | Actions, mask y altstack globales; entrega pasiva | `global de stage0` | Estado y pending mask por proceso; frame real puede posponerse |
| PGID/SID | `linux_syscall.c:488` | Valores globales inicializados a 1 | `global de stage0` | Campos por proceso |
| Nombre/prioridad/personality | `linux_syscall.c:475` | Estado global | `global de stage0` | Campos por proceso |
| Heap/brk | `linux_syscall.c:466` | 512 paginas y cursores globales | `global de stage0` | Metadata/backing por proceso |
| mmap | `linux_syscall.c:462` | 256 paginas y metadata globales | `global de stage0` | Metadata por proceso y backing con owner/ref |
| Buffers `/proc` | `linux_syscall.c:492` | Scratch global para formatear archivos | `global de stage0`, `puede posponerse` | Serializar temporalmente; luego buffer por lectura |
| `/proc/self/status` | `linux_syscall.c:2731` | `Pid: 1`, `PPid: 0` codificados | `sintetico` | Consultar proceso ligado a `current_thread()` |
| `getpid/gettid/getppid` | `linux_syscall.c:9290` | Devuelven constantes | `sintetico` | Consultar proceso/thread actuales |
| Child records | `linux_syscall.c:458`, `:8039` | 8 registros `{used,pid,pgid,status}` | `sintetico` | Sustituir por relacion padre-hijo y zombies reales |
| `clone()` | `linux_syscall.c:8267` | Reserva PID y crea child record ya terminado | `sintetico`, `debe reemplazarse` | No exponer fork hasta crear task/thread/pmap reales |
| `wait4()` | `linux_syscall.c:8297` | Consume child record; ECHILD o WNOHANG | `sintetico`, `debe reemplazarse` | Buscar hijos/zombies y bloquear en wait channel |
| `waitid()` | `linux_syscall.c` | Variante sobre el mismo array sintetico | `sintetico`, `debe reemplazarse` | Misma maquina de estados real |
| `exit()`/`exit_group()` | `linux_syscall.c:9057` | Log y bucle infinito `wfi` | `debe reemplazarse` | Marcar zombie, despertar padre y terminar thread XNU |
| Tabla de procesos | inexistente | Solo convenciones y child records | `debe reemplazarse` | Tabla central con refs, locks, estados y contadores |
| Task teardown | `osfmk/kern/task.c` | XNU tiene APIs de terminacion/dealloc | `reutilizable` | Integrar despues de separar zombie de recursos vivos |
| Thread teardown | `osfmk/kern/thread.c:317` | `thread_terminate_self()` real | `reutilizable` | Invocar sin volver al trap frame del proceso muerto |
| Afinidad CPU | `osfmk/kern/startup.c`, SMP Gate F | Userspace se afirma en CPU0 | `ya ligado a task/thread` | Fijar todos los user threads al master processor |
| Pmap activo/RFENCE | `osfmk/riscv32/pmap.c` | Mascara activa y shootdown remoto verificados | `reutilizable` | Quitar pmap de CPUs antes de destruirlo |

## Pools estaticos y limites

| Pool | Capacidad | Ownership actual | Riesgo multiproceso |
| --- | ---: | --- | --- |
| Imagen ELF | 512 paginas | Global | Un exec pisa la imagen de otro proceso |
| Interpreter | 512 paginas | Global | Igual; no hay referencias |
| Stack U-mode | 32 paginas | Global | Padre e hijo compartirian memoria writable |
| mmap | 256 paginas | Global | Metadata y contenido se mezclan entre procesos |
| brk | 512 paginas | Global | Un solo cursor y backing |
| fd >= 3 | 16 entradas | Global | close/offset/flags afectan a todos |
| stdio | 3 entradas | Global | dup/close afectan a todos |
| pipes | 8 objetos | Global | El buffer sirve, pero faltan refs robustas y waiters |
| children | 8 registros | Global | No representa tasks ni ejecucion |
| signal actions | 65 entradas | Global | Un exec/fork altera el estado de todos |
| proc scratch | varios buffers | Global | Lecturas concurrentes corromperian resultados |

Los pools de ELF, interpreter, stack, mmap y brk se almacenan en BSS del kernel.
No existe hoy un camino completo que desmapee, destruya y contabilice esos
recursos al salir un proceso.

## Suposiciones de proceso unico

1. El pmap activo identifica implicitamente a todo Linux userspace.
2. Toda syscall consulta globals, nunca `current_thread()->task`.
3. `execve()` puede reutilizar los mismos buffers sin excluir a otro proceso.
4. Cualquier descriptor numerico se resuelve en una unica tabla global.
5. CWD, credenciales, mascara de senales, PGID y SID solo tienen una instancia.
6. Todo `/proc/self` significa PID 1.
7. El unico stack de usuario puede volver a mapearse en cada exec.
8. Salir equivale a terminar toda actividad de usuario, por eso `wfi` parecia
   suficiente para el smoke guionizado.

## Objetos XNU reutilizables

* `task_create_internal(TASK_NULL, FALSE, FALSE, &task)` crea un `vm_map_t` y
  un pmap de usuario sin herencia.
* `thread_create(task, &thread)` crea un thread suspendido, su kernel stack al
  despacharlo y un `machine.iss` RV32 propio.
* `set_bsdtask_info(task, process)` permite resolver el proceso desde el task.
* `thread->machine.user_pmap` y `thread->map->pmap` ya sirven a copyio.
* `thread_resume()` y el scheduler XNU pueden ejecutar el thread en CPU0.
* `thread_terminate_self()` termina el thread actual sin estacionar el hart.
* `pmap_switch`, mascara de pmap activa, RFENCE y `sfence.vma` ya pasaron SMP
  Gate E.
* El parser/cargador ELF, initrd, auxv y stack builder son reutilizables si se
  les da backing y contexto explicitos.

## Invariantes del modelo nuevo

1. Un proceso `RUNNABLE` o `RUNNING` tiene `task`, `main_thread` y `pmap`
   validos; su pmap es exactamente `task->map->pmap`.
2. `task->bsd_info` apunta al proceso mientras el task pueda originar syscalls.
3. Cada user thread tiene `machine.user_pmap == thread->map->pmap`.
4. Todo user thread queda ligado al processor de CPU0 en esta fase.
5. PID, parent y estado se leen bajo el lock de procesos o mediante una
   referencia estable.
6. Un zombie conserva identidad, parent y exit status, pero no recursos de
   ejecucion costosos.
7. El padre reapea exactamente una vez; despues el proceso pasa a `DEAD`.
8. Ningun camino normal de `exit`, `wait`, pipe o TTY usa busy-wait o `wfi`.
9. El kernel pmap nunca se destruye ni se trata como pmap de proceso.
10. El ultimo gate estable sigue siendo arrancable si un gate posterior falla.

## Orden de migracion

1. Introducir tabla central, estados, PID allocator, referencias, relaciones y
   dump sin cambiar todavia el ABI observable.
2. Crear PID 1 con `task_t`, `thread_t`, `task->map->pmap` y `machine.iss`
   propios; asociarlo con `task->bsd_info`.
3. Mover fd table, cwd, umask, credenciales, senales, PGID/SID y metadata VM a
   un contexto por proceso. Los objetos pipe pueden seguir en un pool global
   con referencias.
4. Hacer `/proc/self`, getpid/getppid/gettid y diagnostico consultar el proceso
   actual.
5. Verificar Gate 1 y todas las regresiones antes de cambiar `exit`.
6. Implementar teardown/zombie/wakeup y Gate 2.
7. Crear una primitiva interna de segundo task/thread y Gate 3.
8. Solo entonces sustituir `clone()` por fork real y duplicar memoria.

## Deuda que puede posponerse

No bloquean Gate 1-3 si quedan explicitamente aislados:

* Copy-on-write; la primera duplicacion puede ser ansiosa.
* Threads Linux creados con `CLONE_THREAD`.
* Signal frame completo y `rt_sigreturn`.
* Job control y TTY canonica completa.
* Userspace en CPU1.
* Open-file descriptions generales; para Gate 1 basta ownership por proceso,
  pero deben separarse antes de fork/dup concurrente robusto.
* Escritura persistente, red, IOKit completo y hardware fisico.

## Criterio de honestidad

Gate 1 no significa fork. Un segundo PID en una tabla no significa un segundo
proceso ejecutable. Gate 2 no significa wait funcional. SMP de kernel no
significa userspace SMP. Cada log debe decir si observa una estructura,
ejecucion U-mode, bloqueo real, wakeup, teardown o coherencia de memoria.
