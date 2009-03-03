//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/ShapeFactory.h"
#include "MantidAPI/Instrument.h"

#include "MantidGeometry/Detector.h"
#include "MantidGeometry/CompAssembly.h"
#include "MantidGeometry/Component.h"
#include "MantidGeometry/GluGeometryHandler.h"
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

  Element* lastElement=NULL; //This is to store element for the fixed complete objects such as sphere,cone,cylinder and cuboid
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
		  lastElement=pE;
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
		  lastElement=pE;
          idMatching[idFromUser] = parseCylinder(pE, primitives, l_id);  
          numPrimitives++;
        }
        else if ( !primitiveName.compare("cuboid"))
        {
		  lastElement=pE;
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
		  lastElement=pE;
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
  size_t found;
  std::map<size_t,std::string, std::greater<size_t> > allFound;
  for( iter = idMatching.begin(); iter != idMatching.end(); iter++ )
  {
    found = algebraFromUser.find(iter->first);

    if (found==std::string::npos)
      continue;
    else
    {
      allFound[found] = iter->first;
    }
  }

  // Here do the actually swapping of strings 
  std::map<size_t,std::string, std::greater<size_t> >::iterator iter2;
  for( iter2 = allFound.begin(); iter2 != allFound.end(); iter2++ )
  {
    std::string  kuse = iter2->second;
    algebraFromUser.replace(iter2->first, (iter2->second).size(), idMatching[iter2->second]);
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
	//check whether there is only one surface/closed surface
	if(numPrimitives == 1&&lastElement!=NULL)//special case
	{
		//parse the primitive and create a Geometry handler for the object
		createGeometryHandler(lastElement,retVal);
	}

    // get bounding box string
    NodeList* pNL_boundingBox = pElem->getElementsByTagName("bounding-box");
    if ( pNL_boundingBox->length() != 1)  // should probably throw an error if more than 1 bounding box is defined...
      return retVal;
    pNL_boundingBox->release();

    double xmin = atof( ((getShapeElement(pElem, "x-min"))->getAttribute("val")).c_str() );
    double ymin = atof( ((getShapeElement(pElem, "y-min"))->getAttribute("val")).c_str() );
    double zmin = atof( ((getShapeElement(pElem, "z-min"))->getAttribute("val")).c_str() );
    double xmax = atof( ((getShapeElement(pElem, "x-max"))->getAttribute("val")).c_str() );
    double ymax = atof( ((getShapeElement(pElem, "y-max"))->getAttribute("val")).c_str() );
    double zmax = atof( ((getShapeElement(pElem, "z-max"))->getAttribute("val")).c_str() );

    retVal->defineBoundingBox(xmax, ymax, zmax, xmin, ymin, zmin);

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
  Element* pElemCentre = getShapeElement(pElem, "centre"); 
  Element* pElemRadius = getShapeElement(pElem, "radius"); 

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
  Element* pElemPip = getShapeElement(pElem, "point-in-plane"); 
  Element* pElemNormal = getShapeElement(pElem, "normal-to-plane"); 

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
  Element* pElemCentre = getShapeElement(pElem, "centre");  
  Element* pElemAxis = getShapeElement(pElem, "axis");   
  Element* pElemRadius = getShapeElement(pElem, "radius"); 

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
  Element* pElemCentre = getShapeElement(pElem, "centre-of-bottom-base"); 
  Element* pElemAxis = getShapeElement(pElem, "axis"); 
  Element* pElemRadius = getShapeElement(pElem, "radius"); 
  Element* pElemHeight = getShapeElement(pElem, "height"); 

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
  Element* pElem_lfb = getShapeElement(pElem, "left-front-bottom-point"); 
  Element* pElem_lft = getShapeElement(pElem, "left-front-top-point"); 
  Element* pElem_lbb = getShapeElement(pElem, "left-back-bottom-point"); 
  Element* pElem_rfb = getShapeElement(pElem, "right-front-bottom-point"); 

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
  Element* pElemTipPoint = getShapeElement(pElem, "tip-point"); 
  Element* pElemAxis = getShapeElement(pElem, "axis"); 
  Element* pElemAngle = getShapeElement(pElem, "angle");  

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
  Element* pElemTipPoint = getShapeElement(pElem, "tip-point"); 
  Element* pElemAxis = getShapeElement(pElem, "axis");  
  Element* pElemAngle = getShapeElement(pElem, "angle");  
  Element* pElemHeight = getShapeElement(pElem, "height"); 

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
  Element* pElem_lfb = getShapeElement(pElem, "left-front-bottom-point");
  Element* pElem_lft = getShapeElement(pElem, "left-front-top-point"); 
  Element* pElem_lbb = getShapeElement(pElem, "left-back-bottom-point"); 
  Element* pElem_lbt = getShapeElement(pElem, "left-back-top-point"); 
  Element* pElem_rfb = getShapeElement(pElem, "right-front-bottom-point");  
  Element* pElem_rft = getShapeElement(pElem, "right-front-top-point");  
  Element* pElem_rbb = getShapeElement(pElem, "right-back-bottom-point"); 
  Element* pElem_rbt = getShapeElement(pElem, "right-back-top-point"); 

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
  Element* pElemCentre = getShapeElement(pElem, "centre"); 
  Element* pElemAxis = getShapeElement(pElem, "axis");  
  Element* pElemRadiusFromCentre = getShapeElement(pElem, "radius-from-centre-to-tube");
  Element* pElemRadiusTube = getShapeElement(pElem, "radius-tube");

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
std::string ShapeFactory::parseSliceOfCylinderRing(Poco::XML::Element* pElem, std::map<int, Geometry::Surface*>& prim, int& l_id)
{
  Element* pElemArc = getShapeElement(pElem, "arc"); 
  Element* pElemInnerRadius = getShapeElement(pElem, "inner-radius");  
  Element* pElemOuterRadius = getShapeElement(pElem, "outer-radius");  
  Element* pElemDepth = getShapeElement(pElem, "depth"); 

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


/** Return a subelement of an XML element, but also checks that there exist exactly one entry
 *  of this subelement.
 *
 *  @param pElem XML from instrument def. file
 *  @param name Name of subelement 
 *  @return The subelement
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
 */
Poco::XML::Element* ShapeFactory::getShapeElement(Poco::XML::Element* pElem, const std::string& name)
{
  // check if this shape element contain an element with name specified by the 2nd function argument
  NodeList* pNL = pElem->getElementsByTagName(name);
  if ( pNL->length() != 1)
  {
    throw Kernel::Exception::InstrumentDefinitionError("XML element: " + pElem->tagName() +
      " must contain exactly subelement with name: " + name + ".");
  }
  Element* retVal = static_cast<Element*>(pNL->item(0)); 
  pNL->release();
  return retVal;
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

void ShapeFactory::createGeometryHandler(Poco::XML::Element* pElem,boost::shared_ptr<Object> Obj)
{
	if(pElem->tagName()=="cuboid"){
		boost::shared_ptr<GeometryHandler> handler(new GluGeometryHandler(Obj));
		Obj->setGeometryHandler(handler);
		Element* pElem_lfb = getShapeElement(pElem, "left-front-bottom-point"); 
		Element* pElem_lft = getShapeElement(pElem, "left-front-top-point"); 
		Element* pElem_lbb = getShapeElement(pElem, "left-back-bottom-point"); 
		Element* pElem_rfb = getShapeElement(pElem, "right-front-bottom-point"); 

		V3D lfb = parsePosition(pElem_lfb);  // left front bottom
		V3D lft = parsePosition(pElem_lft);  // left front top
		V3D lbb = parsePosition(pElem_lbb);  // left back bottom
		V3D rfb = parsePosition(pElem_rfb);  // right front bottom
		((GluGeometryHandler*)(handler.get()))->setCuboid(lfb,lft,lbb,rfb);
	}else if(pElem->tagName()=="sphere"){
		boost::shared_ptr<GeometryHandler> handler(new GluGeometryHandler(Obj));
		Obj->setGeometryHandler(handler);
		Element* pElemCentre = getShapeElement(pElem, "centre"); 
		Element* pElemRadius = getShapeElement(pElem, "radius"); 
		((GluGeometryHandler*)(handler.get()))->setSphere(parsePosition(pElemCentre),atof( (pElemRadius->getAttribute("val")).c_str() ));
	}else if(pElem->tagName()=="cylinder"){
		boost::shared_ptr<GeometryHandler> handler(new GluGeometryHandler(Obj));
		Obj->setGeometryHandler(handler);
		Element* pElemCentre = getShapeElement(pElem, "centre-of-bottom-base"); 
		Element* pElemAxis = getShapeElement(pElem, "axis"); 
		Element* pElemRadius = getShapeElement(pElem, "radius"); 
		Element* pElemHeight = getShapeElement(pElem, "height");
		V3D normVec = parsePosition(pElemAxis);
		normVec.normalize();
		((GluGeometryHandler*)(handler.get()))->setCylinder(parsePosition(pElemCentre),normVec,atof( (pElemRadius->getAttribute("val")).c_str() ),atof( (pElemHeight->getAttribute("val")).c_str() ));
	}else if(pElem->tagName()=="cone"){
		boost::shared_ptr<GeometryHandler> handler(new GluGeometryHandler(Obj));
		Obj->setGeometryHandler(handler);
		Element* pElemTipPoint = getShapeElement(pElem, "tip-point"); 
		Element* pElemAxis = getShapeElement(pElem, "axis");  
		Element* pElemAngle = getShapeElement(pElem, "angle");  
		Element* pElemHeight = getShapeElement(pElem, "height"); 

		V3D normVec = parsePosition(pElemAxis);
		normVec.normalize();
		double height=atof( (pElemHeight->getAttribute("val")).c_str() );
		double radius=height*tan(atof( (pElemAngle->getAttribute("val")).c_str() ));
		((GluGeometryHandler*)(handler.get()))->setCone(parsePosition(pElemTipPoint),normVec,radius,height);
	}
	
}



} // namespace DataHandling
} // namespace Mantid
