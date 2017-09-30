// WordsWorth install - uses Al Steven's DFlat++ CUA library

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>
#include <fstream.h>
#include <sys/stat.h>
#include <io.h>
#include <errno.h>

#include "dflatpp.h"

#define	S_ISDIR( mode )		((mode) & S_IFDIR)

#define Df void (DFWindow::*)()
#define Ap void (Application::*)()

char srcPath[129];
char destPath[129];

static void showMsg(const char *msg)
{
	ErrorMessageBox msgBox(msg, "Error", BORDER | SAVESELF | SHADOW);
	desktop.speaker().Beep();
	msgBox.Execute();
}

//-------------------------------------------------------------------
// Dialog box for install

class Install : public Dialog
{
protected:
    EditBox srcBox, destBox;
    OKButton ok;
    CancelButton cancel;
    String zipname;
public:
    Install(char *zname);
    void OKFunction();
};

Install::Install(char *zname)
	: Dialog("Install", 19, 60, desktop.ApplWnd(), BORDER | SAVESELF| SHADOW),
	  srcBox("Source Path", 2, 2, 2, 40, this),
	  destBox("Destination Path", 2, 5, 2, 40, this),
	  ok(20, 10, this),
          cancel(40,10, this),
	  zipname(zname)
{
    srcBox.SetText(srcPath);
    destBox.SetText(destPath);
}

void Install::OKFunction()
{
    struct stat sb;
    char src[150], dest[150];
    strcpy(src, srcBox.GetText());
    strcpy(dest, destBox.GetText());
    src[strlen(src)-1] = '\0'; // strip \n
    dest[strlen(dest)-1] = '\0'; // strip \n

    // If the directory exists, get confirmation

    if (access(dest,0)==0)
    {
	if (stat(dest,&sb)!=0 || S_ISDIR(sb.st_mode)==0)
	{
		// Not an accessible directory
		showMsg("Invalid destination directory!");
    		destBox.SetFocus();
		return;
	}
    }
    else
    {
    	// the destination doesn't exist. Try make a directory
	// I think we need to split the path, change to what exists,
	// and then try the make (i.e. mkdir() is stupid).
	if (mkdir(dest)!=0)
	{
		char *p = strerror(errno);
		showMsg(p);
		showMsg("Couldn't make destination directory!");
    		destBox.SetFocus();
		return;
	}
    }

    // check for wwship.zip or wwdem.zip on source

    char tmpsrc[150];
    strcpy(tmpsrc, src);
    strcat(tmpsrc, "\\");
    strcat(tmpsrc, zipname);
    if (access(tmpsrc,0)!=0)
    {
	char buff[150];
	sprintf(buff,"Can't open %s", tmpsrc);
	showMsg(buff);
	return;
    }
    // if all OK, start install
    if (dest[1]==':')
	_chdrive(dest[0]>='a' ? (dest[0]-'a'+1) : (dest[0]-'A'+1));
    chdir(dest);
    char cmd[256];
    sprintf(cmd, "%s\\unzip -qq -o %s", src, tmpsrc);
    system(cmd);
    Dialog::OKFunction();
}

//---------------------------------------------------------------------
//  WWInst top dialog definition

class WWInst : public Dialog
{
	void _CmInstall(char *zipname)
	{
		Install i(zipname);
		i.Execute();
	}
	PushButton instBtn, exitBtn, demoBtn;
public:
	WWInst();
	void CmInstall()	{ _CmInstall("wwship.zip");	}
	void CmInstDemo()	{ _CmInstall("wwdem.zip");	}
	void CmExit()   	{ CloseWindow(); 		}
};

WWInst::WWInst()
	: Dialog("WordsWorth Install by Graham Wheeler (c) 1994",
		25, 80, desktop.ApplWnd(), BORDER | SAVESELF| SHADOW)
	, instBtn("Install ~Program", 10, 3, this)
	, demoBtn("Install ~Demo", 10, 5, this)
	, exitBtn("~Quit", 10, 9, this)
{
    exitBtn.SetButtonFunction(this, (Df) &WWInst::CmExit);
    instBtn.SetButtonFunction(this, (Df) &WWInst::CmInstall);
    demoBtn.SetButtonFunction(this, (Df) &WWInst::CmInstDemo);
    Show();
}

// -----------------------------------------------------------------
// WWInstall application definition

class WWInstall : public Application
{
	WWInst	chat;
public:
	WWInstall()
		: Application("", BORDER | SAVESELF)
		, chat()
	{
		SetClearChar(' ');
		chat.Execute();
		CloseWindow();
	}
};

//-------------------------------------------------------------------


void main(int argc, char *argv[])
{
	if (argv[0][1]==':')
	{
		strcpy(srcPath,argv[0]);
		int i = strlen(srcPath)-1;
		while (srcPath[i] != '\\') i--;
		srcPath[i] = '\0';
	}
	else _getdcwd(0, srcPath, 129);
	if (argc==2)
		strcpy(destPath, argv[1]);
	else strcpy(destPath, "C:\\WW");
	WWInstall ma;
	ma.Execute();
}


