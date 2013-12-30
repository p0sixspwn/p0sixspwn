#ifndef _JAILBREAKER_H_ 
#define _JAILBREAKER_H_

#include <wx/wx.h>

#include "JailbreakWorker.h"

class Jailbreaker : public wxThread
{
private:
	JailbreakWorker* worker;
public:
	Jailbreaker(JailbreakWorker* worker);
	void statusCallback(const char* message, int progress);
	wxThread::ExitCode Entry(void);
};

#endif
