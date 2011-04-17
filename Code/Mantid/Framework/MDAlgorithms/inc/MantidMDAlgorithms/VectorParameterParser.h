#ifndef VECTOR_VALUE_PARAMETER_PARSER_H_
#define VECTOR_VALUE_PARAMETER_PARSER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/File.h>
#include <Poco/Path.h>

#include <boost/algorithm/string.hpp>
#include "MantidKernel/System.h"

#include "MantidAPI/ImplicitFunctionParameterParser.h"
#include "MantidMDAlgorithms/UpParameter.h"
#include "MantidMDAlgorithms/PerpendicularParameter.h"
#include "MantidMDAlgorithms/NormalParameter.h"
#include "MantidMDAlgorithms/OriginParameter.h"

namespace Mantid
{
namespace MDAlgorithms
{
/**

 XML parser for vector value (3 elements) parameter types.

 @author Owen Arnold, Tessella plc
 @date 02/11/2011
 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
template<class VectorValueParameterType>
class DLLExport VectorParameterParser: public Mantid::API::ImplicitFunctionParameterParser
{
public:
  VectorParameterParser();

  VectorValueParameterType* parseVectorParameter(std::string value);

  Mantid::API::ImplicitFunctionParameter* createParameter(Poco::XML::Element* parameterElement);

  void setSuccessorParser(Mantid::API::ImplicitFunctionParameterParser* paramParser);

  ~VectorParameterParser();
};

////////////////////////////////////////////////////////////////////

template<typename VectorValueParameterType>
VectorParameterParser<VectorValueParameterType>::VectorParameterParser()
{
}

template<typename VectorValueParameterType>
VectorValueParameterType* VectorParameterParser<VectorValueParameterType>::parseVectorParameter(
    std::string value)
{
  std::vector<std::string> strs;
  boost::split(strs, value, boost::is_any_of(","));

  double nx, ny, nz;
  try
  {
    nx = atof(strs.at(0).c_str());
    ny = atof(strs.at(1).c_str());
    nz = atof(strs.at(2).c_str());
  } catch (std::exception& ex)
  {
    std::string message = std::string(ex.what()) + " Failed to parse "
        + VectorValueParameterType::parameterName() + " value: " + value;
    throw std::invalid_argument(message.c_str());
  }
  return new VectorValueParameterType(nx, ny, nz);
}

template<typename VectorValueParameterType>
Mantid::API::ImplicitFunctionParameter* VectorParameterParser<VectorValueParameterType>::createParameter(
    Poco::XML::Element* parameterElement)
{
  std::string typeName = parameterElement->getChildElement("Type")->innerText();
  if (VectorValueParameterType::parameterName() != typeName)
  {
    return m_successor->createParameter(parameterElement);
  }
  else
  {
    std::string sParameterValue = parameterElement->getChildElement("Value")->innerText();
    return parseVectorParameter(sParameterValue);
  }
}

template<typename VectorValueParameterType>
void VectorParameterParser<VectorValueParameterType>::setSuccessorParser(
    Mantid::API::ImplicitFunctionParameterParser* paramParser)
{
  Mantid::API::ImplicitFunctionParameterParser::SuccessorType temp(paramParser);
  m_successor.swap(temp);
}

template<typename VectorValueParameterType>
VectorParameterParser<VectorValueParameterType>::~VectorParameterParser()
{
}

//Declare types based on this template.

/// Parses Origin Parameters
typedef VectorParameterParser<OriginParameter> OriginParameterParser;

/// Parses Normal Parameters
typedef VectorParameterParser<NormalParameter> NormalParameterParser;

/// Parses Up Parameters
typedef VectorParameterParser<UpParameter> UpParameterParser;

/// Parses Perpendicular Parameters
typedef VectorParameterParser<PerpendicularParameter> PerpendicularParameterParser;
}
}

#endif
