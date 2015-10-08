#ifndef IPARAMETER_H_
#define IPARAMETER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <sstream>
#include <vector>
#include <memory>

#include "MantidAPI/DllConfig.h"

#ifndef Q_MOC_RUN
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#endif

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>

namespace Mantid {
namespace API {
/** Abstract parameter type for use with IImplicitFunctions.

@author Owen Arnold, Tessella plc
@date 01/10/2010

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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

class MANTID_API_DLL ImplicitFunctionParameter {

public:
  virtual std::string getName() const = 0;

  virtual bool isValid() const = 0;

  virtual std::string toXMLString() const = 0;

  virtual ImplicitFunctionParameter *clone() const = 0;

  virtual ~ImplicitFunctionParameter() {}

protected:
  bool m_isValid;

  std::string parameterXMLTemplate(std::string valueXMLtext) const {
    using namespace Poco::XML;
    AutoPtr<Document> pDoc = new Document;
    AutoPtr<Element> paramElement = pDoc->createElement("Parameter");

    pDoc->appendChild(paramElement);
    AutoPtr<Element> typeElement = pDoc->createElement("Type");
    AutoPtr<Text> typeText = pDoc->createTextNode(this->getName());
    typeElement->appendChild(typeText);
    paramElement->appendChild(typeElement);

    AutoPtr<Element> valueElement = pDoc->createElement("Value");
    AutoPtr<Text> valueText = pDoc->createTextNode(valueXMLtext);
    valueElement->appendChild(valueText);
    paramElement->appendChild(valueElement);

    std::stringstream xmlstream;

    DOMWriter writer;
    writer.writeNode(xmlstream, pDoc);
    return xmlstream.str();
  }
};

//------------------------------------------------------------------------------------
// ElementTraits TypeTraits region
//------------------------------------------------------------------------------------

/** Default ElementTraits SFINAE
Typetraits are used to provide the correct formatting based on the element type
chosen.
*/
template <typename T> struct ElementTraits {};

/** ElementTraits for boolean element types.
*/
template <> struct ElementTraits<size_t> {
  typedef size_t ValueType;
  static std::string formatCS(const ValueType &value) {
    return boost::str(boost::format("%u,") % value);
  }
  static std::string format(const ValueType &value) {
    return boost::str(boost::format("%u") % value);
  }
};

/** ElementTraits for boolean element types.
*/
template <> struct ElementTraits<bool> {
  typedef bool ValueType;
  static std::string formatCS(const ValueType &value) {
    return boost::str(boost::format("%u,") % value);
  }
  static std::string format(const ValueType &value) {
    return boost::str(boost::format("%u") % value);
  }
};

/** ElementTraits for double element types.
*/
template <> struct ElementTraits<double> {
  typedef double ValueType;
  static std::string formatCS(const ValueType &value) {
    return boost::str(boost::format("%.4f,") % value);
  }
  static std::string format(const ValueType &value) {
    return boost::str(boost::format("%.4f") % value);
  }
};

/** ElementTraits for float element types.
*/
template <> struct ElementTraits<float> {
  typedef double ValueType;
  static std::string formatCS(const ValueType &value) {
    return boost::str(boost::format("%.4f,") % value);
  }
  static std::string format(const ValueType &value) {
    return boost::str(boost::format("%.4f") % value);
  }
};

//------------------------------------------------------------------------------------
// End ElementTraits TypeTraits region
//------------------------------------------------------------------------------------
}
}

#endif
