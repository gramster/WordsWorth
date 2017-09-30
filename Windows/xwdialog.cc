/*
 * File:	xwdialog.cc
 * Purpose:	XWord for WxWindows config (and other) dialogs
 * Author:	Graham Wheeler
 * Created:	1995
 * Updated:	
 * Copyright:	(c) 1995, Graham Wheeler
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#ifdef __GNUG__
#pragma implementation
#endif

#include "wx.h"
#include "ggame.h"
#include "gpanel.h"
#include "gdialog.h"
#include "gcanvas.h"
#include "gmenu.h"
#include "gframe.h"
#include "dict.h"
#include "xword.h"
#include "winxw.h"
#include "register.h"
#include "xwdialog.h"

#ifdef wx_msw
#include "mmsystem.h"
#endif

WordFlagDlg::WordFlagDlg(CrossWord *xword_in, int r_in, int c_in)
    : GameDialog("Clue Settings"),
      r(r_in), c(c_in), xword(xword_in)
{
    int across = xword->IsStartOfAcrossWord(r, c);
    if (across)
    {
	int ba = xword->BypassDictionaryWordAcross(r, c);
        ovra = new wxCheckBox(this,0,"Bypass dictionary across");
        ovra->SetValue(ba ? TRUE : FALSE);
        NewLine();

        char av[20], buf[20];
        av[0] = '\0';
        int l = 0;
        int tc = c;
        for (;;)
        {
  	    if (l && xword->IsStartOfAcrossWord(r, tc))
  	    {
  	        sprintf(buf, "%d", l);
  	        if (av[0]) strcat(av, ",");
  	        strcat(av,buf);
  	        l = 0;
  	    }
  	    l++;
  	    tc++;
  	    if (tc == xword->Width() || xword->IsBlack(r, tc))
	  	break;
        }
        sprintf(buf, "%d", l);
        if (av[0]) strcat(av, ",");
        strcat(av,buf);
        avtxt = new wxText(this, 0, "Word length(s) across");
        avtxt->SetValue(av);
	avtxt->SetFocus();
        NewLine();
	atoptxt = new wxText(this, 0, "Concept keys across:");
	char *t = xword->GetAcrossTopic(r, c);
	atoptxt->SetValue(t?t:"");
	NewLine();
	aanatxt = new wxText(this, 0, "Anagram across:");
	char *a = xword->GetAcrossAnagram(r, c);
	aanatxt->SetValue(a?a:"");
	NewLine();
    }  
    else
    {
        avtxt = 0;
        ovra = 0;
	atoptxt = 0;
	aanatxt = 0;
    }
    int down = xword->IsStartOfDownWord(r, c);
    if (down)
    {
	int bd = xword->BypassDictionaryWordDown(r, c);
        ovrd = new wxCheckBox(this,0,"Bypass dictionary down");
        ovrd->SetValue(bd ? TRUE : FALSE);
        NewLine();
        char dv[20], buf[20];
        dv[0] = '\0';
        int l = 0;
        int tr = r;
        for (;;)
        {
  	    if (l && xword->IsStartOfDownWord(tr, c))
  	    {
  	        sprintf(buf, "%d", l);
  	        if (dv[0]) strcat(dv, ",");
  	        strcat(dv,buf);
  	        l = 0;
  	    }
  	    l++;
  	    tr++;
  	    if (tr == xword->Height() || xword->IsBlack(tr, c))
  	        break;
        }
        sprintf(buf, "%d", l);
        if (dv[0]) strcat(dv, ",");
        strcat(dv,buf);
        dvtxt = new wxText(this, 0, "Word length(s) down");
        dvtxt->SetValue(dv);
	if (avtxt==0) dvtxt->SetFocus();
        NewLine();
	dtoptxt = new wxText(this, 0, "Concept keys down:");
	char *t = xword->GetDownTopic(r, c);
	dtoptxt->SetValue(t?t:"");
	NewLine();
	danatxt = new wxText(this, 0, "Anagram down:");
	char *a = xword->GetDownAnagram(r, c);
	danatxt->SetValue(a?a:"");
	NewLine();
    }
    else
    {
        dvtxt = 0;
        ovrd = 0;	
	dtoptxt = 0;
	danatxt = 0;
    }
    AddButton(1, "OK", 1);
    Tab();
    AddButton(0, "Cancel");
    Fit();
}

void WordFlagDlg::Save()
{
   if (avtxt)
   {
	xword->SetAcrossTopic(r, c, atoptxt->GetValue());
	xword->SetAcrossAnagram(r, c, aanatxt->GetValue());
	char *as = avtxt->GetValue();
	int l = 0, tc = c;
 	int ovra_ok = 0;
 	int starta_ok = 0;
	if (ovra && ovra->GetValue()) ovra_ok = 1;
	while (*as)
	{
	    while (isspace(*as)) as++;
	    while (isdigit(*as))
	    {
		l = l * 10 + (*as) - '0';
		as++;
	    }
	    while (--l>=0)
	    {
		xword->ClearAcrossFlags(r, tc);
		if (starta_ok) xword->StartWordAcross(r, tc);
		if (ovra_ok) xword->AllowAnyAcross(r, tc);
		if (xword->IsBlack(r, tc)) xword->AddLetter(r, tc, '?');
		starta_ok = 0;
		tc++;
	    }
	    while (isspace(*as)) as++;
	    if (*as == ',')
	    {
		as++;
		l = 0;
		xword->StartWordAcross(r, tc);
		starta_ok = 1; // prevent clear of this STARTA
	    }
	    else break;
	}
    }
    if (dvtxt)
    {
	xword->SetDownTopic(r, c, dtoptxt->GetValue());
	xword->SetDownAnagram(r, c, danatxt->GetValue());
	char *ds = dvtxt->GetValue();
	int l = 0, tr = r;
 	int ovrd_ok = 0;
 	int startd_ok = 0;
	if (ovrd && ovrd->GetValue()) ovrd_ok = 1;
	while (*ds)
	{
	    while (isspace(*ds)) ds++;
	    while (isdigit(*ds))
	    {
		l = l * 10 + (*ds) - '0';
		ds++;
	    }
	    while (--l>=0)
	    {
		xword->ClearDownFlags(tr, c);
		if (startd_ok) xword->StartWordDown(tr, c);
		if (ovrd_ok) xword->AllowAnyDown(tr, c);
		if (xword->IsBlack(tr, c)) xword->AddLetter(tr, c, '?');
		startd_ok = 0;
		tr++;
	    }
	    while (isspace(*ds)) ds++;
	    if (*ds == ',')
	    {
		ds++;
		l = 0;
		xword->StartWordDown(tr, c);
		startd_ok = 1; // prevent clear of this STARTD
	    }
	    else break;
	}
    }
}

void WordFlagDlg::HandleButton(int bnum)
{
    if (bnum==1) Save();
    GameDialog::HandleButton(0);
}

WordFlagDlg::~WordFlagDlg()
{
}

//-----------------------------------------------------------------------

SquareFlagDlg::SquareFlagDlg(CrossWord *xword_in, int r_in, int c_in)
    : GameDialog("Square Settings"),
       r(r_in), c(c_in), xword(xword_in)
{
    int down = xword->IsInDownWord(r, c);
    int across = xword->IsInAcrossWord(r, c);
    if (across)
    {
        ovra = new wxCheckBox(this,0,"Bypass dictionary across");
        ovra->SetValue(xword->BypassDictionaryAcross(r, c) ? TRUE : FALSE);
        NewLine();
        nwa = new wxCheckBox(this,0,"Start of new across word");
        nwa->SetValue(xword->IsStartOfAcrossWord(r, c) ? TRUE : FALSE);
        NewLine();
    }  
    else
    {
        ovra = 0;	
        nwa = 0;
    }
    if (down)
    {
        ovrd = new wxCheckBox(this,0,"Bypass dictionary down");
        ovrd->SetValue(xword->BypassDictionaryDown(r, c) ? TRUE : FALSE);
        NewLine();
        nwd = new wxCheckBox(this,0,"Start of new down word");
        nwd->SetValue(xword->IsStartOfDownWord(r, c) ? TRUE : FALSE);
        NewLine();
    }  
    else
    {
        ovrd = 0;	
        nwd = 0;
    }
    ltrs = new wxText(this, 0, "Possible Letters:");
    ltrs->SetValue(xword->GetInitialConstraints(r, c));
    NewLine();
    AddButton(1, "OK", 1);
    Tab();
    AddButton(0, "Cancel");
    Fit();
}

void SquareFlagDlg::HandleButton(int bnum)
{
    if (bnum==1)
    {
	xword->ClearFlags(r, c);
	if (ovra && ovra->GetValue()) xword->AllowAnyAcross(r, c);
	if (ovrd && ovrd->GetValue()) xword->AllowAnyDown(r, c);
	if (nwa && nwa->GetValue()) xword->StartWordAcross(r, c);
	if (nwd && nwd->GetValue()) xword->StartWordDown(r, c);
	xword->SetInitialConstraints(r, c, ltrs->GetValue());
    }
    GameDialog::HandleButton(0);
}

SquareFlagDlg::~SquareFlagDlg()
{
}

//-----------------------------------------------------------------------

GridSizeDlg::GridSizeDlg(int &r_in, int &c_in)
    : GameDialog("Grid Size"), r(r_in), c(c_in)
{
    rows = MakeSlider("Rows", r, 7, MAX_XWORD);
    cols = MakeSlider("Cols", c, 7, MAX_XWORD);
    AddButton(1, "OK", 1);
    Fit();
}

void GridSizeDlg::HandleButton(int bnum)
{
    if (bnum==1)
    {
	r = rows->GetValue();
	c = cols->GetValue();
    }
    GameDialog::HandleButton(0);
}

GridSizeDlg::~GridSizeDlg()
{
}

//----------------------------------------------------------------

SymmetryDlg::SymmetryDlg(int &sym_in)
    : GameDialog("Grid Symmetry"), sym(sym_in)
{
    char *symstr[5];
    symstr[0] = "None";
    symstr[1] = "Mirror left to right";
    symstr[2] = "Mirror top to bottom";
    symstr[3] = "Top left to other quadrants";
    symstr[4] = "Mirror and flip";
    symbox = new wxRadioBox(this,0,"Type:",-1,-1,-1,-1,5,symstr,1);
    symbox->SetSelection(sym);
    NewLine();
    AddButton(1, "OK", 1);
    Fit();
}

void SymmetryDlg::HandleButton(int bnum)
{
    if (bnum==1) sym = symbox->GetSelection();
    GameDialog::HandleButton(0);
}

SymmetryDlg::~SymmetryDlg()
{
}

//----------------------------------------------------------------

extern Dictionary *dict;

OptionsDlg::OptionsDlg()
    : GameDialog("Default Options")
{
    int r = 15, c = 15;
    (void)wxGetResource("Grid", "Rows", &r, ini);
    (void)wxGetResource("Grid", "Cols", &c, ini);
    rows = MakeSlider("Rows", r, 7, MAX_XWORD);
    cols = MakeSlider("Cols", c, 7, MAX_XWORD);
    char *symstr[5];
    symstr[0] = "None";
    symstr[1] = "Mirror left to right";
    symstr[2] = "Mirror top to bottom";
    symstr[3] = "Top left to other quadrants";
    symstr[4] = "Mirror and flip";
    symbox = new wxRadioBox(this,0,"Type:",-1,-1,-1,-1,5,symstr,1);
    int sym = 0;
    (void)wxGetResource("Grid", "Symmetry", &sym, ini);
    symbox->SetSelection(sym);
    NewLine();
    char *df = new char [256];
    strcpy(df, "wwmed.dic");
    (void)wxGetResource("Misc", "Dictionary", &df, ini);
    dic = new wxText(this, 0, "Dictionary:");
    dic->SetValue(df);
    delete [] df;
    NewLine();
    AddButton(1, "OK", 1);
    Tab();
    AddButton(0, "Cancel");
    Tab();
    AddButton(2, "Browse");
    Fit();
}

void OptionsDlg::HandleButton(int bnum)
{
    if (bnum == 2)
    {
	char *d = ::wxFileSelector("Select Dictionary", 0, 0,
				       "dic", "*.dic", wxOPEN);
	if (d) dic->SetValue(d);
	return;
    }
    if (bnum == 1)
    {
        (void)wxWriteResource("Misc", "Dictionary", dic->GetValue(), ini);
        (void)wxWriteResource("Grid", "Rows", rows->GetValue(), ini);
        (void)wxWriteResource("Grid", "Cols", cols->GetValue(), ini);
        (void)wxWriteResource("Grid", "Symmetry", symbox->GetSelection(), ini);
	delete dict;
	dict = 0;
	char *msg = MakeDictionary(dict, dic->GetValue());
	if (msg) 
	{
	    (void)wxMessageBox(msg, "Dictionary Load Error", wxOK|wxCENTRE);
	    return;
	}
    }
    GameDialog::HandleButton(0);
}

OptionsDlg::~OptionsDlg()
{
}


