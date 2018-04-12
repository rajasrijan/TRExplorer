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
#pragma once
#include <wx/fileconf.h>
#include "MyFrame.h"
#include "patch.h"
#include "element.h"
#include <string>

class GUI : public MyFrame
{
  public:
	GUI(wxWindow *parent);
	~GUI();

  private:
	patch *m_pTigerFile;
	std::vector<std::pair<std::string, wxTreeItemId>> m_vNodeList;
	std::vector<std::pair<std::string, wxTreeItemId>>::iterator m_itNodeSearchPos;
	wxMenu *p_PopUp;
	wxFileConfig g_mConfigFile;

  private:
	void OnExit(wxCommandEvent &event);
	void OnAbout(wxCommandEvent &event);
	void OnOpenFile(wxCommandEvent &event);
	void OnTreeSelectionChanged(wxTreeEvent &event);
	void OnSearchEnter(wxCommandEvent &event);
	void OnSearch(wxCommandEvent &event);
	void OnThumbnailListContextMenu(wxContextMenuEvent &event);
	void OnThumbContextMenuSelected(wxContextMenuEvent &event);
	void SearchInTree(const std::string &search_term);
	void ExportSelection();
	void ImportSelection();
	void ExportSelectionRaw();
	void ImportSelectionRaw();
	wxDECLARE_EVENT_TABLE();
};

enum POPUP_OPTIONS
{
	ID_Export = 0,
	ID_ExportRaw,
	ID_Import,
	ID_ImportRaw,
};
class MyApp : public wxApp
{
	GUI *frame;

  public:
	MyApp();
	~MyApp();
	virtual bool OnInit();
	virtual int OnExit();
};

class myTreeItemData : public wxTreeItemData
{
  public:
	myTreeItemData(const element_t &_ele) : m_itemElement(_ele), pList(nullptr)
	{
	}
	~myTreeItemData()
	{
		delete pList;
		pList = nullptr;
	}
	element_t m_itemElement;
	std::vector<void *> *pList;
};
