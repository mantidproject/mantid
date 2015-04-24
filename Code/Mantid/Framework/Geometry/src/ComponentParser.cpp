#include "MantidGeometry/ComponentParser.h"

using namespace Mantid::Kernel;

namespace Mantid {
namespace Geometry {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ComponentParser::ComponentParser() { m_current.clear(); }

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ComponentParser::~ComponentParser() {}

/** @return the top-level component created */
Component *ComponentParser::getComponent() {
  if (m_current.size() > 0)
    return m_current[0];
  else
    return NULL;
}

void ComponentParser::characters(const Poco::XML::XMLChar ch[], int start,
                                 int length) {
  m_innerText = std::string(ch + start, length);
}

//----------------------------------------------------------------------------------------------
/// Signals start of element
void ComponentParser::startElement(const Poco::XML::XMLString &,
                                   const Poco::XML::XMLString &localName,
                                   const Poco::XML::XMLString &,
                                   const Poco::XML::Attributes &attr) {
  // Find the parent of this new component.
  Component *current = NULL;
  if (!m_current.empty())
    current = m_current.back();

  // for (int i=0; i<attr.getLength(); i++)  std::cout << i << " : "<<
  // attr.getQName(i) << "," << attr.getLocalName(i) << std::endl;

  // Find the name in the attributes
  std::string name = attr.getValue("", "name");

  Component *newComp = NULL;
  if (localName == "Component")
    newComp = new Component(name, current);
  else {
    // throw std::runtime_error("ComponentParser:: unexpected XML tag '" +
    // localName + "'.");
  }

  // A new component was created
  if (newComp) {
    m_current.push_back(newComp);
    // Read the attributes into the new component
    newComp->readXMLAttributes(attr);
  }
}

//----------------------------------------------------------------------------------------------
/// Signals end of element
void ComponentParser::endElement(const Poco::XML::XMLString &,
                                 const Poco::XML::XMLString &localName,
                                 const Poco::XML::XMLString &) {
  Component *current = NULL;
  if (!m_current.empty())
    current = m_current.back();

  if (localName == "pos") {
    V3D pos;
    pos.fromString(m_innerText);
    // std::cout << "found pos " << pos << std::endl;
    current->setPos(pos);
  } else if (localName == "rot") {
    Quat rot;
    rot.fromString(m_innerText);
    // std::cout << "found rot " << rot << std::endl;
    current->setRot(rot);
  }
}

} // namespace Mantid
} // namespace Geometry
