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

#include "MantidAPI/ImplicitFunctionParameterParser.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#ifndef Q_MOC_RUN
#include <boost/lexical_cast.hpp>
#endif

namespace Mantid {
namespace API {
/**

 XML parser for vector value (n elements) parameter types.

 @author Owen Arnold, Tessella plc
 @date 21/07/2011
 */
template <class VectorValueParameterType>
class DLLExport VectorParameterParser : public Mantid::API::ImplicitFunctionParameterParser {
public:
  VectorValueParameterType *parseVectorParameter(std::string sValue);

  Mantid::API::ImplicitFunctionParameter *createParameter(Poco::XML::Element *parameterElement) override;

  VectorValueParameterType *createWithoutDelegation(Poco::XML::Element *parameterElement);

  void setSuccessorParser(Mantid::API::ImplicitFunctionParameterParser *paramParser) override;
};

////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------
/* Parse the value aspect of the parameter only.
@param sValue : parameter value in string form.
@return new parameter object.
*/
template <typename VectorValueParameterType>
VectorValueParameterType *VectorParameterParser<VectorValueParameterType>::parseVectorParameter(std::string sValue) {
  std::vector<std::string> strs;
  boost::split(strs, sValue, boost::is_any_of(","));

  auto product = new VectorValueParameterType(strs.size());
  using ValType = typename VectorValueParameterType::ValueType;
  ValType value = 0;

  for (size_t i = 0; i < strs.size(); i++) {
    boost::erase_all(strs[i], " ");
    value = boost::lexical_cast<ValType>(strs[i]);
    product->addValue(i, value);
  }
  return product;
}

//----------------------------------------------------------------------
/* Create parameter from xml element.
@param parameterElement : XML element containing the parameter info.
@return new parameter object.
*/
template <typename VectorValueParameterType>
Mantid::API::ImplicitFunctionParameter *
VectorParameterParser<VectorValueParameterType>::createParameter(Poco::XML::Element *parameterElement) {
  std::string typeName = parameterElement->getChildElement("Type")->innerText();
  if (VectorValueParameterType::parameterName() != typeName) {
    if (!m_successor) {
      throw std::runtime_error("No successor ParameterParser!");
    }
    return m_successor->createParameter(parameterElement);
  } else {
    std::string sParameterValue = parameterElement->getChildElement("Value")->innerText();
    return parseVectorParameter(sParameterValue);
  }
}

//------------------------------------------------------------------------------
/* Creates a parameter from an xml element. This is single-shot. Does not defer
to successor if it fails!.
@param parameterElement : xml Element
@return A fully constructed VectorValueParameterType.
*/
template <class VectorValueParameterType>
VectorValueParameterType *
VectorParameterParser<VectorValueParameterType>::createWithoutDelegation(Poco::XML::Element *parameterElement) {
  std::string typeName = parameterElement->getChildElement("Type")->innerText();
  if (VectorValueParameterType::parameterName() != typeName) {
    throw std::runtime_error("The attempted ::createWithoutDelegation failed. "
                             "The type provided does not match this parser.");
  } else {
    std::string sParameterValue = parameterElement->getChildElement("Value")->innerText();
    return parseVectorParameter(sParameterValue);
  }
}

//----------------------------------------------------------------------
/* Setter for a successor parser.
@param paramParser : successor
*/
template <typename VectorValueParameterType>
void VectorParameterParser<VectorValueParameterType>::setSuccessorParser(
    Mantid::API::ImplicitFunctionParameterParser *paramParser) {
  Mantid::API::ImplicitFunctionParameterParser::SuccessorType temp(paramParser);
  m_successor.swap(temp);
}
} // namespace API
} // namespace Mantid
