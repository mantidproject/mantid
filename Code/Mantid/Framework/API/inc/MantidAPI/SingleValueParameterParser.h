#ifndef SINGLE_VALUE_PARAMETER_PARSER_H_
#define SINGLE_VALUE_PARAMETER_PARSER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidKernel/System.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/File.h>
#include <Poco/Path.h>

#include "MantidAPI/ImplicitFunctionParameterParser.h"

#ifndef Q_MOC_RUN
# include <boost/lexical_cast.hpp>
#endif

namespace Mantid
{
namespace API
{
/**

 XML Parser for single value parameter types

 @author Owen Arnold, Tessella plc
 @date 01/10/2010

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

template<class SingleValueParameterType>
class DLLExport SingleValueParameterParser: public Mantid::API::ImplicitFunctionParameterParser
{
public:

  SingleValueParameterParser();

  Mantid::API::ImplicitFunctionParameter* createParameter(Poco::XML::Element* parameterElement);

  SingleValueParameterType* createWithoutDelegation(Poco::XML::Element* parameterElement);

  void setSuccessorParser(Mantid::API::ImplicitFunctionParameterParser* paramParser);

  ~SingleValueParameterParser();
};

//////////////////////////////////////////////////////////////////////////////////


/// Default constructor
template<class SingleValueParameterType>
SingleValueParameterParser<SingleValueParameterType>::SingleValueParameterParser()
{
}

//------------------------------------------------------------------------------
/* Creates a parameter from an xml element, otherwise defers to a successor parser.
@param parameterElement : xml Element
@return A fully constructed ImplicitFunctionParameter.
*/
template<class SingleValueParameterType>
Mantid::API::ImplicitFunctionParameter* SingleValueParameterParser<SingleValueParameterType>::createParameter(
    Poco::XML::Element* parameterElement)
{
  typedef typename SingleValueParameterType::ValueType ValType;
  std::string typeName = parameterElement->getChildElement("Type")->innerText();
  if (SingleValueParameterType::parameterName() != typeName)
  {
    return m_successor->createParameter(parameterElement);
  }
  else
  {
    std::string sParameterValue = parameterElement->getChildElement("Value")->innerText();
    ValType value = boost::lexical_cast<ValType>(sParameterValue);
    return new SingleValueParameterType(value);
  }
}

//------------------------------------------------------------------------------
/* Creates a parameter from an xml element. This is single-shot. Does not defer to successor if it fails!.
@param parameterElement : xml Element
@return A fully constructed SingleValueParameterType.
*/
template<class SingleValueParameterType>
SingleValueParameterType* SingleValueParameterParser<SingleValueParameterType>::createWithoutDelegation(
    Poco::XML::Element* parameterElement)
{
  typedef typename SingleValueParameterType::ValueType ValType;
  std::string typeName = parameterElement->getChildElement("Type")->innerText();
  if (SingleValueParameterType::parameterName() != typeName)
  {
    throw std::runtime_error("The attempted ::createWithoutDelegation failed. The type provided does not match this parser.");
  }
  else
  {
    std::string sParameterValue = parameterElement->getChildElement("Value")->innerText();
    ValType value = boost::lexical_cast<ValType>(sParameterValue);
    return new SingleValueParameterType(value);
  }
}

//------------------------------------------------------------------------------
/* Sets the successor parser
@param parameterParser : the parser to defer to if the current instance can't handle the parameter type.
*/
template<class SingleValueParameterType>
void SingleValueParameterParser<SingleValueParameterType>::setSuccessorParser(
    Mantid::API::ImplicitFunctionParameterParser* paramParser)
{
  Mantid::API::ImplicitFunctionParameterParser::SuccessorType temp(paramParser);
  m_successor.swap(temp);
}

/// Destructor.
template<class SingleValueParameterType>
SingleValueParameterParser<SingleValueParameterType>::~SingleValueParameterParser()
{
}

}
}

#endif
