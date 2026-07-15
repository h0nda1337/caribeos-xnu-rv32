# RV32 UART y TTY

Estado verificado: Gates 10, 11, 13 y 15 pasan en QEMU `virt` RV32 tanto UP
como con dos harts. La entrada viaja por el NS16550 emulado; no se sustituye
`read(0)` por datos sinteticos. Gate 15 inyecta bytes por el puerto serie TCP,
fuera del kernel, para repetir 1,000 lineas deterministas.

## Ruta de datos

```text
QEMU serial TCP
  -> NS16550 RX IRQ 10
  -> PLIC
  -> riscv32_tty_irq()
  -> line discipline
  -> input ring
  -> thread_wakeup(input_sequence)
  -> read/readv de la personalidad Linux
```

El driver toma base e IRQ del DTB conservado por CaribeBootX en bootinfo. El
FIFO se configura con trigger de un byte para que lineas cortas no dependan
del character-timeout del UART. La ISR tiene un presupuesto fijo, drena RX y
TX sin esperar activamente y nunca bloquea.

## Estado y locking

La terminal es un objeto global, igual que un unico dispositivo de consola,
pero sus lectores son procesos reales. `riscv32_tty_lock` protege:

* termios persistente de 36 bytes
* ring RX de 2 KiB y marcas de fin de linea
* buffer canonico de 256 bytes
* ring de echo TX de 512 bytes
* secuencia de wakeup y contadores

El lock se toma con interrupciones locales deshabilitadas porque tambien lo
atraviesa la ISR. El hart secundario puede observar el estado, pero userspace
y sus lecturas siguen ligados a CPU0.

## Disciplina inicial

El modo canonico implementa entrada, echo, erase/backspace, kill, EOF y
conversion CR a LF. Una linea se publica atomicamente al recibir Enter o EOF;
una lectura puede consumirla parcialmente y `FIONREAD` refleja el resto.

Con `ISIG`, VINTR, VQUIT y VSUSP se convierten respectivamente en SIGINT,
SIGQUIT y SIGTSTP para el foreground process group. Los caracteres no se
entregan como datos normales. `TIOCSPGRP` valida que el grupo pertenezca a la
sesion del caller.

El modo no canonico implementa los cuatro casos basicos de `VMIN`/`VTIME`:

* `VMIN > 0, VTIME = 0`: espera al minimo solicitado.
* `VMIN = 0, VTIME = 0`: retorna inmediatamente.
* `VMIN = 0, VTIME > 0`: timeout total en decimas de segundo.
* `VMIN > 0, VTIME > 0`: empieza el deadline al existir entrada parcial.

`O_NONBLOCK` tiene prioridad y retorna `EAGAIN`. Una senal entregable cancela
el wait XNU y retorna `EINTR`; el frame se construye en la salida comun hacia
U-mode.

## Protocolo de bloqueo

El lector captura `input_sequence`, cambia el proceso a `SLEEPING`, arma
`assert_wait` y vuelve a comparar despues de una barrera. Si IRQ, termios o
senal cambiaron el estado, cancela el wait; de lo contrario llama
`thread_block`. Al despertar restaura `RUNNABLE -> RUNNING` y reevalua la
condicion. No hay polling ni busy-wait.

## ABI visible

`/dev/tty`, `/dev/console` y los fd 0-2 de consola usan el mismo objeto. Estan
conectados:

* `read`, `readv`, `ppoll`
* `TCGETS`, `TCSETS`, `TCSETSW`, `TCSETSF`
* `FIONREAD`
* `TIOCGWINSZ`, `TIOCSWINSZ`
* `TIOCGPGRP`, `TIOCSPGRP`, `TIOCGSID`
* flags de open file description mediante `fcntl`
* `pselect6` bloqueante usado por Bash para esperar readiness

La salida normal de echo usa la cola TX y THRE IRQ. La salida temprana y de
panic conserva `early_print`, que escribe de forma independiente y no necesita
el lock del TTY.

## Gate 10

El fixture `/bin/process-tty-input` valida desde U-mode:

1. `TCGETS`, `FIONREAD` y un read no bloqueante vacio.
2. `caribx`, DEL, `e`, CR como la linea cocinada `caribe\n`.
3. Dos reads parciales con `FIONREAD=4` entre ambos.
4. Modo raw sin echo con `VMIN=2` y bytes `XY`.
5. `VMIN=0`, `VTIME=1` y retorno cero por deadline.
6. Read bloqueante cancelado por SIGUSR1 de un hijo real.
7. `rt_sigreturn`, wait/reap y restauracion exacta de termios.

El runner versionado `scripts/qemu-rv32-tty-test.ps1` conecta el puerto serie
TCP de QEMU, espera cada prompt y envia esos bytes. Las ejecuciones verificadas
produjeron:

```text
UP:  UART irq=8 rx-bytes=11; blocks/wakeups=4/4
SMP: UART irq=9 rx-bytes=11; blocks/wakeups=4/4
EAGAIN=1 EINTR=1 VTIME=1; overruns=0; echo=12
```

Comandos reproducibles:

```powershell
.\testproject.ps1 -Mode process-gate-10-only -Seconds 120
.\testproject.ps1 -Mode process-gate-10-only-smp -Seconds 140
```

## Bash y job control

Gate 11 deja Bash esperando en `pselect6` sin polling y comprueba que una linea
UART lo despierta, ejecuta el comando y vuelve al prompt. Gate 13 crea un job
foreground, prueba VSUSP -> stop -> `CLD_STOPPED`, SIGCONT ->
`CLD_CONTINUED`, VINTR -> status 130 y VQUIT -> status 131. Bash sobrevive y
recupera el foreground pgrp despues de cada job.

Gate 15 ejecuta `/bin/gate15-tty 1000`; el runner envia diez lotes de cien
lineas `L0000`..`L0999`. El fixture valida contenido y bloqueo canonico. El
snapshot final exige `tty_lines` +1,000, `tty_overruns` +0 y
`tty_read_blocks == tty_read_wakeups`.

## Limites actuales

* Un solo dispositivo TTY y una sola disciplina termios global.
* Sin flow control, modem control, paridad o errores detallados de linea.
* `poll` informa readiness, pero su espera con timeout aun es limitada.
* No hay multiplexacion de consolas ni hardware fisico.
* Un solo TTY, una sesion activa y sin PTY; job control no cubre multiples
  sesiones ni login terminals.
