#ifndef _PEXPERT_RISCV32_PROTOS_H_
#define _PEXPERT_RISCV32_PROTOS_H_

extern void cninit(void);
extern int sprintf(char *str, const char *format, ...);

int switch_to_serial_console(void);
void switch_to_old_console(int);
boolean_t console_is_serial(void);
int serial_init(void);
void serial_putc(char);
int serial_getc(void);

void cnputc(char);
void cnputc_unbuffered(char);
void cnputcusr(char);
int cngetc(void);

#endif /* _PEXPERT_RISCV32_PROTOS_H_ */
