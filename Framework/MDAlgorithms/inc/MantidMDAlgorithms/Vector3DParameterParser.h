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

#include <boost/algorithm/string.hpp>

namespace Poco {
namespace DOM {
class Element;
}
} // namespace Poco
namespace Mantid {
namespace MDAlgorithms {
/**

 XML parser for vector value (3 elements) parameter types.

 @author Owen Arnold, Tessella plc
 @date 02/11/2011
 */
template <class VectorValueParameterType>
class DLLExport Vector3DParameterParser : public Mantid::API::ImplicitFunctionParameterParser {
public:
  VectorValueParameterType *parseVectorParameter(std::string value);

  Mantid::API::ImplicitFunctionParameter *createParameter(Poco::XML::Element *parameterElement) override;

  void setSuccessorParser(Mantid::API::ImplicitFunctionParameterParser *paramParser) override;
};

////////////////////////////////////////////////////////////////////

template <typename VectorValueParameterType>
VectorValueParameterType *Vector3DParameterParser<VectorValueParameterType>::parseVectorParameter(std::string value) {
  std::vector<std::string> strs;
  boost::split(strs, value, boost::is_any_of(","));

  double nx, ny, nz;
  try {
    nx = std::stod(strs.at(0));
    ny = std::stod(strs.at(1));
    nz = std::stod(strs.at(2));
  } catch (std::exception &ex) {
    std::string message =
        std::string(ex.what()) + " Failed to parse " + VectorValueParameterType::parameterName() + " value: " + value;
    throw std::invalid_argument(message.c_str());
  }
  return new VectorValueParameterType(nx, ny, nz);
}

template <typename VectorValueParameterType>
Mantid::API::ImplicitFunctionParameter *
Vector3DParameterParser<VectorValueParameterType>::createParameter(Poco::XML::Element *parameterElement) {
  std::string typeName = parameterElement->getChildElement("Type")->innerText();
  if (VectorValueParameterType::parameterName() != typeName) {
    return m_successor->createParameter(parameterElement);
  } else {
    std::string sParameterValue = parameterElement->getChildElement("Value")->innerText();
    return parseVectorParameter(sParameterValue);
  }
}

template <typename VectorValueParameterType>
void Vector3DParameterParser<VectorValueParameterType>::setSuccessorParser(
    Mantid::API::ImplicitFunctionParameterParser *paramParser) {
  Mantid::API::ImplicitFunctionParameterParser::SuccessorType temp(paramParser);
  m_successor.swap(temp);
}
} // namespace MDAlgorithms
} // namespace Mantid
