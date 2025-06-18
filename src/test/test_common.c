#include <glib.h>
#include <stdlib.h>

#include "../include/adapter-guard-scanner.h"
#include "../include/adapter-guard.h"

void test_usb_device_enumerator(void)
{
        sd_device_enumerator *enumerator = usb_device_enumerator();
        g_assert(enumerator != NULL);
        sd_device_enumerator_unref(enumerator);
}

void test_read_from_built_ins(void)
{
        char *built_ins = read_from_built_ins();
        g_assert(built_ins != NULL);
        free(built_ins);
}

int main(int argc, char *argv[])
{
        g_test_init(&argc, &argv, NULL);

        g_test_add_func("/tmp/test_usb_device_enumerator", test_usb_device_enumerator);
        g_test_add_func("/tmp/test_read_from_built_ins", test_read_from_built_ins);

        return g_test_run();
}
