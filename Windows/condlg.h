/*
 * File:	commdlg.h
 * Purpose:	Dialogs common to XWord and WordsWorth
 * Author:	Graham Wheeler
 * Created:	1996
 * Updated:	
 * Copyright:	(c) 1996, Graham Wheeler
 */

#ifndef COMMDLG_H
#define COMMDLG_H

#ifdef __GNUG__
#pragma interface
#endif

#ifndef __MSDOS__
char *strupr(char *s);
#endif

class ConsultDlg : public GameDialog
{
    const char *ini;
//    wxFrame *f;
    wxRadioBox *mod;
    wxSlider *minl, *maxl, *minw, *maxw;
    wxText *pat, *topic;
public:
    ConsultDlg(const char *ini_in, wxFrame *f_in = 0);
    virtual void HandleButton(int bnum);
    virtual ~ConsultDlg();
};

// this is a kludge - it's just here cause this is included by
// both xword and ww

void ReadRegKey(char *key);

#endif

