#ifndef _JAILBREAKMAINWINDOW_H_
#define _JAILBREAKMAINWINDOW_H_

#include <wx/wx.h>
#include "JailbreakWorker.h"

#define WND_TITLE "p0sixspwn"
#define WND_WIDTH 450
#define WND_HEIGHT 300

class JailbreakWorker;

class JailbreakMainWindow : public wxFrame
{
private:
	wxStaticText* lbStatus;
	wxButton* btnStart;
	wxGauge* progressBar;
	JailbreakWorker* worker;

public:
	bool closeBlocked;
	JailbreakMainWindow(void);

	int msgBox(const wxString& message, const wxString& caption, int style);

	void setButtonEnabled(int enabled);
	void setStatusText(const wxString& text);
	void setProgress(int percentage);

	void handleStartClicked(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnQuit(wxCommandEvent& event);
};

enum {
	ID_QUIT = 1,
	ID_ABOUT,
};

#endif

