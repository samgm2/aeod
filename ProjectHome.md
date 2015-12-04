This is HCI/L2CAP stack for AVR-microcontrollers. Written in ANSI C and uses AVR-libc. Aims to be really small and only to provide connection-oriented/connectionless L2CAP communication between endpoints. Communication between Bluetooth Host Controller and AVR is made by UART/H4.

Currently this support L2CAP packet delivering between AVR-device and any device capable of L2CAP. Consumes about 4k of AVR program memory and delivered data packet size of about 16 bytes.

Im currenty developing this for Samsung BlueSEM-CII Bluetooth V1.1 Module, BTMZ5012A0.