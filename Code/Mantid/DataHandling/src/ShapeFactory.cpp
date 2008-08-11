//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/ShapeFactory.h"
#include "MantidAPI/Instrument.h"

#include "MantidGeometry/Detector.h"
#include "MantidGeometry/CompAssembly.h"
#include "MantidGeometry/Component.h"

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
  std::string algebra = pElemAlgebra->getAttribute("val");

  // loop over all the sub-elements of pElem

  NodeList* pNL = pElem->childNodes(); // get all child nodes
  unsigned int pNL_length = pNL->length();
  int numPrimitives = 0; // used for counting number of primitives in this 'type' XML element
  std::map<int, Surface*> primitives; // stores the primitives that will be used to build final shape
  for (unsigned int i = 0; i < pNL_length; i++)
  {
    if ( (pNL->item(i))->nodeType() == Node::ELEMENT_NODE )
    {
      Element* pE = static_cast<Element*>(pNL->item(i));

      // assume for now that if sub-element has attribute id then it is a shape element
      if ( pE->hasAttribute("id") )  
      {
        int l_id = atoi( (pE->getAttribute("id")).c_str() ); // get id

        std::string primitiveName = pE->tagName();  // get name of primitive

        if ( !primitiveName.compare("sphere"))
        {
          primitives[l_id] = parseSphere(pE);  
          numPrimitives++;
        }
        else if ( !primitiveName.compare("infinite-plane"))
        {
          primitives[l_id] = parseInfinitePlane(pE);  
          numPrimitives++;
        }
        else if ( !primitiveName.compare("infinite-cylinder"))
        {
          primitives[l_id] = parseInfiniteCylinder(pE);  
          numPrimitives++;
        }
      }
    }
  }


  // check to see if there actually were any primitives in 'type' xml element
  // and if yes then return empty Object. Otherwise populate Object with the
  // primitives

  if ( numPrimitives == 0 )
    return retVal;
  else
  {
    retVal->setObject(21, algebra);
    retVal->populate(primitives);

    return retVal;
  }
}


/** Parse XML 'sphere' element
 *
 *  @param pElem XML 'sphere' element from instrument def. file
 *  @return A pointer to the sphere object allocated in this method 
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
 */
Sphere* ShapeFactory::parseSphere(Poco::XML::Element* pElem)
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
  Sphere* retVal = new Sphere;
  retVal->setCentre(parsePosition(pElemCentre)); 
  retVal->setRadius(atof( (pElemRadius->getAttribute("val")).c_str() ));

  return retVal;
}


/** Parse XML 'infinite-plane' element
 *
 *  @param pElem XML 'infinite-plane' element from instrument def. file
 *  @return A pointer to the infinite-plane object allocated in this method 
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
 */
Plane* ShapeFactory::parseInfinitePlane(Poco::XML::Element* pElem)
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
  Plane* retVal = new Plane();
  retVal->setPlane(parsePosition(pElemPip), parsePosition(pElemNormal)); 

  return retVal;
}


/** Parse XML 'infinite-cylinder' element
 *
 *  @param pElem XML 'infinite-cylinder' element from instrument def. file
 *  @return A pointer to the infinite-cylinder object allocated in this method 
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
 */
Cylinder* ShapeFactory::parseInfiniteCylinder(Poco::XML::Element* pElem)
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
  Cylinder* retVal = new Cylinder();
  retVal->setCentre(parsePosition(pElemCentre));              
  retVal->setNorm(parsePosition(pElemAxis));  
  retVal->setRadius(atof( (pElemRadius->getAttribute("val")).c_str() ));

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




} // namespace DataHandling
} // namespace Mantid
