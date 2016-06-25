#ifndef MINIO_H
#define MINIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include<sys/types.h>   /* for size_t */
#include<fcntl.h>	/* for O_CLOEXEC O_NONBLOCK O_NOFOLLOW etc */
#include<stdarg.h>      /* for va_list */
#include<stdlib.h>	/* for NULL */

int close2(int fd);
int open2(char*path, int flags, ...);
int mkpath(char*path, int mode);
int mkserver(char*path, int flags);

size_t read2(int fd, unsigned char*buf, size_t len);
size_t gets2(int fd, char*buf, size_t len); 
size_t readall(int fd, unsigned char*buf, size_t len);
size_t write2(int fd, unsigned char*buf, size_t len);
size_t puts2(int fd, char*buf); 
size_t writeall(int fd, unsigned char*buf, size_t len); 

size_t filename(int fd, char*buf, size_t len);

int take(int fd, int flags);
int give(int fd, int payload);

int cd(int fd, char*path, int flags);
int cdup(int fd, int flags);

off_t seek(int fd, off_t offset);
off_t tell(int fd);

int delete(char*path);

int duplex(int duplexfd[2],int flags);
int simplex(int simplexfd[2],int flags);
int canread(int fd);
int canwrite(int fd); 
int waitread(int fd,int timelimit);
int waitwrite(int fd,int timelimit);
int isonline(int fd); 
int redirect(int fd, int target, int flags);

int popen3(char*cmd, int stdin,int stdout,int stderr, pid_t*pid);
int filter(int pull, char*cmd, int push, pid_t*pid);
int launch(char*cmd, pid_t*pid);

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

