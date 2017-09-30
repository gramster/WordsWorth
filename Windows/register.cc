#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>

#ifdef __GNUG__
#pragma implementation
#endif

#include "wx.h"
#include "gdialog.h"

#include "blowfish.h"
#include "encode.h"
#include "dict.h"
#define MAIN
#include "register.h"

const unsigned char *DemoKey = "+\\ke;*^23!x$"; // only 64-bits are used

extern const char *ini;

//---------------------------------------------------------
// Private access

static void SetUserName(char *user)
{
    (void)wxWriteResource("Registration", "User", user, ini);
}

static void SetEMail(char *email)
{
    (void)wxWriteResource("Registration", "EMail", email, ini);
}

static void SetRegKey(char *regkey)
{
    (void)wxWriteResource("Registration", "Key", regkey, ini);
}

static char *RegKey()
{
    static char regkey[128] = { 0 };
    if (regkey[0]==0)
    {
        char *k=0;
        (void)wxGetResource("Registration", "Key", &k, ini);
	if (k) strcpy(regkey, k);
	else strcpy(regkey, "################");
	delete [] k;
    }
    return regkey;
}

// decode key to 80-bit vector

static unsigned char *RegVector() // the decoded 80-bit vector
{
    return Key2Vector(RegKey());
}

//static unsigned short Hash()
//{
//    return Hash(RegVector());
//}

//---------------------------------------------------------

static unsigned char *GetRegisteredKey(const unsigned char *rvec = 0,
					const char *user = 0,
					const char *email = 0)
{
    if (rvec == 0) rvec = RegVector();
    if (user == 0) user = UserName();
    if (email == 0) email = EMail();
    if (strcmp(ini, "winww.ini") == 0)
	return RegisteredKey(rvec, user, email);
    else
	return RegisteredKey(rvec, email, user);
}

static unsigned char *RegisteredKey(const char *rkey)
{
    return GetRegisteredKey(Key2Vector(rkey));
}

static unsigned char *RegisteredKey()
{
    static unsigned char key[8];
    static int first = 1;
    if (first)
    {
        first = 0;
	memcpy(key, GetRegisteredKey(), 8);
    }
    return key;
}

static unsigned char *DictKey()
{
    if (IsRegistered())
        return RegisteredKey();
    else
        return (unsigned char *)DemoKey;
}

static int MatchHash(const char *user, const char *email, 
			const char *rkey)
{
    const unsigned char *rvec = Key2Vector(rkey);
    const unsigned char *regkey = GetRegisteredKey(rvec, user, email);
    unsigned short h1 = ComputeHash(user, email, regkey); 
    unsigned short h2 = GetHash(rvec);
    return (h1 == h2);
}

//---------------------------------------------------------
// Public access

char *UserName()
{
    static char user[128] = { 0 };
    if (user[0]==0)
    {
        char *u=0;
        (void)wxGetResource("Registration", "User", &u, ini);
	strcpy(user, u ? u : "Unregistered");
	delete [] u;
    }
    return user;
}

char *EMail()
{
    static char email[128] = { 0 };
    if (email[0]==0)
    {
        char *e=0;
        (void)wxGetResource("Registration", "EMail", &e, ini);
	strcpy(email, e ? e : "Unregistered");
	delete [] e;
    }
    return email;
}

int IsRegistered()
{
    return MatchHash(UserName(), EMail(), RegKey());
}

long MaxDicSize()
{
    return 1000l * (IsRegistered() ? 150l : 30l);
}

int strcasecmp(char *s1, char *s2)
{
    while (*s1 && *s2)
    {
        char c1 = *s1++;
        char c2 = *s2++;
	if (isupper(c1)) c1 = tolower(c1);
	if (isupper(c2)) c2 = tolower(c2);
	if (c1 != c2) break;
    }
    return (*s1) - (*s2);
}

char *LoadDictionary(Dictionary *dict, char *dictname, unsigned char *key)
{
    int l = strlen(dictname);
    if (l<10 || strcasecmp(dictname+l-10, "wwdemo.dic") == 0)
        key = (unsigned char *)DemoKey;
    else if (key == 0)
        key = DictKey();
    return dict->Load(dictname, key);
}

char *MakeDictionary(Dictionary *&dict, char *dictname, unsigned char *key)
{
    if (dict) delete dict;
    dict = new Dictionary;
    char *rtn = LoadDictionary(dict, dictname, key);
    if (rtn)
    {
        delete dict;
	dict = 0;
    }
    return rtn;
}

int Register(char *user, char *email, char *key)
{
    if (!IsRegistered() && MatchHash(user, email, key))
    {
	SetUserName(user);
	SetEMail(email);
	SetRegKey(key);
	return 0;
    }
    return -1;
}


RegisterDlg::RegisterDlg(const char *ini_in)
    : GameDialog("Register"), ini(ini_in)
{
    user = new wxText(this, 0, "Name:");
    user->SetValue("XXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
    user->SetFocus();
    NewLine();
    email = new wxText(this, 0, "E-Mail:");
    email->SetValue("XXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
    NewLine();
    key = new wxText(this, 0, "Key:");
    key->SetValue("XXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
    NewLine();
    AddButton(1, "OK", 1);
    Tab();
    AddButton(0, "Cancel");
    Fit();
    user->SetValue(UserName());
    email->SetValue(EMail());
    key->SetValue(RegKey());
}

void RegisterDlg::HandleButton(int bnum)
{
    if (bnum==1)
    {
        char u[80];
	char e[128];
	char k[64];
	strcpy(u, user->GetValue());
	strcpy(e, email->GetValue());
	strcpy(k, key->GetValue());
	if (Register(u, e, k) == 0)
            (void)wxMessageBox("Success! Please exit and restart...", "Registration", wxOK|wxCENTRE);
	else
            (void)wxMessageBox("Invalid key!", "Registration", wxOK|wxCENTRE);
    }
    GameDialog::HandleButton(0);
}

RegisterDlg::~RegisterDlg()
{
}
