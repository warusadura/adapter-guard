#include <dirent.h>
#include <mntent.h>
#include <sys/types.h>

#include "include/adapter-guard-scanner.h"

static int malicious_file_counter = 0;
static int deep_scan_failed = 0;

void flag_files(char *filename)
{
        const char *formats[] = {".sh",  ".bash", ".zsh", ".ksh", ".tcsh", ".csh", ".elf", ".run", ".bin",
                                 ".out", ".so",   ".py",  ".pl",  ".rb",   ".php", ".cgi", ".jar", ".apk",
                                 ".zip", ".rar",  ".7z",  ".tar", ".gz",   ".xz",  ".img", ".tmp"};

        size_t num_formats = sizeof(formats) / sizeof(formats[0]);

        char *extension = strrchr(filename, '.');
        if (!extension) {
                printf("      Potential malicious file detected - \e[1m%s\e[0m\n", filename);
                return;
        }

        for (size_t i = 0; i < num_formats; i++) {
                if (!strcmp(extension, formats[i])) {
                        malicious_file_counter++;
                        printf("      Potential malicious file detected - \e[1m%s\e[0m\n", filename);
                }
        }
}

void list_recursively(const char *path, bool got_full_path)
{
        /* Assumes the device is already mounted */

        if (got_full_path)
                goto have_path;

        FILE *fp = setmntent("/proc/mounts", "r");
        if (!fp) {
                perror("setmntent");
                deep_scan_failed++;
                return;
        }

        struct mntent *ent;
        char *mount_point = NULL;

        while ((ent = getmntent(fp)) != NULL) {
                if (strcmp(ent->mnt_fsname, path) == 0) {
                        mount_point = strdup(ent->mnt_dir);
                        printf("   Mass Storage detected. Scanning ...\n");
                        printf("   Detected %s\n", mount_point);
                }
        }

        if (fp)
                endmntent(fp);

have_path:
        if (got_full_path)
                mount_point = strdup(path);

        if (mount_point) {
                DIR *dir = opendir(mount_point);
                if (!dir) {
                        perror("opendir");
                        deep_scan_failed++;
                        goto exit;
                }

                struct dirent *entry;
                char full_path[PATH_MAX];

                while ((entry = readdir(dir)) != NULL) {
                        /* Skip . and .. */
                        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                                continue;

                        snprintf(full_path, PATH_MAX, "%s/%s", mount_point, entry->d_name);

                        struct stat st;
                        if (stat(full_path, &st) != 0) {
                                perror("stat");
                                continue;
                        }

                        if (S_ISDIR(st.st_mode))
                                list_recursively(full_path, true);
                        else if (S_ISREG(st.st_mode))
                                flag_files(full_path);
                }

                closedir(dir);
        }

exit:
        if (mount_point)
                free(mount_point);
}

void scan_storage(sd_device *device)
{
        sd_device_enumerator *block_enumerator;

        if (sd_device_enumerator_new(&block_enumerator) < 0) {
                deep_scan_failed++;
                return;
        }

        sd_device_enumerator_add_match_parent(block_enumerator, device);
        sd_device_enumerator_add_match_subsystem(block_enumerator, "block", true);

        for (sd_device *block_device = sd_device_enumerator_get_device_first(block_enumerator); block_device;
             block_device = sd_device_enumerator_get_device_next(block_enumerator)) {

                const char *devname = NULL;
                if (sd_device_get_devname(block_device, &devname) >= 0 && devname)
                        list_recursively(devname, false);
        }

        sd_device_enumerator_unref(block_enumerator);
}

int deep_scan(sd_device *device)
{
        const char *ret_suffix;
        int counter = 0;

        for (sd_device *child = sd_device_get_child_first(device, &ret_suffix); child;
             child = sd_device_get_child_next(device, &ret_suffix)) {
                const char *ifce_class;

                counter++;

                sd_device_get_sysattr_value(child, "bInterfaceClass", &ifce_class);
                /* look for Mass Storage */
                if (!strcmp(ifce_class, "08"))
                        scan_storage(child);
        }

        if (!counter)
                return 1;

        return 0;
}

int scan(char *id)
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
                const char *driver = NULL;
                const char *vendor = NULL;
                const char *model = NULL;
                const char *serial_short = NULL;
                const char *ifce_num = NULL;

                ret = sd_device_get_property_value(device, "DEVNAME", &dev_name);
                if (ret < 0)
                        fprintf(stderr, strerror(ret));

                if (strcmp(dev_name, id)) {
                        continue;
                }

                counter++;

                ret = sd_device_get_property_value(device, "DRIVER", &driver);
                if (ret < 0)
                        fprintf(stderr, strerror(ret));

                sd_device_get_property_value(device, "ID_MODEL", &model);
                sd_device_get_property_value(device, "ID_VENDOR_FROM_DATABASE", &vendor);
                sd_device_get_property_value(device, "ID_SERIAL_SHORT", &serial_short);

                sd_device_get_sysattr_value(device, "bNumInterfaces", &ifce_num);

                printf("\e[1m%s\e[0m\n", dev_name);
                printf("   %s - %s - %s\n", vendor, model, serial_short);

                /* check for authenticity */
                if (!(model && vendor && serial_short))
                        printf("   \e[1mCounterfeit device detected. Remove immediately\e[0m\n");
                /* check for USB driver usage */
                else if (strcmp(driver, "usb"))
                        printf("   \e[1mUSB driver not in use. Adapter could be malicious\e[0m\n");
                /* check bNumInterfaces */
                else if (atoi(ifce_num) > 5)
                        printf("   \e[1mSupporting more than 5 interfaces detected. Adapter could be malicious\e[0m\n");
                /* storage scan */
                else if (!deep_scan(device)) {
                        if (!malicious_file_counter && !deep_scan_failed)
                                printf(
                                    "   \e[1mNo potential malicious factors detected. Adapter is safe to use\e[0m\n");
                        else
                                printf("   \e[1mStorage access scan failed. Adapter could be malicious\e[0m\n");
                }
        }

        sd_device_enumerator_unref(enumerator);

        return 0;
}
