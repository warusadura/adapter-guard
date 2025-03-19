#include <stdio.h>

#include "include/cmd.h"

void print_help()
{
        printf("\e[1mUsage:\e[0m adapter-guard <command>\n\n");
        printf("\e[1mCommands:\e[0m\n");
        printf("   \e[1mlist\e[0m                List external USB devices\n");
        printf("   \e[1mdump <id>\e[0m           Dump device information\n");
        printf("   \e[1mauthenticate <id>\e[0m   Check authenticity of a device\n");
        printf("   \e[1mscan <id>\e[0m           Scan for potential malicious factors\n");
        printf("   \e[1mhelp\e[0m                Print help\n");
        printf("   \e[1mversion\e[0m             Print version\n");
}

void print_version() { printf("adapter-guard 0.1.0\n"); }
