#include"minio.h"

#include<sys/socket.h>
#include<netinet/in.h>
#include<netinet/tcp.h>

#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<poll.h>

#include<unistd.h>

void test_terminal(void)
{
	char buf[8];

	charmode(0);

	print(1,"\nterminal-test\n\nhello: %s\n","substring");
	print(1,"\npress any key to continue\n");

	input(0,NULL);

	cls(1);
	underline(1);print(1,"keyboard test\n");normal(1);
	char key[16];
	input(0,key);

	print(1,"key: %c-%c-%c-%c\n",key[0],key[1],key[2],key[3]);
	print(1,"key: %d-%d-%d-%d\n",key[0],key[1],key[2],key[3]);

	bold(1);
	if(isctrlalt(key))
		print(1,"ctrl+alt+'%c'\n", character(key));
	else if(isctrl(key))
		print(1,"ctrl+'%c'\n", character(key));
	else if(isalt(key))
	{
		if(character(key)=='\t') 
			print(1,"alt+tab\n");
		else if(character(key)=='\n') 
			print(1,"alt+enter\n");
		else if(character(key)==' ') 
			print(1,"alt+space\n");
		else if(character(key)==127) 
			print(1,"alt+backspace\n");
		else 
			print(1,"alt+'%c'\n", character(key));
	}

	if(ischaracter(key))
		print(1,"character '%c'\n", character(key)); 

	if(isescape(key))
		print(1,"it is the escape key\n");
	if(isbackspace(key))
		print(1,"it is the backspace key\n");
	if(isenter(key))
		print(1,"it is the enter key\n"); 
	if(isspace(key))
		print(1,"it is the space key\n"); 
	if(istab(key))
		print(1,"it is the tab key\n"); 

	if(isarrow(key))
		print(1,"it is an arrow key\n");
	if(isup(key))
		print(1,"it is the up arrow key\n");
	if(isdown(key))
		print(1,"it is the down arrow key\n");
	if(isleft(key))
		print(1,"it is the left arrow key\n");
	if(isright(key))
		print(1,"it is the right arrow key\n");
	normal(1);

	print(1,"press any key to continue...\n");
	input(0,NULL);

	cls(1);
	color(1,0x000000FF,-1); print(1,"this is blue"); normal(1); print(1,"\n");
	color(1,0x0000FF00,-1); print(1,"this is green"); normal(1); print(1,"\n");
	color(1,0x00FF0000,-1); print(1,"this is red"); normal(1); print(1,"\n");

	int i;
	int j;

	for(i=0;i<16;++i)
	{
		for(j=0;j<16;++j)
		{
			palette(1,j,i); 
			print(1,"%0d,%0d ",j,i);
		}
		normal(1);
		print(1,"\n");
	}
	print(1,"press any key to continue...\n");
	input(0,buf);	
#if 0
	cls(1);
	for(i=0;i<256;++i)
	{
		for(j=0;j<256;++j)
		{
			websafe(1,j,i); 
			print(1,"%0d,%0d ",j,i);
		}
		normal(1);
		print(1,"\n");
	}
	print(1,"press any key to continue...\n");
	input(0,buf);	
#endif

	cls(1);
	websafe(1,180, -1); print(1,"this is red"); normal(1); print(1,"\n");
	websafe(1,30, -1); print(1,"this is green"); normal(1); print(1,"\n");
	websafe(1,5, -1); print(1,"this is blue"); normal(1); print(1,"\n");

	for(j=0;j<6;++j)
	{
		websafe(1,j*36+0*6+0,-1); 
		print(1,"%0d,%0d ",j,i);
	}
	normal(1);
	print(1,"\n");
	for(j=0;j<6;++j)
	{
		websafe(1,0*36+j*6+0,-1); 
		print(1,"%0d,%0d ",j,i);
	}
	normal(1);
	print(1,"\n");
	for(j=0;j<6;++j)
	{
		websafe(1,0*36+0*6+j,-1); 
		print(1,"%0d,%0d ",j,i);
	}
	normal(1);
	print(1,"\n");

	print(1,"press any key to continue...\n");
	input(0,buf);

	cls(1);
	for(i=0;i<24;++i)
	{
		for(j=0;j<24;++j)
		{
			greyscale(1,j,i); 
			print(1,"%0d,%0d ",j,i);
		}
		normal(1);
		print(1,"\n");
	}

	print(1,"press any key to continue...\n");
	input(0,buf); 

#if 0
	int i;
	int j;
	for(i=0;i<16;++i)
		for(j=0;j<16;++j)
		{
			palette(1,j,i);
			print(1,"this is palette colour %d,%d",j,i);
			normal(1);
			print(1,"\n");
		}
	for(i=0;i<256;++i)
		for(j=0;j<256;++j)
		{
			websafe(1,j,i);
			print(1,"this is websafe colour %d,%d",j,i);
			normal(1);
			print(1,"\n");
		}
#endif

	underline(1); print(1,"24bit rgb colour\n"); normal(1);
	print(1,"red:       "); for(i=0;i<128;++i) { color(1,(i*2)*256*256,-1); print(1,"%d ",i); normal(1); } print(1,"\n");
	print(1,"green:     "); for(i=0;i<128;++i) { color(1,(i*2)*256,-1); print(1,"%d ",i); normal(1); } print(1,"\n");
	print(1,"blue:      "); for(i=0;i<128;++i) { color(1,(i*2),-1); print(1,"%d ",i); normal(1); } print(1,"\n");

	print(1,"cyan:      "); for(i=0;i<128;++i) { color(1,(i*2)*256 + (i*2),-1); print(1,"%d ",i); normal(1); } print(1,"\n");
	print(1,"yellow:    "); for(i=0;i<128;++i) { color(1,(i*2)*256*256 + (i*2)*256,-1); print(1,"%d ",i); normal(1); } print(1,"\n");
	print(1,"magenta:   "); for(i=0;i<128;++i) { color(1,(i*2)*256*256 + (i*2),-1); print(1,"%d ",i); normal(1); } print(1,"\n");

	print(1,"greyscale: "); for(i=0;i<128;++i) { color(1,(i*2)*256*256 + (i*2)*256 + (i*2),-1); print(1,"%d ",i); normal(1); } print(1,"\n"); 

	print(1,"press any key to continue...\n");
	input(0,buf);

	charmode(0);

	cls(1);
	for(;;)
	{
		print(1,"input tester.  press ctrl+c to exit\n\n");

		input(0,key);

		print(1,"key: %c-%c-%c-%c\n",key[0],key[1],key[2],key[3]);
		print(1,"key: %d-%d-%d-%d\n",key[0],key[1],key[2],key[3]);

		bold(1);
		if(isctrlalt(key))
			print(1,"ctrl+alt+'%c'\n", character(key));
		else if(isctrl(key))
			print(1,"ctrl+'%c'\n", character(key));
		else if(isalt(key))
		{
			if(character(key)=='\t') 
				print(1,"alt+tab\n");
			else if(character(key)=='\n') 
				print(1,"alt+enter\n");
			else if(character(key)==' ') 
				print(1,"alt+space\n");
			else if(character(key)==127) 
				print(1,"alt+backspace\n");
			else 
				print(1,"alt+'%c'\n", character(key));
		}

		if(ischaracter(key))
			print(1,"character '%c'\n", character(key)); 

		if(isescape(key))
			print(1,"it is the escape key\n");
		if(isbackspace(key))
			print(1,"it is the backspace key\n");
		if(isenter(key))
			print(1,"it is the enter key\n"); 
		if(isspace(key))
			print(1,"it is the space key\n"); 
		if(istab(key))
			print(1,"it is the tab key\n"); 

		if(isarrow(key))
			print(1,"it is an arrow key\n");
		if(isup(key))
			print(1,"it is the up arrow key\n");
		if(isdown(key))
			print(1,"it is the down arrow key\n");
		if(isleft(key))
			print(1,"it is the left arrow key\n");
		if(isright(key))
			print(1,"it is the right arrow key\n");
		normal(1);

		if(isctrl(key) && character(key) == 'c')
			quit(0);
#if 0
		else if(isctrl(key) && character(key) == 'z')
			ctrlz();
#endif
	}

	linemode(0);
}

#if 0
void crash(char*error, ...)
{
	int err;
	err = errno;

	va_list args; 
	va_start(args,error);
		fprintf(stderr,error,args);
	va_end(args);

	fprintf(stderr,": %d: %s\n",err,strerror(err));

	exit(1);
}
#endif

void test_tcp()
{
	int err;
	int srv;
	int cln;
	char buf[8192];

	printf("test: ipv4 io\n");
	pid_t pid;
	pid=fork();
	if(pid==-1)
		crash("fail: fork"); 
	if(pid==0)
	{
		printf("test: server: create tcp/ip4 server\n");
		srv = tcp4(9889,0);
		if(srv==-1)
			crash("fail: server: could not create tcp/ip4 server");
		printf("pass: server: created tcp/ip4 server\n");

		struct sockaddr_storage tmp;
		size_t len;
		cln = accept(srv,(struct sockaddr*)&tmp,&len);
		if(cln == -1)
			crash("fail: server: accept");
		printf("pass: server: accepted client connection\n");

		printf("test: server: sending string\n");
		print(cln,"hello tcp/ipv4!\n");
		printf("pass: server: string sent\n"); 

		close(srv);
		close(cln);

		exit(0);
	}
	else
	{
		sleep(4);

		printf("test: client: dial tcp/ip4 server\n");
		cln = dial("localhost",9889,0);
		if(cln==-1)
			crash("fail: client: could not dial tcp/ip4 server\n");
		printf("pass: client: dialed tcp/ip4 sever\n");

		printf("test: client: gets2() string\n");
		gets2(cln,buf,8192);
		printf("pass: client: read %s",buf);

		close(cln);
	}
	printf("pass: ipv4 io\n\n");

	printf("test: ipv6 io\n");
	pid = fork();
	if(pid==-1)
		crash("fail: fork");
	if(pid==0)
	{
		printf("test: server: create tcp/ip6 server\n");
		srv = tcp6(8998,0);
		if(srv==-1)
			crash("fail: server: could not create tcp/ip6 server");
		printf("pass: server: created tcp/ip6 server\n");

		struct sockaddr_storage tmp;
		size_t len;
		cln = accept(srv,(struct sockaddr*)&tmp,&len);
		if(cln == -1)
			crash("fail: server: accept");
		printf("pass: server: accepted client connection\n");

		printf("test: server: sending string\n");
		print(cln,"hello tcp/ipv6!\n");
		printf("pass: server: string sent\n"); 

		close(srv);
		close(cln);

		exit(0);
	}
	else
	{
		sleep(4);

		printf("test: client: dial tcp/ip6 server\n");
		cln = dial("localhost",8998,0);
		if(cln==-1)
			crash("fail: client: could not dial tcp/ip6 server\n");
		printf("pass: client: dialed tcp/ip6 sever\n");

		printf("test: client: gets2() string\n");
		gets2(cln,buf,8192);
		printf("pass: client: read %s",buf);

		close(cln);
	}
	printf("pass: ipv6 io\n\n");

	printf("test: dialing www.wikipedia.org\n");
	cln = dial("www.wikipedia.org",80,0);
	if(cln==-1)
		crash("fail: could not dail www.wikipedia.org\n");
	printf("pass: dialed www.wikipedia.org\n");

	printf("test: sending generic http get request\n");
	print(cln,"GET /\r\n\r\n");
	printf("pass: sent\n");

	printf("test: pulling response to http get /\n");
	err = gets2(cln,buf,128);
	if(err==-1||err==0)
		crash("fail: gets2 could not read from fd\n"); 
	buf[8191] = '\0';
	printf("debug: gets2 pulled:\n%s\n",buf);
	printf("pass: http get\n");

	close(cln);
}

void test_popen3(void)
{
	int err;

	printf("test: make duplex\n");
	int subp[2];
	err = duplex(subp,O_CLOEXEC);
	if(err==-1)
	{
		perror("fail: make duplex");
		exit(1);
	}
	printf("pass: make duplex\n");

	printf("test: launch subshell on end of duplex\n");
	err = popen3("echo 'hello there'\n",subp[1],subp[1],-1,NULL);
	if(err==-1)
	{
		perror("fail: launch subshell on end of dupelx");
		exit(1);
	}
	printf("pass: launch subshell on end of dupelx\n");

	printf("test: read pipeline output\n");
	char buf[8192];
	err = gets2(subp[0],buf,64);
	if(err==-1)
	{
		perror("fail: read pipeline output");
		exit(1);
	}
	printf("debug: pipeline output '%s'\n",buf);
	printf("pass: read pipeline output\n");

	close(subp[0]);	
}
void test_launch(void)
{
	int err;

	int sp; 
	printf("test: launch subshell\n");
	sp = launch("echo 'hello there'\n",NULL);
	if(sp==-1)
	{
		perror("fail: launch subshell");
		exit(1);
	}
	printf("pass: launch subshell\n");

	printf("test: read pipeline output\n");
	char buf[8192];
	err = gets2(sp,buf,64);
	if(err==-1)
	{
		perror("fail: read pipeline output");
		exit(1);
	}
	printf("debug: pipeline output '%s'\n",buf);
	printf("pass: read pipeline output\n");

	close(sp);	
}

void test_filter(void)
{
	int err;

	printf("test: make simplex\n");
	int subp[2];
	err = simplex(subp,O_CLOEXEC);
	if(err==-1)
	{
		perror("fail: make simplex");
		exit(1);
	}
	printf("pass: make simplex (fd=%d,%d)\n",subp[0],subp[1]);
#if 1
	printf("test: push process 'echo abcde' to pipeline\n");
	err = filter(subp[0],"echo abcde",-1,NULL);
	if(err==-1)
	{
		perror("fail: push process");
		exit(1);
	}
	printf("pass: push process\n");
#endif
	printf("test: push process \"tr 'a' 'A'\" to pipeline\n");
	err = filter(subp[0],"tr 'a' 'A'",-1,NULL);
	if(err==-1)
	{
		perror("fail: push process");
		exit(1); 
	}
	printf("pass: push process \"tr 'a' 'A'\" to pipeline\n");

	printf("test: push process \"tr 'c' 'C'\" to pipeline\n");
	err = filter(subp[0],"tr 'c' 'C'",-1,NULL);
	if(err==-1)
	{
		perror("fail: push process");
		exit(1); 
	}
	printf("pass: push process \"tr 'c' 'C'\" to pipeline\n"); 

	printf("test: read pipeline output\n");
	char buf[8192];
	err = gets2(subp[0],buf,64);
	if(err==-1)
	{
		perror("fail: read pipeline output");
		exit(1);
	}
	printf("debug: pipeline output '%s'\n",buf);
	printf("pass: read pipeline output\n");

	system("ps");
	close2(subp[0]);
	close2(subp[1]);
	system("ps");
}

void test_redirect()
{
	int err;

	int ch1[2];
	int ch2[2];

	printf("test: making simplex ch1\n");
	err = simplex(ch1,O_CLOEXEC);
	if(err==-1)
	{
		perror("fail: making simplex ch1");
		exit(1);
	}
	printf("pass: making simplex ch1\n");

	printf("test: making simplex ch2\n");
	err = simplex(ch2,O_CLOEXEC);
	if(err==-1)
	{
		perror("fail: making simplex ch2");
		exit(1);
	}
	printf("pass: making simplex ch2\n");

	printf("test: writing 'ch1 data' to ch1\n");
	err = print(ch1[1],"ch1 data\n");
	printf("debug: print returned %d\n",err);

	printf("test: writing 'ch2 data' to ch2\n");
	err = print(ch2[1],"ch2 data\n");
	printf("debug: print returned %d\n",err);

	printf("test: redirecting ch1 read fd to read from ch2\n");
	err = redirect(ch1[0],ch2[0],0);
	if(err==-1)
	{
		perror("fail: redirecting");
		exit(1);
	}
	printf("pass: redirecting ch1 read fd to read from ch2\n");

	char buf[8192];

	printf("test: reading from redirected fd\n");
	err = gets2(ch1[0],buf,8192);
	printf("debug: err=%d\n",err);
	if(err==-1)
	{
		perror("fail: gets2");
		exit(1);
	}
	printf("debug: read '%s'\n",buf);

	if(strcmp(buf,"ch2 data\n"))
	{
		printf("fail: read '%s'-- expected 'ch2 data'\n",buf);
		exit(1);
	}
	printf("pass: reading from redirected fd\n");

	close(ch1[0]);
	close(ch1[1]);
	/* ch2[0] was closed by the redirection */
	close(ch2[1]);
}

void test_mkpath(void)
{
	int err;

	printf("test: make a directory with mkpath\n");
	err = mkpath("tmpdir0",0755);
	if(err!=0)
	{
		perror("fail: mkpath");
		exit(1);
	}
	printf("pass: make a directory with mkpath\n");

	int fd;
	printf("test: check for tmpdir0\n");
	fd = open("tmpdir0",O_RDONLY);
	if(fd==-1)
	{
		perror("fail: open");
		exit(1);
	}
	close(fd);
	printf("pass: check for tmpdir0\n");

	printf("test: make a string of directories with mkpath\n");
	err = mkpath("tmpdir0/tmpdir1/tmpdir2/tmpdir3",0755);
	if(err!=0)
	{
		printf("fail: mkpath returned %d: %s\n",err,strerror(errno));
		exit(1);
	}
	printf("pass: make a string of directies with mkpath\n");

	printf("test: check for string of directories\n");
	err = open("tmpdir0/tmpdir1/tmpdir2/tmpdir3",O_RDONLY);
	if(err==-1)
	{
		perror("fail: open");
		exit(1);
	}
	printf("pass: check for string of directories\n");

	fd = open("/tmpdir0/tmpdir1/tmpfile",O_CREAT,0755);
	close(fd);

	printf("test: attempt to make a string of directories with an invalid path\n");
	err = mkpath("tmpdir0/tmpdir1/tmpfile/tmpdir3/tmpdir4",0755);
	if(err!=0)
	{
		printf("fail: mkpath returned %d: %s\n",err,strerror(errno));
		exit(1);
	}
	printf("pass: attempt to make a string of directories with an invalid path failed\n");

	printf("test: make a new path to a new file\n");
	char*filename="tmpdir0/tmpdir1/tmpdir2/tmpdir3/tmpdir4/tmpdir5/newfile0";
	(mkpath(filename,0755)==0 && rmdir(filename)==0 && (fd = open(filename,O_CREAT,0755))) || (fd=-1);
	if(fd==-1)
	{
		perror("fail: open");
		exit(1);
	}
	printf("pass: make a new path to a new file\n");
	close(fd); 

	/* FIXME test new file can be opened and is a file */

	printf("test: attempt to make a new path to an existing file\n");
	filename="tmpdir0/tmpdir1/tmpdir2/tmpdir3/tmpdir4/tmpdir5/newfile0";
	fd = -1;
	(mkpath(filename,0755)==0 && rmdir(filename)==0 && (fd = open(filename,O_CREAT,0755))) || (fd=-1);
	if(fd>=0)
	{
		perror("fail: open");
		exit(1);
	} 
	printf("pass: attempt to make a new path to an existing file failed\n");

	printf("test: attempt to make a new path and file to an existing dir\n");
	fd = -1;
	filename="tmpdir0/tmpdir1/tmpdir2/tmpdir3/tmpdir4/tmpdir5/tmpdir6";
	err = mkpath(filename,0755);
	if(err!=0)
	{
		perror("fail: failed to make obstacle dir");fflush(stderr);
		exit(1);
	}

	err = mkpath(filename,0755);
	if(err==0)
	{
		printf("debug: mkpath returend %d\n",err);
		err = rmdir(filename);
		if(err==0)
		{
			printf("fail: we deleted a directory that already existed\n");
			exit(1);
		}
		fd = open(filename,O_CREAT);
		if(fd!=-1)
		{
			printf("fail: we should not have been able to clobber the existing dir\n");
			exit(1);
		}
	}
	printf("pass: attempt to make a new path and file to an existing dir failed\n");

	printf("test: attempt to mkpath *in* a dir we have no permission to access\n");
	err = chmod("tmpdir0/tmpdir1",0111);
	if(err==-1)
	{
		perror("fail: chmod");
		exit(1);
	}
	filename="tmpdir0/tmpdir1/forbiddendir";
	err = mkpath(filename,0755);
	printf("debug: mkpath returned %d: %s\n",err,strerror(errno));
	if(err==0)
	{
		printf("fail: somehow mkpath managed it\n");
		exit(1);
	}
	printf("pass: attempt to mkpath *in* a dir we have no permission to access\n");

	printf("test: attmpt to mkpath *through* a dir we have no permission to access\n");
	filename="tmpdir0/tmpdir1/tmpdir2/tmpdir3/forbiddendir";
	err = mkpath(filename,0755);
	printf("debug: mkpath returned %d: %s\n",err,strerror(errno));
	if(err!=0)
	{
		perror("fail: mkpath");fflush(stderr);
		exit(1);
	}
	printf("pass: attmpt to mkpath *though* a dir we have no permission to access\n"); 

	chmod("tmpdir0/tmpdir1",0755);
	system("rm -r tmpdir0");
}

void test_mkserver(void)
{
	int err;

	printf("test: create a local server\n");
	int srv;
	srv = mkserver("tmpsrv",O_NONBLOCK);
	if(srv==-1)
	{
		perror("fail: mkserver");
		exit(1);
	} 
	printf("pass: create a local server\n");

	printf("test: open2 a local server\n");
	int cln;
	cln = open2("tmpsrv",O_RDWR);
	if(cln==-1)
	{
		perror("fail: open2");
		exit(1);
	} 
	printf("pass: open2 a local server\n");

	printf("test: take a connection to a local server\n");
	int con;
	con = take(srv,O_CLOEXEC);
	if(con==-1)
	{
		perror("fail: take");
		exit(1);
	}
	printf("pass: take a connection to a local server\n");

	printf("test: send message from server to client\n");
	err = print(con,"1234\n");
	if(err!=0)
	{
		perror("fail: write");
		exit(1);
	}
	printf("pass: send message from server to client\n");

	char buf[8192];

	printf("test: recv message from server to client\n");
	errno = 0;
	err = gets2(cln,buf,8192);
	printf("debug: gets2 returned %d: %s\n",err,strerror(errno));
	if(err==-1)
	{
		perror("fail: gets2");
		exit(1);	
	}
	printf("debug: gets2 got '%s'\n",buf);
	printf("pass: recv message from server to client\n");

	printf("test: send message from client to server\n");
	err = print(cln,"4321\n");
	if(err!=0)
	{
		perror("fail: write");
		exit(1);
	}
	printf("pass: send message from client to server\n");

	printf("test: recv message from client to server\n");
	errno = 0;
	err = gets2(con,buf,8192);
	printf("debug: gets2 returned %d: %s\n",err,strerror(errno));
	if(err==-1)
	{
		perror("fail: gets2");
		exit(1);	
	}
	printf("debug: gets2 got '%s'\n",buf);
	printf("pass: recv message from client to server\n");

	/* TODO send an fd from client to server */
	/* TODO recv an fd from client to server */
	/* TODO send data over it */

	close2(srv);
	close2(con);
	close2(cln);

	unlink("tmpsrv");
}

void test_delete(void)
{
	int err;

	printf("test: make an empty file\n");
	err = open("tmpfile",O_CREAT,0755);
	if(err==-1)
	{
		perror("fail: mkdir");
		exit(1);
	}
	close(err);
	printf("pass: make a empty file\n"); 

	printf("test: delete an empty file\n");
	err = delete("tmpfile");
	if(err==-1)
	{
		perror("fail: delete");
		exit(1);
	}
	printf("pass: delete an empty file\n");

	printf("test: make an empty directory\n");
	err = mkdir("tmpdir",0755);
	if(err==-1)
	{
		perror("fail: mkdir");
		exit(1);
	}
	printf("pass: make a empty directory\n");

	printf("test: delete empty directory\n");
	err = delete("tmpdir");
	if(err==-1)
	{
		perror("fail: delete");
		exit(1);
	}
	printf("pass: delete empty directory\n");

	system("rm -r tmpdir");

	printf("test: make a 1-level directory\n");
	err = mkdir("tmpdir",0755);
	if(err==-1)
	{
		perror("fail: mkdir");
		exit(1);
	}
	err = open("tmpdir/tmpfile0",O_CREAT,0666);
	if(err==-1)
	{
		perror("fail: open");
		exit(1);
	}
	close(err);
	err = open("tmpdir/tmpfile1",O_CREAT,0666);
	if(err==-1)
	{
		perror("fail: open");
		exit(1);
	}
	close(err);
	err = open("tmpdir/tmpfile2",O_CREAT,0666);
	if(err==-1)
	{
		perror("fail: open");
		exit(1);
	}
	close(err);
	err = open("tmpdir/tmpfile3",O_CREAT,0666);
	if(err==-1)
	{
		perror("fail: open");
		exit(1);
	}
	close(err);
	printf("pass: make a 1-level directory\n"); 

	printf("test: delete a 1-level directory\n");
	err = delete("tmpdir");
	if(err==-1)
	{
		perror("fail: delete");
		exit(1);
	}
	printf("pass: delete a 1-level directory\n");

	system("rm -r tmpdir");

	printf("test: make an N-level directory\n");
	mkdir("tmpdir",0755);
	close(open("tmpdir/tmpfile00",O_CREAT,0666));
	close(open("tmpdir/tmpfile01",O_CREAT,0666));
	close(open("tmpdir/tmpfile02",O_CREAT,0666));
	close(open("tmpdir/tmpfile03",O_CREAT,0666));
	mkdir("tmpdir/tmpdir0",0755);
	close(open("tmpdir/tmpdir0/tmpfile04",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpfile05",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpfile06",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpfile07",O_CREAT,0666));
	mkdir("tmpdir/tmpdir1",0755);
	close(open("tmpdir/tmpdir1/tmpfile08",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpfile09",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpfile10",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpfile11",O_CREAT,0666));
	mkdir("tmpdir/tmpdir2",0755);
	close(open("tmpdir/tmpdir2/tmpfile12",O_CREAT,0666));
	close(open("tmpdir/tmpdir2/tmpfile13",O_CREAT,0666));
	close(open("tmpdir/tmpdir2/tmpfile14",O_CREAT,0666));
	close(open("tmpdir/tmpdir2/tmpfile15",O_CREAT,0666));
	mkdir("tmpdir/tmpdir3",0755);
	close(open("tmpdir/tmpdir3/tmpfile16",O_CREAT,0666));
	close(open("tmpdir/tmpdir3/tmpfile17",O_CREAT,0666));
	close(open("tmpdir/tmpdir3/tmpfile18",O_CREAT,0666));
	close(open("tmpdir/tmpdir3/tmpfile19",O_CREAT,0666));

	mkdir("tmpdir/tmpdir0/tmpdir4",0755);
	close(open("tmpdir/tmpdir0/tmpdir4/tmpfile20",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir4/tmpfile21",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir4/tmpfile22",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir4/tmpfile23",O_CREAT,0666));
	mkdir("tmpdir/tmpdir0/tmpdir5",0755);
	close(open("tmpdir/tmpdir0/tmpdir5/tmpfile24",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir5/tmpfile25",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir5/tmpfile26",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir5/tmpfile27",O_CREAT,0666));
	mkdir("tmpdir/tmpdir0/tmpdir6",0755);
	close(open("tmpdir/tmpdir0/tmpdir6/tmpfile28",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir6/tmpfile29",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir6/tmpfile30",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir6/tmpfile31",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir6/tmpfile32",O_CREAT,0666));

	mkdir("tmpdir/tmpdir1/tmpdir7",0755);
	close(open("tmpdir/tmpdir1/tmpdir7/tmpfile33",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpdir7/tmpfile34",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpdir7/tmpfile35",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpdir7/tmpfile36",O_CREAT,0666));
	mkdir("tmpdir/tmpdir1/tmpdir8",0755);
	close(open("tmpdir/tmpdir1/tmpdir8/tmpfile37",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpdir8/tmpfile38",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpdir8/tmpfile39",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpdir8/tmpfile40",O_CREAT,0666));
	mkdir("tmpdir/tmpdir1/tmpdir9",0755);
	close(open("tmpdir/tmpdir1/tmpdir9/tmpfile41",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpdir9/tmpfile42",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpdir9/tmpfile43",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir9/tmpfile44",O_CREAT,0666));

	printf("pass: make an N-level directory\n"); 

	printf("test: delete an N-level directory\n");
	err = delete("tmpdir");
	if(err==-1)
	{
		perror("fail: delete");
		exit(1);
	}
	printf("pass: delete an N-level directory\n");

	printf("test: make an N-level directory\n");
	mkdir("tmpdir",0755);
	close(open("tmpdir/tmpfile00",O_CREAT,0666));
	close(open("tmpdir/tmpfile01",O_CREAT,0666));
	close(open("tmpdir/tmpfile02",O_CREAT,0666));
	close(open("tmpdir/tmpfile03",O_CREAT,0666));
	mkdir("tmpdir/tmpdir0",0755);
	close(open("tmpdir/tmpdir0/tmpfile04",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpfile05",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpfile06",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpfile07",O_CREAT,0666));
	mkdir("tmpdir/tmpdir1",0755);
	close(open("tmpdir/tmpdir1/tmpfile08",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpfile09",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpfile10",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpfile11",O_CREAT,0666));
	mkdir("tmpdir/tmpdir2",0755);
	close(open("tmpdir/tmpdir2/tmpfile12",O_CREAT,0666));
	close(open("tmpdir/tmpdir2/tmpfile13",O_CREAT,0666));
	close(open("tmpdir/tmpdir2/tmpfile14",O_CREAT,0666));
	close(open("tmpdir/tmpdir2/tmpfile15",O_CREAT,0666));
	mkdir("tmpdir/tmpdir3",0755);
	close(open("tmpdir/tmpdir3/tmpfile16",O_CREAT,0666));
	close(open("tmpdir/tmpdir3/tmpfile17",O_CREAT,0666));
	close(open("tmpdir/tmpdir3/tmpfile18",O_CREAT,0666));
	close(open("tmpdir/tmpdir3/tmpfile19",O_CREAT,0666));

	mkdir("tmpdir/tmpdir0/tmpdir4",0755);
	close(open("tmpdir/tmpdir0/tmpdir4/tmpfile20",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir4/tmpfile21",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir4/tmpfile22",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir4/tmpfile23",O_CREAT,0666));
	mkdir("tmpdir/tmpdir0/tmpdir5",0755);
	close(open("tmpdir/tmpdir0/tmpdir5/tmpfile24",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir5/tmpfile25",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir5/tmpfile26",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir5/tmpfile27",O_CREAT,0666));
	mkdir("tmpdir/tmpdir0/tmpdir6",0755);
	close(open("tmpdir/tmpdir0/tmpdir6/tmpfile28",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir6/tmpfile29",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir6/tmpfile30",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir6/tmpfile31",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir6/tmpfile32",O_CREAT,0666));

	mkdir("tmpdir/tmpdir1/tmpdir7",0755);
	close(open("tmpdir/tmpdir1/tmpdir7/tmpfile33",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpdir7/tmpfile34",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpdir7/tmpfile35",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpdir7/tmpfile36",O_CREAT,0666));
	mkdir("tmpdir/tmpdir1/tmpdir8",0755);
	close(open("tmpdir/tmpdir1/tmpdir8/tmpfile37",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpdir8/tmpfile38",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpdir8/tmpfile39",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpdir8/tmpfile40",O_CREAT,0666));
	mkdir("tmpdir/tmpdir1/tmpdir9",0755);
	close(open("tmpdir/tmpdir1/tmpdir9/tmpfile41",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpdir9/tmpfile42",O_CREAT,0666));
	close(open("tmpdir/tmpdir1/tmpdir9/tmpfile43",O_CREAT,0666));
	close(open("tmpdir/tmpdir0/tmpdir9/tmpfile44",O_CREAT,0666)); 
	printf("pass: make an N-level directory\n"); 

	printf("test: make a file in the N-level directory un-deletable\n");
	err = chmod("tmpdir/tmpdir1/tmpdir8",0000);
	if(err==-1)
	{
		perror("fail: chmod");
		exit(1);
	}
	printf("pass: make a dir in the N-level directory un-deletable\n");

	printf("test: attempt to delete an N-level directory containing an un-deletable subdir\n");
	//err = 0;
	err = delete("tmpdir");
	printf("debug: contents of tmpdir:\n");
	system("find tmpdir"); 
	printf("debug: delete returned %d: %s\n",err,strerror(errno));
	if(err==0)
	{
		printf("fail: delete managed to delete a dir containing an undeletable file\n");
		exit(1);
	}
	printf("pass: attempt to delete an N-level directory containing an un-deletable subdir failed\n");

	chmod("tmpdir/tmpdir1/tmpdir8",0755);
	system("rm -r tmpdir");

}
void test_gets2()
{
	int err;

	printf("test: make a 1-level directory\n");
	err = mkdir("tmpdir",0755);
	if(err==-1)
	{
		perror("fail: mkdir");
		exit(1);
	}
	err = open("tmpdir/tmpfile0",O_CREAT,0666);
	if(err==-1)
	{
		perror("fail: open");
		exit(1);
	}
	close(err);
	err = open("tmpdir/tmpfile1",O_CREAT,0666);
	if(err==-1)
	{
		perror("fail: open");
		exit(1);
	}
	close(err);
	err = open("tmpdir/tmpfile2",O_CREAT,0666);
	if(err==-1)
	{
		perror("fail: open");
		exit(1);
	}
	close(err);
	err = open("tmpdir/tmpfile3",O_CREAT,0666);
	if(err==-1)
	{
		perror("fail: open");
		exit(1);
	}
	close(err);
	printf("pass: make a 1-level directory\n"); 

	int fd;

	printf("test: open 1-level directory\n");
	fd = open("tmpdir",O_RDONLY);
	if(err==-1)
	{
		perror("fail: open");
		exit(1);
	}
	printf("pass: open 1-level directory\n");

	char buf[8192];
	
	printf("test: list 1-level directory\n");
	for(err=0;err>=0;)
	{
		err = gets2(fd,buf,8192);
		printf("debug: gets2 returned %d\n",err);
		if(err==-1)
		{
			perror("fail: gets2");
			exit(1);
		}
		if(err==0)
			break;
		printf("debug: file: %s\n",buf);
	}
	printf("debug: end-of-dir\n");
	printf("pass: list 1-level directory\n");

	close(fd);

	system("rm -r tmpdir");
}

int main(int argc, char*args[])
{
	test_mkpath(); 
	test_gets2();
	test_delete();
	//test_mkserver();

	//test_redirect();

	//test_popen3();
	test_launch();
	//test_filter(); 
	//test_tcp(); 

	//test_terminal(); 

	quit(0);
	return 0;
}

