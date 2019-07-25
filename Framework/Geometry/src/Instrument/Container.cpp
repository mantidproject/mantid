// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/Container.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"

#include "Poco/DOM/AutoPtr.h"
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/SAX/InputSource.h"
#include "Poco/SAX/SAXException.h"
#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {

namespace {
constexpr const char *SAMPLEGEOMETRY_TAG = "samplegeometry";
constexpr const char *VAL_ATT = "val";

//------------------------------------------------------------------------------
// Anonymous methods
//------------------------------------------------------------------------------
/**
 * Update the values of the XML tree tags specified. It assumes that the
 * value is specifed using an attribute named 'val'
 * @param root A pointer to an element whose childnodes contain "val" attributes
 * to be updated
 * @param args A hash of tag names to values
 */
void updateTreeValues(Poco::XML::Element *root,
                      const Container::ShapeArgs &args) {
  using namespace Poco::XML;
  NodeIterator nodeIter(root, NodeFilter::SHOW_ELEMENT);
  Node *node = nodeIter.nextNode();
  while (node) {
    auto *element = static_cast<Element *>(node);
    auto argIter = args.find(node->nodeName());
    if (argIter != args.end()) {
      element->setAttribute(VAL_ATT, std::to_string(argIter->second));
    }
    node = nodeIter.nextNode();
  }
}
} // namespace

//------------------------------------------------------------------------------
// Public methods
//------------------------------------------------------------------------------
Container::Container() : m_shape(boost::make_shared<CSGObject>()) {}

Container::Container(IObject_sptr shape) : m_shape(shape) {}

Container::Container(const Container &container)
    : m_shape(IObject_sptr(container.m_shape->clone())),
      m_sampleShapeXML(container.m_sampleShapeXML) {}

/**
 * Construct a container providing an XML definition shape
 * @param xml Definition of the shape in xml
 */
Container::Container(std::string xml)
    : m_shape(boost::make_shared<CSGObject>(xml)) {}

/**
 * @return True if the can contains a defintion of the sample shape
 */
bool Container::hasSampleShape() const { return !m_sampleShapeXML.empty(); }

/**
 * Return an object that represents the sample shape from the current
 * definition but override the default values with the given values.
 * It throws a std::runtime_error if no sample shape is defined.
 * @param args A hash of tag values to use in place of the default
 * @return A pointer to a object modeling the sample shape
 */
IObject_sptr
Container::createSampleShape(const Container::ShapeArgs &args) const {
  using namespace Poco::XML;
  if (!hasSampleShape()) {
    throw std::runtime_error("Can::createSampleShape() - No definition found "
                             "for the sample geometry.");
  }
  // Parse XML
  std::istringstream instrm(m_sampleShapeXML);
  InputSource src(instrm);
  DOMParser parser;
  AutoPtr<Document> doc;
  try {
    doc = parser.parse(&src);
  } catch (SAXParseException &exc) {
    std::ostringstream os;
    os << "Can::createSampleShape() - Error parsing XML: " << exc.what();
    throw std::invalid_argument(os.str());
  }
  Element *root = doc->documentElement();
  if (!args.empty())
    updateTreeValues(root, args);

  ShapeFactory factory;
  return factory.createShape(root);
}

/**
 * Set the definition of the sample shape for this can
 * @param sampleShapeXML
 */
void Container::setSampleShape(const std::string &sampleShapeXML) {
  using namespace Poco::XML;
  std::istringstream instrm(sampleShapeXML);
  InputSource src(instrm);
  DOMParser parser;
  AutoPtr<Document> doc = parser.parse(&src);
  if (doc->documentElement()->nodeName() != SAMPLEGEOMETRY_TAG) {
    std::ostringstream msg;
    msg << "Can::setSampleShape() - XML definition "
           "expected to be contained within a <"
        << SAMPLEGEOMETRY_TAG << "> tag. Found "
        << doc->documentElement()->nodeName() << "instead.";
    throw std::invalid_argument(msg.str());
  }
  m_sampleShapeXML = sampleShapeXML;
}

/**
 * Set the ID of the shape, if it is a CSG Object
 * @param id ID a string identifying to object
 */
void Container::setID(const std::string &id) {
  // We only do anything if the contained shape is a CSGObject
  if (auto csgObj = boost::dynamic_pointer_cast<CSGObject>(m_shape)) {
    csgObj->setID(id);
  }
}

} // namespace Geometry
} // namespace Mantid
