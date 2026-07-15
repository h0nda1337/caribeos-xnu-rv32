#ifndef _BSD_RISCV32_PARAM_H_
#define _BSD_RISCV32_PARAM_H_

#include <riscv32/_param.h>

#define ALIGNBYTES	__DARWIN_ALIGNBYTES
#define ALIGN(p)	__DARWIN_ALIGN(p)

#define NBPG		4096
#define PGOFSET		(NBPG - 1)
#define PGSHIFT		12

#define DEV_BSIZE	512
#define DEV_BSHIFT	9
#define BLKDEV_IOSIZE	2048
#define MAXPHYS		(128 * 1024)

#define CLSIZE		1
#define CLSIZELOG2	0

#define MSIZESHIFT	8
#define MSIZE		(1 << MSIZESHIFT)
#define MCLSHIFT	11
#define MCLBYTES	(1 << MCLSHIFT)
#define MBIGCLSHIFT	12
#define MBIGCLBYTES	(1 << MBIGCLSHIFT)
#define M16KCLSHIFT	14
#define M16KCLBYTES	(1 << M16KCLSHIFT)

#define MCLOFSET	(MCLBYTES - 1)
#ifndef NMBCLUSTERS
#define NMBCLUSTERS	((1024 * 512) / MCLBYTES)
#endif

#define ctos(x)		(x)
#define stoc(x)		(x)
#define ctod(x)		((x) << (PGSHIFT - DEV_BSHIFT))
#define dtoc(x)		((x) >> (PGSHIFT - DEV_BSHIFT))
#define dtob(x)		((x) << DEV_BSHIFT)
#define ctob(x)		((x) << PGSHIFT)
#define btoc(x)		(((unsigned)(x) + (NBPG - 1)) >> PGSHIFT)

#ifdef __APPLE__
#define btodb(bytes, devBlockSize)	((unsigned)(bytes) / (devBlockSize))
#define dbtob(db, devBlockSize)		((unsigned)(db) * (devBlockSize))
#else
#define btodb(bytes)			((unsigned)(bytes) >> DEV_BSHIFT)
#define dbtob(db)			((unsigned)(db) << DEV_BSHIFT)
#endif

#define bdbtofsb(bn)	((bn) / (BLKDEV_IOSIZE / DEV_BSIZE))

#define USERMODE(x)	(0)
#define BASEPRI(x)	(1)

#if defined(KERNEL) || defined(STANDALONE)
#define DELAY(n)	delay(n)
#else
#define DELAY(n)	{ register int N = (n); while (--N > 0); }
#endif

#endif /* _BSD_RISCV32_PARAM_H_ */
