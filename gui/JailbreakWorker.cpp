#include <cstdio>
#include "JailbreakWorker.h"
#include "JailbreakMainWindow.h"
#include "Jailbreaker.h"
#include "device_types.h"
#include "common.h"
#include "iTunesKiller.h"

JailbreakMainWindow* wnd;

extern "C" {
	typedef int16_t userpref_error_t;
	extern userpref_error_t userpref_remove_device_public_key(const char *uuid);
}

static int detection_blocked = 0;
static JailbreakWorker* self;

static const char* getDeviceName(const char* productType)
{
	int i = 0;
	while (device_types[i].productType) {
		if (strcmp(device_types[i].productType, productType) == 0) {
			return device_types[i].displayName;
		}
		i++;
	}
	return "(Unknown device)";
}

static void device_event_cb(const idevice_event_t* event, void* userdata)
{
	if (!detection_blocked) {
		self->DeviceEventCB(event, userdata);
	}
	jb_device_event_cb(event, (void*)event->udid);
}

JailbreakWorker::JailbreakWorker(void* main)
	: mainWnd(main), device_count(0)
{
	self = this;

	this->current_uuid = NULL;

	int num = 0;
	char **devices = NULL;
	idevice_get_device_list(&devices, &num);
	idevice_device_list_free(devices);
	if (num == 0) {
		this->checkDevice();
	}

	idevice_event_subscribe(&device_event_cb, NULL);
}

JailbreakWorker::~JailbreakWorker(void)
{
	idevice_event_unsubscribe();
	if (this->current_uuid) {
		free(this->current_uuid);
	}
}

void JailbreakWorker::setUUID(const char* uuid)
{
	if (this->current_uuid) {
		free(this->current_uuid);
		this->current_uuid = NULL;
	}
	if (uuid) {
		this->current_uuid = strdup(uuid);
	}
}

char* JailbreakWorker::getUUID(void)
{
	return current_uuid;
}

void JailbreakWorker::DeviceEventCB(const idevice_event_t *event, void *user_data)
{
	if (event->event == IDEVICE_DEVICE_ADD) {
		this->device_count++;
		this->checkDevice();
	} else if (event->event == IDEVICE_DEVICE_REMOVE) {
		this->device_count--;
		this->checkDevice();
	}
}

void JailbreakWorker::checkDevice()
{
	JailbreakMainWindow* mainwnd = (JailbreakMainWindow*)this->mainWnd;
	wnd = mainwnd;

	this->setUUID(NULL);

	if (this->device_count == 0) {
		mainwnd->setButtonEnabled(0);
		mainwnd->setProgress(0);
		mainwnd->setStatusText(wxT("Plug in your device to begin."));
	} else if (this->device_count == 1) {
		idevice_t dev = NULL;
		idevice_error_t ierr = idevice_new(&dev, NULL);
		if (ierr != IDEVICE_E_SUCCESS) {
			wxString str;
			str.Printf(wxT("Error detecting device type (idevice error %d)"), ierr);
			mainwnd->setStatusText(str);
			return;
		}

		lockdownd_client_t client = NULL;
		lockdownd_error_t lerr = lockdownd_client_new_with_handshake(dev, &client, "jailbreak");
		if (lerr == LOCKDOWN_E_PASSWORD_PROTECTED) {
			lockdownd_client_free(client);
			idevice_free(dev);
			wxString str;
			str.Printf(wxT("Your device has a passcode set, please disable it."));
			mainwnd->setStatusText(str);
			return;
		} else if (lerr == LOCKDOWN_E_INVALID_HOST_ID) {
			lerr = lockdownd_unpair(client, NULL);
			if (lerr == LOCKDOWN_E_SUCCESS) {
				char *devuuid = NULL;
				idevice_get_udid(dev, &devuuid);
				if (devuuid) {
					userpref_remove_device_public_key(devuuid);
					free(devuuid);
				}
			}
			lockdownd_client_free(client);
			idevice_free(dev);
			wxString str;
			str.Printf(wxT("Device detection failed due an internal error. Please unplug device and plug it back in."));
			mainwnd->setStatusText(str);
			return;
		} else if (lerr != LOCKDOWN_E_SUCCESS) {
			idevice_free(dev);
			wxString str;
			str.Printf(wxT("Error detecting device (lockdown error %d)"), lerr);
			mainwnd->setStatusText(str);
			return;
		}

		plist_t node = NULL;
		char* productType = NULL;
		char* productVersion = NULL;
		char* buildVersion = NULL;

		node = NULL;
		lerr = lockdownd_get_value(client, NULL, "HardwareModel", &node);
		if (node) {
			plist_get_string_val(node, &productType);
			plist_free(node);
		}
		if ((lerr != LOCKDOWN_E_SUCCESS) || !productType) {
			lockdownd_client_free(client);
			idevice_free(dev);
			wxString str;
			str.Printf(wxT("Error getting product type (lockdown error %d)"), lerr);
			mainwnd->setStatusText(str);
			return;
		}

		node = NULL;
		lerr = lockdownd_get_value(client, NULL, "ProductVersion", &node);
		if (node) {
			plist_get_string_val(node, &productVersion);
			plist_free(node);
		}
		if ((lerr != LOCKDOWN_E_SUCCESS) || !productVersion) {
			free(productType);
			lockdownd_client_free(client);
			idevice_free(dev);
			wxString str;
			str.Printf(wxT("Error getting product version (lockdownd error %d)"), lerr);
			mainwnd->setStatusText(str);
			return;
		}

		node = NULL;
		lerr = lockdownd_get_value(client, NULL, "BuildVersion", &node);
		if (node) {
			plist_get_string_val(node, &buildVersion);
			plist_free(node);
		}
		if ((lerr != LOCKDOWN_E_SUCCESS) || !buildVersion) {
			free(productType);
			free(productVersion);
			lockdownd_client_free(client);
			idevice_free(dev);
			wxString str;
			str.Printf(wxT("Error getting build version (lockdownd error %d)"), lerr);
			mainwnd->setStatusText(str);
			return;
		}

		if (verify_product(productType, buildVersion)) {
			mainwnd->setStatusText(wxT("The attached device is not supported."));
			free(productType);
			free(productVersion);
			free(buildVersion);
			return;
		}

		int cc = check_consistency(productType, buildVersion);
		if (cc == 0) {
			// Consistency check passed
		} else if (cc == -1) {
			mainwnd->setStatusText(wxT("Consistency check failed: attached device not supported"));
			free(productType);
			free(productVersion);
			free(buildVersion);
			lockdownd_client_free(client);
			idevice_free(dev);
			return;
		} else if (cc == -2) {
			mainwnd->setStatusText(wxT("Consistency check failed: could not find required files"));
			free(productType);
			free(productVersion);
			free(buildVersion);
			lockdownd_client_free(client);
			idevice_free(dev);
			return;
		} else {
			mainwnd->setStatusText(wxT("Consistency check failed: unknown error"));
			free(productType);
			free(productVersion);
			free(buildVersion);
			lockdownd_client_free(client);
			idevice_free(dev);
			return;
		}

		node = NULL;
		lockdownd_get_value(client, NULL, "PasswordProtected", &node);
		if (node) {
			uint8_t pcenabled = 0;
			plist_get_bool_val(node, &pcenabled);
			plist_free(node);
			if (pcenabled) {
			        wxString str;
				str.Printf(wxT("Your device has a passcode set, please disable it."));
				mainwnd->setStatusText(str);
				return;
			}
		}

		wxString str;
		str.Printf(wxT("%s with iOS %s (%s) detected. Click 'Jailbreak' to begin."), wxString(getDeviceName(productType), wxConvUTF8).c_str(), wxString(productVersion, wxConvUTF8).c_str(), wxString(buildVersion, wxConvUTF8).c_str());
		mainwnd->setStatusText(str);

		int ready_to_go = 1;

		plist_t pl = NULL;
		lockdownd_get_value(client, NULL, "ActivationState", &pl);
		if (pl && plist_get_node_type(pl) == PLIST_STRING) {
			char* as = NULL;
			plist_get_string_val(pl, &as);
			plist_free(pl);
			if (as) {
				if (strcmp(as, "Unactivated") == 0) {
					ready_to_go = 0;
					mainwnd->msgBox(wxT("The attached device is not activated. You need to activate it."), wxT("Error"), wxOK | wxICON_ERROR);
				}
				free(as);
			}
		}

		pl = NULL;
		lockdownd_get_value(client, "com.apple.mobile.backup", "WillEncrypt", &pl);
		lockdownd_client_free(client);

		if (pl && plist_get_node_type(pl) == PLIST_BOOLEAN) {
			uint8_t c = 0;
			plist_get_bool_val(pl, &c);
			plist_free(pl);
			if (c) {
				ready_to_go = 0;
				mainwnd->msgBox(wxT("The attached device has a backup password set. You need to disable the backup password in iTunes."), wxT("Error"), wxOK | wxICON_ERROR);
			}
		}

		if (ready_to_go) {
			char* uuid = NULL;
			idevice_get_udid(dev, &uuid);
			if (uuid) {
				this->setUUID(uuid);
				free(uuid);
			}
			mainwnd->setButtonEnabled(1);
		}
		idevice_free(dev);

		free(productType);
		free(productVersion);
		free(buildVersion);
	} else {
		mainwnd->setButtonEnabled(0);
		mainwnd->setStatusText(wxT("Please attach only one device."));
	}
}

void JailbreakWorker::processStart(void)
{
	JailbreakMainWindow* mainwnd = (JailbreakMainWindow*)this->mainWnd;

	detection_blocked = 1;
	mainwnd->closeBlocked = 1;

#if defined(__APPLE__) || defined(WIN32)
	iTunesKiller* ik = new iTunesKiller(&detection_blocked);
	ik->Create();
	ik->Run();
#endif

	Jailbreaker* jb = new Jailbreaker(this);
	jb->Create();
	jb->Run();
}

void JailbreakWorker::processStatus(const char* msg, int progress)
{
	JailbreakMainWindow* mainwnd = (JailbreakMainWindow*)this->mainWnd;
	if(progress == 0xDEADBEEF) {
		/* Messagebox. */
		mainwnd->msgBox(wxT("Please run the remount application on the device. Dismiss this dialog box when done."), wxT("p0sixspwn"), wxOK | wxICON_INFORMATION);
		return;
	}
	if (msg) {
		wxString str = wxString(msg, wxConvUTF8);
		mainwnd->setStatusText(str);
	}
	mainwnd->setProgress(progress);
}

void JailbreakWorker::processFinished(const char* error)
{
	JailbreakMainWindow* mainwnd = (JailbreakMainWindow*)this->mainWnd;
	detection_blocked = 0;
	mainwnd->closeBlocked = 0;
}
