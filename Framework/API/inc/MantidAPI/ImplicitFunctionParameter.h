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
#include <memory>
#include <sstream>
#include <vector>

#include "MantidAPI/DllConfig.h"

#ifndef Q_MOC_RUN
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#endif

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>

namespace Mantid {
namespace API {
/** Abstract parameter type for use with IImplicitFunctions.

@author Owen Arnold, Tessella plc
@date 01/10/2010
*/

class MANTID_API_DLL ImplicitFunctionParameter {

public:
  virtual std::string getName() const = 0;

  virtual bool isValid() const = 0;

  virtual std::string toXMLString() const = 0;

  virtual ImplicitFunctionParameter *clone() const = 0;

  virtual ~ImplicitFunctionParameter() = default;

protected:
  bool m_isValid;

  std::string parameterXMLTemplate(const std::string &valueXMLtext) const {
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
  using ValueType = size_t;
  static std::string formatCS(const ValueType &value) { return boost::str(boost::format("%u,") % value); }
  static std::string format(const ValueType &value) { return boost::str(boost::format("%u") % value); }
};

/** ElementTraits for boolean element types.
 */
template <> struct ElementTraits<bool> {
  using ValueType = bool;
  static std::string formatCS(const ValueType &value) { return boost::str(boost::format("%u,") % value); }
  static std::string format(const ValueType &value) { return boost::str(boost::format("%u") % value); }
};

/** ElementTraits for double element types.
 */
template <> struct ElementTraits<double> {
  using ValueType = double;
  static std::string formatCS(const ValueType &value) { return boost::str(boost::format("%.4f,") % value); }
  static std::string format(const ValueType &value) { return boost::str(boost::format("%.4f") % value); }
};

/** ElementTraits for float element types.
 */
template <> struct ElementTraits<float> {
  using ValueType = double;
  static std::string formatCS(const ValueType &value) { return boost::str(boost::format("%.4f,") % value); }
  static std::string format(const ValueType &value) { return boost::str(boost::format("%.4f") % value); }
};

//------------------------------------------------------------------------------------
// End ElementTraits TypeTraits region
//------------------------------------------------------------------------------------
} // namespace API
} // namespace Mantid
