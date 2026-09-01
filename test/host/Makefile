CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror

.PHONY: reserved
reserved:
	$(CC) $(CFLAGS) -Iinclude -I../../include usb_protocol_reserved_test.c ../../src/usb_protocol.c -o /private/tmp/cheesecool_usb_protocol_reserved_test
	/private/tmp/cheesecool_usb_protocol_reserved_test
