#include <libusb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <systemd/sd-device.h>

int list(void);
int dump(char *id);
sd_device_enumerator *usb_device_enumerator(void);
int read_from_file(void);
int write_to_file(void);
