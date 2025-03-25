#include <libusb.h>
#include <stdio.h>
#include <string.h>

#include "include/adapter-guard.h"
#include "include/cmd.h"

int main(int argc, char *argv[])
{
        char *command = argv[1];

        if (argc == 1 || !strcmp(command, HELP))
                print_help();
        else if (!strcmp(command, VERSION))
                print_version();
        else if (!strcmp(command, LIST))
                list();
        else if (!strcmp(command, DUMP)) {
                if (argc != 3) {
                        fprintf(stderr, "Missing device id.\n\n");
                        printf("\e[1mUsage:\e[0m adapter-guard %s <id>\n", DUMP);
                }
                // todo
        } else if (!strcmp(command, AUTHENTICATE)) {
                if (argc != 3) {
                        fprintf(stderr, "Missing device id.\n\n");
                        printf("\e[1mUsage:\e[0m adapter-guard %s <id>\n", AUTHENTICATE);
                }
                // todo
        } else if (!strcmp(command, SCAN)) {
                if (argc != 3) {
                        fprintf(stderr, "Missing device id.\n\n");
                        printf("\e[1mUsage:\e[0m adapter-guard %s <id>\n", SCAN);
                }
                // todo
        } else {
                fprintf(stderr, "Invalid command.\n");
                print_help();
        }

        return 0;
}
