//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadInstrumentHelper.h"
#include "MantidDataHandling/LoadInstCompsIntoOneShape.h"
#include "MantidDataHandling/LoadParameterFile.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"
#include "MantidAPI/Progress.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheReader.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheWriter.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Interpolation.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/ArrayProperty.h"

#include <Poco/DOM/AbstractNode.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/Exception.h>
#include <sstream>

using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Node;
using Poco::XML::NodeList;
using Poco::XML::NodeIterator;
using Poco::XML::NodeFilter;

/// For debugging...
static bool VERBOSE = false;

namespace Mantid
{
  namespace DataHandling
  {

    using namespace Kernel;
    using namespace API;
    using namespace Geometry;

/** Takes as input a <type> element containing a <combine-components-into-one-shape>, and 
*   adjust the <type> element by replacing its containing <component> elements with <cuboid>'s
*   (note for now this will only work for <cuboid>'s and when necessary this can be extended).
*
*  @param pElem ::  Poco::XML <type> element that we want to adjust
*  @param isTypeAssembly[in] :: tell whether any other type, but the special one treated here, is assembly or not
*  @param getTypeElement[in] :: contain pointers to all types but the onces treated here
*
*  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
*/
void LoadInstCompsIntoOneShape::adjust(Poco::XML::Element* pElem, std::map<std::string,bool>& isTypeAssembly, 
                                       std::map<std::string,Poco::XML::Element*>& getTypeElement)
{
  // check if pElem is an element with tag name 'type'
  if ( (pElem->tagName()).compare("type") )
    throw Exception::InstrumentDefinitionError( "Argument to function adjust() must be a pointer to an XML element with tag name type." );
  
  // check that there is a <combine-components-into-one-shape> element in type
  NodeList* pNLccioh = pElem->getElementsByTagName("combine-components-into-one-shape");
  if ( pNLccioh->length() == 0 )
  {
    throw Exception::InstrumentDefinitionError( std::string("Argument to function adjust() must be a pointer to an XML element with tag name type,")
          + " which contain a <combine-components-into-one-shape> element.");
  }
  pNLccioh->release();

  // check that there is a <algebra> element in type
  NodeList* pNLalg = pElem->getElementsByTagName("algebra");
  if ( pNLalg->length() == 0 )
  {
    throw Exception::InstrumentDefinitionError( std::string("An <algebra> element must be part of a <type>, which")
          + " includes a <combine-components-into-one-shape> element. See www.mantidproject.org/IDF." );
  }
  pNLalg->release();

  // check that there is a <location> element in type
  NodeList* pNL = pElem->getElementsByTagName("location");
  unsigned long numLocation = pNL->length();
  if ( numLocation == 0 )
  {
    throw Exception::InstrumentDefinitionError( std::string("At least one <location> element must be part of a <type>, which")
          + " includes a <combine-components-into-one-shape> element. See www.mantidproject.org/IDF." );
  }

  // to convert all <component>'s in type into <cuboid> elements, which are added
  // to pElem, and these <component>'s are deleted after loop

  std::set<Element*> allComponentInType; // used to hold <component>'s found
  std::vector<std::string> allLocationName; // used to check if loc names unique
  for (unsigned long i = 0; i < numLocation; i++)
  {

    Element* pLoc = static_cast<Element*>(pNL->item(i));

    // The location element is required to be a child of a component element. 
    // Get this component element
    Element* pCompElem = LoadInstrumentHelper::getParentComponent(pLoc);
    Element* pType = getTypeElement[pCompElem->getAttribute("type")];

    // get the type (name) of the <location> element in focus
    std::string locationTypeName = pType->getAttribute("name");

    // get the name given to the <location> element in focus
    // note these names are required to be unique for the purpose of 
    // constructing the <algebra>
    std::string locationElementName = pLoc->getAttribute("name");
    if ( std::find(allLocationName.begin(), allLocationName.end(), locationElementName) == allLocationName.end() )
      allLocationName.push_back(locationElementName);
    else
      throw Exception::InstrumentDefinitionError( std::string("Names in a <type> element containing ")
            + "a <combine-components-into-one-shape> element must be unique. "
            + "Here error is that " + locationElementName + " appears at least twice. See www.mantidproject.org/IDF." );

    if ( isTypeAssembly[locationTypeName] )
    {
      throw Exception::InstrumentDefinitionError( std::string("No <type> with name ")
            + locationTypeName + " in IDF. See www.mantidproject.org/IDF." );
    }

    // create dummy component to hold coord. sys. of cuboid
    CompAssembly *baseCoor = new CompAssembly("base");
    LoadInstrumentHelper::setLocation(baseCoor, pLoc, m_angleConvertConst); 

    std::string cuboidStr = translateRotateXMLcuboid(baseCoor, getTypeElement[locationTypeName], locationElementName);

  	DOMParser pParser;
  	Document* pDoc;
  	try
	  {
		  pDoc = pParser.parseString(cuboidStr);
	  }
	  catch(...)
	  {
      throw Exception::InstrumentDefinitionError( std::string("Unable to parse XML string ")
            + cuboidStr + " . Empty geometry Object is returned." );
	  }
	  // Get pointer to root element and add this element to pElem
	  Element* pCuboid = pDoc->documentElement();
    Node* fisse = (pElem->ownerDocument())->importNode(pCuboid, true);
    pElem->appendChild(fisse);

  	pDoc->release();
    allComponentInType.insert(pCompElem);
  }
  pNL->release();

  // delete all <component> found in pElem
  std::set<Element*>::iterator it;
  for ( it=allComponentInType.begin() ; it != allComponentInType.end(); it++ )
    pElem->removeChild(*it);
}

/// return absolute position of point which is set relative to the
/// coordinate system of the input component
/// @param comp Reference coordinate system
/// @param pos A position relative to the coord. sys. of comp
/// @return absolute position
V3D LoadInstCompsIntoOneShape::getAbsolutPositionInCompCoorSys(ICompAssembly* comp, V3D pos)
{
   Component *dummyComp = new Component("dummy", comp);
   comp->add(dummyComp);

   dummyComp->setPos(pos); // set pos relative to comp coord. sys.

   V3D retVal = dummyComp->getPos(); // get absolute position

   return retVal;
}


/// Returns a translated and rotated <cuboid> element  
/// @param comp Coordinate system to translate and rotate cuboid to
/// @param cuboidEle containing one <cuboid> element
/// @return XML string of translated and rotated cuboid
std::string LoadInstCompsIntoOneShape::translateRotateXMLcuboid(ICompAssembly* comp, Poco::XML::Element* cuboidEle, 
                                                                std::string& cuboidName)
{
  Element* pElem_lfb = getShapeElement(cuboidEle, "left-front-bottom-point"); 
  Element* pElem_lft = getShapeElement(cuboidEle, "left-front-top-point"); 
  Element* pElem_lbb = getShapeElement(cuboidEle, "left-back-bottom-point"); 
  Element* pElem_rfb = getShapeElement(cuboidEle, "right-front-bottom-point"); 

  V3D lfb = parsePosition(pElem_lfb);  // left front bottom
  V3D lft = parsePosition(pElem_lft);  // left front top
  V3D lbb = parsePosition(pElem_lbb);  // left back bottom
  V3D rfb = parsePosition(pElem_rfb);  // right front bottom

  // translate and rotate cuboid according to coord. sys. of comp
  V3D p_lfb = getAbsolutPositionInCompCoorSys(comp, lfb);
  V3D p_lft = getAbsolutPositionInCompCoorSys(comp, lft);
  V3D p_lbb = getAbsolutPositionInCompCoorSys(comp, lbb);
  V3D p_rfb = getAbsolutPositionInCompCoorSys(comp, rfb);

  // create output cuboid XML string
  std::ostringstream obj_str;

  obj_str << "<cuboid id=\"" << cuboidName << "\">";
  obj_str << "<left-front-bottom-point ";
  obj_str << "x=\""   <<p_lfb.X();
  obj_str << "\" y=\""<<p_lfb.Y();
  obj_str << "\" z=\""<<p_lfb.Z();
  obj_str << "\"  />";
  obj_str << "<left-front-top-point ";
  obj_str << "x=\""   <<p_lft.X();
  obj_str << "\" y=\""<<p_lft.Y();
  obj_str << "\" z=\""<<p_lft.Z();
  obj_str << "\"  />";
  obj_str << "<left-back-bottom-point ";
  obj_str << "x=\""   <<p_lbb.X();
  obj_str << "\" y=\""<<p_lbb.Y();
  obj_str << "\" z=\""<<p_lbb.Z();
  obj_str << "\"  />";
  obj_str << "<right-front-bottom-point ";
  obj_str << "x=\""   <<p_rfb.X();
  obj_str << "\" y=\""<<p_rfb.Y();
  obj_str << "\" z=\""<<p_rfb.Z();
  obj_str << "\"  />";
  obj_str << "</cuboid>";

  return obj_str.str();
}


/** Return a subelement of an XML element, but also checks that there exist exactly one entry
 *  of this subelement.
 *
 *  @param pElem :: XML from instrument def. file
 *  @param name :: Name of subelement 
 *  @return The subelement
 *
 *  @throw std::invalid_argument Thrown if issues with XML string
 */
Poco::XML::Element* LoadInstCompsIntoOneShape::getShapeElement(Poco::XML::Element* pElem, const std::string& name)
{
  // check if this shape element contain an element with name specified by the 2nd function argument
  Poco::AutoPtr<NodeList> pNL = pElem->getElementsByTagName(name);
  if ( pNL->length() != 1)
  {
    throw std::invalid_argument("XML element: <" + pElem->tagName() +
      "> must contain exactly one sub-element with name: <" + name + ">.");
  }
  Element* retVal = static_cast<Element*>(pNL->item(0));
  return retVal;
}


/** Get position coordinates from XML element
 *
 *  @param pElem :: XML element whose attributes contain position coordinates
 *  @return Position coordinates in the form of a V3D object 
 */
V3D LoadInstCompsIntoOneShape::parsePosition(Poco::XML::Element* pElem)
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
