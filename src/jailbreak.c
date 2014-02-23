/**
 * pris0nbarake - jailbreak.c
 *
 * Exploits from evasi0n and absinthe2. And others.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>

#include <signal.h>
#include <plist/plist.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>

#include <assert.h>

#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/mobile_image_mounter.h>
#include <libimobiledevice/mobilebackup2.h>
#include <libimobiledevice/notification_proxy.h>
#include <libimobiledevice/afc.h>
#include <libimobiledevice/sbservices.h>
#include <libimobiledevice/file_relay.h>
#include <libimobiledevice/diagnostics_relay.h>

#include <zlib.h>

#include <fcntl.h>
#include <sys/mman.h>

#include "partialcommon.h"
#include "partial.h"

#include "common.h"

#include "MobileDevice.h"

#define AFCTMP 	"HackStore"

typedef struct _compatibility {
    char *product;
    char *build;
} compatibility_t;

compatibility_t compatible_devices[] = {
    {"N81AP", "10B500"},
    {"N88AP", "10B500"},

    {"N81AP", "10B400"},

    {"N41AP", "10B350"},
    {"N42AP", "10B350"},

    {"N94AP", "10B329"},

    {"N90BAP", "10B329"},
    {"N90AP", "10B329"},
    {"N92AP", "10B329"},
    {"N81AP", "10B329"},
    {"N88AP", "10B329"},

    {"N78AP", "10B329"},

    {"N41AP", "10B329"},
    {"N42AP", "10B329"},

    {"J1AP", "10B329"},
    {"J2AP", "10B329"},
    {"J2aAP", "10B329"},

    {"P101AP", "10B329"},
    {"P102AP", "10B329"},
    {"P103AP", "10B329"},

    {"K93AP", "10B329"},
    {"K93AAP", "10B329"},
    {"K94AP", "10B329"},
    {"K95AP", "10B329"},

    {"P105AP", "10B329"},
    {"P106AP", "10B329"},
    {"P107AP", "10B329"},

    {NULL, NULL}
};

static int cpio_get_file_name_length(void *cpio)
{
    if (cpio) {
        char buffer[7];
        int val;

        memset(buffer, '\0', 7);

        memcpy(&buffer, (void *) (cpio + 59), 6);   /* File Name Length */

        val = strtoul(buffer, NULL, 8);
        return val;
    } else {
        return 0;
    }
}

static int cpio_get_file_length(void *cpio)
{
    if (cpio) {
        char buffer[12];
        int val;

        memset(buffer, '\0', 12);

        memcpy(&buffer, (void *) (cpio + 65), 11);  /* File Length */

        val = strtoul(buffer, NULL, 8);
        return val;
    } else {
        return 0;
    }
}

/* recursively remove path, including path */
static void rmdir_recursive(const char *path)
{                               /*{{{ */
    if (!path) {
        return;
    }
    DIR *cur_dir = opendir(path);
    if (cur_dir) {
        struct dirent *ep;
        while ((ep = readdir(cur_dir))) {
            if ((strcmp(ep->d_name, ".") == 0)
                || (strcmp(ep->d_name, "..") == 0)) {
                continue;
            }
            char *fpath = (char *) malloc(strlen(path) + 1 + strlen(ep->d_name) + 1);
            if (fpath) {
                struct stat st;
                strcpy(fpath, path);
                strcat(fpath, "/");
                strcat(fpath, ep->d_name);

                if ((stat(fpath, &st) == 0) && S_ISDIR(st.st_mode)) {
                    rmdir_recursive(fpath);
                } else {
                    if (remove(fpath) != 0) {
                        DEBUG("could not remove file %s: %s\n", fpath, strerror(errno));
                    }
                }
                free(fpath);
            }
        }
        closedir(cur_dir);
    }
    if (rmdir(path) != 0) {
        fprintf(stderr, "could not remove directory %s: %s\n", path, strerror(errno));
    }
}                               /*}}} */

static void print_xml(plist_t node)
{
    char *xml = NULL;
    uint32_t len = 0;
    plist_to_xml(node, &xml, &len);
    if (xml)
        puts(xml);
}

/* char** freeing helper function */
static void free_dictionary(char **dictionary)
{                               /*{{{ */
    int i = 0;

    if (!dictionary)
        return;

    for (i = 0; dictionary[i]; i++) {
        free(dictionary[i]);
    }
    free(dictionary);
}                               /*}}} */

/* recursively remove path via afc, (incl = 1 including path, incl = 0, NOT including path) */
static int rmdir_recursive_afc(afc_client_t afc, const char *path, int incl)
{                               /*{{{ */
    char **dirlist = NULL;
    if (afc_read_directory(afc, path, &dirlist) != AFC_E_SUCCESS) {
        //fprintf(stderr, "AFC: could not get directory list for %s\n", path);
        return -1;
    }
    if (dirlist == NULL) {
        if (incl) {
            afc_remove_path(afc, path);
        }
        return 0;
    }

    char **ptr;
    for (ptr = dirlist; *ptr; ptr++) {
        if ((strcmp(*ptr, ".") == 0) || (strcmp(*ptr, "..") == 0)) {
            continue;
        }
        char **info = NULL;
        char *fpath = (char *) malloc(strlen(path) + 1 + strlen(*ptr) + 1);
        strcpy(fpath, path);
        strcat(fpath, "/");
        strcat(fpath, *ptr);
        if ((afc_get_file_info(afc, fpath, &info) != AFC_E_SUCCESS) || !info) {
            // failed. try to delete nevertheless.
            afc_remove_path(afc, fpath);
            free(fpath);
            free_dictionary(info);
            continue;
        }

        int is_dir = 0;
        int i;
        for (i = 0; info[i]; i += 2) {
            if (!strcmp(info[i], "st_ifmt")) {
                if (!strcmp(info[i + 1], "S_IFDIR")) {
                    is_dir = 1;
                }
                break;
            }
        }
        free_dictionary(info);

        if (is_dir) {
            rmdir_recursive_afc(afc, fpath, 0);
        }
        afc_remove_path(afc, fpath);
        free(fpath);
    }

    free_dictionary(dirlist);
    if (incl) {
        afc_remove_path(afc, path);
    }

    return 0;
}                               /*}}} */

static int connected = 0;

void jb_device_event_cb(const idevice_event_t * event, void *user_data)
{
    char *uuid = (char *) user_data;
    DEBUG("device event %d: %s\n", event->event, event->udid);
    if (uuid && strcmp(uuid, event->udid))
        return;
    if (event->event == IDEVICE_DEVICE_ADD) {
        connected = 1;
    } else if (event->event == IDEVICE_DEVICE_REMOVE) {
        connected = 0;
    }
}

static void idevice_event_cb(const idevice_event_t * event, void *user_data)
{
    jb_device_event_cb(event, user_data);
}

typedef struct __csstores {
    uint32_t csstore_number;
} csstores_t;

static csstores_t csstores[16];
static int num_of_csstores = 0;

int check_consistency(char *product, char *build)
{
    // Seems legit.
    return 0;
}

int verify_product(char *product, char *build)
{
    compatibility_t *curcompat = &compatible_devices[0];
    while ((curcompat) && (curcompat->product != NULL)) {
        if (!strcmp(curcompat->product, product) && !strcmp(curcompat->build, build))
            return 0;
        curcompat++;
    }
    return 1;
}

const char *lastmsg = NULL;
static void status_cb(const char *msg, int progress)
{
    if (!msg) {
        msg = lastmsg;
    } else {
        lastmsg = msg;
    }
    DEBUG("[%d%%] %s\n", progress, msg);
}

#ifndef __GUI__
int main(int argc, char *argv[])
{
    device_t *device = NULL;
    char *uuid = NULL;
    char *product = NULL;
    char *build = NULL;
    int old_os = 0;

    /********************************************************/
    /*
     * device detection 
     */
    /********************************************************/
    if (!uuid) {
        device = device_create(NULL);
        if (!device) {
            ERROR("No device found, is it plugged in?\n");
            return -1;
        }
        uuid = strdup(device->uuid);
    } else {
        DEBUG("Detecting device...\n");
        device = device_create(uuid);
        if (device == NULL) {
            ERROR("Unable to connect to device\n");
            return -1;
        }
    }

    DEBUG("Connected to device with UUID %s\n", uuid);

    lockdown_t *lockdown = lockdown_open(device);
    if (lockdown == NULL) {
        ERROR("Lockdown connection failed\n");
        device_free(device);
        return -1;
    }

    if ((lockdown_get_string(lockdown, "HardwareModel", &product) != LOCKDOWN_E_SUCCESS)
        || (lockdown_get_string(lockdown, "BuildVersion", &build) != LOCKDOWN_E_SUCCESS)) {
        ERROR("Could not get device information\n");
        lockdown_free(lockdown);
        device_free(device);
        return -1;
    }

    DEBUG("Device is a %s with build %s\n", product, build);

    if (verify_product(product, build) != 0) {
        ERROR("Device is not supported\n");
        return -1;
    }

    plist_t pl = NULL;
    lockdown_get_value(lockdown, NULL, "ActivationState", &pl);
    if (pl && plist_get_node_type(pl) == PLIST_STRING) {
        char *as = NULL;
        plist_get_string_val(pl, &as);
        plist_free(pl);
        if (as) {
            if (strcmp(as, "Unactivated") == 0) {
                free(as);
                ERROR("The attached device is not activated. You need to activate it before it can be used with this jailbreak.\n");
                lockdown_free(lockdown);
                device_free(device);
                return -1;
            }
            free(as);
        }
    }

    pl = NULL;
    lockdown_get_value(lockdown, "com.apple.mobile.backup", "WillEncrypt", &pl);
    if (pl && plist_get_node_type(pl) == PLIST_BOOLEAN) {
        char c = 0;
        plist_get_bool_val(pl, &c);
        plist_free(pl);
        if (c) {
            ERROR("You have a device backup password set. You need to disable the backup password in iTunes.\n");
            lockdown_free(lockdown);
            device_free(device);
            return -1;
        }
    }
    lockdown_free(lockdown);
    device_free(device);
    device = NULL;

    idevice_event_subscribe(idevice_event_cb, uuid);
    jailbreak_device(uuid, status_cb);

    return 0;
}
#endif

static void plist_replace_item(plist_t plist, char *name, plist_t item)
{
    if (plist_dict_get_item(plist, name))
        plist_dict_remove_item(plist, name);
    plist_dict_insert_item(plist, name, item);
}

kern_return_t send_message(service_conn_t socket, CFPropertyListRef plist);
CFPropertyListRef receive_message(service_conn_t socket);

static char *real_dmg, *real_dmg_signature, *ddi_dmg;

static void print_data(CFDataRef data)
{
    if (data == NULL) {
        DEBUG("[null]\n");
        return;
    }
    DEBUG("[%.*s]\n", (int) CFDataGetLength(data), CFDataGetBytePtr(data));
}

void qwrite(afc_connection * afc, const char *from, const char *to)
{
    DEBUG("Sending %s -> %s... ", from, to);
    afc_file_ref ref;

    int fd = open(from, O_RDONLY);
    assert(fd != -1);
    size_t size = (size_t) lseek(fd, 0, SEEK_END);
    void *buf = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    assert(buf != MAP_FAILED);

    AFCFileRefOpen(afc, to, 3, &ref);
    AFCFileRefWrite(afc, ref, buf, size);
    AFCFileRefClose(afc, ref);

    DEBUG("done.\n");

    close(fd);
}

int timesl, tries = 0;
volatile int is_ddid = 0;

#undef assert
#define assert(x) (x)

/* badcode is bad */
static void cb2(am_device_notification_callback_info * info, void *foo)
{
    timesl = 1000;

    struct am_device *dev;
    DEBUG("... %x\n", info->msg);
    if (is_ddid)
        CFRunLoopStop(CFRunLoopGetCurrent());

    if (info->msg == ADNCI_MSG_CONNECTED) {
        dev = info->dev;
        tries++;

        if (tries >= 30) {
            is_ddid = -1;
            return;
        }

        AMDeviceConnect(dev);
        assert(AMDeviceIsPaired(dev));
        assert(!AMDeviceValidatePairing(dev));
        assert(!AMDeviceStartSession(dev));

        CFStringRef product = AMDeviceCopyValue(dev, 0, CFSTR("ProductVersion"));
        assert(product);
        UniChar first = CFStringGetCharacterAtIndex(product, 0);
        int epoch = first - '0';
 Retry:
        printf(".");
        fflush(stdout);

        service_conn_t afc_socket = 0;
        struct afc_connection *afc = NULL;
        assert(!AMDeviceStartService(dev, CFSTR("com.apple.afc"), &afc_socket, NULL));
        assert(!AFCConnectionOpen(afc_socket, 0, &afc));
        assert(!AFCDirectoryCreate(afc, "PublicStaging"));

        AFCRemovePath(afc, "PublicStaging/staging.dimage");
        qwrite(afc, real_dmg, "PublicStaging/staging.dimage");
        if (ddi_dmg)
            qwrite(afc, ddi_dmg, "PublicStaging/ddi.dimage");

        service_conn_t mim_socket1 = 0;
        service_conn_t mim_socket2 = 0;
        assert(!AMDeviceStartService(dev, CFSTR("com.apple.mobile.mobile_image_mounter"), &mim_socket1, NULL));
        assert(mim_socket1);

        CFPropertyListRef result = NULL;
        CFMutableDictionaryRef dict = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFDictionarySetValue(dict, CFSTR("Command"), CFSTR("MountImage"));
        CFDictionarySetValue(dict, CFSTR("ImageType"), CFSTR("Developer"));

        CFDictionarySetValue(dict, CFSTR("ImagePath"), CFSTR("/var/mobile/Media/PublicStaging/staging.dimage"));
        
        int fd = open(real_dmg_signature, O_RDONLY);
        assert(fd != -1);
        uint8_t sig[128];
        assert(read(fd, sig, sizeof(sig)) == sizeof(sig));
        close(fd);

        CFDictionarySetValue(dict, CFSTR("ImageSignature"), CFDataCreateWithBytesNoCopy(NULL, sig, sizeof(sig), kCFAllocatorNull));
        send_message(mim_socket1, dict);
        
        if (ddi_dmg) {
            DEBUG("sleep %d\n", timesl);
            usleep(timesl);

            assert(!AFCRenamePath(afc, "PublicStaging/ddi.dimage", "PublicStaging/staging.dimage"));
        }

        DEBUG("receive 1:\n");

        result = receive_message(mim_socket1);
        print_data(CFPropertyListCreateXMLData(NULL, result));

        if (strstr(CFDataGetBytePtr(CFPropertyListCreateXMLData(NULL, result)), "ImageMountFailed")) {
            timesl += 100;
            goto Retry;
        }

        is_ddid = 1;
        CFRunLoopStop(CFRunLoopGetCurrent());
        fflush(stdout);
    }
}

void stroke_lockdownd(device_t * device)
{
    plist_t crashy = plist_new_dict();
    char *request = NULL;
    unsigned int size = 0;
    idevice_connection_t connection;
    uint32_t magic;
    uint32_t sent = 0;
    plist_dict_insert_item(crashy, "Request", plist_new_string("Pair"));
    plist_dict_insert_item(crashy, "PairRecord", plist_new_bool(0));
    plist_to_xml(crashy, &request, &size);

    magic = __builtin_bswap32(size);
    plist_free(crashy);

    if (idevice_connect(device->client, 62078, &connection)) {
        ERROR("Failed to connect to lockdownd.\n");
    }
    idevice_connection_send(connection, &magic, 4, &sent);
    idevice_connection_send(connection, request, size, &sent);

    idevice_connection_receive_timeout(connection, &size, 4, &sent, 1500);
    size = __builtin_bswap32(size);
    if (size) {
        void *ptr = malloc(size);
        idevice_connection_receive_timeout(connection, ptr, &size, &sent, 5000);
    }
    idevice_disconnect(connection);

    // XXX: Wait for lockdownd to start.
    sleep(5);
}

struct mobile_image_mounter_client_private {
    void *parent;
    void *mutex;
};

char* overrides_plist = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
"<plist version=\"1.0\">\n"
"<dict>\n"
"    <key>com.apple.syslogd</key>\n"
"    <dict>\n"
"        <key>Disabled</key>\n"
"        <true/>\n"
"    </dict>\n"
"</dict>\n"
"</plist>\n";

void callback(ZipInfo* info, CDFile* file, size_t progress) {
    int percentDone = progress * 100/file->compressedSize;
    printf("Getting: %d%%\n", percentDone);
}

int jailbreak_device(const char *uuid, status_cb_t cb)
{
    char backup_dir[1024];
    device_t *device = NULL;
    char *build = NULL;
    char *product = NULL;
    struct lockdownd_service_descriptor desc = { 0, 0 };
    int is_jailbroken = 0;

    if (!uuid) {
        ERROR("Missing device UDID\n");
        return -1;
    }

    assert(cb);

    tmpnam(backup_dir);
    DEBUG("Backing up files to %s\n", backup_dir);

    // Wait for a connection
    DEBUG("Connecting to device...\n");

    cb("Connecting to device...\n", 2);

    int retries = 20;
    int i = 0;
    while (!connected && (i++ < retries)) {
        sleep(1);
    }

    if (!connected) {
        ERROR("Device connection failed\n");
        return -1;
    }
    // Open a connection to our device
    DEBUG("Opening connection to device\n");
    device = device_create(uuid);
    if (device == NULL) {
        ERROR("Unable to connect to device\n");
    }

    lockdown_t *lockdown = lockdown_open(device);
    if (lockdown == NULL) {
        WARN("Lockdown connection failed\n");
        device_free(device);
        return -1;
    }

    if ((lockdown_get_string(lockdown, "HardwareModel", &product) != LOCKDOWN_E_SUCCESS)
        || (lockdown_get_string(lockdown, "BuildVersion", &build) != LOCKDOWN_E_SUCCESS)) {
        ERROR("Could not get device information\n");
        if (product) {
            free(product);
        }
        if (build) {
            free(build);
        }
        lockdown_free(lockdown);
        device_free(device);
        return -1;
    }

    cb("Getting payload files from Apple... (if this fails, your internet connection has issues...)\n", 5);

    struct stat st;
    /* Hackcheck for network connection... */ 
    ZipInfo* info2 = PartialZipInit("http://appldnld.apple.com/iOS6.1/091-2397.20130319.EEae9/iPad2,1_6.1.3_10B329_Restore.ipsw");
    if(!info2) {
        ERROR("Cannot make PartialZip context\n");
        return -1;
    }
    PartialZipSetProgressCallback(info2, callback);
    CDFile* file = PartialZipFindFile(info2, "BuildManifest.plist");
    if(!file) {
        ERROR("cannot file find\n");
        return -1;
    }
    PartialZipRelease(info2);

    DEBUG("Device info: %s, %s\n", product, build);

    DEBUG("Beginning jailbreak, this may take a while...\n");
    cb("Gathering information to generate jailbreak data...\n", 10);

    uint16_t port = 0;

    is_ddid = 0;

    if (lockdown_start_service(lockdown, "com.apple.afc2", &port) == 0) {
        char **fileinfo = NULL;
        uint32_t ffmt = 0;

        afc_client_t afc2 = NULL;
        desc.port = port;
        afc_client_new(device->client, &desc, &afc2);
        if (afc2) {
            afc_get_file_info(afc2, "/Applications", &fileinfo);
            if (fileinfo) {
                int i;
                for (i = 0; fileinfo[i]; i += 2) {
                    if (!strcmp(fileinfo[i], "st_ifmt")) {
                        if (strcmp(fileinfo[i + 1], "S_IFLNK") == 0) {
                            ffmt = 1;
                        }
                        break;
                    }
                }
                afc_free_dictionary(fileinfo);
                fileinfo = NULL;

                if (ffmt) {
                    ERROR("Device already jailbroken! Detected stash.");
                    afc_client_free(afc2);
                    lockdown_free(lockdown);
                    device_free(device);
                    cb("Device already jailbroken, detected stash.", 100);
                    return 0;
                }
            }

            afc_get_file_info(afc2, "/private/etc/launchd.conf", &fileinfo);
            if (fileinfo) {
                ERROR("Device already jailbroken! Detected untether.");
                afc_client_free(afc2);
                lockdown_free(lockdown);
                device_free(device);
                cb("Device already jailbroken, detected untether.", 100);
                return 0;
            }

            afc_client_free(afc2);
        }
    }

    if (lockdown_start_service(lockdown, "com.apple.afc", &port) != 0) {
        ERROR("Failed to start AFC service", 0);
        lockdown_free(lockdown);
        device_free(device);
        return -1;
    }
    lockdown_free(lockdown);
    lockdown = NULL;

    afc_client_t afc = NULL;
    desc.port = port;
    afc_client_new(device->client, &desc, &afc);
    if (!afc) {
        ERROR("Could not connect to AFC service\n");
        device_free(device);
        return -1;
    }
    // check if directory exists
    char **list = NULL;
    if (afc_read_directory(afc, "/" AFCTMP, &list) != AFC_E_SUCCESS) {
        // we're good, directory does not exist.
    } else {
        free_dictionary(list);
        WARN("Looks like you attempted to apply this Jailbreak and it failed. Will try to fix now...\n", 0);
        sleep(5);
        goto fix;
    }

    afc_client_free(afc);
    afc = NULL;

    /** SYMLINK: Recordings/.haxx -> /var */
    rmdir_recursive(backup_dir);
    mkdir(backup_dir, 0755);
    char *bargv[] = {
        "idevicebackup2",
        "backup",
        backup_dir,
        NULL
    };
    char *rargv[] = {
        "idevicebackup2",
        "restore",
        "--system",
        "--settings",
        "--reboot",
        backup_dir,
        NULL
    };
    char *rargv2[] = {
        "idevicebackup2",
        "restore",
        "--system",
        "--settings",
        backup_dir,
        NULL
    };
    backup_t *backup;

    rmdir_recursive(backup_dir);
    mkdir(backup_dir, 0755);
    idevicebackup2(3, bargv);

    cb("Sending initial data...\n", 15);
    backup = backup_open(backup_dir, uuid);
    if (!backup) {
        fprintf(stderr, "ERROR: failed to open backup\n");
        return -1;
    }

    /* Reboot for the sake of posterity. Gets rid of all Developer images mounted. */
    {
        if (backup_mkdir(backup, "MediaDomain", "Media/Recordings", 0755, 501, 501, 4) != 0) {
            ERROR("Could not make folder\n");
            return -1;
        }

        if (backup_symlink(backup, "MediaDomain", "Media/Recordings/.haxx", "/var/db/launchd.db/com.apple.launchd", 501, 501, 4) != 0) {
            ERROR("Failed to symlink var!\n");
            return -1;
        }

        FILE *f = fopen("payload/common/overrides.plist", "wb+");
        fwrite(overrides_plist, sizeof(overrides_plist), 1, f);
        fclose(f);
        if (backup_add_file_from_path(backup, "MediaDomain", "payload/common/overrides.plist", "Media/Recordings/.haxx/overrides.plist", 0100755, 0, 0, 4) != 0) {
            ERROR("Could not add tar");
            return -1;
        }
    }
    idevicebackup2(6, rargv);
    unlink("payload/common/overrides.plist");
    backup_free(backup);

    cb("Waiting for reboot. Do not unplug your device.\n", 18);

    /********************************************************/
    /* wait for device reboot */
    /********************************************************/

    // wait for disconnect
    while (connected) {
        sleep(2);
    }
    DEBUG("Device %s disconnected\n", uuid);

    // wait for device to connect
    while (!connected) {
        sleep(2);
    }
    DEBUG("Device %s detected. Connecting...\n", uuid);
    sleep(10);

    /********************************************************/
    /* wait for device to finish booting to springboard */
    /********************************************************/
    device = device_create(uuid);
    if (!device) {
        ERROR("ERROR: Could not connect to device. Aborting.");
        // we can't recover since the device connection failed...
        return -1;
    }

    lockdown = lockdown_open(device);
    if (!lockdown) {
        device_free(device);
        ERROR("ERROR: Could not connect to lockdown. Aborting");
        // we can't recover since the device connection failed...
        return -1;
    }

    retries = 100;
    int done = 0;
    sbservices_client_t sbsc = NULL;
    plist_t state = NULL;
    DEBUG("Waiting for SpringBoard...\n");

    while (!done && (retries-- > 0)) {
        port = 0;
        lockdown_start_service(lockdown, "com.apple.springboardservices",
                               &port);
        if (!port) {
            continue;
        }
        sbsc = NULL;
        desc.port = port;
        sbservices_client_new(device->client, &desc, &sbsc);
        if (!sbsc) {
            continue;
        }
        if (sbservices_get_icon_state(sbsc, &state, "2") ==
            SBSERVICES_E_SUCCESS) {
            plist_free(state);
            state = NULL;
            done = 1;
        }
        sbservices_client_free(sbsc);
        if (done) {
            sleep(3);
            DEBUG("bootup complete\n");
            break;
        }
        sleep(3);
    }
    lockdown_free(lockdown);
    lockdown = NULL;

    /* Download images. */
    if(stat("payload/iOSUpdaterHelper.dmg", &st)) {
        ZipInfo* info = PartialZipInit("http://appldnld.apple.com/iOS6/041-8518.20121029.CCrt9/iOSUpdater.ipa");
        if(!info) {
            ERROR("Cannot make PartialZip context\n");
            return -1;
        }
        PartialZipSetProgressCallback(info, callback);
        CDFile* file = PartialZipFindFile(info, "Payload/iOSUpdater.app/iOSUpdaterHelper.dmg");
        if(!file)
        {
            ERROR("Cannot find file in zip 1\n");
            return -1;
        }
        unsigned char* data = PartialZipGetFile(info, file);
        int dataLen = file->size; 
        PartialZipRelease(info);
        data = realloc(data, dataLen + 1);
        data[dataLen] = '\0';
        FILE* out;
        out = fopen("payload/iOSUpdaterHelper.dmg", "wb+");
        if (out == NULL)
        {
            ERROR("Failed to open file");
            return -1;
        }
        fwrite(data, sizeof(char), dataLen, out);
        fclose(out);
        free(data);
    }

    if(stat("payload/iOSUpdaterHelper.dmg.signature", &st)) {
        ZipInfo* info = PartialZipInit("http://appldnld.apple.com/iOS6/041-8518.20121029.CCrt9/iOSUpdater.ipa");
        if(!info) {
            ERROR("Cannot make PartialZip context\n");
            return -1;
        }
        PartialZipSetProgressCallback(info, callback);
        CDFile* file = PartialZipFindFile(info, "Payload/iOSUpdater.app/iOSUpdaterHelper.dmg.signature");
        if(!file)
        {
            ERROR("Cannot find file in zip 2\n");
            return -1;
        }
        unsigned char* data = PartialZipGetFile(info, file);
        int dataLen = file->size; 
        PartialZipRelease(info);
        data = realloc(data, dataLen + 1);
        data[dataLen] = '\0';
        FILE* out;
        out = fopen("payload/iOSUpdaterHelper.dmg.signature", "wb+");
        if (out == NULL)
        {
            ERROR("Failed to open file");
            return -1;
        }
        fwrite(data, sizeof(char), dataLen, out);
        fclose(out);
        free(data);
    }

    /*
     * Upload DDI original. 
     */
    real_dmg = "payload/iOSUpdaterHelper.dmg";
    real_dmg_signature = "payload/iOSUpdaterHelper.dmg.signature";
    ddi_dmg = "payload/hax.dmg";

    cb("Waiting for device...\n", 25);

    //dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
                   AMDAddLogFileDescriptor(2); 
                   am_device_notification * notif;
                   assert(!AMDeviceNotificationSubscribe(cb2, 0, 0, NULL, &notif)); 
                   CFRunLoopRun();
    //});

    while (!is_ddid) ;

    if (is_ddid == -1) {
        ERROR("Failed to mount image\n");
        cb("Failed to mount image\n", 10);
        return -1;
    }

    /** DDI Mounted! */

    if (!lockdown)
        lockdown = lockdown_open(device);

    cb("Remounting root...\n", 40);
    if (lockdown_start_service(lockdown, "r", &port) != 0) {
        DEBUG("Timed out on doing so... doesn't really matter though..\n");
    }

    /* Delete files */
    unlink("payload/iOSUpdaterHelper.dmg");
    unlink("payload/iOSUpdaterHelper.dmg.signature");

    /** Install bootstrap. */
    rmdir_recursive_afc(afc, "/Recordings", 1);

    if (lockdown_start_service(lockdown, "com.apple.afc2", &port) != 0) {
        ERROR("Device failed to mount image proper!\n");
        return -1;
    }

    /*
     * Goody, goody. Let's copy everything over!
     */

    cb("Sending Cydia and untether payload to the device...\n", 70);

    rmdir_recursive(backup_dir);
    mkdir(backup_dir, 0755);

    if (!afc) {
        lockdown = lockdown_open(device);
        port = 0;
        if (lockdown_start_service(lockdown, "com.apple.afc", &port) != 0) {
            WARN("Could not start AFC service. Aborting.\n");
            lockdown_free(lockdown);
            goto leave;
        }
        lockdown_free(lockdown);

        desc.port = port;
        afc_client_new(device->client, &desc, &afc);
        if (!afc) {
            WARN("Could not connect to AFC. Aborting.\n");
            goto leave;
        }
    }
    rmdir_recursive_afc(afc, "/Recordings", 1);
    idevicebackup2(3, bargv);

    backup = backup_open(backup_dir, uuid);
    if (!backup) {
        fprintf(stderr, "ERROR: failed to open backup\n");
        return -1;
    }

    /*
     * Do it again.
     */
    {
        if (backup_mkdir(backup, "MediaDomain", "Media/Recordings", 0755, 501, 501, 4) != 0) {
            ERROR("Could not make folder\n");
            return -1;
        }

        if (backup_symlink(backup, "MediaDomain", "Media/Recordings/.haxx", "/", 501, 501, 4) != 0) {
            ERROR("Failed to symlink root!\n");
            return -1;
        }

        if (backup_mkdir(backup, "MediaDomain", "Media/Recordings/.haxx/var/untether", 0755, 0, 0, 4) != 0) {
            ERROR("Could not make folder\n");
            return -1;
        }

        {
            char jb_path[128];
            char amfi_path[128];
            char launchd_conf_path[128];

            snprintf(jb_path, 128, "payload/common/untether", build, product);
            snprintf(amfi_path, 128, "payload/common/_.dylib", build, product);
            snprintf(launchd_conf_path, 128, "payload/common/launchd.conf", build, product);

            if (backup_add_file_from_path(backup, "MediaDomain", launchd_conf_path, "Media/Recordings/.haxx/var/untether/launchd.conf", 0100644, 0, 0, 4) != 0) {
                ERROR("Could not add launchd.conf");
                return -1;
            }
            if (backup_symlink(backup, "MediaDomain", "Media/Recordings/.haxx/private/etc/launchd.conf", "/private/var/untether/launchd.conf", 0, 0, 4) != 0) {
                ERROR("Failed to symlink launchd.conf!\n");
                return -1;
            }
            if (backup_add_file_from_path(backup, "MediaDomain", "payload/common/tar", "Media/Recordings/.haxx/var/untether/tar", 0100755, 0, 0, 4) != 0) {
                ERROR("Could not add tar");
                return -1;
            }

            if (backup_symlink(backup, "MediaDomain", "Media/Recordings/.haxx/bin/tar", "/private/var/untether/tar", 0, 0, 4) != 0) {
                ERROR("Failed to symlink tar!\n");
                return -1;
            }
            if (backup_symlink(backup, "MediaDomain", "Media/Recordings/.haxx/usr/libexec/dirhelper", "/private/var/untether/dirhelper", 0, 0, 4) != 0) {
                ERROR("Failed to symlink dirhelper!\n");
                return -1;
            }
            if (backup_add_file_from_path(backup, "MediaDomain", "payload/common/install.deb", "Media/Recordings/.haxx/var/untether/install.deb", 0100755, 0, 0, 4) != 0) {
                ERROR("Could not add dirhelper");
                return -1;
            }
            if (backup_add_file_from_path(backup, "MediaDomain", "payload/common/dirhelper", "Media/Recordings/.haxx/var/untether/dirhelper", 0100755, 0, 0, 4) != 0) {
                ERROR("Could not add dirhelper");
                return -1;
            }
            if (backup_add_file_from_path(backup, "MediaDomain", jb_path, "Media/Recordings/.haxx/var/untether/untether", 0100755, 0, 0, 4) != 0) {
                ERROR("Could not add jb");
                return -1;
            }
            if (backup_add_file_from_path(backup, "MediaDomain", amfi_path, "Media/Recordings/.haxx/var/untether/_.dylib", 0100644, 0, 0, 4) != 0) {
                ERROR("Could not add amfi");
                return -1;
            }
            if (backup_add_file_from_path(backup, "MediaDomain", "payload/Cydia.tar", "Media/Recordings/.haxx/var/untether/Cydia.tar", 0100644, 0, 0, 4) != 0) {
                ERROR("Could not add cydia");
                return -1;
            }
        }
    }
    idevicebackup2(5, rargv2);

    backup_free(backup);

    cb("Finalizing...\n", 90);
    DEBUG("Installed jailbreak, fixing up directories.\n");
    rmdir_recursive_afc(afc, "/Recordings", 1);

    /********************************************************/
    /*
     * move back any remaining dirs via AFC 
     */
    /********************************************************/

    is_jailbroken = 1;

 fix:
    DEBUG("Recovering files...\n", 80);
    if (!afc) {
        lockdown = lockdown_open(device);
        port = 0;
        if (lockdown_start_service(lockdown, "com.apple.afc", &port) != 0) {
            WARN("Could not start AFC service. Aborting.\n");
            lockdown_free(lockdown);
            goto leave;
        }
        lockdown_free(lockdown);
        lockdown = NULL;

        desc.port = port;
        afc_client_new(device->client, &desc, &afc);
        if (!afc) {
            WARN("Could not connect to AFC. Aborting.\n");
            goto leave;
        }
    }

    rmdir_recursive(backup_dir);

    WARN("Recovery complete.\n");

    if (is_jailbroken) {
        cb("Your device is now jailbroken, it is now preparing to reboot automatically.\n", 100);
        WARN("Your device is now jailbroken, it is now preparing to reboot automatically.\n");

        /*
         * Reboot device automatically. 
         */
        lockdown = lockdown_open(device);
        diagnostics_relay_client_t diagnostics_client = NULL;
        uint16_t diag_port = 0;

        lockdown_start_service(lockdown, "com.apple.mobile.diagnostics_relay", &diag_port);

        desc.port = diag_port;
        if (diagnostics_relay_client_new(device->client, &desc, &diagnostics_client) == DIAGNOSTICS_RELAY_E_SUCCESS) {
            diagnostics_relay_restart(diagnostics_client, 0);
        }

    } else {
        cb("Your device has encountered an error during the jailbreak process, unplug it and try again.\n", 100);
        WARN("Your device has encountered an error during the jailbreak process, unplug it and try again.\n");
    }

 leave:
    afc_client_free(afc);
    afc = NULL;
    device_free(device);
    device = NULL;
    return 0;
}
