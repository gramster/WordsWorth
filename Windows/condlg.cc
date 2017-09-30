/*
 * File:	condlg.cc
 * Purpose:	Dictionary consult dialog common to XWord and WordsWorth
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
//#include "wwx_timer.h"
//#include "wx_help.h"
#include "gpanel.h"
#include "gdialog.h"
#include "gcanvas.h"
#include "dict.h"
#include "condlg.h"

#ifdef wx_msw
#include "mmsystem.h"
#endif

extern Dictionary *dict;

// kludge callback hook

wxFrame *ShowProgressFrame = 0;

void ShowProgress(int p)
{
    if (ShowProgressFrame)
    {
        char x[8];
        sprintf(x, "%3d%%", p);
        ShowProgressFrame->SetStatusText(x);
    }
}

static void ConsultCycle(void *arg)
{
    if (abortmatch || dict->NextConsult() != 0)
    {
	wxEndBusyCursor();
	RemoveCycler(ConsultCycle);
	FILE *fp = dict->GetLogFile();
	if (fp)
	{
	    if (abortmatch) fprintf(fp, "INTERRUPTED!\n");
	    fprintf(fp, "%ld matches found\n\n\n\n", dict->Matches());
	    fclose(fp);
	}
	dict->SetLogFile(0);
	FileBrowser *viewer = new FileBrowser("Search Results",
	 	(char*)arg);
	if (viewer) viewer->Show(TRUE);
	unlink((char*)arg);
    }
}

#ifndef __MSDOS__
char *strupr(char *s)
{
    char *rtn = s;
    if (s) while (*s) { if (islower(*s)) *s = toupper(*s); s++; }
    return rtn;
}
#endif

//--------------------------------------------------------------------

ConsultDlg::ConsultDlg(const char *ini_in, wxFrame *f_in)
    : GameDialog("Consult"), ini(ini_in)
{
    ShowProgressFrame = f_in;
    char *modes[4], *sec = "Consult";
    modes[0] = "single word anagrams";
    modes[1] = "single word prefixes";
    modes[2] = "single word suffixes";
    modes[3] = "multi-word anagrams";
    int v = 0;
    (void)wxGetResource(sec, "Type", &v, ini);
    mod = new wxRadioBox(this,0,"Find words which are:",-1,-1,-1,-1,4,modes,1);
    mod->SetSelection(v);
    NewLine();
    pat = new wxText(this, 0, "Pattern:");
    pat->SetValue("XXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
    pat->SetFocus();
    NewLine();
    topic = new wxText(this, 0, "Concept Keys:");
    topic->SetValue("XXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
    NewLine();
    (void)new wxMessage(this, "Prefix, suffix and multi-word options:");
    NewLine();
    v = 0; (void)wxGetResource(sec, "MinLen", &v, ini);
    minl = MakeSlider("Min Word Length", 0, 0, 12);
    minl->SetValue(v);
    NewLine();
    v = 0; (void)wxGetResource(sec, "MaxLen", &v, ini);
    maxl = MakeSlider("Max Word Length", 0, 0, 16);
    maxl->SetValue(v);
    NewLine();
    v = 0; (void)wxGetResource(sec, "MinCnt", &v, ini);
    minw = MakeSlider("Min Word Count ", 0, 0, 10);
    minw->SetValue(v);
    NewLine();
    v = 0; (void)wxGetResource(sec, "MaxCnt", &v, ini);
    maxw = MakeSlider("Max Word Count ", 0, 0, 25);
    maxw->SetValue(v);
    NewLine();
    Tab();
    AddButton(1, "OK", 1);
    Tab();
    AddButton(0, "Cancel");
    Tab();
    AddButton(2, "Foreground");
    Fit();
    char *p = new char[80];
    p[0]=0;
    (void)wxGetResource(sec, "Pattern", &p, ini);
    pat->SetValue(p);
    p[0]=0;
    (void)wxGetResource(sec, "Topic", &p, ini);
    topic->SetValue(p);
    delete [] p;
    // kill any background consults
    extern int abortmatch;
    abortmatch = 1;
    RunCycler(ConsultCycle);
}

void ConsultDlg::HandleButton(int bnum)
{
    if (abortmatch==0 || bnum==0)
	abortmatch = 1;
    else
    {
    	char patbuf[256];
    	strcpy(patbuf, pat->GetValue());
    	char topicbuf[256];
    	strcpy(topicbuf, topic->GetValue());
    	int typ = mod->GetSelection();
    	int mincnt = minw->GetValue();
    	int maxcnt = maxw->GetValue();
    	int minlength = minl->GetValue();
    	int maxlength = maxl->GetValue();

    	char *tn = tmpnam(0);
    	FILE *cfp = fopen(tn, "w");
    	if (cfp == 0) return; // show error!

    	char *sec = "Consult";
    	(void)wxWriteResource(sec, "Type", typ, ini);
    	(void)wxWriteResource(sec, "MinLen", minlength, ini);
    	(void)wxWriteResource(sec, "MaxLen", maxlength, ini);
    	(void)wxWriteResource(sec, "MinCnt", mincnt, ini);
    	(void)wxWriteResource(sec, "MaxCnt", maxcnt, ini);
    	(void)wxWriteResource(sec, "Pattern", patbuf, ini);
    	(void)wxWriteResource(sec, "Topic", topicbuf, ini);

    	dict->SetLogFile(cfp);

    	fprintf(cfp, "Looking for words that are %s of %s\n",
			mod->GetString(typ), patbuf);
    	fprintf(cfp, "Min Length: %d\n", minlength);
    	fprintf(cfp, "Max Length: %d\n", maxlength);
    	fprintf(cfp, "Min Count:  %d\n", mincnt);
    	fprintf(cfp, "Max Count:  %d\n", maxcnt);
    	fprintf(cfp, "Concept constraints: %s\n", topicbuf);
    
    	wxBeginBusyCursor();
    	if (bnum == 2)
    	{
#if 0
            dict->MatchPattern(patbuf, typ, minlength, maxlength, mincnt, maxcnt, topicbuf);
#else
            dict->StartConsult(patbuf, typ, minlength, maxlength, mincnt, maxcnt, topicbuf);
	    while (dict->NextConsult() == 0);
#endif
            fprintf(cfp, "%ld matches found\n\n\n\n", dict->Matches());
            fclose(cfp);
	    wxEndBusyCursor();
            dict->SetLogFile(0);
            FileBrowser *viewer = new FileBrowser("Search Results", tn);
            if (viewer) viewer->Show(TRUE);
	    unlink(tn);
    	}
    	else
    	{
            dict->StartConsult(patbuf, typ, minlength, maxlength, mincnt, maxcnt, topicbuf);
            AddCycler(ConsultCycle, tn);
    	}
    }
    GameDialog::HandleButton(0);
}

ConsultDlg::~ConsultDlg()
{
}

