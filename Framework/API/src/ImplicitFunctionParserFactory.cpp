// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/ImplicitFunctionParserFactory.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>

namespace Mantid::API {

std::shared_ptr<ImplicitFunctionParser> ImplicitFunctionParserFactoryImpl::create(const std::string &xmlString) const {
  UNUSED_ARG(xmlString);
  throw std::runtime_error("Use of create in this context is forbidden. Use "
                           "createUnwrappedInstead.");
}

ImplicitFunctionParser *
ImplicitFunctionParserFactoryImpl::createImplicitFunctionParserFromXML(Poco::XML::Element *functionElement) const {
  const std::string &name = functionElement->localName();
  if (name != "Function") {
    throw std::runtime_error("Root node must be a Funtion element. Unable to determine parsers.");
  }

  Poco::XML::Element *typeElement = functionElement->getChildElement("Type");
  std::string functionParserName = typeElement->innerText() + "Parser";
  ImplicitFunctionParser *functionParser = this->createUnwrapped(functionParserName);

  Poco::XML::Element *parametersElement = functionElement->getChildElement("ParameterList");

  // Get the parameter parser for the current function parser and append that to
  // the function parser.
  ImplicitFunctionParameterParser *paramParser =
      Mantid::API::ImplicitFunctionParameterParserFactory::Instance().createImplicitFunctionParameterParserFromXML(
          parametersElement);
  functionParser->setParameterParser(paramParser);

  Poco::AutoPtr<Poco::XML::NodeList> childFunctions = functionElement->getElementsByTagName("Function");
  ImplicitFunctionParser *childParser = nullptr;
  for (unsigned long i = 0; i < childFunctions->length(); i++) {
    // Recursive call to handle nested parameters.
    ImplicitFunctionParser *tempParser =
        createImplicitFunctionParserFromXML(dynamic_cast<Poco::XML::Element *>(childFunctions->item(i)));
    if (i == 0) {
      childParser = tempParser;
      // Add the first child function parser to the parent (composite) directly.
      functionParser->setSuccessorParser(childParser);
    } else {
      // Add all other function parsers are added as successors to those before
      // them in the loop.
      childParser->setSuccessorParser(tempParser);
      childParser = tempParser;
    }
  }

  return functionParser;
}

ImplicitFunctionParser *
ImplicitFunctionParserFactoryImpl::createImplicitFunctionParserFromXML(const std::string &functionXML) const {
  using namespace Poco::XML;
  DOMParser pParser;
  AutoPtr<Document> pDoc = pParser.parseString(functionXML);
  Element *pRootElem = pDoc->documentElement();

  return createImplicitFunctionParserFromXML(pRootElem);
}
} // namespace Mantid::API
