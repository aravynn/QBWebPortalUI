#include "QBUI.h"

std::mutex m;

QBUI::QBUI()
{
}

void QBUI::setButtonColor(HWND hWnd)
{
	// function was meant to alter button color. This is not completed or implemented.
	SendMessage(hWnd, WM_CTLCOLORBTN, (WPARAM)GetDC(m_controls.at((int)ctrlS::START)), (LPARAM)m_controls.at((int)ctrlS::START));
	InvalidateRect(m_controls.at((int)ctrlS::START), NULL, TRUE);
	UpdateWindow(hWnd);
}

QXStatus QBUI::initializeSyncApp(HWND& hWnd, std::string password)
{
	// starts the syncing process, assumes that the system is not running an alternate function. 
	m_transferStatus = std::make_shared<TransferStatus>();
	m_boolStatus = std::make_shared<bool>(false); // must be false for nothing running.
	try {
		m_sync = std::make_shared<QBXMLSync>("QUI", "QB XML Sync", password, m_transferStatus, m_boolStatus);
	}
	catch (ThrownError e) {
		// Quickbooks failed, warn the user to check quickbooks, then exit via FALSE

		std::wstring errval(e.data.begin(), e.data.end());

		MessageBox(hWnd, errval.c_str(), L"ERROR", MB_OK);
		return QXStatus::NOQB;
	}

	if (!m_sync->getSQLStatus()) {
		// SQL failed to initialize, we'll need to load the configuration manager.
		return QXStatus::NOSQL;
	}

	// we were successful
	return QXStatus::QXOK;
}

bool QBUI::startMainSyncFunction(HWND& hWnd)
{
	// create the new thread and generate the setup.
	// we'll also need likely a second thread for "watching" information, or to do so via refreshing.

	// we're already running, so exit this function.
	if (*m_boolStatus == true) {
		return false;
	}

	auto run = [](std::shared_ptr<QBXMLSync> sync) {
		// other pointers are already linked to sync. 
		
		// mutex guard for sync. check later if needed.
		m.lock();

		// this just starts the process of the function, but allows it to run on a second thread.
		sync->fullsync();

		m.unlock();

	};

	// reset the status variable
	*m_boolStatus = true;
	SetTimer(hWnd, IDT_SYNCTIMER, 1000, (TIMERPROC)NULL);

	// initialize the thread.
	std::thread mythread(run, m_sync);

	// after doing so, let it run apart from this thread.
	mythread.detach();

	return true;
}





bool QBUI::startMinMax(HWND& hWnd, int year, int month, int day, int endYear, int endMonth, int endDay)
{
	//  start min/max process. This updates the values on the server based on information in the mysql database.

	// we're running, so exit this function.
	if (*m_boolStatus == true) {
		return false;
	}

	bool endDate = (endYear > -1 && endMonth > -1 && endDay > -1);

	if(endDate){
		std::string msg{ "Run Min/Max from " + std::to_string(day) + '/' + std::to_string(month) + '/' + std::to_string(year) + 
			" to " + std::to_string(endDay) + '/' + std::to_string(endMonth) + '/' + std::to_string(endYear)};
		m_sync->addLog(msg);
	}

	// check the timing, get the total days between NOW and offered date.
	time_t now = time(NULL);
	struct tm tm_struct;
	
	localtime_s(&tm_struct, &now);

	std::vector<int> monthDays{ 31,28,31,30,31,30,31,31,30,31,30,31 };

	int thisYear = tm_struct.tm_year + 1900;
	// month not required, since we have yday.
	int thisDay = tm_struct.tm_yday;

	if (thisYear <= year - 1) {
		std::wstring err = L"Entered date is in the future" + std::to_wstring(thisYear) + L' ' + std::to_wstring(year);
		MessageBox(hWnd, err.c_str(), L"Error", IDOK);
		return false; // this is an invalid claim
	}

	if(thisYear <= endYear - 1) {
		std::wstring err = L"Entered date is in the future" + std::to_wstring(endYear) + L' ' + std::to_wstring(year);
		MessageBox(hWnd, err.c_str(), L"Error", IDOK);
		return false; // this is an invalid claim
	}

	if ((endYear < year ||
		(endYear == year && endMonth < month) ||
		(endYear == year && endMonth == month && endDay < day)) &&
		endDate) {
		std::wstring err = L"The entered start date is after the entered end date.";
		MessageBox(hWnd, err.c_str(), L"Error", IDOK);
		return false; // this is an invalid claim
	}

	int monthCount{ 0 };
	for (int i{ 0 }; i < month - 1; ++i) {
		monthCount += monthDays.at(i);
	}

	int endMonthCount{ 0 };
	for (int i{ 0 }; i < endMonth - 1; ++i) {
		endMonthCount += monthDays.at(i);
	}

	int daysFrom = ((thisYear - year) * 365 + thisDay) - (monthCount + day);

	int daysFromTo = (endDate ? ((thisYear - endYear) * 365 + thisDay) - (endMonth + endDay) : 0);

	if (daysFrom < 90) {
		MessageBox(hWnd, L"At least 90 days of backdata is required.", L"Error", IDOK);
		return false; // this is an invalid claim
	}

	/*
	std::wstring x{ L"days from: " + std::to_wstring(daysFrom) };
	MessageBox(hWnd, x.c_str(), L"Notice", IDOK);
	return false;
	*/

	auto run = [](std::shared_ptr<QBXMLSync> sync, int days, int dayTo) {
		// other pointers are already linked to sync. 

		// mutex guard for sync. check later if needed.
		m.lock();

		// this just starts the process of the function, but allows it to run on a second thread.
		sync->updateMinMaxBatch(50, days, dayTo);

		m.unlock();

	};

	// reset the status variable
	*m_boolStatus = true;
	SetTimer(hWnd, IDT_SYNCTIMER, 1000, (TIMERPROC)NULL);

	// initialize the thread.
	std::thread mythread(run, m_sync, daysFrom,  daysFromTo);

	// after doing so, let it run apart from this thread.
	mythread.detach();

	return true;
}

void QBUI::cancelCurrentAction()
{
	// this will force all actions to reach and execute the break point.
	*m_boolStatus = false;
}

void QBUI::generateConnectionFile(std::string username, std::string password, std::string ip, std::string database, std::string encryptionPassword)
{
	// creates a connection file when closing/clicking ok on the connection window.

	// get a passthrough pointer for the SQL object.
	SQLControl * sql = m_sync->getSqlPtr();

	// assign the connection file.
	sql->generateConnectionFile(username, password, ip, database, encryptionPassword);
}

bool QBUI::setButtonText(ctrlS btn, std::wstring text)
{
	// alter button text on click or other actions.

	return SendMessage(m_controls.at((int)btn), WM_SETTEXT, 0, (LPARAM) text.c_str());
}

bool QBUI::getSyncBoolStatus()
{
	// returns the status of secondary strings.

	return *m_boolStatus;
}

void QBUI::updateSyncProgress(HWND& hWnd)
{
	// manage the statics for all synced data. 
	std::wstring inv = L"Inventory: " + (m_transferStatus->inventory > 0 ? std::to_wstring(m_transferStatus->inventory) : (m_transferStatus->inventory > -1 ? L"Complete" : L"Not Run"));
	setStatic(m_controls.at((int)ctrlS::INVENTORYSTATUS), hWnd, inv);

	std::wstring cus = L"Customers: " + (m_transferStatus->customers > 0 ? std::to_wstring(m_transferStatus->customers) : (m_transferStatus->customers > -1 ? L"Complete" : L"Not Run"));
	setStatic(m_controls.at((int)ctrlS::CUSTOMERSTATUS), hWnd, cus);

	std::wstring est = L"Estimates: " + (m_transferStatus->estimates > 0 ? std::to_wstring(m_transferStatus->estimates) : (m_transferStatus->estimates > -1 ? L"Complete" : L"Not Run"));
	setStatic(m_controls.at((int)ctrlS::ESTIMATESTATUS), hWnd, est);

	std::wstring inc = L"Invoices: " + (m_transferStatus->invoices > 0 ? std::to_wstring(m_transferStatus->invoices) : (m_transferStatus->invoices > -1 ? L"Complete" : L"Not Run"));
	setStatic(m_controls.at((int)ctrlS::INVOICESTATUS), hWnd, inc);

	std::wstring sal = L"Sales Orders: " + (m_transferStatus->salesorders > 0 ? std::to_wstring(m_transferStatus->salesorders) : (m_transferStatus->salesorders > -1 ? L"Complete" : L"Not Run"));
	setStatic(m_controls.at((int)ctrlS::SALESORDERSTATUS), hWnd, sal);

	std::wstring minmaxNote;
	// switch case here, displays current status to user, since this is more complex 
	switch (m_transferStatus->minmax) {
	case -100:
		minmaxNote = L"Reset Inventory";
		break;
	case -99:
		minmaxNote = L"Set Tallys";
		break;
	case -98:
		minmaxNote = L"Set MaxInv";
		break;
	case -97:
		minmaxNote = L"Add Assemblies To Tally";
		break;
	case -96:
		minmaxNote = L"Max Inv Again";
		break;
	case -95:
		minmaxNote = L"Max Inv Updates";
		break;
	case -94:
		minmaxNote = L"Tally Update";
		break;
	case -1:
		minmaxNote = L"Not Run";
		break;
	case 0:
		minmaxNote = L"Complete";
		break;
	default:
		minmaxNote = std::to_wstring(m_transferStatus->minmax);
		break;
	}

	std::wstring min = L"Min/Max: " + minmaxNote;
	setStatic(m_controls.at((int)ctrlS::MINMAXSTATUS), hWnd, min);
}

void QBUI::runMinMaxDailySync(HWND& hWnd, int hour, int minute)
{
	// determine the current time, hours and minutes.
	time_t now = time(NULL);
	struct tm tm_struct;
	
	localtime_s(&tm_struct, &now);

	int thisHour = tm_struct.tm_hour;
	int thisMinute = tm_struct.tm_min;

	int thisTime = (thisHour * 60 * 60 * 1000) + (thisMinute * 60 * 1000); // time in microseconds 
	int runTime = (hour * 60 * 60 * 1000) + (minute * 60 * 1000); // time in microseconds

	/*std::wstring msg = std::to_wstring(hour) + L':' + std::to_wstring(minute) + L' ' +
		std::to_wstring(thisHour) + L':' + std::to_wstring(thisMinute) + L' ' + 
		std::to_wstring(thisTime) + L' ' + std::to_wstring(runTime) + L' ' +
		std::to_wstring(runTime - thisTime);
		*/
	if (thisTime < runTime) {
		// run TODAY.
		//MessageBox(hWnd, msg.c_str(), L"today", IDOK);

		SetTimer(hWnd, IDT_MINMAXTIMER, runTime - thisTime, (TIMERPROC)NULL);
	}
	else {
		// run TOMORROW
		int maxTime = 24 * 60 * 60 * 1000;
		maxTime -= (thisTime - runTime);
		//MessageBox(hWnd, msg.c_str(), L"tomorrow", IDOK);

		SetTimer(hWnd, IDT_MINMAXTIMER, maxTime, (TIMERPROC)NULL);
	}
	// reset the static display
	//m_controls.at((int)ctrlS::MINSYNCTEXT)
	std::wstring newStaticData = L"Min/Max Sync Runs Daily At:\n" + std::to_wstring(hour) + L':' + std::to_wstring(minute) + L":00";
	setStatic(m_controls.at((int)ctrlS::MINSYNCTEXT), hWnd, newStaticData);
}

void QBUI::setStatic(HWND stat, HWND& hWnd, std::wstring string)
{
	// change static text on updates.

	RECT rect;
	GetClientRect(stat, &rect);
	InvalidateRect(stat, &rect, TRUE);
	MapWindowPoints(stat, hWnd, (POINT*)&rect, 2);
	SendMessage(stat, WM_SETTEXT, 0, (LPARAM)string.c_str());
	RedrawWindow(hWnd, &rect, NULL, RDW_ERASE | RDW_INVALIDATE);
}

void QBUI::resetSyncTime(int tableIndex, int year, int month, int day, int hour, int minute, int second) // std::string date, std::string time)
{
	// reset the time on sync values for next run.

	SQLTable tableval = (SQLTable)tableIndex;
	/*
	// cast the table value to the enum
	
	// concatenate the timeframe for insert. 
	std::string newTime = date + ' ' + time;

	int year, month, day, hour, minute, second;
	// break apart time values and generate the new values. 
	//as 2021-12-31 and 12:59:59 PM
	year = std::stoi(date.substr(0, 4));
	month = std::stoi(date.substr(5, 2));
	day = std::stoi(date.substr(8, 2));

	int hourplaces = 1;
	// time is a bit harder, since hours can be either 1 or 2 places.
	if (time.at(2) == ':') {
		// 2 place time.
		hourplaces = 2;
	}

	hour = std::stoi(time.substr(0, hourplaces));
	minute = std::stoi(time.substr(1 + hourplaces, 2));
	second = std::stoi(time.substr(4 + hourplaces, 2));

	if (time.at(hourplaces + 7) == 'P') {
		hour += 12;
	}
	*/
	m_sync->timeReset(tableval, year, month, day, hour, minute, second);

}

void QBUI::displayNextSyncTime(HWND& hWnd, int minute)
{
	// using known sync time update the window to display

	// determine the current time, hours and minutes.
	time_t now = time(NULL);
	
	time_t newTime = now + minute * 60;
	
	struct tm t;

	localtime_s(&t, &newTime);

	// reset the static display
	//m_controls.at((int)ctrlS::MINSYNCTEXT)
	///std::wstring newStaticData = L"Next Sync Run:\n" + std::to_wstring(t.tm_year) + L'-' + std::to_wstring(t.tm_mon) + L'-' + std::to_wstring(t.tm_mday) + L" "
	//	+ std::to_wstring(t.tm_hour) + L':' + std::to_wstring(t.tm_min) + L':' + std::to_wstring(t.tm_sec);
	char str[50];
	asctime_s(str, 50, &t);
	std::string tVal = str;
	std::wstring wtVal(tVal.begin(), tVal.end());
	std::wstring newStaticData = L"Next Sync Run:\n" + wtVal;

	setStatic(m_controls.at((int)ctrlS::MAINSYNCTEXT), hWnd, newStaticData);
}

void QBUI::makeUI(HWND hWnd)
{
	// create the primary user interface for the main window.

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