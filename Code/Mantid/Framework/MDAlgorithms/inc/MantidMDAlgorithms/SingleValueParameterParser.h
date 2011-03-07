#ifndef SINGLE_VALUE_PARAMETER_PARSER_H_
#define SINGLE_VALUE_PARAMETER_PARSER_H_

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

#include "MantidKernel/System.h"
#include "MantidAPI/ImplicitFunctionParameterParser.h"

#include "MantidMDAlgorithms/WidthParameter.h"
#include "MantidMDAlgorithms/HeightParameter.h"
#include "MantidMDAlgorithms/DepthParameter.h"

namespace Mantid
{
namespace MDAlgorithms
{
/**

 XML Parser for single value parameter types

 @author Owen Arnold, Tessella plc
 @date 01/10/2010

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

template<class SingleValueParameterType>
class DLLExport SingleValueParameterParser: public Mantid::API::ImplicitFunctionParameterParser
{
public:

  SingleValueParameterParser();

  Mantid::API::ImplicitFunctionParameter* createParameter(Poco::XML::Element* parameterElement);

  void setSuccessorParser(Mantid::API::ImplicitFunctionParameterParser* paramParser);

  ~SingleValueParameterParser();
};

//////////////////////////////////////////////////////////////////////////////////

template<class SingleValueParameterType>
SingleValueParameterParser<SingleValueParameterType>::SingleValueParameterParser()
{
}

template<class SingleValueParameterType>
Mantid::API::ImplicitFunctionParameter* SingleValueParameterParser<SingleValueParameterType>::createParameter(
    Poco::XML::Element* parameterElement)
{
  std::string typeName = parameterElement->getChildElement("Type")->innerText();
  if (SingleValueParameterType::parameterName() != typeName)
  {
    return m_successor->createParameter(parameterElement);
  }
  else
  {
    std::string sParameterValue = parameterElement->getChildElement("Value")->innerText();
    double value = atof(sParameterValue.c_str());
    return new SingleValueParameterType(value);
  }
}

template<class SingleValueParameterType>
void SingleValueParameterParser<SingleValueParameterType>::setSuccessorParser(
    Mantid::API::ImplicitFunctionParameterParser* paramParser)
{
  m_successor = std::auto_ptr<ImplicitFunctionParameterParser>(paramParser);
}

template<class SingleValueParameterType>
SingleValueParameterParser<SingleValueParameterType>::~SingleValueParameterParser()
{
}

typedef SingleValueParameterParser<WidthParameter> WidthParameterParser;
typedef SingleValueParameterParser<HeightParameter> HeightParameterParser;
typedef SingleValueParameterParser<DepthParameter> DepthParameterParser;
}
}

#endif
