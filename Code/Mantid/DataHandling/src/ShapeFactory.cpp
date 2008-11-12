//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/ShapeFactory.h"
#include "MantidAPI/Instrument.h"

#include "MantidGeometry/Detector.h"
#include "MantidGeometry/CompAssembly.h"
#include "MantidGeometry/Component.h"
#include "MantidKernel/PhysicalConstants.h"

#include <fstream>


#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"

using Poco::XML::DOMParser;
using Poco::XML::XMLReader;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Node;
using Poco::XML::NodeList;
using Poco::XML::NodeIterator;
using Poco::XML::NodeFilter;


namespace Mantid
{
namespace DataHandling
{

using namespace Kernel;
using namespace API;
using namespace Geometry;

Logger& ShapeFactory::g_log = Logger::get("ShapeFactory");

/// Empty default constructor
ShapeFactory::ShapeFactory()
{}


/** Creates a geometric object as specified in an instrument definition file
 *
 *  @param pElem XML element from instrument def. file which may specify a geometric shape
 *  @return A shared pointer to a geometric shape (defaults to an 'empty' shape if XML tags contain no geo. info.) 
 *
 *  @throw logic_error Thrown if argument is not a pointer to a 'type' XML element
 */

boost::shared_ptr<Object> ShapeFactory::createShape(Poco::XML::Element* pElem)
{
  // check if pElem is an element with tag name 'type'

  if ( (pElem->tagName()).compare("type") )
  {
    g_log.error("Argument to function createShape must be a pointer to an XML element with tag name type.");
    throw std::logic_error( "Argument to function createShape must be a pointer to an XML element with tag name type." );
  }


  boost::shared_ptr<Object> retVal = boost::shared_ptr<Object>(new Object);

  // get algebra string
  NodeList* pNL_algebra = pElem->getElementsByTagName("algebra");
  if ( pNL_algebra->length() != 1)
    return retVal;
  Element* pElemAlgebra = static_cast<Element*>(pNL_algebra->item(0)); 
  pNL_algebra->release();
  std::string algebraFromUser = pElemAlgebra->getAttribute("val");

  std::map<std::string,std::string> idMatching; // match id given to a shape by the user to 
                                                // id understandable by Mantid code 

  // loop over all the sub-elements of pElem

  NodeList* pNL = pElem->childNodes(); // get all child nodes
  unsigned int pNL_length = pNL->length();
  int numPrimitives = 0; // used for counting number of primitives in this 'type' XML element
  std::map<int, Surface*> primitives; // stores the primitives that will be used to build final shape
  int l_id = 1; // used to build up unique id's for each shape added. Must start from int > zero.

  for (unsigned int i = 0; i < pNL_length; i++)
  {
    if ( (pNL->item(i))->nodeType() == Node::ELEMENT_NODE )
    {
      Element* pE = static_cast<Element*>(pNL->item(i));

      // assume for now that if sub-element has attribute id then it is a shape element
      if ( pE->hasAttribute("id") )  
      {
        std::string idFromUser = pE->getAttribute("id"); // get id

        std::string primitiveName = pE->tagName();  // get name of primitive

        if ( !primitiveName.compare("sphere"))
        {
          idMatching[idFromUser] = parseSphere(pE, primitives, l_id);  
          numPrimitives++;
        }
        else if ( !primitiveName.compare("infinite-plane"))
        {
          idMatching[idFromUser] = parseInfinitePlane(pE, primitives, l_id);  
          numPrimitives++;
        }
        else if ( !primitiveName.compare("infinite-cylinder"))
        {
          idMatching[idFromUser] = parseInfiniteCylinder(pE, primitives, l_id);  
          numPrimitives++;
        }
        else if ( !primitiveName.compare("cylinder"))
        {
          idMatching[idFromUser] = parseCylinder(pE, primitives, l_id);  
          numPrimitives++;
        }
        else if ( !primitiveName.compare("cuboid"))
        {
          idMatching[idFromUser] = parseCuboid(pE, primitives, l_id);  
          numPrimitives++;
        }
        else if ( !primitiveName.compare("infinite-cone"))
        {
          idMatching[idFromUser] = parseInfiniteCone(pE, primitives, l_id);  
          numPrimitives++;
        }
        else if ( !primitiveName.compare("cone"))
        {
          idMatching[idFromUser] = parseCone(pE, primitives, l_id);  
          numPrimitives++;
        }
        else if ( !primitiveName.compare("hexahedron"))
        {
          idMatching[idFromUser] = parseHexahedron(pE, primitives, l_id);  
          numPrimitives++;
        }
        else if ( !primitiveName.compare("torus"))
        {
          idMatching[idFromUser] = parseTorus(pE, primitives, l_id);  
          numPrimitives++;
        }
        else if ( !primitiveName.compare("slice-of-cylinder-ring"))
        {
          idMatching[idFromUser] = parseSliceOfCylinderRing(pE, primitives, l_id);  
          numPrimitives++;
        }
      }
    }
  }

  // Translate algebra string defined by the user into something Mantid can
  // understand

  std::string algebra;  // to hold algebra in a way Mantid can understand
  std::map<std::string,std::string>::iterator iter;
  size_t found;  // point to location in string
  bool howFoundOne = false;
  size_t previousFound = 0;
  for( iter = idMatching.begin(); iter != idMatching.end(); iter++ )
  {
    found = algebraFromUser.find(iter->first, previousFound);

    if (found==std::string::npos)
      continue;
    else
    {
      previousFound = found;

      algebraFromUser.replace(found, (iter->first).size(), iter->second);
    }
  }


  // check to see if there actually were any primitives in 'type' xml element
  // and if yes then return empty Object. Otherwise populate Object with the
  // primitives

  if ( numPrimitives == 0 )
    return retVal;
  else
  {
    retVal->setObject(21, algebraFromUser);
    retVal->populate(primitives);

    return retVal;
  }
}


/** Parse XML 'sphere' element
 *
 *  @param pElem XML 'sphere' element from instrument def. file
 *  @param prim To add shapes to
 *  @param l_id When shapes added to the map prim l_id is the continuous incremented index 
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
 */
std::string ShapeFactory::parseSphere(Poco::XML::Element* pElem, std::map<int, Geometry::Surface*>& prim, int& l_id)
{
  // check for centre element
  NodeList* pNL_centre = pElem->getElementsByTagName("centre");
  if ( pNL_centre->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <sphere> element with missing <centre> element");
  }
  Element* pElemCentre = static_cast<Element*>(pNL_centre->item(0)); 
  pNL_centre->release();

  // check for radius element
  NodeList* pNL_radius = pElem->getElementsByTagName("radius");
  if ( pNL_radius->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <sphere> element with missing <radius> element");
  }
  Element* pElemRadius = static_cast<Element*>(pNL_radius->item(0)); 
  pNL_radius->release();

  // create sphere
  Sphere* pSphere = new Sphere;
  pSphere->setCentre(parsePosition(pElemCentre)); 
  pSphere->setRadius(atof( (pElemRadius->getAttribute("val")).c_str() ));
  prim[l_id] = pSphere;

  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(-" << l_id << ")";
  l_id++;
  return retAlgebraMatch.str();
}


/** Parse XML 'infinite-plane' element
 *
 *  @param pElem XML 'infinite-plane' element from instrument def. file
 *  @param prim To add shapes to
 *  @param l_id When shapes added to the map prim l_id is the continuous incremented index 
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
 */
std::string ShapeFactory::parseInfinitePlane(Poco::XML::Element* pElem, std::map<int, Geometry::Surface*>& prim, int& l_id)
{
  // check for point-in-plane element
  NodeList* pNL_pip = pElem->getElementsByTagName("point-in-plane");
  if ( pNL_pip->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <infinite-plane> element with missing <point-in-plane> element");
  }
  Element* pElemPip = static_cast<Element*>(pNL_pip->item(0)); 
  pNL_pip->release();

  // check for normal-to-plane element
  NodeList* pNL_normal = pElem->getElementsByTagName("normal-to-plane");
  if ( pNL_normal->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <infinite-plane> element with missing <normal-to-plane> element");
  }
  Element* pElemNormal = static_cast<Element*>(pNL_normal->item(0)); 
  pNL_normal->release();

  // create infinite-plane
  Plane* pPlane = new Plane();
  pPlane->setPlane(parsePosition(pElemPip), parsePosition(pElemNormal)); 
  prim[l_id] = pPlane;

  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(" << l_id << ")";
  l_id++;
  return retAlgebraMatch.str();
}


/** Parse XML 'infinite-cylinder' element
 *
 *  @param pElem XML 'infinite-cylinder' element from instrument def. file
 *  @param prim To add shapes to
 *  @param l_id When shapes added to the map prim l_id is the continuous incremented index 
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
 */
std::string ShapeFactory::parseInfiniteCylinder(Poco::XML::Element* pElem, std::map<int, Geometry::Surface*>& prim, int& l_id)
{
  // check for centre element
  NodeList* pNL_centre = pElem->getElementsByTagName("centre");
  if ( pNL_centre->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <infinite-cylinder> element with missing <centre> element");
  }
  Element* pElemCentre = static_cast<Element*>(pNL_centre->item(0)); 
  pNL_centre->release();

  // check for axis element
  NodeList* pNL_axis = pElem->getElementsByTagName("axis");
  if ( pNL_axis->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <infinite-cylinder> element with missing <axis> element");
  }
  Element* pElemAxis = static_cast<Element*>(pNL_axis->item(0)); 
  pNL_axis->release();

  // check for radius element
  NodeList* pNL_radius = pElem->getElementsByTagName("radius");
  if ( pNL_radius->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <infinite-cylinder> element with missing <radius> element");
  }
  Element* pElemRadius = static_cast<Element*>(pNL_radius->item(0)); 
  pNL_radius->release();

  // create infinite-cylinder
  Cylinder* pCylinder = new Cylinder();
  pCylinder->setCentre(parsePosition(pElemCentre));      

  V3D dummy1 = pCylinder->getCentre();

  pCylinder->setNorm(parsePosition(pElemAxis));
  V3D dummy2 = pCylinder->getNormal();

  pCylinder->setRadius(atof( (pElemRadius->getAttribute("val")).c_str() ));
  prim[l_id] = pCylinder;

  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(-" << l_id << ")";
  l_id++;
  return retAlgebraMatch.str();
}


/** Parse XML 'cylinder' element
 *
 *  @param pElem XML 'cylinder' element from instrument def. file
 *  @param prim To add shapes to
 *  @param l_id When shapes added to the map prim l_id is the continuous incremented index 
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
 */
std::string ShapeFactory::parseCylinder(Poco::XML::Element* pElem, std::map<int, Geometry::Surface*>& prim, int& l_id)
{
  // check for centre element
  NodeList* pNL_centre = pElem->getElementsByTagName("centre-of-bottom-base");
  if ( pNL_centre->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <cylinder> element with missing <centre-of-bottom-base> element");
  }
  Element* pElemCentre = static_cast<Element*>(pNL_centre->item(0)); 
  pNL_centre->release();

  // check for axis element
  NodeList* pNL_axis = pElem->getElementsByTagName("axis");
  if ( pNL_axis->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <cylinder> element with missing <axis> element");
  }
  Element* pElemAxis = static_cast<Element*>(pNL_axis->item(0)); 
  pNL_axis->release();

  // check for radius element
  NodeList* pNL_radius = pElem->getElementsByTagName("radius");
  if ( pNL_radius->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <cylinder> element with missing <radius> element");
  }
  Element* pElemRadius = static_cast<Element*>(pNL_radius->item(0)); 
  pNL_radius->release();

  // check for height element
  NodeList* pNL_height = pElem->getElementsByTagName("height");
  if ( pNL_height->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <cylinder> element with missing <height> element");
  }
  Element* pElemHeight = static_cast<Element*>(pNL_height->item(0)); 
  pNL_height->release();


  V3D normVec = parsePosition(pElemAxis);
  normVec.normalize();

  // add infinite cylinder
  Cylinder* pCylinder = new Cylinder();
  pCylinder->setCentre(parsePosition(pElemCentre));              
  pCylinder->setNorm(normVec);  
  pCylinder->setRadius(atof( (pElemRadius->getAttribute("val")).c_str() ));
  prim[l_id] = pCylinder;

  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(-" << l_id << " ";
  l_id++;

  // add top plane
  Plane* pPlaneTop = new Plane();
  V3D pointInPlane = parsePosition(pElemCentre);
  double height = atof( (pElemHeight->getAttribute("val")).c_str() );
  pointInPlane += (normVec * height); // to get point in top plane
  pPlaneTop->setPlane(pointInPlane, normVec); 
  prim[l_id] = pPlaneTop;
  retAlgebraMatch << "-" << l_id << " ";
  l_id++;

  // add bottom plane
  Plane* pPlaneBottom = new Plane();
  pPlaneBottom->setPlane(parsePosition(pElemCentre), normVec); 
  prim[l_id] = pPlaneBottom;
  retAlgebraMatch << "" << l_id << ")";
  l_id++;

  return retAlgebraMatch.str();
}


/** Parse XML 'cuboid' element
 *
 *  @param pElem XML 'cuboid' element from instrument def. file
 *  @param prim To add shapes to
 *  @param l_id When shapes added to the map prim l_id is the continuous incremented index 
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
 */
std::string ShapeFactory::parseCuboid(Poco::XML::Element* pElem, std::map<int, Geometry::Surface*>& prim, int& l_id)
{
  // check for left-front-bottom-point element
  NodeList* pNL_lfb = pElem->getElementsByTagName("left-front-bottom-point");
  if ( pNL_lfb->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <cuboid> element with missing <left-front-bottom-point> element");
  }
  Element* pElem_lfb = static_cast<Element*>(pNL_lfb->item(0)); 
  pNL_lfb->release();

  // check for left-front-top-point element
  NodeList* pNL_lft = pElem->getElementsByTagName("left-front-top-point");
  if ( pNL_lft->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <cuboid> element with missing <left-front-top-point> element");
  }
  Element* pElem_lft = static_cast<Element*>(pNL_lft->item(0)); 
  pNL_lft->release();

  // check for left-back-bottom-point element
  NodeList* pNL_lbb = pElem->getElementsByTagName("left-back-bottom-point");
  if ( pNL_lbb->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <cuboid> element with missing <left-back-bottom-point> element");
  }
  Element* pElem_lbb = static_cast<Element*>(pNL_lbb->item(0)); 
  pNL_lbb->release();

  // check for right-front-bottom-point element
  NodeList* pNL_rfb = pElem->getElementsByTagName("right-front-bottom-point");
  if ( pNL_rfb->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <cuboid> element with missing <right-front-bottom-point> element");
  }
  Element* pElem_rfb = static_cast<Element*>(pNL_rfb->item(0)); 
  pNL_rfb->release();


  V3D lfb = parsePosition(pElem_lfb);  // left front bottom
  V3D lft = parsePosition(pElem_lft);  // left front top
  V3D lbb = parsePosition(pElem_lbb);  // left back bottom
  V3D rfb = parsePosition(pElem_rfb);  // right front bottom

  V3D pointTowardBack = lbb-lfb;
  pointTowardBack.normalize();

  // add front plane cutoff
  Plane* pPlaneFrontCutoff = new Plane();
  pPlaneFrontCutoff->setPlane(lfb, pointTowardBack); 
  prim[l_id] = pPlaneFrontCutoff;

  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(" << l_id << " ";
  l_id++;

  // add back plane cutoff
  Plane* pPlaneBackCutoff = new Plane();
  pPlaneBackCutoff->setPlane(lbb, pointTowardBack); 
  prim[l_id] = pPlaneBackCutoff;
  retAlgebraMatch << "-" << l_id << " ";
  l_id++;


  V3D pointTowardRight = rfb-lfb;
  pointTowardRight.normalize();

  // add left plane cutoff
  Plane* pPlaneLeftCutoff = new Plane();
  pPlaneLeftCutoff->setPlane(lfb, pointTowardRight); 
  prim[l_id] = pPlaneLeftCutoff;
  retAlgebraMatch << "" << l_id << " ";
  l_id++;

  // add right plane cutoff
  Plane* pPlaneRightCutoff = new Plane();
  pPlaneRightCutoff->setPlane(rfb, pointTowardRight); 
  prim[l_id] = pPlaneRightCutoff;
  retAlgebraMatch << "-" << l_id << " ";
  l_id++;


  V3D pointTowardTop = lft-lfb;
  pointTowardTop.normalize();

  // add bottom plane cutoff
  Plane* pPlaneBottomCutoff = new Plane();
  pPlaneBottomCutoff->setPlane(lfb, pointTowardTop); 
  prim[l_id] = pPlaneBottomCutoff;
  retAlgebraMatch << "" << l_id << " ";
  l_id++;

  // add top plane cutoff
  Plane* pPlaneTopCutoff = new Plane();
  pPlaneTopCutoff->setPlane(lft, pointTowardTop); 
  prim[l_id] = pPlaneTopCutoff;
  retAlgebraMatch << "-" << l_id << ")";
  l_id++;

  return retAlgebraMatch.str();
}


/** Parse XML 'infinite-cone' element
 *
 *  @param pElem XML 'infinite-cone' element from instrument def. file
 *  @param prim To add shapes to
 *  @param l_id When shapes added to the map prim l_id is the continuous incremented index 
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
 */
std::string ShapeFactory::parseInfiniteCone(Poco::XML::Element* pElem, std::map<int, Geometry::Surface*>& prim, int& l_id)
{
  // check for tip-point element
  NodeList* pNL_tipPoint = pElem->getElementsByTagName("tip-point");
  if ( pNL_tipPoint->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <infinite-cone> element with missing <tip-point> element");
  }
  Element* pElemTipPoint = static_cast<Element*>(pNL_tipPoint->item(0)); 
  pNL_tipPoint->release();

  // check for axis element
  NodeList* pNL_axis = pElem->getElementsByTagName("axis");
  if ( pNL_axis->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <infinite-cone> element with missing <axis> element");
  }
  Element* pElemAxis = static_cast<Element*>(pNL_axis->item(0)); 
  pNL_axis->release();

  // check for angle element
  NodeList* pNL_angle = pElem->getElementsByTagName("angle");
  if ( pNL_angle->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <infinite-cone> element with missing <angle> element");
  }
  Element* pElemAngle = static_cast<Element*>(pNL_angle->item(0)); 
  pNL_angle->release();

  V3D normVec = parsePosition(pElemAxis);
  normVec.normalize();

  // add infinite double cone
  Cone* pCone = new Cone();
  pCone->setCentre(parsePosition(pElemTipPoint));              
  pCone->setNorm(normVec);  
  pCone->setAngle(atof( (pElemAngle->getAttribute("val")).c_str() ));
  prim[l_id] = pCone;

  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(" << l_id << " ";
  l_id++;


  // plane top cut of top part of double cone  
  Plane* pPlaneBottom = new Plane();
  pPlaneBottom->setPlane(parsePosition(pElemTipPoint), normVec); 
  prim[l_id] = pPlaneBottom;
  retAlgebraMatch << "-" << l_id << ")";
  l_id++;

  return retAlgebraMatch.str();
}


/** Parse XML 'cone' element
 *
 *  @param pElem XML 'cone' element from instrument def. file
 *  @param prim To add shapes to
 *  @param l_id When shapes added to the map prim l_id is the continuous incremented index 
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
 */
std::string ShapeFactory::parseCone(Poco::XML::Element* pElem, std::map<int, Geometry::Surface*>& prim, int& l_id)
{
  // check for tip-point element
  NodeList* pNL_tipPoint = pElem->getElementsByTagName("tip-point");
  if ( pNL_tipPoint->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <cone> element with missing <tip-point> element");
  }
  Element* pElemTipPoint = static_cast<Element*>(pNL_tipPoint->item(0)); 
  pNL_tipPoint->release();

  // check for axis element
  NodeList* pNL_axis = pElem->getElementsByTagName("axis");
  if ( pNL_axis->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <cone> element with missing <axis> element");
  }
  Element* pElemAxis = static_cast<Element*>(pNL_axis->item(0)); 
  pNL_axis->release();

  // check for angle element
  NodeList* pNL_angle = pElem->getElementsByTagName("angle");
  if ( pNL_angle->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <cone> element with missing <angle> element");
  }
  Element* pElemAngle = static_cast<Element*>(pNL_angle->item(0)); 
  pNL_angle->release();

  // check for height element
  NodeList* pNL_height = pElem->getElementsByTagName("height");
  if ( pNL_height->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <cone> element with missing <height> element");
  }
  Element* pElemHeight = static_cast<Element*>(pNL_height->item(0)); 
  pNL_height->release();


  V3D normVec = parsePosition(pElemAxis);
  normVec.normalize();

  // add infinite double cone
  Cone* pCone = new Cone();
  pCone->setCentre(parsePosition(pElemTipPoint));              
  pCone->setNorm(normVec);  
  pCone->setAngle(atof( (pElemAngle->getAttribute("val")).c_str() ));
  prim[l_id] = pCone;

  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(" << l_id << " ";
  l_id++;

  // Plane to cut off cone from below
  Plane* pPlaneTop = new Plane();
  V3D pointInPlane = parsePosition(pElemTipPoint);
  double height = atof( (pElemHeight->getAttribute("val")).c_str() );
  pointInPlane -= (normVec * height);
  pPlaneTop->setPlane(pointInPlane, normVec); 
  prim[l_id] = pPlaneTop;
  retAlgebraMatch << "" << l_id << " ";
  l_id++; 

  // plane top cut of top part of double cone 
  Plane* pPlaneBottom = new Plane();
  pPlaneBottom->setPlane(parsePosition(pElemTipPoint), normVec); 
  prim[l_id] = pPlaneBottom;
  retAlgebraMatch << "-" << l_id << ")";
  l_id++;

  return retAlgebraMatch.str();
}


/** Parse XML 'hexahedron' element
 *
 *  @param pElem XML 'hexahedron' element from instrument def. file
 *  @param prim To add shapes to
 *  @param l_id When shapes added to the map prim l_id is the continuous incremented index 
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
 */
std::string ShapeFactory::parseHexahedron(Poco::XML::Element* pElem, std::map<int, Geometry::Surface*>& prim, int& l_id)
{
  // check for left-front-bottom-point element
  NodeList* pNL_lfb = pElem->getElementsByTagName("left-front-bottom-point");
  if ( pNL_lfb->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <hexahedron> element with missing <left-front-bottom-point> element");
  }
  Element* pElem_lfb = static_cast<Element*>(pNL_lfb->item(0)); 
  pNL_lfb->release();

  // check for left-front-top-point element
  NodeList* pNL_lft = pElem->getElementsByTagName("left-front-top-point");
  if ( pNL_lft->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <hexahedron> element with missing <left-front-top-point> element");
  }
  Element* pElem_lft = static_cast<Element*>(pNL_lft->item(0)); 
  pNL_lft->release();

  // check for left-back-bottom-point element
  NodeList* pNL_lbb = pElem->getElementsByTagName("left-back-bottom-point");
  if ( pNL_lbb->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <hexahedron> element with missing <left-back-bottom-point> element");
  }
  Element* pElem_lbb = static_cast<Element*>(pNL_lbb->item(0)); 
  pNL_lbb->release();

  // check for left-back-top-point element
  NodeList* pNL_lbt = pElem->getElementsByTagName("left-back-top-point");
  if ( pNL_lbt->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <hexahedron> element with missing <left-back-top-point> element");
  }
  Element* pElem_lbt = static_cast<Element*>(pNL_lbt->item(0)); 
  pNL_lbt->release();

  // check for right-front-bottom-point element
  NodeList* pNL_rfb = pElem->getElementsByTagName("right-front-bottom-point");
  if ( pNL_rfb->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <hexahedron> element with missing <right-front-bottom-point> element");
  }
  Element* pElem_rfb = static_cast<Element*>(pNL_rfb->item(0)); 
  pNL_rfb->release();

  // check for right-front-top-point element
  NodeList* pNL_rft = pElem->getElementsByTagName("right-front-top-point");
  if ( pNL_rft->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <hexahedron> element with missing <right-front-top-point> element");
  }
  Element* pElem_rft = static_cast<Element*>(pNL_rft->item(0)); 
  pNL_rft->release();

  // check for right-back-bottom-point element
  NodeList* pNL_rbb = pElem->getElementsByTagName("right-back-bottom-point");
  if ( pNL_rbb->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <hexahedron> element with missing <right-back-bottom-point> element");
  }
  Element* pElem_rbb = static_cast<Element*>(pNL_rbb->item(0)); 
  pNL_rbb->release();

  // check for right-back-top-point element
  NodeList* pNL_rbt = pElem->getElementsByTagName("right-back-top-point");
  if ( pNL_rbt->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <hexahedron> element with missing <right-back-top-point> element");
  }
  Element* pElem_rbt = static_cast<Element*>(pNL_rbt->item(0)); 
  pNL_rbt->release();


  V3D lfb = parsePosition(pElem_lfb);  // left front bottom
  V3D lft = parsePosition(pElem_lft);  // left front top
  V3D lbb = parsePosition(pElem_lbb);  // left back bottom
  V3D lbt = parsePosition(pElem_lbt);  // left back top
  V3D rfb = parsePosition(pElem_rfb);  // right front bottom
  V3D rft = parsePosition(pElem_rft);  // right front top
  V3D rbb = parsePosition(pElem_rbb);  // right back bottom
  V3D rbt = parsePosition(pElem_rbt);  // right back top

  V3D pointTowardBack = lbb-lfb;
  pointTowardBack.normalize();

  V3D normal;

  // add front face
  Plane* pPlaneFrontCutoff = new Plane();
  normal = (rfb-lfb).cross_prod(lft-lfb);
  pPlaneFrontCutoff->setPlane(lfb, normal); 
  prim[l_id] = pPlaneFrontCutoff;
  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(-" << l_id << " ";
  l_id++;

  // add back face
  Plane* pPlaneBackCutoff = new Plane();
  normal = (rbb-lbb).cross_prod(lbt-lbb);
  pPlaneBackCutoff->setPlane(lbb, normal); 
  prim[l_id] = pPlaneBackCutoff;
  retAlgebraMatch << "" << l_id << " ";
  l_id++;

  // add left face
  Plane* pPlaneLeftCutoff = new Plane();
  normal = (lbb-lfb).cross_prod(lft-lfb);
  pPlaneLeftCutoff->setPlane(lfb, normal); 
  prim[l_id] = pPlaneLeftCutoff;
  retAlgebraMatch << "" << l_id << " ";
  l_id++;

  // add right face
  Plane* pPlaneRightCutoff = new Plane();
  normal = (rbb-rfb).cross_prod(rft-rfb);
  pPlaneRightCutoff->setPlane(rfb, normal); 
  prim[l_id] = pPlaneRightCutoff;
  retAlgebraMatch << "-" << l_id << " ";
  l_id++;

  // add top face
  Plane* pPlaneTopCutoff = new Plane();
  normal = (rft-lft).cross_prod(lbt-lft);
  pPlaneTopCutoff->setPlane(lft, normal); 
  prim[l_id] = pPlaneTopCutoff;
  retAlgebraMatch << "-" << l_id << " ";
  l_id++;

  // add bottom face
  Plane* pPlaneBottomCutoff = new Plane();
  normal = (rfb-lfb).cross_prod(lbb-lfb);
  pPlaneBottomCutoff->setPlane(lfb, normal); 
  prim[l_id] = pPlaneBottomCutoff;
  retAlgebraMatch << "" << l_id << ")";
  l_id++;

  return retAlgebraMatch.str();
}


/** Parse XML 'torus' element
 *
 *  @param pElem XML 'torus' element from instrument def. file
 *  @param prim To add shapes to
 *  @param l_id When shapes added to the map prim l_id is the continuous incremented index 
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
 */
std::string ShapeFactory::parseTorus(Poco::XML::Element* pElem, std::map<int, Geometry::Surface*>& prim, int& l_id)
{
  // check for centre element
  NodeList* pNL_centre = pElem->getElementsByTagName("centre");
  if ( pNL_centre->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <torus> element with missing <centre> element");
  }
  Element* pElemCentre = static_cast<Element*>(pNL_centre->item(0)); 
  pNL_centre->release();

  // check for axis element
  NodeList* pNL_axis = pElem->getElementsByTagName("axis");
  if ( pNL_axis->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <torus> element with missing <axis> element");
  }
  Element* pElemAxis = static_cast<Element*>(pNL_axis->item(0)); 
  pNL_axis->release();

  // check for radius-from-centre-to-tube element
  NodeList* pNL_radiusFromCentre = pElem->getElementsByTagName("radius-from-centre-to-tube");
  if ( pNL_radiusFromCentre->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <torus> element with missing <radius-from-centre-to-tube> element");
  }
  Element* pElemRadiusFromCentre = static_cast<Element*>(pNL_radiusFromCentre->item(0)); 
  pNL_radiusFromCentre->release();

  // check for radius-tube element
  NodeList* pNL_radiusTube = pElem->getElementsByTagName("radius-tube");
  if ( pNL_radiusTube->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <torus> element with missing <radius-from-centre-to-tube> element");
  }
  Element* pElemRadiusTube = static_cast<Element*>(pNL_radiusTube->item(0)); 
  pNL_radiusTube->release();

  V3D normVec = parsePosition(pElemAxis);
  normVec.normalize();

  // add torus
  Torus* pTorus = new Torus();
  pTorus->setCentre(parsePosition(pElemCentre));              
  pTorus->setNorm(normVec);  
  pTorus->setDistanceFromCentreToTube(atof( (pElemRadiusFromCentre->getAttribute("val")).c_str() ));
  pTorus->setTubeRadius(atof( (pElemRadiusTube->getAttribute("val")).c_str() ));
  prim[l_id] = pTorus;

  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(-" << l_id << ")";
  l_id++;

  return retAlgebraMatch.str();
}


/** Parse XML 'slice-of-cylinder-ring' element
 *
 *  @param pElem XML 'slice-of-cylinder-ring' element from instrument def. file
 *  @param prim To add shapes to
 *  @param l_id When shapes added to the map prim l_id is the continuous incremented index 
 *  @return A Mantid algebra string for this shape
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
 */
/*
std::string ShapeFactory::parseSliceOfCylinderRing(Poco::XML::Element* pElem, std::map<int, Geometry::Surface*>& prim, int& l_id)
{
  // check for arc element
  NodeList* pNL_arc = pElem->getElementsByTagName("arc");
  if ( pNL_arc->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <slice-of-cylinder-ring> element with missing <arc> element");
  }
  Element* pElemArc = static_cast<Element*>(pNL_arc->item(0)); 
  pNL_arc->release();

  // check for inner-radius element
  NodeList* pNL_inner_radius = pElem->getElementsByTagName("inner-radius");
  if ( pNL_inner_radius->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <slice-of-cylinder-ring> element with missing <inner-radius> element");
  }
  Element* pElemInnerRadius = static_cast<Element*>(pNL_inner_radius->item(0)); 
  pNL_inner_radius->release();

  // check for outer-radius element
  NodeList* pNL_outer_radius = pElem->getElementsByTagName("outer-radius");
  if ( pNL_outer_radius->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <slice-of-cylinder-ring> element with missing <outer-radius> element");
  }
  Element* pElemOuterRadius = static_cast<Element*>(pNL_outer_radius->item(0)); 
  pNL_outer_radius->release();

  // check for depth element
  NodeList* pNL_depth = pElem->getElementsByTagName("depth");
  if ( pNL_depth->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <slice-of-cylinder-ring> element with missing <depth> element");
  }
  Element* pElemDepth = static_cast<Element*>(pNL_depth->item(0)); 
  pNL_depth->release();


  // the code below is temporarily added to allow for visualisation while none box
  // shaped don't show up

  double innerRadius = atof( (pElemInnerRadius->getAttribute("val")).c_str() );
  double outerRadius = atof( (pElemOuterRadius->getAttribute("val")).c_str() );
  double middleRadius = (outerRadius + innerRadius) / 2.0;
  double depth = atof( (pElemDepth->getAttribute("val")).c_str() );  // in z-direction
  double arc = atof( (pElemArc->getAttribute("val")).c_str() );

  double width = outerRadius - innerRadius;

  double halfHeight = sin((arc/2)*M_PI/180.0)*middleRadius;

  V3D lfb(width/2.0, -halfHeight, 0.0);  // left front bottom
  V3D lft(width/2.0, -halfHeight, depth);  // left front top
  V3D lbb(-width/2.0, -halfHeight, 0.0);  // left back bottom
  V3D rfb(width/2.0, halfHeight, 0.0);  // right front bottom

  V3D pointTowardBack = lbb-lfb;
  pointTowardBack.normalize();

  // add front plane cutoff
  Plane* pPlaneFrontCutoff = new Plane();
  pPlaneFrontCutoff->setPlane(lfb, pointTowardBack); 
  prim[l_id] = pPlaneFrontCutoff;

  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(" << l_id << " ";
  l_id++;

  // add back plane cutoff
  Plane* pPlaneBackCutoff = new Plane();
  pPlaneBackCutoff->setPlane(lbb, pointTowardBack); 
  prim[l_id] = pPlaneBackCutoff;
  retAlgebraMatch << "-" << l_id << " ";
  l_id++;


  V3D pointTowardRight = rfb-lfb;
  pointTowardRight.normalize();

  // add left plane cutoff
  Plane* pPlaneLeftCutoff = new Plane();
  pPlaneLeftCutoff->setPlane(lfb, pointTowardRight); 
  prim[l_id] = pPlaneLeftCutoff;
  retAlgebraMatch << "" << l_id << " ";
  l_id++;

  // add right plane cutoff
  Plane* pPlaneRightCutoff = new Plane();
  pPlaneRightCutoff->setPlane(rfb, pointTowardRight); 
  prim[l_id] = pPlaneRightCutoff;
  retAlgebraMatch << "-" << l_id << " ";
  l_id++;


  V3D pointTowardTop = lft-lfb;
  pointTowardTop.normalize();

  // add bottom plane cutoff
  Plane* pPlaneBottomCutoff = new Plane();
  pPlaneBottomCutoff->setPlane(lfb, pointTowardTop); 
  prim[l_id] = pPlaneBottomCutoff;
  retAlgebraMatch << "" << l_id << " ";
  l_id++;

  // add top plane cutoff
  Plane* pPlaneTopCutoff = new Plane();
  pPlaneTopCutoff->setPlane(lft, pointTowardTop); 
  prim[l_id] = pPlaneTopCutoff;
  retAlgebraMatch << "-" << l_id << ")";
  l_id++;




  return retAlgebraMatch.str();
}
*/


/* This is the correct code for this method. The code above is a hack until visualisation of none box shaped fixed */
std::string ShapeFactory::parseSliceOfCylinderRing(Poco::XML::Element* pElem, std::map<int, Geometry::Surface*>& prim, int& l_id)
{
  // check for arc element
  NodeList* pNL_arc = pElem->getElementsByTagName("arc");
  if ( pNL_arc->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <slice-of-cylinder-ring> element with missing <arc> element");
  }
  Element* pElemArc = static_cast<Element*>(pNL_arc->item(0)); 
  pNL_arc->release();

  // check for inner-radius element
  NodeList* pNL_inner_radius = pElem->getElementsByTagName("inner-radius");
  if ( pNL_inner_radius->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <slice-of-cylinder-ring> element with missing <inner-radius> element");
  }
  Element* pElemInnerRadius = static_cast<Element*>(pNL_inner_radius->item(0)); 
  pNL_inner_radius->release();

  // check for outer-radius element
  NodeList* pNL_outer_radius = pElem->getElementsByTagName("outer-radius");
  if ( pNL_outer_radius->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <slice-of-cylinder-ring> element with missing <outer-radius> element");
  }
  Element* pElemOuterRadius = static_cast<Element*>(pNL_outer_radius->item(0)); 
  pNL_outer_radius->release();

  // check for depth element
  NodeList* pNL_depth = pElem->getElementsByTagName("depth");
  if ( pNL_depth->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML <type> element: " + pElem->tagName() +
      " contains <slice-of-cylinder-ring> element with missing <depth> element");
  }
  Element* pElemDepth = static_cast<Element*>(pNL_depth->item(0)); 
  pNL_depth->release();


  ///////////////////////////////////////////////////////
  // Remember I need to translate the sliced cylinder ring to origin
  // doto when visualisation of none box shapes fixed
  ///////////////////////////////////////////////////////

  double innerRadius = atof( (pElemInnerRadius->getAttribute("val")).c_str() );
  double outerRadius = atof( (pElemOuterRadius->getAttribute("val")).c_str() );
  double middleRadius = (outerRadius + innerRadius)/2.0;

  V3D normVec(0,0,1);
  V3D centrePoint(-middleRadius,0,0);

  // add inner infinite cylinder 
  Cylinder* pCylinder1 = new Cylinder();
  pCylinder1->setCentre(centrePoint);              
  pCylinder1->setNorm(normVec);  
  pCylinder1->setRadius(innerRadius);
  prim[l_id] = pCylinder1;
  std::stringstream retAlgebraMatch;
  retAlgebraMatch << "(" << l_id << " ";
  l_id++;

  // add outer infinite cylinder 
  Cylinder* pCylinder2 = new Cylinder();
  pCylinder2->setCentre(centrePoint);              
  pCylinder2->setNorm(normVec);  
  pCylinder2->setRadius(outerRadius);
  prim[l_id] = pCylinder2;
  retAlgebraMatch << "-" << l_id << " ";
  l_id++;

  // add top cutoff plane of infinite cylinder ring
  Plane* pPlaneTop = new Plane();
  double depth = atof( (pElemDepth->getAttribute("val")).c_str() );
  pPlaneTop->setPlane(V3D(0,0,depth), normVec); 
  prim[l_id] = pPlaneTop;
  retAlgebraMatch << "-" << l_id << " ";
  l_id++;

  // add bottom cutoff plane (which is assumed to fase the sample) 
  // which at this point will result in a cylinder ring
  Plane* pPlaneBottom = new Plane();
  pPlaneBottom->setPlane(V3D(0,0,0), normVec); 
  prim[l_id] = pPlaneBottom;
  retAlgebraMatch << "" << l_id << " ";
  l_id++;


  // the two planes that are going to cut a slice of the cylinder ring

  double arc = (M_PI/180.0) * atof( (pElemArc->getAttribute("val")).c_str() );

  Plane* pPlaneSlice1 = new Plane();
  pPlaneSlice1->setPlane(V3D(-middleRadius,0,0), V3D(cos(arc/2.0+M_PI/2.0),sin(arc/2.0+M_PI/2.0),0)); 
  prim[l_id] = pPlaneSlice1;
  retAlgebraMatch << "-" << l_id << " ";
  l_id++;

  Plane* pPlaneSlice2 = new Plane();
  pPlaneSlice2->setPlane(V3D(-middleRadius,0,0), V3D(cos(-arc/2.0+M_PI/2.0),sin(-arc/2.0+M_PI/2.0),0)); 
  prim[l_id] = pPlaneSlice2;
  retAlgebraMatch << "" << l_id << ")";
  l_id++;

  return retAlgebraMatch.str();
}



/** Get position coordinates from XML element
 *
 *  @param pElem XML element whose attributes contain position coordinates
 *  @return Position coordinates in the form of a V3D object 
 */
V3D ShapeFactory::parsePosition(Poco::XML::Element* pElem)
{
  V3D retVal;

  if ( pElem->hasAttribute("R") || pElem->hasAttribute("theta") || pElem->hasAttribute("phi") )
  {
    double R=0.0, theta=0.0, phi=0.0;

    if ( pElem->hasAttribute("R") ) R = atof((pElem->getAttribute("R")).c_str());
    if ( pElem->hasAttribute("theta") ) theta = atof((pElem->getAttribute("theta")).c_str());
    if ( pElem->hasAttribute("phi") ) phi = atof((pElem->getAttribute("phi")).c_str());

    retVal.spherical(R,theta,phi);
  }
  else if ( pElem->hasAttribute("r") || pElem->hasAttribute("t") ||
    pElem->hasAttribute("p") )
  // This is alternative way a user may specify sphecical coordinates
  // which may be preferred in the long run to the more verbose of
  // using R, theta and phi.
  {
    double R=0.0, theta=0.0, phi=0.0;

    if ( pElem->hasAttribute("r") )
      R = atof((pElem->getAttribute("r")).c_str());
    if ( pElem->hasAttribute("t") )
      theta = atof((pElem->getAttribute("t")).c_str());
    if ( pElem->hasAttribute("p") )
      phi = atof((pElem->getAttribute("p")).c_str());

    retVal.spherical(R,theta,phi);
  }
  else
  {
    double x=0.0, y=0.0, z=0.0;

    if ( pElem->hasAttribute("x") ) x = atof((pElem->getAttribute("x")).c_str());
    if ( pElem->hasAttribute("y") ) y = atof((pElem->getAttribute("y")).c_str());
    if ( pElem->hasAttribute("z") ) z = atof((pElem->getAttribute("z")).c_str());

    retVal(x,y,z);
  }

  return retVal;
}




} // namespace DataHandling
} // namespace Mantid
