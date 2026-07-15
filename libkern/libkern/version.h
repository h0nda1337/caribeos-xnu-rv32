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
