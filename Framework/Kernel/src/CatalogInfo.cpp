// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/CatalogInfo.h"

#include <Poco/AutoPtr.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/XML/XMLString.h>

namespace Mantid {
namespace Kernel {
/**
 * Constructor
 * @param element :: "Catalog" element from Facilities.xml
 */
CatalogInfo::CatalogInfo(const Poco::XML::Element *element) {
  m_catalogName = getAttribute(element, "catalog", "name");
  m_soapEndPoint = getAttribute(element, "soapendpoint", "url");
  m_externalDownloadURL = getAttribute(element, "externaldownload", "url");
  m_catalogPrefix = getAttribute(element, "prefix", "regex");
  m_windowsPrefix = getAttribute(element, "windows", "replacement");
  m_macPrefix = getAttribute(element, "mac", "replacement");
  m_linuxPrefix = getAttribute(element, "linux", "replacement");
}

CatalogInfo::CatalogInfo(const CatalogInfo &other)
    : m_catalogName(other.m_catalogName), m_soapEndPoint(other.m_soapEndPoint),
      m_externalDownloadURL(other.m_externalDownloadURL),
      m_catalogPrefix(other.m_catalogPrefix),
      m_windowsPrefix(other.m_windowsPrefix), m_macPrefix(other.m_macPrefix),
      m_linuxPrefix(other.m_linuxPrefix) {}

/**
 * Obtain catalog name from the facility file.
 */
const std::string CatalogInfo::catalogName() const { return (m_catalogName); }

/**
 * Obtain soap end point from the facility file.
 */
const std::string CatalogInfo::soapEndPoint() const { return (m_soapEndPoint); }

/**
 * Obtain catalog name from the facility file.
 */
const std::string CatalogInfo::externalDownloadURL() const {
  return (m_externalDownloadURL);
}

/**
 * Obtain the regex prefix for default archive path.
 */
const std::string CatalogInfo::catalogPrefix() const {
  return (m_catalogPrefix);
}

/**
 * Obtain Windows prefix from the facility file.
 */
const std::string CatalogInfo::windowsPrefix() const {
  return (m_windowsPrefix);
}

/**
 * Obtain Macintosh prefix from facility file.
 */
const std::string CatalogInfo::macPrefix() const { return (m_macPrefix); }

/**
 * Obtain Linux prefix from facility file.
 */
const std::string CatalogInfo::linuxPrefix() const { return (m_linuxPrefix); }

/**
 * Virtual constructor
 * @return deep copy of this
 */
CatalogInfo *CatalogInfo::clone() const { return new CatalogInfo(*this); }

/**
 * Obtain the attribute from a given element tag and attribute name.
 * @param element       :: The name of the element in the XML file.
 * @param tagName       :: The name of the tag to search for.
 * @param attributeName :: The name of the attribute for the given tag.
 * @return The contents of the attribute from an XML element.
 */
std::string CatalogInfo::getAttribute(const Poco::XML::Element *element,
                                      const std::string &tagName,
                                      const std::string &attributeName) {
  Poco::AutoPtr<Poco::XML::NodeList> elementTag =
      element->getElementsByTagName(tagName);

  // If the tag exists in the XML file.
  if (elementTag->length() == 1) {
    auto *item = dynamic_cast<Poco::XML::Element *>(elementTag->item(0));

    // If the item does exist, then we want to return it.
    if (!item->getAttribute(attributeName).empty()) {
      return (item->getAttribute(attributeName));
    }
  }
  return ("");
}

} // namespace Kernel
} // namespace Mantid
