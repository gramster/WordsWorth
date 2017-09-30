/*
 * File:	winxw.cc
 * Purpose:	XWorth for WxWindows
 * Author:	Graham Wheeler
 * Created:	1996
 * Updated:	
 * Copyright:	(c) 1996, Graham Wheeler
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#ifdef __GNUG__
#pragma implementation
#endif

#include "wx.h"
//#include "wx_timer.h"
//#include "wx_help.h"

#if !USE_PRINTING_ARCHITECTURE
#error You must set USE_PRINTING_ARCHITECTURE to 1 in wx_setup.h to compile this demo.
#endif

#include "wx_mf.h"
#include "wx_print.h"

#include "ggame.h"
#include "gpanel.h"
#include "gdialog.h"
#include "gcanvas.h"
#include "gmenu.h"
#include "gframe.h"
#include "dict.h"
#include "xword.h"
#include "condlg.h"
#include "xwdialog.h"
#include "winids.h"
#include "dicbuild.h"
#include "winxw.h"
#include "version.h"
#define MAIN
#include "register.h"

#ifdef wx_msw
#include "mmsystem.h"
#endif

#ifndef max
#define max(a, b)	( ((a)>(b)) ? (a) : (b) )
#endif
#ifndef min
#define min(a, b)	( ((a)>(b)) ? (b) : (a) )
#endif

// Initialisation file

#ifdef wx_msw
char *ini = "winxw.ini";
#endif
#ifdef wx_x
char *ini = ".xword";
#endif

//wxHelpInstance *help = 0;
XWordApp     myApp; // initialise the application

#define CANVASBACKGROUNDCOLOUR	0
#define BLACKSQUARECOLOUR	1
#define NORMALSQUARECOLOUR	2
#define CURSORCOLOUR		3
#define AUTOPLACEDCOLOUR	4
#define STARTOFWORDCOLOUR	5
#define BYPASSCOLOUR 		6

#define APPSHORTNAME 	"winxw"
#define APPNAME		"XWord for Windows" 

static char aboutXW[200];

//---------------------------------------------------------------

Dictionary *dict = 0;

wxMenu *XWordMenuBar::MakeOptionsMenu()
{
    wxMenu *options_menu = GameMenu::MakeOptionsMenu();
    options_menu->Append(WXWW_OPTIONS, "&Configure Options", "Set the default options");
    return options_menu;
}

void XWordMenuBar::MakeMiddleMenus()
{
    solve_menu = new wxMenu;
    solve_menu->Append(WXWW_THINK, "&Enable Constraints",   "Compute square constraints each time cursor moves");
    solve_menu->AppendSeparator();
    solve_menu->Append(WXWW_CRUNCH, "&Compute Constraints",   "Try to solve crossword");
    solve_menu->Append(WXWW_FLUSH, "Discard Constraints",   "Discard current (partial) solution");
    solve_menu->AppendSeparator();
    solve_menu->Append(WXWW_AVIEW, "View &Across",   "Show all legitimate horizontal words for this square");
    solve_menu->Append(WXWW_DVIEW,"&View Down",   "Show all legitimate vertical words for this square");
    Append(solve_menu, "&Solver");
    tool_menu = new wxMenu;
    tool_menu->Append(WXWW_CONSULT, "&Consult",   "Dictionary consultations");
    tool_menu->AppendSeparator();
    tool_menu->Append(WXWW_DUMPDICT, "&Dump Dictionary",   "Dump the words in the current dictionary to a file");
    tool_menu->AppendSeparator();
    tool_menu->Append(WXWW_BUILDDICT, "&Build Dictionary",   "Build a dictionary from a list of words in a file");
    tool_menu->Append(WXWW_INTERRUPT, "&Interrupt Build",   "Interrupt a dictionary build");
    tool_menu->Append(WXWW_RESUME, "&Resume Build",   "Resume an interrupted dictionary build");
    if (!IsRegistered())
    {
	tool_menu->AppendSeparator();
	tool_menu->Append(WXWW_REGISTER, "Register",   "Register your copy");
    }
    Append(tool_menu,  "&Tools");
    grid_menu = new wxMenu;
    grid_menu->Append(WXWW_BOARDFLIP, "&Flip Grid",   "Exchange top left with top right");
    grid_menu->Append(WXWW_BOARDHREF, "Reflect &Horizontally",   "Reflect top half to bottom half (inverted)");
    grid_menu->Append(WXWW_BOARDVREF, "Reflect &Vertically",   "Copy left half to right half");
    grid_menu->Append(WXWW_SYMMETRY, "Set Grid Symmetry",   "Set the grid symmetry type");
    grid_menu->Append(WXWW_TOGGLELOCK, "Lock Grid",   "Toggle flip/reflect menu options on/off");
    grid_menu->Append(WXWW_TOGGLEMOVE, "Disable Automove",   "Toggle automoving on/off");
    Append(grid_menu, "&Grid");
}

XWordMenuBar::XWordMenuBar(GameCanvas *canvas_in)
    : GameMenu(APPSHORTNAME, APPNAME, aboutXW, ini,  canvas_in, 2),
      grid_menu(0), solve_menu(0), tool_menu(0)
{
}

void XWordMenuBar::Lock()
{
    grid_menu->Enable(WXWW_BOARDFLIP, 0);
    grid_menu->Enable(WXWW_BOARDHREF, 0);
    grid_menu->Enable(WXWW_BOARDVREF, 0);
    grid_menu->Enable(WXWW_SYMMETRY, 0);
    grid_menu->SetLabel(WXWW_TOGGLELOCK, "Unlock Grid");
}

void XWordMenuBar::Unlock()
{
    grid_menu->Enable(WXWW_BOARDFLIP, 1);
    grid_menu->Enable(WXWW_BOARDHREF, 1);
    grid_menu->Enable(WXWW_BOARDVREF, 1);
    grid_menu->Enable(WXWW_SYMMETRY, 1);
    grid_menu->SetLabel(WXWW_TOGGLELOCK, "Lock Grid");
}

void XWordMenuBar::DisableGameSpecific()
{
    Lock();
    solve_menu->Enable(WXWW_THINK, 0);
    solve_menu->Enable(WXWW_CRUNCH, 0);
    solve_menu->Enable(WXWW_FLUSH, 0);
    solve_menu->Enable(WXWW_AVIEW, 0);
    solve_menu->Enable(WXWW_DVIEW, 0);
    tool_menu->Enable(WXWW_INTERRUPT, 0);
    if (!IsRegistered())
    {
        tool_menu->Enable(WXWW_BUILDDICT, 0);
        tool_menu->Enable(WXWW_RESUME, 0);
    }
    grid_menu->Enable(WXWW_TOGGLELOCK, 0);
}

void XWordMenuBar::EnableGameSpecific()
{
    Unlock();
    grid_menu->Enable(WXWW_TOGGLELOCK, 1);
    solve_menu->Enable(WXWW_THINK, 1);
    solve_menu->Enable(WXWW_CRUNCH, 1);
    solve_menu->Enable(WXWW_FLUSH, 1);
    solve_menu->Enable(WXWW_AVIEW, 1);
    solve_menu->Enable(WXWW_DVIEW, 1);
}

void XWordMenuBar::SetCrunch(int v)
{
    solve_menu->SetLabel(WXWW_THINK, (v ? "&Disable Constraints" :
					  "&Enable Constraints"));
}

int XWordMenuBar::Handle(int id)
{
    int rtn = 0;
    CrossWord *xword = (CrossWord*)game;
    switch(id)
    {
    case WXWW_THINK:
	if (xword)
	{
	    int n = 1 - xword->GetCrunch();
	    SetCrunch(n);
	    xword->SetCrunch(n);
	}
	break;
    case WXWW_CRUNCH:
	if (xword)
	{
	    ::wxBeginBusyCursor();
	    xword->ComputeGlobalChecks();
	    ::wxEndBusyCursor();
	}
	break;
    case WXWW_FLUSH:		
	if (xword) xword->ClearChecks(1, 0);
	break;
    case WXWW_AVIEW:
    case WXWW_DVIEW:		
	if (xword) 
	{
	    char *tn = tmpnam(0);
	    xword->SetLogFile(tn);
	    (void)xword->ComputeLocalCheck(xword->Row(), xword->Col(),
		((id == WXWW_DVIEW) ? ANYD :  ANYA));
	    xword->SetLogFile(0);
	    char *lbl = ((id==WXWW_DVIEW) ? "Down Words" : "Across Words");
	    FileBrowser *viewer = new FileBrowser(lbl, tn);
	    if (viewer) viewer->Show(TRUE);
	    else unlink(tn);
	}
	break;
    case WXWW_BOARDFLIP:	
	if (xword) xword->Flip();
	break;
    case WXWW_BOARDHREF:	
	if (xword) xword->ReflectHorizontal();
	break;
    case WXWW_BOARDVREF:
	if (xword) xword->ReflectVertical();
	break;
    case WXWW_SYMMETRY:
	if (xword) 
	{
	    int sym = xword->GetSymmetry();
	    RunDialog(new SymmetryDlg(sym));
	    xword->SetSymmetry(sym);
	}
	break;
    case WXWW_TOGGLELOCK:	
	((XWordFrame*)menu_bar_frame)->ToggleLock();
	break;
    case WXWW_TOGGLEMOVE:	
	if (xword) xword->ToggleAutoSkip();
	break;
    case WXWW_OPTIONS:
	RunDialog(new OptionsDlg());
	break;
    case WXWW_CONSULT:
	RunDialog(new ConsultDlg(ini));
	break;
    case WXWW_REGISTER:
	RunDialog(new RegisterDlg(ini));
	break;
    case WXWW_DUMPDICT:
	((XWordFrame*)menu_bar_frame)->DumpDict();
	break;
    case WXWW_BUILDDICT:
	((XWordFrame*)menu_bar_frame)->BuildDict(0);
	break;
    case WXWW_INTERRUPT:
	((XWordFrame*)menu_bar_frame)->InterruptBuild();
	break;
    case WXWW_RESUME:
	((XWordFrame*)menu_bar_frame)->BuildDict(1);
	break;
    default:
        if (dict || (id != GM_NEW_GAME && id != GM_LOAD_GAME))
	    return GameMenu::Handle(id);
        else 
	    menu_bar_frame->SetStatusText("No dictionary");
	return 0;
    }
    ((XWordFrame*)menu_bar_frame)->UpdateDisplay(1);
    return rtn;
}

//------------------------------------------------------------------------
// The right-hand panel 

XWordPanel::XWordPanel(wxFrame *f, int left, int top, int width, int height)
    : GamePanel(f, left, top, width, height, 0, 18)
{
    Show(TRUE);
}

void XWordPanel::SetDownCount(long cnt)
{
    char buf[20];
    sprintf(buf, (cnt>=0 ? "%5ld DOWN" : "          "), cnt);
    SetLabel(9, buf);
}

void XWordPanel::SetAcrossCount(long cnt)
{
    char buf[20];
    sprintf(buf,(cnt>=0 ? "%5ld ACROSS" : "            "), cnt);
    SetLabel(0, buf);
}

void XWordPanel::SetAcrossLabel(int n, char *txt)
{
    SetLabel(1+n, txt);
}

void XWordPanel::SetDownLabel(int n, char *txt)
{
    SetLabel(10+n, txt);
}

void XWordPanel::Clear()
{
    SetDownCount(-1l);
    SetAcrossCount(-1l);
    for (int i = 0; i < 8; i++)
    {
	SetAcrossLabel(i, " ");
	SetDownLabel(i, " ");
    }
}

XWordPanel::~XWordPanel()
{
}

//---------------------------------------------------------------

XWordCanvas::XWordCanvas(wxFrame *frame, int x, int y, int w, int h)
    : GameCanvas(frame, x, y, w, h, 7, 0)
{
    SetResource(ini, CANVASBACKGROUNDCOLOUR,"Background",	"CanvasBackground", C_WHITE	);
    SetResource(ini, BLACKSQUARECOLOUR,     "Unplayable Square","BlackSquare",	    C_BLACK	);
    SetResource(ini, NORMALSQUARECOLOUR,    "Normal Square",	"NormalSquare",     C_WHITE	);
    SetResource(ini, CURSORCOLOUR,	    "Cursor",		"Cursor",           C_GREEN	);
    SetResource(ini, AUTOPLACEDCOLOUR,	    "Auto-placed Letter","Autoplaced",      C_RED	);
    SetResource(ini, STARTOFWORDCOLOUR,	    "Word Start Marks",  "WordStart",       C_RED	);
    SetResource(ini, BYPASSCOLOUR,	    "Dictionary Bypass Marks",  "Bypass",   C_BLACK	);
}

XWordCanvas::~XWordCanvas()
{
}

void XWordCanvas::SetGame(Game *game_in)
{
    GameCanvas::SetGame(game_in);
    if (game_in)
    {
	SetBoardDimensions(game_in->Height(), game_in->Width(), 1);
	ClearSurroundingArea();
    }
}

void XWordCanvas::Select(int r, int c)
{
    CrossWord *xword = (CrossWord*)game;
    DrawSquare(xword->Row(), xword->Col());
    xword->SetRow(r);
    xword->SetCol(c);
    DrawCursor(r, c);
    ((XWordFrame*)window_parent)->ShowChoices();	
}

void XWordCanvas::HandleLeftMouseClickOnBoard(int r, int c)
{
    CrossWord *xword = (CrossWord*)game;
    if (xword == 0) return;
    if (xword->Row() != r || xword->Col() != c)
	Select(r, c);
    else
    {
        int down = xword->IsStartOfDownWord(r, c);
        int across = xword->IsStartOfAcrossWord(r, c);
        if (down || across)
        {
	    RunDialog(new WordFlagDlg(xword, r, c));
	    xword->ClearChecks(1);
	    DrawBoard();
        }
    }
}

void XWordCanvas::HandleRightMouseClickOnBoard(int r, int c)
{
    CrossWord *xword = (CrossWord*)game;
    if (xword == 0) return;
    if (xword->Row() != r || xword->Col() != c)
	Select(r, c);
    else
    {
	int down = xword->IsInDownWord(r, c);
	int across = xword->IsInAcrossWord(r, c);
        if (down || across)
        {
	    RunDialog(new SquareFlagDlg(xword, r, c));
	    xword->ClearChecks(1);
	    DrawBoard();
        }
    }
}

void XWordCanvas::HandleLeftMouseClickOffBoard(int x, int y)
{
    (void)x; (void)y;
    if (game) ((XWordFrame*)window_parent)->Crunch();
}

void XWordCanvas::HandleKey(int ch)
{
    CrossWord *xword = (CrossWord*)game;
    if (xword==0) return;
    DrawSquare(xword->Row(), xword->Col());
    switch(ch)
    {
    case WXK_HOME:	xword->Home();		break;
    case WXK_END:	xword->End();		break;
    case WXK_PRIOR:	xword->Top();		break;
    case WXK_NEXT:	xword->Bottom();	break;
    case WXK_DOWN:	xword->Down();		break;
    case WXK_UP:	xword->Up();		break;
    case WXK_LEFT:	xword->Left();		break;
    case WXK_RIGHT:	xword->Right();		break;
    default:
	xword->Insert(xword->Row(), xword->Col(), ch);
        DrawSquare(xword->Row(), xword->Col());
	switch(xword->GetSymmetry())
	{
	case 1:
	    DrawSquare(xword->Row(), xword->Width()-xword->Col()-1);
	    break;
	case 2:
	    DrawSquare(xword->Height()-xword->Row()-1, xword->Col());
	    break;
	case 3:
	    DrawSquare(xword->Row(), xword->Width()-xword->Col()-1);
	    DrawSquare(xword->Height()-xword->Row()-1, xword->Col());
	    DrawSquare(xword->Height()-xword->Row()-1, xword->Width()-xword->Col()-1);
	    break;
	case 4:
	    DrawSquare(xword->Height()-xword->Row()-1, xword->Width()-xword->Col()-1);
	    break;
	}
	xword->AdvanceCursor();
        break;
    }
    ((XWordFrame*)window_parent)->ShowChoices();	
    DrawCursor(xword->Row(), xword->Col());
}

void XWordCanvas::DrawCursor(int r, int c)
{
    CrossWord *xword = (CrossWord*)game;
    int x = (c+1)*squarewidth+squarewidth/2;
    int y = (r+1)*squareheight+squareheight/2;
    int backg = xword->IsBlack(r, c) ? BLACKSQUARECOLOUR : NORMALSQUARECOLOUR;
    SelectResourcePen(CURSORCOLOUR); DrawBox(x, y, squareheight/2-1);
    SelectResourcePen(backg);	     DrawBox(x, y, squareheight/2-2);
    SelectResourcePen(CURSORCOLOUR); DrawBox(x, y, squareheight/2-3);
//    SelectPen(C_BLACK);
}

void XWordCanvas::DrawSquare(int r, int c)
{
    CrossWord *xword = (CrossWord*)game;
    int w = squarewidth;
    int h = squareheight;
    SelectPen(C_BLACK);
    if (xword->IsBlack(r,c))
    {
	SetColour(BLACKSQUARECOLOUR);
	DrawBoardSquare(r, c, 1);
    }
    else
    {
	SetColour(NORMALSQUARECOLOUR);
	DrawBoardSquare(r, c, 1);
	SelectResourcePen(STARTOFWORDCOLOUR);
        if (xword->IsStartOfAcrossWord(r,c) && !xword->IsLeftmostLetter(r,c))
	{
  	    dc->DrawLine((float)((c+1)*w+1),(float)((r+1)*h),
			(float)((c+1)*w+1),(float)((r+2)*h-1));
  	    dc->DrawLine((float)((c+1)*w+2),(float)((r+1)*h),
			(float)((c+1)*w+2),(float)((r+2)*h-1));
	}
        if (xword->IsStartOfDownWord(r,c) && !xword->IsTopmostLetter(r,c))
	{
  	    dc->DrawLine((float)((c+1)*w),(float)((r+1)*h+1),
			(float)((c+2)*w-1),(float)((r+1)*h+1));
  	    dc->DrawLine((float)((c+1)*w),(float)((r+1)*h+2),
			(float)((c+2)*w-1),(float)((r+1)*h+2));
	}
	SelectResourcePen(BYPASSCOLOUR);
        if (xword->IsEmpty(r,c))
	{
	    if (xword->BypassDictionaryAcross(r,c)) // mark with horizontal line
  	        dc->DrawLine((float)((c+1)*w+w/4),(float)((r+1)*h+h/2),
			    (float)((c+2)*w-w/4),(float)((r+1)*h+h/2));
	    if (xword->BypassDictionaryDown(r,c)) // mark with vertical line
  	        dc->DrawLine((float)((c+1)*w+w/2),(float)((r+1)*h+h/4),
			    (float)((c+1)*w+w/2),(float)((r+2)*h-h/4));
	}
	else
	{
	    SetColour(0);
            if (xword->AutoPlaced(r, c))
		SelectResourcePen(AUTOPLACEDCOLOUR);
	    DrawCenteredLetter((c+1)*w+w/2, (r+1)*h+h/2, w*3/4, xword->Letter(r,c));
	}
    }
}

void XWordCanvas::DrawBoard()
{
    CrossWord *xword = (CrossWord*)game;
    SelectPen(C_BLACK);
    if (xword==0 || xword->Row()<0) return;
    GameCanvas::DrawBoard();
    DrawCursor(xword->Row(), xword->Col());
}

//------------------------------------------------------------------
// Menu Commands

void XWordFrame::GetGridSize(int &rows, int &cols)
{
    rows = cols = 15;
    (void)wxGetResource("Grid", "Rows", &rows, ini);
    (void)wxGetResource("Grid", "Cols", &cols, ini);
    RunDialog(new GridSizeDlg(rows, cols));
}

void XWordFrame::UpdateDisplay(int clear)
{
    if (clear) canvas->Clear();
    if (game) { ShowChoices(); canvas->DrawBoard(); }
}

int XWordFrame::AllocateGameForReading(FILE *fp)	
{
    game = new CrossWord(dict);
    return game ? 0 : -1;
}

void XWordFrame::AllocateNewGame()	
{
    int rows, cols;
    GetGridSize(rows, cols);
    game = new CrossWord(cols, rows, dict);
    int sym;
    (void)wxGetResource("Grid", "Symmetry", &sym, ini);
    ((CrossWord*)game)->SetSymmetry(sym);
    if (lock) ToggleLock();
}

void XWordFrame::LoadGame()	
{
    GameFrame::LoadGame();
    ((XWordMenuBar*)menubar)->SetCrunch(0);
    if (lock) ToggleLock();
}

void XWordFrame::Crunch()
{
    ::wxBeginBusyCursor();
    ((CrossWord*)game)->ComputeGlobalChecks();
    ::wxEndBusyCursor();
    UpdateDisplay(0);
    //ClearWords();
}

//=====================================================================

void XWordFrame::ToggleLock()
{
    lock = 1- lock;
    if (lock)
    {
        ((XWordMenuBar*)menubar)->Lock();    
        ((CrossWord*)game)->Lock();
    }
    else
    {
        ((XWordMenuBar*)menubar)->Unlock();    
        ((CrossWord*)game)->Unlock();
    }
}

//-----------------------------------------------------------------------

void XWordFrame::InterruptBuild()
{
    delete dicbuilder;
    dicbuilder = 0;
}

void XWordFrame::BuildDict(int restore)
{
    char suf[10];
    strcpy(suf, "*.txt");
    char *w = wxFileSelector("Word List", 0, 0, 0, suf);
    if (w)
    {
        FILE *lex = fopen(w, "r");
	if (lex)
	{
	    strcpy(suf, "*.dic");
	    char fname[256];
	    // copy filename in without path
	    if (strrchr(w, '\\') == 0)
		strcpy(fname, w);
	    else	
		strcpy(fname, strrchr(w, '\\')+1);
	    // add .dic extension, removing existing extension if any
	    if (strlen(fname)>4 && strchr(fname+strlen(fname)-4, '.') != 0)
		strcpy(strchr(fname+strlen(fname)-4, '.'), ".dic");
	    else
	    	strcat(fname, ".dic");
	    char *o = wxFileSelector("Output File", 0, fname, 0, suf);
	    FILE *dawg = fopen(o, "wb");
	    if (dawg != 0)
	    {
	        delete dicbuilder;
	        dicbuilder = new DictionaryBuilder(this, lex, dawg, restore);
	    }
	    else 
	    {
	    	fclose(lex);
	        wxMessageBox("Failed to open output file!",
            	        "Open failed", wxOK|wxCENTRE);
	    }
	}
	else (void)wxMessageBox("Couldn't open word list", "Load failed", wxOK|wxCENTRE);
    }
}

//-------------------------------------------------------------------------

void XWordFrame::DumpDict()
{
    char suf[10];
    strcpy(suf, "*.dic");
    char *s = wxFileSelector("Dictionary File", 0, 0, 0, suf);
    if (s)
    {
        Dictionary *dict= 0;
	char *err = MakeDictionary(dict, s);
	if (err == 0)
	{
	    strcpy(suf, "*.txt");
	    char fname[256];
	    // copy filename in without path
	    if (strrchr(s, '\\') == 0)
		strcpy(fname, s);
	    else	
		strcpy(fname, strrchr(s, '\\')+1);
	    // add .txt extension, removing existing extension if any
	    if (strlen(fname)>4 && strchr(fname+strlen(fname)-4, '.') != 0)
		strcpy(strchr(fname+strlen(fname)-4, '.'), ".txt");
	    else
	    	strcat(fname, ".txt");
	    char *o = wxFileSelector("Output File", 0, fname, 0, suf);
	    FILE *ofp = fopen(o, "w");
	    if (ofp != 0)
	    {
	        dict->Dump(ofp);
		fclose(ofp);
		wxMessageBox("Dictionary Dumped!", "Done", wxOK|wxCENTRE);
	    }
	    else wxMessageBox("Failed to open output file!",
            	        "Open failed", wxOK|wxCENTRE);
	}
	else (void)wxMessageBox(err, "Load failed", wxOK|wxCENTRE);
	delete dict;
    }
}

//-----------------------------------------------------------------------

GamePanel *XWordFrame::MakePanel(int x, int y, int w, int h)
{
    return new XWordPanel(this, x, y, w, h);
}

GameCanvas *XWordFrame::MakeCanvas(int x, int y, int w, int h)
{
    return new XWordCanvas(this, x, y, w, h);
}

GameMenu *XWordFrame::MakeMenuBar()
{
    return new XWordMenuBar(canvas);
}

XWordFrame::XWordFrame()
    : GameFrame("XWord for Windows", 650, 600, "winxw", "*.grd"),
      lock(0), dicbuilder(0)
{
}

void XWordFrame::ShowChoices()
{
    CrossWord *xword = (CrossWord*)game;
    panel->Clear();
    if (xword->GetCrunch() < 1 || !xword->IsEmpty(xword->Row(), xword->Col()))
    {
        SetStatusText("");
	return;
    }
    int acnt, dcnt;
    char *choices = xword->GetPossibleLetters(xword->Row(), xword->Col(),
						acnt, dcnt);
    char buf[64];
    sprintf(buf, "Valid choices: %s", choices);
    SetStatusText(buf);
    for (int i = 0; i < MAX_BUF_WORDS; i++)
    {
	((XWordPanel*)panel)->SetDownLabel(i, xword->GetDownWord(i));
	((XWordPanel*)panel)->SetAcrossLabel(i, xword->GetAcrossWord(i));
    }
    ((XWordPanel*)panel)->SetAcrossCount(acnt);
    ((XWordPanel*)panel)->SetDownCount(dcnt);
}

XWordFrame::~XWordFrame()
{
    delete dicbuilder;
}

//------------------------------------------------------------------------
// The `main program' equivalent, creating the windows and returning the
// main xwframe

char *XWordApp::MakeDictionary()
{
//    dict = new Dictionary;
    char *df = new char [256];
    strcpy(df, "wwmed.dic");
    (void)wxGetResource("Misc", "Dictionary", &df, ini);
    char *msg = ::MakeDictionary(dict, df);
    delete [] df;
    return msg;
}

wxFrame *XWordApp::OnInit(void)
{
    sprintf(aboutXW, "%s v%s\nby Graham Wheeler\ngram@cdsec.com\n(c) 1995-1998\n%s\n%s\n%s",
		APPNAME, WWVERSION,
		(IsRegistered() ? "Registered to:" : "Unregistered"),
		UserName(), EMail());
    char *msg = MakeDictionary();
    if (msg) (void)wxMessageBox(msg, "Dictionary Load Error", wxOK|wxCENTRE);
    GameFrame *xwframe = new XWordFrame();
    xwframe->Init();
//    work_proc = ::BackgroundProcess;
#ifdef wx_msw
    SetPrintMode(wxPRINT_WINDOWS);
#else
    SetPrintMode(wxPRINT_POSTSCRIPT);
#endif
    return xwframe;
}

