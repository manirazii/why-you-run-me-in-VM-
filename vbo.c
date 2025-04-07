#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#else
#include <unistd.h>
#endif

int is_virtual_machine() {
    #ifdef _WIN32
    unsigned int hypervisorPresent = 0;
    
    __asm {
        mov eax, 1
        cpuid
        bt ecx, 31
        setc hypervisorPresent
    }
    
    if (hypervisorPresent) {
        return 1;
    }
    
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        "HARDWARE\\DESCRIPTION\\System\\BIOS", 
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        
        char productName[256] = {0};
        DWORD size = sizeof(productName);
        
        if (RegQueryValueEx(hKey, "SystemProductName", NULL, NULL, 
            (LPBYTE)productName, &size) == ERROR_SUCCESS) {
            
            if (strstr(productName, "Virtual") || 
                strstr(productName, "VMware") || 
                strstr(productName, "VirtualBox") || 
                strstr(productName, "QEMU")) {
                RegCloseKey(hKey);
                return 1;
            }
        }
        RegCloseKey(hKey);
    }
    
    #else
    FILE *fp;
    char buffer[128];

    fp = popen("cat /sys/class/dmi/id/product_name 2>/dev/null", "r");
    if (fp != NULL) {
        while (fgets(buffer, sizeof(buffer), fp) != NULL) {
            if (strstr(buffer, "VirtualBox") || strstr(buffer, "VMware") ||
                strstr(buffer, "QEMU") || strstr(buffer, "KVM")) {
                pclose(fp);
                return 1;
            }
        }
        pclose(fp);
    }

    fp = popen("grep -c hypervisor /proc/cpuinfo 2>/dev/null", "r");
    if (fp != NULL) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            if (atoi(buffer) > 0) {
                pclose(fp);
                return 1;
            }
        }
        pclose(fp);
    }
    #endif

    return 0;
}

void show_message(const char *message) {
    #ifdef _WIN32
    MessageBox(NULL, message, "VM Detector", MB_OK | MB_ICONINFORMATION);
    #else
    printf("NOTIFICATION: %s\n", message);
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "notify-send 'VM Detector' '%s'", message);
    system(cmd);
    #endif
}

void copy_to_host() {
    #ifdef _WIN32
    char exePath[MAX_PATH];
    char destPath[MAX_PATH];
    
    GetModuleFileName(NULL, exePath, MAX_PATH);
    
    const char *userProfile = getenv("USERPROFILE");
    snprintf(destPath, sizeof(destPath), "%s\\Desktop\\VM_Detector.exe", userProfile);
    
    if (CopyFile(exePath, destPath, FALSE)) {
        show_message("Copied to host system: Desktop\\VM_Detector.exe");
    } else {
        show_message("Failed to copy to host system");
    }
    #else
    char cmd[512];
    const char *home = getenv("HOME");
    
    snprintf(cmd, sizeof(cmd), "cp \"%s\" \"%s/.VM_Detector\" 2>/dev/null", 
             "/proc/self/exe", home);
    
    if (system(cmd) == 0) {
        show_message("Copied to host system: ~/.VM_Detector");
    } else {
        show_message("Failed to copy to host system");
    }
    #endif
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
