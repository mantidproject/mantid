//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidGeometry/ShapeFactory.h"
#include "MantidAPI/Instrument.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/XMLlogfile.h"
#include "MantidAPI/Progress.h"
#include "MantidGeometry/Detector.h"
#include "MantidGeometry/vtkGeometryCacheReader.h"
#include "MantidGeometry/vtkGeometryCacheWriter.h"
#include "MantidKernel/PhysicalConstants.h"

#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/File.h"

using Poco::XML::DOMParser;
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

/// Empty default constructor
LoadInstrument::LoadInstrument() : Algorithm(), m_haveDefaultFacing(false), m_deltaOffsets(false)
{}

/// Initialisation method.
void LoadInstrument::init()
{
  // When used as a sub-algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut),
    "The name of the workspace to load the instrument definition into" );
  std::vector<std::string> exts;
  exts.push_back("XML");
  exts.push_back("xml");
  declareProperty("Filename","",new FileValidator(exts,false),
    "The filename (including its full or relative path) of an ISIS instrument\n"
    "defintion file" );
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
  const MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");

  // Clear off any existing instrument for this workspace
  localWorkspace->setInstrument(boost::shared_ptr<Instrument>(new Instrument));

  // Remove the path from the filename for use with the InstrumentDataService
  const int stripPath = m_filename.find_last_of("\\/");
  std::string instrumentFile = m_filename.substr(stripPath+1,m_filename.size());
  // Check whether the instrument is already in the InstrumentDataService
  if ( InstrumentDataService::Instance().doesExist(instrumentFile) )
  {
    // If it does, just use the one from the one stored there
    localWorkspace->setInstrument(InstrumentDataService::Instance().retrieve(instrumentFile));
    return;
  }

  // Set up the DOM parser and parse xml file
  DOMParser pParser;
  Document* pDoc;
  try
  {
    pDoc = pParser.parse(m_filename);
  }
  catch(...)
  {
    g_log.error("Unable to parse file " + m_filename);
    throw Kernel::Exception::FileError("Unable to parse File:" , m_filename);
  }

  // Get pointer to root element
  Element* pRootElem = pDoc->documentElement();

  if ( !pRootElem->hasChildNodes() )
  {
    g_log.error("XML file: " + m_filename + "contains no root element.");
    throw Kernel::Exception::InstrumentDefinitionError("No root element in XML instrument file", m_filename);
  }

  // Check whether spherical coordinates should be treated as offsets to parents position
  std::string offsets;
  Element* offsetElement = pRootElem->getChildElement("defaults")->getChildElement("offsets");
  if (offsetElement) offsets = offsetElement->getAttribute("spherical");
  if ( offsets == "delta" ) m_deltaOffsets = true;

  // Check whether default facing is set
  Element* defaultFacingElement = pRootElem->getChildElement("defaults")->getChildElement("components-are-facing");
  if (defaultFacingElement)
  {
    m_haveDefaultFacing = true;
    m_defaultFacing = parseFacingElementToV3D(defaultFacingElement);
  }

  // create maps: isTypeAssembly and mapTypeNameToShape
  Geometry::ShapeFactory shapeCreator;

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
    {
      isTypeAssembly[typeName] = false;

      // for now try to create a geometry shape associated with every type
      // that does not contain any component elements
      mapTypeNameToShape[typeName] = shapeCreator.createShape(pTypeElem);
	  mapTypeNameToShape[typeName]->setName(iType);
    }
    else
      isTypeAssembly[typeName] = true;
    pNL_local->release();
  }
  pNL_type->release();


  // create hasParameterElement

  NodeList* pNL_parameter = pRootElem->getElementsByTagName("parameter");

  unsigned int numParameter = pNL_parameter->length();
  for (unsigned int i = 0; i < numParameter; i++)
  {
    Element* pParameterElem = static_cast<Element*>(pNL_parameter->item(i));
    hasParameterElement.push_back( static_cast<Element*>(pParameterElem->parentNode()) );
  }
  pNL_parameter->release();


  // Get reference to Instrument and set its name
  m_instrument = localWorkspace->getBaseInstrument();
  // We don't want the name taken out of the file itself, it should be the stem of the filename
  //if ( pRootElem->hasAttribute("name") ) m_instrument->setName( pRootElem->getAttribute("name") );
  // Strip off "_Definition.xml"
  const size_t underScore = instrumentFile.find_first_of("_");
  m_instrument->setName( instrumentFile.substr(0,underScore) );

  // do analysis for each top level compoment element
  NodeList* pNL_comp = pRootElem->childNodes(); // here get all child nodes
  unsigned int pNL_comp_length = pNL_comp->length();
  API::Progress prog(this,0,1,pNL_comp_length);
  for (unsigned int i = 0; i < pNL_comp_length; i++)
  {
      prog.report();
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


      // read detertor IDs into idlist if required

      if ( pElem->hasAttribute("idlist") )
      {
        std::string idlist = pElem->getAttribute("idlist");
        Element* pFound = pDoc->getElementById(idlist, "idname");
        populateIdList(pFound, idList);
      }


      if ( isAssembly(pElem->getAttribute("type")) )
      {
        for (unsigned int i_loc = 0; i_loc < pNL_location_length; i_loc++)
        {
          appendAssembly(m_instrument, static_cast<Element*>(pNL_location->item(i_loc)), idList);
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
          appendLeaf(m_instrument, static_cast<Element*>(pNL_location->item(i_loc)), idList);
        }
      }
      pNL_location->release();
    }
  }

  pNL_comp->release();
  // Don't need this anymore (if it was even used) so empty it out to save memory
  m_tempPosHolder.clear();

  pDoc->release();

  // Get cached file name
  std::string cacheFilename=m_filename.replace(m_filename.length()-3,3,"vtp");
  // check for the geometry cache
  Poco::File defFile(m_filename);
  Poco::File vtkFile(cacheFilename);
  bool cacheAvailable=true;
  if((!vtkFile.exists())||defFile.getLastModified()>vtkFile.getLastModified())
	  cacheAvailable=false;
  if(cacheAvailable)
  {
	  g_log.information("Loading geometry cache from "+cacheFilename);
	  // create a vtk reader
	  std::map<std::string, boost::shared_ptr<Geometry::Object> >::iterator objItr;
	  boost::shared_ptr<Mantid::Geometry::vtkGeometryCacheReader> reader(new Mantid::Geometry::vtkGeometryCacheReader(cacheFilename));
	  for( objItr=mapTypeNameToShape.begin(); objItr!=mapTypeNameToShape.end();objItr++)
	  {
		  ((*objItr).second)->setVtkGeometryCacheReader(reader);
	  }
  }
  else
  {
  	  g_log.information("Geometry cache is not avilable");
	  // create a vtk writer
	  std::map<std::string, boost::shared_ptr<Geometry::Object> >::iterator objItr;
	  boost::shared_ptr<Mantid::Geometry::vtkGeometryCacheWriter> writer(new Mantid::Geometry::vtkGeometryCacheWriter(cacheFilename));
	  for( objItr=mapTypeNameToShape.begin(); objItr!=mapTypeNameToShape.end();objItr++)
	  {
		  ((*objItr).second)->setVtkGeometryCacheWriter(writer);
	  }
	  writer->write();
  }
  // Add the instrument to the InstrumentDataService
  InstrumentDataService::Instance().add(instrumentFile,m_instrument);
  return;
}

/** Assumes second argument is a XML location element and its parent is a component element
 *  which is assigned to be an assemble. This method appends the parent component element of
 %  the location element to the CompAssembly passed as the 1st arg. Note this method may call
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

  // set location for this newly added comp and set facing if specified in instrument def. file. Also
  // check if any logfiles are referred to through the <parameter> element.

  setLocation(ass, pLocElem);
  setFacing(ass, pLocElem);
  setLogfile(ass, pCompElem);


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

      if ( isAssembly(typeName) )
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


/** Assumes second argument is pointing to a leaf, which here means the location element (indirectly
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

  std::string typeName = pCompElem->getAttribute("type");
  Element* pType = getTypeElement[typeName];

  std::string category = "";
  if (pType->hasAttribute("is"))
    category = pType->getAttribute("is");


  // do stuff a bit differently depending on which category the type belong to

  if ( category.compare("detector") == 0 )
  {
    std::string name;
    if ( pLocElem->hasAttribute("name") )
    {
      name = pLocElem->getAttribute("name");
    }
    else if ( pCompElem->hasAttribute("name") )
    {
      name = pCompElem->getAttribute("name");
    }
    else
    {
      name = typeName;
    }

    Geometry::Detector* detector = new Geometry::Detector(name, mapTypeNameToShape[typeName], parent);

    // set detector ID and increment it. Finally add the detector to the parent
    detector->setID(idList.vec[idList.counted]);
    idList.counted++;
    parent->add(detector);

    // set location for this newly added comp and set facing if specified in instrument def. file. Also
    // check if any logfiles are referred to through the <parameter> element.
    setLocation(detector, pLocElem);
    setFacing(detector, pLocElem);
    setLogfile(detector, pCompElem);

    try
    {
      if ( pCompElem->hasAttribute("mark-as") || pLocElem->hasAttribute("mark-as") )
        m_instrument->markAsMonitor(detector);
      else
        m_instrument->markAsDetector(detector);

    }
    catch(Kernel::Exception::ExistsError&)
    {
      std::stringstream convert;
      convert << detector->getID();
      throw Kernel::Exception::InstrumentDefinitionError("Detector with ID = " + convert.str() +
        " present more then once in XML instrument file", m_filename);
    }

    // Add all monitors and detectors to 'facing component' container. This is only used if the
    // "facing" elements are defined in the instrument definition file

    m_facingComponent.push_back(detector);
  }
  else
  {
    std::string name;
    if ( pLocElem->hasAttribute("name") )
      name = pLocElem->getAttribute("name");
    else if ( pCompElem->hasAttribute("name") )
    {
      name = pCompElem->getAttribute("name");
    }
    else
    {
      name = typeName;
    }

    Geometry::ObjComponent *comp = new Geometry::ObjComponent(name, mapTypeNameToShape[typeName], parent);
    parent->add(comp);

    // check if special Source or SamplePos Component
    if ( category.compare("Source") == 0 )
    {
      m_instrument->markAsSource(comp);
    }
    if ( category.compare("SamplePos") == 0 )
    {
      m_instrument->markAsSamplePos(comp);
    }

    // set location for this newly added comp and set facing if specified in instrument def. file. Also
    // check if any logfiles are referred to through the <parameter> element.

    setLocation(comp, pLocElem);
    setFacing(comp, pLocElem);
    setLogfile(comp, pCompElem);
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
 *  @throw logic_error Thrown if second argument is not a pointer to a 'location' XML element
 */
void LoadInstrument::setLocation(Geometry::Component* comp, Poco::XML::Element* pElem)
{
  // Require that pElem points to an element with tag name 'location'

  if ( (pElem->tagName()).compare("location") )
  {
    g_log.error("Second argument to function setLocation must be a pointer to an XML element with tag name location.");
    throw std::logic_error( "Second argument to function setLocation must be a pointer to an XML element with tag name location." );
  }

  // Polar coordinates can be labelled as (r,t,p) or (R,theta,phi)
  if ( pElem->hasAttribute("r") || pElem->hasAttribute("t") || pElem->hasAttribute("p") ||
       pElem->hasAttribute("R") || pElem->hasAttribute("theta") || pElem->hasAttribute("phi") )
  {
    double R=0.0, theta=0.0, phi=0.0;

    if ( pElem->hasAttribute("r") ) R = atof((pElem->getAttribute("r")).c_str());
    if ( pElem->hasAttribute("t") ) theta = atof((pElem->getAttribute("t")).c_str());
    if ( pElem->hasAttribute("p") ) phi = atof((pElem->getAttribute("p")).c_str());

    if ( pElem->hasAttribute("R") ) R = atof((pElem->getAttribute("R")).c_str());
    if ( pElem->hasAttribute("theta") ) theta = atof((pElem->getAttribute("theta")).c_str());
    if ( pElem->hasAttribute("phi") ) phi = atof((pElem->getAttribute("phi")).c_str());

    if ( m_deltaOffsets )
    {
      // In this case, locations given are radial offsets to the (radial) position of the parent,
      // so need to do some extra calculation before they're stored internally as x,y,z offsets.

      // Temporary vector to hold the parent's absolute position (will be 0,0,0 if no parent)
      Geometry::V3D parentPos;
      // Get the parent's absolute position (if the component has a parent)
      if ( comp->getParent() )
      {
        SphVec parent = m_tempPosHolder[comp->getParent().get()];
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
      m_tempPosHolder[comp] = tmp;

      // Create a V3D and set its position to be the child's absolute position
      Geometry::V3D absPos;
      absPos.spherical(R,theta,phi);

      // Subtract the two V3D's to get what we want (child's relative position in x,y,z)
      Geometry::V3D relPos;
      relPos = absPos - parentPos;
      comp->setPos(relPos);
    }
    else
    {
      // In this case, the value given represents a vector from the parent to the child
      Geometry::V3D pos;
      pos.spherical(R,theta,phi);
      comp->setPos(pos);
    }

  }
  else
  {
    double x=0.0, y=0.0, z=0.0;

    if ( pElem->hasAttribute("x") ) x = atof((pElem->getAttribute("x")).c_str());
    if ( pElem->hasAttribute("y") ) y = atof((pElem->getAttribute("y")).c_str());
    if ( pElem->hasAttribute("z") ) z = atof((pElem->getAttribute("z")).c_str());

    comp->setPos(Geometry::V3D(x,y,z));
  }


  // Rotate coordinate system of this component

  if ( pElem->hasAttribute("rot") )
  {
    double rotAngle = atof( (pElem->getAttribute("rot")).c_str() ); // assumed to be in degrees

    double axis_x = 0.0;
    double axis_y = 0.0;
    double axis_z = 1.0;

    if ( pElem->hasAttribute("axis-x") )
      axis_x = atof( (pElem->getAttribute("axis-x")).c_str() );
    if ( pElem->hasAttribute("axis-y") )
      axis_y = atof( (pElem->getAttribute("axis-y")).c_str() );
    if ( pElem->hasAttribute("axis-z") )
      axis_z = atof( (pElem->getAttribute("axis-z")).c_str() );

    comp->rotate(Geometry::Quat(rotAngle, Geometry::V3D(axis_x,axis_y,axis_z)));
  }

  // loop recursively to see if location element containes (further) rotation instructions
  bool stillRotationElement = true;
  while ( stillRotationElement )
  {
    Element* rotElement = pElem->getChildElement("rot");
    if (rotElement) 
    {
      double rotAngle = atof( (rotElement->getAttribute("val")).c_str() ); // assumed to be in degrees

      double axis_x = 0.0;
      double axis_y = 0.0;
      double axis_z = 1.0;

      if ( rotElement->hasAttribute("axis-x") )
        axis_x = atof( (rotElement->getAttribute("axis-x")).c_str() );
      if ( rotElement->hasAttribute("axis-y") )
        axis_y = atof( (rotElement->getAttribute("axis-y")).c_str() );
      if ( rotElement->hasAttribute("axis-z") )
        axis_z = atof( (rotElement->getAttribute("axis-z")).c_str() );

      comp->rotate(Geometry::Quat(rotAngle, Geometry::V3D(axis_x,axis_y,axis_z)));      

      pElem = rotElement; // for recursive action
    }
    else
      stillRotationElement = false;
  }
}


/** Get parent component element of location element.
 *
 *  @param pLocElem  Poco::XML element that points a location element in the XML doc
 *  @return Parent XML element to a location XML element
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

    int increment = 1;
    if ( pE->hasAttribute("step") ) increment = atoi( (pE->getAttribute("step")).c_str() );
    for (int i = startID; i != endID+increment; i += increment)
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
          int valID = atoi( (pIDElem->getAttribute("val")).c_str() );
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

          int increment = 1;
          if ( pIDElem->hasAttribute("step") ) increment = atoi( (pIDElem->getAttribute("step")).c_str() );
          for (int i = startID; i != endID+increment; i += increment)
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
bool LoadInstrument::isAssembly(std::string type)
{
  std::map<std::string,bool>::iterator it;
  it = isTypeAssembly.find(type);

  if ( it == isTypeAssembly.end() )
  {
    throw Kernel::Exception::InstrumentDefinitionError("type with name = " + type +
      " not defined.", m_filename);
  }

  return isTypeAssembly[type];
}


/** Make the shape defined in 1st argument face the component in the second argument,
 *  by rotating the z-axis of the component passed in 1st argument so that it points in the
 *  direction: from the component as specified 2nd argument to the component as specified in 1st argument.
 *
 *  @param in  Component to be rotated
 *  @param facing Object to face
 */
void LoadInstrument::makeXYplaneFaceComponent(Geometry::Component* &in, const Geometry::ObjComponent* facing)
{
  const Geometry::V3D facingPoint = facing->getPos();

  makeXYplaneFaceComponent(in, facingPoint);
}


/** Make the shape defined in 1st argument face the position in the second argument,
 *  by rotating the z-axis of the component passed in 1st argument so that it points in the
 *  direction: from the position (as specified 2nd argument) to the component (1st argument).
 *
 *  @param in  Component to be rotated
 *  @param facingPoint position to face
 */
void LoadInstrument::makeXYplaneFaceComponent(Geometry::Component* &in, const Geometry::V3D& facingPoint)
{
  Geometry::V3D pos = in->getPos();

  // vector from facing object to component we want to rotate

  Geometry::V3D facingDirection = pos - facingPoint;
  facingDirection.normalize();

  if ( facingDirection.norm() == 0.0 ) return;


  // now aim to rotate shape such that the z-axis of of the object we want to rotate
  // points in the direction of facingDirection. That way the XY plane faces the 'facing object'.

  Geometry::V3D z = Geometry::V3D(0,0,1);
  Geometry::Quat R = in->getRotation();
  R.inverse();
  R.rotate(facingDirection);

  Geometry::V3D normal = facingDirection.cross_prod(z);
  normal.normalize();
  double theta = (180.0/M_PI)*facingDirection.angle(z);

  if ( normal.norm() > 0.0 )
    in->rotate(Geometry::Quat(-theta, normal));
  else
  {
    // To take into account the case where the facing direction is in the (0,0,1)
    // or (0,0,-1) direction.
    in->rotate(Geometry::Quat(-theta, Geometry::V3D(0,1,0)));
  }
}


/** Parse position of facing element to V3D
 *
 *  @param pElem  Facing type element to parse
 *  @return Return parsed position as a V3D
 */
Geometry::V3D LoadInstrument::parseFacingElementToV3D(Poco::XML::Element* pElem)
{

  Geometry::V3D retV3D;

  // Polar coordinates can be labelled as (r,t,p) or (R,theta,phi)
  if ( pElem->hasAttribute("r") || pElem->hasAttribute("t") || pElem->hasAttribute("p") ||
       pElem->hasAttribute("R") || pElem->hasAttribute("theta") || pElem->hasAttribute("phi") )
  {
    double R=0.0, theta=0.0, phi=0.0;

    if ( pElem->hasAttribute("r") ) R = atof((pElem->getAttribute("r")).c_str());
    if ( pElem->hasAttribute("t") ) theta = atof((pElem->getAttribute("t")).c_str());
    if ( pElem->hasAttribute("p") ) phi = atof((pElem->getAttribute("p")).c_str());

    if ( pElem->hasAttribute("R") ) R = atof((pElem->getAttribute("R")).c_str());
    if ( pElem->hasAttribute("theta") ) theta = atof((pElem->getAttribute("theta")).c_str());
    if ( pElem->hasAttribute("phi") ) phi = atof((pElem->getAttribute("phi")).c_str());

    retV3D.spherical(R,theta,phi);
  }
  else
  {
    double x=0.0, y=0.0, z=0.0;

    if ( pElem->hasAttribute("x") ) x = atof((pElem->getAttribute("x")).c_str());
    if ( pElem->hasAttribute("y") ) y = atof((pElem->getAttribute("y")).c_str());
    if ( pElem->hasAttribute("z") ) z = atof((pElem->getAttribute("z")).c_str());

    retV3D(x,y,z);
  }

  return retV3D;
}


/** Set facing of comp as specified in XML facing element (which must be sub-element of a location element).
 *
 *  @param comp To set facing of
 *  @param pElem  Poco::XML element that points a location element in the XML doc
 *
 *  @throw logic_error Thrown if second argument is not a pointer to a 'location' XML element
 */
void LoadInstrument::setFacing(Geometry::Component* comp, Poco::XML::Element* pElem)
{
  // Require that pElem points to an element with tag name 'location'

  if ( (pElem->tagName()).compare("location") )
  {
    g_log.error("Second argument to function setLocation must be a pointer to an XML element with tag name location.");
    throw std::logic_error( "Second argument to function setLocation must be a pointer to an XML element with tag name location." );
  }

  Element* facingElem = pElem->getChildElement("facing");
  if (facingElem)
  {
    // check if user want to rotate about z-axis before potentially applying facing

    if ( facingElem->hasAttribute("rot") )
    {
      double rotAngle = atof( (facingElem->getAttribute("rot")).c_str() ); // assumed to be in degrees
      comp->rotate(Geometry::Quat(rotAngle, Geometry::V3D(0,0,1)));
    }


    // For now assume that if has val attribute it means facing = none. This option only has an
    // effect when a default facing setting is set. In which case this then means "ignore the
    // default facing setting" for this component

    if ( facingElem->hasAttribute("val") )
      return;


    // Face the component to the x,y,z or r,t,p coordinates of the facing component

    makeXYplaneFaceComponent(comp, parseFacingElementToV3D(facingElem));

  }
  else // so if no facing element associated with location element apply default facing if set
    if (m_haveDefaultFacing)
      makeXYplaneFaceComponent(comp, m_defaultFacing);
}

/** Set parameter/logfile info (if any) associated with component
 *
 *  @param comp Some component
 *  @param pElem  Associated Poco::XML element to component that may hold a \<parameter\> element
 *
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
 */
void LoadInstrument::setLogfile(Geometry::Component* comp, Poco::XML::Element* pElem)
{
  // check first if pElem contains any <parameter> child elements

  if ( hasParameterElement.end() == std::find(hasParameterElement.begin(),hasParameterElement.end(),pElem) ) return;


  NodeList* pNL = pElem->getElementsByTagName("parameter");
  unsigned int numberParam = pNL->length();

  // Get logfile-cache from instrument
  std::multimap<std::string, boost::shared_ptr<API::XMLlogfile> >& logfileCache = m_instrument->getLogfileCache();

  for (unsigned int i = 0; i < numberParam; i++)
  {
    Element* pParamElem = static_cast<Element*>(pNL->item(i));

    if ( !pParamElem->hasAttribute("name") )
      throw Kernel::Exception::InstrumentDefinitionError("XML element with name or type = " + comp->getName() +
        " contain <parameter> element with no name attribute in XML instrument file", m_filename);

    if ( !pParamElem->hasAttribute("logfile-id") && !pParamElem->hasAttribute("value") )
      throw Kernel::Exception::InstrumentDefinitionError("XML element with name or type = " + comp->getName() +
        " contain <parameter> element with no logfile-id or value attribute in XML instrument file", m_filename);

    std::string logfileID;
    std::string value;

    if ( pParamElem->hasAttribute("logfile-id") )
    {
      // "logfile-id" takes presedence over "value" attribute if both are present
      // hence if "logfile-id" specified value is read from logfile
      logfileID = pParamElem->getAttribute("logfile-id");
    }
    else
    {
      // rather then extracting value from logfile, specify a value directly
      value = pParamElem->getAttribute("value");
    }

    std::string paramName = pParamElem->getAttribute("name");
    std::string type = "double"; // default
    std::string extractSingleValueAs = "mean"; // default
    std::string eq = "";

    if ( pParamElem->hasAttribute("eq") )
      eq = pParamElem->getAttribute("eq");
    if ( pParamElem->hasAttribute("type") )
      type = pParamElem->getAttribute("type");
    if ( pParamElem->hasAttribute("extract-single-value-as") )
      extractSingleValueAs = pParamElem->getAttribute("extract-single-value-as");

    boost::shared_ptr<XMLlogfile> temp(new XMLlogfile(logfileID, value, paramName, type, extractSingleValueAs, eq, comp));
    logfileCache.insert( std::pair<std::string,boost::shared_ptr<XMLlogfile> >(logfileID,temp));
  }
  pNL->release();
}


} // namespace DataHandling
} // namespace Mantid
