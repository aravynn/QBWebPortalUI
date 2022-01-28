#include "SQLControl.h"

// Create all database tables in the case of failure no DB tables.
// ---------------------------------------------------------------
void SQLControl::initializeDatabase()
{
    // TO BE CREATED -----------------------------------------------------------------------------------------------------------!!
    //std::cout << "Database not created. Creating... \n";

    // string for creating the DB.
   // Tables to add: 
   // Users
    std::string REQUEST{
        "CREATE TABLE users (\n"
        "ID int NOT NULL AUTO_INCREMENT,\n"
        "User char(200) NOT NULL DEFAULT '',\n"
        "Pass char(200) NOT NULL DEFAULT '',\n"
        "Salt char(200) NOT NULL DEFAULT '',\n"
        "CustomerID char(40) DEFAULT '',\n"
        "RepID int DEFAULT -1,\n"
        "PRIMARY KEY (ID)"
        ")"
    };

    m_stmt.reset(m_con->prepareStatement(REQUEST));
    if (m_stmt->execute()) {
        std::string error{ "Failed to create table USERS" + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
        ThrownError e(ErrorLevel::ERR, error);
        throw e;
    }

    std::string table{ "users" };
    FilterPass inserts;
    inserts.resize(1);
    // forces a generic user. 
    inserts.at(0) = { std::string("User"), std::string("QBXML") };
    if (!SQLInsert(table, inserts, false)) {
        std::string error{ "Failed to create user on table users. line:" + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
        ThrownError e(ErrorLevel::ERR, error);
        throw e;
    }

    // Inventory
    REQUEST =
        "CREATE TABLE inventory (\n"
        "ListID char(40) NOT NULL,\n"
        "TimeCreated datetime DEFAULT '1000-01-01 00:00:00',\n"
        "TimeModified datetime DEFAULT '1000-01-01 00:00:00',\n"
        "Name char(200) DEFAULT '',\n"
        "SKU char(200) DEFAULT '',\n"
        "ProductType char(60) DEFAULT '',\n"
        "InStock double DEFAULT 0.0,\n"
        "OnOrder double DEFAULT 0.0,\n"
        "OnSalesOrder double DEFAULT 0.0,\n"
        "Price double DEFAULT 0.0,\n"
        "Cost double DEFAULT 0.0, \n"
        "Details text,\n"
        "PurchaseOrderDetails text,\n"
        "UOfM char(100) DEFAULT 'Ea',\n"
        "IsActive tinyint DEFAULT 1,\n"
        "ReorderPoint int DEFAULT 0,\n"
        "MaxInv int DEFAULT -1,\n"
        "RunningTally double DEFAULT 0.0,\n"
        "EditSequence char(50) DEFAULT '',\n"
        "IsEdited tinyint DEFAULT 0, \n"
        "PRIMARY KEY (ListID)\n"
        ")";

    m_stmt.reset(m_con->prepareStatement(REQUEST));
    if (m_stmt->execute()) {
        std::string error{ "Failed to create table INVENTORY" + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
        ThrownError e(ErrorLevel::ERR, error);
        throw e;
    }

    // Assembly Line Items
    REQUEST =
        "CREATE TABLE assembly_lineitems (\n"
        "ID int NOT NULL AUTO_INCREMENT,\n"
        "ListID char(40) NOT NULL,\n"
        "ReferenceListID char(40) NOT NULL,\n"
        "Quantity double DEFAULT 1.0, \n"
        "PRIMARY KEY (ID)\n"
        ")";

    m_stmt.reset(m_con->prepareStatement(REQUEST));
    if (m_stmt->execute()) {
        std::string error{ "Failed to create table ASSEMBLYLINEITEMS" + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
        ThrownError e(ErrorLevel::ERR, error);
        throw e;
    }

    // MinDivider & Content
    REQUEST =
        "CREATE TABLE minimumdivider (\n"
        "ID int NOT NULL AUTO_INCREMENT,\n"
        "Prefix char(2) NOT NULL DEFAULT '',\n"
        "Divider int NOT NULL DEFAULT 4, \n"
        "PRIMARY KEY (ID)\n"
        ")";

    m_stmt.reset(m_con->prepareStatement(REQUEST));
    if (m_stmt->execute()) {
        std::string error{ "Failed to create table MINIMUMDIVIDER" + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
        ThrownError e(ErrorLevel::ERR, error);
        throw e;
    }

    // add the dividers by default.
    std::vector<std::string> prefixes{ "02", "03", "04", "06", "09", "15", "23", "26", "31", "33", "35", "36", "38", "57" };
    inserts.resize(2);
    inserts.at(0) = { std::string("Prefix"), std::string("--") };
    inserts.at(1) = { std::string("Divider"), (long long)4 };

    table = "minimumdivider";

    for (std::string p : prefixes) {
        inserts.at(0).second.str = p;
        SQLInsert(table, inserts, false);
    }

    // Salesorders, estimates, invoices.
    REQUEST =
        "CREATE TABLE orders (\n"
        "TxnID char(40) NOT NULL,\n"
        "OrderType char(2) NOT NULL DEFAULT 'NA', \n" // SO, PO, or ES(timate)
        "TimeCreated datetime DEFAULT '1000-01-01 00:00:00', \n"
        "TimeModified datetime DEFAULT '1000-01-01 00:00:00', \n"
        "TxnDate date DEFAULT '1000-01-01', \n"
        "DueDate date DEFAULT '1000-01-01', \n"
        "ShipDate date DEFAULT '1000-01-01', \n"
        "TermsID char(40) NOT NULL DEFAULT '', \n"
        "CustomerListID char(40) NOT NULL, \n"
        "ShipAddressID int NOT NULL DEFAULT -1, \n"
        "BillAddressID int NOT NULL DEFAULT -1, \n"
        "ShipTypeID char(40) NOT NULL DEFAULT '', \n"
        "TaxCodeID char(40) NOT NULL DEFAULT '', \n"
        "SalesRepID char(40) NOT NULL DEFAULT '', \n"
        "RefNumber char(50) NOT NULL DEFAULT '', \n" // PO, SO, or invoice number
        "PONumber char(50) NOT NULL DEFAULT '', \n"
        "CustomerFullName char(255) DEFAULT '', \n"
        "FOB char(50) NOT NULL DEFAULT '', \n"
        "Subtotal double DEFAULT 0.0, \n"
        "SalesTaxTotal double DEFAULT 0.0, \n"
        "SaleTotal double DEFAULT 0.0, \n"
        "Balance double DEFAULT 0.0, \n"
        "IsPending int(1) NOT NULL DEFAULT 1, \n"
        "Status int(1) NOT NULL DEFAULT 1, \n" // isActive(1) or !isClosed(0),
        "IsInvoiced int (1) NOT NULL DEFAULT 0, \n"
        "Notes text, \n"
        "PRIMARY KEY (TxnID)\n"
        ")";

    m_stmt.reset(m_con->prepareStatement(REQUEST));
    if (m_stmt->execute()) {
        std::string error{ "Failed to create table ORDERS" + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
        ThrownError e(ErrorLevel::ERR, error);
        throw e;
    }

    // SalesOrderLineitems
    REQUEST =
        "CREATE TABLE lineitem (\n"
        "LineID char(40) NOT NULL, \n"
        "TxnID char(40) NOT NULL, \n"
        "ItemListID char(40) NOT NULL, \n"
        "Description text, \n"
        "Quantity double DEFAULT 0.0, \n"
        "Rate double DEFAULT 0.0, \n"
        "UnitOfMeasure char(50) DEFAULT '', \n"
        "TaxCode char(40) NOT NULL DEFAULT '', \n"
        "UniqueIdentifier char(255) DEFAULT '', \n" // will be used for serial numbers, etc.
        "Invoiced int DEFAULT 0, \n" // only for sales orders. 
        "PRIMARY KEY (LineID)\n"
        ")";

    m_stmt.reset(m_con->prepareStatement(REQUEST));
    if (m_stmt->execute()) {
        std::string error{ "Failed to create table LINEITEM" + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
        ThrownError e(ErrorLevel::ERR, error);
        throw e;
    }

    // customers
    REQUEST =
        "CREATE TABLE customers (\n"
        "ListID char(40) NOT NULL, \n"
        "TimeCreated datetime DEFAULT '1000-01-01 00:00:00', \n"
        "TimeModified datetime DEFAULT '1000-01-01 00:00:00', \n"
        "ShipAddressID int NOT NULL DEFAULT -1, \n"
        "BillAddressID int NOT NULL DEFAULT -1, \n"
        "TermsID char(40) NOT NULL DEFAULT '', \n"
        "TaxCodeID char(40) NOT NULL DEFAULT '', \n"
        "SalesRepID char(40) NOT NULL DEFAULT '', \n"
        "PriceLevelID char(40) NOT NULL DEFAULT '', \n"
        "isActive int(1) NOT NULL DEFAULT 1, \n" // isActive    
        "FullName char(255) NOT NULL DEFAULT '', \n"
        "CompanyName char(255) NOT NULL DEFAULT '', \n"               
        "PhoneNumber char(30) DEFAULT '', \n"
        "AltPhoneNumber char(30) DEFAULT '', \n"
        "FaxNumber char(30) DEFAULT '', \n"
        "EmailAddress char(255) DEFAULT '', \n" // will be used to match when creating a new user.       
        "Balance double DEFAULT 0.0, \n"
        "CreditLimit double DEFAULT 0.0, \n"     
        "PRIMARY KEY (ListID) \n"
        ")";

    m_stmt.reset(m_con->prepareStatement(REQUEST));
    if (m_stmt->execute()) {
        std::string error{ "Failed to create table CUSTOMERS" + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
        ThrownError e(ErrorLevel::ERR, error);
        throw e;
    }


    // Price Level Rates
    REQUEST =
        "CREATE TABLE price_level (\n"
        "ListID char(40) NOT NULL, \n"
        "TimeCreated datetime DEFAULT '1000-01-01 00:00:00', \n"
        "TimeModified datetime DEFAULT '1000-01-01 00:00:00', \n"
        "isActive int(1) NOT NULL DEFAULT 1, \n" // isActive    
        "Name char(255) NOT NULL DEFAULT '', \n"
        "DiscountRate double DEFAULT 0.0, \n" //PriceLevelFixedPercentage
        "PRIMARY KEY (ListID) \n"
        ")";

    m_stmt.reset(m_con->prepareStatement(REQUEST));
    if (m_stmt->execute()) {
        std::string error{ "Failed to create table PRICE_LEVEL" + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
        ThrownError e(ErrorLevel::ERR, error);
        throw e;
    }

    // Sales Terms
    REQUEST =
        "CREATE TABLE salesterms (\n"
        "ListID char(40) NOT NULL, \n"
        "TimeCreated datetime DEFAULT '1000-01-01 00:00:00', \n"
        "TimeModified datetime DEFAULT '1000-01-01 00:00:00', \n"
        "isActive int(1) NOT NULL DEFAULT 1, \n" // isActive    
        "Name char(255) NOT NULL DEFAULT '', \n"
        "DaysDue int DEFAULT 0, \n"
        "DiscountDays int DEFAULT 0, \n"
        "DiscountRate double DEFAULT 0.0, \n" //Discountpct
        "PRIMARY KEY (ListID) \n"
        ")";

    m_stmt.reset(m_con->prepareStatement(REQUEST));
    if (m_stmt->execute()) {
        std::string error{ "Failed to create table SALES_TERMS" + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
        ThrownError e(ErrorLevel::ERR, error);
        throw e;
    }
    
    // Tax Codes
    REQUEST =
        "CREATE TABLE taxcodes (\n"
        "ListID char(40) NOT NULL, \n"
        "TaxID char(40) NOT NULL DEFAULT '', \n"
        "Type char(2) NOT NULL, \n" // ST for sales tax, IT for Item Tax
        "TimeCreated datetime DEFAULT '1000-01-01 00:00:00', \n"
        "TimeModified datetime DEFAULT '1000-01-01 00:00:00', \n"
        "isActive int(1) NOT NULL DEFAULT 1, \n" // isActive    
        "isTaxable int(1) NOT NULL DEFAULT 1, \n" // isActive    
        "Name char(255) NOT NULL DEFAULT '', \n"
        "Description text, \n"
        "TaxRate double NOT NULL DEFAULT 0.0, \n"
        "PRIMARY KEY (ListID) \n"
        ")";

    m_stmt.reset(m_con->prepareStatement(REQUEST));
    if (m_stmt->execute()) {
        std::string error{ "Failed to create table TAXCODES" + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
        ThrownError e(ErrorLevel::ERR, error);
        throw e;
    }
    
    // Sales reps
    // Tax Codes
    REQUEST =
        "CREATE TABLE salesreps (\n"
        "ListID char(40) NOT NULL, \n"
        "TimeCreated datetime DEFAULT '1000-01-01 00:00:00', \n"
        "TimeModified datetime DEFAULT '1000-01-01 00:00:00', \n"
        "Initial char(20) NOT NULL DEFAULT '', \n"
        "isActive int(1) NOT NULL DEFAULT 1, \n" 
        "Name char(255) NOT NULL DEFAULT '', \n"
        "PRIMARY KEY (ListID) \n"
        ")";

    m_stmt.reset(m_con->prepareStatement(REQUEST));
    if (m_stmt->execute()) {
        std::string error{ "Failed to create table SALESREPS" + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
        ThrownError e(ErrorLevel::ERR, error);
        throw e;
    }

    // Shipping Type (+ tracking number)
    REQUEST =
        "CREATE TABLE shipmethod (\n"
        "ID int NOT NULL AUTO_INCREMENT,\n"
        "TxnID char(40) NOT NULL DEFAULT '', \n"
        "Name char(255) NOT NULL DEFAULT '', \n"
        "TrackingID char(255) DEFAULT '', \n"
        "PRIMARY KEY (ID) \n"
        ")";

    m_stmt.reset(m_con->prepareStatement(REQUEST));
    if (m_stmt->execute()) {
        std::string error{ "Failed to create table SHIPMETHOD" + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
        ThrownError e(ErrorLevel::ERR, error);
        throw e;
    }


    // Order Links (base to other order. this taxonomy allows for order relations on the far end.)
    REQUEST =
        "CREATE TABLE orderlinks (\n"
        "ID int NOT NULL AUTO_INCREMENT,\n"
        "BaseID char(40) NOT NULL DEFAULT '', \n"
        "LinkID char(40) NOT NULL DEFAULT '', \n"
        "Type char(255) NOT NULL DEFAULT '', \n"
        "PRIMARY KEY (ID) \n"
        ")";

    m_stmt.reset(m_con->prepareStatement(REQUEST));
    if (m_stmt->execute()) {
        std::string error{ "Failed to create table ORDERLINKS" + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
        ThrownError e(ErrorLevel::ERR, error);
        throw e;
    }

    // Addresses
    REQUEST =
        "CREATE TABLE address (\n"
        "ID int NOT NULL AUTO_INCREMENT, \n"
        "CustomerListID char(40) NOT NULL, \n"
        "Address1 char(255) DEFAULT '', \n"
        "Address2 char(255) DEFAULT '', \n"
        "Address3 char(255) DEFAULT '', \n"
        "Address4 char(255) DEFAULT '', \n"
        "Address5 char(255) DEFAULT '', \n"
        "City char(100) DEFAULT '', \n"
        "State char(100) DEFAULT '', \n"
        "Country char(40) DEFAULT '', \n"
        "PostalCode char(20) DEFAULT '', \n"
        "PRIMARY KEY (ID)\n"
        ")";

    m_stmt.reset(m_con->prepareStatement(REQUEST));
    if (m_stmt->execute()) {
        std::string error{ "Failed to create table ADDRESS" + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
        ThrownError e(ErrorLevel::ERR, error);
        throw e;
    }

    // SyncConfig
    REQUEST =
        "CREATE TABLE sync_config (\n"
        "ID int NOT NULL AUTO_INCREMENT, \n"
        "Time datetime DEFAULT '1970-01-01T12:00:00',\n"
        "ConfigKey char(200) NOT NULL, \n"
        "PRIMARY KEY (ID)\n"
        ")";

    m_stmt.reset(m_con->prepareStatement(REQUEST));
    if (m_stmt->execute()) {
        std::string error{ "Failed to create table SYNC_CONFIG" + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
        ThrownError e(ErrorLevel::ERR, error);
        throw e;
    }

    table = "sync_config";
    
    std::vector<std::string> configTables = { "inventory", "customers", "purchaseorders", "salesorders", "invoices", "estimates" };
    
    inserts.clear();
    inserts.resize(0);
    inserts.push_back({ "ConfigKey", std::string("") });

    for (std::string cfg : configTables) {
        inserts.at(0).second = std::string(cfg);
        if (!SQLInsert(table, inserts, false)) {
            std::string error{ "Failed to insert config for " + cfg + " line: " + std::to_string(__LINE__) + " Fn: " + __FUNCTION__};
            ThrownError e(ErrorLevel::ERR, error);
            throw e;
        }
    }

    // SyncQueue
    REQUEST =
        "CREATE TABLE sync_queue (\n"
        "ID int NOT NULL AUTO_INCREMENT, \n"
        "Action char(40) NOT NULL DEFAULT '', \n"
        "Ident char(40) NOT NULL DEFAULT '', \n"
        "Priority int DEFAULT 0, \n"
        "Status char(1) NOT NULL DEFAULT 'q', \n"
        "Message text, \n"
        "PRIMARY KEY (ID)\n"
        ")";

    m_stmt.reset(m_con->prepareStatement(REQUEST));
    if (m_stmt->execute()) {
        std::string error{ "Failed to create table SYNC_QUEUE" + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
        ThrownError e(ErrorLevel::ERR, error);
        throw e;
    }

    //std::cout << "Database Creation Complete. \n";

    REQUEST = 
        "CREATE TABLE error_log ("
        "ID int NOT NULL AUTO_INCREMENT, \n"
        "Level int(1) DEFAULT 0, \n"
        "Note text, \n"
        "PRIMARY KEY (ID)\n"
        ")";

    m_stmt.reset(m_con->prepareStatement(REQUEST));
    if (m_stmt->execute()) {
        std::string error{ "Failed to create table ERROR_LOG" + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
        ThrownError e(ErrorLevel::ERR, error);
        throw e;
    }
}