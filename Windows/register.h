#ifndef _REGISTER_H
#define _REGISTER_H

class Dictionary;

char *UserName();
char *EMail();
int IsRegistered();
long MaxDicSize();
char *LoadDictionary(Dictionary *dict, char *dictname, unsigned char *key = 0);
char *MakeDictionary(Dictionary *&dict, char *dictname, unsigned char *key = 0);
int Register(char *user, char *email, char *key);

#ifdef MAIN

class GameDialog;

class RegisterDlg : public GameDialog
{
    const char *ini;
    wxText *user, *email, *key;
public:
    RegisterDlg(const char *ini_in);
    virtual void HandleButton(int bnum);
    virtual ~RegisterDlg();
};

#endif
#endif

