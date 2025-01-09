// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/Component.h"
#include <Poco/DOM/NamedNodeMap.h>
#include <Poco/SAX/Attributes.h>
#include <Poco/SAX/ContentHandler.h>

namespace Mantid {
namespace Geometry {

/** This class implements the Poco SAX ContentHandler class for reading
 * component XML

  @author Janik Zikovsky
  @date 2011-09-07
*/
class MANTID_GEOMETRY_DLL ComponentParser : public Poco::XML::ContentHandler {
public:
  /// Signals start of element
  void startElement(const Poco::XML::XMLString &, const Poco::XML::XMLString &localName, const Poco::XML::XMLString &,
                    const Poco::XML::Attributes &attr) override;
  /// Signals end of element
  void endElement(const Poco::XML::XMLString &, const Poco::XML::XMLString &localName,
                  const Poco::XML::XMLString &) override;

  void characters(const Poco::XML::XMLChar[], int, int) override;

  Component *getComponent();

  size_t size() const { return m_current.size(); }

  // These functions must be present as they are abstract in the base class.
  // They are not used them here.
  /// Signals start of XML document
  void startDocument() override {}                                                                   ///< Not used
  void setDocumentLocator(const Poco::XML::Locator *) override {}                                    ///< Not used
  void endDocument() override {}                                                                     ///< Not used
  void ignorableWhitespace(const Poco::XML::XMLChar[], int, int) override {}                         ///< Not used
  void processingInstruction(const Poco::XML::XMLString &, const Poco::XML::XMLString &) override {} ///< Not used
  void startPrefixMapping(const Poco::XML::XMLString &, const Poco::XML::XMLString &) override {}    ///< Not used
  void endPrefixMapping(const Poco::XML::XMLString &) override {}                                    ///< Not used
  void skippedEntity(const Poco::XML::XMLString &) override {}                                       ///< Not used

private:
  /// The components currently being built up.
  /// The one at the back of the vector is the latest one.
  std::vector<Component *> m_current;

  std::string m_innerText;
};
} // namespace Geometry
} // namespace Mantid
