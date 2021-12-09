// QBWebPortalUI.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "QBWebPortalUI.h"


#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
QBUI ifc;                                       // interface controls.
HWND hWnd;


bool syncRun = false;
bool minRun = false;
int mmHour = 17;                                // default time for min/max is 5:30 PM, local time.
int mmMinute = 30;
int syncMinutes = 15;                           // default time for sync action.
int monthMinMaxLimit = 12;                      // limit of time for data we have to work with.
int dayMinMaxLimit = 1;                         // default assuming that start date is DECEMBER 1, 2021 
int yearMinMaxLimit = 2021;                     // for the new file.

int monthMinMaxEnd = -1;                        // end date for minmax sync
int dayMinMaxEnd = -1;                           // - 
int yearMinmaxEnd = -1;                       // -
bool doCalculateEndDate = false;                // Use this date, or calculate from current date?


// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

INT_PTR CALLBACK Connection(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK Password(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MinTime(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK TimeReset(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MonthLimit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK startDate(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    HRESULT hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_QBWEBPORTALUI, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_QBWEBPORTALUI));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    CoUninitialize();
    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_QBWEBPORTALUI));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_QBWEBPORTALUI);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindowW(szWindowClass, L"QB Web Portal", WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 500, 250, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        // create window stuff here...
        
        ifc.makeUI(hWnd);
    
        // load the connector, password function etc. here. all functions managed directly in password dialog.
        DialogBox(hInst, MAKEINTRESOURCE(IDD_PASSWORD), hWnd, Password);

        // start the daily timer, we'll need to get this running.
        ifc.runMinMaxDailySync(hWnd, mmHour, mmMinute);

        // start the interval timer, it should be started now.
        SetTimer(hWnd, IDT_SYNCINTERVALTIMER, syncMinutes * 60 * 1000, (TIMERPROC)NULL);
    }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_RUNTIME:
                //MessageBox(hWnd, L"Test", L"Runtime Popup!", MB_OK);
                DialogBox(hInst, MAKEINTRESOURCE(IDD_MINMAXTIME), hWnd, MinTime);
                break;
            case IDM_CONNECT:
                //MessageBox(hWnd, L"Test", L"Connection Popup!", MB_OK);
                DialogBox(hInst, MAKEINTRESOURCE(IDD_SQLSETUP), hWnd, Connection);
                break;
            case IDM_TIMERESET:
                //MessageBox(hWnd, L"Test", L"Time Reset Popup!", MB_OK);
                DialogBox(hInst, MAKEINTRESOURCE(IDD_TIMERESET), hWnd, TimeReset);
                break;
            case ID_FILE_LIMITLOADTIME:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_LIMITLOADTIME), hWnd, MonthLimit);
                break;
            case ID_FILE_SETLASTDATE:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_MINMAXLASTDATE), hWnd, startDate);
                break;
            case BU_EDITTIME:
                //MessageBox(hWnd, L"Test", L"Edit Minmax Time Popup!", MB_OK);
                DialogBox(hInst, MAKEINTRESOURCE(IDD_MINMAXTIME), hWnd, MinTime);
                break;
            case BU_RUNMIN: 
            {
                //std::wstring data = L"Year " + std::to_wstring(yearMinMaxLimit) + L" Month " + std::to_wstring(monthMinMaxLimit) + L" day " + std::to_wstring(dayMinMaxLimit);

                //MessageBox(hWnd, data.c_str(), L"Time", MB_OK);

                // test to output the edit box int.
                if (ifc.getSyncBoolStatus() && !syncRun) {
                    ifc.cancelCurrentAction();
                    ifc.setButtonText(ctrlS::RUNMIN, L"Run Now");
                }
                else {
                    if (!ifc.startMinMax(hWnd, yearMinMaxLimit, monthMinMaxLimit, dayMinMaxLimit, yearMinmaxEnd, monthMinMaxEnd, dayMinMaxEnd)) {
                        // error, we've already started another function 
                        MessageBox(hWnd, L"Another process is already running, please wait or cancel.", L"ERROR", MB_OK);
                    }
                    else {
                        ifc.setButtonText(ctrlS::RUNMIN, L"Cancel");
                        minRun = true;
                    }
                }
            }
                break;
            case BU_START:
            {
                // test to output the edit box int.
                if (ifc.getSyncBoolStatus() && !minRun) {
                    ifc.cancelCurrentAction();
                    ifc.setButtonText(ctrlS::START, L"Start");
                }
                else {
                    if (!ifc.startMainSyncFunction(hWnd)) {
                        // error, we've already started another function 
                        MessageBox(hWnd, L"Another process is already running, please wait or cancel.", L"ERROR", MB_OK);
                    }
                    else {
                        // set button to stop, we've started a new process. 
                        ifc.setButtonText(ctrlS::START, L"Stop");
                        syncRun = true;

                        // reset the timer, get the current interval time, and run.
                        KillTimer(hWnd, IDT_SYNCINTERVALTIMER);
                        TCHAR c[128];
                        //HWND h = GetWindow(hWnd, TE_INTERVALEDIT);
                        GetWindowText(ifc.m_controls.at((int)ctrlS::INTERVALEDIT), c, 128);
                        syncMinutes = std::stoi(std::wstring(c));

                        ifc.displayNextSyncTime(hWnd, syncMinutes);

                        // set a new timer for the new interval.
                        SetTimer(hWnd, IDT_SYNCINTERVALTIMER, syncMinutes * 60 * 1000, (TIMERPROC)NULL);
                    }
                }
                
                //TCHAR c[128];
                //HWND h = GetWindow(m_controls, TE_INTERVALEDIT);
                //GetWindowText(ifc.m_controls.at((int)ctrlS::INTERVALEDIT), c, 128);
                //int x = std::stoi(std::wstring(c));
                //std::wstring s = L"Start/Stop Popup! " + std::to_wstring(x);
                //MessageBox(hWnd, s.c_str(), L"Test", MB_OK);
            }
                break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_CTLCOLORSTATIC:
    {

        { 
            HDC hdcStatic = (HDC)wParam; // or obtain the static handle in some other way
            SetTextColor(hdcStatic, RGB(0, 0, 0)); // text color
            SetBkColor(hdcStatic, RGB(255, 255, 255));
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_TIMER:
    {
        // handles all timer related functionality, based on the SetTimer function. Should be self-killing based on isActive.
        switch (wParam) {
            case IDT_SYNCTIMER:
            {
                // run the watch timer here.
                ifc.updateSyncProgress(hWnd);

                // if isActive is false, kill the timer
                if (!ifc.getSyncBoolStatus()) {
                    ifc.setButtonText(ctrlS::START, L"Start");
                    ifc.setButtonText(ctrlS::RUNMIN, L"Run Now");
                    syncRun = false;
                    minRun = false;
                    KillTimer(hWnd, IDT_SYNCTIMER);
                }
            }
            break;
            case IDT_MINMAXTIMER:
            {
                //MessageBox(hWnd, L"test", L"test", IDOK);

                // run the min max daily timer, loads at timeout.
                // will initialize at the start of the program.
                KillTimer(hWnd, IDT_MINMAXTIMER); // kill this, the interval is wrong.
                
                // run the current min/max program

                if (syncRun || minRun) {
                    // wait a minute, then try again.
                    SetTimer(hWnd, IDT_MINMAXTIMER, 60 * 1000, (TIMERPROC)NULL);
                } else {
                    if (ifc.startMinMax(hWnd, yearMinMaxLimit, monthMinMaxLimit, dayMinMaxLimit, yearMinmaxEnd, monthMinMaxEnd, dayMinMaxEnd)) {
                        ifc.setButtonText(ctrlS::RUNMIN, L"Cancel");
                        minRun = true;
                    }

                    ifc.runMinMaxDailySync(hWnd, mmHour, mmMinute);
                }

            }
            break;
            case IDT_SYNCINTERVALTIMER:
            {
                // runs every x intervals.
                if (!syncRun && !minRun) {
                    // we can run this interval, otherwise we'll skip for the next run.
                    if (ifc.startMainSyncFunction(hWnd))
                    {
                        // set button to stop, we've started a new process. 
                        ifc.setButtonText(ctrlS::START, L"Stop");
                        syncRun = true;
                    }
                }
                // reset the timer, get the current interval time, and run.
                KillTimer(hWnd, IDT_SYNCINTERVALTIMER);
                ifc.displayNextSyncTime(hWnd, syncMinutes);

                // set a new timer for the new interval.
                SetTimer(hWnd, IDT_SYNCINTERVALTIMER, syncMinutes * 60 * 1000, (TIMERPROC)NULL);
            }
            break;
        }
    }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Connection(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            // save the new conenction file.
            const int ipLength = 256;
            TCHAR data[ipLength];

            // create the items required for running the program. IDC_PASSENTRY
            GetDlgItemText(hDlg, IDC_SQLIP, data, ipLength);
            std::wstring nData{ data };
            std::string IP(nData.begin(), nData.end());
            
            GetDlgItemText(hDlg, IDC_SQLUSER, data, ipLength);
            nData = data;
            std::string User(nData.begin(), nData.end());

            GetDlgItemText(hDlg, IDC_SQLPASS, data, ipLength);
            nData = data;
            std::string Pass(nData.begin(), nData.end());

            GetDlgItemText(hDlg, IDC_LOCALPASS, data, ipLength);
            nData = data;
            std::string LocalPass(nData.begin(), nData.end());

            GetDlgItemText(hDlg, IDC_SQLDATABASE, data, ipLength);
            nData = data;
            std::string Database(nData.begin(), nData.end());
            
            ifc.generateConnectionFile(User, Pass, IP, Database, LocalPass);

            std::string d = IP + ' ' + User + ' ' + Pass + ' ' + LocalPass + ' ' + Database;

            std::wstring q(d.begin(), d.end());

            MessageBox(hDlg, q.c_str(), L"Result", MB_OK);

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            MessageBox(hDlg, L"Failed to connect to database.", L"MySQL Error", MB_OK);
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;

        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK Password(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            

            const int ipLength = 256;
            TCHAR pass[ipLength];

            // create the items required for running the program. IDC_PASSENTRY
            GetDlgItemText(hDlg, IDC_PASSENTRY, pass, ipLength);

            // convert password  to usable value.
            std::wstring nPass{ pass };
            std::string sPass(nPass.begin(), nPass.end());

            QXStatus testload = ifc.initializeSyncApp(hWnd, sPass);
            
            switch (testload) {
            case QXStatus::NOQB:
                MessageBox(hDlg, L"Could not find Quickbooks, please login and try again.", L"Quickbooks Error", MB_OK);
                break;
            case QXStatus::NOSQL:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_SQLSETUP), hWnd, Connection);
                break;
            case QXStatus::QXOK:
                // we're good.
                break;
            }

            
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK MinTime(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            // get the variable value from the ID box and update the time, then, reset and re-run the daily timer.
            
            HWND h = GetDlgItem(hDlg, IDC_MINMAXTIME);
            SYSTEMTIME st{};
            DateTime_GetSystemtime(h, &st);
            
            mmHour = st.wHour;
            mmMinute = st.wMinute;
            
            /*
            TCHAR c[128];
            GetDlgItemText(hDlg, IDC_MINMAXTIME, c, 128);
            std::wstring time = c;

            // convert the time into hours and minutes.
            int hourplaces = 1;
            // time is a bit harder, since hours can be either 1 or 2 places.
            if (time.at(2) == ':') {
                // 2 place time.
                hourplaces = 2;
            }
            // using hours and minutes, reset the main variables.
            mmHour = std::stoi(time.substr(0, hourplaces));
            mmMinute = std::stoi(time.substr(1 + hourplaces, 2));
            
            if (time.at(hourplaces + 7) == 'P') {
                mmHour += 12;
            }
            */
            // end the current timer and reset the new one.
            KillTimer(hWnd, IDT_MINMAXTIMER);
            ifc.runMinMaxDailySync(hWnd, mmHour, mmMinute);

            // exit this dialog
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK TimeReset(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    std::vector<std::wstring> options;
    options.push_back(L"Customers");
    options.push_back(L"Estimates");
    options.push_back(L"Inventory");
    options.push_back(L"Invoices");
    options.push_back(L"Purchase Orders");
    options.push_back(L"Sales Orders");


    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
    {
        // load the dropdown options
        HWND d = GetDlgItem(hDlg, IDC_TABLE);
        for (auto o : options) {
            SendMessage(d, CB_ADDSTRING, 0, (LPARAM)o.c_str());
        }
    }

        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            HWND d = GetDlgItem(hDlg, IDC_RESETDATE);
            SYSTEMTIME stday{};
            DateTime_GetSystemtime(d, &stday);

            HWND t = GetDlgItem(hDlg, IDC_RESETTIME);
            SYSTEMTIME sttime{};
            DateTime_GetSystemtime(d, &sttime);

            TCHAR c[128];
            GetDlgItemText(hDlg, IDC_TABLE, c, 128);
            auto it = std::find(options.begin(), options.end(), std::wstring(c));
            int index = it - options.begin();

            /*
            // run the actual update here. 
            GetDlgItemText(hDlg, IDC_RESETDATE, c, 128);

            std::wstring date = c;

            GetDlgItemText(hDlg, IDC_RESETTIME, c, 128);

            std::wstring time = c;

            std::string sdate(date.begin(), date.end());
            std::string stime(time.begin(), time.end());
            */
            ifc.resetSyncTime(index, stday.wYear, stday.wMonth, stday.wDay, sttime.wHour, sttime.wMinute, sttime.wSecond);// sdate, stime);

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK MonthLimit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    SYSTEMTIME* st = new SYSTEMTIME();
    st->wYear = yearMinMaxLimit;
    st->wMonth = monthMinMaxLimit;
    st->wDay = dayMinMaxLimit;

    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
    {
  
        HWND h = GetDlgItem(hDlg, IDC_MONTHLIMIT);
        DateTime_SetSystemtime(h, GDT_VALID, st);
    }
    return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            // update the month limit, for the next sync run. 
            // note that we'll need to remember to set this manually every time - or pop this up maybe? until the 12 months are over (2022)
            // we'll assume a monthly limit from december 1, 2021
            HWND h = GetDlgItem(hDlg, IDC_MONTHLIMIT);
            SYSTEMTIME st{};
            DateTime_GetSystemtime(h, &st);
            
            yearMinMaxLimit = st.wYear;
            monthMinMaxLimit = st.wMonth;
            dayMinMaxLimit = st.wDay;
   
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    delete st;
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK startDate(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    SYSTEMTIME* st = new SYSTEMTIME();
    st->wYear = yearMinMaxLimit;
    st->wMonth = monthMinMaxLimit;
    st->wDay = dayMinMaxLimit;

    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
    {

        HWND h = GetDlgItem(hDlg, IDC_MONTHLIMIT);
        DateTime_SetSystemtime(h, GDT_VALID, st);
    }
    return (INT_PTR)TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
            case IDOK:
            {
                // update the month limit, for the next sync run. 
                // note that we'll need to remember to set this manually every time - or pop this up maybe? until the 12 months are over (2022)
                // we'll assume a monthly limit from december 1, 2021

                //HWND ok = GetDlgItem(hDlg, IDC_CHECKMAXDATE);
            
                if (doCalculateEndDate) {
                    HWND h = GetDlgItem(hDlg, IDC_MAXSETDATE);
                    SYSTEMTIME st{};
                    DateTime_GetSystemtime(h, &st);

                    yearMinmaxEnd = st.wYear;
                    monthMinMaxEnd = st.wMonth;
                    dayMinMaxEnd = st.wDay;
                }
                else {
                    // forcibly reset to -1 in the case of adding then removing. 
                    yearMinmaxEnd = -1;
                    monthMinMaxEnd = -1;
                    dayMinMaxEnd = -1;
                }
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
            case IDC_CHECKMAXDATE:
            {
                if (HIWORD(wParam) == BN_CLICKED) {
                    if (SendDlgItemMessage(hDlg, IDC_CHECKMAXDATE, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                       // MessageBox(hDlg, L"Checked.", L"CheckBox", MB_OK);
                        doCalculateEndDate = true;
                    }
                    else {
                       // MessageBox(hDlg, L"Not Checked.", L"CheckBox", MB_OK);
                        doCalculateEndDate = false;
                    }
                }
            }
            break;
        }
        break;
    }
    delete st;
    return (INT_PTR)FALSE;
}

/*
int  = 12;                        // end date for minmax sync
int  = 1;                           // -
int  = 2021;                       // -
bool  = false;                // Use this date, or calculate from current date?
*/