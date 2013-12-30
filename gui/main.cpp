#include <wx/wx.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include "common.h"

#include "JailbreakMainWindow.h"

#ifdef WIN32
#include <windows.h>
#endif

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#define         ws2s(as)   (std::string((as).mb_str(wxConvUTF8)))

class Jailbreak : public wxApp
{
	virtual bool OnInit();
};

bool Jailbreak::OnInit()
{
#ifndef WIN32
#if defined(__APPLE__)
    char argv0[1024];
    uint32_t argv0_size = sizeof(argv0);
    _NSGetExecutablePath(argv0, &argv0_size);
#else
	std::string argv_0 = ws2s(argv[0]);
	const char* argv0 = argv_0.c_str();
#endif
	char* name = strrchr((char*)argv0, '/');
	if (name) {
		int nlen = strlen(argv0)-strlen(name);
		char path[512];
		memcpy(path, argv0, nlen);
		path[nlen] = 0;
		DEBUG("setting working directory to %s\n", path);
		if (chdir(path) != 0) {
			DEBUG("unable to set working directory\n");
		}
	}
#else
	TCHAR mfn[512];
	mfn[0] = 0;
	int mfl = GetModuleFileName(NULL, mfn, 512);
	if (mfl > 0) {
		int i;
		for (i = mfl-1; i >= 0; i--) {
			if ((mfn[i] == '/') || (mfn[i] == '\\')) {
				mfn[i] = '\0';
				break;
			}
		}
		if (!SetCurrentDirectory(mfn)) {
			DEBUG("unable to set working directory\n");
		}
	}
#endif

	JailbreakMainWindow* mainWnd = new JailbreakMainWindow();
	mainWnd->Show(true);
	SetTopWindow(mainWnd);

	return true;
}

IMPLEMENT_APP(Jailbreak)
