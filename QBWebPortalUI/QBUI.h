#pragma once

#include "resource.h"
#include <vector>

#include "targetver.h"
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>

#include <limits>
#include <thread> // multithreading.
#include <mutex> // thread protection.
#include <ctime> // time management

#include "QBXMLSync.h"  // sync functionality
/**
* 
* QBUI controls everything for the user interface. 
* 
*/

// button and other numerations
#define BU_START 1000
#define BU_RUNMIN 1001
#define BU_EDITTIME 1002
#define TE_INTERVALEDIT 1003
#define ST_INTERVALBG 1014
#define ST_MAINSYNCTEXT 1004
#define ST_MAINNITERVALTEXT 1005
#define ST_MINSYNCTEXT 1006
#define ST_STATUSTITLE 1007
#define ST_INVENTORYSTATUS 1008
#define ST_CUSTOMERSTATUS 1009
#define ST_SALESORDERSTATUS 1010
#define ST_ESTIMATESTATUS 1011
#define ST_INVOICESTATUS 1012
#define ST_MINMAXSTATUS 1013


// control IDS for internal application reference.
enum class ctrlS {
	START,
	RUNMIN,
	EDITTIME,
	INTERVALEDIT,
	INTERVALBG,
	MAINSYNCTEXT,
	MAINNITERVALTEXT,
	MINSYNCTEXT,
	STATUSTITLE,
	INVENTORYSTATUS,
	CUSTOMERSTATUS,
	SALESORDERSTATUS,
	ESTIMATESTATUS,
	INVOICESTATUS,
	MINMAXSTATUS
};

enum class QXStatus {
	NOSQL,
	NOQB,
	QXOK
};

class QBUI
{
private:
	std::shared_ptr<QBXMLSync> m_sync = nullptr;				// stored for shared services.
	std::shared_ptr<TransferStatus> m_transferStatus = nullptr;	// for UI status
	std::shared_ptr<bool> m_boolStatus = nullptr;				// bool to end the thread.



public:

	// HWND controls.
	std::vector<HWND> m_controls;
	
	

	QBUI();	// default constructor will likely do nothing, this is a manual processing class.
	void setButtonColor(HWND hWnd);

	QXStatus initializeSyncApp(HWND& hWnd, std::string password);	// create the required variables. 
	bool startMainSyncFunction(HWND& hWnd);							// start the main sync function 
	bool startMinMax(HWND& hWnd, int year = 2021, int month = 12, int day = 1);		// start the minMax sync functionality. 
	void cancelCurrentAction();										// cancel the current function using boolStatus 
	void generateConnectionFile(std::string username, std::string password, std::string ip, std::string database, std::string encryptionPassword);

	bool setButtonText(ctrlS btn, std::wstring text);

	bool getSyncBoolStatus();
	void updateSyncProgress(HWND& hWnd);
	void runMinMaxDailySync(HWND& hWnd, int hour = 17, int minute = 30);

	void setStatic(HWND stat, HWND& hWnd, std::wstring string);

	void resetSyncTime(int tableIndex, std::string date, std::string time);
	void displayNextSyncTime(HWND& hWnd, int minute);							// display the next sync time

	void makeUI(HWND hWnd);	// generate UI controls, requires the external HWND object.



};

