#ifndef WWINST_H
#define WWINST_H

#include "dflatpp.h"

#define Df void (DFWindow::*)()
#define Ap void (Application::*)()

// ------- WWInst application definition

class WWInst : public Application
{
	  PushButton *instBtn, *exitBtn, *setupBtn, *demoBtn;
    void _CmInstall(char *zipname);
public:
    WWInst();
    ~WWInst();
    void CmInstall();
    void CmInstDemo();
    void CmSetup();
    void CmExit()   { CloseWindow(); }
};

#endif
