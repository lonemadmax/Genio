/*
 * Copyright 2018 A. Mosca <amoscaster@gmail.com>
 *
 * Adapted from Debugger::ConsoleOutputView class
 * Copyright 2013-2014, Rene Gollent, rene@gollent.com
 *
 * Distributed under the terms of the MIT License
 */


#include "ConsoleIOView.h"

//#include <new>

#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <ScrollView.h>
#include <String.h>
#include "WordTextView.h"

#include <AutoDeleter.h>

#include "GenioNamespace.h"
#include "Log.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ConsoleIOView"

enum {
	MSG_CLEAR_OUTPUT	= 'clou',
	MSG_POST_OUTPUT		= 'poou',
	MSG_STOP_PROCESS	= 'stpr',
	MSG_RUN_PROCESS		= 'runp'
};

struct ConsoleIOView::OutputInfo {
	int32 	fd;
	BString	text;

	OutputInfo(int32 fd, const BString& text)
		:
		fd(fd),
		text(text)
	{
	}
};

ConsoleIOView::ConsoleIOView(const BString& name, const BMessenger& target)
	:
	BGroupView(B_VERTICAL, 0.0f)
	, fWindowTarget(target)
	, fConsoleIOText(nullptr)
	, fPendingOutput(nullptr)
	, fConsoleIOThread(nullptr)
{
	SetName(name);

	try {
		_Init();
	} catch (...) {
		throw;
	}

	this->SetFlags(this->Flags() | B_PULSE_NEEDED);
}

ConsoleIOView::~ConsoleIOView()
{
	delete fPendingOutput;
}

/*static*/ ConsoleIOView*
ConsoleIOView::Create(const BString& name, const BMessenger& target)
{
	ConsoleIOView* self = new ConsoleIOView(name, target);

	try {
		self->_Init();
	} catch (...) {
		delete self;
		throw;
	}

	return self;
}

status_t
ConsoleIOView::RunCommand(BMessage* cmd_message)
{
	cmd_message->what = MSG_RUN_PROCESS;
	return BMessenger(this).SendMessage(cmd_message);
}

void
ConsoleIOView::Pulse()
{
	if (fConsoleIOThread && fConsoleIOThread->IsProcessAlive() != B_OK) {
		// Flush the pipes
		BString outStr("");
		BString errStr("");
		status_t status = fConsoleIOThread->GetFromPipe(outStr, errStr);
		if (status == B_OK) {
			if (outStr != "") {
				ConsoleOutputReceived(1, outStr);
			}
			if (errStr != "") {
				ConsoleOutputReceived(2, errStr);
			}
		} else {
			_StopCommand();
		}
	}
}

void
ConsoleIOView::_StopCommand()
{
	if (fConsoleIOThread) {

		fConsoleIOThread->InterruptExternal();
		fConsoleIOThread->Kill();
		delete fConsoleIOThread;
		fConsoleIOThread = nullptr;

		_BannerMessage("ended   --");

		fStopButton->SetEnabled(false);
		BMessage message(CONSOLEIOTHREAD_EXIT);
		message.AddString("cmd_type", fCmdType);
		Window()->PostMessage(&message);
	}
}

void
ConsoleIOView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case CONSOLEIOTHREAD_STDERR: {
			BString string;
			if (message->FindString("stderr",  &string) == B_OK)
				ConsoleOutputReceived(2, string);
			break;
		}
		case CONSOLEIOTHREAD_STDOUT: {
			BString string;
			if (message->FindString("stdout",  &string) == B_OK)
				ConsoleOutputReceived(1, string);
			break;
		}
		case MSG_CLEAR_OUTPUT:
		{
			fConsoleIOText->SetText("");
			fPendingOutput->MakeEmpty();

			// Used to reload settings too
			fWrapEnabled->SetValue(GenioNames::Settings.wrap_console);
			fConsoleIOText->SetWordWrap(fWrapEnabled->Value());
			fBannerEnabled->SetValue(GenioNames::Settings.console_banner);

			break;
		}
		case MSG_POST_OUTPUT:
		{
			OutputInfo* info = fPendingOutput->RemoveItemAt(0);
			if (info == nullptr)
				break;

			ObjectDeleter<OutputInfo> infoDeleter(info);
			_HandleConsoleOutput(info);
			break;
		}
		case MSG_RUN_PROCESS:
		{
			if (fConsoleIOThread) {
				//this should be prevented by the UI...
				BString msg = "\n *** ";
				msg << B_TRANSLATE("Another command is running.");
				msg << "\n";

				ConsoleOutputReceived(1, msg);
				return;
			}
			fStopButton->SetEnabled(true);
			fConsoleIOThread = new ConsoleIOThread(message, BMessenger(this));
			fConsoleIOThread->Start();
			_BannerMessage("started   ");

			break;
		}
		case CONSOLEIOTHREAD_EXIT:
		case MSG_STOP_PROCESS:
		{
			_StopCommand();
			break;
		}
		default:
			BGroupView::MessageReceived(message);
			break;
	}
}


void
ConsoleIOView::_BannerMessage(BString status)
{
	if (GenioNames::Settings.console_banner == false)
		return;

	BString banner;
	banner  << "--------------------------------"
			<< "   "
			<< fCmdType
			<< " "
			<< status
			<< "--------------------------------\n";

	ConsoleOutputReceived(1, banner);
}

void
ConsoleIOView::AttachedToWindow()
{
	BGroupView::AttachedToWindow();

	fStdoutEnabled->SetValue(B_CONTROL_ON);
	fStderrEnabled->SetValue(B_CONTROL_ON);
	fWrapEnabled->SetEnabled(false);
	fBannerEnabled->SetEnabled(false);

	fWrapEnabled->SetValue(GenioNames::Settings.wrap_console);
	fConsoleIOText->SetWordWrap(fWrapEnabled->Value());
	fBannerEnabled->SetValue(GenioNames::Settings.console_banner);

	fClearButton->SetTarget(this);
	fStopButton->SetTarget(this);
	fStopButton->SetEnabled(false);
}

void
ConsoleIOView::Clear()
{
	BMessenger(this).SendMessage(MSG_CLEAR_OUTPUT);
}

void
ConsoleIOView::ConsoleOutputReceived(int32 fd, const BString& output)
{
	if (fd == 1 && fStdoutEnabled->Value() != B_CONTROL_ON)
		return;
	else if (fd == 2 && fStderrEnabled->Value() != B_CONTROL_ON)
		return;

	OutputInfo* info = new(std::nothrow) OutputInfo(fd, output);
	if (info == nullptr)
		return;

	ObjectDeleter<OutputInfo> infoDeleter(info);
	if (fPendingOutput->AddItem(info)) {
		infoDeleter.Detach();
	}

	BMessenger(this).SendMessage(MSG_POST_OUTPUT);
}

void
ConsoleIOView::EnableStopButton(bool doIt)
{
	fStopButton->SetEnabled(doIt);
}

void
ConsoleIOView::_Init()
{
	fPendingOutput = new OutputInfoList(1, true);

	fConsoleIOText = new WordTextView("console_io");
	fConsoleIOText->SetStylable(true);
	fConsoleIOText->SetDoesUndo(false);
	fConsoleIOText->MakeEditable(false);

	BScrollView* consoleScrollView  = new BScrollView("console_scroll", fConsoleIOText, 0,
			true, true);

	fStdoutEnabled = new BCheckBox(B_TRANSLATE("stdout"));
	fStderrEnabled = new BCheckBox(B_TRANSLATE("stderr"));
	fWrapEnabled = new BCheckBox(B_TRANSLATE("Wrap"));
	fBannerEnabled = new BCheckBox(B_TRANSLATE("Banner"));
	fClearButton = new BButton(B_TRANSLATE("Clear"), new BMessage(MSG_CLEAR_OUTPUT));
	fStopButton = new BButton(B_TRANSLATE("Stop"), new BMessage(MSG_STOP_PROCESS));

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, 0.0f)
		.Add(consoleScrollView, 3.0f)
		.AddGroup(B_VERTICAL, 0.0f)
			.SetInsets(B_USE_SMALL_SPACING)
			.Add(fStdoutEnabled)
			.Add(fStderrEnabled)
			.Add(fWrapEnabled)
			.Add(fBannerEnabled)
			.AddGlue()
			.Add(fClearButton)
			.Add(fStopButton)
		.End()
	.End();

}

void
ConsoleIOView::_HandleConsoleOutput(OutputInfo* info)
{
	if (info->fd == 1 && fStdoutEnabled->Value() != B_CONTROL_ON)
		return;
	else if (info->fd == 2 && fStderrEnabled->Value() != B_CONTROL_ON)
		return;

	text_run_array run;
	run.count = 1;
	run.runs[0].font = be_fixed_font;
	run.runs[0].offset = 0;
	// stderr goes orange
	if (info->fd == 2) {
		run.runs[0].color.red = 236;
		run.runs[0].color.green = 126;
		run.runs[0].color.blue = 14;
	} else {
		run.runs[0].color.red = 0;
		run.runs[0].color.green = 0;
		run.runs[0].color.blue = 0;
	}
	run.runs[0].color.alpha = 255;

	bool autoScroll = false;
	BScrollBar* scroller = fConsoleIOText->ScrollBar(B_VERTICAL);
	float min, max;
	scroller->GetRange(&min, &max);
	if (min == max || scroller->Value() == max)
		autoScroll = true;

	fConsoleIOText->Insert(fConsoleIOText->TextLength(), info->text,
		info->text.Length(), &run);
	if (autoScroll) {
		scroller->GetRange(&min, &max);
		fConsoleIOText->ScrollTo(0.0, max);
	}
}

BTextView*
ConsoleIOView::TextView() {
	return fConsoleIOText;
}
