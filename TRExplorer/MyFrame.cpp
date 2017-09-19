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

#include "MyFrame.h"

///////////////////////////////////////////////////////////////////////////

MyFrame::MyFrame(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(parent, id, title, pos, size, style)
{
	this->SetSizeHints(wxSize(800, 600), wxDefaultSize);
	m_mgr.SetManagedWindow(this);
	m_mgr.SetFlags(wxAUI_MGR_DEFAULT);

	m_statusBar2 = this->CreateStatusBar(1, wxST_SIZEGRIP, wxID_ANY);
	m_menubar2 = new wxMenuBar(0);
	m_fileMenu = new wxMenu();
	wxMenuItem* m_menuOpen;
	m_menuOpen = new wxMenuItem(m_fileMenu, wxID_OPEN, wxString(wxT("Open")) + wxT('\t') + wxT("CTRL+O"), wxT("Open TIGER file for processing"), wxITEM_NORMAL);
	m_fileMenu->Append(m_menuOpen);

	wxMenuItem* m_menuClose;
	m_menuClose = new wxMenuItem(m_fileMenu, wxID_EXIT, wxString(wxT("Close")), wxT("Close"), wxITEM_NORMAL);
	m_fileMenu->Append(m_menuClose);

	m_menubar2->Append(m_fileMenu, wxT("File"));

	m_helpMenu = new wxMenu();
	wxMenuItem* m_menuAbout;
	m_menuAbout = new wxMenuItem(m_helpMenu, wxID_ABOUT, wxString(wxT("About")), wxT("About"), wxITEM_NORMAL);
	m_helpMenu->Append(m_menuAbout);

	m_menubar2->Append(m_helpMenu, wxT("Help"));

	this->SetMenuBar(m_menubar2);

	m_NavigationPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(200, -1), wxTAB_TRAVERSAL);
	m_NavigationPanel->SetMinSize(wxSize(200, -1));

	m_mgr.AddPane(m_NavigationPanel, wxAuiPaneInfo().Left().CaptionVisible(false).PinButton(true).Dock().Resizable().FloatingSize(wxDefaultSize));

	wxBoxSizer* bNavigationSizer;
	bNavigationSizer = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* bSearchSizer;
	bSearchSizer = new wxBoxSizer(wxHORIZONTAL);

	m_txtSearch = new wxTextCtrl(m_NavigationPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	m_txtSearch->SetMaxLength(256);
	m_txtSearch->SetMinSize(wxSize(150, -1));

	bSearchSizer->Add(m_txtSearch, 1, 0, 5);

	m_btnPrev = new wxButton(m_NavigationPanel, wxID_FindPrevButton, wxT("<"), wxDefaultPosition, wxSize(20, -1), 0);
	bSearchSizer->Add(m_btnPrev, 1, 0, 5);

	m_btnNext = new wxButton(m_NavigationPanel, wxID_FindNextButton, wxT(">"), wxDefaultPosition, wxSize(20, -1), 0);
	bSearchSizer->Add(m_btnNext, 1, 0, 5);


	bNavigationSizer->Add(bSearchSizer, 1, 0, 5);

	m_NavigationTree = new wxTreeCtrl(m_NavigationPanel, wxID_NavigationTree, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE);
	m_NavigationTree->SetMinSize(wxSize(-1, 200));

	bNavigationSizer->Add(m_NavigationTree, 100, wxEXPAND, 5);


	m_NavigationPanel->SetSizer(bNavigationSizer);
	m_NavigationPanel->Layout();

	m_thumbnailList = new wxThumbnailCtrl(this, wxID_ThumbnailList);

	m_mgr.AddPane(m_thumbnailList, wxAuiPaneInfo().Center().CaptionVisible(false).CloseButton(false).PaneBorder(false).Dock().Resizable().FloatingSize(wxDefaultSize).Row(1));

	m_RenderPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL);
	m_mgr.AddPane(m_RenderPanel, wxAuiPaneInfo().Right().CaptionVisible(false).CloseButton(false).PaneBorder(false).Dock().Resizable().FloatingSize(wxDefaultSize).Row(2).Position(0).MinSize(wxSize(150, 300)));


	m_mgr.Update();
	this->Centre(wxBOTH);
}

MyFrame::~MyFrame()
{
	m_mgr.UnInit();

}
