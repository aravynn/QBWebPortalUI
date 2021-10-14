#include "XMLNode.h"

// Function designed to take a string, and parse to it's appropriate type, being int, double or text.
// --------------------------------------------------------------------------------------------------

DataType XMLNode::inferDataType(const std::string& data)
{
    // override. this MUST be maintained as a string.
    if (m_name == "EditSequence") {
        m_stringVal = data;
        return DataType::STRING;
    }

    // check for a null string, and return an appropriate value for that. 
    if (data == "") {
        m_stringVal = "*NULL*"; // special value will be tested and set as <name /> in XML at write time.
        return DataType::STRING;
    }

    // check if type is either an integer or a decimal, and return the type.
    char* ptr = nullptr;

    bool isDouble = false;

    for (char d : data) {
        // iterate by character and search for and NON 0-9.-
        if ((d > 47 && d < 58) || d == 45 || d == 46) {
           // this is numerical and can be skipped
            if (d == 46) {
                // we're dealing explicitly with a double. 
                isDouble = true;
            }
        }
        else {
            m_stringVal = data;
            return DataType::STRING;
        }
    }

    int i = std::strtol(data.data(), &ptr, 10);
    if (ptr == data.data() + data.size() && !isDouble) {
        m_intVal = i;
        return DataType::INTEGER;
    }

    double d = std::strtod(data.data(), &ptr);
    if (ptr == data.data() + data.size()) {
        m_decVal = d;
        return DataType::DECIMAL;
    }
    
    m_stringVal = data;
    return DataType::STRING;
}

// functions create a new node, assigning values as required. Uses inferred typing. 
// --------------------------------------------------------------------------------
XMLNode::XMLNode(const std::string& name, const std::string& data) : m_name{ name }, m_isActive{ true }
{
    m_nodeType = inferDataType(data);
}

XMLNode::XMLNode(const std::string& name) : m_name{ name }, m_nodeType{ DataType::PARENT_NODE }, m_isActive{ true }
{
    // typing not required, no data to process.
}

XMLNode::~XMLNode()
{
    std::vector<XMLNode>().swap(m_attributes);
    std::vector<XMLNode>().swap(m_children);
}

// functions add a child node to this element. 
// -------------------------------------------
int XMLNode::addNode(const std::string& name, const std::string& data, std::list<int> location)
{
    // check data validity.
    if (data.size() < 1) {
        return -1; // no data to use. by default a failure, though possibly intentional.
    }

    // check location stack, if it is size 0, we'll place as a child here, otherwise, move down a stack.
    if (location.size() == 0) {
        m_children.push_back(XMLNode(name, data));  // add the new node. 
        return m_children.size() - 1; // return last element added. 
    }
    else {
        int x = location.front(); // get the top value
        location.pop_front(); // remove the top element. 
        return m_children.at(x).addNode(name, data, location); // send to next.
    }

    return -2; // shouldn't ever happen, but if it does something is up with the adder script. 
}

int XMLNode::addNode(const std::string& name, std::list<int> location)
{
    // check location stack, if it is size 0, we'll place as a child here, otherwise, move down a stack.
    if (location.size() == 0) {
        m_children.push_back(XMLNode(name));  // add the new node. 
        return m_children.size() - 1; // return last element added. 
    }
    else {
        int x = location.front(); // get the top value
        location.pop_front(); // remove the top element. 
        return m_children.at(x).addNode(name, location); // send to next.
    }

    return -2; // shouldn't ever happen, but if it does something is up with the adder script. 
}

int XMLNode::addNullNode(const std::string& name, std::list<int> location)
{

    // check location stack, if it is size 0, we'll place as a child here, otherwise, move down a stack.
    if (location.size() == 0) {
        m_children.push_back(XMLNode(name, std::string("")));  // add the new node. 
        return m_children.size() - 1; // return last element added. 
    }
    else {
        int x = location.front(); // get the top value
        location.pop_front(); // remove the top element. 
        return m_children.at(x).addNullNode(name, location); // send to next.
    }

    return -2; // shouldn't ever happen, but if it does something is up with the adder script. 
}

// function adds an attribute as pulled from XML <> Uses inferred typing
// ---------------------------------------------------------------------

bool XMLNode::addAttribute(const std::string& name, const std::string& data)
{
    if (data.size() < 1) {
        return false; // no data. 
    }
    m_attributes.push_back(XMLNode(name, data));
    return true;
}

// find and edit an existing attribute, for creating the XML. returns false if not found for add or update function.
// -----------------------------------------------------------------------------------------------------------------

bool XMLNode::updateAttribute(const std::string& name, const std::string& data)
{
    for (unsigned int i{ 0 }; i < m_attributes.size(); ++i) {
        if (m_attributes.at(i).getName() == name) {
            
            m_attributes.at(i).setString(data);
            return true; // if the data was set. 
        }
    }

    return false; // if not found. 
}

bool XMLNode::addAttribute(const std::string& name, const std::string& data, std::list<int> location)
{

    // check data validity.
    if (data.size() < 1) {
        return false; // no data to use. by default a failure, though possibly intentional.
    }

    // check location stack, if it is size 0, we'll place as a child here, otherwise, move down a stack.
    if (location.size() == 0) {
        return addAttribute(name, data);
    }
    else {
        int x = location.front(); // get the top value
        location.pop_front(); // remove the top element. 
        return m_children.at(x).addAttribute(name, data, location); // send to next.
    }

    return false;
}

bool XMLNode::updateAttribute(const std::string& name, const std::string& data, std::list<int> location)
{
    // check data validity.
    if (data.size() < 1) {
        return false; // no data to use. by default a failure, though possibly intentional.
    }

    // check location stack, if it is size 0, we'll place as a child here, otherwise, move down a stack.
    if (location.size() == 0) {
        return updateAttribute(name, data);
    }
    else {
        int x = location.front(); // get the top value
        location.pop_front(); // remove the top element. 
        return m_children.at(x).updateAttribute(name, data, location); // send to next.
    }

    return false;
}

void XMLNode::setString(const std::string& data)
{
    m_stringVal = data;
}

int XMLNode::getChildCount()
{
    // 0 is a valid return value, for bottom level.
    return (int)m_children.size();
}

// function returns reference to child node of this element. 
// ---------------------------------------------------------
XMLNode& XMLNode::child(unsigned int node)
{
    return m_children.at(node);
}

XMLNode XMLNode::findChild(std::string name)
{
    for (unsigned int i{ 0 }; i < m_children.size(); ++i) {
        if (m_children.at(i).getName() == name) {
            return m_children.at(i);
        }
    }
    XMLNode blank;
    return blank;
}

std::string XMLNode::getChildValue(std::string name, std::string s)
{
    for (unsigned int i{ 0 }; i < m_children.size(); ++i) {
        if (m_children.at(i).getName() == name) {
            return m_children.at(i).getStringData();
        }
    }
    return s;
}

int XMLNode::getChildValue(std::string name, int i)
{
    for (unsigned int i{ 0 }; i < m_children.size(); ++i) {
        if (m_children.at(i).getName() == name) {
            return m_children.at(i).getIntData();
        }
    }
    return i;
}

double XMLNode::getChildValue(std::string name, double d)
{
    for (unsigned int i{ 0 }; i < m_children.size(); ++i) {
        if (m_children.at(i).getName() == name) {
            return m_children.at(i).getDecimalData();
        }
    }
    return d;
}

double XMLNode::getDoubleOrInt(std::string name)
{

    for (unsigned int i{ 0 }; i < m_children.size(); ++i) {
        if (m_children.at(i).getName() == name) {
            if (getStatus())
                return (m_children.at(i).getType() == DataType::DECIMAL ? m_children.at(i).getDecimalData() : (m_children.at(i).getType() == DataType::INTEGER ? (double)m_children.at(i).getIntData() : 0.0));
        }
    }

    return 0.0;
}

int XMLNode::getIntOrDouble(std::string name, int def)
{
    for (unsigned int i{ 0 }; i < m_children.size(); ++i) {
        if (m_children.at(i).getName() == name) {
            if (getStatus())
                return (m_children.at(i).getType() == DataType::INTEGER ? m_children.at(i).getIntData() : (m_children.at(i).getType() == DataType::DECIMAL ? (int)m_children.at(i).getDecimalData() : 0));
        }
    }

    return def;
}

// function searched through pathing to discover the location of the requested data.
// ---------------------------------------------------------------------------------
bool XMLNode::getNodeFromPath(std::list<int> path, XMLNode &retNode)
{
    if (m_children.size() <= path.front()) {
        // this is too large of a search function, BOOL false, and return null values.
        return false;
    }

//    std::cout << m_name << '\n';

    int x = path.front();
    path.pop_front();
    if(path.size() == 0)
    {
        retNode = m_children.at(x);
        return true;
    }
    else {
        
        return m_children.at(x).getNodeFromPath(path, retNode);
    }
}

bool XMLNode::getNodeFromNamePath(std::list<std::string> path, XMLNode& retNode)
{
    for (unsigned int i{ 0 }; i < m_children.size(); ++i) {
        if (m_children.at(i).getName() == path.front()) {
            path.pop_front();
            
            if (path.size() == 0)
            {
                retNode = m_children.at(i);
                return true;
            }
            else {
                return m_children.at(i).getNodeFromNamePath(path, retNode);
            }        
        }
    }
    return false; // failed to find.
}



// Get functions to pull data from this node. 
// ------------------------------------------
std::string XMLNode::getName() {
    return m_name;
}

XMLNode& XMLNode::getAttribute(std::string& name) 
{
    for (unsigned int i{ 0 }; i < m_attributes.size(); ++i) {
        if (m_attributes.at(i).getName() == name) {
            return m_attributes.at(i);
        }
    }
    XMLNode blank;
    return blank;
}

std::string XMLNode::getStringData() 
{
    return m_stringVal;
}

int XMLNode::getIntData() 
{
    return m_intVal;
}

double XMLNode::getDecimalData() 
{
    return m_decVal;
}

DataType XMLNode::getType() 
{
    return m_nodeType;
}

bool XMLNode::getStatus()
{
    return m_isActive;
}

void XMLNode::write(int indent)
{

    for (int i{ 0 }; i < indent; ++i) {
        std::cout << '\t';
    }

    std::cout << m_name << ' ';
    switch (m_nodeType)
    {
    case DataType::INTEGER:
        std::cout << "Integer: " << m_intVal << '\n';
        break;
    case DataType::DECIMAL:
        std::cout << "Decimal: " << m_decVal << '\n';
        break;
    case DataType::STRING:
        std::cout << "String: " << m_stringVal << '\n';
        break;
    case DataType::PARENT_NODE:
        std::cout << '\n'; // nothing to add. 
        break;
    default:
        break;
    }
    if (m_attributes.size() > 0) {
        for (int i{ 0 }; i < m_attributes.size(); ++i) {
            m_attributes.at(i).write(indent);
        }
    }
    if (m_children.size() > 0) {
        for (int i{ 0 }; i < m_children.size(); ++i) {
            m_children.at(i).write(indent + 1);
        }
    }
}

void XMLNode::writeXML(std::string& retval, int indent)
{
    std::stringstream ss; // for final add to retval.

    

    ss << '\n';
    for (int i{ 0 }; i < indent; ++i) {
        ss << '\t';
    }

    if (m_stringVal == "*NULL*") {
        // this is a NULL element, not a child or other, and should be created as a self-closing tag.
        ss << '<' << m_name << " />";
        retval.append(ss.str());
        return; // close early, there cannot be children elements here.
    }

    ss << '<' <<  m_name;

    for (XMLNode x : m_attributes) {
        // check for attribute and write if needed.
        ss << ' ' << x.getName() << "=\"";

        switch (x.getType()) {
        case DataType::DECIMAL:
            ss << std::to_string(x.getDecimalData()) << "\"";
            break;
        case DataType::INTEGER:
            ss << std::to_string(x.getIntData()) << "\"";
            break;
        case DataType::STRING:
            ss << x.getStringData() << "\"";
            break;
        }
    }

    ss << '>';
    
    // append and reset retval.
    retval.append(ss.str());
    ss.str(std::string());

    // find the type, and apply the data. 
    switch (m_nodeType)
    {
    case DataType::INTEGER:
        ss << std::to_string(m_intVal);
        break;
    case DataType::DECIMAL:
        ss << std::to_string(m_decVal);
        break;
    case DataType::STRING:
        ss << m_stringVal;
        break;
    case DataType::PARENT_NODE:
        // in the case of a parent, perform recursion.
        //retval.append("\n"); // nothing to add.
        for (int i{ 0 }; i < m_children.size(); ++i) {
            m_children.at(i).writeXML(retval, indent + 1);
        }
        
        ss << '\n';
        for (int i{ 0 }; i < indent; ++i) {
            ss << '\t';
        }
        break;
    default:
        break;
    }

    // Add the closing tag. 
    ss << "</" << m_name << ">";
    retval.append(ss.str());
    
}