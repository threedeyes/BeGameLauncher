#include "BeApp.h"
#include "BeLauncherBase.h"
#include "BeAboutWindow.h"
#include "BeUrlStringView.h"
#include "BeDirectoryFilePanel.h"
#include "BeDirectoryFilter.h"
#include "BeUtils.h"

#include <Rect.h>
#include <String.h>
#include <StringList.h>
#include <StringView.h>
#include <CheckBox.h>
#include <TextControl.h>
#include <ListView.h>
#include <ScrollView.h>
#include <Message.h>
#include <Font.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <InterfaceDefs.h>
#include <Directory.h>
#include <Entry.h>

#include <Catalog.h>

#include <posix/stdlib.h>

#ifndef SIGNATURE
#error "Application SIGNATURE is not defined. Check your build system."
#endif // !SIGNATURE

#undef  B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT          "Xash3DGameLauncher"

// Launcher Settings
#define TITLE                          "Xash3D Launcher"
#define VERSION                        "0.19.2"
#define PACKAGE_DIR                    "Xash3D"
#define SETTINGS_FILE                  "Xash3DLauncher.set"
#define EXECUTABLE_FILE                "Xash3D"
#define DATA_PATH_OPT                  "XASH3D_BASEDIR"
#define MIRROR_PATH_OPT                "XASH3D_MIRRORDIR"
#define GAME_OPT                       "XASH3D_GAME"
#define EXTRAS_DIR_NAME                "/extras"
#define VALVE_DIR_NAME                 "valve"

// Various Strings
#define L_ABOUT_STRING                 B_TRANSLATE("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do " \
                                       "eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim" \
                                       "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea" \
                                       "commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit" \
                                       "esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat " \
                                       "cupidatat non proident, sunt in culpa qui officia deserunt mollit anim " \
                                       "id est laborum.\n\n")
#define L_ABOUT_THANKS_STR_H           B_TRANSLATE("Thanks to:\n\t")
#define L_ABOUT_THANKS_STR             B_TRANSLATE("- my gf")
#define L_ABOUT_LINK                   B_TRANSLATE("http://exlmoto.ru")
#define L_ABOUT_LINK_DESC              B_TRANSLATE("Some useful link: ")
#define L_DATA_LINK                    B_TRANSLATE("https://store.steampowered.com/")
#define L_DATA_FILES_LINK_D            B_TRANSLATE("Buy data files: ")

// Additional options
#define S_CHECKBOX_OPTION              "GAME_OPTION"
#define L_CHECKBOX_OPTION              B_TRANSLATE("Override path to the Xash3D required libraries:")
#define L_CHECKBOX_OPTION_TOOLTIP      B_TRANSLATE("Check to override path to the Xash3D required libraries.")
#define L_EXTRA_TEXT_CONTROL_TOOLTIP   B_TRANSLATE("Path to a directory with Xash3D required libraries.")
#define L_EXTRA_BUTTON_BROWSE          B_TRANSLATE("...")
#define L_EXTRA_BUTTON_BROWSE_TOOLTIP  B_TRANSLATE("Click to open the file dialog.")
#define L_ADDITIONAL_FILE_PANEL_TITLE  B_TRANSLATE("Please choose a libraries folder")
#define L_ERROR_NO_VALVE_CATALOG       B_TRANSLATE("Game files directory does not contain the \"valve\" catalog.")
#define L_GAME_LIST_LABEL              B_TRANSLATE("Please select a Game/Mod:")
#define O_CHECKBOX_OPTION              "checkBoxOption"
#define O_ABOUT_LINK                   "aboutLink"
#define O_ABOUT_LINK_DESC              "aboutLinkDesc"
#define O_DATA_LINK                    "dataLink"
#define O_DATA_LINK_DESC               "dataLinkDesc"
#define O_VIEW_SCROLL                  "viewScroll"
#define O_GAME_LIST_LABEL              "gameListLabel"
#define G_GAMES_LIST_HEIGHT            80.0f
#define G_MAX_GAME_NAME_LENGTH         50

class Xash3DAboutWindow : public BeAboutWindow
{
public:
	explicit Xash3DAboutWindow(const BRect &frame, const char *title, const char *version)
	         : BeAboutWindow(frame, title, version)
	{
		BStringView *urlDescString = new BStringView(O_ABOUT_LINK_DESC, L_ABOUT_LINK_DESC);
		BeUrlStringView *urlString = new BeUrlStringView(O_ABOUT_LINK, L_ABOUT_LINK);

		BeAboutWindow::GetInformationView()->Insert(L_ABOUT_STRING);
		BeAboutWindow::GetInformationView()->SetFontAndColor(be_bold_font);
		BeAboutWindow::GetInformationView()->Insert(L_ABOUT_THANKS_STR_H);
		BeAboutWindow::GetInformationView()->SetFontAndColor(be_plain_font);
		BeAboutWindow::GetInformationView()->Insert(L_ABOUT_THANKS_STR);

		BGroupLayout *boxLayout = BLayoutBuilder::Group<>(B_HORIZONTAL)
		                          .Add(urlDescString)
		                          .Add(urlString)
		                          .AddGlue();
		BeAboutWindow::GetAdditionalBox()->AddChild(boxLayout->View());
	}
};

class Xash3DGameLauncher : public BeLauncherBase
{
	enum
	{
		MSG_CHECKBOX_STATE_CHANGED              = 'chks',
		MSG_ADDITIONAL_BROWSE_BUTTON_CLICKED    = 'abtc',
		MSG_ADDITIONAL_FILE_PANEL_FILE_SELECTED = 'afps',
		MSG_SELECTED_SOME_GAME                  = 'ssgl'
	};

	BString fSelectedGame;

	BCheckBox *fCheckBoxOption;
	BTextControl *fAdditionalTextControl;
	BButton *fAdditionalBrowseButton;
	BListView *fGamesList;
	BScrollView *fScrollView;

	BeDirectoryFilePanel *fAdditionalDirectoryFilePanel;
	BeDirectoryFilter *fAdditionalDirectoryFilter;

	void SelectSomeGame(void)
	{
		bool found = false;
		for(int i = 0; i < fGamesList->CountItems(); ++i)
		{
			BStringItem *stringItem = dynamic_cast<BStringItem *>(fGamesList->ItemAt(i));
			BString textOfStringItem = stringItem->Text();
			if(textOfStringItem.Compare(BeLauncherBase::GetSettings()->GetSettingsString(GAME_OPT)) == 0)
			{
				found = true;
				fGamesList->Select(i);
			}
		}
		if(!found)
		{
			fGamesList->Select(0);
		}
	}

	void FillGamesList(void)
	{
		fGamesList->MakeEmpty();

		entry_ref ref;
		BEntry entry(BeLauncherBase::GetTextControl()->Text());
		if(!entry.Exists() || !entry.IsDirectory())
		{
			fGamesList->AddItem(new BStringItem(L_ERROR_NO_VALVE_CATALOG));
			return;
		}
		entry.GetRef(&ref);

		BDirectory directory(&ref);
		if(directory.InitCheck() != B_OK)
		{
			fGamesList->AddItem(new BStringItem(L_ERROR_NO_VALVE_CATALOG));
			return;
		}
		directory.Rewind();
		BStringList stringList;
		while(directory.GetNextEntry(&entry, true) == B_OK)
		{
			if(entry.IsDirectory())
			{
				entry.GetRef(&ref);
				stringList.Add(BString(ref.name));
			}
		}

		if(stringList.HasString(VALVE_DIR_NAME))
		{
			for(int i = 0; i < stringList.CountStrings(); ++i)
			{
				fGamesList->AddItem(new BStringItem(stringList.StringAt(i)));
			}
		}
		else
		{
			fGamesList->AddItem(new BStringItem(L_ERROR_NO_VALVE_CATALOG));
		}
	}

	static const BString GetPathToPackageExtras(void)
	{
		BString packagePath = BeUtils::GetPathToPackage(PACKAGE_DIR);
		return packagePath << BString(EXTRAS_DIR_NAME);
	}

protected:
	virtual void
	MessageReceived(BMessage *msg)
	{
		switch (msg->what)
		{
			case MSG_CHECKBOX_STATE_CHANGED:
			{
				bool value = static_cast<bool>(fCheckBoxOption->Value());
				fAdditionalTextControl->SetEnabled(value);
				fAdditionalBrowseButton->SetEnabled(value);
				if(!value)
				{
					fAdditionalTextControl->SetText(BeLauncherBase::GetSettings()->GetSettingsString(MIRROR_PATH_OPT));
				}
				break;
			}
			case MSG_ADDITIONAL_BROWSE_BUTTON_CLICKED:
			{
				fAdditionalDirectoryFilePanel->Show();
				break;
			}
			case MSG_ADDITIONAL_FILE_PANEL_FILE_SELECTED:
			{
				BeLauncherBase::DirectorySelected(fAdditionalDirectoryFilePanel, fAdditionalTextControl);
				break;
			}
			case MSG_FILE_PANEL_FILE_SELECTED:
			{
				BeLauncherBase::MessageReceived(msg);
				FillGamesList();
				fSelectedGame = VALVE_DIR_NAME;
				BeLauncherBase::GetSettings()->SetSettingsString(GAME_OPT, fSelectedGame);
				SelectSomeGame();
				break;
			}
			case MSG_SELECTED_SOME_GAME:
			{
				int32 selection = fGamesList->CurrentSelection();
				if(selection < 0)
				{
					fSelectedGame = VALVE_DIR_NAME;
				}
				BStringItem *stringItem = dynamic_cast<BStringItem *>(fGamesList->ItemAt(selection));
				if (stringItem != NULL)
				{
					fSelectedGame = stringItem->Text();
				}
				break;
			}
			default:
			{
				BeLauncherBase::MessageReceived(msg);
				break;
			}
		}
	}

	virtual bool
	CheckCache(void)
	{
		return true;
	}

	virtual bool
	RunGame(void)
	{
		BString mirrorPath;
		if(fCheckBoxOption->Value() == B_CONTROL_ON)
		{
			mirrorPath = fAdditionalTextControl->Text();
		}
		else
		{
			mirrorPath = GetPathToPackageExtras();
		}
		setenv(MIRROR_PATH_OPT, mirrorPath.String(), 1);

		// NOTE: Just an extra check.
		// If it does not fit the condition then this is a error, reset it.
		if((fSelectedGame.Length() > G_MAX_GAME_NAME_LENGTH) ||
		   (fSelectedGame.Compare("") == 0))
		{
			fSelectedGame = VALVE_DIR_NAME;
		}
		setenv(GAME_OPT, fSelectedGame.String(), 1);

		return BeLauncherBase::RunGameViaRoster(true);
	}

	virtual bool
	ReadSettings(void)
	{
		if(!BeLauncherBase::ReadSettings())
		{
			BeDebug("[Info]: First run, set default values.\n");
			fCheckBoxOption->SetValue(B_CONTROL_OFF);
			fAdditionalTextControl->SetEnabled(false);
			fAdditionalBrowseButton->SetEnabled(false);

			fAdditionalTextControl->SetText(GetPathToPackageExtras());
		}
		else
		{
			const char *str = BeLauncherBase::GetSettings()->GetSettingsString(S_CHECKBOX_OPTION);
			int ASCII_MAGIC = 48;
			int value = static_cast<int>(str[0] - ASCII_MAGIC);
			fCheckBoxOption->SetValue(value);
			fAdditionalTextControl->SetEnabled(static_cast<bool>(value));
			fAdditionalBrowseButton->SetEnabled(static_cast<bool>(value));

			fAdditionalTextControl->SetText(BeLauncherBase::GetSettings()->GetSettingsString(MIRROR_PATH_OPT));
		}
		return true;
	}

	virtual void
	SaveSettings(bool def)
	{
		BString value;
		value << fCheckBoxOption->Value();
		BeLauncherBase::GetSettings()->SetSettingsString(S_CHECKBOX_OPTION, value);
		BeLauncherBase::GetSettings()->SetSettingsString(MIRROR_PATH_OPT, fAdditionalTextControl->Text());
		BeLauncherBase::GetSettings()->SetSettingsString(GAME_OPT, fSelectedGame);

		BeLauncherBase::SaveSettings(def);
	}

	virtual void
	ShowAboutDialog(void)
	{
		Xash3DAboutWindow *xash3DAboutWindow = new Xash3DAboutWindow(Frame().InsetBySelf(BannerWidth(), -(Gap() * 3)),
		                                                             TITLE, VERSION);
		xash3DAboutWindow->Show();
	}

	virtual void
	FrameResized(float newWidth, float newHeight)
	{
		// NOTE: Fix some drawing glitches when resizing a window.
		fGamesList->Invalidate();
		fScrollView->Invalidate();

		BeLauncherBase::FrameResized(newWidth, newHeight);
	}

public:
	explicit Xash3DGameLauncher(const char *startPath)
	         : BeLauncherBase(TITLE, PACKAGE_DIR, EXECUTABLE_FILE, SETTINGS_FILE, DATA_PATH_OPT,
	                          startPath, true, false)
	{
		fCheckBoxOption = new BCheckBox(O_CHECKBOX_OPTION, L_CHECKBOX_OPTION, new BMessage(MSG_CHECKBOX_STATE_CHANGED));
		fCheckBoxOption->SetToolTip(L_CHECKBOX_OPTION_TOOLTIP);

		fAdditionalTextControl = new BTextControl(NULL, NULL, NULL);
		fAdditionalTextControl->SetToolTip(L_EXTRA_TEXT_CONTROL_TOOLTIP);

		fAdditionalBrowseButton = new BButton(L_EXTRA_BUTTON_BROWSE, NULL);
		fAdditionalBrowseButton->SetMessage(new BMessage(MSG_ADDITIONAL_BROWSE_BUTTON_CLICKED));
		fAdditionalBrowseButton->SetToolTip(L_EXTRA_BUTTON_BROWSE_TOOLTIP);
		fAdditionalBrowseButton->ResizeToPreferred();
		fAdditionalBrowseButton->SetExplicitSize(BSize(fAdditionalTextControl->Bounds().Height(),
		                                     fAdditionalTextControl->Bounds().Height()));

		BStringView *urlDescString = new BStringView(O_DATA_LINK_DESC, L_DATA_FILES_LINK_D);
		BeUrlStringView *urlString = new BeUrlStringView(O_DATA_LINK, L_DATA_LINK);

		BStringView *gameListLabel = new BStringView(O_GAME_LIST_LABEL, L_GAME_LIST_LABEL);

		fGamesList = new BListView();
		fGamesList->SetExplicitMinSize(BSize(B_SIZE_UNSET, G_GAMES_LIST_HEIGHT));
		fGamesList->SetSelectionMessage(new BMessage(MSG_SELECTED_SOME_GAME));
		fScrollView = new BScrollView(O_VIEW_SCROLL, fGamesList, B_WILL_DRAW | B_FRAME_EVENTS,
		                                          false, true, B_PLAIN_BORDER);

		BGroupLayout *boxLayout = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_HALF_ITEM_SPACING)
		                          .AddGroup(B_HORIZONTAL, 0.0f)
		                              .Add(fCheckBoxOption)
		                              .AddGlue()
		                          .End()
		                          .AddGroup(B_HORIZONTAL, B_USE_HALF_ITEM_SPACING)
		                              .Add(fAdditionalTextControl)
		                              .Add(fAdditionalBrowseButton)
		                          .End()
		                          .AddGroup(B_HORIZONTAL, 0.0f)
		                              .Add(gameListLabel)
		                              .AddGlue()
		                          .End()
		                          .Add(fScrollView)
		                          .AddGroup(B_HORIZONTAL, 0.0f)
		                              .Add(urlDescString)
		                              .Add(urlString)
		                              .AddGlue()
		                          .End();
		BeLauncherBase::GetAdditionalBox()->AddChild(boxLayout->View());

		fAdditionalDirectoryFilter = new BeDirectoryFilter();
		fAdditionalDirectoryFilePanel = new BeDirectoryFilePanel(new BMessenger(this),
		                                                         new BMessage(MSG_ADDITIONAL_FILE_PANEL_FILE_SELECTED),
		                                                         fAdditionalDirectoryFilter,
		                                                         BeUtils::GetPathToHomeDir());
		fAdditionalDirectoryFilePanel->Window()->SetTitle(L_ADDITIONAL_FILE_PANEL_TITLE);

		// NOTE: Be sure to call read settings function.
		ReadSettings();

		// NOTE: Fill Game List and select some Game then.
		FillGamesList();
		SelectSomeGame();
	}

	virtual
	~Xash3DGameLauncher()
	{
		delete fAdditionalDirectoryFilter;
		fAdditionalDirectoryFilter = NULL;
		delete fAdditionalDirectoryFilePanel;
		fAdditionalDirectoryFilePanel = NULL;
	}
};

int
main(void)
{
	BeApp *beApp = new BeApp(SIGNATURE);
	Xash3DGameLauncher *xash3DGameLauncher = new Xash3DGameLauncher(BeUtils::GetPathToHomeDir());
	beApp->SetMainWindow(xash3DGameLauncher );
	beApp->Run();
	delete beApp;
	beApp = NULL;
	return 0;
}
