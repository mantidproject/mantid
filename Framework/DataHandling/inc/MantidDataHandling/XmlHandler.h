/*
 * XmlHandler.h
 *
 *  Created on: Nov 5, 2015
 *      Author: rhf
 */

#ifndef SUBPROJECTS__MANTIDFRAMEWORK_DATAHANDLING_SRC_XMLHANDLER_H_
#define SUBPROJECTS__MANTIDFRAMEWORK_DATAHANDLING_SRC_XMLHANDLER_H_

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/Text.h>
#include <Poco/AutoPtr.h>
#include <Poco/SAX/InputSource.h>
#include <Poco/DOM/NamedNodeMap.h>

namespace Mantid {
namespace DataHandling {

class XmlHandler {
public:
  XmlHandler();
  XmlHandler(std::string /*filename*/);
  virtual ~XmlHandler();

  std::map<std::string, std::string>
  get_metadata(const std::string &tag_to_ignore = "Detector");
  std::string get_text_from_tag(const std::string & /*xpath*/);
  std::map<std::string, std::string>
  get_attributes_from_tag(const std::string & /*xpath*/);

private:
  Poco::AutoPtr<Poco::XML::Document> pDoc;
};
}
/* namespace DataHandling */
} /* namespace Mantid */

#endif /* SUBPROJECTS__MANTIDFRAMEWORK_DATAHANDLING_SRC_XMLHANDLER_H_ */
