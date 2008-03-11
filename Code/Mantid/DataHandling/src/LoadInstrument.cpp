//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/Instrument.h"

#include "Detector.h"
#include "CompAssembly.h"
#include "Component.h"

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

  for (int iType = 0; iType < pNL_type->length(); iType++)
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

  API::Instrument& instrument = localWorkspace->getInstrument();
  if ( pRootElem->hasAttribute("name") ) 
    instrument.setName( pRootElem->getAttribute("name") );


  // do analysis for each top level compoment element

  NodeList* pNL_comp = pRootElem->childNodes(); // here get all child nodes

  for (int i = 0; i < pNL_comp->length(); i++)
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

        appendAssembly(&instrument, pElem, runningDetID);


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
        appendLeaf(&instrument, pElem, runningDetID);
      }
    }
  }

  pNL_comp->release();

  return;
}

/** Assumes second argument is pointing to an assemble and this method appends 
 *  it to the parent passed as the 1st arg. Note this method may call itself, 
 *  i.e. it may act recursively.
 *
 *  @param parent Parent to append assemble to
 *  @param pElem  Poco::XML element that points to the element in the XML doc we want to add  
 *  @return runningDetID Detector ID, which may be incremented if appendLeave is called
 *
 *  @throw logic_error Thrown if second argument is not a pointer to component XML element
 *  @throw NotImplementedError At present a component element is restricted to have a max of one location element
 */
void LoadInstrument::appendAssembly(Geometry::CompAssembly* parent, Element* pCompElem, int& runningDetID)
{
  // for now assume that the tag name of the element is 'component'

  if ( (pCompElem->tagName()).compare("component") )
  {
    g_log.error("Second argument to function appendAssembly must be a pointer to an XML element with tag name component.");
    throw std::logic_error( "Second argument to function appendAssembly must be a pointer to an XML element with tag name component." );
  }

  Geometry::CompAssembly *ass = new Geometry::CompAssembly;
  ass->setParent(parent);
  parent->add(ass);

  if ( pCompElem->hasAttribute("name") )
    ass->setName(pCompElem->getAttribute("name"));
  else
    ass->setName(pCompElem->getAttribute("type"));


  // set location for this comp. Done this way because are likely to need
  // to take into account the fact that a component element may contain more
  // than one location element

  NodeList* pNL_location = pCompElem->getElementsByTagName("location");
  if (pNL_location->length() == 1)
    setLocation(ass, static_cast<Element*>(pNL_location->item(0)));
  else
    throw Kernel::Exception::NotImplementedError("component element restricted to contain one location element");
  pNL_location->release();


  // find all the component elements of this type element and loop over these

  Element* pType = getTypeElement[pCompElem->getAttribute("type")];
  NodeList* pNL_comp_for_this_type = pType->getElementsByTagName("component");

  for (int i = 0; i < pNL_comp_for_this_type->length(); i++)
  {
    Element* pElem = static_cast<Element*>(pNL_comp_for_this_type->item(i));
    std::string typeName = pElem->getAttribute("type");
    if (isTypeAssemply[typeName])
      appendAssembly(ass, pElem, runningDetID);
    else
      appendLeaf(ass, pElem, runningDetID);
  }
  pNL_comp_for_this_type->release();
}


/** Assumes second argument is pointing to a leaf, which here mean component element that
 *  contains no sub-components. This component is appended to the parent (1st argument). 
 *
 *  @param parent Parent to append (none-assemble) component to
 *  @param pElem  Poco::XML element that points to the element in the XML doc we want to add  
 *  @return runningDetID Detector ID, which may be incremented if appendLeave is called
 *
 *  @throw NotImplementedError At present a component element is restricted to have a max of one location element
 */
void LoadInstrument::appendLeaf(Geometry::CompAssembly* parent, Element* pCompElem, int& runningDetID)
{
  Element* pType = getTypeElement[pCompElem->getAttribute("type")];

  std::string typeName = ""; 
  if (pType->hasAttribute("is")) 
    typeName = pType->getAttribute("is");

  if ( typeName.compare("detector") == 0 )
  {
    Geometry::Detector detector;

    if ( pCompElem->hasAttribute("name") )
      detector.setName(pCompElem->getAttribute("name"));
    else
      detector.setName(pCompElem->getAttribute("type"));

    // set location for this comp. Done this way because are likely to need
    // to take into account the fact that a component element may contain more
    // than one location element

    NodeList* pNL_location = pCompElem->getElementsByTagName("location");
    if (pNL_location->length() == 1)
      setLocation(&detector, static_cast<Element*>(pNL_location->item(0)));
    else
      throw Kernel::Exception::NotImplementedError("component element restricted to contain one location element");
    pNL_location->release();


    // set detector ID and increment it. Finally add the detector to the parent

    detector.setID(runningDetID);
    runningDetID++;
    parent->addCopy(&detector);
  }
  else
  {
    Geometry::ObjComponent *comp = new Geometry::ObjComponent;

    comp->setParent(parent);
    parent->add(comp);

    if ( pCompElem->hasAttribute("name") )
      comp->setName(pCompElem->getAttribute("name"));
    else
      comp->setName(pCompElem->getAttribute("type"));

    // set location for this comp. Done this way because are likely to need
    // to take into account the fact that a component element may contain more
    // than one location element

    NodeList* pNL_location = pCompElem->getElementsByTagName("location");
    if (pNL_location->length() == 1)
      setLocation(comp, static_cast<Element*>(pNL_location->item(0)));
    else
      throw Kernel::Exception::NotImplementedError("component element restricted to contain one location element");
    pNL_location->release();
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


  if ( pElem->hasAttribute("R") || pElem->hasAttribute("theta") || 
    pElem->hasAttribute("phi") )
  {
    double R=0.0, theta=0.0, phi=0.0;

    if ( pElem->hasAttribute("R") )
      R = atof((pElem->getAttribute("R")).c_str());
    if ( pElem->hasAttribute("theta") )
      theta = atof((pElem->getAttribute("theta")).c_str());
    if ( pElem->hasAttribute("phi") )
      phi = atof((pElem->getAttribute("phi")).c_str());    

    Geometry::V3D pos;
    pos.spherical(R,theta,phi);
    comp->setPos(pos);
  }
  else
  {
    double x=0.0, y=0.0, z=0.0;

    if ( pElem->hasAttribute("x") )
      x = atof((pElem->getAttribute("x")).c_str());
    if ( pElem->hasAttribute("y") )
      y = atof((pElem->getAttribute("y")).c_str());
    if ( pElem->hasAttribute("z") )
      z = atof((pElem->getAttribute("z")).c_str());
    
    comp->setPos(Geometry::V3D(x,y,z));
  }
}


} // namespace DataHandling
} // namespace Mantid
