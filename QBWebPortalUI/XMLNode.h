#pragma once

/**
* 
* XML Node is for node functions and maintenance. 
* Will not be used on it's own, instead as a "helper" class to the main parser. 
* 
* AddNode will work autonomousyl 
* 
*/

#include <iostream>

#include <string>	// std::string
#include <sstream>	// std::stringstream
#include <vector>	// vectors
#include <utility>	// for pair
#include <stack>	// for adding children.
#include <list>		// replacing stack for adding children

// datatype for XML 
enum class DataType {
	INTEGER,
	DECIMAL,
	STRING,
	PARENT_NODE
};


class XMLNode
{
private: 
	// data classification
	bool m_isActive = false;			// Was activated properly, for data recall functions.
	int m_intVal{0};					// integer data
	double m_decVal{0};					// float or double data
	std::string m_stringVal{""};		// string data

	// parent node classification
	std::vector<XMLNode> m_children;	// children nodes, not used on datatypes

	//node classification
	DataType m_nodeType;				// node type, parent or datatype.
	std::string m_name;					// Node name, for search functions
	std::vector<XMLNode> m_attributes;	// Attributes found within the node object

	DataType inferDataType(const std::string& data);

public:
	// constructors based on type, using implied typing
	XMLNode() {}												// for reference purposes.
	XMLNode(const std::string &name, const std::string &data);	// data node
	XMLNode(const std::string &name);							// parent node
	~XMLNode();
	int addNode(const std::string& name, const std::string& data, std::list<int> location);	// add data node child, return position of child
	int addNode(const std::string& name, std::list<int> location);							// add parent node child, return position of child.
	int addNullNode(const std::string& name, std::list<int> location);						// add a data child node(string) with no data, return child position.

	bool addAttribute(const std::string& name, const std::string& data);	// add attribute to this node.
	bool updateAttribute(const std::string& name, const std::string& data);	// edit an existing attribute.

	bool addAttribute(const std::string& name, const std::string& data, std::list<int> location);	// add attribute to a child node
	bool updateAttribute(const std::string& name, const std::string& data, std::list<int> location);	// edit an existing attribute of a child node.

	void setString(const std::string& data); // resets the data on this item.

	int getChildCount();					// return total children, for procedural functions.
	XMLNode& child(unsigned int node);		// return child reference of this node.
	XMLNode findChild(std::string name);	// return child reference by name.
	
	std::string getChildValue(std::string name, std::string s = "");
	int getChildValue(std::string name, int i = 0);
	double getChildValue(std::string name, double d = 0.0);
	double getDoubleOrInt(std::string name);
	int getIntOrDouble(std::string name, int def = 0);

	bool getNodeFromPath(std::list<int> path, XMLNode& retNode);				// use direct int to get path location of node
	bool getNodeFromNamePath(std::list<std::string> path, XMLNode& retNode);	// use name search to get path location of node

	std::string getName();
	XMLNode& getAttribute(std::string& name);
	std::string getStringData();
	int getIntData();
	double getDecimalData();
	DataType getType();
	bool getStatus();

	// Data Output
	void write(int indent = 0); // write the node data out to screen.
	void writeXML(std::string& retval, int indent = 0); // write the node data out to screen.
};

