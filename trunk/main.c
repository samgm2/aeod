/*
 * 
 */
#include <string.h>
#include <aeod/aeod.h>



void main(void) {
		/* bluetooth */
	struct aeod bt;
	u8 len;
	u8 buff[16];
	u8 reply = 0;
	
	wdt_enable(WDTO_8S);
		/* Bluetooth core init */
	aeod_init(&bt);
	
	elayer_blink(1);
	
	while(1) {
		wdt_reset();
		
		
			/* Read data send by some .... stuff */
		if ((len = aeod_acl_read(&bt,buff,16))) {
				/* Example reply to Hello */
			if (memcmp(buff,"Hello",5) == 0) {
				memcpy(buff,"World",5);
				reply = 5;
			}
			
				/* Set Leds */
			if (memcmp(buff,"LED:",4) == 0) {
				if (buff[4] == '1') {
					elayer_blink(1);
				} else {
					elayer_blink(0);
				}
				
					/* Dummy reply */
				memcpy(buff,"LED:N",5);
				reply = 5;
			}
			
			
		}
		
			/* Post replies only if there is no tranfer in progress */
		if ((reply > 0) && (aeod_tranfer(&bt) == 0)) {
			aeod_acl_write(&bt,buff,reply,0);
			reply = 0;
		}
		
	}	
	
}

