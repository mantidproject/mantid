//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/Instrument.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidGeometry/Detector.h"
#include "MantidGeometry/CompAssembly.h"
#include "MantidGeometry/Component.h"

#include <fstream>


#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"

using Poco::XML::DOMParser;
using Poco::XML::XMLReader;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Node;
using Poco::XML::NodeList;


namespace Mantid
{
namespace DataHandling
{

DECLARE_ALGORITHM(LoadInstrument)

using namespace Kernel;
using namespace API;

Logger& LoadInstrument::g_log = Logger::get("LoadInstrument");

/// Empty default constructor
LoadInstrument::LoadInstrument()
{}

/// Initialisation method.
void LoadInstrument::init()
{
  // When used as a sub-algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
  declareProperty(new WorkspaceProperty<Workspace>("Workspace","Anonymous",Direction::InOut));
  declareProperty("Filename","",new MandatoryValidator);
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 * 
 *  @throw FileError Thrown if unable to parse XML file
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
 */
void LoadInstrument::exec()
{
  // Retrieve the filename from the properties
  m_filename = getPropertyValue("Filename");

  // Get the input workspace
  const Workspace_sptr localWorkspace = getProperty("Workspace");


  // Set up the DOM parser and parse xml file

  DOMParser pParser; 
  Document* pDoc;
  try 
  {
    pDoc = pParser.parse(m_filename);
  }
  catch(...)
  {
    g_log.error("Unable to XML parse file " + m_filename);
    throw Kernel::Exception::FileError("Unable to XML parse File:" , m_filename);	      
  }


  // Get pointer to root element

  Element* pRootElem = pDoc->documentElement();

  if ( !pRootElem->hasChildNodes() )
  {
    g_log.error("XML file: " + m_filename + "contains no root element.");
    throw Kernel::Exception::InstrumentDefinitionError("No root element in XML instrument file", m_filename);	 
  }


  // create 'hash' tables for the type elements

  NodeList* pNL_type = pRootElem->getElementsByTagName("type");

  if ( pNL_type->length() == 0 )
  {
    g_log.error("XML file: " + m_filename + "contains no type elements.");
    throw Kernel::Exception::InstrumentDefinitionError("No type elements in XML instrument file", m_filename);	
  }

  for (unsigned int iType = 0; iType < pNL_type->length(); iType++)
  {  
    Element* pTypeElem = static_cast<Element*>(pNL_type->item(iType));
    std::string typeName = pTypeElem->getAttribute("name");
    getTypeElement[typeName] = pTypeElem;

    // identify for now a type to be an assemble by it containing elements
    // with tag name 'component'
    NodeList* pNL_local = pTypeElem->getElementsByTagName("component");
    if (pNL_local->length() == 0)
      isTypeAssemply[typeName] = false;
    else
      isTypeAssemply[typeName] = true;
    pNL_local->release();
  }
  pNL_type->release();


  // Get reference to Instrument and set its name

  instrument = &(localWorkspace->getInstrument());
  if ( pRootElem->hasAttribute("name") ) 
    instrument->setName( pRootElem->getAttribute("name") );


  // do analysis for each top level compoment element

  NodeList* pNL_comp = pRootElem->childNodes(); // here get all child nodes

  for (unsigned int i = 0; i < pNL_comp->length(); i++)
  {
    // we are only interest in the top level component elements hence
    // the reason for the if statement below

    if ( (pNL_comp->item(i))->nodeType() == Node::ELEMENT_NODE && 
        ((pNL_comp->item(i))->nodeName()).compare("component") == 0 ) 
    {
      Element* pElem = static_cast<Element*>(pNL_comp->item(i));

      int runningDetID = 1, detectorLastID = 0; // Used for assigning detector IDs
                                                // The initialisation is to make a
                                                // check below to be true by default

      // get all location element contained in component element

      NodeList* pNL_location = pElem->getElementsByTagName("location");
      if (pNL_location->length() == 0)
      {
        g_log.error(std::string("All component elements must contain at least one location element") +
          " even it is just an empty location element of the form <location />");
        throw Kernel::Exception::InstrumentDefinitionError(
          std::string("All component elements must contain at least one location element") +
          " even it is just an empty location element of the form <location />", m_filename);
      }

      if ( isTypeAssemply[pElem->getAttribute("type")] )
      {
        // read detertor ID start and end values if idlist specified
        // (only for 'detector' elements at present)

        if ( pElem->hasAttribute("idlist") )
        {
          std::string idlist = pElem->getAttribute("idlist");
          Element* pFound = pDoc->getElementById(idlist, "idname");
          runningDetID = atoi( (pFound->getAttribute("start")).c_str() ); 
          detectorLastID = atoi( (pFound->getAttribute("end")).c_str() ); 
        }


        for (unsigned int i_loc = 0; i_loc < pNL_location->length(); i_loc++)
        {
          appendAssembly(instrument, static_cast<Element*>(pNL_location->item(i_loc)), runningDetID);
        }
        

        // a check

        if (runningDetID != detectorLastID+1)
        {
          g_log.error("The number of detector IDs listed in idlist named "
            + pElem->getAttribute("idlist") + 
            " is not equal to the number of detectors listed in type = "
            + pElem->getAttribute("type")); 
          throw Kernel::Exception::InstrumentDefinitionError(
            "Number of IDs listed in idlist does not match number of detectors listed in type = "
            + pElem->getAttribute("type"), m_filename);
        }
      }
      else
      {
        for (unsigned int i_loc = 0; i_loc < pNL_location->length(); i_loc++)
        {
          appendLeaf(instrument, static_cast<Element*>(pNL_location->item(i_loc)), runningDetID);
        }
      }
      pNL_location->release();
    }
  }

  pNL_comp->release();
  pDoc->release();

  return;
}

/** Assumes second argument is a XML location element and its parent is a component element
 *  which is assigned to be an assemble. This method appends the parent component element of
 %  the location element to the CompAssemply passed as the 1st arg. Note this method may call 
 %  itself, i.e. it may act recursively.
 *
 *  @param parent CompAssembly to append new component to
 *  @param pLocElem  Poco::XML element that points to a location element in an instrument description XML file
 *  @param runningDetID Detector ID, which may be incremented if appendLeave is called
 */
void LoadInstrument::appendAssembly(Geometry::CompAssembly* parent, Poco::XML::Element* pLocElem, int& runningDetID)
{
  // The location element is required to be a child of a component element. Get this component element

  Element* pCompElem = getParentComponent(pLocElem);


  Geometry::CompAssembly *ass = new Geometry::CompAssembly;
  ass->setParent(parent);
  parent->add(ass);


  // set name of newly added component assembly.

  if ( pLocElem->hasAttribute("name") )
    ass->setName(pLocElem->getAttribute("name"));
  else if ( pCompElem->hasAttribute("name") )
  {
    ass->setName(pCompElem->getAttribute("name"));
  }
  else
  {
    ass->setName(pCompElem->getAttribute("type"));
  }

  // set location for this newly added comp. 

  setLocation(ass, pLocElem);


  // The newly added component is required to have a type. Find out what this
  // type is and find all the location elements of this type. Finally loop over these
  // location elements

  Element* pType = getTypeElement[pCompElem->getAttribute("type")];
  NodeList* pNL_loc_for_this_type = pType->getElementsByTagName("location");

  for (unsigned int i = 0; i < pNL_loc_for_this_type->length(); i++)
  {
    Element* pElem = static_cast<Element*>(pNL_loc_for_this_type->item(i));
    std::string typeName = (getParentComponent(pElem))->getAttribute("type");
    if (isTypeAssemply[typeName])
    {
      appendAssembly(ass, pElem, runningDetID);
    }
    else
    {
      appendLeaf(ass, pElem, runningDetID);
    }
  }
  pNL_loc_for_this_type->release();
}


/** Assumes second argument is pointing to a leaf, which here mean location element (indirectly 
 *  representing a component element) that contains no sub-components. This component is appended 
 %  to the parent (1st argument). 
 *
 *  @param parent CompAssembly to append component to
 *  @param pLocElem  Poco::XML element that points to the element in the XML doc we want to add  
 *  @param runningDetID Detector ID, which may be incremented if appendLeave is called
 */
void LoadInstrument::appendLeaf(Geometry::CompAssembly* parent, Poco::XML::Element* pLocElem, int& runningDetID)
{
  // The location element is required to be a child of a component element. Get this component element

  Element* pCompElem = getParentComponent(pLocElem);


  // get the type element of the component element in order to determine if the type
  // belong to the catogory: "detector", "SamplePos or "Source".

  Element* pType = getTypeElement[pCompElem->getAttribute("type")];

  std::string category = ""; 
  if (pType->hasAttribute("is")) 
    category = pType->getAttribute("is");


  // do stuff a bit differently depending on which category the type belong to

  if ( category.compare("detector") == 0 )
  {
    Geometry::Detector detector;

    if ( pLocElem->hasAttribute("name") )
      detector.setName(pLocElem->getAttribute("name"));
    else if ( pCompElem->hasAttribute("name") )
    {
      detector.setName(pCompElem->getAttribute("name"));
    }
    else
    {
      detector.setName(pCompElem->getAttribute("type"));
    }

    // set location for this comp

    setLocation(&detector, pLocElem);


    // set detector ID and increment it. Finally add the detector to the parent
    detector.setID(runningDetID);
    runningDetID++;
    int toGetHoldOfDetectorCopy = parent->addCopy(&detector);
    Geometry::Detector* temp = dynamic_cast<Geometry::Detector*>((*parent)[toGetHoldOfDetectorCopy-1]);
    instrument->markAsDetector(temp);
  }
  else
  {
    Geometry::ObjComponent *comp = new Geometry::ObjComponent;

    comp->setParent(parent);
    parent->add(comp);

    if ( pLocElem->hasAttribute("name") )
      comp->setName(pLocElem->getAttribute("name"));
    else if ( pCompElem->hasAttribute("name") )
    {
      comp->setName(pCompElem->getAttribute("name"));
    }
    else
    {
      comp->setName(pCompElem->getAttribute("type"));
    }

    // check if special Source or SamplePos Component
    if ( category.compare("Source") == 0 )
    {
      instrument->markAsSource(comp);
    }
    if ( category.compare("SamplePos") == 0 )
    {
      instrument->markAsSamplePos(comp);
    }

    // set location for this comp

    setLocation(comp, pLocElem);
  }
}



/** Set location (position) of comp as specified in XML location element.
 *
 *  @param comp To set position/location off
 *  @param pElem  Poco::XML element that points a location element in the XML doc  
 *
 *  @throw logic_error Thrown if second argument is not a pointer to component XML element
 */
void LoadInstrument::setLocation(Geometry::Component* comp, Poco::XML::Element* pElem)
{
  // Require that pElem points to an element with tag name 'location'

  if ( (pElem->tagName()).compare("location") )
  {
    g_log.error("Second argument to function setLocation must be a pointer to an XML element with tag name location.");
    throw std::logic_error( "Second argument to function setLocation must be a pointer to an XML element with tag name location." );
  }


  if ( pElem->hasAttribute("R") || pElem->hasAttribute("theta") || pElem->hasAttribute("phi") )
  {
    double R=0.0, theta=0.0, phi=0.0;

    if ( pElem->hasAttribute("R") ) R = atof((pElem->getAttribute("R")).c_str());
    if ( pElem->hasAttribute("theta") ) theta = atof((pElem->getAttribute("theta")).c_str());
    if ( pElem->hasAttribute("phi") ) phi = atof((pElem->getAttribute("phi")).c_str());    

    Geometry::V3D pos;
    pos.spherical(R,theta,phi);
    comp->setPos(pos);
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

    Geometry::V3D pos;
    pos.spherical(R,theta,phi);
    comp->setPos(pos);
  }
  else
  {
    double x=0.0, y=0.0, z=0.0;

    if ( pElem->hasAttribute("x") ) x = atof((pElem->getAttribute("x")).c_str());
    if ( pElem->hasAttribute("y") ) y = atof((pElem->getAttribute("y")).c_str());
    if ( pElem->hasAttribute("z") ) z = atof((pElem->getAttribute("z")).c_str());
    
    comp->setPos(Geometry::V3D(x,y,z));
  }
}

/** Get parent component element of location element.
 *
 *  @param pLocElem  Poco::XML element that points a location element in the XML doc  
 *
 *  @throw logic_error Thrown if argument is not a child of component element
 */
Poco::XML::Element* LoadInstrument::getParentComponent(Poco::XML::Element* pLocElem)
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
