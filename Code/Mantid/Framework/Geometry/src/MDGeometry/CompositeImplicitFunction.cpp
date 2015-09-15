#include <sstream>

#include "MantidGeometry/MDGeometry/CompositeImplicitFunction.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>

namespace Mantid {
namespace Geometry {

CompositeImplicitFunction::CompositeImplicitFunction() {}

CompositeImplicitFunction::~CompositeImplicitFunction() {}

bool CompositeImplicitFunction::addFunction(
    Mantid::Geometry::MDImplicitFunction_sptr constituentFunction) {
  bool bSuccess = false;
  if (constituentFunction.get() != NULL) {
    this->m_Functions.push_back(constituentFunction);
    bSuccess = true;
  }
  return bSuccess;
}

std::string CompositeImplicitFunction::getName() const {
  return CompositeImplicitFunction::functionName();
}

/** Serialize to XML */
std::string CompositeImplicitFunction::toXMLString() const {
  using namespace Poco::XML;
  AutoPtr<Document> pDoc = new Document;
  AutoPtr<Element> functionElement = pDoc->createElement("Function");
  pDoc->appendChild(functionElement);
  AutoPtr<Element> typeElement = pDoc->createElement("Type");
  AutoPtr<Text> typeText = pDoc->createTextNode(this->getName());
  typeElement->appendChild(typeText);
  functionElement->appendChild(typeElement);
  AutoPtr<Element> parameterListElement = pDoc->createElement("ParameterList");
  functionElement->appendChild(parameterListElement);

  std::string functionXML;
  for (FunctionIterator it = m_Functions.begin(); it != m_Functions.end();
       ++it) {
    functionXML += (*it)->toXMLString();
  }
  AutoPtr<Text> functionFormatText = pDoc->createTextNode("%s");
  functionElement->appendChild(functionFormatText);

  std::stringstream xmlstream;
  DOMWriter writer;
  writer.writeNode(xmlstream, pDoc);

  std::string formattedXMLString =
      boost::str(boost::format(xmlstream.str().c_str()) % functionXML.c_str());
  return formattedXMLString;
}

/** Return the number of functions in this composite
 * */
int CompositeImplicitFunction::getNFunctions() const {
  return static_cast<int>(this->m_Functions.size());
}

bool CompositeImplicitFunction::isPointContained(const coord_t *coords) {
  bool evalResult = false;
  std::vector<boost::shared_ptr<
      Mantid::Geometry::MDImplicitFunction>>::const_iterator it;
  for (it = this->m_Functions.begin(); it != this->m_Functions.end(); ++it) {
    evalResult = (*it)->isPointContained(coords);
    if (!evalResult) {
      break;
    }
  }
  return evalResult;
}

bool CompositeImplicitFunction::isPointContained(
    const std::vector<coord_t> &coords) {
  bool evalResult = false;
  std::vector<boost::shared_ptr<
      Mantid::Geometry::MDImplicitFunction>>::const_iterator it;
  for (it = this->m_Functions.begin(); it != this->m_Functions.end(); ++it) {
    evalResult = (*it)->isPointContained(coords);
    if (!evalResult) {
      break;
    }
  }
  return evalResult;
}
}
}
