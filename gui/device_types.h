#ifndef _DEVICE_TYPES_H_
#define _DEVICE_TYPES_H_

struct device_type_t {
	const char* productType;
	const char* displayName;
};

static struct device_type_t device_types[] = {
	{"M68AP", "iPhone"},
	{"N45AP", "iPod touch"},
	{"N82AP", "iPhone 3G"},
	{"N72AP", "iPod touch 2G"},
	{"N88AP", "iPhone 3GS"},
	{"N18AP", "iPod touch 3G"},
	{"K48AP", "iPad"},
	{"N90AP", "iPhone 4 (GSM)"},
	{"N81AP", "iPod touch 4G"},
	{"K66AP", "Apple TV 2G"},
	{"N92AP", "iPhone 4 (CDMA)"},
	{"N90BAP", "iPhone 4 (GSM, revision A)"},
	{"K93AP", "iPad 2"},
	{"K94AP", "iPad 2 (GSM)"},
	{"K95AP", "iPad 2 (CDMA)"},
	{"K93AAP", "iPad 2 (Wi-Fi, revision A)"},
	{"P105AP", "iPad mini"},
	{"P106AP", "iPad mini (GSM)"},
	{"P107AP", "iPad mini (CDMA)"},
	{"N94AP", "iPhone 4S"},
	{"N41AP", "iPhone 5 (GSM)"},
	{"N42AP", "iPhone 5 (Global/CDMA)"},
	{"N48AP", "iPhone 5c (GSM)"},
	{"N49AP", "iPhone 5c (Global/CDMA)"},
	{"N51AP", "iPhone 5s (GSM)"},
	{"N53AP", "iPhone 5s (Global/CDMA)"},
	{"J1AP", "iPad 3"},
	{"J2AP", "iPad 3 (GSM)"},
	{"J2AAP", "iPad 3 (CDMA)"},
	{"P101AP", "iPad 4"},
	{"P102AP", "iPad 4 (GSM)"},
	{"P103AP", "iPad 4 (CDMA)"},
	{"N78AP", "iPod touch 5G"},
	{"J33AP", "Apple TV 3G"},
	{"J33IAP", "Apple TV 3.1G"},
	{NULL, NULL}
};

#endif
