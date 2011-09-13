//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadInstrumentHelper.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/OneToOneSpectraDetectorMap.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/Exception.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/RegularExpression.h>
#include <Poco/DirectoryIterator.h>

#include <Poco/SAX/ContentHandler.h>
#include <Poco/SAX/SAXParser.h>
#include <Poco/SAX/Attributes.h>


namespace Mantid
{
namespace DataHandling
{
  using Kernel::DateAndTime;
  using Kernel::ConfigService;
  using Kernel::Logger;
  using Geometry::Instrument;
  using Geometry::OneToOneSpectraDetectorMap;
  using API::MatrixWorkspace;
  using API::Run;
  using API::SpectraDetectorMap;
  using Poco::XML::XMLString;
  using Poco::XML::Attributes;
  using Poco::XML::SAXParser;
  using Poco::XML::XMLChar;
  using Poco::XML::Locator;
  using Poco::XML::DOMParser;
  using Poco::XML::Document;
  using Poco::XML::Element;
  using Poco::XML::Node;
  using Poco::XML::NodeList;
  using Poco::XML::NodeIterator;
  using Poco::XML::NodeFilter;

Logger& LoadInstrumentHelper::g_log = Logger::get("LoadInstrumentHelper");



// Map to store positions of parent components in spherical coordinates
std::map<const Geometry::IComponent*,SphVec> LoadInstrumentHelper::m_tempPosHolder;

    //-----------------------------------------------------------------------------------------------------------------------
    /** Set location (position) of comp as specified in XML location element.
    *
    *  @param comp :: To set position/location off
    *  @param pElem ::  Poco::XML element that points a location element in the XML doc
    *
    *  @throw logic_error Thrown if second argument is not a pointer to a 'location' XML element
    */
void LoadInstrumentHelper::setLocation(Geometry::IComponent* comp, Poco::XML::Element* pElem,
                                  const double angleConvertConst, const bool deltaOffsets)
{
  // Require that pElem points to an element with tag name 'location' or 'neutronic'
  if ( pElem->tagName() != "location" && pElem->tagName() != "neutronic" )
  {
    g_log.error("Second argument to function setLocation must be a pointer to an XML element with tag name location.");
    throw std::logic_error( "Second argument to function setLocation must be a pointer to an XML element with tag name location." );
  }

  comp->setPos(LoadInstrumentHelper::getRelativeTranslation(comp, pElem, angleConvertConst, deltaOffsets));

  // Rotate coordinate system of this component
  if ( pElem->hasAttribute("rot") )
  {
    double rotAngle = angleConvertConst*atof( (pElem->getAttribute("rot")).c_str() ); // assumed to be in degrees

    double axis_x = 0.0;
    double axis_y = 0.0;
    double axis_z = 1.0;

    if ( pElem->hasAttribute("axis-x") )
      axis_x = atof( (pElem->getAttribute("axis-x")).c_str() );
    if ( pElem->hasAttribute("axis-y") )
      axis_y = atof( (pElem->getAttribute("axis-y")).c_str() );
    if ( pElem->hasAttribute("axis-z") )
      axis_z = atof( (pElem->getAttribute("axis-z")).c_str() );

    comp->rotate(Kernel::Quat(rotAngle, Kernel::V3D(axis_x,axis_y,axis_z)));
  }


  // Check if sub-elements <trans> or <rot> of present - for now ignore these if m_deltaOffset = true

  Element* pRecursive = pElem;
  bool stillTransElement = true;
  while ( stillTransElement )
  {
      // figure out if child element is <trans> or <rot> or none of these

      Element* tElem = pRecursive->getChildElement("trans");
      Element* rElem = pRecursive->getChildElement("rot");

      if (tElem && rElem)
      {
      // if both a <trans> and <rot> child element present. Ignore <rot> element
      rElem = NULL;
      }

      if (!tElem && !rElem)
      {
      stillTransElement = false;
      }

      Kernel::V3D posTrans;

      if (tElem)
      {
          posTrans = LoadInstrumentHelper::getRelativeTranslation(comp, tElem, angleConvertConst, deltaOffsets);

      // to get the change in translation relative to current rotation of comp
      Geometry::CompAssembly compToGetRot;
      Geometry::CompAssembly compRot;
      compRot.setRot(comp->getRotation());
      compToGetRot.setParent(&compRot);
      compToGetRot.setPos(posTrans);

      // Apply translation
      comp->translate(compToGetRot.getPos());   

      // for recursive action
      pRecursive = tElem; 
      }  // end translation

      if (rElem) 
      {
      double rotAngle = angleConvertConst*atof( (rElem->getAttribute("val")).c_str() ); // assumed to be in degrees

      double axis_x = 0.0;
      double axis_y = 0.0;
      double axis_z = 1.0;

      if ( rElem->hasAttribute("axis-x") )
          axis_x = atof( (rElem->getAttribute("axis-x")).c_str() );
      if ( rElem->hasAttribute("axis-y") )
          axis_y = atof( (rElem->getAttribute("axis-y")).c_str() );
      if ( rElem->hasAttribute("axis-z") )
          axis_z = atof( (rElem->getAttribute("axis-z")).c_str() );

      comp->rotate(Kernel::Quat(rotAngle, Kernel::V3D(axis_x,axis_y,axis_z)));      

      // for recursive action
      pRecursive = rElem; 
      }

  } // end while
}


//-----------------------------------------------------------------------------------------------------------------------
/** Calculate the position of comp relative to its parent from info provided by \<location\> element.
*
*  @param comp :: To set position/location off
*  @param pElem ::  Poco::XML element that points a location element in the XML doc
*
*  @return  Thrown if second argument is not a pointer to a 'location' XML element
*/
Kernel::V3D LoadInstrumentHelper::getRelativeTranslation(const Geometry::IComponent* comp, const Poco::XML::Element* pElem,
                                  const double angleConvertConst, const bool deltaOffsets)
{
  Kernel::V3D retVal;  // position relative to parent

  // Polar coordinates can be labelled as (r,t,p) or (R,theta,phi)
  if ( pElem->hasAttribute("r") || pElem->hasAttribute("t") || pElem->hasAttribute("p") ||
    pElem->hasAttribute("R") || pElem->hasAttribute("theta") || pElem->hasAttribute("phi") )
  {
    double R=0.0, theta=0.0, phi=0.0;

    if ( pElem->hasAttribute("r") ) R = atof((pElem->getAttribute("r")).c_str());
    if ( pElem->hasAttribute("t") ) theta = angleConvertConst*atof((pElem->getAttribute("t")).c_str());
    if ( pElem->hasAttribute("p") ) phi = angleConvertConst*atof((pElem->getAttribute("p")).c_str());

    if ( pElem->hasAttribute("R") ) R = atof((pElem->getAttribute("R")).c_str());
    if ( pElem->hasAttribute("theta") ) theta = angleConvertConst*atof((pElem->getAttribute("theta")).c_str());
    if ( pElem->hasAttribute("phi") ) phi = angleConvertConst*atof((pElem->getAttribute("phi")).c_str());

    if ( deltaOffsets )
    {
      // In this case, locations given are radial offsets to the (radial) position of the parent,
      // so need to do some extra calculation before they're stored internally as x,y,z offsets.

      // Temporary vector to hold the parent's absolute position (will be 0,0,0 if no parent)
      Kernel::V3D parentPos;
      // Get the parent's absolute position (if the component has a parent)
      if ( comp->getParent() )
      {
        std::map<const Geometry::IComponent*, SphVec>::iterator it;
        it = LoadInstrumentHelper::m_tempPosHolder.find(comp);
        SphVec parent;
        if ( it == LoadInstrumentHelper::m_tempPosHolder.end() )
          parent = LoadInstrumentHelper::m_tempPosHolder[comp->getParent().get()];
        else
          parent = it->second;

        // Add to the current component to get its absolute position
        R     += parent.r;
        theta += parent.theta;
        phi   += parent.phi;
        // Set the temporary V3D with the parent's absolute position
        parentPos.spherical(parent.r,parent.theta,parent.phi);
      }

      // Create a temporary vector that holds the absolute r,theta,phi position
      // Needed to make things work in situation when a parent object has a phi value but a theta of zero
      SphVec tmp(R,theta,phi);
      // Add it to the map with the pointer to the Component object as key
      LoadInstrumentHelper::m_tempPosHolder[comp] = tmp;

      // Create a V3D and set its position to be the child's absolute position
      Kernel::V3D absPos;
      absPos.spherical(R,theta,phi);

      // Subtract the two V3D's to get what we want (child's relative position in x,y,z)
      retVal = absPos - parentPos;
    }
    else
    {
      // In this case, the value given represents a vector from the parent to the child
      retVal.spherical(R,theta,phi);
    }

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

//-----------------------------------------------------------------------------------------------------------------------
/** Get parent component element of location element.
*
*  @param pLocElem ::  Poco::XML element that points a location element in the XML doc
*  @return Parent XML element to a location XML element
*
*  @throw logic_error Thrown if argument is not a child of component element
*/
Poco::XML::Element* LoadInstrumentHelper::getParentComponent(Poco::XML::Element* pLocElem)
{
  if ( (pLocElem->tagName()).compare("location") )
  {
    g_log.error("Argument to function getParentComponent must be a pointer to an XML element with tag name location.");
    throw std::logic_error( "Argument to function getParentComponent must be a pointer to an XML element with tag name location." );
  }

  // The location element is required to be a child of a component element. Get this component element

  Node* pCompNode = pLocElem->parentNode();

  Element* pCompElem;
  if (pCompNode->nodeType() == 1)
  {
    pCompElem = static_cast<Element*>(pCompNode);
    if ( (pCompElem->tagName()).compare("component") )
    {
      g_log.error("Argument to function getParentComponent must be a XML element sitting inside a component element.");
      throw std::logic_error( "Argument to function getParentComponent must be a XML element sitting inside a component element." );
    }
  }
  else
  {
    g_log.error("Argument to function getParentComponent must be a XML element whos parent is an element.");
    throw std::logic_error( "Argument to function getParentComponent must be a XML element whos parent is an element." );
  }

  return pCompElem;
}

//-----------------------------------------------------------------------------------------------------------------------
/** get name of location element. Will be the name attribute, or the
  * parent's name attribute, or the parent's type, if all else fails.
*
*  @param pElem ::  Poco::XML element that points to a location element
*  @return name of location element
*/
std::string LoadInstrumentHelper::getNameOfLocationElement(Poco::XML::Element* pElem)
{
  Element* pCompElem = LoadInstrumentHelper::getParentComponent(pElem);

  std::string retVal;

  if ( pElem->hasAttribute("name") )
    retVal = pElem->getAttribute("name");
  else if ( pCompElem->hasAttribute("name") )
  {
    retVal = pCompElem->getAttribute("name");
  }
  else
  {
    retVal = pCompElem->getAttribute("type");
  }

  return retVal;
}

} // namespace DataHandling
} // namespace Mantid
