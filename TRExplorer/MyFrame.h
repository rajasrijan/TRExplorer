/*
MIT License

Copyright (c) 2017 Srijan Kumar Sharma

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef __NONAME_H__
#define __NONAME_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/statusbr.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/srchctrl.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/treectrl.h>
#include <wx/panel.h>
#include <wx/frame.h>
#include <wx/aui/aui.h>
#include <wx/wxprec.h>
#include <wx/dataview.h>
#include <wx/imaglist.h>
#include <wx/statbmp.h>
#include <wx/app.h>
#include "thumbnailctrl.h"

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
/// WindowID Enum
///////////////////////////////////////////////////////////////////////////////
enum
{
	wxID_NavigationTree,
	wxID_ThumbnailList,
	wxID_Search
};
///////////////////////////////////////////////////////////////////////////////
/// Class MyFrame
///////////////////////////////////////////////////////////////////////////////
class MyFrame : public wxFrame
{
private:

protected:
	wxStatusBar* m_statusBar2;
	wxMenuBar* m_menubar2;
	wxMenu* m_fileMenu;
	wxMenu* m_helpMenu;
	wxPanel* m_NavigationPanel;
	wxSearchCtrl* m_txtSearch;
	wxTreeCtrl* m_NavigationTree;
	wxPanel* m_RenderPanel;
	wxThumbnailCtrl *m_thumbnailList;

public:

	MyFrame(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("TRExplorer"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(800, 600), long style = wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL);
	wxAuiManager m_mgr;

	~MyFrame();

};

#endif //__NONAME_H__
