#include "SQLControl.h"

// Search the database and return a user. This can be any user but 
// is used only to determine that there is in fact something in the DB. 
// --------------------------------------------------------------------
bool SQLControl::testConnection()
{
    try {
        m_stmt.reset(m_con->prepareStatement("SELECT TABLE_ROWS as Len FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = ? AND TABLE_NAME = 'users'"));
        m_stmt->setString(1, m_database);
        m_res.reset(m_stmt->executeQuery());

        while (m_res->next()) {
            int x = m_res->getInt("Len");
            if (x > 0) {
                return true; // there is something initialized as expected.
            }
        }

        return false;
    }
    catch (sql::SQLException& e) {
        handleException(e, __LINE__, __FUNCTION__);
        return false; // failed.
    }

    return true;
}

// create a WHERE string for SQL commands
// --------------------------------------

std::string SQLControl::whereString(FilterPass filter, std::string separator)
{
    // take the filter variable, and filter it into a QString object for return..
    if (filter.at(0).first != "") {
        // there is a filter, we'll iterate through and add them. we'll need to do this a second time for binding.
        std::string REQUEST = ""; // NULL value, so it can be used for multiple functions.
        int count = (int)filter.size();

        for (std::pair<std::string, DataPass>& f : filter) { // DataPass second value is unused here.

            if (count != filter.size()) {
                REQUEST.append(separator);
            }

            std::string equal = "=";

            if (f.second.notEqual) {
                equal = "!=";
            }

            if (--count > 0) {
                REQUEST.append(" " + f.first + " " + equal + " ?"); // there are more
            }
            else {
                REQUEST.append(" " + f.first + " " + equal + " ?");
            }
        }
        return REQUEST;
    }
    // if no filters exist, bypass and return a NULL value.
    return "";
}

// bind variables to the statement, prior to execution.
// ----------------------------------------------------
bool SQLControl::Bind(FilterPass filter)
{
    // bind the filter variables to the database call.
    // if there is anything that is NOT in filter, we'll need to add it prior to calling this.

    // return value of true or false on completion and success.
    if (filter.at(0).first != "") {

        int i{ 1 }; // iterator for binding, set numerically.

        for (std::pair<std::string, DataPass>& f : filter) {

            // due to complications with the DB, this code will fix the time issues.
           
            if (f.first == "TimeCreated" || f.first == "TimeModified") {
                // structured as: 
                // 2021-01-12T16:05:29-06:00
                // 0000000000111111111122222
                // 0123456789012345678901234

                std::vector<int> daysOfMonth{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }; // I mean, screw leap days.

                std::string d = f.second.str; // simpler naming.
                int year, month, day, hour, minute, second;

                year = std::stoi(d.substr(0, 4));
                month = std::stoi(d.substr(5, 2));
                day = std::stoi(d.substr(8, 2));
                hour = std::stoi(d.substr(11, 2));
                minute = std::stoi(d.substr(14, 2));
                second = std::stoi(d.substr(17, 2));
                
                // apply offset 
                int offset = std::stoi(d.substr(20, 2));
                if (d.at(19) == '-') {
                    // negative offset, remove the hours
                    hour -= offset;
                    if (hour < 0) {
                        hour += 24;
                        day--;
                        if (day < 1) {
                            day = daysOfMonth.at(month - 1); 
                            month -= 1;
                            if (month == 0) {
                                month = 12;
                                year--;
                            }
                        }
                    }
                }
                else {
                    // positive offset, add the hours
                    hour += offset;
                    if (hour > 23) {
                        hour -= 24;
                        day++;
                        if (day > daysOfMonth.at(month)) {
                            day -= daysOfMonth.at(month);
                            month++;
                            if (month > 12) {
                                month -= 12;
                                year++;
                            }
                        }
                    }
                }

                // reconstruct and reset the time for insert.
                std::stringstream ss;
                ss << std::setfill('0') <<
                    std::setw(4) << year << '-' <<
                    std::setw(2) << month << '-' <<
                    std::setw(2) << day << ' ' <<
                    std::setw(2) << hour << ':' <<
                    std::setw(2) << minute << ':' <<
                    std::setw(2) << second;

                f.second.str = ss.str();
            }
            

            // iterate through and bind values for the filters.
            //std::string binder = ":" + f.first;
            if (f.second.o == DPO::NUMBER) {
                m_stmt->setInt(i, f.second.num);
            }
            else if (f.second.o == DPO::DOUBLE) {
                m_stmt->setDouble(i, f.second.dbl);
            }
            else if (f.second.o == DPO::STRING) {
                m_stmt->setString(i, f.second.str); // bind the STRING value.
            }

            ++i;
            //   qWarning() << "Bound Value for " << binder << ": " << query.boundValue(":" + f.first);
        }
        return true; // values are added and we are done.
    }
    else {
        return false; // techincal success, though false will declare no values were added.
    }
}

void SQLControl::handleException(sql::SQLException& e, int line, std::string fn)
{
    std::string error = "# ERR: SQLException in " + std::string(__FILE__) + "(" + fn + ") on line " + std::to_string(line)
        + "# ERR: " + e.what() + " (MySQL error code: " + std::to_string(e.getErrorCode()) + ", SQLState: " + e.getSQLState() + " )";

    logError(ErrorLevel::ERR, error);
}

SQLControl::SQLControl(std::string password)
{
    if (loadConnection(password)) {
        connect();
    }
}

SQLControl::~SQLControl()
{
    // no values need manual destruction.
}

bool SQLControl::SQLSelect(std::vector<std::string> columns, std::string table, FilterPass filter, int limit, int offset, bool desc) 
{
    try {

        // NEW CODE HERE. 
        std::string REQUEST = "SELECT ";

        // go though columns and add to the statement
        if (columns.at(0) == "") {
            // there is nothing, use *
            REQUEST.append("* ");
        }
        else {
            // cycle though, and add the strinng to the final select statement.
            int count = (int)columns.size();
            for (std::string col : columns) {

                if (--count > 0) {
                    REQUEST.append(col + ", ");
                }
                else {
                    REQUEST.append(col + " ");
                }
            }
        }

        // add the table name.
        REQUEST.append("FROM " + table);

        // check for and add any filters.

        if (filter.size() > 0 && filter.at(0).first != "") {
            // there is a valid filter value.
            REQUEST.append(" WHERE");
            REQUEST.append(whereString(filter, " AND"));
        }

        if (desc) {
            // this is a descending query, add orderby
            REQUEST.append(" ORDER BY ID DESC"); // always PK so we can get the newest first.
        }
        // Check if a limit is set, and add it if it.
        if (limit > 0)
            REQUEST.append(" LIMIT " + std::to_string(limit));
        if (offset > 0)
            REQUEST.append(" OFFSET " + std::to_string(offset));

        //std::cout << "Select SQL: " <<  REQUEST << '\n';

        /* SELECT line, and return it. */
        m_stmt.reset(m_con->prepareStatement(REQUEST));
        
        // bind values to DB call
        if (!Bind(filter) && (int)filter.size() > 0 && filter.at(0).first != "") {
            std::string error{ "Error @ Bind SQL: Filter failed to bind. LINE " + std::to_string(__LINE__)+ " Fn: " + __FUNCTION__};
            logError(ErrorLevel::WARNING, error);
        }

        m_res.reset(m_stmt->executeQuery());

        if (m_res->rowsCount() < 1) {
            // nothing returned, send blank
            return false;
        }

    }
    catch (sql::SQLException& e) {
        handleException(e, __LINE__, __FUNCTION__);
    }

    return true;
}

// search the database at columns for X. Returns true if something found. 
// ----------------------------------------------------------------------

bool SQLControl::SQLSearch(std::vector<std::string> getColumns, std::vector<std::string> searchColumns, std::string table, std::string searchTerm, FilterPass filter, int limit, int offset, bool desc)
{
    // perform a SEARCH action on the Database. use the vector to get the part and assemble the request.
    try {
        std::string REQUEST = "SELECT ";

        // go though columns and add to the statement
        if (getColumns.at(0) == "") {
            // there is nothing, use *
            REQUEST.append("* ");
        }
        else {
            // cycle though, and add the strinng to the final select statement.
            int count = (int)getColumns.size();
            for (std::string col : getColumns) {

                if (--count > 0) {
                    REQUEST.append(col + ", ");
                }
                else {
                    REQUEST.append(col + " ");
                }
            }
        }

        // add the table name.
        REQUEST.append("FROM " + table);

        int count = (int)searchColumns.size();
        REQUEST.append(" WHERE ");

        for (std::string col : searchColumns) {

            if (--count > 0) {
                REQUEST.append(col + " LIKE ? OR ");
            }
            else {
                REQUEST.append(col + " LIKE ?");
            }
        }

        

        // add additional filters, if warranted.
        if (filter.size() > 0 && filter.at(0).first != "") {
            // unfortunately, we need to get this more complicated.
            int filters = filter.size();
            for (int i{ 0 }; i < filters; ++i) {
                // for each item add the appropriate value
                if (filter.at(i).first != "") {
                    REQUEST.append(" AND " + filter.at(i).first + " = ?" );
                }
            }
        }

        // Check if a limit is set, and add it if it.
        if (limit > 0)
            REQUEST.append(" LIMIT " + std::to_string(limit));
        if (offset > 0)
            REQUEST.append(" OFFSET " + std::to_string(offset));

        //std::cout << "Search SQL: " << REQUEST << '\n';

        // prepare the statement to actually process.
        m_stmt.reset(m_con->prepareStatement(REQUEST));

        // bind values to DB call
        if (filter.size() < 1 || filter.at(0).first == "") {
            filter.clear();
        }
    
        // resize for all of the search values.
        FilterPass fil2;
        fil2.resize(searchColumns.size());
        for (int i{ 0 }; i < searchColumns.size(); ++i) {
            fil2.at(i) = { "searchterm", '%' + searchTerm + '%'};
        }
   
        // concatenate 2 vectors to create final output. 
        fil2.insert(fil2.end(), filter.begin(), filter.end());

        // bind values, return error if it couldn't
        if (!Bind(fil2) && (int)fil2.size() > 0) {
            std::string error{ "Error @ Bind SQL: search filter failed to bind. LINE " + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
            logError(ErrorLevel::WARNING, error);
        }
        /*
        for (std::pair<std::string, DataPass> d : fil2) {
            std::cout << d.first << '-' << d.second.str << '\n';
        }
        */
        m_res.reset(m_stmt->executeQuery());
        
        

        if (m_res->rowsCount() < 1) {
            // nothing returned, send blank
            return false;
        }

        return true;

    }
    catch (sql::SQLException& e) {
        handleException(e, __LINE__, __FUNCTION__);
        return false;
    }

}

// Update the database based on values
// -----------------------------------

bool SQLControl::SQLUpdate(std::string table, FilterPass updates, FilterPass filter)
{
    try {

        std::string REQUEST = "UPDATE " + table + " SET";

        if (updates.at(0).first == "") {
            //   qWarning() << "ERROR: No provided update information for SQL Update";
            return false;
        }
        else {
            // there is data for update.
            REQUEST.append(whereString(updates, ", "));
        }

        if (filter.at(0).first == "") {
            //   qWarning() << "ERROR: No provided filter information for SQL Update";
            return false; // we MUST provide filters for update functions.
        }
        else {
            // apply  the filters.
            REQUEST.append(" WHERE");
            REQUEST.append(whereString(filter, " AND"));
        }

        //std::cout << "Update SQL: " << REQUEST << '\n';

        m_stmt.reset(m_con->prepareStatement(REQUEST));

        updates.insert(updates.end(), filter.begin(), filter.end());

        if (!Bind(updates) && (int)updates.size() > 0) {
            std::string error{ "Error @ Bind SQL: Updates failed to bind. LINE " + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
            logError(ErrorLevel::WARNING, error);
            return false;
        }

        if (m_stmt->execute()) {
            std::string error{ "Error @ Update: failed to execute. LINE " + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
            logError(ErrorLevel::ERR, error);
            return false;
        }

    }
    catch (sql::SQLException& e) {
        handleException(e, __LINE__, __FUNCTION__);
        return false;
    }

    return true;
}

// Insert a new row
// ----------------

bool SQLControl::SQLInsert(std::string table, FilterPass inserts)
{
    try {
        std::string REQUEST = "INSERT INTO " + table + " (";
        std::string VALUES = ") VALUES (";
        // run though all inserts, add to table. During same time, create secondary values string.
        int inSize = (int)inserts.size();
        for (int i{ 0 }; i < inSize; ++i) {
            //std::cout << inserts.at(i).first << ' ' << inserts.at(i).second.str << ' ' << inserts.at(i).second.num << ' ' << inserts.at(i).second.dbl << '\n';

            if (i + 1 == inSize) {
                // at max value
                REQUEST.append(inserts.at(i).first);
                VALUES.append("?");
            }
            else {
                // normal value.
                REQUEST.append(inserts.at(i).first + ", ");
                VALUES.append("?, ");
            }
        }

        // create the final query string
        REQUEST.append(VALUES + ")");

        //std::cout << "Insert SQL: " << REQUEST << '\n';
        //std::cout << inserts.at(0).second.num << ' ' << inserts.at(1).second.str << '\n';

        m_stmt.reset(m_con->prepareStatement(REQUEST));

        // bind the values
        if (!Bind(inserts) && (int)inserts.size() > 0) {
            //    qWarning() << "ERROR @ Bind: Filter failed to bind.";
            return false;
        }

        if (m_stmt->execute()) {
            std::string error{ "Error @ insertaction SQL: insert failed to bind. LINE " + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
            logError(ErrorLevel::WARNING, error);
            return false;
        }

    }
    catch (sql::SQLException& e) {
        handleException(e, __LINE__, __FUNCTION__);
        return false;
    }

    return true;
}

// Mass insert function adds or updates for multiple rows.
// -------------------------------------------------------

bool SQLControl::SQLMassInsert(std::string table, std::vector<FilterPass> inserts)
{
    try {
        std::string REQUEST = "INSERT INTO " + table + " ("; // save for table structure
        std::string VALUES = "("; // save for value structure
        std::string ONDUPLICATE = " ON DUPLICATE KEY UPDATE "; // save for update structure
        FilterPass binds;
        // for each insert set, create the update format. 
        // if this is table 1, we'll build the call structure.
        if (inserts.size() < 1) {
            // fails, no data to insert.
            //std::cout << "WARNING: Aborting mass import, no dataa given \n";
            return false; 
        }


        int inSize = (int)inserts.at(0).size();
        for (int i{ 0 }; i < inSize; ++i) {
            //std::cout << inserts.at(i).first << ' ' << inserts.at(i).second.str << ' ' << inserts.at(i).second.num << ' ' << inserts.at(i).second.dbl << '\n';


            
            if (i + 1 == inSize) {
                // at max value
                REQUEST.append(inserts.at(0).at(i).first);
                VALUES.append("?");
                ONDUPLICATE.append(inserts.at(0).at(i).first + " = VALUES(" + inserts.at(0).at(i).first + ")");
            }
            else {
                // normal value.
                REQUEST.append(inserts.at(0).at(i).first + ", ");
                VALUES.append("?, ");
                ONDUPLICATE.append(inserts.at(0).at(i).first + " = VALUES(" + inserts.at(0).at(i).first + "), ");
            }

            
        }

        // reserve space total for all binding information. 
        binds.reserve(inserts.size() * inserts.at(0).size());

        REQUEST.append(") VALUES ");
        VALUES.append(")");
        
        for (int i{ 0 }; i < inserts.size(); ++i) {
            
            if (inserts.at(i).size() < 1) {
                // this is nothing, so therefore we need to ignore it.
                
                // in the case of the last element, we'll need to remove the ", " from the end if somehow excess elements exist.
                if (i + 1 == inserts.size())
                    REQUEST.resize(REQUEST.size() - 2);

                continue;
            }
            
            if (i + 1 == inserts.size()) {
                REQUEST.append(VALUES);
            }
            else {
                REQUEST.append(VALUES + ", ");
            }

            // add to binding string. 
            binds.insert(binds.end(), inserts.at(i).begin(), inserts.at(i).end());

        }
        // add the duplicate string.
        REQUEST.append(ONDUPLICATE);

        //std::cout << REQUEST << '\n';

        m_stmt.reset(m_con->prepareStatement(REQUEST));

        // bind the values
        if (!Bind(binds) && (int)binds.size() > 0) {
            //    qWarning() << "ERROR @ Bind: Filter failed to bind.";
            return false;
        }

        if (m_stmt->execute()) {
            std::string error{ "Error @ insertaction SQL:  failed to execute. LINE " + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
            logError(ErrorLevel::WARNING, error);
            return false;
        }

    }
    catch (sql::SQLException& e) {
        handleException(e, __LINE__, __FUNCTION__);
        return false;
    }

    return true;
    
}


// Delete a row, cannot mass delete whole file
// -------------------------------------------

bool SQLControl::SQLDelete(std::string table, FilterPass filter)
{
    try {
        std::string REQUEST = "DELETE FROM " + table + " WHERE";

        // check filter size
        if (filter.size() < 1) {
            //   qWarning() << "ERROR: No filters for remove statement.";
            return false;
        }

        // load filter onto the string.
        REQUEST.append(whereString(filter, " AND"));
        
        //std::cout << REQUEST << '\n';

        m_stmt.reset(m_con->prepareStatement(REQUEST));

        // bind the values
        if (!Bind(filter) && (int)filter.size() > 0) {
            std::string error{ "Error @ Bind SQL: delete failed to bind. LINE " + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
            logError(ErrorLevel::WARNING, error);
            return false;
        }

        if (m_stmt->execute()) {
            std::string error{ "Error @ Bind SQL:  failed to delete. LINE " + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
            logError(ErrorLevel::ERR, error);
            return false;
        }
    }
    catch (sql::SQLException& e) {
        handleException(e, __LINE__, __FUNCTION__);
        return false;
    }

    return true;

}

bool SQLControl::SQLUpdateOrInsert(std::string table, FilterPass updates, FilterPass filter)
{
    std::vector<std::string> columns;
    
    for (auto u : filter) {
        columns.push_back(u.first);
    }
    
    if (SQLSelect(columns, table, filter)) {
        // update this value, it exists in the DB
        return SQLUpdate(table, updates, filter);
    }
    else {
        // insert instead, it is a new value.
        return SQLInsert(table, updates);
    }

    return false; // will never fire.
}

// Complex removes all internal setup, to allow true functions through
// User is responsible for the entire setup externally. Results may vary.
// ----------------------------------------------------------------------

bool SQLControl::SQLComplex(std::string& request, FilterPass inserts, bool expectData)
{
    try {

        m_stmt.reset(m_con->prepareStatement(request));

        // bind the values
        if (!Bind(inserts) && (int)inserts.size() > 0 && inserts.at(0).first != "") {
            std::string error{ "Error @ Bind SQL: complex failed to bind. LINE " + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
            logError(ErrorLevel::WARNING, error);
            return false;
        }
        if (expectData) {
            m_res.reset(m_stmt->executeQuery());

            if (m_res->rowsCount() < 1) {
                // nothing returned, send blank
                return true;
            }
        }
        else {
            if (m_stmt->execute()) {
                std::string error{ "Error SQL: complex failed. LINE " + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
                logError(ErrorLevel::WARNING, error);
                return false;
            }
        }
    }
    catch (sql::SQLException& e) {
        handleException(e, __LINE__, __FUNCTION__);
        return false;
    }

    return true;
}

std::string SQLControl::getConfigTime(std::string name)
{
    std::vector<std::string> columns = { "Time" };
    FilterPass filter = { {"ConfigKey", name} };
    if (SQLSelect(columns, "sync_config", filter, 1)) {
        if (nextRow()) {
            std::string cfgTime =  m_res->getString("Time");
            cfgTime.at(10) = 'T';

            return cfgTime;
        }
    }

    return "NA"; // somehow nothing.
    
}

bool SQLControl::updateConfigTime(std::string name)
{
    try {
        FilterPass filter = { { "ConfigKey", name } };

        std::string REQUEST = "UPDATE sync_config SET Time = NOW() WHERE ConfigKey = ?";

        m_stmt.reset(m_con->prepareStatement(REQUEST));

        if (!Bind(filter)) {
            std::string error{ "Error @ Bind SQL: configtime failed to bind. LINE " + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
            logError(ErrorLevel::WARNING, error);
            return false;
        }

        if (m_stmt->execute()) {
            std::string error{ "Error @ Bind SQL: configtime failed to execute. LINE " + std::to_string(__LINE__) + " Fn: " + __FUNCTION__ };
            logError(ErrorLevel::WARNING, error);
            return false;
        }
    }
    catch (sql::SQLException& e) {
        handleException(e, __LINE__, __FUNCTION__);
        return false;
    }

    return true;
}

// get the next row, for search and select.
// ----------------------------------------

bool SQLControl::nextRow()
{
    if (m_res->rowsCount() < 1) return false;

    return m_res->next();
}

// get a string based on name string
// ---------------------------------

std::string SQLControl::getString(std::string name)
{
    return m_res->getString(name);
}

// get int by name string
// ----------------------

int SQLControl::getInt(std::string name)
{
    return m_res->getInt(name);
}

// get double by name string
// -------------------------

double SQLControl::getDouble(std::string name)
{
    return m_res->getDouble(name);
}

// Generate an encryption file for login. Will require manual insertion of connection data.
// ----------------------------------------------------------------------------------------

bool SQLControl::generateConnectionFile(std::string username, std::string password, std::string ip, std::string database, std::string encryptionPassword)
{
    //we'll store the data using a VERY rudimentary file ecryption, only to prevent basic attempts to read the file. 
    std::size_t hash = std::hash<std::string>{}(encryptionPassword);

    std::string passStr = username + '\n' + password + '\n' + ip + '\n' + database + '\n';

    srand(hash);

    for (int i{ 0 }; i < passStr.size(); ++i) {
        passStr.at(i) = passStr.at(i) ^ (rand() % 256 + 1);
    }
    
    std::ofstream myfile("connect.cfg");
    if (myfile.is_open()) {
        myfile << passStr;
        myfile.close();

        m_user = username;
        m_pass = password;
        m_IP = ip;
        m_database = database;

        connect();

        return true;
    }
    else {
        return false;
    }



}

void SQLControl::logError(ErrorLevel level, std::string& message)
{
    // log errors to SQL in the case of problems. 
    FilterPass inserts;
    inserts.resize(2);
    inserts.at(0) = { "Level", (long long)level };
    inserts.at(1) = { "Note", message};

    // standard insert handles the process. 
    SQLInsert("error_log", inserts);
}

bool SQLControl::loadConnection(std::string password)
{
    std::size_t hash = std::hash<std::string>{}(password);
    srand(hash);

    std::ifstream myfile;
    std::stringstream fdata;
    
    myfile.open("connect.cfg");
    if (!myfile) {
        return false; // we failed
    }

    while(myfile){
        std::string d;
        std::getline(myfile, d);
        fdata << d << '\n';
    }

    myfile.close(); // we can lose the file now.

    std::string confData = fdata.str();

    fdata.str(std::string());

    int step{ 0 };
    for (int i{ 0 }; i < confData.size(); ++i) {
        
        char s = confData.at(i) ^ (rand() % 256 + 1);
        if (s != '\n') {
            // add to the stream.
            fdata << s;
        }
        else {
            // end of stream, we can elimiate the rest.
            switch (step++) {
            case 0: // username
                m_user = fdata.str();
                break;
            case 1: // password
                m_pass = fdata.str();
                break;
            case 2: // ip
                m_IP = fdata.str();
                break;
            case 3: // database
                m_database = fdata.str();
                break;
            default:// extraneous information.
                break; 
            }
            fdata.str(std::string());
        }
    }

    if (step < 3 && m_IP.substr(0,5) != "tcp://") {
        return false; // authentication failed.
    } 

    return true; // authentication passed.
}

void SQLControl::connect()
{
    if (m_con != nullptr) {
        m_con.reset();
        m_res.reset();
        m_stmt.reset();
        //std::cout << "resetting connection... \n";
    }
        
    try {
        // initialize the main driver, this is not a unique_ptr but is self deleting
        if (m_driver == nullptr) {
            m_driver = get_driver_instance();
        }

        // start the SQL connection
        m_con = (std::unique_ptr<sql::Connection>)m_driver->connect(m_IP, m_user, m_pass);
        
        // set the SQL database.
        m_con->setSchema(m_database);

        if (!testConnection()) {
            // there is nothing in the DB, create it.
            initializeDatabase();   
        }
        //std::cout << "SQL Connected \n"; 
        connected = true;
    }
    catch (sql::SQLException& e) {
        handleException(e, __LINE__, __FUNCTION__);
    }
}

/*

// take the table, and return what would be the next ID on the table. To be used BEFORE inserts
   

    QSqlQuery query;
    query.prepare(REQUEST);

    if(!query.exec()){
     //   qWarning() << "ERROR @ getPK: " << query.lastError().text(); // ensure no errors occur during request.
     //   qWarning() << query.executedQuery();
    }

    return getQueryLength(query);

*/


/* // backup of working code. in case I fuck it up.

SQLControl::SQLControl()
{
    // PDO example goes here.
    try {
        sql::Driver* driver = get_driver_instance();
        std::unique_ptr<sql::Connection> con(driver->connect("tcp://127.0.0.1:3306", m_user, m_pass));
        con->setSchema( m_database );
        std::unique_ptr<sql::PreparedStatement> stmt;
        std::unique_ptr<sql::ResultSet> res;

        /* INSERT a line, as a test.  /
stmt.reset(con->prepareStatement("INSERT INTO sample (DATA) values (?)"));
stmt->setString(1, "SQL Added");
stmt->execute();

/* SELECT line, and return it. * /
stmt.reset(con->prepareStatement("SELECT Data FROM sample WHERE ID > ?"));
//stmt->setString(1, "2");
stmt->setInt(1, 2);
res.reset(stmt->executeQuery());

while (res->next()) {
    std::cout << "\t... MySQL replies: ";
    /* Access column data by alias or column name * /
    std::cout << res->getString("Data") << std::endl;
    std::cout << "\t... MySQL says it again: ";
    /* Access column data by numeric offset, 1 is the first column * /
    std::cout << res->getString(1) << std::endl;
}

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