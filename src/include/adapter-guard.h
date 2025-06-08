#ifndef ADAPTER_GUARD
#define ADAPTER_GUARD

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libusb.h>
#include <systemd/sd-device.h>

int list(void);
int dump(char *id);
sd_device_enumerator *usb_device_enumerator(void);
int authenticate(char *id);
int init(void);

#endif
