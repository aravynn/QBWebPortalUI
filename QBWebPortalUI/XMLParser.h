#pragma once

/**
*
* The XML parser class will take a string as an "at creation" input, then parse the data into a data tree.
* the tree will be a list of structs, each holding either data or a further sub-array of data, to which we will drill down. 
* we should include functionality to pass a string for quick drill-down functions, as well as "place holder" functionality for quicker later access. 
* 
* 
*/

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <stack>
#include <list> // for children lists.
#include <utility>
#include <algorithm>
#include <cctype>

#include "XMLNode.h"
#include "Enums.h"

// steps for determining the current portion of the content. 
// ---------------------------------------------------------
enum class XMLStep {
	HEADER,
	ATTRIBUTE,
	ATTRIBUTECONTENT,
	CONTENT,
	FOOTER,
	NOTNODE,
	METADATA
};

class XMLParser
{
private: 
	XMLNode m_nodes{ XMLNode("Root") }; // all nodes found. 
	std::vector<std::pair<std::string, std::string>> parseAttributes(std::string& data);	// return a vector of string data, with title in row 0
	bool m_error = false;
public:
	XMLParser(std::string& data);
	~XMLParser();

	XMLNode getNode(); // get a reference to m_nodes.

	// Create XML functions. 
	static std::string createXMLRequest(XMLNode& inserts);

	bool errorStatus();

};

