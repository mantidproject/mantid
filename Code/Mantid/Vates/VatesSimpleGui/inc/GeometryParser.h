/*
 * geometryparser.h
 *
 *  Created on: Apr 14, 2011
 *      Author: 2zr
 */

#ifndef GEOMETRYPARSER_H_
#define GEOMETRYPARSER_H_

#include "Poco/DOM/AutoPtr.h"
#include "Poco/DOM/Document.h"

class AxisInformation;

class GeometryParser {
public:
	GeometryParser(const char *xml);
	virtual ~GeometryParser() {};

	AxisInformation *getAxisInfo(const std::string dimension);

private:
	double convertBounds(Poco::XML::XMLString val);

	Poco::AutoPtr<Poco::XML::Document> pDoc;
};

#endif /* GEOMETRYPARSER_H_ */
