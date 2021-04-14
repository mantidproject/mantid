// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidKernel/System.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/File.h>
#include <Poco/Path.h>

#include "MantidAPI/ImplicitFunctionParameterParser.h"

#ifndef Q_MOC_RUN
#include <boost/lexical_cast.hpp>
#endif

namespace Mantid {
namespace API {
/**

 XML Parser for single value parameter types

 @author Owen Arnold, Tessella plc
 @date 01/10/2010
 */

template <class SingleValueParameterType>
class DLLExport SingleValueParameterParser : public Mantid::API::ImplicitFunctionParameterParser {
public:
  Mantid::API::ImplicitFunctionParameter *createParameter(Poco::XML::Element *parameterElement) override;

  SingleValueParameterType *createWithoutDelegation(Poco::XML::Element *parameterElement);

  void setSuccessorParser(Mantid::API::ImplicitFunctionParameterParser *paramParser) override;
};

//////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
/* Creates a parameter from an xml element, otherwise defers to a successor
parser.
@param parameterElement : xml Element
@return A fully constructed ImplicitFunctionParameter.
*/
template <class SingleValueParameterType>
Mantid::API::ImplicitFunctionParameter *
SingleValueParameterParser<SingleValueParameterType>::createParameter(Poco::XML::Element *parameterElement) {
  using ValType = typename SingleValueParameterType::ValueType;
  std::string typeName = parameterElement->getChildElement("Type")->innerText();
  if (SingleValueParameterType::parameterName() != typeName) {
    return m_successor->createParameter(parameterElement);
  } else {
    std::string sParameterValue = parameterElement->getChildElement("Value")->innerText();
    ValType value = boost::lexical_cast<ValType>(sParameterValue);
    return new SingleValueParameterType(value);
  }
}

//------------------------------------------------------------------------------
/* Creates a parameter from an xml element. This is single-shot. Does not defer
to successor if it fails!.
@param parameterElement : xml Element
@return A fully constructed SingleValueParameterType.
*/
template <class SingleValueParameterType>
SingleValueParameterType *
SingleValueParameterParser<SingleValueParameterType>::createWithoutDelegation(Poco::XML::Element *parameterElement) {
  using ValType = typename SingleValueParameterType::ValueType;
  std::string typeName = parameterElement->getChildElement("Type")->innerText();
  if (SingleValueParameterType::parameterName() != typeName) {
    throw std::runtime_error("The attempted ::createWithoutDelegation failed. "
                             "The type provided does not match this parser.");
  } else {
    std::string sParameterValue = parameterElement->getChildElement("Value")->innerText();
    ValType value = boost::lexical_cast<ValType>(sParameterValue);
    return new SingleValueParameterType(value);
  }
}

//------------------------------------------------------------------------------
/* Sets the successor parser
@param parameterParser : the parser to defer to if the current instance can't
handle the parameter type.
*/
template <class SingleValueParameterType>
void SingleValueParameterParser<SingleValueParameterType>::setSuccessorParser(
    Mantid::API::ImplicitFunctionParameterParser *paramParser) {
  Mantid::API::ImplicitFunctionParameterParser::SuccessorType temp(paramParser);
  m_successor.swap(temp);
}
} // namespace API
} // namespace Mantid
