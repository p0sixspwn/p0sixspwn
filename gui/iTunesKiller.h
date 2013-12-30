#ifndef __ITUNESKILLER_H 
#define __ITUNESKILLER_H
#if defined(__APPLE__) || defined(WIN32)
#include <wx/wx.h>

class iTunesKiller : public wxThread
{
private:
	int* watchit;
public:
	iTunesKiller(int* watchdog);
	wxThread::ExitCode Entry(void);
};
#endif
#endif /* __ITUNESKILLER_H */
