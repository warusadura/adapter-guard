#include "include/adapter-guard.h"

int list(void)
{
        sd_device_enumerator *enumerator;
        int ret;

        ret = sd_device_enumerator_new(&enumerator);
        if (ret < 0) {
                fprintf(stderr, "Failed to create device enumerator: %s\n", strerror(-ret));
                return 1;
        }

        ret = sd_device_enumerator_add_match_subsystem(enumerator, "usb", true);
        if (ret < 0) {
                fprintf(stderr, "Failed to add match for USB devices: %s\n", strerror(ret));
                sd_device_enumerator_unref(enumerator);
                return 1;
        }

        // to further filter out
        ret = sd_device_enumerator_add_match_property(enumerator, "DEVTYPE", "usb_device");
        if (ret < 0) {
                fprintf(stderr, "Failed to add match for USB device type: usb_device: %s\n", strerror(ret));
                sd_device_enumerator_unref(enumerator);
                return 1;
        }

        for (sd_device *device = sd_device_enumerator_get_device_first(enumerator); device;
             device = sd_device_enumerator_get_device_next(enumerator)) {
                const char *vendor = NULL;
                const char *model = NULL;
                const char *path = NULL;

                ret = sd_device_get_property_value(device, "ID_VENDOR_FROM_DATABASE", &vendor);
                if (ret < 0)
                        fprintf(stderr, strerror(ret));

                ret = sd_device_get_property_value(device, "ID_MODEL", &model);
                if (ret < 0)
                        fprintf(stderr, strerror(ret));

                ret = sd_device_get_property_value(device, "DEVNAME", &path);
                if (ret < 0)
                        fprintf(stderr, strerror(ret));

                printf("%s : %s : \e[1m%s\e[0m\n", vendor, model, path);
        }

        sd_device_enumerator_unref(enumerator);

        return 0;
}

int write_to_file(void) { return 0; }

int print_devices()
{
        // depricated - do not use.
        libusb_device **devices = NULL;
        libusb_device *device = NULL;
        int ret = 0;
        int i = 0;

        ret = libusb_init_context(NULL, NULL, 0);
        if (ret < 0)
                return ret;

        ret = libusb_get_device_list(NULL, &devices);
        if (ret < 0)
                goto error;

        while ((device = devices[i++]) != NULL) {
                struct libusb_device_descriptor descriptor;
                ret = libusb_get_device_descriptor(device, &descriptor);
                if (ret < 0)
                        goto error;

                libusb_device_handle *handle;
                ret = libusb_open(device, &handle);
                if (ret == LIBUSB_ERROR_ACCESS) {
                        printf("LIBUSB_ERROR_ACCESS: insufficient permissions\n");
                        continue;
                }

                char manufacturer[256];
                libusb_get_string_descriptor_ascii(handle, descriptor.iManufacturer, (unsigned char *)manufacturer,
                                                   sizeof(manufacturer));

                printf("%04x : %04x %s\n", descriptor.idVendor, descriptor.idProduct, manufacturer);
        }

        return ret;
error:
        if (devices != NULL)
                libusb_free_device_list(devices, 1);
        libusb_exit(NULL);

        return ret;
}
