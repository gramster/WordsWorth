/*
 * File:	test.cc
 * Purpose:	Demo for toolbar class library
 * Author:	Julian Smart
 * Created:	1993
 * Updated:	
 * Copyright:	(c) 1993, AIAI, University of Edinburgh
 */

/* static const char sccsid[] = "%W% %G%"; */

// For compilers that support precompilation, includes "wx.h".
#include "wx_prec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx.h"
#endif

#include "wx_tbar.h"
#include "test.h"

// Declare two frames
MyFrame   *frame = NULL;
wxMenuBar *menu_bar = NULL;
wxIcon    *test_icon = NULL;

wxFrame *toolBarFrame = NULL;
TestToolBar *toolBar = NULL;
TestRibbon *toolRibbon = NULL;
wxBitmap *toolBarBitmaps[25];

// This statement initialises the whole application
MyApp     myApp;

#ifdef wx_x
#include "bitmaps/dirplain.xbm"
#include "bitmaps/draft.xbm"
#include "bitmaps/drawing.xbm"
#include "bitmaps/flowchar.xbm"
#include "bitmaps/write.xbm"
#endif

// The `main program' equivalent, creating the windows and returning the
// main frame
wxFrame *MyApp::OnInit(void)
{
  // Create the main frame window
  frame = new MyFrame(NULL, "Hello wxToolBar", 100, 100, 450, 300);

  // Give it a status line
  frame->CreateStatusLine();

  // Give it an icon
#ifdef wx_msw
  test_icon = new wxIcon("aiai_icn");
#endif
#ifdef wx_x
  test_icon = new wxIcon("aiai.xbm");
#endif
  frame->SetIcon(test_icon);

  // Make a menubar
  wxMenu *file_menu = new wxMenu;
  file_menu->Append(TEST_QUIT, "&Quit");

  wxMenu *help_menu = new wxMenu;
  help_menu->Append(TEST_ABOUT, "&About");

  menu_bar = new wxMenuBar;

  menu_bar->Append(file_menu, "&File");
  menu_bar->Append(help_menu, "&Help");

  // Associate the menu bar with the frame
  frame->SetMenuBar(menu_bar);

  frame->textWindow = new wxTextWindow(frame, 0, 0, -1, -1, wxNATIVE_IMPL);

  // Set up toolbar
#ifdef wx_msw
  toolBarBitmaps[0] = new wxBitmap("icon1");
  toolBarBitmaps[1] = new wxBitmap("icon2");
  toolBarBitmaps[2] = new wxBitmap("icon3");
  toolBarBitmaps[3] = new wxBitmap("icon4");
  toolBarBitmaps[4] = new wxBitmap("icon5");
  toolBarBitmaps[5] = new wxBitmap("icon6");
  toolBarBitmaps[6] = new wxBitmap("icon7");
  toolBarBitmaps[7] = new wxBitmap("icon8");

  toolBarBitmaps[10] = new wxBitmap("tool1");
  toolBarBitmaps[11] = new wxBitmap("tool2");
  toolBarBitmaps[12] = new wxBitmap("tool3");
  toolBarBitmaps[13] = new wxBitmap("tool4");
  toolBarBitmaps[14] = new wxBitmap("tool5");
  toolBarBitmaps[15] = new wxBitmap("tool6");
  toolBarBitmaps[16] = new wxBitmap("tool7");
  toolBarBitmaps[17] = new wxBitmap("tool8");
  toolBarBitmaps[18] = new wxBitmap("tool9");
  toolBarBitmaps[19] = new wxBitmap("tool10");
  toolBarBitmaps[20] = new wxBitmap("tool11");
  toolBarBitmaps[21] = new wxBitmap("tool12");
  toolBarBitmaps[22] = new wxBitmap("tool13");
  toolBarBitmaps[23] = new wxBitmap("tool14");
#endif
#ifdef wx_x
  toolBarBitmaps[0] = new wxBitmap(dirplain_bits, dirplain_width, dirplain_height);
  toolBarBitmaps[1] = new wxBitmap(draft_bits, draft_width, draft_height);
  toolBarBitmaps[2] = new wxBitmap(drawing_bits, drawing_width, drawing_height);
  toolBarBitmaps[3] = new wxBitmap(flowchart_bits, flowchart_width, flowchart_height);
  toolBarBitmaps[4] = new wxBitmap(write_bits, write_width, write_height);
  toolBarBitmaps[5] = new wxBitmap(write_bits, write_width, write_height);
  toolBarBitmaps[6] = new wxBitmap(write_bits, write_width, write_height);
  toolBarBitmaps[7] = new wxBitmap(write_bits, write_width, write_height);

  for (int j = 10; j < 24; j++)
    toolBarBitmaps[j] = new wxBitmap(write_bits, write_width, write_height);
#endif
  toolBarBitmaps[24] = new wxBitmap(32, 32);

  // Draw into the last bitmap.
  // This memory DC must be created dynamically, because only by deleting the DC
  // can we deselect the bitmap from the DC. Windows doesn't allow more than
  // one DC having the same bitmap selected.
  wxMemoryDC *memDC = new wxMemoryDC;
  memDC->SetUserScale(0.15, 0.15);
  memDC->SelectObject(toolBarBitmaps[24]);
  memDC->SetBackground(wxWHITE_BRUSH);

  memDC->Clear();
  memDC->SetPen(wxRED_PEN);
  memDC->SetBrush(wxBLUE_BRUSH);
  memDC->DrawLine(0, 0, 200, 200);
  memDC->DrawLine(200, 0, 0, 200);
  memDC->DrawEllipse(250, 250, 100, 50);

  delete memDC;

  toolBarFrame = new wxFrame(frame, "Tools", 0, 0, 300, 200,
    wxSDI | wxSYSTEM_MENU | wxCAPTION | wxTHICK_FRAME);
  toolBar = new TestToolBar(toolBarFrame, 10, 10, -1, -1, 0, wxVERTICAL, 5);
  toolBar->SetMargins(2, 2);
  toolBar->GetDC()->SetBackground(wxGREY_BRUSH);
  int i;
  for (i = 10; i < 25; i++)
    toolBar->AddTool(i, toolBarBitmaps[i], NULL, TRUE, -1, -1, NULL);

  toolBar->Layout();
  float maxWidth, maxHeight;
  toolBar->GetMaxSize(&maxWidth, &maxHeight);
  toolBarFrame->SetClientSize((int)maxWidth, (int)maxHeight);
  toolBarFrame->Show(TRUE);

  toolRibbon = new TestRibbon(frame, 0, 0, -1, -1,
     wxTB_3DBUTTONS, // 3D buttons, auto-layout
//   0,              // No 3D buttons
    wxVERTICAL, 1);
  toolRibbon->SetMargins(5, 5);
  toolRibbon->GetDC()->SetBackground(wxGREY_BRUSH);
  frame->SetToolBar(toolRibbon);

  int width = toolBarBitmaps[0]->GetWidth();
  int currentX = 5;

  toolRibbon->AddTool(0, toolBarBitmaps[0]);
  currentX += width + 5;
  toolRibbon->AddTool(1, toolBarBitmaps[1]);
  currentX += width + 5;
  toolRibbon->AddTool(2, toolBarBitmaps[2]);
  toolRibbon->AddSeparator();
  currentX += width + 15;
  toolRibbon->AddTool(3, toolBarBitmaps[3]);
  currentX += width + 5;
  toolRibbon->AddTool(4, toolBarBitmaps[4]);
  currentX += width + 5;
  toolRibbon->AddTool(5, toolBarBitmaps[5]);
  currentX += width + 5;
  toolRibbon->AddTool(6, toolBarBitmaps[6]);
  currentX += width + 15;
  toolRibbon->AddSeparator();
  toolRibbon->AddTool(7, toolBarBitmaps[7]);
  
  toolRibbon->Layout();

  frame->OnSize(-1, -1);
  frame->Show(TRUE);

  frame->SetStatusText("Hello, wxWindows");
  
  // Return the main frame window
  return frame;
}

// Toolbar-handling frame constructor
wxFrameWithToolBar::wxFrameWithToolBar(wxFrame *frame, char *title, int x, int y, int w, int h,
  long style):
  wxFrame(frame, title, x, y, w, h, style)
{
  frameToolBar = NULL;
}

// Reposition the toolbar and child subwindow
void wxFrameWithToolBar::OnSize(int w, int h)
{
  float maxToolBarWidth = 0.0, maxToolBarHeight = 0.0;
  if (frameToolBar)
    frameToolBar->GetMaxSize(&maxToolBarWidth, &maxToolBarHeight);
  else
  {
    wxFrame::OnSize(w, h);
    return;
  }

  // Find the frame's child (assuming there is only one)
  wxWindow *child = NULL;
  wxNode *node = GetChildren()->First();
  while (node && !child)
  {
    wxWindow *win = (wxWindow *)node->Data();
    if ((win != frameToolBar) &&
        (win->IsKindOf(CLASSINFO(wxPanel)) ||
         win->IsKindOf(CLASSINFO(wxTextWindow)) ||
         win->IsKindOf(CLASSINFO(wxCanvas))))
      child = win;

    node = node->Next();
  }
  if (!child)
    return;

  int frameWidth, frameHeight;
  GetClientSize(&frameWidth, &frameHeight);

  child->SetSize(0, (int)maxToolBarHeight, (int)frameWidth, (int)(frameHeight - maxToolBarHeight));
  frameToolBar->SetSize(0, 0, (int)frameWidth, (int)maxToolBarHeight);
}

// Define my frame constructor
MyFrame::MyFrame(wxFrame *frame, char *title, int x, int y, int w, int h):
  wxFrameWithToolBar(frame, title, x, y, w, h, wxSDI|wxDEFAULT_FRAME)
{
  textWindow = NULL;
}

// Intercept menu commands
void MyFrame::OnMenuCommand(int id)
{
  switch (id)
  {
    case TEST_QUIT:
    {
      OnClose();
      delete this;
      break;
    }
    case TEST_ABOUT:
    {
      (void)wxMessageBox("wxWindows toolbar demo\nAuthor: Julian Smart J.Smart@ed.ac.uk\nAIAI (c) 1996", "About wxHello");
      break;
    }
  }
}

// Intercept menu item selection - only has an effect in Windows
void MyFrame::OnMenuSelect(int id)
{
  char *msg = NULL;
  switch (id)
  {
    case TEST_QUIT:
      msg = "Quit program";
      break;
    case -1:
      msg = "";
      break;
  }
  if (msg)
    frame->SetStatusText(msg);
}

// Define the behaviour for the frame closing
// - must delete all frames except for the main one.
Bool MyFrame::OnClose(void)
{
  delete toolBarFrame;
  return TRUE;
}

TestToolBar::TestToolBar(wxFrame *frame, int x, int y, int w, int h,
            long style, int direction, int RowsOrColumns):
  wxToolBar(frame, x, y, w, h, style, direction, RowsOrColumns)
{
}

Bool TestToolBar::OnLeftClick(int toolIndex, Bool toggled)
{
  char buf[200];
  sprintf(buf, "Clicked on tool %d", toolIndex);
  frame->SetStatusText(buf);
  return TRUE;
}

void TestToolBar::OnMouseEnter(int toolIndex)
{
  char buf[200];
  if (toolIndex > -1)
  {
    sprintf(buf, "This is tool number %d", toolIndex);
    frame->SetStatusText(buf);
  }
  else frame->SetStatusText("");
}

TestRibbon::TestRibbon(wxFrame *frame, int x, int y, int w, int h,
            long style, int direction, int RowsOrColumns):
  wxToolBar(frame, x, y, w, h, style, direction, RowsOrColumns)
{
}

Bool TestRibbon::OnLeftClick(int toolIndex, Bool toggled)
{
  char buf[200];
  sprintf(buf, "Clicked on tool %d", toolIndex);
  frame->SetStatusText(buf);
  return TRUE;
}

void TestRibbon::OnMouseEnter(int toolIndex)
{
  char buf[200];
  if (toolIndex > -1)
  {
    sprintf(buf, "This is tool number %d", toolIndex);
    frame->SetStatusText(buf);
  }
  else frame->SetStatusText("");
}
