/*
 * File:	winxw.h
 * Purpose:	XWord for wxWindows
 * Author:	Graham Wheeler
 * Created:	1995
 * Updated:	
 * Copyright:	(c) 1995, Graham Wheeler
 */

#ifndef WINXW_H
#define WINXW_H

#ifdef __GNUG__
#pragma interface
#endif

extern char *ini;

class XWordCanvas;
class XWordFrame;
class CrossWord;

//----------------------------------------------------------------

class XWordApp: public wxApp
{
    char *XWordApp::MakeDictionary();
  public:
    XWordApp() {}
    wxFrame *OnInit();
};

class XWordMenuBar : public GameMenu
{
  protected:
    wxMenu *grid_menu;
    wxMenu *solve_menu;
    wxMenu *tool_menu;
    virtual wxMenu *MakeOptionsMenu();
    virtual void MakeMiddleMenus();
    virtual void EnableGameSpecific();
    virtual void DisableGameSpecific();
  public:
    XWordMenuBar(GameCanvas *canvas_in);
    void Lock();
    void Unlock();
    void SetCrunch(int v);
    virtual int Handle(int id);
    virtual ~XWordMenuBar()
    {}
};

class XWordCanvas: public GameCanvas
{
  protected:
    void Select(int r, int c);
    virtual void HandleLeftMouseClickOnBoard(int r, int c);
    virtual void HandleRightMouseClickOnBoard(int r, int c);
    virtual void HandleLeftMouseClickOffBoard(int x, int y);
    virtual void HandleKey(int ch);
  public:
    XWordCanvas(wxFrame *frame, int x, int y, int w, int h);
    virtual void SetGame(Game *game_in);
    void DrawCursor(int r, int c);
    virtual void DrawSquare(int r, int c);
    virtual void DrawBoard();
    virtual ~XWordCanvas() ;
};

class XWordPanel : public GamePanel
{
  public:
    XWordPanel(wxFrame *f, int left, int top, int width, int height);
    void SetAcrossLabel(int n, char *txt);
    void SetDownLabel(int n, char *txt);
    void SetDownCount(long cnt);
    void SetAcrossCount(long cnt);
    void Clear();
    virtual ~XWordPanel();
};

class XWordFrame: public GameFrame
{
    int		lock;
    class DictionaryBuilder *dicbuilder;

    int IsDecideable(int r, int c);
    int AddCrossWord(char *word);
    int RecursiveSearch(int pos, long n);
    void ComputeCrossCheck(int r, int c, unsigned dir);
    void ComputeGlobalChecks();
    int AddEntry(int r, int c, int ch);
    int SetEntry(int r, int c, int ch);
    int ComputeLocalCheck(int r, int c, int dir);
    void SolveIt(int clear);
    friend class WordFlagDlg;
    friend class SquareFlagDlg;

    virtual void LoadGame();
    virtual GamePanel *MakePanel(int x, int y, int w, int h);
    virtual GameCanvas *MakeCanvas(int x, int y, int w, int h);
    virtual GameMenu *MakeMenuBar();
    virtual void AllocateNewGame();
    virtual int AllocateGameForReading(FILE *fp);

    void GetGridSize(int &rows, int &cols);
  public:
    void InterruptBuild();
    void BuildDict(int restore);
    void DumpDict();
    void ToggleLock();
    void Crunch();
    void UpdateDisplay(int clear);
    void ShowChoices();
    void BackgroundProcess();
    XWordFrame();
    virtual ~XWordFrame();
};

#ifndef max
#define max(a, b)	( ((a)>(b)) ? (a) : (b) )
#endif
#ifndef min
#define min(a, b)	( ((a)>(b)) ? (b) : (a) )
#endif

#endif

