#include "JailbreakMainWindow.h"



JailbreakMainWindow::JailbreakMainWindow(void)
	: wxFrame(NULL, wxID_ANY, wxT(WND_TITLE), wxDefaultPosition, wxSize(WND_WIDTH, WND_HEIGHT), (wxDEFAULT_FRAME_STYLE) & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX))
{
	wxPanel* panel = new wxPanel(this, wxID_ANY, wxPoint(0, 0), wxSize(WND_WIDTH, WND_HEIGHT));

	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
#if defined(__WXGTK__)
#define FNTSIZE 10
#endif
#if defined(__WXOSX_COCOA__) || defined(__WXMAC__)
#define FNTSIZE 12
#endif
#if defined(__WXMSW__)
#define FNTSIZE 9
#endif
	wxFont fnt(FNTSIZE, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	wxStaticText* lbTop = new wxStaticText(panel, wxID_ANY, wxT("Welcome to p0sixspwn, an iOS 6.1.3-6.1.6 userland jailbreaking\nprogram. Make a backup of your device before proceeding.\n"), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE | wxALIGN_LEFT);
	lbTop->SetFont(fnt);
	lbTop->Wrap(WND_WIDTH-20);

	lbStatus = new wxStaticText(panel, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
	lbStatus->SetFont(fnt);

#if defined (__WXMAC__)
	progressBar = new wxGauge(panel, wxID_ANY, 100, wxDefaultPosition, wxSize(300,11), wxGA_HORIZONTAL | wxGA_SMOOTH);
#else
	progressBar = new wxGauge(panel, wxID_ANY, 100, wxDefaultPosition, wxSize(300,17), wxGA_HORIZONTAL | wxGA_SMOOTH);
#endif

	btnStart = new wxButton(panel, 1111, wxT("Jailbreak"));
	btnStart->Enable(0);
	Connect(1111, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(JailbreakMainWindow::handleStartClicked));

	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->Add(progressBar, 0, wxALIGN_CENTER_VERTICAL | wxALL, 10);
	hbox->Add(btnStart, 0, wxALIGN_CENTER_VERTICAL | wxALL, 10);

	wxStaticText* lbCredits = new wxStaticText(panel, wxID_ANY, wxT("greetz to: @winocm, @iH8sn0w, @SquiffyPwn, @planetbeing, @pimskeks\n"), wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER | wxST_NO_AUTORESIZE);

	lbCredits->SetFont(fnt);

	wxBoxSizer* hbox2 = new wxBoxSizer(wxHORIZONTAL);

	vbox->Add(lbTop, 0, wxEXPAND | wxALL, 10);
	vbox->Add(lbStatus, 1, wxEXPAND | wxALL, 10);
	vbox->Add(hbox, 0, wxCENTER | wxALL, 4);
	vbox->Add(lbCredits, 0, wxCENTER | wxALL, 10);
	vbox->Add(hbox2, 0, wxCENTER | wxALL, 4);

	panel->SetSizer(vbox);

	Centre();

	this->worker = new JailbreakWorker((JailbreakMainWindow*)this);
	this->closeBlocked = 0;
	this->Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(JailbreakMainWindow::OnClose));
}

#define THREAD_SAFE(X) \
	if (!wxIsMainThread()) { \
		wxMutexGuiEnter(); \
		X; \
		wxMutexGuiLeave(); \
	} else { \
		X; \
	}

int JailbreakMainWindow::msgBox(const wxString& message, const wxString& caption, int style)
{
	int res;
	THREAD_SAFE(res = wxMessageBox(message, caption, style));
	return res;
}

void JailbreakMainWindow::setButtonEnabled(int enabled)
{
	THREAD_SAFE(this->btnStart->Enable(enabled));
}

void JailbreakMainWindow::setStatusText(const wxString& text)
{
	THREAD_SAFE(this->lbStatus->SetLabel(text));
}

void JailbreakMainWindow::setProgress(int percentage)
{
	THREAD_SAFE(this->progressBar->SetValue(percentage));
}

void JailbreakMainWindow::handleStartClicked(wxCommandEvent& WXUNUSED(event))
{
	this->setButtonEnabled(0);
	this->setProgress(0);
	this->worker->processStart();
}

void JailbreakMainWindow::OnClose(wxCloseEvent& event)
{
	if (this->closeBlocked) {
		event.Veto();
	} else {
		event.Skip();
	}
}

void JailbreakMainWindow::OnQuit(wxCommandEvent& event)
{
	Close(true);
}

