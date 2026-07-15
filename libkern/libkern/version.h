/*
 * SPDX-License-Identifier: APSL-2.0
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * https://opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 *
 * CARIBEOS MODIFICATION NOTICE
 * This file was created as part of the experimental CaribeOS RV32IMACSU
 * port of Apple XNU 2050.48.11 on 2026-07-14.
 */

#line 1
#ifndef LIBKERN_VERSION_H
#define LIBKERN_VERSION_H

#define VERSION_MAJOR 2050
#define VERSION_MINOR 48
#define VERSION_VARIANT ".11"
#define VERSION_REVISION 11
#define VERSION_STAGE_DEV 0x20
#define VERSION_STAGE_ALPHA 0x40
#define VERSION_STAGE_BETA 0x60
#define VERSION_STAGE_RELEASE 0x80
#define VERSION_STAGE VERSION_STAGE_RELEASE
#define VERSION_PRERELEASE_LEVEL 0
#define OSTYPE "Darwin"
#define OSRELEASE "2050.48.11"

#ifndef ASSEMBLER

extern const int version_major;
extern const int version_minor;
extern const char version_variant[];
extern const int version_revision;
extern const int version_stage;
extern const int version_prerelease_level;
extern const char ostype[];
extern const char osrelease[];
extern const char osbuilder[];
extern const char version[];

#define OSVERSIZE 256
extern char osversion[];

#endif /* !ASSEMBLER */

#endif /* LIBKERN_VERSION_H */
