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

class QBUI
{
private:
public:

	// HWND controls.
	std::vector<HWND> m_controls;

	QBUI();	// default constructor will liekly do nothing, this is a manual processing class.
	void setButtonColor(HWND hWnd);

	void makeUI(HWND hWnd);	// generate UI controls, requires the external HWND object.
};

