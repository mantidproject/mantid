// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/SampleEnvironmentSpecParser.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"

#include "MantidKernel/MaterialXMLParser.h"

#include "Poco/DOM/AutoPtr.h"
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/DOMWriter.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/NamedNodeMap.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/SAX/InputSource.h"
#include "Poco/SAX/SAXException.h"

#include <boost/make_shared.hpp>
#include <sstream>

using namespace Poco::XML;

//------------------------------------------------------------------------------
// Anonymous
//------------------------------------------------------------------------------
namespace {
std::string MATERIALS_TAG = "materials";
std::string COMPONENTS_TAG = "components";
std::string COMPONENT_TAG = "component";
std::string CONTAINERS_TAG = "containers";
std::string CONTAINER_TAG = "container";
std::string COMPONENTGEOMETRY_TAG = "geometry";
std::string SAMPLEGEOMETRY_TAG = "samplegeometry";
} // namespace

namespace Mantid {
namespace Geometry {

//------------------------------------------------------------------------------
// Public methods
//------------------------------------------------------------------------------
/**
 * Takes a stream that is assumed to contain a single complete
 * SampleEnvironmentSpec definition, reads the definition
 * and produces a new SampleEnvironmentSpec object.
 * @param name The name of the environment
 * @param istr A reference to a stream
 * @return A new SampleEnvironmentSpec object
 */
SampleEnvironmentSpec_uptr
SampleEnvironmentSpecParser::parse(const std::string &name,
                                   std::istream &istr) {
  using DocumentPtr = AutoPtr<Document>;

  InputSource src(istr);
  DOMParser parser;
  // Do not use auto here or anywhereas the Poco API returns raw pointers
  // but in some circumstances requires AutoPtrs to manage the memory
  DocumentPtr doc;
  try {
    doc = parser.parse(&src);
  } catch (SAXParseException &exc) {
    std::ostringstream msg;
    msg << "SampleEnvironmentSpecParser::parse() - Error parsing content "
           "as valid XML: "
        << exc.what();
    throw std::runtime_error(msg.str());
  }
  return parse(name, doc->documentElement());
}

/**
 * Takes a pointer to an XML node that is assumed to point at a
 * "environmentspec" tag.
 * It reads the definition and produces a new SampleEnvironmentSpec object.
 * @param name The name of the environment
 * @param element A pointer to an Element node that is a "environmentspec" tag
 * @return A new SampleEnvironmentSpec object
 */
SampleEnvironmentSpec_uptr
SampleEnvironmentSpecParser::parse(const std::string &name,
                                   Poco::XML::Element *element) {
  validateRootElement(element);

  // Iterating is apparently much faster than getElementsByTagName
  NodeIterator nodeIter(element, NodeFilter::SHOW_ELEMENT);
  Node *node = nodeIter.nextNode();
  auto spec = std::make_unique<SampleEnvironmentSpec>(name);
  while (node) {
    auto *childElement = static_cast<Element *>(node);
    if (node->nodeName() == MATERIALS_TAG) {
      parseMaterials(childElement);
    } else if (node->nodeName() == COMPONENTS_TAG) {
      parseAndAddComponents(spec.get(), childElement);
    }
    node = nodeIter.nextNode();
  }
  return spec;
}

//------------------------------------------------------------------------------
// Private methods
//------------------------------------------------------------------------------
/**
 * Validate that the element points to the expected root element
 * @param element A pointer to the root element
 */
void SampleEnvironmentSpecParser::validateRootElement(
    Poco::XML::Element *element) const {
  if (element->nodeName() != ROOT_TAG) {
    std::ostringstream msg;
    msg << "SampleEnvironmentSpecParser::validateRootElement() - Element tag "
           "does not match '"
        << ROOT_TAG << "'. Found " << element->nodeName() << "\n";
    throw std::invalid_argument(msg.str());
  }
}

/**
 * Parse the set of materials in the document
 * @param element A pointer to the materials tag
 */
void SampleEnvironmentSpecParser::parseMaterials(Poco::XML::Element *element) {
  using Kernel::MaterialXMLParser;

  m_materials.clear();
  NodeIterator nodeIter(element, NodeFilter::SHOW_ELEMENT);
  // Points at <materials>
  nodeIter.nextNode();
  // Points at first <material>
  Node *node = nodeIter.nextNode();
  MaterialXMLParser parser;
  while (node) {
    auto material = parser.parse(static_cast<Poco::XML::Element *>(node));
    m_materials.emplace(material.name(), material);
    node = nodeIter.nextNode();
  }
}

/**
 * Take a \<components\> tag, parse the definitions and add them to the spec.
 * It requires the materials to have been parsed
 * @param spec A pointer to a SampleEnvironmentSpec to update
 * @param element A pointer to a components element
 */
void SampleEnvironmentSpecParser::parseAndAddComponents(
    SampleEnvironmentSpec *spec, Element *element) const {
  if (m_materials.empty()) {
    throw std::runtime_error("SampleEnvironmentSpecParser::parseComponents() - "
                             "Trying to parse list of components but no "
                             "materials have been found. Please ensure the "
                             "materials are defined first.");
  }
  NodeIterator nodeIter(element, NodeFilter::SHOW_ELEMENT);
  // Points at <components>
  nodeIter.nextNode();
  // Points at first <child>
  Node *node = nodeIter.nextNode();
  while (node) {
    auto *childElement = static_cast<Element *>(node);
    const auto &nodeName = childElement->nodeName();
    if (nodeName == CONTAINERS_TAG) {
      parseAndAddContainers(spec, childElement);
    } else if (nodeName == COMPONENT_TAG) {
      spec->addComponent(parseComponent(childElement));
    }
    node = nodeIter.nextNode();
  }
}

/**
 * Take a \<containers\> tag, parse the definitions and add them to the spec.
 * It requires the materials to have been parsed.
 * @param spec A pointer to a SampleEnvironmentSpec to update
 * @param element A pointer to a cans element
 */
void SampleEnvironmentSpecParser::parseAndAddContainers(
    SampleEnvironmentSpec *spec, Element *element) const {
  NodeIterator nodeIter(element, NodeFilter::SHOW_ELEMENT);
  nodeIter.nextNode();
  Node *node = nodeIter.nextNode();
  while (node) {
    auto *childElement = static_cast<Element *>(node);
    if (childElement->nodeName() == CONTAINER_TAG) {
      spec->addContainer(parseContainer(childElement));
    }
    node = nodeIter.nextNode();
  }
}

/**
 * Parse a single definition of a Can
 * @param element A pointer to an XML \<container\> element
 * @return A new Can instance
 */
Container_const_sptr
SampleEnvironmentSpecParser::parseContainer(Element *element) const {
  using Mantid::Geometry::Container;
  auto can = boost::make_shared<Container>(parseComponent(element));
  auto sampleGeometry = element->getChildElement(SAMPLEGEOMETRY_TAG);
  if (sampleGeometry) {
    DOMWriter writer;
    std::stringstream sampleShapeXML;
    writer.writeNode(sampleShapeXML, sampleGeometry);
    can->setSampleShape(sampleShapeXML.str());
  }
  return can;
}

/**
 * Parse a single definition of a component. If the component is a can the
 * sample geometry, if available, is also parsed.
 * @param element A pointer to an XML \<container\> element
 * @return A new Object instance of the given type
 */
boost::shared_ptr<IObject>
Mantid::Geometry::SampleEnvironmentSpecParser::parseComponent(
    Element *element) const {
  Element *geometry = element->getChildElement(COMPONENTGEOMETRY_TAG);
  if (!geometry) {
    throw std::runtime_error(
        "SampleEnvironmentSpecParser::parseCan() - Expected a " +
        COMPONENTGEOMETRY_TAG + " child tag. None found.");
  }
  ShapeFactory factory;
  auto comp = factory.createShape(geometry);
  auto materialID = element->getAttribute("material");
  auto iter = m_materials.find(materialID);
  Kernel::Material mat;
  if (iter != m_materials.end()) {
    mat = iter->second;
  } else {
    throw std::runtime_error("SampleEnvironmentSpecParser::parseComponent() - "
                             "Unable to find material with id=" +
                             materialID);
  }
  comp->setID(element->getAttribute("id"));
  comp->setMaterial(mat);
  return comp;
}

} // namespace Geometry
} // namespace Mantid
