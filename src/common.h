/** 
 * Debug support
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/afc.h>

#include "device.h"
#include "file.h"
#include "afc.h"
#include "backup.h"
#include "backup_file.h"
#include "lockdown.h"
#include "mbdb.h"

#include "hell.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*status_cb_t)(const char* message, int progress);

int jailbreak_device(const char *uuid, status_cb_t cb);
int check_consistency(char *product, char *build);
int verify_product(char *product, char *build);
void jb_device_event_cb(const idevice_event_t *event, void *user_data);

#ifdef __cplusplus
}
#endif

#undef DEBUG

#if 1

#define DEBUG(x...) \
 	printf("[debug] "), printf(x)

#define ERROR(x...) \
 	do { printf("[error] "), printf(x); } while(0);

#define WARN(x...) \
 	printf("[warn] "), printf(x)

#else

#define DEBUG(x...)
#define ERROR(x...)
#define WARN(x...)

#endif


#endif
