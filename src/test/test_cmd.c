#include <glib.h>

#include "../include/adapter-guard-scanner.h"
#include "../include/adapter-guard.h"

void test_list(void)
{
        int ret = list();
        g_assert(ret == 0);
}

void test_dump(void)
{
        int ret = dump("/dev/bus/usb/003/046");
        g_assert(ret == 0);
}

void test_authenticate(void)
{
        int ret = authenticate("/dev/bus/usb/003/048");
        g_assert(ret == 0);
}

void test_scan(void)
{
        int ret = scan("/dev/bus/usb/003/049");
        g_assert(ret == 0);
}

int main(int argc, char *argv[])
{
        g_test_init(&argc, &argv, NULL);

        g_test_add_func("/tmp/test_list", test_list);
        g_test_add_func("/tmp/test_dump", test_dump);
        g_test_add_func("/tmp/test_authenticate", test_authenticate);
        g_test_add_func("/tmp/test_scan", test_authenticate);

        return g_test_run();
}
