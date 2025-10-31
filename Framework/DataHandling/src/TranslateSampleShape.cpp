// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright © 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/TranslateSampleShape.h"

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultipleExperimentInfos.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceProperty.h"

#include "MantidDataHandling/CreateSampleShape.h"

#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/V3D.h"

#include <Poco/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>

#include <sstream>

namespace {

// we need some xml parsing and altering functions, which we will just define here

using namespace Poco::XML;

// Decide if an element has attributes that should be shifted
bool isPointLike(const std::string &tag) {
  if (tag == "axis" || tag == "normal-to-plane")
    // ignore vector like tags
    return false;
  if (tag == "centre" || tag == "centre-of-bottom-base" || tag == "tip" || tag == "point-in-plane")
    return true;
  // Cuboids/hexahedra can be defined by corners which end with the six characters: "-point"
  if (tag.size() >= 6 && tag.rfind("-point") == tag.size() - 6)
    return true;
  // all other tags should also be ignored
  return false;
}

void shiftAttribute(Element *e, const std::string &attr, double d) {
  if (!e)
    return;
  if (!e->hasAttribute(attr))
    return;
  const double v = std::stod(e->getAttribute(attr));
  e->setAttribute(attr, std::to_string(v + d));
};

Element *firstElem(Element *parent, const std::string &tag) {
  if (!parent)
    return nullptr;
  Poco::AutoPtr<NodeList> nl = parent->getElementsByTagName(tag);
  if (nl && nl->length() > 0)
    return static_cast<Poco::XML::Element *>(nl->item(0));
  return nullptr;
};

// Shift x/y/z attributes if present
void addXYZ(Element *el, double dx, double dy, double dz) {
  if (el->hasAttribute("x") && el->hasAttribute("y") && el->hasAttribute("z")) {
    shiftAttribute(el, "x", dx);
    shiftAttribute(el, "y", dy);
    shiftAttribute(el, "z", dz);
  }
}

// If user has defined a bounding box, this needs to be shifted accordingly
void shiftBoundingBox(Element *root, double dx, double dy, double dz) {

  // find <bounding-box> anywhere under root
  Element *bb = firstElem(root, "bounding-box");
  if (!bb)
    return;

  shiftAttribute(firstElem(bb, "x-min"), "val", dx);
  shiftAttribute(firstElem(bb, "x-max"), "val", dx);
  shiftAttribute(firstElem(bb, "y-min"), "val", dy);
  shiftAttribute(firstElem(bb, "y-max"), "val", dy);
  shiftAttribute(firstElem(bb, "z-min"), "val", dz);
  shiftAttribute(firstElem(bb, "z-max"), "val", dz);
}

// Go through all the children of the provided root node and write those corresponding to elements (the only ones we are
// interested in)
std::string writeOnlyElementChildNodes(Poco::XML::Element *root) {
  if (!root)
    return std::string();

  std::ostringstream out;
  DOMWriter writer;

  // iterate over the child nodes in linked list style loop
  for (Node *n = root->firstChild(); n; n = n->nextSibling()) {
    if (n->nodeType() == Node::ELEMENT_NODE) {
      writer.writeNode(out, n);
    }
  }
  return out.str();
}

// If XML is user defined and created out the ShapeFactory, it will be wrapped in a <type> tag
// this needs to be removed for the translated xml string to be re-parsed by the Factory
std::string removeTypeTagWrapper(const std::string &xml) {
  DOMParser parser;
  Poco::AutoPtr<Document> doc;
  try {
    doc = parser.parseString(xml);
  } catch (...) {
    return xml; // If parsing fails, just return the original
  }
  Element *root = doc ? doc->documentElement() : nullptr;
  if (!root)
    return xml;

  const std::string tag = root->tagName();
  if (tag == "type") {
    return writeOnlyElementChildNodes(root);
  }

  // No type tag, return original xml
  return xml;
}

std::string translateCSG(const std::string &xml, double dx, double dy, double dz) {
  DOMParser parser;
  Poco::AutoPtr<Document> doc = parser.parseString(xml);

  // Access root element
  Element *root = doc ? doc->documentElement() : nullptr;
  if (!root)
    return xml; // fallback on malformed input

  // Iterate over all nodes, selecting only elements, check if they should be translated, and if so update their
  // definition
  NodeIterator it(doc, NodeFilter::SHOW_ELEMENT);
  for (Node *n = it.nextNode(); n; n = it.nextNode()) {
    auto *el = dynamic_cast<Element *>(n);
    if (!el)
      continue;
    const std::string tag = el->tagName();
    // check if element tag is one that has x,y,z attributes that should be translated
    if (isPointLike(tag)) {
      addXYZ(el, dx, dy, dz);
    }
  }

  // If there is a bounding-box, this should also be shifted
  shiftBoundingBox(root, dx, dy, dz);

  std::ostringstream out;
  DOMWriter writer;
  writer.writeNode(out, doc);
  return out.str();
}

} // anonymous namespace

namespace Mantid::DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(TranslateSampleShape)

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::API;

/** Initialize the algorithm's properties.
 */
void TranslateSampleShape::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("InputWorkspace", "", Direction::InOut),
                  "The workspace containing the sample whose shape is to be translated");

  // Vector to translate shape
  declareProperty(std::make_unique<ArrayProperty<double>>("TranslationVector", "0,0,0"),
                  "Vector by which to translate the loaded sample shape (metres)");
}

/** Execute the algorithm.
 */
void TranslateSampleShape::exec() {
  Workspace_sptr ws = getProperty("InputWorkspace");
  auto ei = std::dynamic_pointer_cast<ExperimentInfo>(ws);
  const std::vector<double> translationVector = getProperty("TranslationVector");

  if (!ei) {
    // We're dealing with an MD workspace which has multiple experiment infos
    auto infos = std::dynamic_pointer_cast<MultipleExperimentInfos>(ws);
    if (!infos) {
      throw std::invalid_argument("Input workspace does not support TranslateSampleShape");
    }
    if (infos->getNumExperimentInfo() < 1) {
      ExperimentInfo_sptr info(new ExperimentInfo());
      infos->addExperimentInfo(info);
    }
    ei = infos->getExperimentInfo(0);
  }

  std::string shapeXML;
  bool isMeshShape = false;
  if (!checkIsValidShape(ei, shapeXML, isMeshShape)) {
    throw std::runtime_error("Input sample does not have a valid shape!");
  }

  const auto dx = translationVector[0];
  const auto dy = translationVector[1];
  const auto dz = translationVector[2];

  if (isMeshShape) {
    // Mesh shapes can be translated directly
    auto meshShape = std::dynamic_pointer_cast<MeshObject>(ei->sample().getShapePtr());
    meshShape->translate(V3D(dx, dy, dz));
  } else {
    // CSG shapes need to translate XML definition, then use this to set a new shape
    auto csgShape = std::dynamic_pointer_cast<CSGObject>(ei->sample().getShapePtr());

    // get shape xml
    const std::string &origXML = csgShape->getShapeXML();

    // translate xml def
    std::string translatedXML = translateCSG(origXML, dx, dy, dz);

    // remove type tag, if there is one
    translatedXML = removeTypeTagWrapper(translatedXML);

    // use new csg xml string to replace the shape
    Mantid::DataHandling::CreateSampleShape creator;
    creator.initialize();
    creator.setChild(true);
    creator.setProperty("InputWorkspace", ws);
    creator.setPropertyValue("ShapeXML", translatedXML);
    creator.execute();
  }
}

bool TranslateSampleShape::checkIsValidShape(const API::ExperimentInfo_sptr &ei, std::string &shapeXML,
                                             bool &isMeshShape) {
  if (ei->sample().hasShape()) {
    const auto csgShape = std::dynamic_pointer_cast<CSGObject>(ei->sample().getShapePtr());
    if (csgShape && csgShape->hasValidShape()) {
      shapeXML = csgShape->getShapeXML();
      if (!shapeXML.empty()) {
        return true;
      }
    } else {
      const auto meshShape = std::dynamic_pointer_cast<MeshObject>(ei->sample().getShapePtr());
      if (meshShape && meshShape->hasValidShape()) {
        isMeshShape = true;
        return true;
      }
    }
  }
  return false;
}

} // namespace Mantid::DataHandling
