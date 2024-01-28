/*******************************************************************************
* File: PMT Viewer.cpp
*
* Description: main file in project.
*
* Copyright (c) Ditenbir Pavel, 2007.
*
*******************************************************************************/

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include "Resource.h"
#include "TransportStream.h"


// Used by PMSNavigate function.
enum Navigation {first, last, prev, next};


int WINAPI     WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
BOOL CALLBACK  DlgProcMain(HWND, UINT, WPARAM, LPARAM);

void PMSNavigate(HWND hwndDlg, CTransportStream& TS, Navigation navigation);
void ShowPMSInfo(HWND hwndDlg, const PM_SECTION* pPMS, UINT uPMSNum, UINT uPacketNum);
void ResetAllControls(HWND hwndDlg);


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, DlgProcMain);
	return 0;
}

BOOL CALLBACK DlgProcMain(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static CTransportStream s_TS;

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			// set dialog icon
			HICON hIcon = LoadIcon(NULL, IDI_INFORMATION);
			SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

			ResetAllControls(hwndDlg);

			return TRUE;
		}

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDB_OPEN:
					if (HIWORD(wParam) == BN_CLICKED)
					{
						// user pushed "open" button, prepare OPENFILENAME
						// structure and show standard dialog box for choosing file

						TCHAR szFileName[MAX_PATH] = {0};

						OPENFILENAME ofn = {0};
						ofn.lStructSize  = sizeof(ofn);
						ofn.hwndOwner    = hwndDlg;
						ofn.lpstrFilter  = TEXT("MPEG files (*.mpg, *.mpeg)\0*.mpg;*.mpeg\0"
												"MPEG Transport Stream files (*.ts)\0*.ts\0"
												"All files (*.*)\0*.*\0");
						ofn.nFilterIndex = 2;
						ofn.lpstrFile    = szFileName;
						ofn.nMaxFile     = sizeof(szFileName) / sizeof(szFileName[0]);
						ofn.lpstrTitle   = TEXT("Open media file");
						ofn.Flags        = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

						if (GetOpenFileName(&ofn))
						{
							s_TS.Close();
							ResetAllControls(hwndDlg);

							if (!s_TS.Open(szFileName))
							{
								// file not opened
								MessageBox(hwndDlg, TEXT("File not opened."), TEXT("PMT Viewer"), MB_OK | MB_ICONERROR);
								break;
							}

							if (!s_TS.IsMPEG2TS())
							{
								// file is not a MPEG-2 Transport Stream; so close it
								MessageBox(hwndDlg,
									TEXT("File is not MPEG-2 Transport Stream or some packets are incorrect. File will be closed."),
									TEXT("PMT Viewer"), MB_OK | MB_ICONWARNING);
								s_TS.Close();
								break;
							}

							if (!s_TS.GetPMSCount())
							{
								// there is no PM Sections in file
								MessageBox(hwndDlg,
									TEXT("File doesn't contains Program Map Sections and will be closed."),
									TEXT("PMT Viewer"), MB_OK | MB_ICONWARNING);
								s_TS.Close();
								break;
							}

							SetDlgItemText(hwndDlg, IDE_FILE, s_TS.GetFileName());

							PMSNavigate(hwndDlg, s_TS, first);
						}
					}
					break;

				case IDB_FIRST:
					if (HIWORD(wParam) == BN_CLICKED)
						PMSNavigate(hwndDlg, s_TS, first);

					break;

				case IDB_NEXT:
					if (HIWORD(wParam) == BN_CLICKED)
						PMSNavigate(hwndDlg, s_TS, next);

					break;

				case IDB_PREVIOUS:
					if (HIWORD(wParam) == BN_CLICKED)
						PMSNavigate(hwndDlg, s_TS, prev);

					break;

				case IDB_LAST:
					if (HIWORD(wParam) == BN_CLICKED)
						PMSNavigate(hwndDlg, s_TS, last);

					break;
			}
			return TRUE;

		case WM_CLOSE:
			s_TS.Close();
			EndDialog(hwndDlg, 0);
			return FALSE;
	}

	return FALSE;
}

//
// PMSNavigate
//
// Movement beetween PM Sections in TS and enable or disable appropriate buttons.
void PMSNavigate(HWND hwndDlg, CTransportStream& TS, Navigation navigation)
{
	static UINT s_uCurPMS = 0; // current PMS number

	UINT uPMS = 0;
	PM_SECTION PMS;
 
	switch (navigation)
	{
		case first:
			if (UINT uNum = TS.GetFirstPMSection(&PMS, &uPMS))
			{
				s_uCurPMS = 1;
				ShowPMSInfo(hwndDlg, &PMS, uPMS, uNum);
			}
			else
				return;

			break;

		case last:
			if (UINT uNum = TS.GetLastPMSection(&PMS, &uPMS))
			{
				s_uCurPMS = TS.GetPMSCount();
				ShowPMSInfo(hwndDlg, &PMS, uPMS, uNum);
			}
			else
				return;

			break;

		case prev:
			if (UINT uNum = TS.GetPrevPMSection(&PMS, &uPMS))
			{
				s_uCurPMS--;
				ShowPMSInfo(hwndDlg, &PMS, uPMS, uNum);
			}
			else
				return;

			break;

		case next:
			if (UINT uNum = TS.GetNextPMSection(&PMS, &uPMS))
			{
				s_uCurPMS++;
				ShowPMSInfo(hwndDlg, &PMS, uPMS, uNum);
			}
			else
				return;

			break;

		default:
			return;
	}

	BOOL fBtnFirst = TRUE,
		 fBtnPrev  = TRUE,
		 fBtnNext  = TRUE,
		 fBtnLast  = TRUE;

	if (s_uCurPMS <= 1)
		fBtnFirst = fBtnPrev = FALSE;
	if (s_uCurPMS >= TS.GetPMSCount())
		fBtnNext = fBtnLast = FALSE;

	EnableWindow(GetDlgItem(hwndDlg, IDB_FIRST), fBtnFirst);
	EnableWindow(GetDlgItem(hwndDlg, IDB_PREVIOUS), fBtnPrev);
	EnableWindow(GetDlgItem(hwndDlg, IDB_NEXT), fBtnNext);
	EnableWindow(GetDlgItem(hwndDlg, IDB_LAST), fBtnLast);
}

//
// ShowPMSInfo
//
void ShowPMSInfo(HWND hwndDlg, const PM_SECTION* pPMS, UINT uPMSNum, UINT uPacketNum)
{
	TCHAR sz[256 * 5 + 101] = {0};
	wsprintf(sz, TEXT("Program Map Section #%u (Packet #%u)"), uPMSNum, uPacketNum);
	SetDlgItemText(hwndDlg, IDC_TITLE, sz);

	SetDlgItemInt (hwndDlg, IDE_TABLE_ID, pPMS->table_id, FALSE);
	SetDlgItemText(hwndDlg, IDE_SECTION_SYNTAX_INDICATOR, pPMS->section_syntax_indicator ? TEXT("Yes") : TEXT("No"));
	SetDlgItemInt (hwndDlg, IDE_SECTION_LENGTH, pPMS->section_length, FALSE);
	SetDlgItemInt (hwndDlg, IDE_PROGRAM_NUMBER, pPMS->program_number, FALSE);
	SetDlgItemInt (hwndDlg, IDE_VERSION_NUMBER, pPMS->version_number, FALSE);
	SetDlgItemText(hwndDlg, IDE_CURRENT_NEXT_INDICATOR, pPMS->current_next_indicator ? TEXT("Yes") : TEXT("No"));
	SetDlgItemInt (hwndDlg, IDE_SECTION_NUMBER, pPMS->section_number, FALSE);
	SetDlgItemInt (hwndDlg, IDE_LAST_SECTION_NUMBER, pPMS->last_section_number, FALSE);
	SetDlgItemInt (hwndDlg, IDE_PCR_PID, pPMS->PCR_PID, FALSE);
	SetDlgItemInt (hwndDlg, IDE_PROGRAM_INFO_LENGTH, pPMS->program_info_length, FALSE);
	SetDlgItemInt (hwndDlg, IDE_CRC, pPMS->CRC_32, FALSE);

	// Fill the list box with the values of program descriptors

	SendDlgItemMessage(hwndDlg, IDC_PROGRAM_DESCRIPTORS, LB_RESETCONTENT, 0, 0);

	for (Descriptors::const_iterator iter = pPMS->program_descriptors.begin();
		iter != pPMS->program_descriptors.end(); iter++)
	{
		TCHAR szByte[6]            = {0};
		TCHAR szBytes[256 * 5 + 1] = {0}; // maximum 256 1-byte values (" 0xFF")
		for (BYTE i = 0; i < iter->length; i++)
		{
			wsprintf(szByte, " 0x%X", iter->pbData[i]);
			lstrcat(szBytes, szByte);
		}
		wsprintf(sz, TEXT("{tag: %u; length: %u; data:%s}"), iter->tag, iter->length, szBytes);
		SendDlgItemMessage(hwndDlg, IDC_PROGRAM_DESCRIPTORS, LB_ADDSTRING, 0, (LPARAM)sz);
	}

	// Fill the tree-view with the values of ES descriptors

	HWND hwndTV = GetDlgItem(hwndDlg, IDC_ES_DESCRIPTORS);

	TreeView_DeleteAllItems(hwndTV);

	for (PMTable::const_iterator iter = pPMS->m_PMT.begin(); iter != pPMS->m_PMT.end(); iter++)
	{
		wsprintf(sz, TEXT("{stream type: %u; elementary PID: %u; ES info length: %u}"),
			iter->stream_type, iter->elementary_PID, iter->ES_info_length);

		TVITEMEX tvi = {0};
		tvi.mask       = TVIF_TEXT;
		tvi.pszText    = sz;
		tvi.cchTextMax = sizeof(sz) / sizeof(sz[0]);

		TVINSERTSTRUCT tvis = {0};
		tvis.hParent      = TVI_ROOT;
		tvis.hInsertAfter = TVI_LAST;
		tvis.itemex       = tvi;

		HTREEITEM hTIRoot = TreeView_InsertItem(hwndTV, &tvis);

		for (Descriptors::const_iterator descriptorsIter = iter->ES_descriptors.begin();
			descriptorsIter != iter->ES_descriptors.end(); descriptorsIter++)
		{
			TCHAR szByte[6]    = {0};
			TCHAR szBytes[256 * 5 + 1] = {0}; // maximum 256 1-byte values (" 0xFF")
			for (BYTE i = 0; i < descriptorsIter->length; i++)
			{
				wsprintf(szByte, " 0x%X", descriptorsIter->pbData[i]);
				lstrcat(szBytes, szByte);
			}
			wsprintf(sz, TEXT("{tag: %u; length: %u; data:%s}"),
				descriptorsIter->tag, descriptorsIter->length, szBytes);

			tvis.hParent      = hTIRoot;
			tvis.hInsertAfter = TVI_LAST;
			TreeView_InsertItem(hwndTV, &tvis);
		}
	}
}

//
// ResetAllControls
//
// Reset text in all controls of dialog to it's initial state.
void ResetAllControls(HWND hwndDlg)
{
	SetDlgItemText(hwndDlg, IDE_FILE, TEXT(""));
	SetDlgItemText(hwndDlg, IDC_TITLE, TEXT("Program Map Section"));

	TCHAR sz[] = TEXT("-");

	SetDlgItemText(hwndDlg, IDE_TABLE_ID, sz);
	SetDlgItemText(hwndDlg, IDE_SECTION_SYNTAX_INDICATOR, sz);
	SetDlgItemText(hwndDlg, IDE_SECTION_LENGTH, sz);
	SetDlgItemText(hwndDlg, IDE_PROGRAM_NUMBER, sz);
	SetDlgItemText(hwndDlg, IDE_VERSION_NUMBER, sz);
	SetDlgItemText(hwndDlg, IDE_CURRENT_NEXT_INDICATOR, sz);
	SetDlgItemText(hwndDlg, IDE_SECTION_NUMBER, sz);
	SetDlgItemText(hwndDlg, IDE_LAST_SECTION_NUMBER, sz);
	SetDlgItemText(hwndDlg, IDE_PCR_PID, sz);
	SetDlgItemText(hwndDlg, IDE_PROGRAM_INFO_LENGTH, sz);
	SetDlgItemText(hwndDlg, IDE_CRC, sz);

	SendDlgItemMessage(hwndDlg, IDC_PROGRAM_DESCRIPTORS, LB_RESETCONTENT, 0, 0);
	TreeView_DeleteAllItems(GetDlgItem(hwndDlg, IDC_ES_DESCRIPTORS));

	EnableWindow(GetDlgItem(hwndDlg, IDB_FIRST), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDB_PREVIOUS), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDB_NEXT), FALSE);
	EnableWindow(GetDlgItem(hwndDlg, IDB_LAST), FALSE);
}