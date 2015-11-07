#ifndef MINIO_H
#define MINIO_H

#ifdef __cplusplus
exter "C" {
#endif

#include<sys/types.h>   /* for size_t ssize_t */
#include<sys/stat.h>    /* for mode_t */
#include<fcntl.h>	/* for O_CLOEXEC */
#include<stdarg.h>      /* for va_list */
#include<stdlib.h>	/* for NULL */

int close2(int fd);
int open2(char*path, int flags, ...);
int mkpath(char*path, mode_t mode);
int mkserver(char*path, int flags);

ssize_t read2(int fd, unsigned char*buf, size_t len);
ssize_t gets2(int fd, char*buf, size_t len);

ssize_t filename(int fd, char*buf, size_t len);

int take(int fd, int flags);
int give(int fd, int payload);

void cwdseek(off_t offset);
off_t cwdtell(void);
int cwdgets(char*buf,size_t len);
int cwdopen(int flags);
int chdirfd(int fd);
int chdirup(void);

int delete(char*path);

int duplex(int duplexfd[2],int flags);
int simplex(int simplexfd[2],int flags);
int canread(int fd);
int canwrite(int fd); 
int waitread(int fd,int timeout);
int waitwrite(int fd,int timeout);
int isonline(int fd); 
int redirect(int src, int dst, int flags);

int popen3(char*cmd, int stdin,int stdout,int stderr, pid_t*pid);
int filter(int pull, char*cmd, int push, pid_t*pid);

int printva(int fd, char*string, va_list*args); 
int print(int fd, char*string, ...);

int nonblocking(int fd); 
int blocking(int fd); 
int cloexec(int fd); 
int noncloexec(int fd);
int getflags(int fd);
int setflags(int fd,int flags);

int cls(int tty);
int locate(int tty, int x,int y);

int normal(int tty); 
int color(int tty, int fg, int bg);
int websafe(int tty, int fg, int bg); 
int greyscale(int tty, int fg, int bg); 
int palette(int tty, int fg, int bg);

int invert(int tty);
int bold(int tty);
int italics(int tty);
int underline(int tty);
int doubleunderline(int tty);

int size(int tty, int*x,int*y);

int charmode(int fd);
int linemode(int fd);

int input(int tty, char buf[8]);
int isarrow(char buf[8]);
int isup(char buf[8]);
int isdown(char buf[8]);
int isleft(char buf[8]);
int isright(char buf[8]);
int isescape(char buf[8]);
int isbackspace(char buf[8]);
int isenter(char buf[8]);
int isspace(char buf[8]);
int istab(char buf[8]); 
int isctrl(char buf[8]);
int isalt(char buf[8]);
int isctrlalt(char buf[8]);
int ischaracter(char buf[8]);
int character(char buf[8]);

void quit(int code);
void crash(char*string,...);

int tcp4(short port, int flags);
int tcp6(short port, int flags);
int dial(char*hostname, short port, int flags);

#ifdef __cplusplus
}
#endif

#endif
