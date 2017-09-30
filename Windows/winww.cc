/*
 * File:	winww.cc
 * Purpose:	WordsWorth for WxWindows
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
#include "wx_timer.h"
#include "wx_bbar.h"

#if !USE_PRINTING_ARCHITECTURE
#error You must set USE_PRINTING_ARCHITECTURE to 1 in wx_setup.h to compile this demo.
#endif

#include "wx_mf.h"
#include "wx_print.h"

#include "ggame.h"
#include "gmenu.h"
#include "gpanel.h"
#include "gtoolb.h"
#include "gdialog.h"
#include "gcanvas.h"
#include "gframe.h"
#include "dict.h"
#include "pool.h"
#include "winids.h"
#include "condlg.h"
#include "wwdialog.h"
#include "wwedit.h"
#include "wwcanvas.h"
#include "dicbuild.h"
#include "winww.h"
#include "version.h"
#define MAIN
#include "register.h"

#ifdef wx_msw
#include "mmsystem.h"
#endif

#define APPSHORTNAME 	"winww"
#define APPNAME		"WordsWorth for Windows" 

static char aboutWW[200];

#ifdef wx_x
char *ini = ".winww";
#else
char *ini = "winww.ini";
#endif

#ifndef max
#define max(a, b)	( ((a)>(b)) ? (a) : (b) )
#endif
#ifndef min
#define min(a, b)	( ((a)>(b)) ? (b) : (a) )
#endif

int scrabfreq[] = {
    9, 2, 2, 4, 12, 2, 3, 2, 9, 1, 1, 4, 2, 6,
    8, 2, 1, 6, 4, 6, 4, 2, 2, 1, 2, 1, 2
};

int scrabscore[] = {
    1, 3, 3, 2, 1, 4, 2, 4, 1, 8, 5, 1, 3, 1,
    1, 3, 10, 1, 1, 1, 1, 4, 4, 8, 4, 10, 0
};

static char *scrabboard[] = {
	"CaabaaaCaaabaaC",
	"aBaaacaaacaaaBa",
	"aaBaaababaaaBaa",
	"baaBaaabaaaBaab",
	"aaaaBaaaaaBaaaa",
	"acaaacaaacaaaca",
	"aabaaababaaabaa",
	"CaabaaaBaaabaaC",
	"aabaaababaaabaa",
	"acaaacaaacaaaca",
	"aaaaBaaaaaBaaaa",
	"baaBaaabaaaBaab",
	"aaBaaababaaaBaa",
	"aBaaacaaacaaaBa",
	"CaabaaaCaaabaaC"
};

WWApp     myApp; // initialise the application

Dictionary *dict = 0;

//------------------------------------------------------------------------

WinWWGame::WinWWGame(WWFrame *frame_in, int gnum_in, int cheating_in,
	int numplayers_in, int rows, int cols, int frow, int fcol, int fdir,
	int numtiles_in, int bonus, int bonustiles, Dictionary *dict,
	int *poolfreq, int *letterscores, char **boardscores,
	int freeform, int hotsquares, int picktiles)
    : WWGame(numplayers_in, rows, cols, frow, fcol, fdir,
		numtiles_in, bonus, bonustiles, dict,
		poolfreq, letterscores, boardscores,
		freeform, hotsquares, picktiles),
	frame(frame_in), gnum(gnum_in), cheating(cheating_in)
{
}

// Hooks from game to user interface

void WinWWGame::Paint(int nextplayer, int slowly)
{
    frame->PaintCallback(slowly);
}

int WinWWGame::Override(char *msg)
{
    char buf[64];
    sprintf(buf, "%s - accept anyway?", msg);
    frame->ShowErrorLocation(msg);
    int rtn = (wxMessageBox(buf, "Override", wxYES_NO|wxCANCEL, frame) == wxYES);
    frame->HideErrorLocation(msg);
    return rtn;
}

char *WinWWGame::GetTileSelection(char *errmsg)
{
    static char buf[64];
    if (errmsg) // last attempt failed
        (void)wxMessageBox(errmsg, "You can't do that!", wxOK|wxCENTRE);
    RunDialog(new ShowPoolDlg(((WinWWGame*)frame->GetGame()), buf));
    return buf;
}

WinWWGame::~WinWWGame()
{
}

//------------------------------------------------------------------------
// Experimental toolbar

#ifdef HAS_TOOLBAR

class WWToolbar : public GameToolbar
{
  public:
    enum ToolSet { Pass, Done, Undo, Redo, Auto, Next, Run, Stop };

    WWToolbar(GameFrame *f)
      : GameToolbar(f)
    { }
    virtual void Init()
    {
#if 1
        AddToolFromFile(Pass, "bitmaps\\pass.bmp", "Pass", "Passmove/Start discard");
        AddToolFromFile(Done, "bitmaps\\done.bmp", "Done", "Finish turn");
	AddSeparator();
        AddToolFromFile(Undo, "bitmaps\\undo.bmp", "Undo", "Undo last move");
        AddToolFromFile(Redo, "bitmaps\\redo.bmp", "Redo", "Redo last undone move (if possible)");
	AddSeparator();
        AddToolFromFile(Auto, "bitmaps\\auto.bmp", "Auto", "Let computer play this move");
        AddToolFromFile(Next, "bitmaps\\next.bmp", "Next", "Make computer player change move");
        AddToolFromFile(Run, "bitmaps\\run.bmp", "Run", "Let computer play all moves for this player");
        AddToolFromFile(Stop, "bitmaps\\stop.bmp", "Stop", "Stop computer player or consultation");
#else
        AddToolFromResource(Pass, "pass_icon", "Pass", "Pass move/Start discard");
        AddToolFromResource(Done, "done_icon", "Done", "Finish turn");
	AddSeparator();
        AddToolFromResource(Undo, "undo_icon", "Undo", "Undo last move");
        AddToolFromResource(Redo, "redo_icon", "Redo", "Redo last undone move (if possible)");
	AddSeparator();
        AddToolFromResource(Auto, "auto_icon", "Auto", "Let computer play this move");
        AddToolFromResource(Next, "next_icon", "Next", "Make computer player change move");
        AddToolFromResource(Run, "run_icon", "Run", "Let computer play all moves for this player");
        AddToolFromResource(Stop, "stop_icon", "Stop", "Stop computer player or consultation");
#endif
	GameToolbar::Init();
    }
    virtual ~WWToolbar()
    {}
};

#endif

//------------------------------------------------------------------------
// The various menu bars for the different modes

wxMenu *GameMenuBar::MakeOptionsMenu()
{
    wxMenu *options_menu = GameMenu::MakeOptionsMenu();
    options_menu->Append(WXWW_MISC,"&Game Defaults",      "Dictionary, tile display options, etc");
    options_menu->Append(WXWW_BOARD,"&Board Layouts",      "Configure board size and layout");
    options_menu->Append(WXWW_POOL_FREQ, "&Letter Pool Counts", "Set number of letters in pool");
    options_menu->Append(WXWW_POOL_SCORE, "&Letter Pool Scores",       "Set letter scores");
    options_menu->Append(WXWW_POOL_WEIGHT, "&Letter Pool Weights",       "Set autoplay letter weights");
    options_menu->Append(WXWW_LEVEL, "&Auto Player Levels",      "Change other autoplay move selection controls");
    options_menu->Append(WXWW_GAME, "&Games",       "Change the game rule sets");
    return options_menu;
}

void GameMenuBar::MakeMiddleMenus()
{
    tool_menu = new wxMenu;
    tool_menu->Append(WXWW_SHOWPOOL,"&Show Pool", "Show current pool contents");
    tool_menu->AppendSeparator();
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
}

void GameMenuBar::DisableGameSpecific()
{
    if (!IsRegistered())
    {
        tool_menu->Enable(WXWW_BUILDDICT, 0);
        tool_menu->Enable(WXWW_RESUME, 0);
    }
    tool_menu->Enable(WXWW_INTERRUPT, 0);
}

void GameMenuBar::ConfigureMisc()
{
    RunDialog(new CfgMiscDlg());
    int showown;
    (void)wxGetResource("Misc", "ShowOwners", &showown, ini);
    ((WWCanvas*)canvas)->ShowOwners(showown);
}

void GameMenuBar::ConfigurePoolSize(int gnum)
{
    int sn = 1;
    char sec[16];
    sprintf(sec, "Game%d", gnum);
    (void)wxGetResource(sec, "Pool", &sn, ini);
    extern int scrabfreq[];
    RunDialog(new CfgPoolDlg("Configure Pool Size", "Pool", sn, "Number",
			0, 99, scrabfreq));
}

void GameMenuBar::ConfigureLetterScores(int gnum)
{
    char sec[16];
    int sn = 1;
    sprintf(sec, "Game%d", gnum);
    (void)wxGetResource(sec, "Pool", &sn, ini);
    extern int scrabscore[];
    RunDialog(new CfgPoolDlg("Configure Letter Scores", "Pool", sn,
			"Score", 0, 50, scrabscore));
}

void GameMenuBar::ConfigureLetterWeights(int gnum)
{
    char sec[16], res[16];
    int sn = 1;
    sprintf(sec, "Game%d", gnum);
    sprintf(res, "Strategy%d", game ? ((WinWWGame*)game)->GetPlayerNum() : 0);
    (void)wxGetResource(sec, res, &sn, ini);
    RunDialog(new CfgPoolDlg("Configure Letter Weights", "Strategy", sn,
			"Weight", -10, 10, 0));
    if (game)
	for (int i = 0; i < ((WinWWGame*)game)->GetNumPlayers(); i++)
	    ((WWFrame*)menu_bar_frame)->SetPlayerWeights(i); // in case weights have changed
}

int GameMenuBar::Handle(int id)
{
    int	gnum = game ? ((WinWWGame*)game)->GameNum() : 0;
    switch(id)
    {
    case WXWW_MISC:		
	ConfigureMisc();	
	break;
    case WXWW_POOL_FREQ:	
	ConfigurePoolSize(gnum);	
	break;
    case WXWW_POOL_SCORE:	
	ConfigureLetterScores(gnum);
	break;
    case WXWW_GAME:
	RunDialog(new CfgGameDlg(gnum));
	break;
    case WXWW_POOL_WEIGHT:
	ConfigureLetterWeights(gnum); 
	break;
    case WXWW_LEVEL:		
	RunDialog(new CfgStrategyDlg(((WWFrame*)menu_bar_frame),
				     (game ? ((WinWWGame*)game)->GetPlayerNum() : 0)));
	break;
    case WXWW_SHOWPOOL:
	if (game) RunDialog(new ShowPoolDlg(((WinWWGame*)game)));
	break;
    case WXWW_CONSULT:
	if (game==0 || ((WinWWGame*)game)->Cheating())
	    RunDialog(new ConsultDlg(ini, menu_bar_frame));
	break;
    case WXWW_BOARD:
	((WWFrame*)menu_bar_frame)->EnterBoardEditMode();
	break;
    case WXWW_REGISTER:
	RunDialog(new RegisterDlg(ini));
	break;
    case WXWW_DUMPDICT:
	((WWFrame*)menu_bar_frame)->DumpDict();
	break;
    case WXWW_BUILDDICT:
	((WWFrame*)menu_bar_frame)->BuildDict(0);
	break;
    case WXWW_INTERRUPT:
	((WWFrame*)menu_bar_frame)->InterruptBuild();
	break;
    case WXWW_RESUME:
	((WWFrame*)menu_bar_frame)->BuildDict(1);
	break;
    default:
	return GameMenu::Handle(id);
    }
    return 0;
}

GameMenuBar::GameMenuBar(WWCanvas *canvas_in)
    : GameMenu(APPSHORTNAME, APPNAME, aboutWW, ini, canvas_in, 1),
      tool_menu(0)
{
}

//-------------------------------------------------------------------------

wxMenu *EditMenuBar::MakeFileMenu()
{
    wxMenu *file_menu = new wxMenu;
    file_menu->Append(GM_SAVE_GAME,  "&Save and Exit",  "Save board and return to game");
    file_menu->Append(GM_QUIT,  "&Exit without Saving","Return to game, discarding changes");
    return file_menu;
}

void EditMenuBar::SaveGame()
{
    ((WWFrame*)menu_bar_frame)->SaveBoard();
}

int EditMenuBar::Quit()
{
    ((WWFrame*)menu_bar_frame)->ExitBoardEditMode();
    return 0;
}

EditMenuBar::EditMenuBar(WWCanvas *canvas_in)
    : GameMenu(APPSHORTNAME, APPNAME, aboutWW, ini, canvas_in, 0)
{
}

//------------------------------------------------------------------------
// The right-hand panel for the different modes

void WWGamePanel::Update(WinWWGame *game)
{
    char buf[60];
    if (game == 0) return;
    SetLabelColour(*wxBLACK);
    sprintf(buf, "Round %d Move %d", game->GetRoundNum(), game->GetMoveNum());
    SetLabel(0, buf);
    sprintf(buf, "Tiles left: %d", game->Pool()->Size());
    SetLabel(1, buf);
    for (int p = 0; p < game->GetNumPlayers(); p++)
    {
	if (game->GameOver())
	    sprintf(buf, "Plyr %d: %3d (%-3d)", p+1,
		game->GetPlayer(p)->Score() - game->GetPlayer(p)->Penalty(),
		game->GetPlayer(p)->Score());
	else
	    sprintf(buf, "Plyr %d: %3d", p+1,
	    	game->GetPlayer(p)->Score());
	SetLabel(p+2, buf);
    }
}

WWGamePanel::WWGamePanel(wxFrame *f, int left, int top, int width, int height)
    : GamePanel(f, left, top, width, height,
	 	"Pass@Done@Auto@Next@Run@Stop@Undo@Redo", 2+MaxPlayers)
{
}

void WWGamePanel::ClearLabels()
{
    GamePanel::ClearLabels(2);
}

void WWGamePanel::HandleButton(int bnum)
{
    ((WWFrame*)window_parent)->HandleButton(bnum);
}

WWGamePanel::~WWGamePanel()
{
}

WWEditPanel::WWEditPanel(wxFrame *f, int left, int top, int width, int height)
    : GamePanel(f, left, top, width, height, 0, 0)
{
}

WWEditPanel::~WWEditPanel()
{
}

//--------------------------------------------------------------------

void WWFrame::PaintCallback(int slowly)
{
    ((WWCanvas*)canvas)->UndoTilePlacement();
    Refresh(slowly);
}

void WWFrame::UpdateStatusbar()
{
    char buf[80];
    if (mode == EDIT)
	strcpy(buf, "Edit the board layout.");
    else if (game==0)
	strcpy(buf, "No game.");
    else if (((WinWWGame*)game)->GameOver())
	strcpy(buf, "Game over.");
    else if (((WWCanvas*)canvas)->AreTilesExposed())
        sprintf(buf, "Player %d make your move!", ((WinWWGame*)game)->GetPlayerNum()+1);
    else
        sprintf(buf, "Player %d get ready!", ((WinWWGame*)game)->GetPlayerNum()+1);
    SetStatusText(buf);
}

void WWFrame::Refresh(int slowly)
{
    ((WWCanvas*)canvas)->Refresh(slowly);
    if (mode == GAME)
	game_panel->Update(((WinWWGame*)game));
    UpdateStatusbar();
    wxFlushEvents();
}

void WWFrame::AutoPlay()
{
    if (game && ((WinWWGame*)game)->GetPlayer()->IsAuto())
    {
        wxBeginBusyCursor();
	busy = 1;
    	char sec[16];
    	menubar->Disable();
    	sprintf(sec, "Game%d", gamenum);
    	int oldexposed = ((WWCanvas*)canvas)->AreTilesExposed();
    	int exposed = oldexposed;
    	(void)wxGetResource("Misc", "ShowRacks", &exposed, ini);
        ((WinWWGame*)game)->PlayMove();
	busy = 0;
    	(void)wxGetResource("Misc", "HideRacks", &exposed, ini);
    	((WWCanvas*)canvas)->ExposeTiles(oldexposed);
    	menubar->Enable();
        wxEndBusyCursor();
	UpdateStatusbar();
    }
}

void WWFrame::SetPlayerWeights(int pnum)
{
    if (game == 0) return;
    if (pnum < 0) pnum = ((WinWWGame*)game)->GetPlayerNum();
    char sec[16];
    int sn = 1;
    (void)wxGetResource(sec, "Strategy", &sn, ini);
    sprintf(sec, "Strategy%d", pnum+1);
    int lw[27];
    for (int t = 0; t < 27; t++)
    {
	char resn[12];
	sprintf(resn, "Weight%c", t==26 ? '_' : (t+'A'));
	lw[t] = 0;
	(void)wxGetResource(sec, resn, &lw[t], ini);
    }
    ((WinWWGame*)game)->GetPlayer(pnum)->SetWeights(lw);
}

void WWFrame::SetPlayerStrategy(int pnum)
{
    if (game == 0) return;
    char sec[16];
    if (pnum < 0) pnum = ((WinWWGame*)game)->GetPlayerNum();
    sprintf(sec, "Strategy%d", pnum+1);
    int rv;
    Player *p = ((WinWWGame*)game)->GetPlayer(pnum);
    if (p == 0) return;
    rv = 1; (void)wxGetResource(sec, "Adaptive", &rv, ini);
    p->SetAdaptive(rv);
    rv = 0; (void)wxGetResource(sec, "PenaliseSquares", &rv, ini);
    p->SetPenaliseSquares(rv);
    rv = 0; (void)wxGetResource(sec, "UseLetterWeights", &rv, ini);
    p->SetUseLetterWeights(rv);
    rv = 0; (void)wxGetResource(sec, "LengthWeight", &rv, ini);
    p->SetLengthWeight(rv);
    rv = 0; (void)wxGetResource(sec, "MinScore", &rv, ini);
    p->SetMinScore(rv);
    rv = 0; (void)wxGetResource(sec, "MaxScore", &rv, ini);
    p->SetMaxScore(rv);
    rv = 20; (void)wxGetResource(sec, "BlankThreshold", &rv, ini);
    p->SetBlankThreshhold(rv);
    rv = 0; (void)wxGetResource(sec, "MinLength", &rv, ini);
    p->SetMinLength(rv);
    rv = 21; (void)wxGetResource(sec, "MaxLength", &rv, ini);
    p->SetMaxLength(rv);
    rv = 0; (void)wxGetResource(sec, "MaxXWord", &rv, ini);
    p->SetMaxNewXWord(rv);
    rv = 0; (void)wxGetResource(sec, "Fudge", &rv, ini);
    p->SetFudge(rv);
    rv = 0; (void)wxGetResource(sec, "AnchorSkip", &rv, ini);
    p->SetSkip(rv);
}

//-------------------------------------------------------------------------

void WWFrame::SetMode(int m, GameMenu *mb, GamePanel *p)
{
    ((WWCanvas*)canvas)->SetMode(mode = m);
    mb->Init();
    SetMenuBar(mb);
    menubar = mb;
    panel->Show(FALSE);
    panel = p;
    panel->Show(TRUE);
    OnSize(0,0);
}

int WWFrame::FindFreeSlot(char *sec, char *res)
{
    int sn = 1;
    for (;;)
    {
	char ss[16];
	int junk;
	sprintf(ss, "%s%d", sec, sn);
	if (wxGetResource(ss, res, &junk, ini)== FALSE)
	    break;
	sn++;
    }
    return sn;
}

// Load a line of the board setup from ini file

void WWFrame::GetGameResources(int gnum, int &ff, int &hs, int &bn, int &pn,
			       int &rt, int &numtiles, int &bonus, 
			       int &bonustiles, int &pick, int &brows, 
			       int &bcols, int &frow, int &fcol, int &fdir,
			       int &showown)
{
    (void)wxGetResource("Misc", "ShowOwners", &showown, ini);
    char secname[64];
    sprintf(secname, "Game%d", gnum);
    (void)wxGetResource(secname, "FreeForm", &ff, ini);
    (void)wxGetResource(secname, "HotSquares", &hs, ini);
    (void)wxGetResource(secname, "Board", &bn, ini);
    (void)wxGetResource(secname, "Pool", &pn, ini);
    (void)wxGetResource(secname, "RackType", &rt, ini);
    (void)wxGetResource(secname, "RackTiles", &numtiles, ini);
    (void)wxGetResource(secname, "Bonus", &bonus, ini);
    (void)wxGetResource(secname, "BonusTiles", &bonustiles, ini);
    (void)wxGetResource(secname, "PickTiles", &pick, ini);
    sprintf(secname, "Board%d", bn);
    (void)wxGetResource(secname, "Rows", &brows, ini);
    (void)wxGetResource(secname, "Cols", &bcols, ini);
    (void)wxGetResource(secname, "FirstRow", &frow, ini);
    (void)wxGetResource(secname, "FirstCol", &fcol, ini);
    (void)wxGetResource(secname, "FirstDir", &fdir, ini);
}

char **WWFrame::LoadInitialBoard(int bnum, int brows, int bcols)
{
    char **bs = new char*[brows];
    for (int r = 0; r < brows; r++)
    {
	bs[r] = new char [bcols+1];
	LoadBoardLine(bnum, r, (r<15)?scrabboard[r]:0, bs[r], bcols);
    }
    return bs;
}

void WWFrame::FreeInitialBoard(char **bs, int brows)
{
    for (int r = 0; r < brows; r++)
	delete [] bs[r];
    delete [] bs;
}

void WWFrame::LoadPool(int pn, int ls[], int lf[])
{
    char secname[64];
    sprintf(secname, "Pool%d", pn);
    for (int t = 0; t < 27; t++)
    {
	char resn[12];
	sprintf(resn, "Score%c", t==26 ? '_' : (t+'A'));
	ls[t] = scrabscore[t];
	(void)wxGetResource(secname, resn, &ls[t], ini);
	sprintf(resn, "Number%c", t==26 ? '_' : (t+'A'));
	lf[t] = scrabfreq[t];
	(void)wxGetResource(secname, resn, &lf[t], ini);
    }
}

void WWFrame::NewGame()
{
    gamenum = -1;
    GameFrame::NewGame();
    ((WWCanvas*)canvas)->ExposeTiles(1);
    int showown;
    (void)wxGetResource("Misc", "ShowOwners", &showown, ini);
    ((WWCanvas*)canvas)->ShowOwners(showown);
    game_panel->Update(((WinWWGame*)game));
    if (game) ((WinWWGame*)game)->StartGame();
}

void WWFrame::EnterBoardEditMode()
{
    delete editboard;
    editboard = new WWEditBoard();
    ((WWCanvas*)canvas)->SetEditBoard(editboard);
    SetMode(EDIT, new EditMenuBar((WWCanvas*)canvas), edit_panel);
}

void WWFrame::ExitBoardEditMode()
{
    SetMode(GAME, new GameMenuBar((WWCanvas*)canvas), game_panel);
}

void WWFrame::SaveBoard()
{
    editboard->Save();
    ExitBoardEditMode();
}

//-----------------------------------------------------------------------
// Handle panel button pushes

void WWFrame::HandleButton(int bnum)
{
    if (bnum == 5) // stop background consults
	abortmatch = 1;
    if (dict == 0)
    {
	SetStatusText("No dictionary");
	return;
    }
    else if (game == 0 || ((WinWWGame*)game)->GameOver())
    {
	SetStatusText("No game in progress");
	return;
    }
    if (bnum == 5) // stop any background processing
    {
        if (game) 
	    ((WinWWGame*)game)->StopAuto();
    }
    else if (!AutoPlaying())
    {
        if ((bnum == 3 || bnum==6) && CanCheat())
	{
            switch (bnum)
            {
            case 3: // NEXT:
                ((WinWWGame*)game)->PlayNext();
      	        break;
            case 6: // UNDO
                game->Undo();
      	        break;
	    }
	}
	else if (!((WinWWGame*)game)->GameOver())
	{
            switch (bnum)
            {
            case 0: // PASS
	        ((WWCanvas*)canvas)->SetPassing(1);
	        SetStatusText("Discard tiles and click Done");
	        break;
            case 1: // DONE
	        char input[80];
	        if (((WWCanvas*)canvas)->AssembleMove(input) == 0)
	        {
	            char *msg = ((WinWWGame*)game)->PlayMove(input);
	            if (msg) SetStatusText(msg);
	            else
	            {
		        ((WWCanvas*)canvas)->UndoTilePlacement();
		        ((WinWWGame*)game)->Paint(((WinWWGame*)game)->GetPlayerNum(), 1);
	            }       
	        }
	    break;
            case 2: // AUTO
	        if (CanCheat())
	        {
	            ((WWCanvas*)canvas)->UndoTilePlacement();
	            wxBeginBusyCursor();
	            game->AutoPlay(1);
	            wxEndBusyCursor();
	        }
	        break;
            case 4: // RUN
	        ((WWCanvas*)canvas)->UndoTilePlacement();
	    	((WinWWGame*)game)->GetPlayer()->MakeAuto();
	        break;
            case 7: // REDO
                game->Redo();
      	        break;
	    }
	}
    }
}

// Toolbar buttons

void WWFrame::HandleToolbarClick(int tool)
{
#ifdef HAS_TOOLBAR
    int b;
    switch (tool) // just map tools to buttons
    {
    case WWToolbar::Pass:	b = 0;	break;
    case WWToolbar::Done:	b = 1;	break;
    case WWToolbar::Auto:	b = 2;	break;
    case WWToolbar::Next:	b = 3;	break;
    case WWToolbar::Run:	b = 4;	break;
    case WWToolbar::Stop:	b = 5;	break;
    case WWToolbar::Undo:	b = 6;	break;
    case WWToolbar::Redo:	b = 7;	break;
    }
    HandleButton(b);
#endif
}

//-----------------------------------------------------------------------
// Virtual methods

void WWFrame::WriteGame(FILE *fp)
{
    fprintf(fp, "%d %d\n", gamenum, ((WinWWGame*)game)->GetNumPlayers());
    game->Save(fp);
}

void WWFrame::AllocateNewGame()
{
    int brows = 15, bcols = 15, numtiles = 7, bonustiles = 7, bonus = 50;
    int lf[27], ls[27], frow = 8, fcol = 8, rt = 0, ff = 0,
	hs = 0, bn = 1, pn = 1, fdir = 0, pick = 0, showown = 0, p;
    if (gamenum < 0)
    {
	(void)wxGetResource("Misc", "NumPlayers", &numplayers, ini);
	RunDialog(new GetGameDlg(gamenum, numplayers, cheating));
    }
    GetGameResources(gamenum, ff, hs, bn, pn, rt, numtiles, bonus, bonustiles, 
		 	pick, brows, bcols, frow, fcol, fdir, showown);
    char **bs = LoadInitialBoard(bn, brows, bcols);
    LoadPool(pn, ls, lf);
    game = new WinWWGame(this, gamenum, cheating, numplayers, 
			brows, bcols, frow, fcol, fdir, numtiles, bonus,
			bonustiles, dict, lf, ls, bs, ff, hs, pick);
    FreeInitialBoard(bs, brows);
    for (p = 0; p < numplayers; p++)
	((WinWWGame*)game)->AddPlayer(1);
    game_panel->ClearLabels();
    ((WinWWGame*)game)->Reset();
    for (p = 0; p < numplayers; p++)
    {
	SetPlayerStrategy(p);
	SetPlayerWeights(p);
	((WinWWGame*)game)->GetPlayer(p)->SetRackType(rt);
    }
}

int WWFrame::AllocateGameForReading(FILE *fp)
{
    char buff[80];
    fgets(buff, 80, fp);
    if (sscanf(buff, "%d %d", &gamenum, &numplayers) == 2)
    {
	AllocateNewGame();
	if (game) return 0;
    }
    return -1;
}

GameToolbar *WWFrame::MakeToolbar()
{
#ifdef HAS_TOOLBAR
    return new WWToolbar(this);
#else
    return 0; // toolbar bitmaps/help is incomplete...
#endif
}

GamePanel *WWFrame::MakePanel(int x, int y, int w, int h)
{
    edit_panel = new WWEditPanel(this, x, y, w, h);
    game_panel = new WWGamePanel(this, x, y, w, h);
    return game_panel; // the initial panel
}

GameCanvas *WWFrame::MakeCanvas(int x, int y, int w, int h)
{
    return new WWCanvas(this, x, y, w, h);
}

GameMenu *WWFrame::MakeMenuBar()
{
    return new GameMenuBar(((WWCanvas*)canvas));
}

void WWFrame::InterruptBuild()
{
    delete dicbuilder;
    dicbuilder = 0;
}

void WWFrame::BuildDict(int restore)
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

void WWFrame::DumpDict()
{
    char suf[10];
    strcpy(suf, "*.dic");
    char *s = wxFileSelector("Dictionary File", 0, 0, 0, suf);
    if (s)
    {
        Dictionary *dict = 0;
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
	    delete dict;
	}
	else (void)wxMessageBox(err, "Load failed", wxOK|wxCENTRE);
    }
}

//-------------------------------------------------------------------------

WWFrame::WWFrame(char *title, int w, int h)
    : GameFrame(title, w, h, "winww", "*.gam"),
      game_panel(0),
      edit_panel(0),
      game_menu(0),
      edit_menu(0),
      editboard(0),
      busy(0),
      mode(GAME),
      gamenum(1),
      cheating(1),
      dicbuilder(0)
{
}

void WWFrame::Init()
{
    GameFrame::Init();
    ((WWCanvas*)canvas)->ExposeTiles(0);
}

void WWFrame::BackgroundProcess()
{
    AutoPlay();
}

WWFrame::~WWFrame()
{
    delete dicbuilder;
    delete dict;
    delete game;
}

//------------------------------------------------------------------------
// The `main program' equivalent, creating the windows and returning the
// main frame

char *WWApp::MakeDictionary()
{
//    dict = new Dictionary;
    char *df = new char [256];
    strcpy(df, "wwmed.dic");
    (void)wxGetResource("Misc", "Dictionary", &df, ini);
    char *msg = ::MakeDictionary(dict, df);
    delete [] df;
    return msg;
}

void MyBackgroundProcess(void *a)
{
    ((WWApp*)a)->BackgroundProcess();
}

wxFrame *WWApp::OnInit(void)
{
#ifdef wx_x
    srand(time(0));
#endif
    char *u = UserName(), *e = EMail();
    sprintf(aboutWW, "%s v%s\nby Graham Wheeler\ngram@cdsec.com\n(c) 1991-1998\n%s\n%s\n%s",
		APPNAME, WWVERSION, 
		(IsRegistered() ? "Registered to:" : "Unregistered"),
		UserName(), EMail());
    // Create the main frame window
    char *msg = MakeDictionary();
    if (msg) (void)wxMessageBox(msg, "Dictionary Load Error", wxOK|wxCENTRE);
    frame = new WWFrame("WordsWorth for Windows", 650, 500);
    frame->Init();
    AddCycler(MyBackgroundProcess, this);
#ifdef wx_msw
    SetPrintMode(wxPRINT_WINDOWS);
#else
    SetPrintMode(wxPRINT_POSTSCRIPT);
#endif
    return frame;
}

