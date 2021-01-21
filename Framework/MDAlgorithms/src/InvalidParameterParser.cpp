// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/InvalidParameterParser.h"
#include "MantidAPI/ImplicitFunctionParameterParserFactory.h"
#include <boost/algorithm/string.hpp>
#include <utility>

namespace Mantid {
namespace MDAlgorithms {
DECLARE_IMPLICIT_FUNCTION_PARAMETER_PARSER(InvalidParameterParser)

InvalidParameterParser::InvalidParameterParser() {}

Mantid::API::ImplicitFunctionParameter *InvalidParameterParser::createParameter(Poco::XML::Element *parameterElement) {
  std::string sParameterValue = parameterElement->getChildElement("Value")->innerText();
  return parseInvalidParameter(sParameterValue);
}

InvalidParameter *InvalidParameterParser::parseInvalidParameter(std::string value) {
  return new InvalidParameter(std::move(value));
}

void InvalidParameterParser::setSuccessorParser(ImplicitFunctionParameterParser *parser) {
  UNUSED_ARG(parser);
  // Do nothing. No successor allowed.
}
} // namespace MDAlgorithms
} // namespace Mantid
