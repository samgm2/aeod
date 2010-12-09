/*
 * hciattach -n -s 115200 /dev/ttyUSB0 csr 115200 noflow
 */
#ifndef __ELAYER_H
#define __ELAYER_H									1
#define __ELAYER_LINUX								1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <termios.h>

#ifndef u8
	typedef unsigned char							u8;
#endif

#ifndef u16
	typedef unsigned short int						u16;
#endif

struct elayer {
	int												fd;
    fd_set											fds;
};


void elayer_error() {
	printf("ERROR\n");
	exit(-1);
}

void elayer_blink(u8 msg) {
	printf("BLNK:%d\n",msg);
}

	/* Write n bytes */
char elayer_write(struct elayer *ctx,u8 *bytes,u8 len) {
	u8 i;
	u8 b;
	u8 s = 0;
	for (i = 0;i < len;i ++) {
		usleep(100);
		b = bytes[i];
		s = write(ctx->fd,&b,1);
	}
	return 0;
}

	/* Return one byte, this is non blocking */
char elayer_read(struct elayer *ctx,u8 *byte) {
	int rbytes = 0;

	if ((rbytes = read(ctx->fd,byte,1)) > 0) {
		return 1;
	}

	/*
	if (select(ctx->fd+1, &ctx->fds, NULL, NULL, NULL)) {
		rbytes = read(ctx->fd,byte,1);
		return 1;
	}*/
	return 0;
}

void elayer_hwreset(struct elayer *ctx) {
	
}

void elayer_init(struct elayer *ctx) {
	ctx->fd = -1;
}


char elayer_priv_port(struct elayer *ctx,char *data) {
	struct termios port;
	ctx->fd = open((char*)data, O_RDWR | O_NONBLOCK);
//	fcntl(ctx->fd, F_SETFL, FASYNC);

	tcgetattr(ctx->fd,&port);
	port.c_cflag = B38400 | CS8 | CREAD | CLOCAL;
	port.c_lflag = 0;

	tcflush(ctx->fd, TCIFLUSH);
	if (tcsetattr(ctx->fd,TCSANOW,&port) < 0) {
		printf("Error:");
		perror("tcsetattr");
		return -1;
	}
	
	
	FD_ZERO(&ctx->fds);
    FD_SET(ctx->fd, &ctx->fds);
	return 0;
}

void elayer_free(struct elayer *ctx) {
	close(ctx->fd);
}


#endif /* __ELAYER_H */
