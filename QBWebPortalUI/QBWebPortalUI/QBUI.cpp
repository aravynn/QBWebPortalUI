#include "QBUI.h"

QBUI::QBUI()
{
}

void QBUI::setButtonColor(HWND hWnd)
{
	
	SendMessage(hWnd, WM_CTLCOLORBTN, (WPARAM)GetDC(m_controls.at((int)ctrlS::START)), (LPARAM)m_controls.at((int)ctrlS::START));
	InvalidateRect(m_controls.at((int)ctrlS::START), NULL, TRUE);
	UpdateWindow(hWnd);
}

void QBUI::makeUI(HWND hWnd)
{
	if (m_controls.size() > 0) {
		// controls already exist, delete all controls.
		for (int i{ 0 }; i < (int)m_controls.size(); ++i) {
			DestroyWindow(m_controls.at(i));
		}
	}

	// resize for new data.
	m_controls.resize(15);

	int lineHeight = 35;
	int textLineHeight = 20;
	int lineGap = lineHeight + 5;
	

	
	// buttons
	m_controls.at((int)ctrlS::START) = CreateWindowW(L"BUTTON", L"Start", WS_CHILD | WS_VISIBLE , 370, 0, 110, lineHeight, hWnd, (HMENU)BU_START, NULL, NULL);
	m_controls.at((int)ctrlS::RUNMIN) = CreateWindowW(L"BUTTON", L"Run Now", WS_CHILD | WS_VISIBLE , 370, lineGap, 110, lineHeight, hWnd, (HMENU)BU_RUNMIN, NULL, NULL);
	m_controls.at((int)ctrlS::EDITTIME) = CreateWindowW(L"BUTTON", L"Edit Time", WS_CHILD | WS_VISIBLE , 270, lineGap, 80, lineHeight, hWnd, (HMENU)BU_EDITTIME, NULL, NULL);

	// textedit area
	m_controls.at((int)ctrlS::INTERVALBG) = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 270, 0, 80, lineHeight, hWnd, (HMENU)ST_INTERVALBG, NULL, NULL);
	m_controls.at((int)ctrlS::INTERVALEDIT) = CreateWindowW(L"EDIT", L"15", WS_CHILD | WS_VISIBLE | SS_CENTER, 271, 10, 78, textLineHeight, hWnd, (HMENU)TE_INTERVALEDIT, NULL, NULL);

	HFONT hfb = CreateFontW(/*size=*/24, 0, 0, 0, FW_BOLD, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Calibri");
	HFONT hfn = CreateFontW(/*size=*/18, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Calibri");

	// static text boxes.
	m_controls.at((int)ctrlS::MAINSYNCTEXT) = CreateWindowW(L"STATIC", L"Next Sync Run:\n2021-09-17 11:54:29", WS_CHILD | WS_VISIBLE , 5, 0, 150, lineHeight, hWnd, (HMENU)ST_MAINSYNCTEXT, NULL, NULL);
	m_controls.at((int)ctrlS::MAINNITERVALTEXT) = CreateWindowW(L"STATIC", L"Interval:", WS_CHILD | WS_VISIBLE | SS_RIGHT, 170, 10, 80, textLineHeight, hWnd, (HMENU)ST_MAINNITERVALTEXT, NULL, NULL);
	m_controls.at((int)ctrlS::MINSYNCTEXT) = CreateWindowW(L"STATIC", L"Min/Max Sync Runs Daily At:\n17:30:00", WS_CHILD | WS_VISIBLE , 5, lineGap, 250, lineHeight, hWnd, (HMENU)ST_MINSYNCTEXT, NULL, NULL);
	m_controls.at((int)ctrlS::STATUSTITLE) = CreateWindowW(L"STATIC", L"Sync Status:", WS_CHILD | WS_VISIBLE , 5, lineGap * 2, 250, textLineHeight+5, hWnd, (HMENU)ST_STATUSTITLE, NULL, NULL);
	m_controls.at((int)ctrlS::INVENTORYSTATUS) = CreateWindowW(L"STATIC", L"Inventory: Not Run", WS_CHILD | WS_VISIBLE , 5, lineGap * 2 + 25, 250, textLineHeight, hWnd, (HMENU)ST_INVENTORYSTATUS, NULL, NULL);
	m_controls.at((int)ctrlS::CUSTOMERSTATUS) = CreateWindowW(L"STATIC", L"Customers: Not Run", WS_CHILD | WS_VISIBLE , 5, lineGap * 2 + 50, 250, textLineHeight, hWnd, (HMENU)ST_CUSTOMERSTATUS, NULL, NULL);
	m_controls.at((int)ctrlS::SALESORDERSTATUS) = CreateWindowW(L"STATIC", L"Sales Orders: Not Run", WS_CHILD | WS_VISIBLE , 250, lineGap * 2 + 25, 250, textLineHeight, hWnd, (HMENU)ST_SALESORDERSTATUS, NULL, NULL);
	m_controls.at((int)ctrlS::ESTIMATESTATUS) = CreateWindowW(L"STATIC", L"Estimates: Not Run", WS_CHILD | WS_VISIBLE , 250, lineGap * 2 + 50, 250, textLineHeight, hWnd, (HMENU)ST_ESTIMATESTATUS, NULL, NULL);
	m_controls.at((int)ctrlS::INVOICESTATUS) = CreateWindowW(L"STATIC", L"Invoices: Not Run", WS_CHILD | WS_VISIBLE , 250, lineGap * 2 + 75, 250, textLineHeight, hWnd, (HMENU)ST_INVOICESTATUS, NULL, NULL);
	m_controls.at((int)ctrlS::MINMAXSTATUS) = CreateWindowW(L"STATIC", L"Min/Max: Not Run", WS_CHILD | WS_VISIBLE , 5, lineGap * 2 + 75, 250, textLineHeight, hWnd, (HMENU)ST_MINMAXSTATUS, NULL, NULL);


	for (auto h : m_controls) {
		SendMessageW(h, WM_SETFONT, (WPARAM)hfn, 0);
	}

	SendMessageW(m_controls.at((int)ctrlS::STATUSTITLE), WM_SETFONT, (WPARAM)hfb, 0);
}