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

// used to terminate SAX process
class DummyException {
public:
  std::string m_validFrom;
  std::string m_validTo;
  DummyException(std::string validFrom, std::string validTo) 
    : m_validFrom(validFrom), m_validTo(validTo) {}
};

// SAX content handler for grapping stuff quickly from IDF
class myContentHandler : public Poco::XML::ContentHandler
{
  virtual void startElement(
    const XMLString &,
    const XMLString & localName,
    const XMLString &,
    const Attributes & attrList
    ) 
  { 
    if ( localName == "instrument" )
    {
      throw DummyException(static_cast<std::string>(attrList.getValue("","valid-from")), 
        static_cast<std::string>(attrList.getValue("","valid-to")));
    }
  }
  virtual void endElement(
    const XMLString &,
    const XMLString &,
    const XMLString & 
    ) {}
  virtual void startDocument() {}
  virtual void endDocument() {}
  virtual void characters(
    const XMLChar [],
    int ,
    int 
    ) {}
  virtual void endPrefixMapping(
    const XMLString &
    ) {}
  virtual void ignorableWhitespace(
    const XMLChar [],
    int ,
    int 
    ) {}
  virtual void processingInstruction(
    const XMLString & ,
    const XMLString & 
    ) {}
  virtual void setDocumentLocator(
    const Locator *
    ) {}
  virtual void skippedEntity(
    const XMLString &
    ) {}
  virtual void startPrefixMapping(
    const XMLString & ,
    const XMLString & 
    ) {}
};


/** Return from an IDF the values of the valid-from and valid-to attributes 
*
*  @param IDFfilename :: Full path of an IDF
*  @param[out] outValidFrom :: Used to return valid-from date
*  @param[out] outValidTo :: Used to return valid-to date
*/
void LoadInstrumentHelper::getValidFromTo(const std::string& IDFfilename, std::string& outValidFrom,
  std::string& outValidTo)
{
      SAXParser pParser;
      // Create on stack to ensure deletion. Relies on pParser also being local variable.
      myContentHandler  conHand;
      pParser.setContentHandler(&conHand);

      try
      {
        pParser.parse(IDFfilename);
      }
      catch(DummyException &e)
      {
        outValidFrom = e.m_validFrom;
        outValidTo = e.m_validTo;
      }
      catch(...)
      {
        // should throw some sensible here
      }
}


/** Return workspace start date as an ISO 8601 string. If this info not stored in workspace the 
*   method returns current date.
*
*  @param workspace :: workspace to get information from
*  @return workspace start date as a string
*/
std::string LoadInstrumentHelper::getWorkspaceStartDate(const boost::shared_ptr<API::MatrixWorkspace>& workspace)
{
    const API::Run& runDetails = workspace->run();
    std::string date;
    if ( runDetails.hasProperty("run_start") )
    {
      date = runDetails.getProperty("run_start")->value();
    }
    else
    {
      g_log.information("run_start not stored in workspace. Default to current date.");

      date = Kernel::DateAndTime::get_current_time().to_ISO8601_string();
    }

    return date;
}

/** A given instrument may have multiple IDFs associated with it. This method return an 
*  identifier which identify a given IDF for a given instrument. An IDF filename is 
*  required to be of the form IDFname + _Definition + Identifier + .xml, the identifier
*  then is the part of a filename that identifies the IDF valid at a given date.
*
*  @param instrumentName :: Instrument name e.g. GEM, TOPAS or BIOSANS
*  @param date :: ISO 8601 date
*  @return full path of IDF
*/
std::string LoadInstrumentHelper::getInstrumentFilename(const std::string& instrumentName, const std::string& date)
{
  g_log.debug() << "Looking for instrment XML file for " << instrumentName << " that is valid on '" << date << "'\n";
  // Lookup the instrument (long) name
  std::string instrument(Kernel::ConfigService::Instance().getInstrument(instrumentName).name());

  // Get the search directory for XML instrument definition files (IDFs)
  std::string directoryName = Kernel::ConfigService::Instance().getInstrumentDirectory();

  Poco::RegularExpression regex(instrument+"_Definition.*\\.xml", Poco::RegularExpression::RE_CASELESS );
  Poco::DirectoryIterator end_iter;
  DateAndTime d(date);
  std::string mostRecentIDF; // store most recent IDF which is returned if no match for the date found
  DateAndTime refDate("1900-01-31 23:59:59"); // used to help determine the most recent IDF
  for ( Poco::DirectoryIterator dir_itr(directoryName); dir_itr != end_iter; ++dir_itr )
  {
    if ( !Poco::File(dir_itr->path() ).isFile() ) continue;

    std::string l_filenamePart = Poco::Path(dir_itr->path()).getFileName();
    if ( regex.match(l_filenamePart) )
    {
      g_log.debug() << "Found file: '" << dir_itr->path() << "'\n";
      std::string validFrom, validTo;
      LoadInstrumentHelper::getValidFromTo(dir_itr->path(), validFrom, validTo);
      g_log.debug() << "File '" << dir_itr->path() << " valid dates: from '" << validFrom << "' to '" << validTo << "'\n";
      DateAndTime from(validFrom);
      // Use a default valid-to date if none was found.
      DateAndTime to;
      if (validTo.length() > 0)
        to.set_from_ISO8601_string(validTo);
      else
        to.set_from_ISO8601_string("2100-01-01");

      if ( from <= d && d <= to )
      {
        return dir_itr->path();
      }
      if ( to > refDate )
      {
        refDate = to;
        mostRecentIDF = dir_itr->path();
      }
    }
  }

  // No date match found return most recent   
  return mostRecentIDF;
}

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

} // namespace DataHandling
} // namespace Mantid
