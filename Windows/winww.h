/*
 * File:	winww.h
 * Purpose:	WordsWorth for wxWindows
 * Author:	Graham Wheeler
 * Created:	1995
 * Updated:	
 * Copyright:	(c) 1995, Graham Wheeler
 */

/* sccsid[] = "%W% %G%" */

#ifndef WINWW_H
#define WINWW_H

#ifdef __GNUG__
#pragma interface
#endif

// major modes

#define GAME	0	// game play
#define EDIT	1	// board editor

// Initialisation file

extern char *ini;

class Game;
class GameMenu;
class WWEditBoard;
class WWCanvas;
class WWFrame;

class WinWWGame : public WWGame
{
    WWFrame *frame;
    int gnum;
    int cheating;
public:
    WinWWGame(WWFrame *frame_in, int gnum_in, int cheating_in,
	int numplayers_in, int rows, int cols, int frow, int fcol, int fdir,
	int numtiles_in, int bonus, int bonustiles, Dictionary *dict,
	int *poolfreq, int *letterscores, char **boardscores,
	int freeform, int hotsquares, int picktiles);
    virtual void Paint(int nextplayer, int slowly);
    virtual int Override(char *msg);
    virtual char *GetTileSelection(char *errmsg);
    inline int GameNum() const
    {
	return gnum;
    }
    inline int Cheating() const
    {
	return cheating;
    }
    virtual ~WinWWGame();
};

//----------------------------------------------------------------
// The right hand panels for the different modes

class WWGamePanel : public GamePanel
{
  public:
    WWGamePanel(wxFrame *f, int left, int top, int width, int height);
    void ClearLabels();
    void Update(WinWWGame *game);
    virtual void HandleButton(int bnum);
    virtual ~WWGamePanel();
};

class WWEditPanel : public GamePanel
{
  public:
    WWEditPanel(wxFrame *f, int left, int top, int width, int height);
    virtual ~WWEditPanel();
};

class GameMenuBar : public GameMenu
{
  protected:
    wxMenu *tool_menu;
    int cheating;
    int gnum;
    WWFrame *frame;
    virtual wxMenu *MakeOptionsMenu();
    virtual void MakeMiddleMenus();
    virtual void DisableGameSpecific();
    void ConfigureMisc();
    void ConfigurePoolSize(int gnum);
    void ConfigureLetterScores(int gnum);
    void ConfigureLetterWeights(int gnum);
  public:
    GameMenuBar(WWCanvas *canvas_in);
    virtual int Handle(int id);
    virtual ~GameMenuBar()
    {}
};

class EditMenuBar : public GameMenu
{
  protected:
    virtual wxMenu *MakeFileMenu();
    virtual void SaveGame();
    virtual int Quit();
  public:
    EditMenuBar(WWCanvas *canvas_in);
    virtual ~EditMenuBar()
    {}
};

class WWFrame: public GameFrame
{
    WWGamePanel *game_panel;
    WWEditPanel *edit_panel;
    GameMenu    *game_menu;
    GameMenu    *edit_menu;
    WWEditBoard *editboard;
    int busy;
    int mode;
    int gamenum;
    int numplayers;
    int cheating;
    class DictionaryBuilder *dicbuilder;

    // board editing/creation

    void EditSquare(int r, int c);

    void SetMode(int m, GameMenu *mb, GamePanel *p);

    int  AddPlace(int tnum, int newr, int newc);
    int  FindFreeSlot(char *sec, char *res);
    void LoadEditBoard();
    void SaveEditBoard();
    void UpdateStatusbar();
    void ChooseGameType(int &gnum, int &numplayers, int &cheating);
    void GetGameResources(int gnum, int &ff, int &hs, int &bn, int &pn,
			       int &rt, int &numtiles, int &bonus, 
			       int &bonustiles, int &pick, int &brows, 
			       int &bcols, int &frow, int &fcol, int &fdir,
			       int &showown);
    char **LoadInitialBoard(int bnum, int brows, int bcols);
    void FreeInitialBoard(char **bs, int brows);
    void LoadPool(int pn, int ls[], int lf[]);
    int CanCheat() { return cheating; }
    int AutoPlaying() const;

    virtual void WriteGame(FILE *fp);
    virtual void AllocateNewGame();
    virtual int AllocateGameForReading(FILE *fp);
    virtual GamePanel *MakePanel(int x, int y, int w, int h);
    virtual GameToolbar *MakeToolbar();
    virtual GameCanvas *MakeCanvas(int x, int y, int w, int h);
    virtual GameMenu *MakeMenuBar();
  public:
    void AutoPlay();
    void InterruptBuild();
    void BuildDict(int restore);
    void DumpDict();
    void ShowErrorLocation(const char *errmsg);
    void HideErrorLocation(const char *errmsg);
    void PaintCallback(int slowly); // hook called indirectly by Game
    virtual void HandleButton(int bnum);
    virtual void HandleToolbarClick(int tool);
    void Refresh(int slowly);
    void SetPlayerStrategy(int pnum = -1);
    int NewGame(int gnum, int numplay);
    void SetPlayerWeights(int pnum = -1);
    // Menu handlers
    virtual void NewGame();
    void SaveBoard();
    void EnterBoardEditMode();
    void ExitBoardEditMode();
    WWFrame(char *title, int w, int h);
    void BackgroundProcess();
    virtual void Init();
    virtual ~WWFrame();
};

class WWApp: public wxApp
{
    WWFrame *frame;
    char *MakeDictionary();
  public:
    WWApp() {}
    void BackgroundProcess();
    wxFrame *OnInit();
};

//-----------------------------------------------------------------------

inline void WWApp::BackgroundProcess()
{
    frame->AutoPlay();
}

inline void WWFrame::ShowErrorLocation(const char *errmsg)
{
    ((WWCanvas*)canvas)->PaintErrorLocation(errmsg, 0);
}

inline void WWFrame::HideErrorLocation(const char *errmsg)
{
    ((WWCanvas*)canvas)->PaintErrorLocation(errmsg, 1);
}

inline int WWFrame::AutoPlaying() const
{
    return busy;
}

#ifndef max
#define max(a, b)	( ((a)>(b)) ? (a) : (b) )
#endif
#ifndef min
#define min(a, b)	( ((a)>(b)) ? (b) : (a) )
#endif

#endif

