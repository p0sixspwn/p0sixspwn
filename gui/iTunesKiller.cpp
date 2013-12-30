#if defined(__APPLE__) || defined(WIN32)
#include "iTunesKiller.h"
#include "JailbreakMainWindow.h"

extern JailbreakMainWindow* wnd;

#include "common.h"

#if defined(__APPLE__)
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>

typedef struct kinfo_proc kinfo_proc;

static int GetBSDProcessList(kinfo_proc **procList, size_t *procCount) /*{{{*/
{
	int err;
	kinfo_proc* result;
	bool done;
	static const int name[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
	size_t length;

	assert(procList != NULL);
	assert(*procList == NULL);
	assert(procCount != NULL);

	*procCount = 0;

	result = NULL;
	done = false;
	do {
		assert(result == NULL);

		length = 0;
		err = sysctl( (int *) name, (sizeof(name) / sizeof(*name)) - 1, NULL, &length, NULL, 0);
		if (err == -1) {
			err = errno;
		}

		if (err == 0) {
			result = (kinfo_proc*)malloc(length);
			if (result == NULL) {
				err = ENOMEM;
			}
		}

		if (err == 0) {
			err = sysctl( (int *) name, (sizeof(name) / sizeof(*name)) - 1, result, &length, NULL, 0);
			if (err == -1) {
				err = errno;
			}
			if (err == 0) {
				done = true;
			} else if (err == ENOMEM) {
				assert(result != NULL);
				free(result);
				result = NULL;
				err = 0;
			}
		}
	} while (err == 0 && ! done);

	if (err != 0 && result != NULL) {
		free(result);
		result = NULL;
	}
	*procList = result;
	if (err == 0) {
		*procCount = length / sizeof(kinfo_proc);
	}

	assert( (err == 0) == (*procList != NULL) );

	return err;
} /*}}}*/
#endif

#if defined(WIN32)
#include <windows.h>
#include <tlhelp32.h>
#define sleep(x) Sleep(x*1000)
#endif

iTunesKiller::iTunesKiller(int* watchdog)
	: wxThread(wxTHREAD_JOINABLE), watchit(watchdog)
{
	DEBUG("ITK:init\n");
}

wxThread::ExitCode iTunesKiller::Entry(void)
{
#if defined(WIN32)
	PROCESSENTRY32 pe;
#endif
	while (*(this->watchit)) {
#if defined(__APPLE__)
		size_t proc_count = 0;
		kinfo_proc *proc_list = NULL;
		GetBSDProcessList(&proc_list, &proc_count);
		int i;
		for (i = 0; i < proc_count; i++) {
			if (!strcmp((&proc_list[i])->kp_proc.p_comm, "Xcode")) {
				wnd->msgBox(wxT("Xcode will need to be closed for the jailbreak process to run. Make sure to also reboot your device otherwise the jailbreak process *will* fail. p0sixspwn will now close."), wxT("p0sixspwn"), wxOK | wxICON_INFORMATION);
				exit(1);
			}
			if ((!strcmp((&proc_list[i])->kp_proc.p_comm, "iTunesHelper")) || (!strcmp((&proc_list[i])->kp_proc.p_comm, "iTunes"))) {
				kill((&proc_list[i])->kp_proc.p_pid, SIGKILL);
			}
		}
		free(proc_list);
#endif
#if defined(WIN32)
		memset(&pe, 0, sizeof(pe));
		pe.dwSize = sizeof(PROCESSENTRY32);

		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE) {
			sleep(2);
			continue;
		}

		BOOL i = Process32First(hSnapshot, &pe);
		while (i) {
			if (!_tcscmp(pe.szExeFile, _T("iTunesHelper.exe")) || !_tcscmp(pe.szExeFile, _T("iTunes.exe"))) {
				HANDLE p = OpenProcess(PROCESS_ALL_ACCESS, 0, pe.th32ProcessID);
				if (p != INVALID_HANDLE_VALUE) {
					TerminateProcess(p, 0);
					CloseHandle(p);
				}
			}
			i = Process32Next(hSnapshot, &pe);
		}
		CloseHandle(hSnapshot);
#endif
		sleep(2);
	}
	DEBUG("ITK:Exiting.\n");
	return 0;
}
#endif
