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
#include <string>
#include <wx\fileconf.h>
#include "MyFrame.h"
#include "patch.h"
#include "element.h"

using namespace std;

class GUI :public MyFrame
{
public:
	GUI(wxWindow* parent);
	~GUI();
private:
	patch *m_pTigerFile;
	vector<wxTreeItemId> m_vNodeList;
	vector<wxTreeItemId>::iterator m_itNodeSearchPos;
	wxMenu *p_PopUp;
	wxFileConfig g_mConfigFile;
private:
	void OnExit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnOpenFile(wxCommandEvent & event);
	void OnTreeSelectionChanged(wxTreeEvent & event);
	void OnFindNext(wxCommandEvent & event);
	void OnFindPrev(wxCommandEvent & event);
	void OnThumbnailListContextMenu(wxContextMenuEvent & event);
	void OnThumbContextMenuSelected(wxContextMenuEvent & event);
	void ExportSelection();
	void ImportSelection();
	void ExportSelectionRaw();
	wxDECLARE_EVENT_TABLE();
};

enum POPUP_OPTIONS
{
	ID_Export = 0,
	ID_ExportRaw,
	ID_Import,
};
class MyApp : public wxApp
{
	GUI *frame;
public:
	MyApp();
	virtual bool OnInit();
	virtual int OnExit();
};

class myTreeItemData :public wxTreeItemData
{
public:
	myTreeItemData(const element_t &_ele) :m_itemElement(_ele), pList(nullptr)
	{}
	~myTreeItemData()
	{
		delete pList;
		pList = nullptr;
	}
	element_t m_itemElement;
	vector<void*>* pList;
};