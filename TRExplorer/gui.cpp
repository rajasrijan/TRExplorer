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
#include "gui.h"
#define NOMINMAX
#include <windows.h>

#include <assert.h>
#include <stdio.h>
#include <dxgiformat.h>
#include <d3d11.h>
#include <algorithm>
#include <directxmath.h>
#include "DirectXTex.h"
#include <thread>
#include <mutex>

#pragma comment(lib,"DirectXTex.lib")

using namespace DirectX;

wxBEGIN_EVENT_TABLE(GUI, wxFrame)
EVT_MENU(wxID_OPEN, GUI::OnOpenFile)
EVT_MENU(wxID_EXIT, GUI::OnExit)
EVT_MENU(wxID_ABOUT, GUI::OnAbout)
EVT_TREE_SEL_CHANGED(wxID_NavigationTree, GUI::OnTreeSelectionChanged)
EVT_COMMAND_CONTEXT_MENU(wxID_ThumbnailList, GUI::OnThumbnailListContextMenu)
EVT_BUTTON(wxID_FindNextButton, GUI::OnFindNext)
EVT_BUTTON(wxID_FindPrevButton, GUI::OnFindPrev)
wxEND_EVENT_TABLE()
wxIMPLEMENT_APP_NO_MAIN(MyApp);

MyApp::MyApp() :frame(nullptr)
{
}

bool MyApp::OnInit()
{
    frame = new GUI(NULL);
    frame->Show(true);
    return true;
}

int MyApp::OnExit()
{
    return 0;
}

void GUI::OnAbout(wxCommandEvent& event)
{
    wxMessageBox("Editor by rajasrijan",
        "About", wxOK | wxICON_INFORMATION);
}

void GUI::OnOpenFile(wxCommandEvent & event)
{
    if (m_pTigerFile)
    {
        return;
    }
    string m_pTigerFilePath;
    wxString gameDir = wxEmptyString;
    g_mConfigFile.Read("GameDirectory", &gameDir);

    wxFileDialog m_pTigerFileDIalog(this, wxFileSelectorPromptStr, gameDir, "bigfile.000.tiger", "TIGER File|*.tiger", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (m_pTigerFileDIalog.ShowModal() != wxID_OK)
    {
        return;
    }
    g_mConfigFile.Write("GameDirectory", m_pTigerFileDIalog.GetDirectory());
    g_mConfigFile.Flush();
    m_pTigerFilePath = m_pTigerFileDIalog.GetPath();
    if (m_pTigerFile)
    {
        delete m_pTigerFile;
    }
    m_pTigerFile = new patch(m_pTigerFilePath);

    wxTreeItemId rootId = m_NavigationTree->AddRoot("ROOT");

    m_NavigationTree->Freeze();
    std::mutex mtxNodeList;
    const size_t ElementCount = m_pTigerFile->getElementCount();
    for (size_t i = 0; i < ElementCount; i++)
    {
        myTreeItemData *pData = new myTreeItemData(m_pTigerFile->getElement(i));
        auto itemId = m_NavigationTree->AppendItem(rootId, pData->m_itemElement.getName(), -1, -1, pData);
        {
            std::lock_guard<mutex> lock(mtxNodeList);
            m_vNodeList.push_back(itemId);
        }
    }
    m_itNodeSearchPos = m_vNodeList.begin();
    m_NavigationTree->Thaw();
}

void GUI::OnTreeSelectionChanged(wxTreeEvent & event)
{
    int ret = 0;
    HRESULT hr = S_OK;
    wxTreeItemId selectedItem = event.GetItem();
    myTreeItemData *pData = (myTreeItemData*)m_NavigationTree->GetItemData(selectedItem);
    if (!pData)
    {
        return;
    }
    m_itNodeSearchPos = find(m_vNodeList.begin(), m_vNodeList.end(), selectedItem);
    if (!pData->pList)
    {
        pData->pList = new vector<void*>;
        m_pTigerFile->getUHList(pData->m_itemElement, pData->pList);
    }
    if (pData->pList->size() == 0)
    {
        return;
    }

    m_thumbnailList->Clear();
    m_thumbnailList->Freeze();

    for (auto unknown_header : *(pData->pList))
    {
        char *pDataU = nullptr;
        shared_ptr<char> aData;
        size_t sz = 0;
        size_t szOut = 0;
        CDRM_TYPES type = CDRM_TYPE_UNKNOWN;
        ret = m_pTigerFile->uncompressCDRM(unknown_header, aData, sz);
        if (ret)
        {
            continue;
        }
        ret = m_pTigerFile->getDataType(aData.get(), sz, type);
        if (ret || type == CDRM_TYPE_UNKNOWN)
        {
            /*char thumbLabel[MAX_PATH] = { 0 };
            sprintf_s(thumbLabel, "%08X", *(uint32_t*)aData.get());
            m_thumbnailList->Append(new wxImageThumbnailItem("NA.BMP", unknown_header, thumbLabel));
            */
            continue;
        }
        else if (type == CDRM_TYPE_DDS)
        {
            ret = m_pTigerFile->decodeData(aData.get(), sz, &pDataU, szOut, type);
            m_thumbnailList->Append(new wxImageThumbnailItem(shared_ptr<char>(pDataU), szOut, unknown_header));
            continue;
        }
        else if (type == CDRM_TYPE_MESH)
        {
            m_thumbnailList->Append(new wxImageThumbnailItem("MESH.BMP", unknown_header));
            continue;
        }
        else
        {
            //fstream drm(to_string(cdrmIndex) + ".cdrm", ios_base::binary | ios_base::out);
            //drm.write(aData.get(), sz);
            continue;
        }
    }
    m_thumbnailList->Thaw();
}

void GUI::OnFindNext(wxCommandEvent & event)
{
    string sSearchString;
    string sPathName;
    if (m_vNodeList.empty())
    {
        return;
    }
    sSearchString = m_txtSearch->GetLineText(0);
    if (sSearchString.empty())
    {
        return;
    }
    transform(sSearchString.begin(), sSearchString.end(), sSearchString.begin(), ::tolower);
    m_NavigationTree->Freeze();
    auto itFinder = m_itNodeSearchPos;
    do
    {
        sPathName = m_NavigationTree->GetItemText(*itFinder);
        transform(sPathName.begin(), sPathName.end(), sPathName.begin(), ::tolower);
        if (strstr(sPathName.c_str(), sSearchString.c_str()))
        {
            m_NavigationTree->SelectItem(*itFinder, true);
            m_NavigationTree->ScrollTo(*itFinder);
            m_itNodeSearchPos = itFinder + 1;
            if (m_itNodeSearchPos == m_vNodeList.end())
            {
                m_itNodeSearchPos = m_vNodeList.begin();
            }
            break;
        }
        itFinder++;
        if (itFinder == m_vNodeList.end())
        {
            itFinder = m_vNodeList.begin();
        }
    } while (itFinder != m_itNodeSearchPos);
    m_NavigationTree->Thaw();
}

void GUI::OnFindPrev(wxCommandEvent & event)
{
    wxMessageBox("Not Implimented");
}

void GUI::OnThumbnailListContextMenu(wxContextMenuEvent & event)
{
    event.Skip();
    if (p_PopUp)
    {
        PopupMenu(p_PopUp);
    }
}

void GUI::OnThumbContextMenuSelected(wxContextMenuEvent & event)
{
    POPUP_OPTIONS menuId = (POPUP_OPTIONS)event.GetId();

    switch (menuId)
    {
    case ID_Export:
        ExportSelection();
        break;
    case ID_ExportRaw:
        ExportSelectionRaw();
        break;
    case ID_Import:
        ImportSelection();
        break;
    case ID_ImportRaw:
        ImportSelectionRaw();
        break;
    default:
        break;
    }
}

void GUI::ExportSelection()
{
    wxFileDialog saveDialog(this, wxFileSelectorPromptStr, wxEmptyString, wxEmptyString, wxFileSelectorDefaultWildcardStr, wxFD_SAVE);
    int ret = 0;
    if ((ret = saveDialog.ShowModal()) != wxID_OK)
    {
        return;
    }
    string saveFileName = saveDialog.GetPath();
    wxImageThumbnailItem *item = (wxImageThumbnailItem*)m_thumbnailList->GetSelected();
    ret = m_pTigerFile->decodeAndSaveToFile(item->getItemData(), saveFileName);
    if (ret)
    {
        wxMessageBox("Failed to export data");
    }
    else
    {
        wxMessageBox("File exported to [" + saveFileName + "]");
    }
}

void GUI::ImportSelection()
{
    if (m_thumbnailList->GetSelection() == -1)
    {
        return;
    }
    int ret = 0;
#ifndef _DEBUG
    ret = wxMessageBox("Importing arbitrary data can break game.\nContinue at your own risk?",
        "CONTINUE AT YOUR OWN RISK!!", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING);
    if (ret != wxYES)
    {
        return;
    }
#endif // !_DEBUG
    wxFileDialog openFile(this);
    if (openFile.ShowModal() != wxID_OK)
    {
        return;
    }
    string importFileName = openFile.GetPath();
    wxImageThumbnailItem *item = (wxImageThumbnailItem*)m_thumbnailList->GetSelected();
    ret = m_pTigerFile->encodeAndCompress(item->getItemData(), importFileName);
    if (ret)
    {
        wxMessageBox("Failed to import data");
    }
    else
    {
        wxMessageBox("File imported from [" + importFileName + "]");
    }
}

void GUI::ImportSelectionRaw()
{
    if (m_thumbnailList->GetSelection() == -1)
    {
        return;
    }
    int ret = 0;
    shared_ptr<char> pRawData;
#ifndef _DEBUG
    ret = wxMessageBox("Importing arbitrary data can break game.\nContinue at your own risk?",
        "CONTINUE AT YOUR OWN RISK!!", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING);
    if (ret != wxYES)
    {
        return;
    }
#endif // !_DEBUG
    wxFileDialog openFile(this);
    if (openFile.ShowModal() != wxID_OK)
    {
        return;
    }
    string importFileName = openFile.GetPath();
    wxImageThumbnailItem *item = (wxImageThumbnailItem*)m_thumbnailList->GetSelected();
    //ret = m_pTigerFile->encodeAndCompress(item->getItemData(), importFileName);
    HANDLE hFile = CreateFileA(importFileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        printf("Unable to open file");
        ret = 1;
        goto exit;
    }
    //	get file size
    DWORD szFile = GetFileSize(hFile, nullptr);
    // allocate sufficient memory.
    pRawData = shared_ptr<char>(new char[szFile]);
    DWORD bytesRead = 0;
    ret = (int)ReadFile(hFile, pRawData.get(), (DWORD)szFile, &bytesRead, NULL);
    if (!ret)
    {
        printf("Unable to write file");
        ret = GetLastError();
        goto exit;
    }
    else
    {
        ret = 0;
    }
    ret = m_pTigerFile->compressCDRM(item->getItemData(), pRawData.get(), bytesRead);
exit:
    if (ret)
    {
        wxMessageBox("Failed to import data");
    }
    else
    {
        wxMessageBox("File imported from [" + importFileName + "]");
    }
}

void GUI::ExportSelectionRaw()
{
    wxFileDialog saveDialog(this, wxFileSelectorPromptStr, wxEmptyString, wxEmptyString, wxFileSelectorDefaultWildcardStr, wxFD_SAVE);
    int ret = 0;
    if ((ret = saveDialog.ShowModal()) != wxID_OK)
    {
        return;
    }
    string saveFileName = saveDialog.GetPath();
    wxImageThumbnailItem *item = (wxImageThumbnailItem*)m_thumbnailList->GetSelected();
    size_t sz = 0;
    shared_ptr<char> aData;
    ret = m_pTigerFile->uncompressCDRM(item->getItemData(), aData, sz);
    if (ret)
    {
        wxMessageBox("Failed to decompress cdrm data");
        return;
    }
    fstream saveFile(saveFileName, ios_base::binary | ios_base::out);
    if (!saveFile.is_open())
    {
        wxMessageBox("Failed to open save file");
        return;
    }
    saveFile.write(aData.get(), sz);
    saveFile.close();
    wxMessageBox("File exported to [" + saveFileName + "]");
}

GUI::GUI(wxWindow * parent) : MyFrame(parent), m_pTigerFile(nullptr), p_PopUp(nullptr), g_mConfigFile("TRExplorer", "", "config.ini")
{
    p_PopUp = new wxMenu();
    p_PopUp->Append(ID_Export, wxT("Export"));
    p_PopUp->Append(ID_ExportRaw, wxT("Export Raw"));
    p_PopUp->AppendSeparator();
    p_PopUp->Append(ID_Import, wxT("Import"));
    p_PopUp->Append(ID_ImportRaw, wxT("Import Raw"));
    p_PopUp->Connect(wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)(&GUI::OnThumbContextMenuSelected), nullptr, this);
}

GUI::~GUI()
{
    if (m_pTigerFile)
    {
        delete m_pTigerFile;
        m_pTigerFile = nullptr;
    }
}

void GUI::OnExit(wxCommandEvent & event)
{
    if (m_pTigerFile)
    {
        delete m_pTigerFile;
        m_pTigerFile = nullptr;
    }
    Close(true);
}
