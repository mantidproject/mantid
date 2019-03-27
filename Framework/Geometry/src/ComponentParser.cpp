// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/ComponentParser.h"

using namespace Mantid::Kernel;

namespace Mantid {
namespace Geometry {

/** @return the top-level component created */
Component *ComponentParser::getComponent() {
  if (!m_current.empty())
    return m_current[0];
  else
    return nullptr;
}

void ComponentParser::characters(const Poco::XML::XMLChar ch[], int start,
                                 int length) {
  m_innerText = std::string(ch + start, length);
}

//----------------------------------------------------------------------------------------------
/// Signals start of element
void ComponentParser::startElement(const Poco::XML::XMLString & /*uri*/,
                                   const Poco::XML::XMLString &localName,
                                   const Poco::XML::XMLString & /*qname*/,
                                   const Poco::XML::Attributes &attr) {
  // Find the parent of this new component.
  Component *current = nullptr;
  if (!m_current.empty())
    current = m_current.back();

  // for (int i=0; i<attr.getLength(); i++)  std::cout << i << " : "<<
  // attr.getQName(i) << "," << attr.getLocalName(i) << '\n';

  // Find the name in the attributes
  std::string name = attr.getValue("", "name");

  Component *newComp = nullptr;
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
void ComponentParser::endElement(const Poco::XML::XMLString & /*uri*/,
                                 const Poco::XML::XMLString &localName,
                                 const Poco::XML::XMLString & /*qname*/) {
  Component *current = nullptr;
  if (!m_current.empty())
    current = m_current.back();

  if (!current) {
    throw std::runtime_error("Failed to find last component");
  }

  if (localName == "pos") {
    V3D pos;
    pos.fromString(m_innerText);
    // std::cout << "found pos " << pos << '\n';
    current->setPos(pos);
  } else if (localName == "rot") {
    Quat rot;
    rot.fromString(m_innerText);
    // std::cout << "found rot " << rot << '\n';
    current->setRot(rot);
  }
}

} // namespace Geometry
} // namespace Mantid
