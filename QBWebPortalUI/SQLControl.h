#pragma once

/**
* 
* SQL Control for all SQL Calls. Will likely have generic machinations as well as more complicated SQL Call constructors. 
* PHP Front end for other half, not part of this however. 
* 
* We will create a brand new database, no longer using shellaccess.
* We should test for DB existance and CREATE if not exists.
* 
*/

// TO DO: 
// ------
// <DONE> ---- Create successful connection to local DB
// <DONE> ---- create connection using PDO connectivity
// add error processing
// <DONE> ---- add SSL support for all calls
// create database if not exists
// <DONE> ---- create base functionality of class.

// Standard C++ includes
#include <stdlib.h>
#include <vector>   // vector
#include <string>   // string
#include <iostream> // std::cout
#include <memory>   // auto_ptr
#include <sstream>  // stringstream
#include <iomanip>  // setfill, setw

#include <functional> // std::hash for rand seed
#include <fstream>  // filestream services, reading and writing files.
#include <thread>
#include <chrono>
/*
  Include directly the different
  headers from cppconn/ and mysql_driver.h + mysql_util.h
  (and mysql_connection.h). This will reduce your build time!
*/


#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/metadata.h>
#include <cppconn/resultset_metadata.h>
#include <cppconn/warning.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

#include "Enums.h" // dataType, datapass objects for creating and storing data. ErrorLevel for error reporting

// sample users for SQL
// root uDx@7euG?/d>
// QBXML cVyio%90pxE4
// database: qbxmlstorage
// ip string: tcp://127.0.0.1:3306

using FilterPass = std::vector<std::pair<std::string, DataPass>>;
using StrPair = std::vector<std::pair<std::string, std::string>>;

class SQLControl
{
private:
    // connection strings
    sql::SQLString m_IP{ "" };            // IP of server
    sql::SQLString m_user{ "" };          // username for server
    sql::SQLString m_pass{ "" };          // password for server
    sql::SQLString m_database{ "" };    // database.

    // unviersal variables for sql controls.
    sql::Driver* m_driver;
    std::unique_ptr<sql::PreparedStatement> m_stmt;
    std::unique_ptr<sql::ResultSet> m_res;
    std::unique_ptr<sql::Connection> m_con;

    // private class functions.
    bool testConnection();      // test if database exists and is set up, to bypass database creation at startup. 
    void initializeDatabase();  // build the database. Will create all tables required and initialize any data needed. 

    std::string whereString(FilterPass filter, std::string separator);   // create the "where" component of a string.
    bool Bind(FilterPass filter);

    void handleException(sql::SQLException& e, int line, std::string fn);

    bool loadConnection(std::string password);
    void connect();

public:
    SQLControl(std::string password);           // start the DB and any processes required.
    //SQLControl(const SQLControl&) = default;
    //SQLControl& operator=(const SQLControl&) = default;
    ~SQLControl();          // close any processes.
    
    // REAL functionality. Will be syntactically similar to the system used in hosecontrol
    bool SQLSelect(std::vector<std::string> columns, std::string table, FilterPass filter, int limit = -1, int offset = -1, bool desc = false);
    bool SQLSearch(std::vector<std::string> getColumns, std::vector<std::string> searchColumns, std::string table, std::string searchTerm, FilterPass filter, int limit = -1, int offset = -1, bool desc = false);

    bool SQLUpdate(std::string table, FilterPass updates, FilterPass filter);
    bool SQLInsert(std::string table, FilterPass inserts);              // for a single insert
    bool SQLMassInsert(std::string table, std::vector<FilterPass> inserts);     // for many lines insert
    bool SQLDelete(std::string table, FilterPass filter);
    bool SQLUpdateOrInsert(std::string table, FilterPass updates, FilterPass filter);
    bool SQLComplex(std::string& request, FilterPass inserts, bool expectData = false);

    // config management.
    std::string getConfigTime(std::string name);
    bool updateConfigTime(std::string name);

    // functions for retrieving data, as per the SELECT action.
    bool nextRow();
    std::string getString(std::string name);
    int getInt(std::string name);
    double getDouble(std::string name);

    bool generateConnectionFile(std::string username, std::string password, std::string ip, std::string database, std::string encryptionPassword);
    
    bool connected = false; // quick check if a valid connection exists.
    bool isConnected();

    void logError(ErrorLevel level, std::string& message);

};





/*
void SQLControl::testFunction() {

    // basic working functionality.

    try {
        sql::Driver* driver;
        sql::Connection* con;
        sql::Statement* stmt;
        sql::ResultSet* res;

        /* Create a connection * /
        driver = get_driver_instance();
        con = driver->connect("tcp://127.0.0.1:3306", "root", "uDx@7euG?/d>");
        /* Connect to the MySQL test database * /
        con->setSchema("qbxmlstorage");

        stmt = con->createStatement();
        res = stmt->executeQuery("SELECT Data FROM sample");
        while (res->next()) {
            std::cout << "\t... MySQL replies: ";
            /* Access column data by alias or column name * /
            std::cout << res->getString("Data") << std::endl;
            std::cout << "\t... MySQL says it again: ";
            /* Access column data by numeric offset, 1 is the first column * /
            std::cout << res->getString(1) << std::endl;
        }
        delete res;
        delete stmt;
        delete con;

    }
    catch (sql::SQLException& e) {
        std::cout << "# ERR: SQLException in " << __FILE__;
        std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
        std::cout << "# ERR: " << e.what();
        std::cout << " (MySQL error code: " << e.getErrorCode();
        std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
    }

    std::cout << std::endl;
}
*/