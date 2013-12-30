#ifndef _JAILBREAK_WORKER_H_
#define _JAILBREAK_WORKER_H_

#include <wx/wx.h>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>

#include "JailbreakMainWindow.h"

class JailbreakWorker
{
private:
	void* mainWnd;
	int device_count;
	char* current_uuid;
public:
	JailbreakWorker(void* main);
	~JailbreakWorker(void);
	void setUUID(const char* uuid);
	char* getUUID(void);
	void DeviceEventCB(const idevice_event_t *event, void *user_data);
	void checkDevice();
	void processStart(void);
	void processStatus(const char* msg, int progress);
	void processFinished(const char* error);
};

#endif
