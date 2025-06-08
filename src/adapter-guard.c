#include "include/adapter-guard.h"

#define PATH_SIZE 5000

sd_device_enumerator *usb_device_enumerator(void)
{
        sd_device_enumerator *enumerator = NULL;
        int ret;

        ret = sd_device_enumerator_new(&enumerator);
        if (ret < 0) {
                fprintf(stderr, "Failed to create device enumerator: %s\n", strerror(-ret));
                return NULL;
        }

        ret = sd_device_enumerator_add_match_subsystem(enumerator, "usb", true);
        if (ret < 0) {
                fprintf(stderr, "Failed to add match for USB devices: %s\n", strerror(ret));
                sd_device_enumerator_unref(enumerator);
                return NULL;
        }

        /* To further filter out */
        ret = sd_device_enumerator_add_match_property(enumerator, "DEVTYPE", "usb_device");
        if (ret < 0) {
                fprintf(stderr, "Failed to add match for USB device type: usb_device: %s\n", strerror(ret));
                sd_device_enumerator_unref(enumerator);
                return NULL;
        }

        /* Caller must free this */
        return enumerator;
}

int store_built_ins(void)
{
        int ret;
        char config_dir_path[PATH_SIZE - 100];
        char config_file_path[PATH_SIZE];

        char *home = getenv("HOME");
        snprintf(config_dir_path, PATH_SIZE - 1, "%s/%s/%s", home, ".config", "adapter-guard");

        ret = mkdir(config_dir_path, 0777);
        if (errno == EEXIST) {
                fprintf(stderr, "%s \e[1malready exists.\e[0m\n", config_dir_path);
                fprintf(stderr, "Remove %s if it's neccessary to run `init` again.\n", config_dir_path);
                return 0;
        }
        if (ret) {
                fprintf(stderr, "mkdir: %s\n", strerror(errno));
                return ret;
        }

        snprintf(config_file_path, PATH_SIZE - 1, "%s/%s", config_dir_path, "devices.list");

        int fd = open(config_file_path, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
        if (fd < 0) {
                fprintf(stderr, "open %s\n", strerror(errno));
                return ret;
        }

        sd_device_enumerator *enumerator = usb_device_enumerator();
        if (enumerator == NULL) {
                close(fd);
                return 1;
        }

        for (sd_device *device = sd_device_enumerator_get_device_first(enumerator); device;
             device = sd_device_enumerator_get_device_next(enumerator)) {
                const char *serial = NULL;

                ret = sd_device_get_property_value(device, "ID_SERIAL", &serial);
                if (!ret) {
                        size_t len = strlen(serial);
                        write(fd, serial, len);
                        write(fd, "\n", 1);
                }
        }

        close(fd);
        sd_device_enumerator_unref(enumerator);

        printf("Identified built-ins.\n");

        return 0;
}

char *read_from_built_ins(void)
{
        char path[PATH_SIZE];
        FILE *device_file;

        char *home = getenv("HOME");
        snprintf(path, PATH_SIZE - 1, "%s/%s", home, ".config/adapter-guard/devices.list");

        device_file = fopen(path, "r");
        if (device_file == NULL)
                return NULL;

        fseek(device_file, 0, SEEK_END);
        long size = ftell(device_file);
        fseek(device_file, 0, SEEK_SET);

        char *buffer = malloc(size);
        memset(buffer, 0, size);

        fread(buffer, 1, size, device_file);

        fclose(device_file);

        return buffer; /* Caller must free this */
}

int init(void)
{
        int ret;

        printf("\e[1mAre all external USB devices detached (y/n):\e[0m ");

        switch (getchar()) {
        case 121:
                ret = store_built_ins();
                break;
        case 110:
                printf("Remove all external USB devices and then continue.\n");
                break;
        }

        return ret;
}

int list(void)
{
        sd_device_enumerator *enumerator = NULL;
        int ret;

        enumerator = usb_device_enumerator();
        if (enumerator == NULL)
                return 1;

        char *built_ins = read_from_built_ins();
        if (built_ins == NULL) {
                sd_device_enumerator_unref(enumerator);
                fprintf(stderr, "Execute \e[1minit\e[0m command first to identify built-ins.\n");
                return 1;
        }

        for (sd_device *device = sd_device_enumerator_get_device_first(enumerator); device;
             device = sd_device_enumerator_get_device_next(enumerator)) {
                const char *serial = NULL;
                const char *vendor = NULL;
                const char *model = NULL;
                const char *path = NULL;
                bool own = false;

                char *built_ins_copy = strdup(built_ins);
                /* Tokenize the copy to avoid modifying the original string */
                char *token = strtok(built_ins_copy, "\n");

                ret = sd_device_get_property_value(device, "ID_SERIAL", &serial);
                while (token != NULL) {
                        if (!strcmp(serial, token)) {
                                own = true;
                                break;
                        } else
                                token = strtok(NULL, "\n"); /* Tokenize the same string */
                }

                /* Tokens are exhausted at this point */
                free(built_ins_copy);

                if (own)
                        continue;

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
        free(built_ins);

        return 0;
}

int dump(char *id)
{
        /* udevadm info --query=all --name=/dev/bus/usb/003/021 */
        sd_device_enumerator *enumerator = NULL;
        int ret;
        int counter = 0;

        enumerator = usb_device_enumerator();
        if (enumerator == NULL)
                return 1;

        for (sd_device *device = sd_device_enumerator_get_device_first(enumerator); device;
             device = sd_device_enumerator_get_device_next(enumerator)) {
                const char *vendor = NULL;
                const char *model = NULL;
                const char *dev_name = NULL;
                const char *serial = NULL;
                const char *serial_short = NULL;
                const char *dev_id = NULL;
                const char *bus = NULL;
                const char *dev_type = NULL;
                const char *revision = NULL;
                const char *interfaces = NULL;
                const char *id_path = NULL;
                const char *id_vendor = NULL;
                const char *dev_path = NULL;
                const char *sys_path = NULL;
                const char *driver = NULL;
                const char *bus_num = NULL;
                const char *dev_num = NULL;

                ret = sd_device_get_property_value(device, "DEVNAME", &dev_name);
                if (ret < 0)
                        fprintf(stderr, strerror(ret));

                if (strcmp(dev_name, id)) {
                        continue;
                }

                counter++;

                printf("\e[1m%s\e[0m\n", dev_name);

                ret = sd_device_get_property_value(device, "ID_MODEL", &model);
                if (!ret)
                        printf("   Model: %s\n", model);

                ret = sd_device_get_property_value(device, "ID_VENDOR_FROM_DATABASE", &vendor);
                if (!ret)
                        printf("   Manufacturer: %s\n", vendor);

                ret = sd_device_get_property_value(device, "ID_SERIAL", &serial);
                if (!ret)
                        printf("   Serial No: %s\n", serial);

                ret = sd_device_get_property_value(device, "ID_SERIAL_SHORT", &serial_short);
                if (!ret)
                        printf("   Serial Short: %s\n", serial_short);

                ret = sd_device_get_device_id(device, &dev_id);
                if (!ret)
                        printf("   Device ID: %s\n", dev_id);

                ret = sd_device_get_property_value(device, "ID_BUS", &bus);
                if (!ret)
                        printf("   Bus: %s\n", bus);

                ret = sd_device_get_property_value(device, "DEVTYPE", &dev_type);
                if (!ret)
                        printf("   Device Type: %s\n", dev_type);

                ret = sd_device_get_property_value(device, "ID_REVISION", &revision);
                if (!ret)
                        printf("   Revision No: %s\n", revision);

                ret = sd_device_get_property_value(device, "ID_USB_INTERFACES", &interfaces);
                if (!ret)
                        printf("   Interfaces: %s\n", interfaces);

                ret = sd_device_get_property_value(device, "ID_PATH", &id_path);
                if (!ret)
                        printf("   ID path: %s\n", id_path);

                ret = sd_device_get_property_value(device, "ID_USB_VENDOR", &id_vendor);
                if (!ret)
                        printf("   ID vendor: %s\n", id_vendor);

                ret = sd_device_get_property_value(device, "DEVPATH", &dev_path);
                if (!ret)
                        printf("   Device path: %s\n", dev_path);

                ret = sd_device_get_syspath(device, &sys_path);
                if (!ret)
                        printf("   SYSFS path: %s\n", sys_path);

                ret = sd_device_get_property_value(device, "DRIVER", &driver);
                if (!ret)
                        printf("   Driver: %s\n", driver);

                ret = sd_device_get_property_value(device, "BUSNUM", &bus_num);
                if (!ret)
                        printf("   Bus number: %s\n", bus_num);

                ret = sd_device_get_property_value(device, "DEVNUM", &dev_num);
                if (!ret)
                        printf("   Device number: %s\n", dev_num);
        }

        sd_device_enumerator_unref(enumerator);

        if (!counter) {
                fprintf(stderr, "Invalid ID: %s\n", id);
                return 1;
        }

        return 0;
}

int authenticate(char *id)
{
        sd_device_enumerator *enumerator = NULL;
        int ret;
        int counter = 0;

        enumerator = usb_device_enumerator();
        if (enumerator == NULL)
                return 1;

        for (sd_device *device = sd_device_enumerator_get_device_first(enumerator); device;
             device = sd_device_enumerator_get_device_next(enumerator)) {
                const char *dev_name = NULL;
                const char *vendor = NULL;
                const char *model = NULL;
                const char *serial = NULL;
                const char *serial_short = NULL;

                ret = sd_device_get_property_value(device, "DEVNAME", &dev_name);
                if (ret < 0)
                        fprintf(stderr, strerror(ret));

                if (strcmp(dev_name, id)) {
                        continue;
                }

                counter++;

                printf("\e[1m%s\e[0m\n", dev_name);

                sd_device_get_property_value(device, "ID_MODEL", &model);
                sd_device_get_property_value(device, "ID_VENDOR_FROM_DATABASE", &vendor);
                sd_device_get_property_value(device, "ID_SERIAL", &serial);
                sd_device_get_property_value(device, "ID_SERIAL_SHORT", &serial_short);

                if (vendor && model && serial_short) {
                        printf("   %s - %s - %s\n", vendor, model, serial_short);
                        printf("   \e[1mDevice verified as authentic\e[0m\n");
                } else if (vendor && model && serial) {
                        printf("   %s - %s - %s\n", vendor, model, serial);
                        printf("   \e[1mDevice verified as authentic\e[0m\n");
                } else {
                        printf("   \e[1mPotentially counterfeit device detected. Remove immediately\e[0m\n");
                }
        }

        sd_device_enumerator_unref(enumerator);

        if (!counter) {
                fprintf(stderr, "Invalid ID: %s\n", id);
                return 1;
        }

        return 0;
}

int print_devices()
{
        /* Depricated - do not use */
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
