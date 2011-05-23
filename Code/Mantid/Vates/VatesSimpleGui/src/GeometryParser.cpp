/*
 * geometryparser.cpp
 *
 *  Created on: Apr 14, 2011
 *      Author: 2zr
 */

#include "GeometryParser.h"
#include "AxisInformation.h"

#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/Node.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NamedNodeMap.h"

#include <iostream>
#include <sstream>

GeometryParser::GeometryParser(const char *xml)
{
	Poco::XML::DOMParser parser;
	this->pDoc = parser.parseString(Poco::XML::XMLString(xml));
}

AxisInformation *GeometryParser::getAxisInfo(const std::string dimension)
{
	AxisInformation *axis = new AxisInformation();

	Poco::XML::NodeList *pNodes = this->pDoc->getElementsByTagName(dimension);
	Poco::XML::Node *pNode = pNodes->item(0)->childNodes()->item(0);
	Poco::XML::XMLString label = pNode->innerText();

	pNodes = this->pDoc->getElementsByTagName("Dimension");
	for (unsigned long int i = 0; i < pNodes->length(); ++i)
	{
		pNode = pNodes->item(i);
		Poco::XML::NamedNodeMap *aMap = pNode->attributes();
		Poco::XML::XMLString id = aMap->getNamedItem("ID")->getNodeValue();
		if (id == label)
		{
			break;
		}
	}

	Poco::XML::NodeList *cNodes = pNode->childNodes();
	double min, max;
	std::string title;
	for (unsigned long int j = 0; j < cNodes->length(); ++j)
	{
		Poco::XML::Node *cNode = cNodes->item(j);
		Poco::XML::XMLString elem = cNode->nodeName();
		if (elem == Poco::XML::XMLString("Name"))
		{
			title = cNode->innerText();
		}
		if (elem == Poco::XML::XMLString("LowerBounds"))
		{
			min = this->convertBounds(cNode->innerText());
		}
		if (elem == Poco::XML::XMLString("UpperBounds"))
		{
			max = this->convertBounds(cNode->innerText());
		}
	}

	axis->setTitle(title);
	axis->setMinimum(min);
	axis->setMaximum(max);

	return axis;
}

double GeometryParser::convertBounds(Poco::XML::XMLString val)
{
  double temp;
  std::stringstream number(val);
  number >> temp;
  return temp;
}

