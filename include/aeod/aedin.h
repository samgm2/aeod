/*
 * H4 transport implementation
 * 
 * Provides command, event and now also SCO,ACL data.
 * 
 * NOTE:
 * SCO is not newer tested ;=P
 *
 * there is 2 buffer counters,
 * 
 * len:
 * which is counter for received data, can't be bigger than AEDIN_BUFF
 * 
 * t_cnt_virt:
 * counter for received data, this can exeed limit of AEDIN_BUFF but
 * all bytes which overflow AEDIN_BUFF are dismissed
 * 
 * 
 */
#include "elayer_avr.h"
	/* 
	 * CONNECTION ORIENTED L2CAP
	 * - data max len = AEDIN_BUFF - 4
	 * -> len + channel id
	 * 
	 * CONNECTIONLESS L2CAP
	 * - data max len = AEDIN_BUFF - 6
	 * -> len + channel id + psm
	 */
#define AEDIN_BUFF					24	/* This is for HCI payload, eg ACL data, data portion */

	/* HCI cmd */
#define AEDIN_H4_CMD				1
	/* ACL */
#define AEDIN_H4_ACL				2
	/* SCO */
#define AEDIN_H4_SCO				3
	/* HCI event */
#define AEDIN_H4_EVENT				4

#define AEDIN_FLAGS_READY			0x10

struct aedin {
		/* harware send and rec layer */
	struct elayer					layer;
		/* Internal flags */
	 u8						flags;
		/* allways packet type */
	 u8						type;
		/* either when type is:
		 * -> cmd, ident is opcode
		 * -> data, ident is connection handle
		 * -> event, ident is event code */
	 u16					ident;
	
	
		/* buff where len point */
	 u8						buff[AEDIN_BUFF];
		/* parameter len, can't exeed AEDIN_BUFF */
	 u16					len;
		/* cnt for all received data */
	 u16					cnt_virt;
	 u16					cnt_real;
};

	/* Reset state */
void aedin_reset(struct aedin *ctx) {
	ctx->type = 0;
	ctx->len = 0;
	ctx->cnt_real = 0;
	ctx->cnt_virt = 0;
	ctx->flags = 0;
}

	/* This just initialize the struct, all null */
void aedin_init(struct aedin *ctx) {
	elayer_init(&ctx->layer);
	aedin_reset(ctx);
}

	/* Perform close stuff ?
	 * - meaningless only for linux */
void aedin_free(struct aedin *ctx) {
	elayer_free(&ctx->layer);
}


	/* Send one command and its params */
void aedin_cmd_send(struct aedin *ctx,u16 cmd,u8 *data,u8 len) {
	u8 buff[6];
		/* This is */
	buff[0] = AEDIN_H4_CMD;
	((u16*)&buff[1])[0] = cmd;
	buff[3] = len;
	
		/* Send control header, H4 + HCICMD */
	elayer_write(&ctx->layer,buff,4);
		/* Send data */
	elayer_write(&ctx->layer,data,len);
}

u8 aedin_receive(struct aedin *ctx) {
	u8 byte;
	
		/* Reset counters */
	if (ctx->flags & AEDIN_FLAGS_READY) {
		aedin_reset(ctx);
	}
	
	if (elayer_read(&ctx->layer,&byte)) {
/*		printf("AEDIN: recv all:%d real:%d 0x%X\n",ctx->cnt_virt,ctx->cnt_real,byte);*/

			/* This block only collects data,
			 *  type and packet len must be declared */
		if ((ctx->type != 0) && (ctx->len != 0)) {
				/* Allways increase virtual pointer */
			ctx->cnt_virt ++;
			
				/* Increase real pointer only to limit of AEDIN_BUFF */
			if (ctx->cnt_real < (AEDIN_BUFF - 1)) {
				ctx->buff[ctx->cnt_real] = byte;
				ctx->cnt_real ++;
			}
			
			if (ctx->cnt_virt >= ctx->len) {
				ctx->flags |= AEDIN_FLAGS_READY;
				return 1;
			}
			return 0;
		}
			
			/* Tell header packet byte */
		ctx->cnt_real ++;
		
		
			/* pakcet type and len is not yet declared */
		switch(ctx->type) {
			case 0:
					/* Check if type of packet is correct
					 * this is first received byte */
				switch (byte) {
					
						/* Theese are types regognized */
					case AEDIN_H4_CMD:
					case AEDIN_H4_ACL:
					case AEDIN_H4_SCO:
					case AEDIN_H4_EVENT:
						ctx->type = byte;
						ctx->len = 0;
					break;
					
						/* By default reset all */
					default:
						aedin_reset(ctx);
					break;
				}
			break;
			
				/* Decode Event header */
			case AEDIN_H4_EVENT:
				switch (ctx->cnt_real) {
					case 2:
							/* this is event code */
						ctx->ident = byte;
					break;

					case 3:
							/* Zero counters */
						ctx->cnt_virt = 0;
						ctx->cnt_real = 0;
						
						ctx->len = byte;
						
							/* IF events have no params, this packet is ready then */
						if (ctx->len == 0) {
							ctx->flags |= AEDIN_FLAGS_READY;
							return 1;
						}
					break;
					
						/* Should not happen */
					default:
					break;
				}
			break;

				/* Decode Data header */
			case AEDIN_H4_ACL:
			case AEDIN_H4_SCO:
				switch (ctx->cnt_real) {
					case 2:
							/* This is lower byte of HCI data packets "connection handle" */
						ctx->ident = byte;
					break;
					
					case 3:
							/* Set hi byte for connetion handle, this 3 bytes  */
						ctx->ident |= (((u16)byte << 8) & 0x0F00);
						
							/* Hi 4 bytes are reserved for internal use, so clr lower 4 */
						ctx->flags &= 0xF0;
						
							/* Lo 4 bits are flags 
							 * BC, Broadcast_Flag
							 * BP, Packet_Boundary_Flag:
							 * */
						ctx->flags |= (byte & 0xF0) >> 4;
					break;
					
					case 4:
							/* If SCO, this is litle smaller than ACL */
						if (ctx->type == AEDIN_H4_SCO) {
							ctx->cnt_virt = 0;
							ctx->cnt_real = 0;
							ctx->len = byte;

							if (ctx->len == 0) {
								ctx->flags |= AEDIN_FLAGS_READY;
								return 1;
							}
							return 0;
						}
							/* Must store byte for len */
						ctx->buff[0] = byte;
					break;
					
					case 5:
						ctx->cnt_virt = 0;
						ctx->cnt_real = 0;
						ctx->len = (u16)ctx->buff[0] | ((u16)byte << 8);
						
						
						if (ctx->len == 0) {
							ctx->flags |= AEDIN_FLAGS_READY;
							return 1;
						}
					break;
					
					default:
					break;
				}
			break;
			
			default:
			break;
		}
	}
	return 0;
}

