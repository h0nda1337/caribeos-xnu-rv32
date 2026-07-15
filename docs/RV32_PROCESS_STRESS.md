# Pruebas de procesos RV32

Estado: Gates 1-15 automatizados. Gate 14 completo 10,000 lifecycles ligados a
CPU1; Gate 15 completo la matriz general en UP y SMP el 2026-07-14.

## Runner

Desde el arbol CaribeOS en `F:`:

```powershell
.\testproject.ps1 -Mode process-gates -Seconds 60
.\testproject.ps1 -Mode interactive -Seconds 120
.\testproject.ps1 -Mode process-stress -Seconds 300 -StressRounds 10000
.\testproject.ps1 -Mode process-stability -Seconds 90
.\testproject.ps1 -Mode all -Seconds 300
```

`process-gates` recorre Gates 1-15 usando los modos aislados de 7-10 para no
repetir innecesariamente estreses previos. `process-stress` conserva la prueba
CPU1 de Gate 14. `process-stability` ejecuta Gate 15 con harts=1 y harts=2.
`all` agrega UP, SMP A-F y las suites anteriores.

El runner religa primero el XNU actual, actualiza `/kernel.elf` en HFS+,
reconstruye initrd cuando corresponde y aplica aserciones al serial log. No
acepta un `PASS` visual como sustituto de contadores.

## Gate 14 CPU1

Cada ciclo hace fork desde el driver CPU0, entra a U-mode en CPU1, exec,
`getcpu`, exit en CPU1 y reap en CPU0. La prueba de 10,000 ciclos exige:

```text
fork=entry=exec=getcpu=exit=reap=10000
pmap-create/destroy=19998/19998
backing-alloc/free=69993/69993
free-pages, wired-pages, stacks y zones en baseline
kernel-pt-growth=0
stale=0, overlap=0, errors=0
```

Los rounds aceptados son 100, 1,000 y 10,000; el de 10,000 es el gate de
regresion estable.

## Gate 15 matriz

Despues de un warmup corto, Bash ejecuta:

* 1,000 fork/exit/wait.
* 1,000 fork/exec/exit/wait.
* 1,000 pipes con alloc, close, read y write.
* 1,000 entregas SIGUSR1 con siginfo, altstack y `rt_sigreturn`.
* 100 pipelines de dos procesos y 1,228,800 bytes.
* Diez lotes de diez hijos, 100 lifecycles totales y cero zombies.
* 1,000 lineas canonicas TTY inyectadas por serial TCP.

Las siete invocaciones externas de la matriz hacen que Bash cree siete pipes
de sincronizacion adicionales. Por eso el delta exacto es 1,107 pipes y
1,292,800 bytes, no 1,100.

## Snapshots

`/proc/caribe/resources` se lee una vez tras warmup y una al final. Deben ser
identicos:

```text
process_capacity/entries/live, zombies
tasks, threads, pmaps, pt_pages, kernel_pt_pages
user_pages, exec_arenas
fd_refs, ofds, pipes, signal_frames
free_pages, wired_pages
kernel_stacks_active/total
zones, zone_active, zone_bytes
```

Los pares acumulativos deben ser monotonos y balancear: process
create/remove/exit/reap, pmap create/destroy, PT alloc/free, backing
alloc/free, OFD alloc/free/retain/release, pipe alloc/release y signal
deliver/return. Tambien se exige `zombies=pipes=signal_frames=0`,
`signal_failures=0`, `tty_overruns=0` y bloques igual a wakeups.

## Defecto encontrado

La repeticion inicial de Gate 15 encontro una corrupcion intermitente del stack
de Bash. `machine_stack_handoff()` transferia `kernel_stack`, pero no
intercambiaba `reserved_stack` cuando ambos eran el mismo objeto. Dos threads
podian conservar ownership logico del mismo stack y una reanudacion escribia
un trampoline con continuation nula sobre el contexto de Bash.

La correccion porta la semantica XNU de reserved-stack swap, valida cada
contexto antes de switch/load y publica `reserved_stack_handoffs`. Cuatro boots
UP consecutivos, un boot SMP directo y `process-stability` completo pasaron.
En el ultimo SMP el contador avanzo de 2 a 1,976 durante la matriz.

## Evidencia archivada

```text
caribeos/build/process-gates/process-gate-14-userspace-cpu1.txt
caribeos/build/process-gates/process-stress-latest.txt
caribeos/build/process-gates/process-gate-15-stability-up.txt
caribeos/build/process-gates/process-gate-15-stability-smp.txt
caribeos/build/testproject-process-stability-run.txt
```

Los logs historicos del panic y las cuatro repeticiones UP se conservan en
`caribeos/build/gate15-*.serial.log`. `-KeepHistory` agrega copias timestamped.

## Limites

La suite es intensiva y determinista, no una prueba de duracion ilimitada ni
un fuzzer. Cubre dos harts como maximo, un thread Linux por proceso, initrd
read-only y una sola TTY. No demuestra COW, red, filesystem escribible, PTY,
hotplug ni migracion arbitraria de userspace.
