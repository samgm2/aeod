#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>



int main(int argc,char *argv[]) {
	struct sockaddr_l2 addr;
	int sock,len;
	char message[32];
	
    if (argc < 3) {
		fprintf(stderr, "usage: %s <message> <bt_addr>\n", argv[0]);
		fprintf(stderr, "example: %s Hello 00:02:78:02:56:CF\n", argv[0]);
		exit(-1);
    }
	
	memset(&addr,0,sizeof(struct sockaddr_l2));
	
	
	addr.l2_family = AF_BLUETOOTH;
	addr.l2_psm = htobs(0x1001);
	str2ba( argv[2], &addr.l2_bdaddr );
	
	printf("Socket\n");
		/* Create new Bluetooth socket */
	if ((sock = socket(AF_BLUETOOTH,SOCK_DGRAM , BTPROTO_L2CAP)) < 0) {
		perror("socket");
		return -1;
	}
	
	printf("Connect\n");
	if ((connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_l2))) < 0) {
		perror("connect");
		close(sock);
		return -1;
	}
	memset(message,0,32);
	memcpy(message,argv[1],strlen(argv[1]));
	
	printf("Write:%s\n",message);
	if ((len = write(sock, message, 5)) < 0) {
		perror("write");
		close(sock);
		return -1;
	}
	
	printf("Read\n");
	memset(message,0,32);
	if ((len = read(sock,  message,32)) < 0) {
		perror("write");
		close(sock);
		return -1;
	}
	
	printf("Got:%s\n",message);
	
	close(sock);
	return 0;
}
