#include "MantidGeometry/Instrument/Container.h"
#include "MantidGeometry/Objects/ShapeFactory.h"

#include "Poco/DOM/AutoPtr.h"
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/SAX/InputSource.h"
#include "Poco/SAX/SAXException.h"

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
    Element *element = static_cast<Element *>(node);
    auto argIter = args.find(node->nodeName());
    if (argIter != args.end()) {
      element->setAttribute(VAL_ATT, std::to_string(argIter->second));
    }
    node = nodeIter.nextNode();
  }
}
}

//------------------------------------------------------------------------------
// Public methods
//------------------------------------------------------------------------------
/**
 * Construct a Can providing an XML definition shape
 * @param canXML Definition of the can shape in xml
 */
Container::Container(std::string xml) : Object(xml) {}

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
Object_sptr
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
  return factory.createShape<Object>(root);
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
           "expected to be contained within a <" << SAMPLEGEOMETRY_TAG
        << "> tag. Found " << doc->documentElement()->nodeName() << "instead.";
    throw std::invalid_argument(msg.str());
  }
  m_sampleShapeXML = sampleShapeXML;
}

//------------------------------------------------------------------------------
// Private methods
//------------------------------------------------------------------------------

} // namespace Geometry
} // namespace Mantid
