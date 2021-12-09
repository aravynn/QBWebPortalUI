#include "QBXMLSync.h"

void QBXMLSync::updateInventoryMinMax(std::string ListID, int min)
{
    // --------------------------------------------------------------------------------------------------------------------!!
}

QBXMLSync::QBXMLSync(std::string QBID, std::string appName, std::string password, std::shared_ptr<TransferStatus> status, std::shared_ptr<bool> active) 
    : m_sql{ new SQLControl(std::string(password)) }, m_req{ QBRequest(QBID, appName) }, m_isActive{ active }
{
    // if an external status object is given, assign it, otherwise, assume internal only.
    if (status != nullptr) {
        m_status = status;
    }
    else {
        m_status = std::make_shared<TransferStatus>();
    }

}

QBXMLSync::~QBXMLSync()
{
   // delete m_sql;   // solve the deleted function issue.
}

bool QBXMLSync::getSQLStatus()
{
    // check that sql is there. if not, we failed and need to initialize.

    return m_sql->isConnected();
}

SQLControl *  QBXMLSync::getSqlPtr()
{
    return m_sql.get();
}

void QBXMLSync::addLog(std::string& msg)
{
    // passthrough for a message (notice only.)
    m_sql->logError(ErrorLevel::EXTRA, msg);
}



bool QBXMLSync::getIteratorData(XMLNode& nodeRet, std::list<int> path, std::string& iteratorID, int& statusCode, int& remainingCount) {
    XMLNode BaseNode;
    
    if (nodeRet.getNodeFromPath(path, BaseNode)) {

        std::string name{ "iteratorID" };
        XMLNode attr = BaseNode.getAttribute(name);
        iteratorID.clear();
        iteratorID = attr.getStringData();

        name = "statusCode";
        attr = BaseNode.getAttribute(name);
        statusCode = attr.getIntData();

        name = "iteratorRemainingCount";
        attr = BaseNode.getAttribute(name);
        remainingCount = attr.getIntData();

        return true;
    }
    
    return false; //(failed to get data.)

}

// FOR ALL ITERATIVE FUNCTIONS. either create or run an iteration on the DB. returns number of row remaining or -1 for bad request.

//bool iterate(std::string& returnData, std::string table, int maximum = 100, bool useOwner = false, std::string iteratorID = "INIT");
int QBXMLSync::iterate(std::string& returnData, std::string& iteratorID, std::string table, int maximum, std::string date, bool useOwner, bool modDateFilter, bool orderLinks) {
    
    // steps: 
    // start the iterator with the initial call.
    // create the XML
    XMLNode node(table);

    if (iteratorID.size() > 0 && iteratorID.at(0) == '{') {
        node.addAttribute("iterator", "Continue"); // Start or Continue.
        node.addAttribute("iteratorID", iteratorID);
    }
    else {
        node.addAttribute("iterator", "Start"); // Start or Continue.
    }

    node.addNode("MaxReturned", std::to_string(maximum), std::list<int>{});
    
    if (date != "NA") {
        // non default should be legitimate value
        if (modDateFilter) {
            std::list<int> li;

            li.push_back(node.addNode(std::string("ModifiedDateRangeFilter"), li));

            node.addNode("FromModifiedDate", date, li);
        }
        else {
            node.addNode("FromModifiedDate", date, std::list<int>{});
        }
    }

    if (orderLinks) {
        // add nodes for line items and linked txns.
        node.addNode("IncludeLineItems", "true", std::list<int>{});
        node.addNode("IncludeLinkedTxns", "true", std::list<int>{});
    }


    if (useOwner) {
        node.addNode("OwnerID", "0", std::list<int>{});
    }

    std::string xml = XMLParser::createXMLRequest(node);

    // send to QB, and get return value.
    m_req.processRequest(xml);

    returnData.clear();
    returnData = m_req.getResponse();

    XMLNode nodeRet;

    try {
        XMLParser p(returnData);

        nodeRet = p.getNode();
    }
    catch (ThrownError e) {
        m_sql->logError(e.error, e.data);
    }

    std::list<int> path = { 0, 0, 0 };

    XMLNode BaseNode;
    int remainingCount{ -1 };       // remaining count, used repeptitively.
    int statusCode{ 0 };            // status, used frequently.

    if (!getIteratorData(nodeRet, path, iteratorID, statusCode, remainingCount)) {
        std::string err = "Iterator failed at get node: ID: " + iteratorID + " File: " + __FILE__ + " Line: " + std::to_string(__LINE__) + " Data: " + returnData;
        m_sql->logError(ErrorLevel::NOTICE, err);
        return -10; //failed to get data.
    }

    if (statusCode != 0) {
        nodeRet.getNodeFromPath(path, BaseNode);
        // output the error here. 
        std::string name{ "statusMessage" };
        XMLNode attr = BaseNode.getAttribute(name);

        std::string err = "QBXML Error Code: " + std::to_string(statusCode) + ": " + attr.getStringData() + " File: " + __FILE__ + " Line: " + std::to_string(__LINE__) + " Data: " + returnData;
        m_sql->logError(ErrorLevel::NOTICE, err);

        return -100000 + statusCode;
    }
  
    return remainingCount;

}

// Add an address as provided by any other function, return the ID of the entered line. 
// ------------------------------------------------------------------------------------

bool QBXMLSync::queryAll(std::string& returnData, std::string table)
{
    // steps: 
    // start the iterator with the initial call.
    // create the XML
    XMLNode node(table);

    std::string xml = XMLParser::createXMLRequest(node);

    // send to QB, and get return value.
    m_req.processRequest(xml);

    returnData.clear();
    returnData = m_req.getResponse();

    XMLNode nodeRet;

    try {
        XMLParser p(returnData);
        nodeRet = p.getNode();
    }
    catch (ThrownError e) {
        m_sql->logError(e.error, e.data);
    }

    std::list<int> path = { 0, 0, 0 };

    XMLNode BaseNode;
    
    nodeRet.getNodeFromPath(path, BaseNode);

    std::string name = "statusCode";
    XMLNode attr = BaseNode.getAttribute(name);
    int statusCode = attr.getIntData();

    if (statusCode != 0) {
        // output the error here. 
        std::string name{ "statusMessage" };
        XMLNode attr = BaseNode.getAttribute(name);

        std::string err = "QueryAll failed at get node: File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__) + " Data: " + returnData;
        m_sql->logError(ErrorLevel::NOTICE, err);

        return false;
    }
    return true;
}

int QBXMLSync::addAddress(XMLNode& addressNode, std::string customerListID)
{
    std::string table = "address";

    std::vector<std::string> columns = { "ID" }; // just getting the ID.

    FilterPass inserts;
    inserts.resize(10);
    inserts.at(0) = { "CustomerListID", customerListID };
    inserts.at(1) = { "Address1", addressNode.getChildValue("Addr1", "") };
    inserts.at(2) = { "Address2", addressNode.getChildValue("Addr2", "") };
    inserts.at(3) = { "Address3", addressNode.getChildValue("Addr3", "") };
    inserts.at(4) = { "Address4", addressNode.getChildValue("Addr4", "") };
    inserts.at(5) = { "Address5", addressNode.getChildValue("Addr5", "") };
    inserts.at(6) = { "City", addressNode.getChildValue("City", "") };
    inserts.at(7) = { "State", addressNode.getChildValue("State", "") };
    inserts.at(8) = { "Country", addressNode.getChildValue("Country", "") };
    inserts.at(9) = { "PostalCode", addressNode.getChildValue("PostalCode", "") };

    // first, check if this ALREADY exists, and return that.
    if (m_sql->SQLSelect(columns, table, inserts, 1)) {
        m_sql->nextRow();
        return m_sql->getInt("ID");
    }

    // if it does not, then add and return THAT value.
    if (m_sql->SQLInsert(table, inserts)) {
        // inserted, so return the line ID.

        if (m_sql->SQLSelect(columns, table, inserts, 1)) {
            m_sql->nextRow();
            return m_sql->getInt("ID");
        } 
    }

    return -1; // failed for some reason.
}

bool QBXMLSync::isActive() {
    // check if set, if it is return, otherwise assume a return of true.
    if (m_isActive.use_count() > 0) {
        return *m_isActive;
    }
    else {
        return true;
    }
}

// Pull inventory data, and parse it for insert into DB. 
// -----------------------------------------------------

bool QBXMLSync::getInventory()
{
    // NEW FUNCTION: pulls as usual, but build a COMPLEX update to mass insert/update values.
    std::string data;
    std::string iterator;

    std::string time = m_sql->getConfigTime("inventory");

    std::list<int> path = { 0, 0, 0 }; // path direct to top node of data.

    //int left{ 1 };

    m_status->inventory = 1;

    while (m_status->inventory > 0) {

        m_status->inventory = iterate(data, iterator, "ItemQueryRq", 100, time, true);

        if (!isActive()) {
            return false;
        }

        // declare storage variables for mass calls.
        std::vector<FilterPass> insertsInventory;       // main invnetory content
        std::vector<FilterPass> childInserts;           // assembly line content
        std::string table{ "inventory" };               // we'll reuse this to reset as "inventoryassembly" later.

        // format data for insert into database.
        
        XMLNode node;

        try {
            XMLParser p(data);
            node = p.getNode();
        }
        catch (ThrownError e) {
            m_sql->logError(e.error, e.data);
        }
        
        XMLNode nodeChildren;
        node.getNodeFromPath(path, nodeChildren);

        bool findErr = false;

        int nodeCount = nodeChildren.getChildCount();

        // save space for all insert lines, assuming that there is sufficient numbers.
        insertsInventory.resize(nodeCount);
        
        for (int i{ 0 }; i < nodeCount; ++i) {
            // for each child, convert and output. 

            XMLNode child = nodeChildren.child(i);

            insertsInventory.at(i).resize(19); // all the data we'll need to pass.
            insertsInventory.at(i).at(0) = { "ListID", child.getChildValue("ListID", "") };
            insertsInventory.at(i).at(1) = { "TimeCreated", child.getChildValue("TimeCreated", "") };

            insertsInventory.at(i).at(2) = { "TimeModified", child.getChildValue("TimeModified", "") };
            insertsInventory.at(i).at(3) = { "Name", child.getChildValue("Name", "") };
            insertsInventory.at(i).at(4) = { "SKU", child.getChildValue("FullName", "") };
            insertsInventory.at(i).at(5) = { "ProductType", child.getName() };

            insertsInventory.at(i).at(11) = { "Details", child.getChildValue("SalesDesc", "") };
            insertsInventory.at(i).at(12) = { "PurchaseOrderDetails", child.getChildValue("PurchaseDesc", "") };

            std::string AsstName = "UnitOfMeasureSetRef";
            XMLNode Asst = child.findChild(AsstName);
            if (Asst.getStatus()) {
                insertsInventory.at(i).at(13) = { "UOfM", Asst.getChildValue("FullName", "") };
            }
            else {
                insertsInventory.at(i).at(13) = { "UOfM", std::string("") };
            }

            insertsInventory.at(i).at(6) = { "InStock", child.getDoubleOrInt("QuantityOnHand") };

            insertsInventory.at(i).at(7) = { "OnOrder", child.getDoubleOrInt("QuantityOnOrder") };
            insertsInventory.at(i).at(8) = { "OnSalesOrder", child.getDoubleOrInt("QuantityOnSalesOrder") };
            insertsInventory.at(i).at(9) = { "Price", child.getDoubleOrInt("SalesPrice") };
            insertsInventory.at(i).at(10) = { "Cost", child.getDoubleOrInt("AverageCost") };
            insertsInventory.at(i).at(15) = { "ReorderPoint", (long long)child.getIntOrDouble("ReorderPoint") };
            insertsInventory.at(i).at(16) = { "MaxInv", (long long)child.getIntOrDouble("Max", -1) };

            insertsInventory.at(i).at(14) = { "IsActive", (long long)(child.getChildValue("IsActive", "") == "true" ? 1 : 0) };
            insertsInventory.at(i).at(17) = { "RunningTally", (long long)0 };
            insertsInventory.at(i).at(18) = { "EditSequence", child.getChildValue("EditSequence", "") };

            // if this is an assembly line, we'll need to also upload the lineitems.
            if (child.getName() == "ItemInventoryAssemblyRet") {
                
                // resize to accomodate space for the maximum possible amount of addable elements.
                //childInserts.resize(childInserts.size() + child.getChildCount()); // this is more than required

                // update the child array to store all of the data to be kept. 
                int childNodeCount = child.getChildCount();
                for (int q{ 0 }; q < childNodeCount; ++q) {
                    Asst = child.child(q);
                    if (Asst.getName() == "ItemInventoryAssemblyLine") {
                        // this line can be added to the assemblies.
                        //childInserts.at(q).resize(3);

                        FilterPass ci;
                        ci.resize(3);

                        std::string list = "ItemInventoryRef";
                        XMLNode c = Asst.findChild(list);
                        ci.at(0) = { "ListID", c.getChildValue("ListID", "") };
                        ci.at(1) = { "ReferenceListID", insertsInventory.at(i).at(0).second.str };
                        ci.at(2) = { "Quantity", Asst.getDoubleOrInt("Quantity") };

                        childInserts.push_back(ci);
                    }
                }
            }
            


        }

        // run the actual inserts here 
        if (!m_sql->SQLMassInsert(table, insertsInventory)) {
            std::string err = "Mass Insert/Update inventory Failed " + table + " File: " + __FILE__ + " Line: " + std::to_string(__LINE__) + " Data: " + data;
            m_sql->logError(ErrorLevel::WARNING, err);
        }
        else {
            // on a success only, do we add the child lines. 
            table = "assembly_lineitems";

            if (childInserts.size() > 0) {
                // only run this in the case of child inserts to be entered.
                if (!m_sql->SQLMassInsert(table, childInserts)) {
                    std::string err = "Mass Insert/Update AssemblyLine failed " + table + " File: " + __FILE__ + " Line: " + std::to_string(__LINE__) + " Data: " + data;
                    m_sql->logError(ErrorLevel::NOTICE, err);
                }
            }
        }
    }

    return m_sql->updateConfigTime("inventory");
}

bool QBXMLSync::fullsync()
{
    // check and reaffirm connection.
    if (!m_sql->isConnected()) return false;

    // run a sync of everything, for simpler code later.
    if (!getInventory()) { return false; }
    if (!getSalesOrders()) { return false; }
    if (!getEstimates()) { return false; }
    if (!getInvoices()) { return false; }
    if (!getCustomers()) { return false; }
    if (!getPriceLevels()) { return false; }
    if (!getSalesTerms()) { return false; }
    if (!getTaxCodes()) { return false; }
    if (!getSalesReps()) { return false; }

    *m_isActive = false;

    return true; // everything worked.
}

bool QBXMLSync::getSalesOrders()
{

    std::string data;
    std::string iterator;

    std::string time = m_sql->getConfigTime("salesorders");

    std::list<int> path = { 0, 0, 0 }; // path direct to top node of data.

    //int left{ 1 };

    m_status->salesorders = 1;

    while (m_status->salesorders > 0) {

        m_status->salesorders = iterate(data, iterator, "SalesOrderQueryRq", 50, time, false, true, true);

        if (!isActive()) {
            return false;
        }

        std::string table{ "orders" }; // GENERIC table used for all orders.
        std::vector<FilterPass> inserts;        // sales orders
        std::vector<FilterPass> lineInserts;    // line items
        std::vector<FilterPass> linkedInserts;  // linked transactions.

        XMLNode node;

        try {
            XMLParser p(data);
            node = p.getNode();
        }
        catch (ThrownError e) {
            m_sql->logError(e.error, e.data);
        }
        XMLNode nodeChildren;
        node.getNodeFromPath(path, nodeChildren);

        bool findErr = false;

        // run though, set by set.
        int nodeCount = nodeChildren.getChildCount();

        inserts.resize(nodeCount);

        // format data for insert into database.

        for (int i{ 0 }; i < nodeCount; ++i) {
            // for each child, convert and output. 
            XMLNode child = nodeChildren.child(i);
            
            inserts.at(i).resize(26); // all the data we'll need to pass.
            inserts.at(i).at(0) = { "TxnID", child.getChildValue("TxnID", "") };

            inserts.at(i).at(1) = { "OrderType", std::string("SO") };
            inserts.at(i).at(2) = { "TimeCreated", child.getChildValue("TimeCreated", "") };
            inserts.at(i).at(3) = { "TimeModified", child.getChildValue("TimeModified", "") };
            inserts.at(i).at(4) = { "TxnDate", child.getChildValue("TxnDate", "1970-01-01") };
            inserts.at(i).at(5) = { "DueDate", child.getChildValue("DueDate", "1970-01-01") };
            inserts.at(i).at(6) = { "ShipDate", child.getChildValue("ShipDate", "1970-01-01") };

            XMLNode Asset = child.findChild("TermsRef");
            inserts.at(i).at(7) = { "TermsID", Asset.getChildValue("ListID", "") };
            Asset = child.findChild("CustomerRef");
            inserts.at(i).at(8) = { "CustomerListID", Asset.getChildValue("ListID", "") };
            inserts.at(i).at(16) = { "CustomerFullName", Asset.getChildValue("FullName", "") };
            Asset = child.findChild("ShipAddress");
            inserts.at(i).at(9) = { "ShipAddressID", (long long)addAddress(Asset, inserts.at(i).at(8).second.str) };
            Asset = child.findChild("BillAddress");
            inserts.at(i).at(10) = { "BillAddressID", (long long)addAddress(Asset, inserts.at(i).at(8).second.str) };
            Asset = child.findChild("ShipMethodRef");
            inserts.at(i).at(11) = { "ShipTypeID", Asset.getChildValue("ListID", "") };
            Asset = child.findChild("ItemSalesTaxRef");
            inserts.at(i).at(12) = { "TaxCodeID", Asset.getChildValue("ListID", "") };
            Asset = child.findChild("SalesRepRef");
            inserts.at(i).at(13) = { "SalesRepID", Asset.getChildValue("ListID", "") };

            inserts.at(i).at(14) = { "RefNumber", child.getChildValue("RefNumber", "") };
            inserts.at(i).at(15) = { "PONumber", child.getChildValue("PONumber", "") };
            inserts.at(i).at(17) = { "FOB", child.getChildValue("FOB", "") };
            inserts.at(i).at(18) = { "Subtotal", child.getDoubleOrInt("Subtotal") };
            inserts.at(i).at(19) = { "SalesTaxTotal", child.getDoubleOrInt("SalesTaxTotal") };
            inserts.at(i).at(20) = { "SaleTotal", child.getDoubleOrInt("TotalAmount") };
            inserts.at(i).at(21) = { "Balance", 0.0 };
            inserts.at(i).at(22) = { "IsPending", (long long)1 };
            inserts.at(i).at(23) = { "Status", (long long)(child.getChildValue("IsManuallyClosed", "") == "true" ? 0 : 1) };
            inserts.at(i).at(24) = { "IsInvoiced", (long long)(child.getChildValue("IsFullyInvoiced", "") == "true" ? 1 : 0) };
            inserts.at(i).at(25) = { "Notes", child.getChildValue("Memo", "") };

            // order lines and linked transaction inserting.
            int childNodeCount = child.getChildCount();
            for (int q{ 0 }; q < childNodeCount; ++q) {
                Asset = child.child(q);
                // Order Lines 
                if (Asset.getName() == "SalesOrderLineRet") {

                    FilterPass ci;

                    ci.resize(10);
                    ci.at(0) = { "LineID", Asset.getChildValue("TxnLineID", "") };
                    ci.at(1) = { "TxnID", inserts.at(i).at(0).second.str };

                    XMLNode c = Asset.findChild("ItemRef");
                    ci.at(2) = { "ItemListID", c.getChildValue("ListID", "") };
                    ci.at(3) = { "Description", Asset.getChildValue("Desc", "") };
                    ci.at(4) = { "Quantity", Asset.getDoubleOrInt("Quantity") };
                    ci.at(5) = { "Rate", Asset.getDoubleOrInt("Rate") };
                    ci.at(6) = { "UnitOfMeasure", Asset.getChildValue("UnitOfMeasure", "") };

                    c = Asset.findChild("SalesTaxCodeRef");
                    ci.at(7) = { "TaxCode", c.getChildValue("ListID", "") };
                    ci.at(8) = { "UniqueIdentifier", (Asset.getChildValue("SerialNumber", "") != "" ? Asset.getChildValue("SerialNumber", "") : Asset.getChildValue("LotNumber", "")) };
                    ci.at(9) = { "Invoiced", (long long)Asset.getIntOrDouble("Invoiced") };

                    lineInserts.push_back(ci);

                }
                // Linked Transactions 
                if (Asset.getName() == "LinkedTxn") {

                    FilterPass ci;

                    ci.resize(3);
                    ci.at(0) = { "BaseID", inserts.at(i).at(0).second.str };
                    ci.at(1) = { "LinkID", Asset.getChildValue("TxnID", "") };
                    ci.at(2) = { "Type", Asset.getChildValue("TxnType", "") };

                    linkedInserts.push_back(ci);

                }
            }
        }

        // Final insert calls here 
        if (!m_sql->SQLMassInsert(table, inserts)) {
            std::string err = "Mass Insert/Update sales order Failed " + table + " File: " + __FILE__ + " Line: " + std::to_string(__LINE__) + " Data: " + data;
            m_sql->logError(ErrorLevel::WARNING, err);
        }
        else {
            // on a success only, do we add the child lines. 

            table = "lineitem";

            if (lineInserts.size() > 0) {
                // only run this in the case of child inserts to be entered.
                if (!m_sql->SQLMassInsert(table, lineInserts)) {
                    std::string err = "Mass Insert/Update order lines Failed " + table + " File: " + __FILE__ + " Line: " + std::to_string(__LINE__) + " Data: " + data;
                    m_sql->logError(ErrorLevel::NOTICE, err);
                }
            }

            table = "orderlinks";

            if (linkedInserts.size() > 0) {
                // only run this in the case of child inserts to be entered.
                if (!m_sql->SQLMassInsert(table, linkedInserts)) {
                    std::string err = "Mass Insert/Update linked orders Failed " + table + " File: " + __FILE__ + " Line: " + std::to_string(__LINE__) + " Data: " + data;
                    m_sql->logError(ErrorLevel::NOTICE, err);
                }
            }
        }

    }

    return m_sql->updateConfigTime("salesorders");
}

bool QBXMLSync::getEstimates() 
{
    std::string data;
    std::string iterator;

    std::string time = m_sql->getConfigTime("estimates");

    std::list<int> path = { 0, 0, 0 }; // path direct to top node of data.

    //int left{ 1 };

    m_status->estimates = 1;

    while (m_status->estimates > 0) {

        m_status->estimates = iterate(data, iterator, "EstimateQueryRq", 50, time, false, true, true);

        if (!isActive()) {
            return false;
        }

        std::string table{ "orders" }; // GENERIC table used for all orders.
        std::vector<FilterPass> inserts;        // estimates
        std::vector<FilterPass> lineInserts;    // line items
        std::vector<FilterPass> linkedInserts;  // linked transactions.

        // format data for insert into database.
        
        XMLNode node;

        try {
            XMLParser p(data);
            node = p.getNode();
        }
        catch (ThrownError e) {
            m_sql->logError(e.error, e.data);
        }

        XMLNode nodeChildren;
        node.getNodeFromPath(path, nodeChildren);

        bool findErr = false;

        // run though, set by set.
        int nodeCount = nodeChildren.getChildCount();

        inserts.resize(nodeCount);
        
        for (int i{ 0 }; i < nodeCount; ++i) {
            // for each child, convert and output. 
            XMLNode child = nodeChildren.child(i);

            inserts.at(i).resize(26); // all the data we'll need to pass.
            inserts.at(i).at(0) = { "TxnID", child.getChildValue("TxnID", "") };
            inserts.at(i).at(1) = { "OrderType", std::string("ES") };
            inserts.at(i).at(2) = { "TimeCreated", child.getChildValue("TimeCreated", "") };
            inserts.at(i).at(3) = { "TimeModified", child.getChildValue("TimeModified", "") };
            inserts.at(i).at(4) = { "TxnDate", child.getChildValue("TxnDate", "1970-01-01") };
            inserts.at(i).at(5) = { "DueDate", child.getChildValue("DueDate", "1970-01-01") };
            inserts.at(i).at(6) = { "ShipDate", std::string("1970-01-01") };

            XMLNode Asset = child.findChild("TermsRef");
            inserts.at(i).at(7) = { "TermsID", Asset.getChildValue("ListID", "") };
            Asset = child.findChild("CustomerRef");
            inserts.at(i).at(8) = { "CustomerListID", Asset.getChildValue("ListID", "") };
            inserts.at(i).at(16) = { "CustomerFullName", Asset.getChildValue("FullName", "") };
            Asset = child.findChild("ShipAddress");
            inserts.at(i).at(9) = { "ShipAddressID", (long long)addAddress(Asset, inserts.at(i).at(8).second.str) };
            Asset = child.findChild("BillAddress");
            inserts.at(i).at(10) = { "BillAddressID", (long long)addAddress(Asset, inserts.at(i).at(8).second.str) };
            inserts.at(i).at(11) = { "ShipTypeID", std::string("") };
            Asset = child.findChild("ItemSalesTaxRef");
            inserts.at(i).at(12) = { "TaxCodeID", Asset.getChildValue("ListID", "") };
            Asset = child.findChild("SalesRepRef");
            inserts.at(i).at(13) = { "SalesRepID", Asset.getChildValue("ListID", "") };

            inserts.at(i).at(14) = { "RefNumber", child.getChildValue("RefNumber", "") };
            inserts.at(i).at(15) = { "PONumber", child.getChildValue("PONumber", "") };
            inserts.at(i).at(17) = { "FOB", child.getChildValue("FOB", "") };
            inserts.at(i).at(18) = { "Subtotal", child.getDoubleOrInt("Subtotal") };
            inserts.at(i).at(19) = { "SalesTaxTotal", child.getDoubleOrInt("SalesTaxTotal") };
            inserts.at(i).at(20) = { "SaleTotal", child.getDoubleOrInt("TotalAmount") };
            inserts.at(i).at(21) = { "Balance", 0.0 };
            inserts.at(i).at(22) = { "IsPending", (long long)1 };
            inserts.at(i).at(23) = { "Status", (long long)(child.getChildValue("IsActive", "") == "true" ? 0 : 1) };
            inserts.at(i).at(24) = { "IsInvoiced", (long long)1 };
            inserts.at(i).at(25) = { "Notes", child.getChildValue("Memo", "") };

            // order lines and linked transaction inserting.
            int childNodeCount = child.getChildCount();
            for (int q{ 0 }; q < childNodeCount; ++q) {
                Asset = child.child(q);
                // Order Lines 
                if (Asset.getName() == "EstimateLineRet") {

                    FilterPass childInserts;
                    childInserts.resize(10);
                    childInserts.at(0) = { "LineID", Asset.getChildValue("TxnLineID", "") };
                    childInserts.at(1) = { "TxnID", inserts.at(i).at(0).second.str };

                    XMLNode c = Asset.findChild("ItemRef");
                    childInserts.at(2) = { "ItemListID", c.getChildValue("ListID", "") };
                    childInserts.at(3) = { "Description", Asset.getChildValue("Desc", "") };
                    childInserts.at(4) = { "Quantity", Asset.getDoubleOrInt("Quantity") };
                    childInserts.at(5) = { "Rate", Asset.getDoubleOrInt("Rate") };
                    childInserts.at(6) = { "UnitOfMeasure", Asset.getChildValue("UnitOfMeasure", "") };

                    c = Asset.findChild("SalesTaxCodeRef");
                    childInserts.at(7) = { "TaxCode", c.getChildValue("ListID", "") };
                    childInserts.at(8) = { "UniqueIdentifier", std::string("") };
                    childInserts.at(9) = { "Invoiced", (long long)0 };

                    lineInserts.push_back(childInserts);

                }
                // Linked Transactions 
                if (Asset.getName() == "LinkedTxn") {

                    FilterPass childInserts;
                    childInserts.resize(3);
                    childInserts.at(0) = { "BaseID", inserts.at(i).at(0).second.str };
                    childInserts.at(1) = { "LinkID", Asset.getChildValue("TxnID", "") };
                    childInserts.at(2) = { "Type", Asset.getChildValue("TxnType", "") };

                    linkedInserts.push_back(childInserts);
                }
            }
        }


        // Final insert calls here 
        if (!m_sql->SQLMassInsert(table, inserts)) {
            std::string err = "Mass Insert/Update estimates Failed " + table + " File: " + __FILE__ + " Line: " + std::to_string(__LINE__) + " Data: " + data;
            m_sql->logError(ErrorLevel::WARNING, err);
            findErr = true; // skip the remainder of this line, as to not pointlessly fill the dataabase.
        }
        else {
            // on a success only, do we add the child lines. 

            table = "lineitem";

            if (lineInserts.size() > 0) {
                // only run this in the case of child inserts to be entered.
                if (!m_sql->SQLMassInsert(table, lineInserts)) {
                    std::string err = "Mass Insert/Update order lines Failed " + table + " File: " + __FILE__ + " Line: " + std::to_string(__LINE__) + " Data: " + data;
                    m_sql->logError(ErrorLevel::NOTICE, err);
                }
            }

            table = "orderlinks";

            if (linkedInserts.size() > 0) {
                // only run this in the case of child inserts to be entered.
                if (!m_sql->SQLMassInsert(table, linkedInserts)) {
                    std::string err = "Mass Insert/Update linked orders Failed " + table + " File: " + __FILE__ + " Line: " + std::to_string(__LINE__) + " Data: " + data;
                    m_sql->logError(ErrorLevel::NOTICE, err);
                }
            }
        }
    }
    return m_sql->updateConfigTime("estimates");
    
}

bool QBXMLSync::getInvoices() 
{

    std::string data;
    std::string iterator;

    std::string time = m_sql->getConfigTime("invoices");

    std::list<int> path = { 0, 0, 0 }; // path direct to top node of data.

    //int left{ 1 };
    m_status->invoices = 1;

    while (m_status->invoices > 0) {

        m_status->invoices = iterate(data, iterator, "InvoiceQueryRq", 50, time, false, true, true);

        if (!isActive()) {
            return false;
        }

        std::string table{ "orders" }; // GENERIC table used for all orders.
        std::vector<FilterPass> inserts;        // estimates
        std::vector<FilterPass> lineInserts;    // line items
        std::vector<FilterPass> linkedInserts;  // linked transactions.

        // format data for insert into database.
        
        XMLNode node;

        try {
            XMLParser p(data);
            node = p.getNode();
        }
        catch (ThrownError e) {
            m_sql->logError(e.error, e.data);
        }

        XMLNode nodeChildren;
        node.getNodeFromPath(path, nodeChildren);

        bool findErr = false;

        // run though, set by set.
        int nodeCount = nodeChildren.getChildCount();

        inserts.resize(nodeCount);

        for (int i{ 0 }; i < nodeCount; ++i) {
            // for each child, convert and output. 
            XMLNode child = nodeChildren.child(i);

            inserts.at(i).resize(26); // all the data we'll need to pass.
            inserts.at(i).at(0) = { "TxnID", child.getChildValue("TxnID", "") };
            inserts.at(i).at(1) = { "OrderType", std::string("IN") };
            inserts.at(i).at(2) = { "TimeCreated", child.getChildValue("TimeCreated", "") };
            inserts.at(i).at(3) = { "TimeModified", child.getChildValue("TimeModified", "") };
            inserts.at(i).at(4) = { "TxnDate", child.getChildValue("TxnDate", "1970-01-01") };
            inserts.at(i).at(5) = { "DueDate", child.getChildValue("DueDate", "1970-01-01") };
            inserts.at(i).at(6) = { "ShipDate", child.getChildValue("ShipDate", "1970-01-01") };

            XMLNode Asset = child.findChild("TermsRef");
            inserts.at(i).at(7) = { "TermsID", Asset.getChildValue("ListID", "") };
            Asset = child.findChild("CustomerRef");
            inserts.at(i).at(8) = { "CustomerListID", Asset.getChildValue("ListID", "") };
            inserts.at(i).at(16) = { "CustomerFullName", Asset.getChildValue("FullName", "") };
            Asset = child.findChild("ShipAddress");
            inserts.at(i).at(9) = { "ShipAddressID", (long long)addAddress(Asset, inserts.at(i).at(8).second.str) };
            Asset = child.findChild("BillAddress");
            inserts.at(i).at(10) = { "BillAddressID", (long long)addAddress(Asset, inserts.at(i).at(8).second.str) };
            Asset = child.findChild("ShipMethodRef");
            inserts.at(i).at(11) = { "ShipTypeID", Asset.getChildValue("ListID", "") };
            Asset = child.findChild("ItemSalesTaxRef");
            inserts.at(i).at(12) = { "TaxCodeID", Asset.getChildValue("ListID", "") };
            Asset = child.findChild("SalesRepRef");
            inserts.at(i).at(13) = { "SalesRepID", Asset.getChildValue("ListID", "") };

            inserts.at(i).at(14) = { "RefNumber", child.getChildValue("RefNumber", "") };
            inserts.at(i).at(15) = { "PONumber", child.getChildValue("PONumber", "") };
            inserts.at(i).at(17) = { "FOB", child.getChildValue("FOB", "") };
            inserts.at(i).at(18) = { "Subtotal", child.getDoubleOrInt("Subtotal") };
            inserts.at(i).at(19) = { "SalesTaxTotal", child.getDoubleOrInt("SalesTaxTotal") };
            inserts.at(i).at(20) = { "SaleTotal", child.getDoubleOrInt("AppliedAmount") };
            inserts.at(i).at(21) = { "Balance", child.getDoubleOrInt("BalanceRemaining") };
            inserts.at(i).at(22) = { "IsPending", (long long)(child.getChildValue("IsPending", "") == "true" ? 0 : 1) };
            inserts.at(i).at(23) = { "Status", (long long)(child.getChildValue("IsPaid", "") == "true" ? 0 : 1) };
            inserts.at(i).at(24) = { "IsInvoiced", (long long)1 };
            inserts.at(i).at(25) = { "Notes", child.getChildValue("Memo", "") };

            // order lines and linked transaction inserting.
            int childNodeCount = child.getChildCount();
            for (int q{ 0 }; q < childNodeCount; ++q) {
                Asset = child.child(q);
                // Order Lines 
                if (Asset.getName() == "InvoiceLineRet") {

                    FilterPass childInserts;

                    childInserts.resize(10);
                    childInserts.at(0) = { "LineID", Asset.getChildValue("TxnLineID", "") };
                    childInserts.at(1) = { "TxnID", inserts.at(i).at(0).second.str };

                    XMLNode c = Asset.findChild("ItemRef");
                    childInserts.at(2) = { "ItemListID", c.getChildValue("ListID", "") };
                    childInserts.at(3) = { "Description", Asset.getChildValue("Desc", "") };
                    childInserts.at(4) = { "Quantity", Asset.getDoubleOrInt("Quantity") };
                    childInserts.at(5) = { "Rate", Asset.getDoubleOrInt("Rate") };
                    childInserts.at(6) = { "UnitOfMeasure", Asset.getChildValue("UnitOfMeasure", "") };

                    c = Asset.findChild("SalesTaxCodeRef");
                    childInserts.at(7) = { "TaxCode", c.getChildValue("ListID", "") };
                    childInserts.at(8) = { "UniqueIdentifier", (Asset.getChildValue("SerialNumber", "") != "" ? Asset.getChildValue("SerialNumber", "") : Asset.getChildValue("LotNumber", "")) };
                    childInserts.at(9) = { "Invoiced", (long long)0 };

                    lineInserts.push_back(childInserts);

                }
                // Linked Transactions 
                if (Asset.getName() == "LinkedTxn") {

                    FilterPass childInserts;
                    childInserts.resize(3);
                    childInserts.at(0) = { "BaseID", inserts.at(i).at(0).second.str };
                    childInserts.at(1) = { "LinkID", Asset.getChildValue("TxnID", "") };
                    childInserts.at(2) = { "Type", Asset.getChildValue("TxnType", "") };

                    linkedInserts.push_back(childInserts);

                }
            }
        }

        // Final insert calls here 
        if (!m_sql->SQLMassInsert(table, inserts)) {
            std::string err = "Mass Insert/Update invoices Failed " + table + " File: " + __FILE__ + " Line: " + std::to_string(__LINE__) + " Data: " + data;
            m_sql->logError(ErrorLevel::WARNING, err);
            findErr = true; // skip the remainder of this line, as to not pointlessly fill the dataabase.
        }
        else {
            // on a success only, do we add the child lines. 

            table = "lineitem";

            if (lineInserts.size() > 0) {
                // only run this in the case of child inserts to be entered.
                if (!m_sql->SQLMassInsert(table, lineInserts)) {
                    std::string err = "Mass Insert/Update order lines Failed " + table + " File: " + __FILE__ + " Line: " + std::to_string(__LINE__) + " Data: " + data;
                    m_sql->logError(ErrorLevel::NOTICE, err);
                }
            }

            table = "orderlinks";

            if (linkedInserts.size() > 0) {
                // only run this in the case of child inserts to be entered.
                if (!m_sql->SQLMassInsert(table, linkedInserts)) {
                    std::string err = "Mass Insert/Update linked orders Failed " + table + " File: " + __FILE__ + " Line: " + std::to_string(__LINE__) + " Data: " + data;
                    m_sql->logError(ErrorLevel::NOTICE, err);
                }
            }
        }
    }

    return m_sql->updateConfigTime("invoices");
}

bool QBXMLSync::getCustomers() 
{

    std::string data;
    std::string iterator;

    std::string time = m_sql->getConfigTime("customers");

    std::list<int> path = { 0, 0, 0 }; // path direct to top node of data.

    //int left{ 1 };
    m_status->customers = 1;

    while (m_status->customers > 0) {

        m_status->customers = iterate(data, iterator, "CustomerQueryRq", 50, time);

        if (!isActive()) {
            return false;
        }

        std::string table{ "customers" };
        std::vector<FilterPass> inserts;

        // format data for insert into database.
        XMLNode node;

        try {
            XMLParser p(data);
            node = p.getNode();
        }
        catch (ThrownError e) {
            m_sql->logError(e.error, e.data);
        }
        XMLNode nodeChildren;
        node.getNodeFromPath(path, nodeChildren);

        bool findErr = false;

        // run though, set by set.
        int nodeCount = nodeChildren.getChildCount();

        inserts.resize(nodeCount);

        for (int i{ 0 }; i < nodeCount; ++i) {
            // for each child, convert and output. 
            XMLNode child = nodeChildren.child(i);

            inserts.at(i).resize(18); // all the data we'll need to pass.
            inserts.at(i).at(0) = { "ListID", child.getChildValue("ListID", "") };
            inserts.at(i).at(1) = { "TimeCreated", child.getChildValue("TimeCreated", "") };
            inserts.at(i).at(2) = { "TimeModified", child.getChildValue("TimeModified", "") };

            XMLNode Asset = child.findChild("ShipAddress");
            inserts.at(i).at(3) = { "ShipAddressID", (long long)addAddress(Asset, inserts.at(i).at(0).second.str) };
            Asset = child.findChild("BillAddress");
            inserts.at(i).at(4) = { "BillAddressID", (long long)addAddress(Asset, inserts.at(i).at(0).second.str) };
            Asset = child.findChild("TermsRef");
            inserts.at(i).at(5) = { "TermsID", Asset.getChildValue("ListID", "") };
            Asset = child.findChild("ItemSalesTaxRef");
            inserts.at(i).at(6) = { "TaxCodeID", Asset.getChildValue("ListID", "") };
            Asset = child.findChild("SalesRepRef");
            inserts.at(i).at(7) = { "SalesRepID", Asset.getChildValue("ListID", "") };
            Asset = child.findChild("CustomerTypeRef");
            inserts.at(i).at(8) = { "PriceLevelID", Asset.getChildValue("ListID", "") };

            inserts.at(i).at(9) = { "isActive", (long long)(child.getChildValue("IsActive", "") == "true" ? 1 : 0) };
            inserts.at(i).at(10) = { "FullName", child.getChildValue("FullName", "") };
            inserts.at(i).at(11) = { "CompanyName", child.getChildValue("CompanyName", "") };
            inserts.at(i).at(12) = { "PhoneNumber", child.getChildValue("Phone", "") };
            inserts.at(i).at(13) = { "AltPhoneNumber", child.getChildValue("AltPhone", "") };
            inserts.at(i).at(14) = { "FaxNumber", child.getChildValue("Fax", "") };
            inserts.at(i).at(15) = { "EmailAddress", child.getChildValue("Email", "") };
            inserts.at(i).at(16) = { "Balance", child.getChildValue("Balance", 0.0) };
            inserts.at(i).at(17) = { "CreditLimit", child.getChildValue("CreditLimit", 0.0) };

            // insert all additional addresses 
            int childNodeCount = child.getChildCount();
            for (int q{ 0 }; q < childNodeCount; ++q) {
                Asset = child.child(q);
                if (Asset.getName() == "ShipToAddress") {
                    addAddress(Asset, inserts.at(i).at(0).second.str);
                }
            }
        }

        if (!m_sql->SQLMassInsert(table, inserts)) {
            std::string err = "Mass Insert/Update customers Failed " + table + " File: " + __FILE__ + " Line: " + std::to_string(__LINE__) + " Data: " + data;
            m_sql->logError(ErrorLevel::WARNING, err);
            findErr = true; // skip the remainder of this line, as to not pointlessly fill the dataabase.
        }
    }

    return m_sql->updateConfigTime("customers");
}

bool QBXMLSync::getPriceLevels() 
{
    //------------------------------------------------------------------------------------------------------------------------!!
    std::string data;
    std::string iterator;

    int loopLimit = 0;

    std::list<int> path = { 0, 0, 0 }; // path direct to top node of data.

    if (queryAll(data, "CustomerTypeQueryRq")) {
        
        // format data for insert into database.
        XMLNode node;

        try {
            XMLParser p(data);
            node = p.getNode();
        }
        catch (ThrownError e) {
            m_sql->logError(e.error, e.data);
        }
        XMLNode nodeChildren;
        node.getNodeFromPath(path, nodeChildren);

        bool findErr = false;

        int nodeCount = nodeChildren.getChildCount();
        for (int i{ 0 }; i < nodeCount; ++i) {

            XMLNode child = nodeChildren.child(i);

            std::string table{ "price_level" }; // GENERIC table used for all orders.
            FilterPass inserts;
            FilterPass filter;
            inserts.resize(6); // all the data we'll need to pass.
            inserts.at(0) = { "ListID", child.getChildValue("ListID", "") };
            filter.push_back({ "ListID", child.getChildValue("ListID", "") });
            inserts.at(1) = { "TimeCreated", child.getChildValue("TimeCreated", "") };
            inserts.at(2) = { "TimeModified", child.getChildValue("TimeModified", "") };
            inserts.at(3) = { "isActive", (long long)(child.getChildValue("IsActive", "") == "true" ? 1 : 0) };
            inserts.at(4) = { "Name", child.getChildValue("Name", "") };
            inserts.at(5) = { "DiscountRate", 0.0 };

            if (!m_sql->SQLUpdateOrInsert(table, inserts, filter)) {
                std::string err = "Mass Insert/Update price levels Failed " + table + " File: " + __FILE__ + " Line: " + std::to_string(__LINE__) + " Data: " + data;
                m_sql->logError(ErrorLevel::WARNING, err);
            }

        }
    }
    else {
        return false;
    }
    return true;
}

bool QBXMLSync::getSalesTerms() 
{
    std::string data;
    std::string iterator;

    int loopLimit = 0;

    std::list<int> path = { 0, 0, 0 }; // path direct to top node of data.

    if(queryAll(data, "TermsQueryRq")) {
        
        // format data for insert into database.
        XMLNode node;

        try {
            XMLParser p(data);
            node = p.getNode();
        }
        catch (ThrownError e) {
            m_sql->logError(e.error, e.data);
        }
        XMLNode nodeChildren;
        node.getNodeFromPath(path, nodeChildren);

        bool findErr = false;

        int nodeCount = nodeChildren.getChildCount();
        for (int i{ 0 }; i < nodeCount; ++i) {

            XMLNode child = nodeChildren.child(i);

            std::string table{ "salesterms" }; // GENERIC table used for all orders.
            FilterPass inserts;
            FilterPass filter;
            inserts.resize(8); // all the data we'll need to pass.
            inserts.at(0) = { "ListID", child.getChildValue("ListID", "") };
            filter.push_back({ "ListID", child.getChildValue("ListID", "") });
            inserts.at(1) = { "TimeCreated", child.getChildValue("TimeCreated", "") };
            inserts.at(2) = { "TimeModified", child.getChildValue("TimeModified", "") };
            inserts.at(3) = { "isActive", (long long)(child.getChildValue("IsActive", "") == "true" ? 1 : 0) };
            inserts.at(4) = { "Name", child.getChildValue("Name", "") };
            inserts.at(5) = { "DaysDue", (long long)child.getChildValue("StdDueDays", 0) };
            inserts.at(6) = { "DiscountDays", (long long)child.getChildValue("StdDiscountDays", 0) };
            inserts.at(7) = { "DiscountRate", child.getChildValue("DiscountPct", 0.0) };

            if (!m_sql->SQLUpdateOrInsert(table, inserts, filter)) {
                std::string err = "Mass Insert/Update terms Failed " + table + " File: " + __FILE__ + " Line: " + std::to_string(__LINE__) + " Data: " + data;
                m_sql->logError(ErrorLevel::WARNING, err);
            }
        }
    }
    else {
        return false;
    }
    return true; 
}

bool QBXMLSync::getTaxCodes() 
{
    std::string data;
    std::string iterator;

    int loopLimit = 0;

    std::list<int> path = { 0, 0, 0 }; // path direct to top node of data.

    if (queryAll(data, "SalesTaxCodeQueryRq")) {
        
        // format data for insert into database.
        XMLNode node;

        try {
            XMLParser p(data);
            node = p.getNode();
        }
        catch (ThrownError e) {
            m_sql->logError(e.error, e.data);
        }
        XMLNode nodeChildren;
        node.getNodeFromPath(path, nodeChildren);

        bool findErr = false;

        int nodeCount = nodeChildren.getChildCount();
        for (int i{ 0 }; i < nodeCount; ++i) {

            XMLNode child = nodeChildren.child(i);

            std::string table{ "taxcodes" }; // GENERIC table used for all orders.
            FilterPass inserts;
            FilterPass filter;
            inserts.resize(10); // all the data we'll need to pass.
            inserts.at(0) = { "ListID", child.getChildValue("ListID", "") };
            XMLNode Asset = child.findChild("ItemSalesTaxRef");
            filter.push_back({ "ListID", child.getChildValue("ListID", "") });
            inserts.at(1) = { "TaxID", Asset.getChildValue("ListID", "") };
            inserts.at(2) = { "Type", std::string("ST")};
            inserts.at(3) = { "TimeCreated", child.getChildValue("TimeCreated", "") };
            inserts.at(4) = { "TimeModified", child.getChildValue("TimeModified", "") };
            inserts.at(5) = { "isActive", (long long)(child.getChildValue("IsActive", "") == "true" ? 1 : 0) };
            inserts.at(6) = { "isTaxable", (long long)(child.getChildValue("IsTaxable", "") == "true" ? 1 : 0) };
            inserts.at(7) = { "Name", child.getChildValue("Name", "") };
            inserts.at(8) = { "Description", child.getChildValue("Desc", "")};
            inserts.at(9) = { "TaxRate", 0.0 };

            if (!m_sql->SQLUpdateOrInsert(table, inserts, filter)) {
                std::string err = "Mass Insert/Update tax codes Failed " + table + " File: " + __FILE__ + " Line: " + std::to_string(__LINE__) + " Data: " + data;
                m_sql->logError(ErrorLevel::WARNING, err);
            }
        }
    }
    else {
        return false;
    }

    loopLimit = 0;

    if (queryAll(data, "ItemSalesTaxQueryRq")) {
        
        // format data for insert into database.
        XMLNode node;

        try {
            XMLParser p(data);
            node = p.getNode();
        }
        catch (ThrownError e) {
            m_sql->logError(e.error, e.data);
        }
        XMLNode nodeChildren;
        node.getNodeFromPath(path, nodeChildren);

        bool findErr = false;

        int nodeCount = nodeChildren.getChildCount();
        for (int i{ 0 }; i < nodeCount; ++i) {

            XMLNode child = nodeChildren.child(i);

            std::string table{ "taxcodes" }; // GENERIC table used for all orders.
            FilterPass inserts;
            FilterPass filter;
            inserts.resize(10); // all the data we'll need to pass.
            inserts.at(0) = { "ListID", child.getChildValue("ListID", "") };
            filter.push_back({ "ListID", child.getChildValue("ListID", "") });
            inserts.at(1) = { "TaxID", std::string("")};
            inserts.at(2) = { "Type", std::string("IT") };
            inserts.at(3) = { "TimeCreated", child.getChildValue("TimeCreated", "") };
            inserts.at(4) = { "TimeModified", child.getChildValue("TimeModified", "") };
            inserts.at(5) = { "isActive", (long long)(child.getChildValue("IsActive", "") == "true" ? 1 : 0) };
            inserts.at(6) = { "isTaxable", (long long)1 };
            inserts.at(7) = { "Name", child.getChildValue("Name", "") };
            inserts.at(8) = { "Description", child.getChildValue("ItemDesc", "")};
            inserts.at(9) = { "TaxRate", child.getDoubleOrInt("TaxRate") };

            if (!m_sql->SQLUpdateOrInsert(table, inserts, filter)) {
                std::string err = "Mass Insert/Update sales tax query Failed " + table + " File: " + __FILE__ + " Line: " + std::to_string(__LINE__) + " Data: " + data;
                m_sql->logError(ErrorLevel::WARNING, err);
            }
        }
    }
    else {
        return false;
    }
    return true;
}

bool QBXMLSync::getSalesReps() 
{
    std::string data;
    std::string iterator;

    int loopLimit = 0;

    std::list<int> path = { 0, 0, 0 }; // path direct to top node of data.

    if (queryAll(data, "SalesRepQueryRq")) {
        
        // format data for insert into database.
        XMLNode node;

        try {
            XMLParser p(data);
            node = p.getNode();
        }
        catch (ThrownError e) {
            m_sql->logError(e.error, e.data);
        }
        XMLNode nodeChildren;
        node.getNodeFromPath(path, nodeChildren);

        bool findErr = false;

        int nodeCount = nodeChildren.getChildCount();
        for (int i{ 0 }; i < nodeCount; ++i) {

            XMLNode child = nodeChildren.child(i);

            std::string table{ "salesreps" }; // GENERIC table used for all orders.
            FilterPass inserts;
            FilterPass filter;
            inserts.resize(6); // all the data we'll need to pass.
            inserts.at(0) = { "ListID", child.getChildValue("ListID", "") };
            filter.push_back({ "ListID", child.getChildValue("ListID", "") });
            inserts.at(1) = { "TimeCreated", child.getChildValue("TimeCreated", "") };
            inserts.at(2) = { "TimeModified", child.getChildValue("TimeModified", "") };
            inserts.at(3) = { "Initial", child.getChildValue("Initial", "") };
            inserts.at(4) = { "isActive", (long long)(child.getChildValue("IsActive", "") == "true" ? 1 : 0) };
            XMLNode Asset = child.findChild("SalesRepEntityRef");
            inserts.at(5) = { "Name", Asset.getChildValue("FullName", "") };

            if (!m_sql->SQLUpdateOrInsert(table, inserts, filter)) {
                std::string err = "Mass Insert/Update sales reps Failed " + table + " File: " + __FILE__ + " Line: " + std::to_string(__LINE__) + " Data: " + data;
                m_sql->logError(ErrorLevel::WARNING, err);
            }
        }
    }
    else {
        return false;
    }
    return true;
}

bool QBXMLSync::updateMinMax()
{
    // Set the current running tally for every item in the DB, for total inventory sold for the past year, does not include assemblies.
    std::string request = "UPDATE inventory as i JOIN "
        "(SELECT i.ListID as n, SUM(x.quantity) as q FROM "
        "(SELECT soline.ItemListID as listID, soline.quantity as quantity FROM lineitem as soline, orders as so WHERE "
        "so.OrderType = 'SO' AND so.TxnID = soline.TxnID AND so.TimeCreated < NOW() AND so.TimeCreated > NOW() - INTERVAL 1 YEAR) as x "
        "JOIN inventory as i ON x.listID = i.ListID "
        "GROUP BY x.listID) as u SET i.RunningTally = u.q, i.ReorderPoint = (u.q / 4) WHERE i.ListID = u.n";

    FilterPass inserts;
    inserts.push_back({ "", std::string("") });

    if (!m_sql->SQLComplex(request, inserts, false)) {
        std::string err = "min/max runningtally Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
        m_sql->logError(ErrorLevel::WARNING, err);
        return false;
    }

    // update all reorder points that have assemblies associated with them, and add those numbers to the reorder point.
    request = "UPDATE inventory as i JOIN "
        "(SELECT i.ListID as n, SUM(x.qty) as q FROM "
        "(SELECT(al.quantity * i.runningtally) as qty, al.ListID as listID FROM assembly_lineitems as al, inventory as i WHERE al.ReferenceListID = i.ListID) as x "
        "JOIN inventory as i ON x.listID = i.ListID "
        "GROUP BY x.listID) as u SET i.ReorderPoint = ((u.q + i.RunningTally) / 4) WHERE i.ListID = u.n";

    if (!m_sql->SQLComplex(request, inserts, false)) {
        std::string err = "min/max reorder points Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
        m_sql->logError(ErrorLevel::WARNING, err);
        return false;
    }

    // get all of the products that have been changed, and process them through XML. If there is a Max, we'll need to also update that to <Max ></Max>

    // COMPLEX grab: SELECT I.ListID, I.Name, I.ReorderPoint, I.MaxInv, I.EditSequence FROM INVENTORY as I, minimumdivider as D WHERE SUBSTRING(I.Name, 1, 2) = D.Prefix

    request = "SELECT I.ListID, I.Name, I.ReorderPoint, I.MaxInv, I.EditSequence FROM INVENTORY as I, minimumdivider as D WHERE SUBSTRING(I.Name, 1, 2) = D.Prefix AND I.ProductType = 'ItemInventoryRet'";

    if (!m_sql->SQLComplex(request, inserts, true)) {
        std::string err = "min/max Failed to retrieve products File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
        m_sql->logError(ErrorLevel::WARNING, err);
        return false;
    }
    else {
        // FOR loop to go product by product, updating in quickbooks.

        int loop = 0;

        while (m_sql->nextRow()) {

            if (!isActive()) {
                return false;
            }

            std::string listID = m_sql->getString("ListID");
            std::string editSequence = m_sql->getString("EditSequence");
            bool Max = (m_sql->getInt("MaxInv") > 0 ? true : false);
            int ReorderPoint = m_sql->getInt("ReorderPoint");

            if (Max) {
                if (!updateMinMaxInventory(listID, editSequence, ReorderPoint, Max)) {
                    std::string err = "min/max Failed to update maximum File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
                    m_sql->logError(ErrorLevel::WARNING, err);
                    return false;
                }
            }

            if (!updateMinMaxInventory(listID, editSequence, ReorderPoint, false)) {
                std::string err = "min/max runningtally update Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
                m_sql->logError(ErrorLevel::WARNING, err);
                return false;
            }
        }
    }

    return true;
    
}

bool QBXMLSync::updateMinMaxInventory(const std::string& listID, std::string& editSequence, int reorderPoint, bool max)
{
    // get data required for the update. 
    std::string name = "ItemInventoryModRq";

    XMLNode QBAlter{ name };
    name = "ItemInventoryMod";
    QBAlter.addNode(name, std::list<int>{});
    std::list<int> path{ 0 };


    QBAlter.addNode("ListID", listID, path);
    QBAlter.addNode("EditSequence", editSequence, path);

    if (max) {
        // we'll only add the BLANK max variable.
        QBAlter.addNode("Max", std::string(" "), path);
    }
    else {
        QBAlter.addNode("ReorderPoint", std::to_string(reorderPoint), path);
    }

    std::string xml = XMLParser::createXMLRequest(QBAlter);

    m_req.processRequest(xml);

    std::string returnData = m_req.getResponse();

    XMLNode nodeRet;

    try {
        XMLParser p(returnData);
        nodeRet = p.getNode();
    }
    catch (ThrownError e) {
        m_sql->logError(e.error, e.data);
    }

    XMLNode nodeChildRet;
    nodeRet.getNodeFromPath(std::list<int>{0, 0, 0, 0}, nodeChildRet); // I think? 

    if (nodeChildRet.getName() != "ItemInventoryRet") {
        // in this case, we may have just caught an unknown exception, dump it so we can see, 
        std::string err = "Unknown Child Element File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__) + " XML: " + xml + " return data: " + returnData;
        m_sql->logError((ErrorLevel)2, err);

        return false;
    }

    // on returned value, pull the editsequence and update in sql
    std::string table = "inventory";
    FilterPass updates;

    editSequence = nodeChildRet.getChildValue("EditSequence", "");
    updates.push_back({ "EditSequence", editSequence });
    
    FilterPass filter;
    filter.push_back({ "ListID", listID });

    if (!m_sql->SQLUpdate(table, updates, filter)) {
        std::string err = "Failed to update editsequence: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__) + " For ListID: " + listID + " data: " + returnData;
        m_sql->logError(ErrorLevel::WARNING, err);

        return false;
    }

    return true;
}

// build to allow XML files of MANY products, not just singles. Hopefully faster.
// ------------------------------------------------------------------------------

bool QBXMLSync::updateMinMaxBatch(int batch, int daysLimit, int daysEnd) {

    // check and reaffirm connection.
    if (!m_sql->isConnected()) {
        return false;
    }
    else {
        std::string test = "MySQL Connected " + std::to_string(__LINE__) + " file " + std::string(__FILE__);
        m_sql->logError(ErrorLevel::EXTRA, test);
    }


    FilterPass inserts;
    inserts.push_back({ "", std::string("") });
    
    double daysMult = 1.0f;
    // set the interval string for all updates. 
    std::string interval{ std::to_string(365 + daysEnd) + " DAY" };
    if (daysLimit < 365) {
        interval = std::to_string(daysLimit) + " DAY";
        daysMult = 365.0f / ((double)daysLimit - daysEnd); // account for the adjustment to the end date.
    }

    std::string endInterval{ "" }; // start as a blank string, or assume - X DAY to add to the query strings.
    
    // set if not 0. Value is defined as NOW() - this date.
    if (daysEnd > 0) {
        endInterval = " - INTERVAL " + std::to_string(daysEnd) + " DAY ";
    }

    // reset generic values for runningtally as well as IsEdited. Reorder Point and MaxInv must not be edited until final edit time.
    std::string request = "UPDATE inventory SET RunningTally = 0, IsEdited = 0";
    m_status->minmax = -100;

    if (!m_sql->SQLComplex(request, inserts, false)) {
        std::string err = "min/max tally reset Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
        m_sql->logError(ErrorLevel::WARNING, err);
        return false;
    }
    else {
        std::string err = "min/max tally reset success File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__) + ' ' + request;
        m_sql->logError(ErrorLevel::WARNING, err);
    }

    // get the total lines that need consideration.
    request = "SELECT count(soline.ItemListID) as run FROM lineitem as soline, orders as so WHERE so.OrderType = 'SO' AND so.TxnID = soline.TxnID AND so.TimeCreated < NOW()" + endInterval + " AND so.TimeCreated > NOW() - INTERVAL " + interval;

    m_status->minmax = -99;
    int rtLines = 0;
    int rLim = 10000; // 10000 per run should be good.

    if (!m_sql->SQLComplex(request, inserts, true)) {
        std::string err = "min/max runningtally Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
        m_sql->logError((ErrorLevel)2, err);
        return false;
    }
    else {
       // std::string e = "complex sent";
        //m_sql->logError((ErrorLevel)2, e);
        while (m_sql->nextRow()) {
            rtLines = m_sql->getInt("run");
        }
        //e = "complex complete: " + std::to_string(rtLines);
        //m_sql->logError((ErrorLevel)2, e);

        {
            std::string test = "select count " + std::to_string(__LINE__) + " file " + std::string(__FILE__) + " Request: " + request;
            m_sql->logError(ErrorLevel::EXTRA, test);
        }
    }

    for (int i{ 0 }; i < rtLines; i += rLim) {

        //std::string notes = "testing: rtLines: " + std::to_string(rtLines) + " Lines: " + std::to_string(i);
        //m_sql->logError(ErrorLevel::NOTICE, notes);
        // Set the current running tally for every item in the DB, for total inventory sold for the past year, does not include assemblies.
        request = "UPDATE inventory as i JOIN "
            "(SELECT i.ListID as n, SUM(x.quantity) as q, i.ReorderPoint as rp FROM "
            "(SELECT soline.ItemListID as listID, soline.quantity as quantity FROM lineitem as soline, orders as so WHERE "
            "so.OrderType = 'SO' AND so.TxnID = soline.TxnID AND so.TimeCreated < NOW()" + endInterval + " AND so.TimeCreated > NOW() - INTERVAL " + interval + " LIMIT "
            + std::to_string((rtLines - i >= rLim ? rLim : rtLines - i)) + " OFFSET " + std::to_string(i) +") as x "
            "JOIN inventory as i ON x.listID = i.ListID "
            "GROUP BY x.listID) as u "
            "SET i.RunningTally = i.RunningTally + u.q "
            "WHERE i.ListID = u.n";

        //m_sql->logError(ErrorLevel::NOTICE, request);

        if (!m_sql->SQLComplex(request, inserts, false)) {
            std::string err = "min/max runningtally Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
            m_sql->logError((ErrorLevel)2, err);
            return false;
        }
        else {
            std::string test = "updated runningtally  " + std::to_string(__LINE__) + " file " + std::string(__FILE__) + " request: " + request;
            m_sql->logError(ErrorLevel::EXTRA, test);
        }

    }
    // to preserve usability, this additional call will check the isEdited position and set the reorder point for the next section. 
    
    //i.ReorderPoint = (u.q / 4), i.IsEdited = IF(u.rp != (u.q / 4), 1, i.IsEdited) 
    request = "UPDATE inventory as i JOIN "
        "(SELECT ListID as l, RunningTally as r FROM inventory) as x "
        "SET i.ReorderPoint = (RunningTally / 4 * " + std::to_string(daysMult) + " ), i.IsEdited = IF(x.r != i.RunningTally, 1, IsEdited) "
        "WHERE i.ListID = x.l";

    if (!m_sql->SQLComplex(request, inserts, false)) {
        std::string err = "Reorder Point update Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
        m_sql->logError((ErrorLevel)2, err);
        return false;
    }
    else {
        std::string test = "updated reorder point  " + std::to_string(__LINE__) + " file " + std::string(__FILE__) + " request: " + request;
        m_sql->logError(ErrorLevel::EXTRA, test);
    }

    request = "UPDATE inventory SET MaxInv = IF(RunningTally > ReorderPoint, IF(RunningTally > MaxInv, RunningTally, MaxInv), IF(MaxInv > ReorderPoint * 4, MaxInv, ReorderPoint * 4)) WHERE MaxInv > -1";
    
    m_status->minmax = -98;

    if (!m_sql->SQLComplex(request, inserts, false)) {
        std::string err = "min/max max inv Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
        m_sql->logError((ErrorLevel)2, err);
        return false;
    }
    else {
        std::string test = "updated max inv  " + std::to_string(__LINE__) + " file " + std::string(__FILE__) + " request: " + request;
        m_sql->logError(ErrorLevel::EXTRA, test);
    }

    // update all reorder points that have assemblies associated with them, and add those numbers to the reorder point.
    request = "UPDATE inventory as i JOIN "
        "(SELECT i.ListID as n, SUM(x.qty) as q, i.ReorderPoint as rp FROM "
        "(SELECT (al.Quantity * i.RunningTally) as qty, al.ListID as listID FROM assembly_lineitems as al, inventory as i WHERE al.ReferenceListID = i.ListID) as x "
        "JOIN inventory as i ON x.listID = i.ListID "
        "GROUP BY x.listID) as u "
        "SET i.ReorderPoint = ((u.q + i.RunningTally) / 4 * " + std::to_string(daysMult) + " ), i.IsEdited = IF(u.rp != ((u.q + i.RunningTally) / 4 * " + std::to_string(daysMult) + " ), 1, i.IsEdited) "
        "WHERE i.ListID = u.n";

    m_status->minmax = -97;

    if (!m_sql->SQLComplex(request, inserts, false)) {
        std::string err = "min/max reorder point Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
        m_sql->logError((ErrorLevel)2, err);
        return false;
    }
    else {
        std::string test = "updated reorder point  " + std::to_string(__LINE__) + " file " + std::string(__FILE__) + " request: " + request;
        m_sql->logError(ErrorLevel::EXTRA, test);
    }

    request = "UPDATE inventory SET MaxInv = ReorderPoint * 4 WHERE MaxInv > -1 AND MaxInv < ReorderPoint";

    m_status->minmax = -96;

    if (!m_sql->SQLComplex(request, inserts, false)) {
        std::string err = "min/max maxinv 2 Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
        m_sql->logError((ErrorLevel)2, err);
        return false;
    }
    else {
        std::string test = "updated maxinv 2  " + std::to_string(__LINE__) + " file " + std::string(__FILE__) + " request: " + request;
        m_sql->logError(ErrorLevel::EXTRA, test);
    }

    // Start with updating all MAX products. We'll need to get then set all the values in a dedicated loop. 
//    request = "SELECT I.ListID, I.Name, I.ReorderPoint, I.MaxInv, I.EditSequence FROM INVENTORY as I, minimumdivider as D WHERE SUBSTRING(I.Name, 1, 2) = D.Prefix AND I.ProductType = 'ItemInventoryRet' AND MaxInv > 0";
    //request = "SELECT I.ListID, I.Name, I.ReorderPoint, I.MaxInv, I.EditSequence FROM INVENTORY as I WHERE I.ProductType = 'ItemInventoryRet' AND MaxInv > -1";
    request = "SELECT I.ListID, I.Name, I.ReorderPoint, I.MaxInv, I.EditSequence FROM inventory as I, minimumdivider as D WHERE SUBSTRING(I.Name, 1, 2) = D.Prefix AND I.ProductType = 'ItemInventoryRet' AND I.IsEdited = 1 AND I.MaxInv > -1";
    
    m_status->minmax = -95;

    if (!m_sql->SQLComplex(request, inserts, true)) {
        std::string err = "min/max retreive maxinv Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
        m_sql->logError((ErrorLevel)2, err);
        return false;
    }
    else {
        // generate the lists to be updated.
        std::vector<std::string> listIDs;
        std::vector<std::string> editSequences;
        std::vector<int> ReorderPoints;
        std::vector<int> maxInv;
        listIDs.reserve(batch);
        editSequences.reserve(batch);
        ReorderPoints.reserve(batch);
        // max will be manually set.

        {
         std::string test = "updated retreive maxinv  " + std::to_string(__LINE__) + " file " + std::string(__FILE__) + " request: " + request;
         m_sql->logError(ErrorLevel::EXTRA, test);
        }

        // while loop for data assignment
        //int loop = 0;
        m_status->minmax = 1;
        
        int countToUpdate = 0;

        while (m_sql->nextRow()) {
            
            if (!isActive()) {
                return false;
            }

            listIDs.push_back(m_sql->getString("ListID"));
            editSequences.push_back(m_sql->getString("EditSequence"));
            ReorderPoints.push_back(m_sql->getInt("ReorderPoint"));
            maxInv.push_back(m_sql->getInt("MaxInv"));

            if (countToUpdate++ >= batch) {
                m_status->minmax++;
                
                // send through the update here. 

                if (!updateMinMaxInventory(listIDs, editSequences, maxInv, true)) { // true for always maxiumums.
                    std::string err = "min/max maxinv update Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
                    m_sql->logError((ErrorLevel)2, err);
                    return false;
                }
                listIDs.clear();
                editSequences.clear();
                ReorderPoints.clear();
                countToUpdate = 0;

            }
        }

        // final check determines if there is anything outstanding to be dealt with.
        if (!listIDs.empty()) {
            if (!updateMinMaxInventory(listIDs, editSequences, maxInv, true)) { // true for always maxiumums.
                std::string err = "min/max maxinv update Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
                m_sql->logError((ErrorLevel)2, err);
                return false;
            }
        }
    }


    // get all of the products that have been changed, and process them through XML. If there is a Max, we'll need to also update that to <Max ></Max>
    
    request = "SELECT I.ListID, I.Name, I.ReorderPoint, I.MaxInv, I.EditSequence FROM inventory as I, minimumdivider as D WHERE SUBSTRING(I.Name, 1, 2) = D.Prefix AND I.ProductType = 'ItemInventoryRet' AND I.IsEdited = 1";

    m_status->minmax = -94;

    if (!m_sql->SQLComplex(request, inserts, true)) {
        std::string err = "min/max retreive minimums Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
        m_sql->logError((ErrorLevel)2, err);
        return false;
    }
    else {
        // generate the lists to be updated.
        std::vector<std::string> listIDs;
        std::vector<std::string> editSequences;
        std::vector<int> ReorderPoints;
        std::vector<int> MaxInv;
        listIDs.reserve(batch);
        editSequences.reserve(batch);
        ReorderPoints.reserve(batch);
        // max will be manually set.

        {
         std::string test = "updated minimums retreives  " + std::to_string(__LINE__) + " file " + std::string(__FILE__) + " request: " + request;
         m_sql->logError(ErrorLevel::EXTRA, test);
        }

        // while loop for data assignment
        //int loop = 0;
        m_status->minmax = 1;

        int countToUpdate = 0;

        while (m_sql->nextRow()) {
            
            if (!isActive()) {
                return false;
            }

            listIDs.push_back(m_sql->getString("ListID"));
            editSequences.push_back(m_sql->getString("EditSequence"));
            ReorderPoints.push_back(m_sql->getInt("ReorderPoint"));
            MaxInv.push_back(m_sql->getInt("MaxInv"));

            if (countToUpdate++ >= batch) {

                m_status->minmax++;
                
                // send through the update here. 

                if (!updateMinMaxInventory(listIDs, editSequences, ReorderPoints, false)) { // true for always maxiumums.
                    std::string err = "min/max reorder point update Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
                    m_sql->logError((ErrorLevel)2, err);
                    return false;
                }
                listIDs.clear();
                editSequences.clear();
                ReorderPoints.clear();
                countToUpdate = 0;

            }
        }

        // final check determines if there is anything outstanding to be dealt with.
        if (!listIDs.empty()) {
            if (!updateMinMaxInventory(listIDs, editSequences, ReorderPoints, false)) { // true for always maxiumums.
                std::string err = "min/max reporder point update Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
                m_sql->logError((ErrorLevel)2, err);
                return false;
            }
        }
    }

     {
     std::string test = "minmax completed.  " + std::to_string(__LINE__) + " file " + std::string(__FILE__);
     m_sql->logError(ErrorLevel::EXTRA, test);
     }

    *m_isActive = false;
    return true;
}

bool QBXMLSync::updateMinMaxInventory(const std::vector<std::string>& listID, std::vector<std::string>& editSequence, std::vector<int> newValue, bool max)
{
    // must be done manually and bypass the main function due to the unique construction requirements.
    std::string request{ "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                        "<?qbxml version=\"13.0\"?>\n"
                        "<QBXML>\n"
                        "\t<QBXMLMsgsRq onError=\"stopOnError\">" };
    // get data required for the update.
    
    std::string tblname = "ItemQueryRq";
    XMLNode QBQuery{ tblname };

    // for each, add a mod node here.
    for (int i{ 0 }; i < listID.size(); ++i) {
        std::string name = "ItemInventoryModRq";
        XMLNode QBAlter{ name };

        name = "ItemInventoryMod";
        QBAlter.addNode(name, std::list<int>{});
        std::list<int> path{ 0 };   // for current node 

        QBAlter.addNode("ListID", listID.at(i), path);
        QBAlter.addNode("EditSequence", editSequence.at(i), path);

        if (max) {
            // we'll only add the BLANK max variable.
            //QBAlter.addNode("Max", std::to_string(newValue.at(i)), path);
            if (newValue.at(i) > -1) {
                QBAlter.addNullNode("Max", path);
            }
            else {
                continue; // don't add this one.
            }
        }
        else {
            QBAlter.addNode("ReorderPoint", std::to_string(newValue.at(i)), path);
        }

        // simultaneously, let'd create the Rq for the inventory query.
        QBQuery.addNode("ListID", listID.at(i), std::list<int>{});

        std::string xml;
        
        QBAlter.writeXML(xml, 2);

        request.append(xml);
    }

    // close the node tree, full call here.
    request.append("\n\t</QBXMLMsgsRq>\n"
        "</QBXML>");

    {
        std::string test = "minmax req: " + std::to_string(__LINE__) + " file " + std::string(__FILE__) + " REQUEST: " + request;
        m_sql->logError(ErrorLevel::EXTRA, test);
    }

    // send the completed request, and get the response data. 
    m_req.processRequest(request);
    
    // get and process return data.
    std::string returnData = m_req.getResponse();

    XMLNode nodeRet;

    try {
        XMLParser p(returnData);
        nodeRet = p.getNode();
    }
    catch (ThrownError e) {
        m_sql->logError(e.error, e.data);
    }

    {
        std::string test = "minmax resp: " + std::to_string(__LINE__) + " file " + std::string(__FILE__) + " response: " + returnData;
        m_sql->logError(ErrorLevel::EXTRA, test);
    }

    XMLNode respParentNode; // each child is the ones we need to check
    nodeRet.getNodeFromPath(std::list<int>{0, 0}, respParentNode);
    for (int i{ 0 }; i < respParentNode.getChildCount(); ++i) {
        XMLNode child = respParentNode.child(i);
        std::string code = "statusCode";
        if (child.getAttribute(code).getIntData() != 0) {
        //   std::cout << "ERROR: Update Node return bad.\n";
            child.write();
            return false;
        }
    }

    // SINCE response data is terribly lacking, for reasons undisclosed, we'll need to do a manual pull for the inventory call. 
    std::string xml = XMLParser::createXMLRequest(QBQuery);
    m_req.processRequest(xml);
    returnData = m_req.getResponse();

    try {
        XMLParser pp(returnData);
        nodeRet = pp.getNode();
    }
    catch (ThrownError e) {
        m_sql->logError(e.error, e.data);
    }
    
    nodeRet.getNodeFromPath(std::list<int>{0, 0}, respParentNode);

    for (int i{ 0 }; i < respParentNode.getChildCount(); ++i) {
        XMLNode child = respParentNode.child(i);
        std::string code = "statusCode";
        if (child.getAttribute(code).getIntData() != 0) {
            std::string err = "Node Return Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__) + " Return Data: " + returnData + " Request: " + request;
            m_sql->logError(ErrorLevel::WARNING, err);

            return false;
        }
    }

    XMLNode nodeChildRet;
    nodeRet.getNodeFromPath(std::list<int>{0, 0, 0}, nodeChildRet); // this gives the Rs node, which has the Ret children.

    std::vector<FilterPass> insertsInventory;

    int nodeCount = nodeChildRet.getChildCount();

    // save space for all insert lines, assuming that there is sufficient numbers.
    insertsInventory.resize(nodeCount);

    for (int i{ 0 }; i < nodeCount; ++i) {
        // for each child, convert and output. 

        XMLNode child = nodeChildRet.child(i);

        insertsInventory.at(i).resize(2); // all the data we'll need to pass.
        insertsInventory.at(i).at(0) = { "ListID", child.getChildValue("ListID", "") };
        insertsInventory.at(i).at(1) = { "EditSequence", child.getChildValue("EditSequence", "") };
    }

    // this is techincally a "hack" to force through updates, this shouldn't have any impact on the final inserts, or create "blank" inserts.
    std::string table = "inventory";

    // run the actual inserts here 

    if (insertsInventory.size() > 0) {
        if (!m_sql->SQLMassInsert(table, insertsInventory)) {
            std::string err = "min/max inventory adjust Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
            m_sql->logError(ErrorLevel::WARNING, err);
            return false;
        }
    }

    return true;
}

bool QBXMLSync::timeReset(SQLTable table, int Y, int M, int D, int H, int m, int S) {
    // reset the time for all event markers.
    //  1970-01-01 12:00:00
    std::stringstream ss;
    ss << std::setfill('0') << (Y < 1970 ? "1970" : std::to_string(Y)) << '-'
        << std::setw(2) << M << '-'
        << std::setw(2) << D << ' '
        << std::setw(2) << H << ':'
        << std::setw(2) << m << ':'
        << std::setw(2) << S;
    std::string datetime = ss.str();

    return timeReset(table, datetime);
}

bool QBXMLSync::timeReset(SQLTable table, std::string datetime) {
    // reset time, allow for manual string.
    FilterPass filters;
    switch (table){
        case SQLTable::customer:
            filters.push_back({ "ConfigKey",std::string("customers") });
            break;
        case SQLTable::estimate:
            filters.push_back({ "ConfigKey",std::string("estimates") });
            break;
        case SQLTable::inventory:
            filters.push_back({ "ConfigKey",std::string("inventory") });
            break;
        case SQLTable::invoice:
            filters.push_back({ "ConfigKey",std::string("invoices") });
            break;
        case SQLTable::purchaseorder:
            filters.push_back({ "ConfigKey",std::string("purchaseorders") });
            break;
        case SQLTable::salesorder:
            filters.push_back({ "ConfigKey",std::string("salesorders") });
            break;
    }
    
    FilterPass updates;
    updates.push_back({ "time", datetime });

    return m_sql->SQLUpdate("sync_config", updates, filters);
}

bool QBXMLSync::updateInventoryPartnumbers(int limit, std::string type)
{
    // will run a single time to update all parts of the DB inventory. Will require that inventory is synced prior to running.

    FilterPass inserts;
    
    // get all of the products that have been changed, and process them through XML. If there is a Max, we'll need to also update that to <Max ></Max>

    std::string request = "SELECT ListID, Name, EditSequence FROM INVENTORY WHERE ProductType = ? AND SUBSTRING(Name, 3, 1) = '-' AND IsActive = 1";

    inserts.push_back({ "ProductType", std::string(type == "ItemInventoryMod" ? "ItemInventoryRet" : "ItemInventoryAssemblyRet")});

    if (!m_sql->SQLComplex(request, inserts, true)) {
        std::string err = "part number update Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
        m_sql->logError((ErrorLevel)2, err);
        return false;
    }
    else {
        std::vector<std::string> listIDs;
        std::vector<std::string> editSequences;
        std::vector<std::string> renames;
        listIDs.reserve(limit);
        editSequences.reserve(limit);
        renames.reserve(limit);

        // generate the lists to be updated.
        // while loop for data assignment

        int countToUpdate = 0;

        

        while (m_sql->nextRow()) {

            if (!isActive()) {
                return false;
            }

            listIDs.push_back(m_sql->getString("ListID"));
            editSequences.push_back(m_sql->getString("EditSequence"));
            
            //name edit occurs here, based on name actual length.
            std::string newName = m_sql->getString("Name");
            
            if (newName[2] == '-') {
                if (newName.size() >= 31) {
                    // longer names will replace the first - with *
                    newName[2] = '*';
                }
                else {
                    // short names will add a 1 in front of the name.
                    newName = "1" + newName;
                }
            }
            renames.push_back(newName);

            if (countToUpdate++ >= limit) {
                // send through the update here.

                if (!updatePartsBatch(listIDs, editSequences, renames, type)) { // true for always maxiumums.
                    std::string err = "rename update Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
                    m_sql->logError((ErrorLevel)2, err);
                    return false;
                }
                listIDs.clear();
                editSequences.clear();
                renames.clear();
                countToUpdate = 0;
                }
            }
            

        // final check determines if there is anything outstanding to be dealt with.
        if (!listIDs.empty()) {
            if (!updatePartsBatch(listIDs, editSequences, renames, type)) { // true for always maxiumums.
                std::string err = "rename update Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
                m_sql->logError((ErrorLevel)2, err);
                return false;
            }
        }
    }

    return true;
}

bool QBXMLSync::updatePartsBatch(const std::vector<std::string>& listID, std::vector<std::string>& editSequence, std::vector<std::string> newValue, std::string requestType) {
    
    // must be done manually and bypass the main function due to the unique construction requirements.
    std::string request{ "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
                        "<?qbxml version=\"13.0\"?>\n"
                        "<QBXML>\n"
                        "\t<QBXMLMsgsRq onError=\"stopOnError\">" };

    // get data required for the update.

    std::string tblname = "ItemQueryRq";
    XMLNode QBQuery{ tblname };

    // for each, add a mod node here.
    for (int i{ 0 }; i < listID.size(); ++i) {
        std::string name = requestType + "Rq";
        XMLNode QBAlter{ name };

        QBAlter.addNode(requestType, std::list<int>{});
        std::list<int> path{ 0 };   // for current node 

        QBAlter.addNode("ListID", listID.at(i), path);
        QBAlter.addNode("EditSequence", editSequence.at(i), path);
        QBAlter.addNode("Name", newValue.at(i), path);

        // simultaneously, let'd create the Rq for the inventory query.
        QBQuery.addNode("ListID", listID.at(i), std::list<int>{});

        std::string xml;

        QBAlter.writeXML(xml, 2);

        request.append(xml);
    }
    
    // close the node tree, full call here.
    request.append("\n\t</QBXMLMsgsRq>\n"
        "</QBXML>");

    // send the completed request, and get the response data. 
    m_req.processRequest(request);

    // get and process return data.
    std::string returnData = m_req.getResponse();

    XMLNode nodeRet;

    try {
        XMLParser p(returnData);
        nodeRet = p.getNode();
    }
    catch (ThrownError e) {
        m_sql->logError(e.error, e.data);
    }

    XMLNode respParentNode; // each child is the ones we need to check
    nodeRet.getNodeFromPath(std::list<int>{0, 0}, respParentNode);
    for (int i{ 0 }; i < respParentNode.getChildCount(); ++i) {
        XMLNode child = respParentNode.child(i);
        std::string code = "statusCode";
        if (child.getAttribute(code).getIntData() != 0) {
            //std::cout << "ERROR: Update Node return bad.\n";
            child.write();
            return false;
        }
    }

    // SINCE response data is terribly lacking, for reasons undisclosed, we'll need to do a manual pull for the inventory call. 
    std::string xml = XMLParser::createXMLRequest(QBQuery);
    m_req.processRequest(xml);
    returnData = m_req.getResponse();

    try {
        XMLParser pp(returnData);
        nodeRet = pp.getNode();
    }
    catch (ThrownError e) {
        m_sql->logError(e.error, e.data);
    }

    nodeRet.getNodeFromPath(std::list<int>{0, 0}, respParentNode);

    for (int i{ 0 }; i < respParentNode.getChildCount(); ++i) {
        XMLNode child = respParentNode.child(i);
        std::string code = "statusCode";
        if (child.getAttribute(code).getIntData() != 0) {
            std::string err = "Node Return Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__) + " Return Data: " + returnData + " Request: " + request;
            m_sql->logError(ErrorLevel::WARNING, err);

            return false;
        }
    }

    XMLNode nodeChildRet;
    nodeRet.getNodeFromPath(std::list<int>{0, 0, 0}, nodeChildRet); // this gives the Rs node, which has the Ret children.

    std::vector<FilterPass> insertsInventory;

    int nodeCount = nodeChildRet.getChildCount();

    // save space for all insert lines, assuming that there is sufficient numbers.
    insertsInventory.resize(nodeCount);

    for (int i{ 0 }; i < nodeCount; ++i) {
        // for each child, convert and output. 

        XMLNode child = nodeChildRet.child(i);

        insertsInventory.at(i).resize(2); // all the data we'll need to pass.
        insertsInventory.at(i).at(0) = { "ListID", child.getChildValue("ListID", "") };
        insertsInventory.at(i).at(1) = { "EditSequence", child.getChildValue("EditSequence", "") };
        insertsInventory.at(i).at(1) = { "Name", child.getChildValue("Name", "") };
    }

    // this is techincally a "hack" to force through updates, this shouldn't have any impact on the final inserts, or create "blank" inserts.
    std::string table = "inventory";

    // run the actual inserts here 

    if (insertsInventory.size() > 0) {
        if (!m_sql->SQLMassInsert(table, insertsInventory)) {
            std::string err = "part number inventory adjust Failed File: " + std::string(__FILE__) + " Line: " + std::to_string(__LINE__);
            m_sql->logError(ErrorLevel::WARNING, err);
            return false;
        }
    }

    return true;
}