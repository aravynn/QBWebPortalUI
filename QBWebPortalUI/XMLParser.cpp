#include "XMLParser.h"

// Parser initialization. Requires a string of data to begin the process. It will 
// take the string, and go character at a time to find every instance of data and 
// return it. will return an error in the case of malformed XML
// ------------------------------------------------------------------------------
XMLParser::XMLParser(std::string& data)
{
	// note: we MUST initialize the XML node structure.
	bool isHeader{ false };

	std::vector<std::pair<XMLStep, std::string>> rawData;
	std::stringstream ss;

	for (unsigned int i{ 0 }; i < data.size(); ++i) {
		// iterate by character, and use the value to determine structure.

		// NEW STRUCTURE IDEA: 
		// data will be stored in a vector of data. We can then take said data (listed either as HEADER or CONTENT) and parse it for relevance. 

		// TODO:
		// create error checking algorithm, to first test then later write to file.

		if (data.at(i) == '<') {
			std::string d = ss.str();

			if (d.empty() || std::all_of(d.begin(), d.end(), [](char c) { return std::isspace(c); })) {

			}
			else {
				rawData.push_back({ XMLStep::CONTENT, ss.str() });
			}
			ss.str(std::string());
		}
		else if (data.at(i) == '>') {
			if (ss.str().at(0) != '!' && ss.str().at(0) != '?') {
				if (ss.str().at(0) == '/')
				{
					rawData.push_back({ XMLStep::FOOTER, ss.str().substr(1, ss.str().size() - 1) });
				}
				else {
					// check if tag is self-closing. If it is, ignore and move on.
					if (ss.str().at(ss.str().size() - 1) != '/') {
						rawData.push_back({ XMLStep::HEADER, ss.str() });
					}
				}

			}
			ss.str(std::string());
		}
		else {
			// content to stringstream.
			ss << data.at(i);
		}

	}

	// stack declarations for XML
	std::stack<std::string> nodeTitleStack;
	std::list<int> nodePath;

	// iterate through, and create the tree. 
	for (int i{ 0 }; i < rawData.size(); ++i) {
		// go by header declaration, of how to handle the given value. 

		switch (rawData.at(i).first) {
		case XMLStep::HEADER:
			/*for (std::pair<std::string, std::string> d : parseAttributes(rawData.at(i).second)) {
				std::cout << '\t' << d.first << ' ' << d.second << '\n';
			}*/
		{
			auto headerData = parseAttributes(rawData.at(i).second);
			std::string title = headerData.at(0).second; // always title
			headerData.erase(headerData.begin()); // remove the front element. 

			if (rawData.at(i + 1).first == XMLStep::CONTENT)
			{	// this is content, we don't need to add a node.
				m_nodes.addNode(title, rawData.at(i + 1).second, nodePath);

				for(auto add : headerData ){
					m_nodes.addAttribute(add.first, add.second, nodePath);
				}

				++i; // add to the iterator, to skip the content.
			}
			else
			{	// this is a header, add a node. 
				nodePath.push_back(m_nodes.addNode(title, nodePath)); // add node, and list in the node stack. 
				nodeTitleStack.push(title); // add a header to the stack.

				for (auto add : headerData) {
					m_nodes.addAttribute(add.first, add.second, nodePath);
				}				
			}
		}
		break;
		case XMLStep::FOOTER:
			if (nodeTitleStack.top() == rawData.at(i).second) {
				nodeTitleStack.pop();
				nodePath.pop_back();
			}
			else if (rawData.at(i - 1).first != XMLStep::CONTENT) {
				//std::cerr << "ERROR: Stack Mismatch for " << nodeTitleStack.top() << " and " << rawData.at(i).second << '\n';
				m_error = true;
				
				// generate error, throw for catch.
				std::string error = "XML Parser Footer: Stack Mismatch for " + nodeTitleStack.top() + " and " + rawData.at(i).second + " Raw Data: " + data;
				ThrownError e(ErrorLevel::ERR, error);
				throw e;
			}
			break;
		case XMLStep::CONTENT:
			// we... don't do anything here actually.
			{
				//std::cerr << "ERROR: Raw Content leaked for " << rawData.at(i).second << '\n';
				m_error = true;
				// generate error, throw for catch.
				std::string error = "XML Parser Content: Raw Content leaked for " + rawData.at(i).second + " Raw Data: " + data;
				ThrownError e(ErrorLevel::WARNING, error);
				throw e;
			}
			break;
		}
	}
}

XMLParser::~XMLParser()
{
}

XMLNode XMLParser::getNode()
{
	return m_nodes;
}

// return a vector of string data, with title in row 0
// ---------------------------------------------------
std::vector<std::pair<std::string, std::string>> XMLParser::parseAttributes(std::string& data) {
	
	std::vector<std::pair<std::string, std::string>> ret;
	std::stringstream ss;
	XMLStep step{ XMLStep::HEADER };
	for (unsigned int i{ 0 }; i < data.size(); ++i) {
		// iterate though, check for spaces, " and =
		switch (data.at(i)) {
		case ' ':
			if (step == XMLStep::HEADER) {
				// we've got our title
				ret.push_back({ "Title", ss.str() }); // save the title to dedicated title space. 
				ss.str(std::string()); // clear stringstream for next variable. 
				step = XMLStep::ATTRIBUTE;
			}
			else if (step == XMLStep::ATTRIBUTECONTENT) {
				ss << data.at(i);
			} // no else.
			break;
		case '"':
			if (step == XMLStep::ATTRIBUTE) {
				// we'll start the content. 
				step = XMLStep::ATTRIBUTECONTENT;
			}
			else if (step == XMLStep::ATTRIBUTECONTENT) {
				// completed value to be saved.
				ret.at(ret.size() - 1).second = ss.str();
				ss.str(std::string());
				step = XMLStep::ATTRIBUTE;
			}
			break;
		case '=':
			if (step == XMLStep::ATTRIBUTE) {
				// save the title of the attribute.
				ret.push_back({ ss.str(), "" });
				ss.str(std::string());
			}
			break;
		case '\t':
			// I think this is the breaking factor for parsing new XML
			ss << ' '; // add a space instead of a tab.

			break;
		default:
			ss << data.at(i);
			break;
		}
	}

	// at the end, check if still header and if stringstream has value, and add the title now if it is needed.
	if (step == XMLStep::HEADER) {
		// we've got our title
		ret.push_back({ "Title", ss.str() });
	}

	return ret;
}

// Develop the XML required for any request. 
// -----------------------------------------

std::string XMLParser::createXMLRequest(XMLNode& inserts)
{
	// The XML Send Request will only apply to the starter Rq element, the header and footer values are "pre supplied"
	// and will not be implemented in the main node structure. 
	// additional future proofing and additional changes may be needed to improve the XML generator below.

	std::string retval{ "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
						"<?qbxml version=\"13.0\"?>\n"
						"<QBXML>\n"
						"\t<QBXMLMsgsRq onError=\"stopOnError\">" };

	// take the inserts node, and recursively run though children, getting the child value and returning a string. 
	std::string x; 
	inserts.writeXML(x, 2);
	retval.append(x);

	retval.append("\n\t</QBXMLMsgsRq>\n"
		"</QBXML>");

	return retval;
}

// Check if creation errors occured.
// ---------------------------------

bool XMLParser::errorStatus()
{
	return m_error;
}
