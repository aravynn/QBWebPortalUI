#pragma once


/**
* 
* QBXMLSync controls the actual processing functions required for the transfer of data from Quickbooks to the database. 
* Additionally, this class will also manage any required inserts and updates to the file, as per the given data. 
* This class is unlikely to store a great deal of held information.
* 
*/

// std functions 
#include <vector>
#include <string>
#include <sstream> // for date generator.
#include <iomanip> // setw
#include <memory>  // weak, shared pointers.
#include <map>	   // will be used for back and forth data transfer.

// app classes. 
#include "SQLControl.h"
#include "XMLParser.h"
#include "XMLNode.h"
#include "QBRequest.h"

enum class SQLTable {
	inventory = 2,
	customer = 0,
	purchaseorder = 4,
	salesorder = 5,
	invoice = 3,
	estimate = 1
};

// used for data transfer to and from the new thread.
struct TransferStatus {
	int inventory{ -1 };
	int customers{ -1 };
	int salesorders{ -1 };
	int estimates{ -1 };
	int invoices{ -1 };
	int minmax{ -1 };
};

class QBXMLSync
{
private:
	std::unique_ptr<SQLControl> m_sql = nullptr;	// connects to SQL and handles any collection and return. 
	QBRequest m_req;	// connects to QB and handles any collection and return. 
	std::shared_ptr<bool> m_isActive; // bool for determining the status of the run. will be defaulted to TRUE externally.
	std::shared_ptr<TransferStatus> m_status;
	bool m_verbose = false ;

	void updateInventoryMinMax(std::string ListID, int min); // per line, update the min/max, to minimum, 0.

	bool getIteratorData(XMLNode& nodeRet, std::list<int> path, std::string& iteratorID, int& statusCode, int& remainingCount);
	int iterate(std::string& returnData, std::string& iteratorID, std::string table, int maximum = 100, std::string date = "NA", bool useOwner = false, bool modDateFilter = false, bool orderLinks = false);
	bool queryAll(std::string& returnData, std::string table);
	int addAddress(XMLNode& addressNode, std::string customerListID);

	bool isActive();

public:
	QBXMLSync(std::string QBID, std::string appName, std::string password, std::shared_ptr<TransferStatus> status = nullptr, std::shared_ptr<bool> active = nullptr);	// generate the SQL and request items. 
	~QBXMLSync();
	std::shared_ptr<TransferStatus> getStatus() { return m_status; }
	bool getSQLStatus(); // check the current SQL status and return TRUE if ok or FALSE if SQL is not set.
	SQLControl * getSqlPtr();	// probably inappropriate solution, buuut it works!
	void addLog(std::string& msg);	// create a message to pass back for logging in DB.

	bool getInventory();	
	bool getSalesOrders();
	bool getEstimates();
	bool getInvoices();
	bool getCustomers();

	bool getPriceLevels();
	bool getSalesTerms();
	bool getTaxCodes();
	bool getSalesReps();
//	bool getShippingTypes();	// not sure if required. 

	bool updateMinMax(); // update the minmax for all items. 
	bool updateMinMaxBatch(int batch = 100, int daysLimit = 365, int daysEnd = 0); // update the minmax for all items, in pre-grouped sets. This may not be at all valid. 
	bool updateMinMaxInventory(const std::string& listID, std::string& editSequence, int reorderPoint = -1, bool max = false);
	bool updateMinMaxInventory(const std::vector<std::string>& listID, std::vector<std::string>& editSequence, std::vector<int> newValue, bool max = false); // max is assumed for ALL items.
	bool updateInventoryPartnumbers(int limit = 100, std::string type = "ItemInventoryMod"); // limited update function for all parts. Just a one-off.
	bool updatePartsBatch(const std::vector<std::string>& listID, std::vector<std::string>& editSequence, std::vector<std::string> newValue, std::string requestType); // limited update function continuance.

	bool timeReset(SQLTable table, int Y = 1970, int M = 1, int D = 1, int H = 12, int m = 0, int S = 0);
	bool timeReset(SQLTable table, std::string datetime = "1970-01-01 12:00:00");

	bool fullsync();
};

