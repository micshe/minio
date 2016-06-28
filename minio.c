#include<sys/types.h>
#include<sys/stat.h>   /* for filename, cd, cdup */
#include<sys/wait.h>   /* for filter, popen3 */
#include<sys/ioctl.h>  /* for area */
#include<sys/socket.h> /* for mkserver, open, tcp, dial */
#include<sys/un.h>     /* for mkserver */
#include<netinet/in.h> /* htons() etc (FreeBSD) */ /* deprecated */
#include<netdb.h>      /* getaddrinfo() */
#include<dirent.h>     /* for gets2 */
#include<poll.h>       /* for canread, waitread, canwrite, waitwrite */
#include<fcntl.h>      /* O_CLOEXEC|O_NONBLOCK|etc */
#include<termios.h>    /* for charmode, linemode */

#include<unistd.h>

#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<string.h>
#include<errno.h>

#include<limits.h>

#include"minio.h"

/* global variables */
static off_t minio_cwd_offset = 0;

/* public api */ 
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
	int mode;

	if((flags & O_CREAT)==O_CREAT)
	{
		va_start(va,flags); 
			mode = va_arg(va,int);
		va_end(va);
	}

	int err;
	int sck;
	struct sockaddr_un addr;
	if(strlen(path)>=108)
		/* because linux (and only linux, it seems) cannot handle sockfile names longer then this */
		goto fallback;
	sck = socket(AF_LOCAL,SOCK_STREAM,0);
	if(sck<0)
		goto fallback;	
	addr.sun_family=AF_LOCAL;
	strcpy(addr.sun_path,path);
	err = connect(sck, (struct sockaddr*)&addr, sizeof(struct sockaddr_un));

	if(flags&O_CLOEXEC)
		fcntl(sck,F_SETFD,FD_CLOEXEC);
	fcntl(sck,F_SETFL,flags & (O_NONBLOCK|O_ASYNC|O_APPEND/*|O_DIRECT*//*|O_NOATIME*/));

	if(err>=0)
		return sck;

	close(sck); 
fallback:
	return ((errno=0),open(path,flags,mode));
} 
int mkpath(char*path, int mode)
{
	int err; 
	char buf[8192];

	size_t l;
	l = strlen(path);
	if(l>8192)
		return ((errno=ENOMEM),-1);
	strcpy(buf,path);
	path = buf;

	if(path[l-1]=='/')
		path[l-1]='\0';

	size_t count = 0;
	size_t i;
	/* i=1 to skip leading '/' if there is one */
	for(i=1;i<l;++i)
		if(path[i]=='/')
		{
			path[i]='\0';
				err = mkdir(path,mode);
				if(err<0 && errno!=EACCES && errno!=EEXIST)
					return -1;
				++count; 
			path[i]='/';
		}
	/* make the final entry */
	err = mkdir(path,mode);
	if(err<0)
		return -1;

	return ((errno=0),0);
}
int mkserver(char*path, int flags)
{
	int err;
	int sck;
	struct sockaddr_un addr;
	if(strlen(path)>=108)
		/* because linux (and only linux, it seems) cannot handle sockfile names longer then this */
		return ((errno=ENAMETOOLONG),-1);
	sck = socket(AF_LOCAL,SOCK_STREAM,0);
	if(sck<0)
		return -1;

	if((flags&O_CLOEXEC)==O_CLOEXEC)
	{
		err = fcntl(sck,F_SETFD,FD_CLOEXEC);
		if(err<0)
			goto fail;	
	}
	err = fcntl(sck,F_SETFL,flags&(O_NONBLOCK|O_ASYNC));
	if(err<0)
		goto fail;

	memset(&addr,0,sizeof(struct sockaddr_un));
	addr.sun_family=AF_LOCAL;
	strcpy(addr.sun_path,path);
	err = bind(sck, (struct sockaddr*)&addr, sizeof(struct sockaddr_un));
	if(err<0)
		goto fail; 

	/* 
	man tcp(7) suggests that 1024 is the default.  
	it seems to think 128mb of ram is large for a 
	server though, so it may be out of date 
	*/
	err = listen(sck,1024);
	if(err<0)
		goto fail;

	return ((errno=0),sck);

fail:
	close2(sck);
	return -1;
}

static size_t readuntil(int fd,unsigned char*buf,size_t len)
{
	ssize_t err;

	size_t i; 
	for(i=0;i<len;i+=err)
	{
		waitread(fd,-1);
		err = read(fd,buf+i,1);
		if(err<0)
		{
			if(errno==EWOULDBLOCK && errno==EINTR && errno==EAGAIN /* && errno!=ERETRY */)
				continue;
			else if(errno==EINVAL||errno==EISDIR)
				/* do not alter @buf */
				return 0;
			else
			{
				buf[i]='\0';
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
static size_t readentry(int fd, char*buf, size_t len)
{
#ifdef __FreeBSD__ /* FIXME should be testing for FreeBSD's default libc */
	int err;

	off_t offset;
	offset = lseek(fd,0,SEEK_CUR);
	if(offset<0)
		return 0;

	DIR*dp;
	dp = fdopendir(fd);
	if(dp==NULL)
		return 0;
	rewinddir(dp);

	struct dirent*ent;
	size_t l=0;
	off_t i;
	for(i=0;i<=offset;++i)
	{
retry:
		ent=((errno=0),readdir(dp));
		if(ent==NULL)
			goto fail;

		if((ent->d_name[0]=='.' && ent->d_name[1]=='\0')||
		   (ent->d_name[0]=='.' && ent->d_name[1]=='.' && ent->d_name[2]=='\0'))
{
			/* FIXME ensusre this behaves the same as linux/glibc version */
			i = (i>0)?i-1:0;
			goto retry;
}
	}
	/* leaves backing fd open */
	fdclosedir(dp);

	strncpy(buf,ent->d_name,len);
	l=strlen(ent->d_name);
	if(l > len-1)
		/* manually null terminate because strncpy does not */
		buf[len-1]='\0';

	/* no recourse if this fails */
	lseek(fd,i,SEEK_SET);

	return ((errno=0),l);

fail:
	err=errno;
		/* leaves backing fd open */
		fdclosedir(dp);
	errno=err;

	return 0;
#else /* FIXME should be testing for GNU libc */
	int err;

	struct dirent*ent;

	DIR*dp=NULL;
	off_t offset;
	size_t l;

	int tmpfd=-1;
	if(fd==-1)
		tmpfd=open(".",O_CLOEXEC);
	else
	{
		/* manual dup(fd,O_CLOEXEC) */
		tmpfd=openat(fd,".",O_CLOEXEC);
		if(tmpfd<0)
			return 0;
		offset = lseek(fd,0,SEEK_CUR);
		if(offset<0)
			goto fail;
		offset = lseek(tmpfd,offset,SEEK_SET);
		if(offset<0)
			goto fail;
	}
	if(tmpfd<0)
		/* do not change @buf */
		return 0;

	dp = fdopendir(tmpfd);
	if(dp==NULL)
		/* do not change @buf */
		return 0;

retry:
	ent = readdir(dp);
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

	return ((errno=0),l); 

fail:
	err=errno;
		if(dp!=NULL)
			closedir(dp);
		if(tmpfd!=-1)
			close(tmpfd);
	errno=err;

	return 0;
#endif
} 
size_t read2(int fd, unsigned char*buf, size_t len) /* FIXME rename: pull() */
{
	ssize_t err;
	size_t e;

	if(fd==-1)
	{
		fd = open(".",O_RDONLY|O_CLOEXEC);
		if(fd==-1)
			return -1;

		lseek(fd,minio_cwd_offset,SEEK_SET);
		e = readentry(fd,(char*)buf,len); 
		/* FIXME roll into readentry? */
		minio_cwd_offset = lseek(fd,0,SEEK_CUR); 

		close2(fd);

		return e;
	}

	if(len>SSIZE_MAX)
	{
		e = read2(fd,buf,SSIZE_MAX);
		if(e<SSIZE_MAX)
			return e;
		if(errno!=0)
			return e;

		buf = buf + SSIZE_MAX;
		len = len - SSIZE_MAX;
	} 

	err = ((errno=0),recv(fd,buf,len,MSG_DONTWAIT|MSG_NOSIGNAL));
	if(err>=0)
		return err;
	if(err<0 && errno != ENOTSOCK)
		return 0;

	err = ((errno=0),read(fd,buf,len));
	if(err>=0)
		return err;
	if(err<0 && errno!=EISDIR)
		return 0;

	return readentry(fd,(char*)buf,len);
}
size_t gets2(int fd, char*buf, size_t len) /* FIXME rename: getln() */
{
	size_t err; 

	if(fd==-1)
	{
		fd = open(".",O_RDONLY|O_CLOEXEC);
		if(fd==-1)
			return -1;

		lseek(fd,minio_cwd_offset,SEEK_SET);
		err = readentry(fd,buf,len);
		/* FIXME roll into readentry? */
		minio_cwd_offset = lseek(fd,0,SEEK_CUR); 

		close2(fd);

		return err;
	}

	err = readentry(fd,buf,len);
	if(errno==ENOTDIR || errno==ESPIPE || errno==EINVAL)
		/* 
		FreeBSD libc readir returns errno=EINVAL if 
		fd is not a directory 
		*/
		return readuntil(fd,(unsigned char*)buf,len);

	return err;
}

size_t readall(int fd, unsigned char*buf, size_t len) /* FIXME rename: get() */
{
	ssize_t err;
	size_t e;

	if(len>SSIZE_MAX)
	{
		e = readall(fd,buf,SSIZE_MAX);
		if(e<SSIZE_MAX)
			return e;

		buf = buf + SSIZE_MAX;
		len = len - SSIZE_MAX;
	}

	err = ((errno=0),recv(fd,buf,len,MSG_WAITALL|MSG_NOSIGNAL));
	if(err>=0)
		return e; 
	if(err<0 && errno != ENOTSOCK)
		return 0;

	size_t i;
	for(i=0;i<len;)
	{
		waitread(fd,-1);
		err = ((errno=0),read(fd,buf+i,len-i));
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
size_t writeall(int fd, unsigned char*buf, size_t len) /* FIXME rename: put() */
{
	ssize_t err;
	err = ((errno=0),send(fd,buf,len,MSG_WAITALL|MSG_NOSIGNAL));
	if(err>=0)
		return err;
	if(err<0 && errno != ENOTSOCK)
		return 0;

	size_t i;
	for(i=0;i<len;)
	{
		waitwrite(fd,-1);
		err = ((errno=0),write(fd,buf+i,len-i));
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

size_t filename(int fd, char*buf, size_t len)
{
	int err; 
	dev_t dev;
	ino_t ino;	
	struct stat meta;

	if(len < 2)
		return ((errno = ENOMEM),-1);

	size_t e;
	if(fd==-1)
	{
		fd = ((errno=0),open(".",O_RDONLY|O_CLOEXEC));
		if(fd<0)
			return fd; 
		e = filename(fd,buf,len); 
		close2(fd); 
		return e;
	}

	err = fstat(fd,&meta);
	if(err<0)
		return err;
	if(!S_ISDIR(meta.st_mode))
		return ((errno=ENOTDIR),-1);

	dev = meta.st_dev;
	ino = meta.st_ino;

	int up;
	up = openat(fd,"..",O_RDONLY|O_CLOEXEC);
	if(up<0)
		return up;

	err = fstat(up,&meta);
	if(err<0)
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
	while(readentry(up,tmp,8192) != 0)
	{
		err = fstatat(up,tmp,&meta,AT_SYMLINK_NOFOLLOW);
		if(err<0)
			goto fail;
		if(meta.st_dev==dev && meta.st_ino==ino)
		{
			close(up);
			strncpy(buf,tmp,len);
			buf[len-1]='\0';
			return ((errno=0),strlen(buf));
		}
	}
	errno = ENOENT;

fail:
	close2(up);
	return -1;
} 

static int chdirfd(int fd)
{
	off_t offset;
	offset = lseek(fd,0,SEEK_CUR);

	int err;
	err = fchdir(fd);
	if(err<0)
		return -1;

	minio_cwd_offset = offset; 
	return 0;
}
int cd(int fs, char*path, int flags)
{
	int err;

	/*
	FIXME
	since we're providing a flags arg,
	we could support an mkpath()-based
	O_CREAT|O_EXCL behaviour.
	*/

	int fd;
	fd = openat(fs,path,flags&O_CLOEXEC);
	if(fd<0)
		return fd;

	struct stat meta;
	err = fstat(fd,&meta);
	if(err<0)
		return err;

	if(!S_ISDIR(meta.st_mode))
		return ((errno=ENOTDIR),-1);

	err = redirect(fs,fd,flags);
	if(err<0)
		return err;
	
	return ((errno=0),0);
}
int cdup(int fs, int flags)
{
	int err;

	char tmp[8192]; 
	size_t e;
	e = filename(fs,tmp,8192);
	if(e==0)
		return ((errno=ENOENT),-1);

	int up;
	if(fs==-1)
		up = open("..",O_CLOEXEC);
	else
		up = openat(fs,"..",O_CLOEXEC);
	if(up<0)
		return up;

	struct stat needle;
	if(fs==-1)
		err = stat(".",&needle);
	else
		err = fstat(fs,&needle);
	if(err<0)
		goto fail;

	if(!S_ISDIR(needle.st_mode))
	{
		errno=ENOTDIR;
		goto fail;
	}	

	struct stat haystack;
	do
	{
		e = gets2(up,tmp,8192);
		if(e==0)
			goto fail;

		err = fstatat(up,tmp,&haystack,AT_SYMLINK_NOFOLLOW);
		if(err<0)
			goto fail; 
	} 
	while (needle.st_dev!=haystack.st_dev && needle.st_ino!=haystack.st_ino);

	/* redirect correctly handles fs==-1 */
	/* redirect closes up on success */
	err = redirect(fs,up,flags&O_CLOEXEC);
	if(err<0)
		goto fail;

	return ((errno=0),0);

fail:
	err=errno;
		close(up);
	errno=err;

	return -1;
}

off_t seek(int fd, off_t offset)
{
	errno = 0;

	if(fd==-1)
	{
		minio_cwd_offset = offset;
		return minio_cwd_offset;
	}

	return lseek(fd,offset,SEEK_SET);
}
off_t tell(int fd)
{
	errno = 0;

	if(fd==-1)
		return minio_cwd_offset;

	return lseek(fd,0,SEEK_CUR);
}

int delete(char*path)
{
	int err; 
	size_t e;
	char tmp[8192];

	/* if it is a file */
	err = unlink(path);
	if(err==0)
		return ((errno=0),0);

	/* if it is a directory */
	int head;
	head = open(path,O_CLOEXEC|O_NOFOLLOW);
	if(head<0)
		return head;

	ssize_t stack=0;
	while(stack>=0 && stack<SSIZE_MAX-1)
	{
		/*
		the ordering of the contents of a
		directory may change after an entry
		is unlinked.  due to minio's FreeBSD
		gets2() workaround it does under
		FreeBSD.

		because we're unlinking every file we
		come accross, we don't need proper
		directory-walking behaviour, we can
		rewind and 'let the files come to us'.

		deleting a directory does not require
		proper cdup() behaviour, we just
		use this function to illustrait
		cdup().
		*/
		err = lseek(head,0,SEEK_SET);
		if(err<0)
			break;
		e = gets2(head,tmp,8192);
		if(e>0)
		{
			/* if it is a directory */
			err = cd(head,tmp,O_CLOEXEC|O_NOFOLLOW);
			if(err==0)
			{
				++stack;
				continue;
			}

			/* if it is a file */
			err = unlinkat(head,tmp,0);
			if(err<0)
				break;

			continue;
		}

		if(stack==0)
			break;

		e = filename(head,tmp,8192);
		if(e==0)
			break;

		err = cdup(head,O_CLOEXEC|O_NOFOLLOW);
		if(err<0)
			break;

		err = unlinkat(head,tmp,AT_REMOVEDIR);
		if(err<0)
			break;

		--stack;
	}
	err = rmdir(path);

	close2(head);

	return err;
}

int simplex(int simplexfd[2],int flags) 
{ 
	int fd[2];

	int err;
	err = socketpair(AF_LOCAL,SOCK_STREAM,0,fd); 
	if(err<0)
		return -1;

	if((flags&O_CLOEXEC)==O_CLOEXEC)
	{
		err = fcntl(fd[0], F_SETFD, FD_CLOEXEC);
		if(err<0)
			goto fail;
		err = fcntl(fd[1], F_SETFD, FD_CLOEXEC);
		if(err<0)
			goto fail; 
	}

	err = fcntl(fd[0], F_SETFL, flags);
	if(err<0)
		goto fail;

	err = fcntl(fd[1], F_SETFL, flags);
	if(err<0)
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
	if(err<0)
		return -1;

	if((flags&O_CLOEXEC)==O_CLOEXEC)
	{
		err = fcntl(fd[0], F_SETFD, FD_CLOEXEC);
		if(err<0)
			goto fail;
		err = fcntl(fd[1], F_SETFD, FD_CLOEXEC);
		if(err<0)
			goto fail; 
	}

	err = fcntl(fd[0], F_SETFL, flags);
	if(err<0)
		goto fail;

	err = fcntl(fd[1], F_SETFL, flags);
	if(err<0)
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
	if(err<0)
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
	if(err<0)
		return 0;

	return (data.revents & POLLOUT)==POLLOUT && 
	       (data.revents & POLLERR)!=POLLERR && 
	       (data.revents & POLLHUP)!=POLLHUP;
}
int waitread(int fd, int timelimit)
{
	struct pollfd data;
	data.fd = fd;
	data.events = POLLIN;

	int err;
	err = poll(&data,1,timelimit);
	if(err<0)
		return 0;

	return (data.revents & POLLIN)==POLLIN && 
	       (data.revents & POLLERR)!=POLLERR && 
	       (data.revents & POLLHUP)!=POLLHUP; 
}
int waitwrite(int fd,int timelimit)
{
	struct pollfd data;
	data.fd = fd;
	data.events = POLLOUT;

	int err;
	err = poll(&data,1,timelimit);
	if(err<0)
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
	if(err<0)
		return 0;

	return (data.revents & POLLERR)!=POLLERR && 
	       (data.revents & POLLHUP)!=POLLHUP; 
}
int redirect(int fd, int target, int flags) /* consider renaming rd() so it is analogus to cd() */
{
	int err;

	if(fd==-1 && target==-1)
		/* equivalent to chdir(".") or fchdir(open(".",O_RDONLY)); */
		return seek(-1,0);
	if(fd==-1)
	{
		err = chdirfd(target);
		if(err==0)
			close2(target);
		return err;
	}
	if(target==-1)
	{
		target = open(".",O_RDONLY|O_CLOEXEC);
		if(target<0)
			return target; 
	}

	err = dup2(target,fd);
	if(err<0)
		goto fail;

	/* no recourse if these fcntl()s fail */
	if((flags&O_CLOEXEC)==O_CLOEXEC)
		fcntl(fd,F_SETFD, FD_CLOEXEC);
	fcntl(fd,F_SETFL, flags);

	close2(target);
	return 0;

fail:
	close2(target);
	return -1;
}

int popen3(char*cmd, int stdin,int stdout,int stderr, pid_t*pid)
{ 
	int i;
	int o;
	int e;

	pid_t err;
	err = fork();
	if(err<0)
		return err;
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
	if(err<0)
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
	if(err<0)
		return -1;

	if(push==-1&&pull>-1)
	{ 
		flags = fcntl(pull,F_GETFD);
		if(flags<0)
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
		if(flags<0)
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
	if(err<0)
		return -1;

	err = popen3(cmd,channel[1],channel[1],-1,pid);
	if(err==0)
		return channel[0];

	err = errno;
	close(channel[0]);
	errno = err;

	return -1;
}

size_t write2(int fd, unsigned char*buf, size_t len) /* FIXME rename: push() */
{
	ssize_t err;
	size_t e;

	if(len>SSIZE_MAX)
	{
		e = write2(fd,buf,SSIZE_MAX);
		if(e<SSIZE_MAX)
			return e;

		buf = buf + SSIZE_MAX;
		len = len - SSIZE_MAX;
	}

	err = ((errno=0),send(fd,buf,len,MSG_NOSIGNAL));
	if(err<0 && errno != ENOTSOCK)
		return 0;

	return ((errno=0),write(fd,buf,len)); 
}
size_t puts2(int fd, char*string) /* FIXME rename: putln() */
{
	size_t len;
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

	size_t len;
	len = strlen(buf); 

	size_t e;
	e = puts2(fd,buf);
	if(e<len)
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
	if(err<0)
		return -1;
	return fcntl(fd,F_SETFL,err | O_NONBLOCK);
}
int blocking(int fd)
{
	int err;
	err = fcntl(fd,F_GETFL);
	if(err<0)
		return -1;
	return fcntl(fd,F_SETFL,err & ~O_NONBLOCK);
}
int cloexec(int fd)
{
	int err;
	err = fcntl(fd,F_GETFD);
	if(err<0)
		return -1;
	return fcntl(fd,F_SETFD,err | FD_CLOEXEC);
}
int noncloexec(int fd)
{
	int err;
	err = fcntl(fd,F_GETFD);
	if(err<0)
		return -1;
	return fcntl(fd,F_SETFD,err & ~FD_CLOEXEC);
}

int getflags(int fd)
{
	int flags;
	int err;

	err = fcntl(fd,F_GETFD);
	if(err<0)
		return -1;
	if(err&FD_CLOEXEC) flags = O_CLOEXEC; else flags = 0;

	err = fcntl(fd,F_GETFL);
	if(err<0)
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
	err = ioctl(tty,TIOCGWINSZ, &w);
	if(err<0)
		return -1;

	if(x!=NULL)
		*x = w.ws_col;
	if(y!=NULL)
		*y = w.ws_row;

	return 0;
}

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
	if(err==0||err<0)
		goto fail;

	if(err>1 && buf[0]<0)
	{
		/* it was a utf8 char or an xterm:alt+... key press */

		if((buf[0]&0x7F)=='B' && buf[2]=='\0')
		{
			/*
			in xterm:
			alt+(ascii values between 27 and A)
			alt+shift+(ascii values between 27 and A)
			are reported with a leading B character

			ctrl+alt+letter
			are reported with a leading B character

			(because ctrl+alt+letter characters are reported, 
			 by terminals as ascii values between 1 and 27) 

			we translate it into the same input keycodes
			that most other sofware terminals use.
			*/ 
			buf[0]='\033';
			buf[1]=buf[1]&0x7F;
		}
		else if((buf[0]&0x7F)=='C' && buf[2]=='\0')
		{ 
			/* 
			in xterm:
			alt+(ascii values between A and 127)
			alt+shift+(ascii values between A and 127) 
			are reported with a leading C character,
			and begin from where the "leading B" characters
			finish (ascii value A).

			we translate it into the same input keycodes
			that most other sofware terminals use.
			*/
			buf[0]='\033';
			buf[1]=(buf[1]&0x7F)+'A'-1;
		} 
		/*
		FIXME determine how these xterm-specific keycodes clash with utf8 characters.
		*/
	}

	if(buf[0]!='\033')
		/* we read a control+char or shift+char or regular char key press */
		goto pass;

	if(err==1)
	{
		/* usleep(1) gives us a 1 milisecond pause, long enough for read() to return the next char */
		usleep(1000);
		err += read(tty,buf+1,1);
		if(err==1+0||err==1+-1)
			/* it was an escapekey press */
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
	else if(buf[0]<0)
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

static int socket2(int family, int type, int protocol, int flags)
{
	int err;

#ifdef SOCK_CLOEXEC
	if(flags&O_CLOEXEC)
		type|=SOCK_CLOEXEC;
#endif
#ifdef SOCK_NONBLOCK
	if(flags&O_NONBLOCK)
		type|=SOCK_NONBLOCK;
#endif

	int sck;
	sck = socket(family,type,protocol);
	if(sck<0)
		goto fail;

#ifndef SOCK_CLOEXEC
	if(flags&O_CLOEXEC)
	{
		/* we don't have to worry about overwriting existing flags */
		err = fcntl(F_SETFD,sck,FD_CLOEXEC);
		if(err<0)
			goto fail;
	}
#endif
#ifndef SOCK_NONBLOCK
	if(flags&O_NONBLOCK)
	{
		/* we don't have to worry about overwriting existing flags */
		err = fcntl(F_SETFL,sck,O_NONBLOCK);
		if(err<0)
			goto fail;
	}	
#endif

	return ((errno=0),sck);

fail:
	err = errno;
		if(sck>=0)
			close(sck);
	errno = err;

	return -1;
}
int dial(char*hostname, short port, int flags)
{
	int err;

	struct addrinfo*info;
	struct addrinfo hint;

	memset(&hint,0,sizeof(struct addrinfo));
	hint.ai_family = AF_UNSPEC;     /* ipv6|ipv4 */
	hint.ai_socktype = SOCK_STREAM; /* insist on tcp-- Linux */
	hint.ai_protocol = IPPROTO_TCP; /* insist on tcp-- OSX */
	hint.ai_flags = 0;              /* only connect to localhost iff hostname==NULL */

	char service[32];
	snprintf(service,31,"%d",port); /* getaddrinfo only allows specifying the destination port via this mechanism */
	err = getaddrinfo(hostname,service,&hint,&info);
	if(err<0)
		/* FIXME how to return dns specific errors? */
		return -1;

	int cln;
	struct addrinfo*next;
	for(next=info;next!=NULL;next=next->ai_next)
	{
		cln=socket2(next->ai_family,next->ai_socktype,next->ai_protocol,flags);
		if(cln<0)
			goto fail;

		err=connect(cln,next->ai_addr,next->ai_addrlen);
		if(err<0)
			close(cln);
		else
			goto pass;
	}
	/* fallthrough */
fail:
	err=errno;
		freeaddrinfo(info);
		if(cln>=0)
			close(cln);
	errno=err;
	return -1;

pass:
	freeaddrinfo(info);
	return ((errno=0),cln);
} 
int tcp(char*hostname, short port, int flags)
{
	/**
	create a tcp server, that listens on @hostname:@port.

	to create a localhost-only tcp server,
	 srv=tcp("localhost",port,0);

	to create a publicaly available tcp server,
	 srv=tcp(NULL,port,0);

	to create a tcp server on only a specific ip address,
	 srv=tcp("xxx.xxx.xxx.xxx",port,0);
	or
	 srv=tcp("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx",port,0);

	returns server-fd on success, -1 on failure.
	**/

	/* FIXME this should loop through all getaddrinfo results like dial() */

	int err;
	struct addrinfo*info;
	struct addrinfo hint;

	memset(&hint,0,sizeof(struct addrinfo));
	hint.ai_family = AF_UNSPEC;     /* both ipv4 and ipv6 */
	hint.ai_socktype = SOCK_STREAM; /* insist on tcp -- Linux gives this hint precidence over ai_protocol */
	hint.ai_protocol = IPPROTO_TCP; /* insist on tcp --   OSX gives this hint precidence over ai_socktype */	
	/* 
	iff hostname==NULL then listen on ALL addresses the machine has 
	else listen only on hostname
	*/
	hint.ai_flags = AI_PASSIVE;

	char service[32];
	snprintf(service,31,"%d",port);
	err = getaddrinfo(hostname,service,&hint,&info);
	if(err!=0)
		/* FIXME how to return dns specific errors? */
		return -1;

	int srv;
	srv = socket2(info->ai_family, info->ai_socktype, info->ai_protocol, flags);
	if(srv<0)
		goto fail;

	err = bind(srv,info->ai_addr, info->ai_addrlen);
	if(err<0)
		goto fail;

	err = listen(srv,4096);
	if(err<0)
		goto fail;

	freeaddrinfo(info);

	return ((errno=0),srv);

fail:
	err=errno;
		freeaddrinfo(info);
		if(srv>=0)
			close(srv);
	errno=err;
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
	if(err<0)
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
		if(err<0)
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
	if(err<0)
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
	if(err<0)
	{
		close2(fd);
		return -1;
	}

	return fd;
}

