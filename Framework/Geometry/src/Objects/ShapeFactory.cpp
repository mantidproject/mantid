// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Instrument/Container.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include "MantidGeometry/Surfaces/Cone.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidGeometry/Surfaces/Torus.h"

#include "MantidKernel/Logger.h"
#include "MantidKernel/Quat.h"

#include <Poco/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>

#include "boost/make_shared.hpp"

using Poco::XML::DOMParser;
using Poco::XML::DOMWriter;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Node;
using Poco::XML::NodeList;

namespace Mantid {
namespace Geometry {

using namespace Kernel;

namespace {
const V3D DEFAULT_CENTRE(0, 0, 0);
const V3D DEFAULT_AXIS(0, 0, 1);

/// static logger
Logger g_log("ShapeFactory");
} // namespace

/** Creates a geometric object directly from a XML shape string
 *
 *  @param shapeXML :: XML shape string
 *  @param addTypeTag :: true to wrap a \<type\> tag around the XML supplied
 *(default)
 *  @return A shared pointer to a geometric shape (defaults to an 'empty' shape
 *if XML tags contain no geo. info.)
 */
boost::shared_ptr<CSGObject> ShapeFactory::createShape(std::string shapeXML,
                                                       bool addTypeTag) {
  // wrap in a type tag
  if (addTypeTag)
    shapeXML = "<type name=\"userShape\"> " + shapeXML + " </type>";

  // Set up the DOM parser and parse xml string
  DOMParser pParser;
  Poco::AutoPtr<Document> pDoc;
  try {
    pDoc = pParser.parseString(shapeXML);
  } catch (...) {
    g_log.warning("Unable to parse XML string " + shapeXML +
                  " . Empty geometry Object is returned.");

    return boost::make_shared<CSGObject>();
  }
  // Get pointer to root element
  Element *pRootElem = pDoc->documentElement();
  auto thing = createShape(pRootElem);
  // convert into a Geometry object
  return createShape(pRootElem);
}

/** Creates a geometric object from a DOM-element-node pointing to an element
 * whose child nodes contain the shape information. If no shape information
 * an empty Object is returned.
 *
 * @param pElem A pointer to an Element node whose children fully define the
 * object. The name of this element is unimportant.
 * @return A shared pointer to a geometric shape
 */
boost::shared_ptr<CSGObject>
ShapeFactory::createShape(Poco::XML::Element *pElem) {
  // Write the definition to a string to store in the final object
  std::stringstream xmlstream;
  DOMWriter writer;
  writer.writeNode(xmlstream, pElem);
  std::string shapeXML = xmlstream.str();
  auto retVal = boost::make_shared<CSGObject>(shapeXML);

  // if no <algebra> element then use default algebra
  bool defaultAlgebra(false);
  // get algebra string
  Poco::AutoPtr<NodeList> pNL_algebra = pElem->getElementsByTagName("algebra");
  std::string algebraFromUser;
  if (pNL_algebra->length() == 0) {
    defaultAlgebra = true;
  } else if (pNL_algebra->length() == 1) {
    Element *pElemAlgebra = static_cast<Element *>(pNL_algebra->item(0));
    algebraFromUser = pElemAlgebra->getAttribute("val");
  } else {
    g_log.warning()
        << "More than one algebra string defined for this shape. "
        << "Maximum one allowed. Therefore empty shape is returned.";
    return retVal;
  }

  // match id given to a shape by the user to
  // id understandable by Mantid code
  std::map<std::string, std::string> idMatching;

  // loop over all the sub-elements of pElem
  Poco::AutoPtr<NodeList> pNL = pElem->childNodes(); // get all child nodes
  unsigned long pNL_length = pNL->length();
  int numPrimitives = 0;
  // stores the primitives that will be
  // used to build final shape
  std::map<int, boost::shared_ptr<Surface>> primitives;
  // used to build up unique id's for each shape added. Must start
  // from int > zero.
  int l_id = 1;
  // Element of fixed complete object
  Element *lastElement = nullptr;
  for (unsigned int i = 0; i < pNL_length; i++) {
    if ((pNL->item(i))->nodeType() == Node::ELEMENT_NODE) {
      Element *pE = static_cast<Element *>(pNL->item(i));

      // assume for now that if sub-element has attribute id then it is a shape
      // element
      if (pE->hasAttribute("id")) {
        std::string idFromUser = pE->getAttribute("id"); // get id

        std::string primitiveName = pE->tagName(); // get name of primitive

        // if there are any error thrown while parsing the XML string for a
        // given shape
        // write out a warning to the user that this shape is ignored. If all
        // shapes are ignored
        // this way an empty object is returned to the user.
        try {
          if (primitiveName == "sphere") {
            lastElement = pE;
            idMatching[idFromUser] = parseSphere(pE, primitives, l_id);
            numPrimitives++;
          } else if (primitiveName == "infinite-plane") {
            idMatching[idFromUser] = parseInfinitePlane(pE, primitives, l_id);
            retVal->setFiniteGeometryFlag(false);
            numPrimitives++;
          } else if (primitiveName == "infinite-cylinder") {
            idMatching[idFromUser] =
                parseInfiniteCylinder(pE, primitives, l_id);
            retVal->setFiniteGeometryFlag(false);
            numPrimitives++;
          } else if (primitiveName == "cylinder") {
            lastElement = pE;
            idMatching[idFromUser] = parseCylinder(pE, primitives, l_id);
            numPrimitives++;
          } else if (primitiveName == "hollow-cylinder") {
            lastElement = pE;
            idMatching[idFromUser] = parseHollowCylinder(pE, primitives, l_id);
            numPrimitives++;
          } else if (primitiveName == "cuboid") {
            lastElement = pE;
            idMatching[idFromUser] = parseCuboid(pE, primitives, l_id);
            numPrimitives++;
          } else if (primitiveName == "infinite-cone") {
            idMatching[idFromUser] = parseInfiniteCone(pE, primitives, l_id);
            retVal->setFiniteGeometryFlag(false);
            numPrimitives++;
          } else if (primitiveName == "cone") {
            lastElement = pE;
            idMatching[idFromUser] = parseCone(pE, primitives, l_id);
            numPrimitives++;
          } else if (primitiveName == "hexahedron") {
            lastElement = pE;
            idMatching[idFromUser] = parseHexahedron(pE, primitives, l_id);
            numPrimitives++;
          } else if (primitiveName == "tapered-guide") {
            idMatching[idFromUser] = parseTaperedGuide(pE, primitives, l_id);
            numPrimitives++;
          } else if (primitiveName == "torus") {
            idMatching[idFromUser] = parseTorus(pE, primitives, l_id);
            numPrimitives++;
          } else if (primitiveName == "slice-of-cylinder-ring") {
            idMatching[idFromUser] =
                parseSliceOfCylinderRing(pE, primitives, l_id);
            numPrimitives++;
          } else {
            g_log.warning(
                primitiveName +
                " not a recognised geometric shape. This shape is ignored.");
          }
        } catch (std::invalid_argument &e) {
          g_log.warning() << e.what() << " <" << primitiveName
                          << "> shape is ignored.";
        } catch (std::runtime_error &e) {
          g_log.warning() << e.what() << " <" << primitiveName
                          << "> shape is ignored.";
        } catch (...) {
          g_log.warning() << " Problem with parsing XML string for <"
                          << primitiveName << ">. This shape is ignored.";
        }
      }
    }
  }

  if (!defaultAlgebra) {
    // Translate algebra string defined by the user into something Mantid can
    // understand

    // std::string algebra;  // to hold algebra in a way Mantid can understand
    std::map<std::string, std::string>::iterator iter;
    std::map<size_t, std::string, std::greater<size_t>> allFound;
    for (iter = idMatching.begin(); iter != idMatching.end(); ++iter) {
      size_t found = algebraFromUser.find(iter->first);

      if (found == std::string::npos) {
        defaultAlgebra = true;
        g_log.warning()
            << "Algebra shape Warning: " + iter->first +
                   " not found in algebra string: " + algebraFromUser + "\n" +
                   ". Default to equal shape to intersection of those defined.";
        break;
      } else {
        allFound[found] = iter->first;
      }
    }

    // Here do the actually swapping of strings
    // but only if the algebra containes all the shapes
    if (allFound.size() == idMatching.size()) {
      std::map<size_t, std::string, std::greater<size_t>>::iterator iter2;
      for (iter2 = allFound.begin(); iter2 != allFound.end(); ++iter2) {
        // std::string  kuse = iter2->second;
        algebraFromUser.replace(iter2->first, (iter2->second).size(),
                                idMatching[iter2->second]);
      }
    }
  }
  if (defaultAlgebra) {
    algebraFromUser = ""; // reset in case we are overwriten invalid string
    std::map<std::string, std::string>::iterator iter;
    for (iter = idMatching.begin(); iter != idMatching.end(); ++iter) {
      algebraFromUser.append(iter->second + " "); // default is intersection
    }
  }

  // check to see if there actually were any primitives in 'type' xml element
  // and if yes then return empty Object. Otherwise populate Object with the
  // primitives

  if (numPrimitives == 0)
    return retVal;
  else {
    retVal->setObject(21, algebraFromUser);
    retVal->populate(primitives);
    // check whether there is only one surface/closed surface
    if (numPrimitives == 1 && lastElement != nullptr) // special case
    {
      // parse the primitive and create a Geometry handler for the object
      createGeometryHandler(lastElement, retVal);
    }

    // get bounding box string
    Poco::AutoPtr<NodeList> pNL_boundingBox =
        pElem->getElementsByTagName("bounding-box");
    if (pNL_boundingBox->length() != 1) // should probably throw an error if
                                        // more than 1 bounding box is
                                        // defined...
      return retVal;

    double xmin =
        std::stod((getShapeElement(pElem, "x-min"))->getAttribute("val"));
    double ymin =
        std::stod((getShapeElement(pElem, "y-min"))->getAttribute("val"));
    double zmin =
        std::stod((getShapeElement(pElem, "z-min"))->getAttribute("val"));
    double xmax =
        std::stod((getShapeElement(pElem, "x-max"))->getAttribute("val"));
    double ymax =
        std::stod((getShapeElement(pElem, "y-max"))->getAttribute("val"));
    double zmax =
        std::stod((getShapeElement(pElem, "z-max"))->getAttribute("val"));

    retVal->defineBoundingBox(xmax, ymax, zmax, xmin, ymin, zmin);

    return retVal;
  }
}

/** Parse XML 'sphere' element
 *
 *  @param pElem :: XML 'sphere' element from instrument def. file
 *  @param prim :: To add shapes to
 *  @param l_id :: When shapes added to the map prim l_id is the continuous
 *incremented index
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML
 *instrument file
 */
std::string
ShapeFactory::parseSphere(Poco::XML::Element *pElem,
                          std::map<int, boost::shared_ptr<Surface>> &prim,
                          int &l_id) {
  Element *pElemCentre = getOptionalShapeElement(pElem, "centre");
  Element *pElemRadius = getShapeElement(pElem, "radius");

  // getDoubleAttribute can throw - put the calls above any new
  const double radius = getDoubleAttribute(pElemRadius, "val");

  // create sphere
  const V3D centre = pElemCentre ? parsePosition(pElemCentre) : DEFAULT_CENTRE;
  auto pSphere = boost::make_shared<Sphere>();
  pSphere->setCentre(centre);
  pSphere->setRadius(radius);
  prim[l_id] = pSphere;

  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(-" << l_id << ")";
  l_id++;
  return retAlgebraMatch.str();
}

/** Parse XML 'infinite-plane' element
 *
 *  @param pElem :: XML 'infinite-plane' element from instrument def. file
 *  @param prim :: To add shapes to
 *  @param l_id :: When shapes added to the map prim l_id is the continuous
 *incremented index
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML
 *instrument file
 */
std::string ShapeFactory::parseInfinitePlane(
    Poco::XML::Element *pElem, std::map<int, boost::shared_ptr<Surface>> &prim,
    int &l_id) {
  Element *pElemPip = getShapeElement(pElem, "point-in-plane");
  Element *pElemNormal = getShapeElement(pElem, "normal-to-plane");

  // create infinite-plane
  auto pPlane = boost::make_shared<Plane>();
  pPlane->setPlane(parsePosition(pElemPip), parsePosition(pElemNormal));
  prim[l_id] = pPlane;

  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(" << l_id << ")";
  l_id++;
  return retAlgebraMatch.str();
}

/** Parse XML 'infinite-cylinder' element
 *
 *  @param pElem :: XML 'infinite-cylinder' element from instrument def. file
 *  @param prim :: To add shapes to
 *  @param l_id :: When shapes added to the map prim l_id is the continuous
 *incremented index
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML
 *instrument file
 */
std::string ShapeFactory::parseInfiniteCylinder(
    Poco::XML::Element *pElem, std::map<int, boost::shared_ptr<Surface>> &prim,
    int &l_id) {
  Element *pElemCentre = getShapeElement(pElem, "centre");
  Element *pElemAxis = getShapeElement(pElem, "axis");
  Element *pElemRadius = getShapeElement(pElem, "radius");

  // getDoubleAttribute can throw - put the calls above any new
  const double radius = getDoubleAttribute(pElemRadius, "val");

  // create infinite-cylinder
  auto pCylinder = boost::make_shared<Cylinder>();
  pCylinder->setCentre(parsePosition(pElemCentre));
  pCylinder->setNorm(parsePosition(pElemAxis));

  pCylinder->setRadius(radius);
  prim[l_id] = pCylinder;

  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(-" << l_id << ")";
  l_id++;
  return retAlgebraMatch.str();
}

/** Parse XML 'cylinder' element
 *
 *  @param pElem :: XML 'cylinder' element from instrument def. file
 *  @param prim :: To add shapes to
 *  @param l_id :: When shapes added to the map prim l_id is the continuous
 *incremented index
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML
 *instrument file
 */
std::string
ShapeFactory::parseCylinder(Poco::XML::Element *pElem,
                            std::map<int, boost::shared_ptr<Surface>> &prim,
                            int &l_id) {
  Element *pElemBase = getShapeElement(pElem, "centre-of-bottom-base");
  Element *pElemAxis = getShapeElement(pElem, "axis");
  Element *pElemRadius = getShapeElement(pElem, "radius");
  Element *pElemHeight = getShapeElement(pElem, "height");

  const V3D normVec = normalize(parsePosition(pElemAxis));

  // getDoubleAttribute can throw - put the calls above any new
  const double radius = getDoubleAttribute(pElemRadius, "val");
  const double height = getDoubleAttribute(pElemHeight, "val");

  // add infinite cylinder
  auto pCylinder = boost::make_shared<Cylinder>();
  V3D centreOfBottomBase = parsePosition(pElemBase);
  pCylinder->setCentre(centreOfBottomBase + normVec * (0.5 * height));
  pCylinder->setNorm(normVec);
  pCylinder->setRadius(radius);
  prim[l_id] = pCylinder;

  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(-" << l_id << " ";
  l_id++;

  // add top plane
  auto pPlaneTop = boost::make_shared<Plane>();
  // to get point in top plane
  V3D pointInPlane = centreOfBottomBase + (normVec * height);
  pPlaneTop->setPlane(pointInPlane, normVec);
  prim[l_id] = pPlaneTop;
  retAlgebraMatch << "-" << l_id << " ";
  l_id++;

  // add bottom plane
  auto pPlaneBottom = boost::make_shared<Plane>();
  pPlaneBottom->setPlane(centreOfBottomBase, normVec);
  prim[l_id] = pPlaneBottom;
  retAlgebraMatch << "" << l_id << ")";
  l_id++;

  return retAlgebraMatch.str();
}

/** Parse XML 'cylinder' element
 *
 *  @param pElem :: XML 'cylinder' element from instrument def. file
 *  @param prim :: To add shapes to
 *  @param l_id :: When shapes added to the map prim l_id is the continuous
 *incremented index
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML
 *instrument file
 */
std::string ShapeFactory::parseSegmentedCylinder(
    Poco::XML::Element *pElem, std::map<int, boost::shared_ptr<Surface>> &prim,
    int &l_id) {
  Element *pElemBase = getShapeElement(pElem, "centre-of-bottom-base");
  Element *pElemAxis = getShapeElement(pElem, "axis");
  Element *pElemRadius = getShapeElement(pElem, "radius");
  Element *pElemHeight = getShapeElement(pElem, "height");

  const V3D normVec = normalize(parsePosition(pElemAxis));

  // getDoubleAttribute can throw - put the calls above any new
  const double radius = getDoubleAttribute(pElemRadius, "val");
  const double height = getDoubleAttribute(pElemHeight, "val");

  // add infinite cylinder
  auto pCylinder = boost::make_shared<Cylinder>();
  V3D centreOfBottomBase = parsePosition(pElemBase);
  pCylinder->setCentre(centreOfBottomBase + normVec * (0.5 * height));
  pCylinder->setNorm(normVec);
  pCylinder->setRadius(radius);
  prim[l_id] = pCylinder;

  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(-" << l_id << " ";
  l_id++;

  // add top plane
  auto pPlaneTop = boost::make_shared<Plane>();
  // to get point in top plane
  V3D pointInPlane = centreOfBottomBase + (normVec * height);
  pPlaneTop->setPlane(pointInPlane, normVec);
  prim[l_id] = pPlaneTop;
  retAlgebraMatch << "-" << l_id << " ";
  l_id++;

  // add bottom plane
  auto pPlaneBottom = boost::make_shared<Plane>();
  pPlaneBottom->setPlane(centreOfBottomBase, normVec);
  prim[l_id] = pPlaneBottom;
  retAlgebraMatch << "" << l_id << ")";
  l_id++;

  return retAlgebraMatch.str();
}

/** Parse XML 'hollow-cylinder' element
 *
 *  @param pElem :: XML 'hollow-cylinder' element from instrument def. file
 *  @param prim :: To add shapes to
 *  @param l_id :: When shapes added to the map prim l_id is the continuous
 *incremented index
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML
 *instrument file
 */

std::string ShapeFactory::parseHollowCylinder(
    Poco::XML::Element *pElem, std::map<int, boost::shared_ptr<Surface>> &prim,
    int &l_id) {
  Element *pElemBase = getShapeElement(pElem, "centre-of-bottom-base");
  Element *pElemAxis = getShapeElement(pElem, "axis");
  Element *pElemInnerRadius = getShapeElement(pElem, "inner-radius");
  Element *pElemOuterRadius = getShapeElement(pElem, "outer-radius");
  Element *pElemHeight = getShapeElement(pElem, "height");

  const V3D normVec = normalize(parsePosition(pElemAxis));
  const double innerRadius = getDoubleAttribute(pElemInnerRadius, "val");
  if (innerRadius <= 0.0) {
    throw std::runtime_error(
        "ShapeFactory::parseHollowCylinder(): inner-radius < 0.0");
  }
  const double outerRadius = getDoubleAttribute(pElemOuterRadius, "val");
  if (outerRadius <= 0.0) {
    throw std::runtime_error(
        "ShapeFactory::parseHollowCylinder(): outer-radius < 0.0");
  }
  if (innerRadius > outerRadius) {
    throw std::runtime_error(
        "ShapeFactory::parseHollowCylinder(): inner-radius > outer-radius.");
  }
  const double height = getDoubleAttribute(pElemHeight, "val");
  if (height <= 0.0) {
    throw std::runtime_error(
        "ShapeFactory::parseHollowCylinder(): height < 0.0");
  }
  V3D centreOfBottomBase = parsePosition(pElemBase);

  // add outer infinite cylinder surface
  auto outerCylinder = boost::make_shared<Cylinder>();
  outerCylinder->setCentre(centreOfBottomBase + normVec * (0.5 * height));
  outerCylinder->setNorm(normVec);
  outerCylinder->setRadius(outerRadius);
  prim[l_id] = outerCylinder;

  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(-" << l_id << " ";
  l_id++;

  // add inner infinite cylinder surface
  auto innerCylinder = boost::make_shared<Cylinder>();
  innerCylinder->setCentre(centreOfBottomBase + normVec * (0.5 * height));
  innerCylinder->setNorm(normVec);
  innerCylinder->setRadius(innerRadius);
  prim[l_id] = innerCylinder;
  retAlgebraMatch << l_id << " ";
  l_id++;

  // add top plane
  auto pPlaneTop = boost::make_shared<Plane>();
  // to get point in top plane
  V3D pointInPlane = centreOfBottomBase + (normVec * height);
  pPlaneTop->setPlane(pointInPlane, normVec);
  prim[l_id] = pPlaneTop;
  retAlgebraMatch << "-" << l_id << " ";
  l_id++;

  // add bottom plane
  auto pPlaneBottom = boost::make_shared<Plane>();
  pPlaneBottom->setPlane(centreOfBottomBase, normVec);
  prim[l_id] = pPlaneBottom;
  retAlgebraMatch << "" << l_id << ")";
  l_id++;

  return retAlgebraMatch.str();
}

/**
 * Get the four corners of a cuboid from an XML element.  The XML may consist
 * of one of the two available syntaxes.  We disallow a mixture of syntaxes.
 *
 * @param pElem :: XML 'cuboid' element from instrument definition file.
 * @return The four corners of the cuboid.
 *
 * @throw std::invalid_argument if XML string is invalid.
 */
CuboidCorners ShapeFactory::parseCuboid(Poco::XML::Element *pElem) {
  // Users have two syntax options when defining cuboids:

  // A - "Point" syntax.
  Element *pElem_lfb =
      getOptionalShapeElement(pElem, "left-front-bottom-point");
  Element *pElem_lft = getOptionalShapeElement(pElem, "left-front-top-point");
  Element *pElem_lbb = getOptionalShapeElement(pElem, "left-back-bottom-point");
  Element *pElem_rfb =
      getOptionalShapeElement(pElem, "right-front-bottom-point");

  // B - "Alternate" syntax.
  Element *pElem_height = getOptionalShapeElement(pElem, "height");
  Element *pElem_width = getOptionalShapeElement(pElem, "width");
  Element *pElem_depth = getOptionalShapeElement(pElem, "depth");
  Element *pElem_centre = getOptionalShapeElement(pElem, "centre");
  Element *pElem_axis = getOptionalShapeElement(pElem, "axis");

  const bool usingPointSyntax =
      pElem_lfb && pElem_lft && pElem_lbb && pElem_rfb;
  const bool usingAlternateSyntax = pElem_height && pElem_width && pElem_depth;

  const bool usedPointSyntaxField =
      pElem_lfb || pElem_lft || pElem_lbb || pElem_rfb;
  const bool usedAlternateSyntaxField =
      pElem_height || pElem_width || pElem_depth || pElem_centre || pElem_axis;

  const std::string SYNTAX_ERROR_MSG =
      "XML element: <" + pElem->tagName() +
      "> may contain EITHER corner points (LFB, LFT, LBB and RFB) OR " +
      "height, width, depth, centre and axis values.";

  CuboidCorners result;

  if (usingPointSyntax && !usingAlternateSyntax) {
    if (usedAlternateSyntaxField)
      throw std::invalid_argument(SYNTAX_ERROR_MSG);

    result.lfb = parsePosition(pElem_lfb);
    result.lft = parsePosition(pElem_lft);
    result.lbb = parsePosition(pElem_lbb);
    result.rfb = parsePosition(pElem_rfb);
  } else if (usingAlternateSyntax && !usingPointSyntax) {
    if (usedPointSyntaxField)
      throw std::invalid_argument(SYNTAX_ERROR_MSG);

    const double deltaH = getDoubleAttribute(pElem_height, "val") / 2;
    const double deltaW = getDoubleAttribute(pElem_width, "val") / 2;
    const double deltaD = getDoubleAttribute(pElem_depth, "val") / 2;

    const V3D centre =
        pElem_centre ? parsePosition(pElem_centre) : DEFAULT_CENTRE;

    result.lfb = V3D(-deltaW, -deltaH, -deltaD);
    result.lft = V3D(-deltaW, deltaH, -deltaD);
    result.lbb = V3D(-deltaW, -deltaH, deltaD);
    result.rfb = V3D(deltaW, -deltaH, -deltaD);

    if (pElem_axis) {
      // Use a quarternion to do a rotation for us, with respect to the default
      // axis.  Our "Quat" implementation requires that the vectors passed to
      // it be normalised.
      const V3D axis = normalize(parsePosition(pElem_axis));
      const Quat rotation(DEFAULT_AXIS, axis);

      rotation.rotate(result.lfb);
      rotation.rotate(result.lft);
      rotation.rotate(result.lbb);
      rotation.rotate(result.rfb);
    }

    result.lfb += centre;
    result.lft += centre;
    result.lbb += centre;
    result.rfb += centre;
  } else
    throw std::invalid_argument(SYNTAX_ERROR_MSG);

  return result;
}

/** Parse XML 'cuboid' element
 *
 *  @param pElem :: XML 'cuboid' element from instrument def. file
 *  @param prim :: To add shapes to
 *  @param l_id :: When shapes added to the map prim l_id is the continuous
 *incremented index
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML
 *instrument file
 */
std::string
ShapeFactory::parseCuboid(Poco::XML::Element *pElem,
                          std::map<int, boost::shared_ptr<Surface>> &prim,
                          int &l_id) {
  auto corners = parseCuboid(pElem);

  const V3D pointTowardBack = normalize(corners.lbb - corners.lfb);

  // add front plane cutoff
  auto pPlaneFrontCutoff = boost::make_shared<Plane>();
  try {
    pPlaneFrontCutoff->setPlane(corners.lfb, pointTowardBack);
  } catch (std::invalid_argument &) {
    throw;
  }
  prim[l_id] = pPlaneFrontCutoff;

  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(" << l_id << " ";
  l_id++;

  // add back plane cutoff
  auto pPlaneBackCutoff = boost::make_shared<Plane>();
  try {
    pPlaneBackCutoff->setPlane(corners.lbb, pointTowardBack);
  } catch (std::invalid_argument &) {
    throw;
  }
  prim[l_id] = pPlaneBackCutoff;
  retAlgebraMatch << "-" << l_id << " ";
  l_id++;

  const V3D pointTowardRight = normalize(corners.rfb - corners.lfb);

  // add left plane cutoff
  auto pPlaneLeftCutoff = boost::make_shared<Plane>();
  try {
    pPlaneLeftCutoff->setPlane(corners.lfb, pointTowardRight);
  } catch (std::invalid_argument &) {
    throw;
  }
  prim[l_id] = pPlaneLeftCutoff;
  retAlgebraMatch << "" << l_id << " ";
  l_id++;

  // add right plane cutoff
  auto pPlaneRightCutoff = boost::make_shared<Plane>();
  try {
    pPlaneRightCutoff->setPlane(corners.rfb, pointTowardRight);
  } catch (std::invalid_argument &) {
    throw;
  }
  prim[l_id] = pPlaneRightCutoff;
  retAlgebraMatch << "-" << l_id << " ";
  l_id++;

  const V3D pointTowardTop = normalize(corners.lft - corners.lfb);

  // add bottom plane cutoff
  auto pPlaneBottomCutoff = boost::make_shared<Plane>();
  try {
    pPlaneBottomCutoff->setPlane(corners.lfb, pointTowardTop);
  } catch (std::invalid_argument &) {
    throw;
  }
  prim[l_id] = pPlaneBottomCutoff;
  retAlgebraMatch << "" << l_id << " ";
  l_id++;

  // add top plane cutoff
  auto pPlaneTopCutoff = boost::make_shared<Plane>();
  try {
    pPlaneTopCutoff->setPlane(corners.lft, pointTowardTop);
  } catch (std::invalid_argument &) {
    throw;
  }
  prim[l_id] = pPlaneTopCutoff;
  retAlgebraMatch << "-" << l_id << ")";
  l_id++;

  return retAlgebraMatch.str();
}

/** Parse XML 'infinite-cone' element
 *
 *  @param pElem :: XML 'infinite-cone' element from instrument def. file
 *  @param prim :: To add shapes to
 *  @param l_id :: When shapes added to the map prim l_id is the continuous
 *incremented index
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML
 *instrument file
 */
std::string
ShapeFactory::parseInfiniteCone(Poco::XML::Element *pElem,
                                std::map<int, boost::shared_ptr<Surface>> &prim,
                                int &l_id) {
  Element *pElemTipPoint = getShapeElement(pElem, "tip-point");
  Element *pElemAxis = getShapeElement(pElem, "axis");
  Element *pElemAngle = getShapeElement(pElem, "angle");

  const V3D normVec = normalize(parsePosition(pElemAxis));

  // getDoubleAttribute can throw - put the calls above any new
  const double angle = getDoubleAttribute(pElemAngle, "val");

  // add infinite double cone
  auto pCone = boost::make_shared<Cone>();
  pCone->setCentre(parsePosition(pElemTipPoint));
  pCone->setNorm(normVec);
  pCone->setAngle(angle);
  prim[l_id] = pCone;

  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(" << l_id << " ";
  l_id++;

  // plane top cut of top part of double cone
  auto pPlaneBottom = boost::make_shared<Plane>();
  pPlaneBottom->setPlane(parsePosition(pElemTipPoint), normVec);
  prim[l_id] = pPlaneBottom;
  retAlgebraMatch << "-" << l_id << ")";
  l_id++;

  return retAlgebraMatch.str();
}

/** Parse XML 'cone' element
 *
 *  @param pElem :: XML 'cone' element from instrument def. file
 *  @param prim :: To add shapes to
 *  @param l_id :: When shapes added to the map prim l_id is the continuous
 *incremented index
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML
 *instrument file
 */
std::string
ShapeFactory::parseCone(Poco::XML::Element *pElem,
                        std::map<int, boost::shared_ptr<Surface>> &prim,
                        int &l_id) {
  Element *pElemTipPoint = getShapeElement(pElem, "tip-point");
  Element *pElemAxis = getShapeElement(pElem, "axis");
  Element *pElemAngle = getShapeElement(pElem, "angle");
  Element *pElemHeight = getShapeElement(pElem, "height");

  const V3D normVec = normalize(parsePosition(pElemAxis));

  // getDoubleAttribute can throw - put the calls above any new
  const double angle = getDoubleAttribute(pElemAngle, "val");
  const double height = getDoubleAttribute(pElemHeight, "val");

  // add infinite double cone
  auto pCone = boost::make_shared<Cone>();
  pCone->setCentre(parsePosition(pElemTipPoint));
  pCone->setNorm(normVec);
  pCone->setAngle(angle);
  prim[l_id] = pCone;

  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(" << l_id << " ";
  l_id++;

  // Plane to cut off cone from below
  auto pPlaneTop = boost::make_shared<Plane>();
  V3D pointInPlane = parsePosition(pElemTipPoint);
  pointInPlane -= (normVec * height);
  pPlaneTop->setPlane(pointInPlane, normVec);
  prim[l_id] = pPlaneTop;
  retAlgebraMatch << "" << l_id << " ";
  l_id++;

  // plane top cut of top part of double cone
  auto pPlaneBottom = boost::make_shared<Plane>();
  pPlaneBottom->setPlane(parsePosition(pElemTipPoint), normVec);
  prim[l_id] = pPlaneBottom;
  retAlgebraMatch << "-" << l_id << ")";
  l_id++;

  return retAlgebraMatch.str();
}

/**
 * The "tapered-guide" shape is actually a special case of hexahedron; once we
 * have
 * the 8 points that make up either shape, the process of parsing them can be
 * exactly the same in both cases.
 */
std::string ShapeFactory::parseHexahedronFromStruct(
    Hexahedron &hex, std::map<int, boost::shared_ptr<Surface>> &prim,
    int &l_id) {
  // add front face
  auto pPlaneFrontCutoff = boost::make_shared<Plane>();
  auto normal = (hex.rfb - hex.lfb).cross_prod(hex.lft - hex.lfb);

  // V3D jjj = (normal*(rfb-rbb));
  if (normal.scalar_prod(hex.rfb - hex.rbb) < 0)
    normal *= -1.0;
  pPlaneFrontCutoff->setPlane(hex.lfb, normal);
  prim[l_id] = pPlaneFrontCutoff;
  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(-" << l_id << " ";
  l_id++;

  // add back face
  auto pPlaneBackCutoff = boost::make_shared<Plane>();
  normal = (hex.rbb - hex.lbb).cross_prod(hex.lbt - hex.lbb);
  if (normal.scalar_prod(hex.rfb - hex.rbb) < 0)
    normal *= -1.0;
  pPlaneBackCutoff->setPlane(hex.lbb, normal);
  prim[l_id] = pPlaneBackCutoff;
  retAlgebraMatch << "" << l_id << " ";
  l_id++;

  // add left face
  auto pPlaneLeftCutoff = boost::make_shared<Plane>();
  normal = (hex.lbb - hex.lfb).cross_prod(hex.lft - hex.lfb);
  if (normal.scalar_prod(hex.rfb - hex.lfb) < 0)
    normal *= -1.0;
  pPlaneLeftCutoff->setPlane(hex.lfb, normal);
  prim[l_id] = pPlaneLeftCutoff;
  retAlgebraMatch << "" << l_id << " ";
  l_id++;

  // add right face
  auto pPlaneRightCutoff = boost::make_shared<Plane>();
  normal = (hex.rbb - hex.rfb).cross_prod(hex.rft - hex.rfb);
  if (normal.scalar_prod(hex.rfb - hex.lfb) < 0)
    normal *= -1.0;
  pPlaneRightCutoff->setPlane(hex.rfb, normal);
  prim[l_id] = pPlaneRightCutoff;
  retAlgebraMatch << "-" << l_id << " ";
  l_id++;

  // add top face
  auto pPlaneTopCutoff = boost::make_shared<Plane>();
  normal = (hex.rft - hex.lft).cross_prod(hex.lbt - hex.lft);
  if (normal.scalar_prod(hex.rft - hex.rfb) < 0)
    normal *= -1.0;
  pPlaneTopCutoff->setPlane(hex.lft, normal);
  prim[l_id] = pPlaneTopCutoff;
  retAlgebraMatch << "-" << l_id << " ";
  l_id++;

  // add bottom face
  auto pPlaneBottomCutoff = boost::make_shared<Plane>();
  normal = (hex.rfb - hex.lfb).cross_prod(hex.lbb - hex.lfb);
  if (normal.scalar_prod(hex.rft - hex.rfb) < 0)
    normal *= -1.0;
  pPlaneBottomCutoff->setPlane(hex.lfb, normal);
  prim[l_id] = pPlaneBottomCutoff;
  retAlgebraMatch << "" << l_id << ")";
  l_id++;

  return retAlgebraMatch.str();
}

/**
 * Get all corners of a hexahedron from an XML element.
 *
 * @param pElem :: XML 'hexahedron' element from instrument definition file.
 * @return All corners of the hexahedron.
 *
 * @throw std::invalid_argument if XML string is invalid.
 */
Hexahedron ShapeFactory::parseHexahedron(Poco::XML::Element *pElem) {
  Element *pElem_lfb = getShapeElement(pElem, "left-front-bottom-point");
  Element *pElem_lft = getShapeElement(pElem, "left-front-top-point");
  Element *pElem_lbb = getShapeElement(pElem, "left-back-bottom-point");
  Element *pElem_lbt = getShapeElement(pElem, "left-back-top-point");
  Element *pElem_rfb = getShapeElement(pElem, "right-front-bottom-point");
  Element *pElem_rft = getShapeElement(pElem, "right-front-top-point");
  Element *pElem_rbb = getShapeElement(pElem, "right-back-bottom-point");
  Element *pElem_rbt = getShapeElement(pElem, "right-back-top-point");

  const bool isValid = pElem_lfb && pElem_lft && pElem_lbb && pElem_lbt &&
                       pElem_rfb && pElem_rft && pElem_rbb && pElem_rbt;

  std::ostringstream ERROR_MSG;
  ERROR_MSG << "XML element: <" + pElem->tagName() + ""
            << "> contains invalid syntax for defining hexahedron. The "
               "following points have not been defined:\n\n";

  if (!pElem_lfb)
    ERROR_MSG << "left-front-bottom-point\n";
  if (!pElem_lft)
    ERROR_MSG << "left-front-top-point\n";
  if (!pElem_lbb)
    ERROR_MSG << "left-back-bottom-point\n";
  if (!pElem_lbt)
    ERROR_MSG << "left-back-top-point\n";
  if (!pElem_rfb)
    ERROR_MSG << "right-front-bottom-point\n";
  if (!pElem_rft)
    ERROR_MSG << "right-front-top-point\n";
  if (!pElem_rbb)
    ERROR_MSG << "right-back-bottom-point\n";
  if (!pElem_rbt)
    ERROR_MSG << "right-back-top-point\n";

  if (!isValid)
    throw std::invalid_argument(ERROR_MSG.str());

  Hexahedron hex;
  hex.lfb = parsePosition(pElem_lfb);
  hex.lft = parsePosition(pElem_lft);
  hex.lbb = parsePosition(pElem_lbb);
  hex.lbt = parsePosition(pElem_lbt);
  hex.rfb = parsePosition(pElem_rfb);
  hex.rft = parsePosition(pElem_rft);
  hex.rbb = parsePosition(pElem_rbb);
  hex.rbt = parsePosition(pElem_rbt);

  return hex;
}

/** Parse XML 'hexahedron' element
 *
 *  @param pElem :: XML 'hexahedron' element from instrument def. file
 *  @param prim :: To add shapes to
 *  @param l_id :: When shapes added to the map prim l_id is the continuous
 *incremented index
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML
 *instrument file
 */
std::string
ShapeFactory::parseHexahedron(Poco::XML::Element *pElem,
                              std::map<int, boost::shared_ptr<Surface>> &prim,
                              int &l_id) {
  Hexahedron hex = parseHexahedron(pElem);

  return parseHexahedronFromStruct(hex, prim, l_id);
}

/** Parse XML 'tapered-guide' element, which is a special case of the XML
 *'hexahedron' element.
 *
 *  @param pElem :: XML 'hexahedron' element from instrument def. file
 *  @param prim :: To add shapes to
 *  @param l_id :: When shapes added to the map prim l_id is the continuous
 *incremented index
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML
 *instrument file
 */
std::string
ShapeFactory::parseTaperedGuide(Poco::XML::Element *pElem,
                                std::map<int, boost::shared_ptr<Surface>> &prim,
                                int &l_id) {
  Element *pElemApertureStart = getShapeElement(pElem, "aperture-start");
  Element *pElemLength = getShapeElement(pElem, "length");
  Element *pElemApertureEnd = getShapeElement(pElem, "aperture-end");
  Element *pElemCentre = getOptionalShapeElement(pElem, "centre");
  Element *pElemAxis = getOptionalShapeElement(pElem, "axis");

  // For centre and axis we allow defaults.
  const V3D centre = pElemCentre ? parsePosition(pElemCentre) : DEFAULT_CENTRE;
  // Quat requires normalised axes.
  const V3D axis =
      normalize(pElemAxis ? parsePosition(pElemAxis) : DEFAULT_AXIS);

  const double apertureStartWidth =
      getDoubleAttribute(pElemApertureStart, "width");
  const double apertureStartHeight =
      getDoubleAttribute(pElemApertureStart, "height");
  const double length = getDoubleAttribute(pElemLength, "val");
  const double apertureEndWidth = getDoubleAttribute(pElemApertureEnd, "width");
  const double apertureEndHeight =
      getDoubleAttribute(pElemApertureEnd, "height");

  const double halfSW = apertureStartWidth / 2;
  const double halfSH = apertureStartHeight / 2;
  const double halfEW = apertureEndWidth / 2;
  const double halfEH = apertureEndHeight / 2;

  // Build the basic shape.
  Hexahedron hex;
  hex.lfb = V3D(-halfSW, -halfSH, 0);
  hex.lft = V3D(-halfSW, halfSH, 0);
  hex.lbb = V3D(-halfEW, -halfEH, length);
  hex.lbt = V3D(-halfEW, halfEH, length);
  hex.rfb = V3D(halfSW, -halfSH, 0);
  hex.rft = V3D(halfSW, halfSH, 0);
  hex.rbb = V3D(halfEW, -halfEH, length);
  hex.rbt = V3D(halfEW, halfEH, length);

  // Point it along the defined axis.
  if (axis != DEFAULT_AXIS) {
    const Quat q(DEFAULT_AXIS, axis);

    q.rotate(hex.lfb);
    q.rotate(hex.lft);
    q.rotate(hex.lbb);
    q.rotate(hex.lbt);
    q.rotate(hex.rfb);
    q.rotate(hex.rft);
    q.rotate(hex.rbb);
    q.rotate(hex.rbt);
  }

  // Move it to the defined centre.
  hex.lfb += centre;
  hex.lft += centre;
  hex.lbb += centre;
  hex.lbt += centre;
  hex.rfb += centre;
  hex.rft += centre;
  hex.rbb += centre;
  hex.rbt += centre;

  return parseHexahedronFromStruct(hex, prim, l_id);
}

/** Parse XML 'torus' element
 *
 *  @param pElem :: XML 'torus' element from instrument def. file
 *  @param prim :: To add shapes to
 *  @param l_id :: When shapes added to the map prim l_id is the continuous
 *incremented index
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML
 *instrument file
 */
std::string
ShapeFactory::parseTorus(Poco::XML::Element *pElem,
                         std::map<int, boost::shared_ptr<Surface>> &prim,
                         int &l_id) {
  Element *pElemCentre = getShapeElement(pElem, "centre");
  Element *pElemAxis = getShapeElement(pElem, "axis");
  Element *pElemRadiusFromCentre =
      getShapeElement(pElem, "radius-from-centre-to-tube");
  Element *pElemRadiusTube = getShapeElement(pElem, "radius-tube");

  const V3D normVec = normalize(parsePosition(pElemAxis));

  // getDoubleAttribute can throw - put the calls above any new
  const double radiusCentre = getDoubleAttribute(pElemRadiusFromCentre, "val");
  const double radiusTube = getDoubleAttribute(pElemRadiusTube, "val");

  // add torus
  auto pTorus = boost::make_shared<Torus>();
  pTorus->setCentre(parsePosition(pElemCentre));
  pTorus->setNorm(normVec);
  pTorus->setDistanceFromCentreToTube(radiusCentre);
  pTorus->setTubeRadius(radiusTube);
  prim[l_id] = pTorus;

  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(-" << l_id << ")";
  l_id++;

  return retAlgebraMatch.str();
}

/** Parse XML 'slice-of-cylinder-ring' element
 *
 *  @param pElem :: XML 'slice-of-cylinder-ring' element from instrument def.
 *file
 *  @param prim :: To add shapes to
 *  @param l_id :: When shapes added to the map prim l_id is the continuous
 *incremented index
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML
 *instrument file
 */
std::string ShapeFactory::parseSliceOfCylinderRing(
    Poco::XML::Element *pElem, std::map<int, boost::shared_ptr<Surface>> &prim,
    int &l_id) {
  Element *pElemArc = getShapeElement(pElem, "arc");
  Element *pElemInnerRadius = getShapeElement(pElem, "inner-radius");
  Element *pElemOuterRadius = getShapeElement(pElem, "outer-radius");
  Element *pElemDepth = getShapeElement(pElem, "depth");

  const double innerRadius = getDoubleAttribute(pElemInnerRadius, "val");
  const double outerRadius = getDoubleAttribute(pElemOuterRadius, "val");
  const double middleRadius = (outerRadius + innerRadius) / 2.0;

  const double depth = getDoubleAttribute(pElemDepth, "val");
  const double arc = (M_PI / 180.0) * getDoubleAttribute(pElemArc, "val");

  V3D normVec(0, 0, 1);
  V3D centrePoint(-middleRadius, 0, 0);

  // add inner infinite cylinder
  auto pCylinder1 = boost::make_shared<Cylinder>();
  pCylinder1->setCentre(centrePoint);
  pCylinder1->setNorm(normVec);
  pCylinder1->setRadius(innerRadius);
  prim[l_id] = pCylinder1;
  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(" << l_id << " ";
  l_id++;

  // add outer infinite cylinder
  auto pCylinder2 = boost::make_shared<Cylinder>();
  pCylinder2->setCentre(centrePoint);
  pCylinder2->setNorm(normVec);
  pCylinder2->setRadius(outerRadius);
  prim[l_id] = pCylinder2;
  retAlgebraMatch << "-" << l_id << " ";
  l_id++;

  // add top cutoff plane of infinite cylinder ring
  auto pPlaneTop = boost::make_shared<Plane>();
  pPlaneTop->setPlane(V3D(0, 0, depth), normVec);
  prim[l_id] = pPlaneTop;
  retAlgebraMatch << "-" << l_id << " ";
  l_id++;

  // add bottom cutoff plane (which is assumed to fase the sample)
  // which at this point will result in a cylinder ring
  auto pPlaneBottom = boost::make_shared<Plane>();
  pPlaneBottom->setPlane(V3D(0, 0, 0), normVec);
  prim[l_id] = pPlaneBottom;
  retAlgebraMatch << "" << l_id << " ";
  l_id++;

  // the two planes that are going to cut a slice of the cylinder ring

  auto pPlaneSlice1 = boost::make_shared<Plane>();
  pPlaneSlice1->setPlane(
      V3D(-middleRadius, 0, 0),
      V3D(cos(arc / 2.0 + M_PI / 2.0), sin(arc / 2.0 + M_PI / 2.0), 0));
  prim[l_id] = pPlaneSlice1;
  retAlgebraMatch << "-" << l_id << " ";
  l_id++;

  auto pPlaneSlice2 = boost::make_shared<Plane>();
  pPlaneSlice2->setPlane(
      V3D(-middleRadius, 0, 0),
      V3D(cos(-arc / 2.0 + M_PI / 2.0), sin(-arc / 2.0 + M_PI / 2.0), 0));
  prim[l_id] = pPlaneSlice2;
  retAlgebraMatch << "" << l_id << ")";
  l_id++;

  return retAlgebraMatch.str();
}

/** Return a subelement of an XML element, but also checks that there exist
 *exactly one entry
 *  of this subelement.
 *
 *  @param pElem :: XML from instrument def. file
 *  @param name :: Name of subelement
 *  @return The subelement
 *
 *  @throw std::invalid_argument Thrown if issues with XML string
 */
Poco::XML::Element *ShapeFactory::getShapeElement(Poco::XML::Element *pElem,
                                                  const std::string &name) {
  // check if this shape element contain an element with name specified by the
  // 2nd function argument
  Poco::AutoPtr<NodeList> pNL = pElem->getElementsByTagName(name);
  if (pNL->length() != 1) {
    throw std::invalid_argument(
        "XML element: <" + pElem->tagName() +
        "> must contain exactly one sub-element with name: <" + name + ">.");
  }
  Element *retVal = static_cast<Element *>(pNL->item(0));
  return retVal;
}

/**
 * Return a subelement of an XML element.  The subelement is optional so it may
 *not exist, but
 * we also check that there is never more than one.
 *
 * @param pElem :: XML from instrument definition file.
 * @param name :: Name of the subelement.
 * @return The subelement, or a null pointer if it does not exist.
 *
 * @throw std::invalid_argument if XML string is invalid.
 */
Poco::XML::Element *
ShapeFactory::getOptionalShapeElement(Poco::XML::Element *pElem,
                                      const std::string &name) {
  // Allow zero or one occurances of subelements with the given name.
  Poco::AutoPtr<NodeList> pNL = pElem->getElementsByTagName(name);
  if (pNL->length() == 0)
    return nullptr;
  else if (pNL->length() > 1)
    throw std::invalid_argument(
        "XML element: <" + pElem->tagName() +
        "> may contain at most one sub-element with name: <" + name + ">.");

  Element *retVal = static_cast<Element *>(pNL->item(0));
  return retVal;
}

/** Return value of attribute to XML element. It is an extension of poco's
 *getAttribute method, which
 *  in addition check that this attribute exists and if not throws an error.
 *
 *  @param pElem :: XML from instrument def. file
 *  @param name :: Name of subelement
 *  @return Value of attribute
 *
 *  @throw std::invalid_argument Thrown if issues with XML string
 */
double ShapeFactory::getDoubleAttribute(Poco::XML::Element *pElem,
                                        const std::string &name) {
  if (pElem->hasAttribute(name)) {
    return std::stod(pElem->getAttribute(name));
  } else {
    throw std::invalid_argument("XML element: <" + pElem->tagName() +
                                "> does not have the attribute: " + name + ".");
  }
}

/** Get position coordinates from XML element
 *
 *  @param pElem :: XML element whose attributes contain position coordinates
 *  @return Position coordinates in the form of a V3D object
 */
V3D ShapeFactory::parsePosition(Poco::XML::Element *pElem) {
  V3D retVal;

  if (pElem->hasAttribute("R") || pElem->hasAttribute("theta") ||
      pElem->hasAttribute("phi")) {
    double R = 0.0, theta = 0.0, phi = 0.0;

    if (pElem->hasAttribute("R"))
      R = std::stod(pElem->getAttribute("R"));
    if (pElem->hasAttribute("theta"))
      theta = std::stod(pElem->getAttribute("theta"));
    if (pElem->hasAttribute("phi"))
      phi = std::stod(pElem->getAttribute("phi"));

    retVal.spherical(R, theta, phi);
  } else if (pElem->hasAttribute("r") || pElem->hasAttribute("t") ||
             pElem->hasAttribute("p"))
  // This is alternative way a user may specify sphecical coordinates
  // which may be preferred in the long run to the more verbose of
  // using R, theta and phi.
  {
    double R = 0.0, theta = 0.0, phi = 0.0;

    if (pElem->hasAttribute("r"))
      R = std::stod(pElem->getAttribute("r"));
    if (pElem->hasAttribute("t"))
      theta = std::stod(pElem->getAttribute("t"));
    if (pElem->hasAttribute("p"))
      phi = std::stod(pElem->getAttribute("p"));

    retVal.spherical(R, theta, phi);
  } else {
    double x = 0.0, y = 0.0, z = 0.0;

    if (pElem->hasAttribute("x"))
      x = std::stod(pElem->getAttribute("x"));
    if (pElem->hasAttribute("y"))
      y = std::stod(pElem->getAttribute("y"));
    if (pElem->hasAttribute("z"))
      z = std::stod(pElem->getAttribute("z"));

    retVal(x, y, z);
  }

  return retVal;
}

/** Create a hexahedral shape object
@param xlb :: Left-back x point or hexahedron
@param xlf :: Left-front x point of hexahedron
@param xrf :: Right-front x point of hexahedron
@param xrb :: Right-back x point of hexahedron
@param ylb :: Left-back y point or hexahedron
@param ylf :: Left-front y point of hexahedron
@param yrf :: Right-front y point of hexahedron
@param yrb :: Right-back y point of hexahedron

@returns the newly created hexahedral shape object
*/
boost::shared_ptr<CSGObject>
ShapeFactory::createHexahedralShape(double xlb, double xlf, double xrf,
                                    double xrb, double ylb, double ylf,
                                    double yrf, double yrb) {
  Hexahedron hex;
  static const double ZDEPTH = 0.001;
  hex.lbb = V3D(xlb, ylb, 0);
  hex.lbt = V3D(xlb, ylb, ZDEPTH);
  hex.lfb = V3D(xlf, ylf, 0);
  hex.lft = V3D(xlf, ylf, ZDEPTH);
  hex.rbb = V3D(xrb, yrb, 0);
  hex.rbt = V3D(xrb, yrb, ZDEPTH);
  hex.rfb = V3D(xrf, yrf, 0);
  hex.rft = V3D(xrf, yrf, ZDEPTH);

  std::map<int, boost::shared_ptr<Surface>> prim;
  int l_id = 1;
  auto algebra = parseHexahedronFromStruct(hex, prim, l_id);

  auto shape = boost::make_shared<CSGObject>();
  shape->setObject(21, algebra);
  shape->populate(prim);

  auto handler = boost::make_shared<GeometryHandler>(shape);
  detail::ShapeInfo shapeInfo;
  shape->setGeometryHandler(handler);

  shapeInfo.setHexahedron(hex.lbb, hex.lfb, hex.rfb, hex.rbb, hex.lbt, hex.lft,
                          hex.rft, hex.rbt);

  handler->setShapeInfo(std::move(shapeInfo));

  shape->defineBoundingBox(std::max(xrb, xrf), yrf, ZDEPTH, std::min(xlf, xlb),
                           ylb, 0);

  return shape;
}

/// create a special geometry handler for the known finite primitives
void ShapeFactory::createGeometryHandler(Poco::XML::Element *pElem,
                                         boost::shared_ptr<CSGObject> Obj) {

  auto geomHandler = boost::make_shared<GeometryHandler>(Obj);
  detail::ShapeInfo shapeInfo;
  Obj->setGeometryHandler(geomHandler);

  if (pElem->tagName() == "cuboid") {
    auto corners = parseCuboid(pElem);
    shapeInfo.setCuboid(corners.lfb, corners.lft, corners.lbb, corners.rfb);
  } else if (pElem->tagName() == "hexahedron") {
    auto corners = parseHexahedron(pElem);
    shapeInfo.setHexahedron(corners.lbb, corners.lfb, corners.rfb, corners.rbb,
                            corners.lbt, corners.lft, corners.rft, corners.rbt);
  } else if (pElem->tagName() == "sphere") {
    Element *pElemCentre = getOptionalShapeElement(pElem, "centre");
    Element *pElemRadius = getShapeElement(pElem, "radius");
    V3D centre;
    if (pElemCentre)
      centre = parsePosition(pElemCentre);
    shapeInfo.setSphere(centre, std::stod(pElemRadius->getAttribute("val")));
  } else if (pElem->tagName() == "cylinder") {
    Element *pElemCentre = getShapeElement(pElem, "centre-of-bottom-base");
    Element *pElemAxis = getShapeElement(pElem, "axis");
    Element *pElemRadius = getShapeElement(pElem, "radius");
    Element *pElemHeight = getShapeElement(pElem, "height");
    const V3D normVec = normalize(parsePosition(pElemAxis));
    shapeInfo.setCylinder(parsePosition(pElemCentre), normVec,
                          std::stod(pElemRadius->getAttribute("val")),
                          std::stod(pElemHeight->getAttribute("val")));
  } else if (pElem->tagName() == "hollow-cylinder") {
    Element *pElemCentre = getShapeElement(pElem, "centre-of-bottom-base");
    Element *pElemAxis = getShapeElement(pElem, "axis");
    Element *pElemInnerRadius = getShapeElement(pElem, "inner-radius");
    Element *pElemOuterRadius = getShapeElement(pElem, "outer-radius");
    Element *pElemHeight = getShapeElement(pElem, "height");
    V3D normVec = parsePosition(pElemAxis);
    normVec.normalize();
    shapeInfo.setHollowCylinder(
        parsePosition(pElemCentre), normVec,
        std::stod(pElemInnerRadius->getAttribute("val")),
        std::stod(pElemOuterRadius->getAttribute("val")),
        std::stod(pElemHeight->getAttribute("val")));
  } else if (pElem->tagName() == "cone") {
    Element *pElemTipPoint = getShapeElement(pElem, "tip-point");
    Element *pElemAxis = getShapeElement(pElem, "axis");
    Element *pElemAngle = getShapeElement(pElem, "angle");
    Element *pElemHeight = getShapeElement(pElem, "height");

    const V3D normVec = normalize(parsePosition(pElemAxis));
    const double height = std::stod(pElemHeight->getAttribute("val"));
    const double radius =
        height * tan(M_PI * std::stod(pElemAngle->getAttribute("val")) / 180.0);
    shapeInfo.setCone(parsePosition(pElemTipPoint), normVec, radius, height);
  }

  geomHandler->setShapeInfo(std::move(shapeInfo));
}

} // namespace Geometry
} // namespace Mantid
