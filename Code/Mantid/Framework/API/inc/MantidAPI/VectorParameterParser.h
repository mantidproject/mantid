#ifndef VECTOR_VALUE_PARAMETER_PARSER_H_
#define VECTOR_VALUE_PARAMETER_PARSER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"

#include "MantidAPI/ImplicitFunctionParameterParser.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#ifndef Q_MOC_RUN
# include <boost/lexical_cast.hpp>
#endif 

namespace Mantid
{
namespace API
{
/**

 XML parser for vector value (n elements) parameter types.

 @author Owen Arnold, Tessella plc
 @date 21/07/2011
 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

 File change history is stored at: <https://github.com/mantidproject/mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
template<class VectorValueParameterType>
class DLLExport VectorParameterParser: public Mantid::API::ImplicitFunctionParameterParser
{
public:
  VectorParameterParser();

  VectorValueParameterType* parseVectorParameter(std::string value);

  Mantid::API::ImplicitFunctionParameter* createParameter(Poco::XML::Element* parameterElement);

  VectorValueParameterType* createWithoutDelegation(Poco::XML::Element* parameterElement);

  void setSuccessorParser(Mantid::API::ImplicitFunctionParameterParser* paramParser);

  ~VectorParameterParser();
};

////////////////////////////////////////////////////////////////////

/// Default constructor
template<typename VectorValueParameterType>
VectorParameterParser<VectorValueParameterType>::VectorParameterParser()
{
}

//----------------------------------------------------------------------
/* Parse the value aspect of the parameter only.
@param sValue : parameter value in string form.
@return new parameter object.
*/
template<typename VectorValueParameterType>
VectorValueParameterType* VectorParameterParser<VectorValueParameterType>::parseVectorParameter(
    std::string sValue)
{
  std::vector<std::string> strs;
  boost::split(strs, sValue, boost::is_any_of(","));

  VectorValueParameterType* product = new VectorValueParameterType(strs.size());
  typedef typename VectorValueParameterType::ValueType ValType;
  ValType value = 0;

  for(size_t i = 0; i < strs.size(); i++)
  {
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
template<typename VectorValueParameterType>
Mantid::API::ImplicitFunctionParameter* VectorParameterParser<VectorValueParameterType>::createParameter(
    Poco::XML::Element* parameterElement)
{
  std::string typeName = parameterElement->getChildElement("Type")->innerText();
  if (VectorValueParameterType::parameterName() != typeName)
  {
    if(!m_successor)
    {
      throw std::runtime_error("No successor ParameterParser!");
    }
    return m_successor->createParameter(parameterElement);
  }
  else
  {
    std::string sParameterValue = parameterElement->getChildElement("Value")->innerText();
    return parseVectorParameter(sParameterValue);
  }
}

//------------------------------------------------------------------------------
/* Creates a parameter from an xml element. This is single-shot. Does not defer to successor if it fails!.
@param parameterElement : xml Element
@return A fully constructed VectorValueParameterType.
*/
template<class VectorValueParameterType>
VectorValueParameterType* VectorParameterParser<VectorValueParameterType>::createWithoutDelegation(
    Poco::XML::Element* parameterElement)
{
  std::string typeName = parameterElement->getChildElement("Type")->innerText();
  if (VectorValueParameterType::parameterName() != typeName)
  {
    throw std::runtime_error("The attempted ::createWithoutDelegation failed. The type provided does not match this parser.");
  }
  else
  {
    std::string sParameterValue = parameterElement->getChildElement("Value")->innerText();
    return parseVectorParameter(sParameterValue);
  }
}

//----------------------------------------------------------------------
/* Setter for a successor parser.
@param paramParser : successor
*/
template<typename VectorValueParameterType>
void VectorParameterParser<VectorValueParameterType>::setSuccessorParser(
    Mantid::API::ImplicitFunctionParameterParser* paramParser)
{
  Mantid::API::ImplicitFunctionParameterParser::SuccessorType temp(paramParser);
  m_successor.swap(temp);
}

/// Destructor
template<typename VectorValueParameterType>
VectorParameterParser<VectorValueParameterType>::~VectorParameterParser()
{
}

}
}

#endif
