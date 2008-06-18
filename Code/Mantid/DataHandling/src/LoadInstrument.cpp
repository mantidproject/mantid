//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadInstrument.h"
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

  unsigned int numberTypes = pNL_type->length();
  for (unsigned int iType = 0; iType < numberTypes; iType++)
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

  instrument = (localWorkspace->getInstrument());
  if ( pRootElem->hasAttribute("name") ) 
    instrument->setName( pRootElem->getAttribute("name") );


  // do analysis for each top level compoment element

  NodeList* pNL_comp = pRootElem->childNodes(); // here get all child nodes
  unsigned int pNL_comp_length = pNL_comp->length();
  for (unsigned int i = 0; i < pNL_comp_length; i++)
  {
    // we are only interest in the top level component elements hence
    // the reason for the if statement below

    if ( (pNL_comp->item(i))->nodeType() == Node::ELEMENT_NODE && 
        ((pNL_comp->item(i))->nodeName()).compare("component") == 0 ) 
    {
      Element* pElem = static_cast<Element*>(pNL_comp->item(i));

      IdList idList; // structure to possibly be populated with detector IDs 

      // get all location element contained in component element

      NodeList* pNL_location = pElem->getElementsByTagName("location");
      unsigned int pNL_location_length = pNL_location->length();
      if (pNL_location_length == 0)
      {
        g_log.error(std::string("All component elements must contain at least one location element") +
          " even it is just an empty location element of the form <location />");
        throw Kernel::Exception::InstrumentDefinitionError(
          std::string("All component elements must contain at least one location element") +
          " even it is just an empty location element of the form <location />", m_filename);
      }

      if ( isAssemply(pElem->getAttribute("type")) )
      {
        // read detertor IDs into idlist if required

        if ( pElem->hasAttribute("idlist") )
        {
          std::string idlist = pElem->getAttribute("idlist");
          Element* pFound = pDoc->getElementById(idlist, "idname");
          populateIdList(pFound, idList);
        }


        for (unsigned int i_loc = 0; i_loc < pNL_location_length; i_loc++)
        {
          appendAssembly(instrument, static_cast<Element*>(pNL_location->item(i_loc)), idList);
        }
        

        // a check

        if (idList.counted != static_cast<int>(idList.vec.size()) )
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
        for (unsigned int i_loc = 0; i_loc < pNL_location_length; i_loc++)
        {
          appendLeaf(instrument, static_cast<Element*>(pNL_location->item(i_loc)), idList);
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
 *  @param idList The current IDList
 */
void LoadInstrument::appendAssembly(Geometry::CompAssembly* parent, Poco::XML::Element* pLocElem, IdList& idList)
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
	NodeIterator it(pType, NodeFilter::SHOW_ELEMENT);

  Node* pNode = it.nextNode();
  while (pNode)
  {
    if ( pNode->nodeName().compare("location")==0 )
    {
      Element* pElem = static_cast<Element*>(pNode);

      std::string typeName = (getParentComponent(pElem))->getAttribute("type");

      if ( isAssemply(typeName) )
        appendAssembly(ass, pElem, idList);
      else
        appendLeaf(ass, pElem, idList);      
    }

    pNode = it.nextNode();
  }
}

void LoadInstrument::appendAssembly(boost::shared_ptr<Geometry::CompAssembly> parent, Poco::XML::Element* pLocElem, IdList& idList)
{
  appendAssembly(parent.get(), pLocElem, idList);
}


/** Assumes second argument is pointing to a leaf, which here mean location element (indirectly 
 *  representing a component element) that contains no sub-components. This component is appended 
 %  to the parent (1st argument). 
 *
 *  @param parent CompAssembly to append component to
 *  @param pLocElem  Poco::XML element that points to the element in the XML doc we want to add  
 *  @param idList The current IDList
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
 */
void LoadInstrument::appendLeaf(Geometry::CompAssembly* parent, Poco::XML::Element* pLocElem, IdList& idList)
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
    Geometry::Detector* detector = new Geometry::Detector;

    if ( pLocElem->hasAttribute("name") )
      detector->setName(pLocElem->getAttribute("name"));
    else if ( pCompElem->hasAttribute("name") )
    {
      detector->setName(pCompElem->getAttribute("name"));
    }
    else
    {
      detector->setName(pCompElem->getAttribute("type"));
    }

    // set location for this comp

    setLocation(detector, pLocElem);


    // set detector ID and increment it. Finally add the detector to the parent
    detector->setID(idList.vec[idList.counted]);
    idList.counted++;
    parent->add(detector);
    try
    {
      instrument->markAsDetector(detector);
    }
    catch(Kernel::Exception::ExistsError& e)
    { 
      std::stringstream convert;
      convert << detector->getID();
      throw Kernel::Exception::InstrumentDefinitionError("Detector with ID = " + convert.str() +
        " present more then once in XML instrument file", m_filename);	
    }
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

void LoadInstrument::appendLeaf(boost::shared_ptr<Geometry::CompAssembly> parent, Poco::XML::Element* pLocElem, IdList& idList)
{
	appendLeaf(parent.get(), pLocElem, idList);
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

/** Method for populating IdList.
 *
 *  @param pE  Poco::XML element that points a idlist element in the XML doc
 *  @param idList The structure to populate with detector ID numbers
 *
 *  @throw logic_error Thrown if argument is not a child of component element
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
 */
void LoadInstrument::populateIdList(Poco::XML::Element* pE, IdList& idList)
{
  if ( (pE->tagName()).compare("idlist") )
  {
    g_log.error("Argument to function createIdList must be a pointer to an XML element with tag name idlist.");
    throw std::logic_error( "Argument to function createIdList must be a pointer to an XML element with tag name idlist." );
  }

  // If idname element has start and end attributes then just use those to populate idlist.
  // Otherwise id sub-elements

  if ( pE->hasAttribute("start") )
  {
    int startID = atoi( (pE->getAttribute("start")).c_str() ); 

    int endID;
    if ( pE->hasAttribute("end") )
      endID = atoi( (pE->getAttribute("end")).c_str() ); 
    else
      endID = startID;

    for (int i = startID; i <= endID; i++)
      idList.vec.push_back(i);
  }
  else
  {
    // test first if any id elements

    NodeList* pNL = pE->getElementsByTagName("id");

    if ( pNL->length() == 0 )
    {
      throw Kernel::Exception::InstrumentDefinitionError("No id subelement of idlist element in XML instrument file", m_filename);	
    }

    pNL->release();


    // get id numbers

	  NodeIterator it(pE, NodeFilter::SHOW_ELEMENT);

    Node* pNode = it.nextNode();
    while (pNode)
    {
      if ( pNode->nodeName().compare("id")==0 )
      {
        Element* pIDElem = static_cast<Element*>(pNode);

        if ( pIDElem->hasAttribute("val") )
        {
          int valID = atoi( (pIDElem->getAttribute("start")).c_str() ); 
          idList.vec.push_back(valID);
        }
        else if ( pIDElem->hasAttribute("start") )
        {
          int startID = atoi( (pIDElem->getAttribute("start")).c_str() ); 

          int endID;
          if ( pIDElem->hasAttribute("end") )
            endID = atoi( (pIDElem->getAttribute("end")).c_str() ); 
          else
            endID = startID;

          for (int i = startID; i <= endID; i++)
            idList.vec.push_back(i);
        }
        else
        {
          throw Kernel::Exception::InstrumentDefinitionError("id subelement of idlist " + 
            std::string("element wrongly specified in XML instrument file"), m_filename);	
        }  
      }

      pNode = it.nextNode();
    } // end while loop
  }
}


/** Method for populating IdList.
 *
 *  @param type  name of the type of a component in XML instrument definition
 *
 *  @throw InstrumentDefinitionError Thrown if type not defined in XML definition
 */
bool LoadInstrument::isAssemply(std::string type)
{
  std::map<std::string,bool>::iterator it;
  it = isTypeAssemply.find(type);

  if ( it == isTypeAssemply.end() )
  {
    throw Kernel::Exception::InstrumentDefinitionError("type with name = " + type +
      " not defined.", m_filename);	
  }

  return isTypeAssemply[type];
}

} // namespace DataHandling
} // namespace Mantid
