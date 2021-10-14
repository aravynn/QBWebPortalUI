#pragma once

#include <string>

#include "XMLParser.h"
#include <comdef.h>
#include <comutil.h>
#include "Enums.h"  // ThrownError, ErrorLevel

#pragma comment(lib, "comsuppwd.lib")

#import "QBXMLRP2.dll" named_guids //raw_interfaces_only

/**
* 
* QB request will take generic requests, send to file, then return a pointer XML Parser. 
* Note: constructor will initialize QB and the destructor will close resources. 
* Calls independently done in XML Request and Return, which should be store for later. 
* 
*/

// request enumerator for called function. 
// Will be expanded later. 
enum class QBReq {
	ITEMQUERY,
	CUSTOMERQUERY
};

class QBRequest
{
private:
	std::string m_responseXML;		// returned XML from quickbooks, stored after a call.
    std::string m_appID;            // chosen app ID
    std::string m_appName;          // chosen app name
    std::string m_companyFile{""};	// company file location, blank if current file only.
	std::string m_Msg;				// message returned from system, in any case.
    bool m_ErrorStatus = false;		// error Status, if true means there is an error

    bool isSessionOn = false;       // Is the session been started? required functionality at load. 

    // variables for request.
    QBXMLRP2Lib::IRequestProcessor6Ptr pReqProc;
    BSTR m_ticket; // ticket ID for this request.
public:
	QBRequest(std::string ID, std::string appName, std::string company = ""); // we'll assume an active file is typically used, for this call. 
	~QBRequest(); // close resource.	

    void processRequest(std::string& requestXML);

    std::string getResponse(); // returns response string.

    static void BSTRToString(BSTR inVal, std::string& outVal);
};


