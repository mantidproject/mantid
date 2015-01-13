#ifndef MANTID_GEOMETRY_COMPONENTPARSER_H_
#define MANTID_GEOMETRY_COMPONENTPARSER_H_

#include "MantidKernel/System.h"
#include <Poco/SAX/ContentHandler.h>
#include <Poco/DOM/NamedNodeMap.h>
#include <Poco/SAX/Attributes.h>
#include "MantidGeometry/Instrument/Component.h"

namespace Mantid {
namespace Geometry {

/** This class implements the Poco SAX ContentHandler class for reading
 * component XML

  @author Janik Zikovsky
  @date 2011-09-07

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport ComponentParser : public Poco::XML::ContentHandler {
public:
  ComponentParser();
  ~ComponentParser();

  /// Signals start of element
  void startElement(const Poco::XML::XMLString &,
                    const Poco::XML::XMLString &localName,
                    const Poco::XML::XMLString &,
                    const Poco::XML::Attributes &attr);
  /// Signals end of element
  void endElement(const Poco::XML::XMLString &,
                  const Poco::XML::XMLString &localName,
                  const Poco::XML::XMLString &);

  void characters(const Poco::XML::XMLChar[], int, int);

  Component *getComponent();

  size_t size() const { return m_current.size(); }

  // These functions must be present as they are abstract in the base class.
  // They are not used them here.
  /// Signals start of XML document
  void startDocument() {}                                ///<Not used
  void setDocumentLocator(const Poco::XML::Locator *) {} ///< Not used
  void endDocument() {}                                  ///< Not used
  void ignorableWhitespace(const Poco::XML::XMLChar[], int, int) {
  } ///< Not used
  void processingInstruction(const Poco::XML::XMLString &,
                             const Poco::XML::XMLString &) {} ///< Not used
  void startPrefixMapping(const Poco::XML::XMLString &,
                          const Poco::XML::XMLString &) {} ///< Not used
  void endPrefixMapping(const Poco::XML::XMLString &) {}   ///< Not used
  void skippedEntity(const Poco::XML::XMLString &) {}      ///< Not used

private:
  /// The components currently being built up.
  /// The one at the back of the vector is the latest one.
  std::vector<Component *> m_current;

  std::string m_innerText;
};
} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_COMPONENTPARSER_H_ */
