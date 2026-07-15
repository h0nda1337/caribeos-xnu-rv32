# GNU Bash interactivo en XNU-CaribeOS RV32

Estado verificado: GNU Bash 5.3.5 RV32-musl funciona como proceso persistente
sobre el TTY NS16550. Gates 11-15 pasan en UP y con dos harts de kernel.

## Cadena de arranque

```text
OpenSBI (M-mode)
  -> CaribeBootX (S-mode, virtio-blk, HFS+)
  -> /kernel.elf XNU-CaribeOS
  -> initrd /sbin/init, PID 1
  -> fork/exec /bin/bash, PID 2
  -> prompt bash-5.3$ por UART
```

PID 1 y Bash son tasks/threads XNU reales con pmaps Sv32 propios. PID 1 crea
una sesion, adquiere el TTY, liga Bash a CPU0 y bloquea en `waitid` sin polling.
Bash usa `pselect6` para esperar entrada y permanece vivo entre comandos.

## Estado persistente

Gate 11 prueba que builtins modifican el mismo proceso Bash:

* `pwd`, `cd /usr` y segundo `pwd` observan CWD persistente.
* Una variable shell conserva su valor entre lineas.
* `true`, `false` y `$?` conservan status real.
* `exit 0` termina PID 2; PID 1 lo reapea y sale limpio.

Un comando externo crea un hijo real, hace exec transaccional, exit y wait. El
prompt solo reaparece despues del reap; el parent no consulta en busy loop.

## Pipelines y redirecciones

Gate 12 cubre:

```text
producer | consumer
consumer < archivo-initrd
producer --stderr 2>&1
writer sin readers -> SIGPIPE
pipeline repetido -> recursos en baseline
```

Los pipes tienen ring de 4 KiB, endpoints referenciados, bloqueo XNU para
reader/writer, EOF al cerrar el ultimo writer y wakeups con secuencia. La
transferencia de 12,288 bytes fuerza tanto pipe lleno como vacio y verifica
checksum, no solo un mensaje corto.

## Job control

Bash administra SID, PGID y foreground pgrp mediante la ABI TTY. Gate 13
ejecuta un proceso foreground que bloquea en read y prueba:

```text
Ctrl+Z -> SIGTSTP -> STOPPED -> CLD_STOPPED
SIGCONT al process group -> RUNNABLE -> CLD_CONTINUED
Ctrl+C -> SIGINT -> status 130
Ctrl+\ -> SIGQUIT -> status 131
```

El shell recupera el foreground pgrp y vuelve al prompt. Stop/continue no
libera la VM; exit si entra al teardown normal.

## Afinidad

Bash y PID 1 permanecen ligados a CPU0. Gate 14 permite que un hijo explicito
ejecute U-mode y exec en CPU1, termine alli y despierte el wait de Bash en
CPU0. Esto prueba cruce de CPU para lifecycle y pmap, no migracion arbitraria
del shell.

## Runner

```powershell
.\testproject.ps1 -Mode interactive -Seconds 120
.\testproject.ps1 -Mode process-gate-12 -Seconds 180
.\testproject.ps1 -Mode process-gate-13 -Seconds 180
.\testproject.ps1 -Mode process-stability -Seconds 90
```

El runner controla QEMU por serial TCP, espera patrones antes de enviar cada
linea o caracter de control y falla ante prompt prematuro, panic, trap, status
incorrecto, zombies o drift de recursos. Los logs canonicos viven en
`caribeos/build/process-gates/`.

## Limites

* Un solo TTY y una sesion de login; no hay PTY.
* Filesystem initrd read-only y HFS+ usado por el bootloader, no como root
  writable de userspace.
* No hay red ni procesos en background de larga duracion probados.
* Un thread Linux por proceso y sin pthreads completos.
* No hay migracion libre de Bash entre CPUs.
