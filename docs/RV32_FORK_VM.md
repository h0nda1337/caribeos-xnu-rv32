# Fork y VM Sv32 en RV32

Estado: copia ansiosa funcional, verificada hasta Gate 15 y en el recorrido
CPU1 de Gate 14. Copy-on-write no esta implementado.

## Espacio virtual

El pmap RV32 usa Sv32 con esta division:

```text
0x00000000-0x7fffffff  usuario, privado por task
0x80000000-0xffffffff  kernel, compartido y supervisor-only
```

`riscv32_pmap_clone_user_eager()` recorre solamente root entries inferiores a
`RISCV32_KERNEL_ROOT_INDEX`. Copia solo hojas con `PTE_U`; no copia mappings de
kernel, firmware ni entradas supervisor-only.

## Algoritmo actual

Para cada pagina visible a U-mode del padre:

1. Obtiene VA, pagina fisica fuente y permisos R/W/X.
2. Reserva una pagina del pool de backing de fork.
3. Registra owner PID en el pool.
4. Copia 4096 bytes desde la pagina fuente por su KVA.
5. Crea la PTE destino con los mismos permisos de usuario.
6. Registra el ID de backing en el proceso hijo.

Al final publica `resident_count` y una barrera de memoria. El pmap destino
debe estar inactivo y vacio al empezar. Su mascara activa es independiente.

## Registros y metadata

El child thread recibe una copia de `riscv_saved_state_t` y `a0=0`; el padre
recibe el PID. El PC y SP vuelven a la instruccion posterior al syscall.

El personality copia:

* Stack, segmentos ELF, heap/brk y paginas mmap mediante el recorrido PTE.
* Metadata de `brk` y `mmap` mediante copia de estado.
* CWD, umask, credenciales, PGID/SID y signal dispositions.
* Tabla de fd, reteniendo las mismas OFD.

Pending signals se vacia para el hijo. El exec descriptor se adopta del padre.

## Ownership y rollback

Cada proceso puede registrar hasta 512 IDs de backing owned. La asignacion usa
lock, free stack, owner PID y contadores de alloc/free/high-watermark. Liberar
una pagina valida tanto el ID como el owner; un double free o owner incorrecto
produce panic.

Si la copia falla antes de crear el thread, rollback:

```text
libera paginas ya copiadas
retira process child y referencias
destruye task/map/pmap destino
devuelve page tables
```

Al exit, el pmap se destruye primero de forma XNU. El waiter espera la
notificacion `pmap_release_sequence` antes de devolver las paginas de backing,
evitando que una PTE activa apunte a memoria reutilizada.

## Pruebas

Gate 4 verifica 1,000 forks con memoria privada y status correcto. Gate 6
verifica 1,000 fork-exec-exit-wait. Gate 7 mantiene diez hijos simultaneos y
14 forks totales; la corrida aislada actual reporta:

```text
35 paginas privadas por hijo
490 paginas clonadas acumuladas
350 paginas de backing simultaneas en el pico
15 pmaps creados y destruidos
45 page-table pages devueltas
0 paginas en uso al terminar
```

Gate 14 repite 10,000 ciclos fork/exec/exit/wait con el hijo ligado a CPU1.
Reporta `pmap-create/destroy=19998/19998`,
`backing-alloc/free=69993/69993`, page tables y paginas fisicas en baseline y
`stale=0`. Gate 15 agrega la matriz general y termina con
`backing_alloc-backing_free=0`, pmaps y PT alloc/free balanceados tanto en UP
como SMP.

## Limitaciones antes de COW

* El costo es O(paginas residentes) por fork.
* El limite de 512 IDs owned restringe procesos grandes y concurrencia.
* El pool limita el numero de hijos grandes simultaneos.
* Todas las hojas U-mode se copian; `MAP_SHARED` general debe distinguirse y
  referenciar backing compartido antes de declarar semantica completa.
* No hay dirty/accessed-bit preservation explicita; se regeneran permisos.
* No hay overcommit ni page faults COW.

La siguiente evolucion VM debe introducir un objeto de backing referenciado y
PTE read-only/COW con shootdown remoto, sin debilitar la copia ansiosa como
fallback de diagnostico.
