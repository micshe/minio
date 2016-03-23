#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>   /* for filter, popen3 */
#include<fcntl.h>      /* for fd flags */

#include<sys/ioctl.h>  /* for size */
#include<termios.h>    /* for charmode, linemode */
#include<sys/socket.h>
#include<sys/un.h>     /* for mkserver */
#include<dirent.h>
#include<poll.h>

#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<string.h>
#include<errno.h>

#include<netinet/in.h>  /* */
#include<netinet/tcp.h> /* */
#include<netdb.h>       /* getaddrinfo() */

#include"minio.h"

int close2(int fd)
{
	int cache;
	cache = errno;
		close(fd);
	errno = cache;
	return 0;
} 
int open2(char*path, int flags, ...)
{
	va_list va;
	mode_t mode;

	if((flags & O_CREAT)==O_CREAT)
	{
		va_start(va,flags); 
			mode = va_arg(va,mode_t);
		va_end(va);
	}

	int err;
	int sck;
	struct sockaddr_un addr;
	if(strlen(path)>=108)
		/* because linux (and only linux, it seems) cannot handle sockfile names longer then this */
		goto fallback;
	sck = socket(AF_LOCAL,SOCK_STREAM,0);
	if(sck==-1)
		goto fallback;	
	addr.sun_family=AF_LOCAL;
	strcpy(addr.sun_path,path);
	err = connect(sck, (struct sockaddr*)&addr, sizeof(struct sockaddr_un));

	fcntl(sck,F_SETFD,flags & O_CLOEXEC);
	fcntl(sck,F_SETFL,flags & (O_NONBLOCK|O_ASYNC|O_APPEND/*|O_DIRECT*//*|O_NOATIME*/));

	if(err!=-1)
		return sck;

	close(sck); 
fallback:
	return open(path,flags,mode);
} 
int mkpath(char*path, mode_t mode)
{
	int err; 
	char buf[8192];

	size_t l;
	l = strlen(path);
	if(l>8192)
	{
		errno = ENOMEM;
		return -1;
	} 
	strcpy(buf,path);
	path = buf;

	if(path[l-1]=='/')
		path[l-1]='\0';

	int count;
	count = 0;
	int i;
	/* i=1 to skip leading '/' if there is one */
	for(i=1;i<l;++i)
		if(path[i]=='/')
		{
			path[i]='\0';
				err = mkdir(path,mode);
				if(err==-1 && errno!=EACCES && errno!=EEXIST)
					return -(count+1);
				++count; 
			path[i]='/';
		}
	/* make the final entry */
	err = mkdir(path,mode);
	if(err==-1)
		return -(count+1);

	errno = 0;
	return 0;
}
int mkserver(char*path, int flags)
{
	int err;
	int sck;
	struct sockaddr_un addr;
	if(strlen(path)>=108)
		/* because linux (and only linux, it seems) cannot handle sockfile names longer then this */
		return -1;
	sck = socket(AF_LOCAL,SOCK_STREAM,0);
	if(sck==-1)
		return -1;

	if((flags&O_CLOEXEC)==O_CLOEXEC)
	{
		err = fcntl(sck,F_SETFD,FD_CLOEXEC);
		if(err==-1)
			goto fail;	
	}
	err = fcntl(sck,F_SETFL,flags&(O_NONBLOCK|O_ASYNC));
	if(err==-1)
		goto fail;

	addr.sun_family=AF_LOCAL;
	strcpy(addr.sun_path,path);
	err = bind(sck, (struct sockaddr*)&addr, sizeof(struct sockaddr_un));
	if(err==-1)
		goto fail; 

	/* 
	man tcp(7) suggests that 1024 is the default.  
	it seems to think 128mb of ram is large for a 
	server though, so it may be out of date 
	*/
	err = listen(sck,1024);

	return sck;

fail:
	close2(sck);
	return -1;
}

#if 0
static ssize_t recvuntil(int fd,unsigned char*buf, size_t len)
{
	unsigned char tmp[8192];
	ssize_t err;

	err = recv(fd,tmp,len,MSG_PEEK|MSG_DONTWAIT);
	if(err==-1 && errno==ENOTSOCK)
		return -1;

	/* FIXME handle errors and retries */

	int i;
	for(i=0;i<err;++i)
		if(tmp[i]=='\n'||tmp[i]=='\0')
			break;

	return recv(fd,buf,i,MSG_WAITALL); 
}
#endif
static ssize_t readuntil(int fd,unsigned char*buf, size_t len)
{
	ssize_t err;
	ssize_t i;

	for(i=0;i<len;i+=err)
	{
		waitread(fd,-1);
		err = read(fd,buf+i,1);
		if(err==-1)
		{
			if(errno==EWOULDBLOCK && errno==EINTR && errno==EAGAIN /* && errno!=ERETRY */)
				continue;
			else if(errno==EISDIR)
				return -1;
			else
			{
				if(i==0)
					return -1;
				buf[i]='\0';
				errno = 0;
				return i;
			}
		}

		if(buf[i]=='\n'||buf[i]=='\0')
			break;
	}
	buf[++i]='\0';

	errno = 0;
	return i;
}
static ssize_t readentry(int fd, char*buf, size_t len)
{
#if 0
	unsigned char tmp[8192];
	void*dir;
	dir = tmp;
	struct dirent*ent;
	ent = dir;
#else
	struct dirent*ent;
#endif

	DIR*dp;
	off_t offset;
	ssize_t l;

	int tmpfd;
	tmpfd = dup(fd);
	if(tmpfd==-1)
		return -1;

	dp = fdopendir(tmpfd);
	if(dp==NULL)
		return -1;

retry:
#if 0
	readdir_r(dp,dir,&ent);
#else
	ent = readdir(dp);
#endif
	if(ent!=NULL)
	{
		offset = telldir(dp);
		lseek(fd,offset,SEEK_SET);

		/* skip . and .. */ 
		if(ent->d_name[0] == '.' && ent->d_name[1] == '\0')
			goto retry;
		if(ent->d_name[0] == '.' && ent->d_name[1] == '.' && ent->d_name[2] == '\0')
			goto retry;

		strncpy(buf,ent->d_name, len);
		l = strlen(ent->d_name);
		if(l > len-1)
			/* manually null terminate because strncpy does not */
			buf[len-1]='\0'; 
	}
	else
		l = 0;

	closedir(dp);

	errno = 0;
	return l; 
} 
ssize_t read2(int fd, unsigned char*buf, size_t len)
{
	ssize_t err;

	errno = 0;
	err = recv(fd,buf,len,MSG_DONTWAIT|MSG_NOSIGNAL);
	if(err>=0)
		return err;
	if(err==-1 && errno != ENOTSOCK)
		return -1;

	errno = 0;
	err = read(fd,buf,len); 
	if(err>=0)
		return err;
	if(err==-1 && errno!=EISDIR)
		return -1;

	errno = 0; 
	return readentry(fd,(char*)buf,len);
} 
ssize_t gets2(int fd, char*buf, size_t len)
{
	ssize_t err; 
#if 0
	errno = 0;
	err = recvuntil(fd,(unsigned char*)buf,len);
	if(err>=0)
		return err;
	if(err==-1 && errno!=ENOTSOCK)
		return -1;
#endif

	errno = 0;
	err = readuntil(fd,(unsigned char*)buf,len);
	if(err>=0)
		return err;
	if(err==-1 && errno!=EISDIR)
		return -1;

	errno = 0;
	return readentry(fd,buf,len);
}

ssize_t readall(int fd, unsigned char*buf, size_t len)
{
	ssize_t err;
	errno = 0;
	err = recv(fd,buf,len,MSG_WAITALL|MSG_NOSIGNAL);
	if(err>=0)
		return err; 
	if(err==-1 && errno != ENOTSOCK)
		return -1;
	errno = 0; 
	size_t i;
	for(i=0;i<len;)
	{
		waitread(fd,-1);
		err = read(fd,buf+i,len-i);
		if(err<0)
		{
			if(errno==EWOULDBLOCK || errno==EAGAIN || errno==EINTR)
				continue;
			else
				return i;
		} 
		i+=err;
	} 
	return len; 
}
ssize_t writeall(int fd, unsigned char*buf, size_t len)
{
	ssize_t err;
	errno = 0;
	err = send(fd,buf,len,MSG_WAITALL|MSG_NOSIGNAL);
	if(err>=0)
		return err;
	if(err==-1 && errno != ENOTSOCK)
		return -1;
	errno = 0; 
	size_t i;
	for(i=0;i<len;)
	{
		waitwrite(fd,-1);
		err = write(fd,buf+i,len-i);
		if(err<0)
		{
			if(errno==EWOULDBLOCK || errno==EAGAIN || errno==EINTR)
				continue;
			else
				return i;
		} 
		i+=err;
	} 
	return len; 
}

ssize_t filename(int fd, char*buf, size_t len)
{
	int err; 
	dev_t dev;
	ino_t ino;	
	struct stat meta;

	if(len < 2)
	{
		errno = ENOMEM;
		return -1;
	}

	err = fstat(fd,&meta);
	if(err==-1)
		return -1;
	if(!S_ISDIR(meta.st_mode))
	{
		errno = ENOTDIR;
		return -1;
	}

	dev = meta.st_dev;
	ino = meta.st_ino;

	int up;
	up = openat(fd,"..",O_RDONLY|O_CLOEXEC);
	if(up==-1)
		return -1;

	err = fstat(up,&meta);
	if(err==-1)
		goto fail;
	if(dev == meta.st_dev && ino == meta.st_ino)
	{
		/* deal with root special-case */
		close2(up);
		buf[0] = '.';
		buf[1] = '\0';
		return 2;
	}

	char tmp[8192];
	while(readentry(up,tmp,8192) != -1)
	{
		err = fstatat(up,tmp,&meta,AT_SYMLINK_NOFOLLOW);
		if(err==-1)
			goto fail;
		if(meta.st_dev==dev && meta.st_ino==ino)
		{
			close(up);
			strncpy(buf,tmp,len);
			buf[len-1]='\0';
			return strlen(buf);
		}
	}
	errno = ENOENT;

fail:
	close2(up);
	return -1;	
} 
static off_t minio_cwd_offset = 0;
void cwdseek(off_t offset) { if(offset>=0) minio_cwd_offset = offset; }
off_t cwdtell(void) { return minio_cwd_offset; } 
int cwdgets(char*buf,size_t len)
{
	int fd;
	fd = open(".",O_RDONLY|O_CLOEXEC);
	if(fd==-1)
		return -1;

	ssize_t err;
	lseek(fd,minio_cwd_offset,SEEK_SET);
	err = gets2(fd,buf,len);
	cwdseek(lseek(fd,0,SEEK_CUR));

	close2(fd);

	return err;
}
int cwdopen(int flags) { return open(".",flags); }
int chdirfd(int fd)
{
	off_t offset;
	offset = lseek(fd,0,SEEK_CUR);

	int err;
	err = fchdir(fd);
	if(err==-1)
		return -1;

	minio_cwd_offset = offset; 
	return 0;
}
int chdir2(char*path, int flags)
{
	int fd;
	/* flags is here for O_NOFOLLOW, no other flags matter */
	fd = open(path,O_RDONLY|O_CLOEXEC|(flags&O_NOFOLLOW));
	if(fd==-1)
		return -1;

	int err;
	err = chdirfd(fd);

	close2(fd);
	return err;	
} 
ssize_t cwdfilename(char*buf,size_t len) 
{ 
	int err;
	int fd;

	errno=0;
	fd = open(".",O_RDONLY|O_CLOEXEC);
	if(fd==-1)
		return -1;
	err = filename(fd,buf,len); 
	close2(fd);
	return err;
}
int chdirup(void) 
{
	int err;

	char name[8192];
	err = cwdfilename(name,8192);
	if(err==-1)
		return -1;

	int fd;
	fd = open("..",O_RDONLY|O_CLOEXEC);
	if(fd==-1)
		return -1;

	char buf[8192];
	for(;;)
	{
		err = readentry(fd,buf,8192);
		if(err==0)
		{
			errno = ENOENT;
			goto fail;
		}
		if(err==-1)
			goto fail;

		if(!strcmp(buf,name))
			break;
	}

	err=chdirfd(fd);
	if(err==-1)
		goto fail; 
	close(fd);

	return 0;
fail:
	close2(fd);
	return -1;
}

#if 0
int rmdir2(char*path)
{
	int err;
	if(strcmp(path,"."))
		return rmdir(path);

	int fd;
	fd = open(path,O_CLOEXEC|O_NOFOLLOW|O_RDONLY);
	if(fd==-1)
		return -1;

	char buf[8192];
	err = filename(fd,buf,8192);

	int up;
	up = openat(fd,"..",O_CLOEXEC|O_NOFOLLOW|O_RDONLY);
	close2(fd);
	if(up==-1)
		return -1;

	err = unlinkat(up,buf,AT_REMOVEDIR);
	close2(up);
	return err;
}
#endif
int delete(char*path)
{
	int err; 
	err = unlink(path);
	if(err==0)
		return 0;

	int fd;
	fd = cwdopen(O_NOFOLLOW);
	if(fd==-1)
		return -1;
	err = chdir2(path,O_NOFOLLOW);
	if(err==-1)
		goto fail;

	char buf[8192];
	ssize_t stack;
	stack = 0;
	while(stack>=0)
	{
		err = cwdgets(buf,8192);
		if(err==-1)
			continue;
		if(err==0)
		{
			if(stack==0)
				break;
#if 1
			err = cwdfilename(buf,8192);
			if(err==-1)
				break;
	
			err = chdirup();
			if(err==-1)
				break;

			--stack;
			rmdir(buf);
			/* just keep going if this fails */ 
			continue;
#else
			/* just keep going if this fails */ 
			rmdir2(".");
			/* 
			this may fail as chdirup() won't be
			able to *find* the name of the cwd,
			since it's just been deleted.
			*/
			err = chdirup();
			if(err==-1)
				break;
			--stack;
			continue;
#endif
		}
		err = unlink(buf);
		if(err == 0)
			continue;

		err = chdir2(buf,O_NOFOLLOW);
		if(err==-1)
			/* just keep going if this fails */
			continue;
		++stack;
	} 
	chdirfd(fd);
	close(fd);

	/* if there were any errors in the deleting rmdir will report them now */
	return rmdir(path);

fail:
	close2(fd);
	return -1;
}

int simplex(int simplexfd[2],int flags) 
{ 
	int fd[2];

	int err;
	err = socketpair(AF_LOCAL,SOCK_STREAM,0,fd); 
	if(err==-1)
		return -1;

	if((flags&O_CLOEXEC)==O_CLOEXEC)
	{
		err = fcntl(fd[0], F_SETFD, FD_CLOEXEC);
		if(err==-1)
			goto fail;
		err = fcntl(fd[1], F_SETFD, FD_CLOEXEC);
		if(err==-1)
			goto fail; 
	}

	err = fcntl(fd[0], F_SETFL, flags);
	if(err==-1)
		goto fail;

	err = fcntl(fd[1], F_SETFL, flags);
	if(err==-1)
		goto fail;

	simplexfd[0]=fd[0];
	simplexfd[1]=fd[1];

	return 0;

fail:
	close2(fd[0]);
	close2(fd[1]);
	return -1;
}
int duplex(int duplexfd[2],int flags) 
{ 
	int fd[2];

	int err;
	err = socketpair(AF_LOCAL,SOCK_STREAM,0,fd); 
	if(err==-1)
		return -1;

	if((flags&O_CLOEXEC)==O_CLOEXEC)
	{
		err = fcntl(fd[0], F_SETFD, FD_CLOEXEC);
		if(err==-1)
			goto fail;
		err = fcntl(fd[1], F_SETFD, FD_CLOEXEC);
		if(err==-1)
			goto fail; 
	}

	err = fcntl(fd[0], F_SETFL, flags);
	if(err==-1)
		goto fail;

	err = fcntl(fd[1], F_SETFL, flags);
	if(err==-1)
		goto fail;

	duplexfd[0]=fd[0];
	duplexfd[1]=fd[1];

	return 0;

fail:
	close2(fd[0]);
	close2(fd[1]);
	return -1;
}
int canread(int fd)
{
	struct pollfd data;
	data.fd = fd;
	data.events = POLLIN;

	int err;
	err = poll(&data,1,0);
	if(err==-1)
		return 0;

	return (data.revents & POLLOUT)==POLLOUT &&
	       (data.revents & POLLERR)!=POLLERR && 
	       (data.revents & POLLHUP)!=POLLHUP;
}
int canwrite(int fd)
{
	struct pollfd data;
	data.fd = fd;
	data.events = POLLOUT;

	int err;
	err = poll(&data,1,0);
	if(err==-1)
		return 0;

	return (data.revents & POLLOUT)==POLLOUT && 
	       (data.revents & POLLERR)!=POLLERR && 
	       (data.revents & POLLHUP)!=POLLHUP;
}
int waitread(int fd, int timeout)
{
	struct pollfd data;
	data.fd = fd;
	data.events = POLLIN;

	int err;
	err = poll(&data,1,timeout);
	if(err==-1)
		return 0;

	return (data.revents & POLLIN)==POLLIN && 
	       (data.revents & POLLERR)!=POLLERR && 
	       (data.revents & POLLHUP)!=POLLHUP; 
}
int waitwrite(int fd,int timeout)
{
	struct pollfd data;
	data.fd = fd;
	data.events = POLLOUT;

	int err;
	err = poll(&data,1,timeout);
	if(err==-1)
		return 0;

	return (data.revents & POLLOUT)==POLLOUT && 
	       (data.revents & POLLERR)!=POLLERR && 
	       (data.revents & POLLHUP)!=POLLHUP; 
}
int isonline(int fd)
{
	struct pollfd data;
	data.fd = fd;
	data.events = POLLOUT|POLLIN;

	int err;
	err = poll(&data,1,0);
	if(err==-1)
		return 0;

	return (data.revents & POLLERR)!=POLLERR && 
	       (data.revents & POLLHUP)!=POLLHUP; 
}
int redirect(int src, int dst, int flags)
{
	int err;
	err = dup2(dst,src);
	if(err==-1)
		return -1;

	if((flags&O_CLOEXEC)==O_CLOEXEC)
		fcntl(F_SETFD, FD_CLOEXEC);
	fcntl(F_SETFL, flags);

	close2(dst);
	return 0;
}

int popen3(char*cmd, int stdin,int stdout,int stderr, pid_t*pid)
{ 
	int i;
	int o;
	int e;

	pid_t err;
	err = fork();
	if(err==-1)
		return -1;
	else if(err==0)
	{
		if(pid==NULL)
		{
			err = fork();
			if(err>0)
				_exit(0);
			else if(err<0)
				_exit(127);
		}

		if(stdin==-1) 
			i = open("/dev/null",O_RDWR);
		else
			i = dup(stdin);
		if(stdout==-1) 
			o = open("/dev/null",O_RDWR);
		else
			o = dup(stdout);
		if(stderr==-1) 
			e = open("/dev/null",O_RDWR);
		else
			e = dup(stderr);

		if(stdin!=stdout && stdin!=stderr) close2(stdin);
		if(stdout!=stderr) close2(stdout);
		close2(stderr);

		/* FIXME avoid collisions and clobbering with 0,1,2 */
		redirect(0,i,0);
		redirect(1,o,0);
		redirect(2,e,0);

		char*shell[4];
		shell[0]="/bin/sh";
		shell[1]="-c";
		shell[2]=cmd;
		shell[3]=NULL;	
		execv(shell[0],shell);

		/* failure */
		_exit(127);
	}

	if(stderr != stdin && stderr != stdout) close2(stderr);
	if(stdout != stdin) close2(stdout);
	close2(stdin); 

	if(pid!=NULL) 
	{
		*pid = err;
		return 0;
	}

	int status;
	err = waitpid(err,&status,0);
	if(err==-1)
		/* false positive */
		return 0;

	if(WIFEXITED(status)&&WEXITSTATUS(status)==127)
	{
		/* using this to literally mean 'we did not execute' */
		errno = ENOEXEC;
		return -1;
	}

	return 0;
}
int filter(int pull, char*cmd, int push, pid_t*pid)
{
	int err;
	int flags;
	int channel[2];

	if(pull>-1 && push >-1)
		return popen3(cmd,pull,push,-1,pid);

	err = simplex(channel,O_CLOEXEC);
	if(err==-1)
		return -1;

	if(push==-1&&pull>-1)
	{ 
		flags = fcntl(pull,F_GETFD);
		if(flags==-1)
			goto fail;
		if((flags&FD_CLOEXEC)==FD_CLOEXEC) flags = O_CLOEXEC;
		flags |= fcntl(pull,F_GETFL); 

		popen3(cmd,pull,channel[1],-1,pid);
		redirect(pull,channel[0],flags);
		return 0;
	}
	if(pull==-1&&push>-1)
	{
		flags = fcntl(push,F_GETFD);
		if(flags==-1)
			goto fail;
		if((flags&FD_CLOEXEC)==FD_CLOEXEC) flags = O_CLOEXEC;
		flags |= fcntl(push,F_GETFL); 

		popen3(cmd,channel[0],push,-1,pid);
		redirect(push,channel[1],flags);
		return 0;
	}

	/* unreachable */
fail:
	close2(channel[0]);
	close2(channel[1]);
	return -1;
}
int launch(char*cmd,pid_t*pid)
{
	int err;

	int channel[2];
	err = duplex(channel,O_CLOEXEC);
	if(err==-1)
		return -1;

	err = popen3(cmd,channel[1],channel[1],-1,pid);
	if(err==0)
		return channel[0];

	err = errno;
	close(channel[0]);
	errno = err;

	return -1;
}

ssize_t write2(int fd, unsigned char*buf, size_t len)
{
	ssize_t err;
	errno = 0; 
	err = send(fd,buf,len,MSG_NOSIGNAL);
	if(err==-1 && errno != ENOTSOCK)
		return -1;
	errno = 0; 
	return write(fd,buf,len); 
}
int puts2(int fd, char*string)
{
	int len;
	len = strlen(string); 

	return writeall(fd,(unsigned char*)string,len);
}
int printva(int fd, char*string, va_list*args)
{
	int err;

	char buf[8192];
	buf[0]='\0';
	err = vsnprintf(buf,8192,string,*args);
	if(err<0)
		return err;

	int len;
	len = strlen(buf); 

	err = puts2(fd,buf); 
	if(err<len)
		/* print can't be resumed */
		return -1;

	return 0;
}
int print(int fd, char*string, ...)
{
	int err;
	va_list args;

	va_start(args,string);
		err = printva(fd,string,&args);
	va_end(args);

	return err; 
}

int nonblocking(int fd)
{
	int err;
	err = fcntl(fd,F_GETFL);
	if(err==-1)
		return -1;
	return fcntl(fd,F_SETFL,err | O_NONBLOCK);
}
int blocking(int fd)
{
	int err;
	err = fcntl(fd,F_GETFL);
	if(err==-1)
		return -1;
	return fcntl(fd,F_SETFL,err & ~O_NONBLOCK);
}
int cloexec(int fd)
{
	int err;
	err = fcntl(fd,F_GETFD);
	if(err==-1)
		return -1;
	return fcntl(fd,F_SETFD,err | O_CLOEXEC);
}
int noncloexec(int fd)
{
	int err;
	err = fcntl(fd,F_GETFD);
	if(err==-1)
		return -1;
	return fcntl(fd,F_SETFD,err & ~O_CLOEXEC);
}

int getflags(int fd)
{
	int flags;
	int err;

	err = fcntl(fd,F_GETFD);
	if(err==-1)
		return -1;
	if(err&FD_CLOEXEC) flags = O_CLOEXEC; else flags = 0;

	err = fcntl(fd,F_GETFL);
	if(err==-1)
		return -1;

	return flags | err;
}
int setflags(int fd,int flags)
{
	if((flags&O_CLOEXEC)==O_CLOEXEC)
		fcntl(fd,F_SETFD,FD_CLOEXEC);
	return fcntl(fd,F_SETFL,flags); 
}

int cls(int tty) { return print(tty,"\033[2J\033[1;1H"); }
int locate(int tty, int x,int y) { return print(tty,"\033[%d;%dH",x,y); }
int normal(int tty) { return print(tty, "\033[0m"); }
int color(int tty, int fg, int bg)
{
	if(fg>-1)
		print(tty,"\033[38;2;%d;%d;%dm", (fg&0x00FF0000)>>16, (fg&0x0000FF00)>>8, (fg&0x0000FF));
	if(bg>-1)
		print(tty,"\033[48;2;%d;%d;%dm", (bg&0x00FF0000)>>16, (bg&0x0000FF00)>>8, (bg&0x0000FF));
	return 0;
}
int greyscale(int tty, int fg, int bg)
{
	/* these are from the x11 256 colour palette -- dark grey to light grey -- does not contain black or white */
	if(fg>-1 && fg<24)
		print(tty,"\033[38;5;%dm",fg+232);
	if(bg>-1 && bg<24)
		print(tty,"\033[48;5;%dm",bg+232); 
	return 0; 
}
int websafe(int tty, int fg, int bg)
{
	/* these are from the x11 256 colour palette -- r*36+g*6+b */
	if(fg>-1 && fg<224)
		print(tty,"\033[38;5;%dm",fg+16);
	if(bg>-1 && bg<224)
		print(tty,"\033[48;5;%dm",bg+16); 
	return 0;
}
int palette(int tty, int fg, int bg)
{
	/* these are from the bottom of the x11 256 colour palette */
	if(fg>-1 && fg < 16)
	{
		if(fg<9)
			print(tty,"\033[3%dm", fg);
		else
			print(tty,"\033[9%dm", fg - 8);
	}
	if(bg>-1 && bg<16)
	{
		if(bg<9)
			print(tty,"\033[4%dm", bg);
		else
			print(tty,"\033[10%dm", bg - 8);
	}
	return 0;
}
int invert(int tty) { return print(tty,"\033[7m"); }
int bold(int tty) { return print(tty,"\033[1m"); }
int italics(int tty) { return print(tty,"\033[3m"); }
int underline(int tty) { return print(tty,"\033[4m"); }

int area(int tty, int*x,int*y)
{
	int err;

	struct winsize w;
	err = ioctl(0,TIOCGWINSZ, &w);
	if(err==-1)
		return -1;

	if(x!=NULL)
		*x = w.ws_col;
	if(y!=NULL)
		*y = w.ws_row;

	return 0;
}
#if 0
int resize(int tty, int x, int y)
{
	struct winsize w;
	w.ws_col = x;
	w.ws_row = y;
	return ioctl(0,TIOCSWINSZ, &w);
}
#endif

int charmode(int tty)
{
        struct termios data;

        tcgetattr(tty, &data);
        data.c_lflag &= ~ICANON;
        data.c_lflag &= ~ECHO;
	data.c_lflag &= ~ISIG;  /* disable C-c C-\ C-z */
	data.c_iflag &= ~IXON;  /* disable C-q C-s */
	data.c_iflag &= ~ICRNL;	/* disable translate \r to \n (makes ctrl+m be readable, and ctrl+j doppleganger'd with enter-key) */
        data.c_cc[VMIN] = 0;
        data.c_cc[VTIME] = 0;
        tcsetattr(tty, TCSANOW, &data);

        return 0;
}
int linemode(int tty)
{
	tcflush(tty,TCIFLUSH);

        struct termios data;

        tcgetattr(tty, &data);
        data.c_lflag |= ICANON;
        data.c_lflag |= ECHO;
	data.c_lflag |= ISIG;   /* enable C-c C-\ C-z */
	data.c_iflag |= IXON;   /* enable C-q C-s */ 
	data.c_iflag |= ICRNL;	/* enable translate \r to \n (makes ctrl+j be readable, and ctrl+m doppleganger'd with enter-key) */
        data.c_cc[VMIN] = 1;
        data.c_cc[VTIME] = 0;
        tcsetattr(tty, TCSANOW, &data);

        return 0;
} 
int input(int tty, char buf[8])
{
	int err;
	char tmp[8];
	/* to allow for using input() as a press-any-key-to-continue function easily */
	if(buf==NULL)buf=tmp;

	for(err=0;err<8;++err) buf[err] = 0;

	waitread(tty,-1);
	err = read(tty,buf,8);
	if(err==0||err==-1)
		goto fail;

	if(buf[0]!='\033')
		/* we read a control+char or shift+char or regular char key press */
		goto pass;

	if(err==1)
	{
		/* usleep(1) gives us a 1 milisecond pause, long enough for read() to return the next char */
		usleep(1000);
		err += read(tty,buf+1,1);
		if(err==1+0||err==1+-1)
			/* it was an escape press */
			goto pass;

		if(buf[1]!='[')
			/* it was an alt+char or alt+shift+char or ctrl+alt+char key press */
			goto pass;

		/* 
		otherwise fallthrough, but this should not ever occur.  escape codes generated by
		function/edit keys or modified function/edit/arrow keys should be read in their
		entirity by the first read(,,8)
		*/
	} 
	/* else MORE then one character read */
	else if(buf[0]>127)
		/* we read a utf8 character */
		goto pass;
	else if(buf[2]>='A' && buf[2]<='D')
		/* we read an unmodified arrow key press */
		goto pass;
	else
		/* we read a more involved key press, which we ignore/censor and signal failure */
		goto fail;
fail:
	return -1; 
pass:
	return err;
}
int isarrow(char buf[8]) { return buf[0]=='\033' && buf[1] == '[' && buf[2] >= 'A' && buf[2] <= 'D'; }
int isup(char buf[8]) { return !strcmp(buf,"\033[A"); }
int isdown(char buf[8]) { return !strcmp(buf,"\033[B"); }
int isleft(char buf[8]) { return !strcmp(buf,"\033[D"); }
int isright(char buf[8]) { return !strcmp(buf,"\033[C"); }
int isescape(char buf[8]) { return buf[0] == 27 && buf[1] == 0; }
int isbackspace(char buf[8]) { return buf[0] == 127; }
int isenter(char buf[8]) { return buf[0] == 13; }
int isspace(char buf[8]) { return buf[0] == 32; }
int istab(char buf[8]) { return buf[0] == 9; }
int isalt(char buf[8]) { return buf[0] == '\033' && buf[1] > 32 && buf[2] == 0; }
int isctrl(char buf[8]) { return !isalt(buf) && buf[0] < 32 && buf[0] != 27 && buf[0] != 9 && buf[0] != 13; }
int isctrlalt(char buf[8]) 
{ 
	return buf[0]=='\033' && 
	       buf[1] > 0 && buf[1] < 32 && 
#if 0
	       buf[1] != 27 && 
#else 
	       /* allow ctrl+alt+[ doppleganger of alt+esc */
#endif
	       /* 
	       we allow doppleganers of tab and enter (ctrl+alt+i, ctrl+alt+j)
	       because most gui terminals will trap alt+tab, alt+enter, etc
	       making those variants of them useless to us.
	       */
	       buf[2] == 0; 
}
int ischaracter(char buf[8]) { return (buf[0]>' ' && buf[0] != 127); }
int character(char buf[8])
{
	if(buf[0] == 27 && buf[1] >= 32 && buf[2] == 0)
		/* alt + char (discluding space,tab,enter, since they are frequently gui accelerators) */
		return buf[1];
	else if(buf[0] == 27 && buf[1] < 32 && buf[1] != 27 && buf[1] != 13 && buf[1] != 9 && buf[2] == 0)
		/* ctrl + alt + char (discluding space,tab,enter,escape) */
		return buf[1] + 'a'-1;
	else if(buf[0] == 27 && buf[1] < 32 && buf[1] == 9 && buf[2] == 0)
		/* ctrl + alt + i doppleganger of alt+tab */
		return 'i'; 
	else if(buf[0] == 27 && buf[1] < 32 && buf[1] == 13 && buf[2] == 0)
		/* ctrl + alt + m doppleganger of alt+enter */
		return 'm';
	else if(buf[0] == 27 && buf[1] == 27 && buf[2] == 0)
		/* ctrl + alt + [ doppleganger of alt+escape */
		return '[';
	else if(buf[0] < 32 && buf[0] != 27 && buf[0] != 13 && buf[0] != 9 && buf[1] == 0)
		/* ctrl + char (discluding space,tab,enter,escape dopplegangers) */
		return buf[0] + 'a'-1;
	else if((buf[0] >= ' '||buf[0]==9||buf[0]==13) && buf[0] < 127)
		/* regular char (including space,tab,enter -- discluding backspace) */
		return buf[0];
	else if(buf[0]>127)
		/* it is a utf8 character, which we must parse and package into the return int */
		return *((int*)buf);
	else	
		/* not a char */
		return 0;
}

void quit(int code)
{
	int tty;
	tty = open("/dev/tty",O_RDWR|O_NONBLOCK|O_CLOEXEC);
	if(tty == -1)
		/* we're a daemon, so just exit */
		exit(code);

	/* restore normal terminal settings */
	normal(tty);
	linemode(tty); 

	/* exit & invoke atexit() handlers */
	exit(code);
}
void crash(char*string,...)
{
	va_list args;
	va_start(args,string);
		printva(2,string,&args);
	va_end(args);

	/* exit without invoking atexit() handlers or restoring tty settings (shell does this in event of crash) */
	_exit(EXIT_FAILURE);
}

static struct sockaddr_storage* dns(char*hostname, struct sockaddr_storage*addr)
{
	struct addrinfo hints;
	struct addrinfo*servinfo;
	struct sockaddr_storage *h;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; 
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(hostname,NULL,&hints,&servinfo);

	/* FIXME is this valid? */ 
	h = (struct sockaddr_storage*) servinfo->ai_addr;
	memcpy(addr,h,sizeof(struct sockaddr_storage));
     
	freeaddrinfo(servinfo);
	return addr;
} 
int dial(char*hostname, short port, int flags)
{
	struct sockaddr_storage addr; 
	struct sockaddr_in*ipv4;
	struct sockaddr_in6*ipv6;
	ipv4 = (struct sockaddr_in*)&addr;
	ipv6 = (struct sockaddr_in6*)&addr;

	dns(hostname,&addr);

	int domain;
	domain = addr.ss_family;
	if(domain==AF_INET6)
		ipv6->sin6_port = htons(port);
	else
		ipv4->sin_port = htons(port);

	int sck; 
	sck = socket(domain,SOCK_STREAM,0); 

	int err;
	if((flags&O_CLOEXEC)==O_CLOEXEC)
	{
		err = fcntl(sck,F_SETFD,flags&O_CLOEXEC);
		if(err==-1)
			goto fail;
	}
	err = fcntl(sck,F_SETFL,flags&(O_NONBLOCK|O_ASYNC)); 
	if(err==-1)
		goto fail;

	err = connect(sck, (struct sockaddr*)&addr, sizeof(struct sockaddr_storage));
	if(err==-1)
		goto fail;

	return sck;
fail:
	close2(sck);
	return -1; 
} 
int tcp4(short port, int flags)    /* tcp/ip4 */
{
	int sck;
	sck = socket(AF_INET,SOCK_STREAM,0);

	int err;
	if((flags&O_CLOEXEC)==O_CLOEXEC)
	{
		err = fcntl(sck,F_SETFD,FD_CLOEXEC);
		if(err==-1)
			goto fail;
	}
	err = fcntl(sck,F_SETFL,flags&(O_NONBLOCK|O_ASYNC)); 
	if(err==-1)
		goto fail;

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;	
	addr.sin_port = htons(port);	

	err = bind(sck,(struct sockaddr*)&addr,sizeof(struct sockaddr_in));
	if(err==-1)
		goto fail; 

	err = listen(sck,1024);
	if(err==-1)
		goto fail;

	return sck;
fail:
	close2(sck);
	return -1;
}
int tcp6(short port, int flags)    /* tcp/ip6 */
{
	int sck;
	sck = socket(AF_INET6,SOCK_STREAM,0);

	int err;
	if((flags&O_CLOEXEC)==O_CLOEXEC)
	{
		err = fcntl(sck,F_SETFD,FD_CLOEXEC);
		if(err==-1)
			goto fail;
	}
	err = fcntl(sck,F_SETFL,flags&(O_NONBLOCK|O_ASYNC)); 
	if(err==-1)
		goto fail;

	struct sockaddr_in6 addr;
	addr.sin6_family = AF_INET6;
	addr.sin6_addr = in6addr_any;	
	addr.sin6_port = htons(port);	

	err = bind(sck,(struct sockaddr*)&addr,sizeof(struct sockaddr_in6));
	if(err==-1)
		goto fail;

	err = listen(sck,1024);
	if(err==-1)
		goto fail;

	return sck;
fail:
	close2(sck);
	return -1;
}

int give(int fd, int payload)
{
	char byte;
        byte = '\0';
        struct iovec unused;
        unused.iov_base = &byte;
        unused.iov_len = sizeof(char);

        struct body
        {
                /* these have to be stored end-to-end */
                struct cmsghdr body;
                int payloadfd;
        };

        struct msghdr head;
        struct body body;
        memset(&body,0,sizeof(struct body));

        head.msg_name = NULL;
        head.msg_namelen = 0;
        head.msg_flags = 0;
        head.msg_iov = &unused;
        head.msg_iovlen = 1;

        head.msg_control = &body;
        head.msg_controllen = sizeof(body);

        struct cmsghdr *interface;
        interface = CMSG_FIRSTHDR(&head); 
        interface->cmsg_level = SOL_SOCKET;
        interface->cmsg_type = SCM_RIGHTS; 
        interface->cmsg_len = CMSG_LEN(1*sizeof(int)); 
        *((int*)CMSG_DATA(interface)) = payload;

	int err;
	err = sendmsg(fd, &head, MSG_WAITALL);
	if(err==-1)
		return -1;

	/* 
	it's less confusing if the fd only
	exists in one process at a time, so
	we close if the send succeeded.
	*/
	close(payload);

	return 0;	
}
int take(int fd, int flags)
{
	int err;

	err = accept(fd,NULL,NULL);
	if(err!=-1)
	{
		fd = err;
		err = setflags(fd,flags);
		if(err==-1)
		{
			close2(fd);
			return -1;
		}
		return fd;
	}

	/* else, we recv an fd over the socket @fd */

	char byte;
	byte = '\0';
	struct iovec unused;
	unused.iov_base = &byte;
	unused.iov_len = sizeof(char);

	struct body
	{
		/* these have to be stored end-to-end */
		struct cmsghdr body;
		int fd;
	}; 
	struct msghdr head;
	struct body body;
	memset(&body,0,sizeof(struct body));

	head.msg_name = NULL;
	head.msg_namelen = 0;
	head.msg_flags = 0;
	head.msg_iov = &unused;
	head.msg_iovlen = 1; 
	head.msg_control = &body;
	head.msg_controllen = sizeof(body);

	struct cmsghdr *interface;
	interface = CMSG_FIRSTHDR(&head); 
	interface->cmsg_level = SOL_SOCKET;
	interface->cmsg_type = SCM_RIGHTS; 
	interface->cmsg_len = CMSG_LEN(1*sizeof(int));

	err = recvmsg(fd, &head, MSG_WAITALL);
	if(err==-1)
		return -1; 

	if(interface->cmsg_len != CMSG_LEN(1*sizeof(int)))
		/* 
		we've not go the wrong message somehow

		FIXME check and see if we have 
		actually received them?  do we 
		need to close them?  how do we 
		find out how many we recieved? 
		*/
		return -1;

	/* return the new fd that we recieved in the payload */
	fd = *((int*)CMSG_DATA(interface));
	err = setflags(fd,flags);
	if(err==-1)
	{
		close2(fd);
		return -1;
	}

	return fd;
}
