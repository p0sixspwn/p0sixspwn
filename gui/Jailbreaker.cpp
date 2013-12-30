#include "Jailbreaker.h"
#include "common.h"

static Jailbreaker* self;

static void _status_cb(const char* message, int progress)
{
	self->statusCallback(message, progress);
}

Jailbreaker::Jailbreaker(JailbreakWorker* worker)
	: wxThread(wxTHREAD_JOINABLE), worker(worker)
{
	self = this;
}

void Jailbreaker::statusCallback(const char* message, int progress)
{
	worker->processStatus(message, progress);
}

wxThread::ExitCode Jailbreaker::Entry(void)
{
	char* uuid = strdup(worker->getUUID());
	jailbreak_device(uuid, _status_cb);
	free(uuid);

	const char* error = "Done!";

	worker->processFinished(error);
	return 0;
}
