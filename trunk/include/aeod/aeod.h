/*
 * This is my lii((i)^n)iitle tiny HCI implementation library.
 * 
 * 
 * Basically we just let baseband chip to make connections.
 */
#include "aedin.h"
#ifndef __AEOD_H
#define __AEOD_H								1

	/* include connection oriented L2CAP handling, atleast 446 bytes  */
#define AEOD_CONNECTION_ORIENTED_CHANNEL		1


#define	AEOD_LPC								0x02
#define	AEOD_HCBC								0x03
#define AEOD_IP									0x04
												/* OCF 				  OGF */
#define	AEOD_HCBC_RESET 						(0x0003 & 0x3FF) | ((AEOD_HCBC << 10) & 0xFC00)
#define AEOD_HCBC_SET_EVENT_FILTER				(0x0005 & 0x3FF) | ((AEOD_HCBC << 10) & 0xFC00)
#define AEOD_HCBC_DISCONNECT					(0x0006 & 0x3FF) | ((AEOD_HCBC << 10) & 0xFC00)
#define AEOD_HCBC_ACCEPT_CONNECTION_REQUEST		(0x0009 & 0x3FF) | ((AEOD_HCBC << 10) & 0xFC00)
#define AEOD_HCBC_WRITE_SCAN_ENABLE				(0x001A & 0x3FF) | ((AEOD_HCBC << 10) & 0xFC00)
#define AEOD_HCBC_READ_SCAN_ENABLE				(0x0019 & 0x3FF) | ((AEOD_HCBC << 10) & 0xFC00)
#define AEOD_HCBC_WRITE_CLASS_OF_DEVICE			(0x0024 & 0x3FF) | ((AEOD_HCBC << 10) & 0xFC00)

#define AEOD_UNKNOWN_OP							0x0000

#define AEOD_LPC_REJECT_CONNECTION_REQUEST		(0x000A & 0x3FF) | ((AEOD_HCBC << 10) & 0xFC00)
#define AEOD_LPC_SWITCH_ROLE					(0x000B & 0x3FF) | ((AEOD_HCBC << 10) & 0xFC00)


#define AEOD_IP_READ_BUFFER_SIZE				(0x0005 & 0x3FF) | ((AEOD_IP << 10) & 0xFC00)
#define AEOD_IP_READ_BD_ADDR					(0x0009 & 0x3FF) | ((AEOD_IP << 10) & 0xFC00)


	/* Events */
#define AOED_EVENT_CONNECTION_COMPLETE			0x03
#define AEOD_EVENT_CONNECTION_REQUEST			0x04
#define AEOD_EVENT_DISCONNECTION_COMPLETE		0x05
#define AEOD_EVENT_COMMAND_COMPLETE				0x0E
#define AEOD_EVENT_COMMAND_STATUS				0x0F
#define AEOD_EVENT_MAX_SLOTS_CHANGE				0x1B
#define AEOD_EVENT_PAGE_SCAN_REPETITION_MODE_CHANGE		0x20
#define AEOD_EVENT_NUMBER_OF_COMPLETED_PACKETS	0x13

#define AEOD_EVENT_HARDWARE_ERROR				0x10


		 /* Transfer in progress, can't send new until this is complete.*/
#define AEOD_FLAGS_TRANFER						0x01
	/* Set to initialize period, */
#define AEOD_FLAGS_INITIALIZE					0x02
	/* Initialization in phase */
#define AEOD_FLAGS_INITIALIZE_PHASE				0x04



struct aeod {
	u8						flags;
	volatile u8				status;
	struct aedin			trans;
		/* Current session handle */
	u16						handle;
	u8						channel;
	u16						psm;
};

	/* Structure for L2CAP connection oriented message signalling
	 * see BLUETOOTH SPECIFICATION Version 1.1 page 283 */
struct aeod_l2cap_conreq {
	u8													code;
	u8													ident;
	u16													length;
	u16													psm;
	u16													sourceCID;
};


	/* Just send reset message which perform sw reset */
void aeod_reset(struct aeod *ctx) {
	aedin_cmd_send(&ctx->trans,AEOD_HCBC_RESET,0,0);
}

	/* Set scan mode */
void aeod_scan(struct aeod *ctx) {
	u8 buff[1];
	buff[0] = 0x03;
	aedin_cmd_send(&ctx->trans,AEOD_HCBC_WRITE_SCAN_ENABLE,buff,1);
}

	/* Apply events */
void aeod_set_event_filter(struct aeod *ctx) {
	u8 buff[3];
	buff[0] = 0x02;
	buff[1] = 0x00;
	buff[2] = 0x02;
	aedin_cmd_send(&ctx->trans,AEOD_HCBC_SET_EVENT_FILTER,buff,3);
}


	/* Send acl packet, 4346
	 * -> PSM is allways 2 bytes even if received whould be different */
void aeod_acl_send(struct aeod *ctx,u16 handle,u16 channel,u16 psm,u8 *payload,u8 header_len,u8 data_len) {
	u8 buff[12];
	u8 *adata;
		/* H4 and HCI 		- 5 bytes */
	buff[0] = AEDIN_H4_ACL;
	((u16*)&buff[1])[0] = handle;
	
		/* ACL-data Flags:
		 * PB Packet_Boundary_Flag
		 * 0x10
		 * 0x20
				00 Reserved for future use
				01 Continuing fragment packet of Higher Layer Message
				10 First packet of Higher Layer Message (i.e. start of an L2CAP packet)
				11 Reserved for future use
		 * 
		 * BC Broadcast_Flag
		 * 0x40
		 * 0x80
		 * 
		 *  */
	buff[2] |= 0x20;
	
	
	((u16*)&buff[3])[0] = (header_len + data_len) + 6;	/* LEN + ACL + PSM/(or data) */
	
	adata = &buff[5];
		/* ACL 				- 4 bytes */
	((u16*)&adata[0])[0] = (header_len + data_len) + 2;	/* PSM */
	((u16*)&adata[2])[0] = channel;
		/* PSM 				- 2 bytes */
	((u16*)&adata[4])[0] = psm;
	
	ctx->flags |= AEOD_FLAGS_TRANFER;
	
		/* Send 11 bytes for header */
	elayer_write(&ctx->trans.layer,buff,11);
		/* And send payload */
	elayer_write(&ctx->trans.layer,payload,header_len);
}

	/* Write acl data */
u8 aeod_acl_write(struct aeod *ctx,void *vdata,u8 header_len,u8 data_len) {
	u8 *data = vdata;
	
		/* No connection */
	if (ctx->handle == 0) return 0;
		
	if (ctx->channel == 0x0002) {
		aeod_acl_send(ctx,ctx->handle,ctx->channel,ctx->psm,data,header_len,data_len);
	} else {
#ifdef AEOD_CONNECTION_ORIENTED_CHANNEL
		u16 psm_field = 0;
		u8 *ndata;
		
		ndata = data;
			/* There is posibility to send 0 bytes */
		if (header_len > 0) {
			psm_field = data[0];
			header_len --;
			ndata = &data[1];
		}
		
			/* There is posibility to send 1 byte */
		if (header_len > 0) {
			psm_field |= ((u16)data[1]) << 8;
			header_len --;
			ndata = &data[2];
		}
		
		aeod_acl_send(ctx,ctx->handle,ctx->channel,psm_field,ndata,header_len,data_len);
#endif
	}
	return 1;
}

	/* Disconnect from host */
void aeod_disconnect(struct aeod *ctx,u16 handle) {
	u8 buff[3];
	u16 *dest = (u16*)buff;
	(*dest) = handle;
		/* Some misc reason :/ */
	buff[2] = 0x13;
	aedin_cmd_send(&ctx->trans,AEOD_HCBC_DISCONNECT,buff,3);
}

	/* Return 1 if is connected to someone */
u8 aeod_connected(struct aeod *ctx) {
	if (ctx->handle == 0) return 0;
	return 1;
}

	/* Return 1 if in tranfer */
u8 aeod_tranfer(struct aeod *ctx) {
	if (ctx->trans.type != 0) return 1;
		/* Can't send new one until previous is finished */
	if (ctx->flags & AEOD_FLAGS_TRANFER) return 1;
	return 0;
}

void aeod_handle_event(struct aeod *ctx) {
	
/*	printf("  EVENT: code:0x%X \n",ctx->trans.ident);*/
	
	switch (ctx->trans.ident) {
		case AEOD_EVENT_NUMBER_OF_COMPLETED_PACKETS:
			
/*			printf("    Conpleted:%d handle:%d num:%d\n",ctx->trans.buff[0],((u16*)&ctx->trans.buff[1])[0],((u16*)&ctx->trans.buff[3])[0]);*/
				/* This just assume all is okay. */
			ctx->flags &= 255 ^ AEOD_FLAGS_TRANFER;
		break;

		case AEOD_EVENT_COMMAND_STATUS:
			switch(((u16*)(&ctx->trans.buff[2]))[0]) {
				case AEOD_UNKNOWN_OP:
					/* This is possible for csr chip with happens to have hw reset. */
				break;
				
				default:
				break;
			}
		break;
		
			/* Must not occur */
		case AEOD_EVENT_HARDWARE_ERROR:
				/* Let indicate error */
			elayer_error(128);
		break;
		
		case AEOD_EVENT_PAGE_SCAN_REPETITION_MODE_CHANGE:
/*			printf("    Repetition to:0x%X\n",ctx->trans.buff[9]);*/
		break;
		
		case AOED_EVENT_CONNECTION_COMPLETE:
				/* Connection did not to be created. */
			if (ctx->trans.buff[0] != 0) return;
			
				/* We already have connection, so disconnect this. 
			if (ctx->handle != 0) {
				aeod_disconnect(ctx,((u16*)&ctx->trans.payload[1])[0] & 0x0FFF);
				return 1;
			}*/
			ctx->handle = ((u16*)&ctx->trans.buff[1])[0] & 0x0FFF;
			
				/* Connection type
				 * ctx->trans.buff[7];
				 * 
				 *  */
/*			printf("    Got connection handle:%d type:0x%X encryption:0x%X\n",ctx->handle,ctx->trans.buff[9],ctx->trans.buff[10]);*/
			
/*
 * If you need BT-addr of connected device, uncomment this
 *
 			{
			u8 i;
 			for (i = 0;i < 6;i ++) {
				ctx->paddr[i] = ctx->trans.buff[i + 3];
			}
			}
*/
		break;
		
		case AEOD_EVENT_DISCONNECTION_COMPLETE:
				/* Disconnection complete */
			if (ctx->trans.buff[0] == 0) {
				ctx->handle = 0;
			}
		break;
		
		case AEOD_EVENT_COMMAND_COMPLETE:
			switch(((u16*)(&ctx->trans.buff[1]))[0]) {
				case AEOD_HCBC_RESET:
					ctx->status ++;
				break;

				case AEOD_HCBC_SET_EVENT_FILTER:
					ctx->status ++;
				break;
				
				case AEOD_HCBC_WRITE_SCAN_ENABLE:
					ctx->status ++;
				break;
				
				default:
				break;
			}
		break;
		
		default:
		break;
	}	
}


	/*  */
void aeod_handle_l2cap_signal(struct aeod *ctx) {
#ifdef AEOD_CONNECTION_ORIENTED_CHANNEL
	struct aeod_l2cap_conreq *rec;
	u16 psm;
	u16 answer[5];
	
	rec = (struct aeod_l2cap_conreq*)&ctx->trans.buff[4];
/*	printf("  L2CAP: code:0x%X ident:%d len:%d psm:%d CID:%d\n",rec->code ,rec->ident ,rec->length ,rec->psm ,rec->sourceCID);*/

	if (rec->code == 0x04) {
			/* Code and identifier */
		psm = 0x05 | ((u16)rec->ident << 8);
			/* Aswe length */
		answer[0] = 8;
		
		answer[1] = rec->psm;
		answer[2] = rec->sourceCID;
		answer[3] = 0;
		answer[4] = 0;
		aeod_acl_send(ctx,ctx->handle,ctx->channel,psm,(u8*)answer,10,0);
		
		
			/* This is litle ankward, but pecause this is /neccessary/ we must do it. */
		
			/* Code and identifier */
		psm = 0x04 | ((u16)(rec->ident + 1) << 8);
			/* Aswe length */
		answer[0] = 4;
		answer[1] = rec->psm;
		answer[2] = rec->sourceCID;
		aeod_acl_send(ctx,ctx->handle,ctx->channel,psm,(u8*)answer,6,0);
		return;
	}
	
	if (rec->code == 0x02) {
		
		ctx->psm = rec->psm;
			/* Code and identifier */
		psm = 0x03 | ((u16)rec->ident << 8);
			/* Aswe length */
		answer[0] = 8;
		
		answer[1] = rec->sourceCID;
		answer[2] = rec->sourceCID;
		answer[3] = 0x0000;
		answer[4] = 0x0000;
		aeod_acl_send(ctx,ctx->handle,ctx->channel,psm,(u8*)answer,10,0);
		return;
	}
	
	if (rec->code == 0x0A) {
			/* Code and identifier */
		psm = 0x0B | ((u16)rec->ident << 8);
			/* Aswe length */
		answer[0] = 4;
		answer[1] = rec->psm;
		answer[2] = 0x0000;
		aeod_acl_send(ctx,ctx->handle,ctx->channel,psm,(u8*)answer,6,0);
		return;
	}
		/* Connection teminate */
	if (rec->code == 0x06) {
			/* Code and identifier */
		psm = 0x07 | ((u16)rec->ident << 8);
			/* Aswe length */
		answer[0] = 4;
		answer[1] = rec->sourceCID;
		answer[2] = rec->sourceCID;
		aeod_acl_send(ctx,ctx->handle,ctx->channel,psm,(u8*)answer,6,0);
		return;
	}
#endif
}

	/* Handle acl packet */
u8 aeod_handle_acl(struct aeod *ctx,void *vdata,u8 len) {
	u8 i,s;
	u8 *b;
	u16 *p;

	p = (u16*)&ctx->trans.buff;
	
	/* p = L2CAP Packet see BT1.1, S 280, Figure 4.1: L2CAP Packet (field sizes in bits)
	 * p[0] = len
	 * p[1] = channel
	 * p[2] = info
	 * 
	 * -in CONNECTION-ORIENTED packet, data begins directly from info.
	 * -in CONNECTIONLESS packet, data begins after psm, which is 2-bytes long and is before data.
	 * Thereforce CONNECTIONLESS packet data portion begin in p[3].
	 * 
	 */
	
/*	Not really needed
 * ctx->len = p[0];*/
	ctx->channel = p[1];
	
	if (ctx->channel == 0x0001) {
		/* if you want L2CAP signalling, then finnish this */
		aeod_handle_l2cap_signal(ctx);
		return 0;
	}

		/* By default, assume CONNECTION-ORIENTED CHANNEL  */
	b = (u8*)&p[2];
	s = ctx->trans.cnt_real - 4;
	
		/*  CONNECTIONLESS DATA CHANNEL */
	if (ctx->channel == 0x0002) {
			/* Set psm */
		ctx->psm = p[2];
			/* NEED IMPLEMENTATION FOR (PSM SIZE) */
		b = (u8*)&p[3];
		s = ctx->trans.cnt_real - 6;
	}
	
		/* Copy only up to len */
	if (s > len) s = len;
	
	
	for (i = 0;i < s;i ++) {
		((u8*)vdata)[i] = b[i];
	}
	return i;
}

	/* Set state so itshould start init sequence */
void aeod_handle_status(struct aeod *ctx) {
	switch(ctx->status) {
		case 0:
/*				elayer_hwreset(&ctx->trans.layer);*/
			ctx->status = 3;
		break;
			
			/* This is inacse to test functionality */
		case 3:
			ctx->status ++;
			aeod_reset(ctx);
		break;
		
		case 5:
			ctx->status ++;
			aeod_set_event_filter(ctx);
		break;
		
		case 7:
			ctx->status ++;
			aeod_scan(ctx);
		break;
		
		case 9:
			return;
		break;
		
		default:
		break;
	}
}

	/* Read acl data */
u8 aeod_acl_read(struct aeod *ctx,void *vdata,u8 len) {
	
	aeod_handle_status(ctx);
	
	if (aedin_receive(&ctx->trans)) {
/*		printf("AEOD: got rlen:%d clen:%d\n",ctx->trans.cnt_real,ctx->trans.cnt_virt);*/
		
		
		switch(ctx->trans.type) {
			case AEDIN_H4_EVENT:
				aeod_handle_event(ctx);
			break;
			
			case AEDIN_H4_ACL:
				return aeod_handle_acl(ctx,vdata,len);
			break;
			
			default:
			break;
		}
	}
	return 0;
}


void aeod_init(struct aeod *ctx) {
	ctx->status = 0;
	ctx->handle = 0;
	ctx->flags = 0;
	aedin_init(&ctx->trans);
}


void aeod_free(struct aeod *ctx) {
	aedin_free(&ctx->trans);
}


#endif /* __AEOD_H */

