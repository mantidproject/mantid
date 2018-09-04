#include "MantidAPI/ImplicitFunctionParameterParserFactory.h"

#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>

namespace Mantid {
namespace API {

boost::shared_ptr<ImplicitFunctionParameterParser>
ImplicitFunctionParameterParserFactoryImpl::create(
    const std::string &xmlString) const {
  UNUSED_ARG(xmlString);
  throw std::runtime_error("Use of create in this context is forbidden. Use "
                           "createUnwrappedInstead.");
}

ImplicitFunctionParameterParser *ImplicitFunctionParameterParserFactoryImpl::
    createImplicitFunctionParameterParserFromXML(
        Poco::XML::Element *parametersElement) const {
  if (parametersElement->localName() != "ParameterList") {
    throw std::runtime_error("Expected passed element to be ParameterList.");
  }
  Poco::AutoPtr<Poco::XML::NodeList> parameters =
      parametersElement->getElementsByTagName("Parameter");
  ImplicitFunctionParameterParser *paramParser = nullptr;
  ImplicitFunctionParameterParser *nextParser = nullptr;
  for (unsigned long i = 0; i < parameters->length(); i++) {
    Poco::XML::Element *parameter =
        dynamic_cast<Poco::XML::Element *>(parameters->item(i));
    std::string paramParserName =
        parameter->getChildElement("Type")->innerText() +
        "Parser"; // Append parser to the name. Fixed convention
    ImplicitFunctionParameterParser *childParamParser =
        this->createUnwrapped(paramParserName);
    if (paramParser != nullptr) {
      nextParser->setSuccessorParser(childParamParser);
    } else {
      paramParser = childParamParser;
    }
    nextParser = childParamParser;
  }
  return paramParser;
}
} // namespace API
} // namespace Mantid
