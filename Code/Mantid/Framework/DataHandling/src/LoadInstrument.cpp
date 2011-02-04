//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadParameterFile.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Instrument/Instrument.h"
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
#include "MantidDataHandling/LoadInstrumentHelper.h"
#include "MantidKernel/ArrayProperty.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/File.h>
#include <Poco/Path.h>
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

    DECLARE_ALGORITHM(LoadInstrument)

    using namespace Kernel;
    using namespace API;
    using namespace Geometry;

    /// Empty default constructor
    LoadInstrument::LoadInstrument() : Algorithm(), hasParameterElement_beenSet(false),
      m_haveDefaultFacing(false), m_deltaOffsets(false)
    {}


    //------------------------------------------------------------------------------------------------------------------------------
    /// Initialisation method.
    void LoadInstrument::init()
    {
      // When used as a sub-algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
      declareProperty(
        new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut),
        "The name of the workspace to load the instrument definition into" );
      declareProperty(new FileProperty("Filename","", FileProperty::OptionalLoad, ".xml"),
        "The filename (including its full or relative path) of an instrument\n"
        "definition file");
      declareProperty("MonitorList", std::vector<int>(), new NullValidator< std::vector<int> >,
        "List of detector ids of monitors loaded in to the workspace", Direction::Output);
      declareProperty( "InstrumentName", "",
        "Name of instrument. Can be used instead of Filename to specify an IDF" );

      m_angleConvertConst = 1.0;
    }


    //------------------------------------------------------------------------------------------------------------------------------
    /** Executes the algorithm. Reading in the file and creating and populating
    *  the output workspace
    *
    *  @throw FileError Thrown if unable to parse XML file
    *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
    */
    void LoadInstrument::exec()
    {
      // Get the input workspace
      m_workspace = getProperty("Workspace");

      // Retrieve the filename from the properties
      m_filename = getPropertyValue("Filename");
      if ( m_filename.empty() )
      {
        // look to see if an Instrument name provided in which case create
        // IDF filename on the fly
        std::string instName = getPropertyValue("InstrumentName");
        if ( instName.empty() )
        {
          g_log.error("Either the InstrumentName or Filename property of LoadInstrument most be specified"); 
          throw Kernel::Exception::FileError("Either the InstrumentName or Filename property of LoadInstrument most be specified to load an IDF" , m_filename);
        }
        else
        {
          LoadInstrumentHelper helper;
          m_filename = helper.getInstrumentFilename(instName, 
            helper.getWorkspaceStartDate(m_workspace));
        }
      }


      // Clear off any existing instrument for this workspace
      m_workspace->setInstrument(boost::shared_ptr<Instrument>(new Instrument));

      // Remove the path from the filename for use with the InstrumentDataService
      const int stripPath = m_filename.find_last_of("\\/");
      std::string instrumentFile = m_filename.substr(stripPath+1,m_filename.size());

      // Get reference to Instrument and set its name
      m_instrument = m_workspace->getBaseInstrument();

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

      // check if IDF has valid-from and valid-to tags defined
      if ( !pRootElem->hasAttribute("valid-from") )
      {
        throw Kernel::Exception::InstrumentDefinitionError("<instrument> element must contain a valid-from tag", m_filename);
      }
      else
      {
        try
        {
          DateAndTime d(pRootElem->getAttribute("valid-from"));
        }
        catch(...)
        {
          throw Kernel::Exception::InstrumentDefinitionError("The valid-from <instrument> tag must be a ISO8601 string", m_filename);
        }
      }

      if ( !pRootElem->hasAttribute("valid-to") )
      {
        // Ticket #2335: no required valid-to date.
        //throw Kernel::Exception::InstrumentDefinitionError("<instrument> element must contain a valid-to tag", m_filename);
      }
      else
      {
        try
        {
          DateAndTime d(pRootElem->getAttribute("valid-to"));
        }
        catch(...)
        {
          throw Kernel::Exception::InstrumentDefinitionError("The valid-to <instrument> tag must be a ISO8601 string", m_filename);
        }
      }

      // Handle used in the singleton constructor for instrument file should append the value
      // of the last-modified tag inside the file to determine if it is already in memory so that
      // changes to the instrument file will cause file to be reloaded.
      instrumentFile = instrumentFile + pRootElem->getAttribute("last-modified"); // pRootElem->getAttribute("name") + pRootElem->getAttribute("last-modified");

      // Check whether the instrument is already in the InstrumentDataService
      if ( InstrumentDataService::Instance().doesExist(instrumentFile) )
      {
        // If it does, just use the one from the one stored there
        m_workspace->setInstrument(InstrumentDataService::Instance().retrieve(instrumentFile));
        // Get reference to Instrument 
        m_instrument = m_workspace->getBaseInstrument();
        //get list of monitors and set the property
        std::vector<int>monitordetIdList=m_instrument->getMonitors();
        setProperty("MonitorList",monitordetIdList);
      }
      else
      {
        readDefaults(pRootElem->getChildElement("defaults"));

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

          // Each type in the IDF must be uniquely named, hence return error if type
          // has already been defined
          if ( getTypeElement.find(typeName) != getTypeElement.end() )
          {
            g_log.error("XML file: " + m_filename + "contains more than one type element named " + typeName);
            throw Kernel::Exception::InstrumentDefinitionError("XML instrument file contains more than one type element named " + typeName, m_filename);
          }
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
          {
            isTypeAssembly[typeName] = true;
            if (pTypeElem->hasAttribute("outline"))
            {
              pTypeElem->setAttribute("object_created","no");
            }
          }
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
        hasParameterElement_beenSet = true;

        // We don't want the name taken out of the XML file itself, it should come from the filename
        // Strip off "_Definition.xml"
        const size_t underScore = instrumentFile.find_first_of("_");
        m_instrument->setName( instrumentFile.substr(0,underScore) );

        // See if any parameters set at instrument level
        setLogfile(m_instrument.get(), pRootElem, m_instrument->getLogfileCache());

        // do analysis for each top level compoment element
        NodeList* pNL_comp = pRootElem->childNodes(); // here get all child nodes
        unsigned int pNL_comp_length = pNL_comp->length();
        API::Progress prog(this,0,1,pNL_comp_length);
        for (unsigned int i = 0; i < pNL_comp_length; i++)
        {
          if (VERBOSE) std::cout << "exec(): Node = "<< pNL_comp->item(i)->nodeName() << "\n";
          prog.report();
          // we are only interest in the top level component elements hence
          // the reason for the if statement below

          if ( (pNL_comp->item(i))->nodeType() == Node::ELEMENT_NODE &&
            ((pNL_comp->item(i))->nodeName()).compare("component") == 0 )
          {
            Element* pElem = static_cast<Element*>(pNL_comp->item(i));

            IdList idList; // structure to possibly be populated with detector IDs

            // get all location elements contained in component element
            NodeList* pNL_location = pElem->getElementsByTagName("location");
            unsigned int pNL_location_length = pNL_location->length();
            if (pNL_location_length == 0)
            {
              g_log.error(std::string("A component element must contain at least one location element") +
                " even if it is just an empty location element of the form <location />");
              throw Kernel::Exception::InstrumentDefinitionError(
                std::string("A component element must contain at least one location element") +
                " even if it is just an empty location element of the form <location />", m_filename);
            }


            if ( isAssembly(pElem->getAttribute("type")) )
            {
              if (VERBOSE) std::cout << "exec(): This element has a type that is an assembly\n";

              for (unsigned int i_loc = 0; i_loc < pNL_location_length; i_loc++)
              {
                Element* pLocElem = static_cast<Element*>(pNL_location->item(i_loc));

                // check if <exclude> sub-elements for this location and create new exclude list to pass on 
                NodeList* pNLexclude = pLocElem->getElementsByTagName("exclude");
                unsigned int numberExcludeEle = pNLexclude->length();
                std::vector<std::string> newExcludeList;
                for (unsigned int i = 0; i < numberExcludeEle; i++)
                {
                  Element* pExElem = static_cast<Element*>(pNLexclude->item(i));
                  if ( pExElem->hasAttribute("sub-part") )
                    newExcludeList.push_back(pExElem->getAttribute("sub-part"));
                }
                pNLexclude->release();

                if (VERBOSE) std::cout << "exec(): AppendAssembly of " << pLocElem->nodeName() << "\n";

                appendAssembly(m_instrument, pLocElem, idList, newExcludeList);
              }

              // a check
              if (idList.counted != static_cast<int>(idList.vec.size()) )
              {
                std::stringstream ss1, ss2;
                ss1 << idList.vec.size(); ss2 << idList.counted;
                g_log.error("The number of detector IDs listed in idlist named "
                  + pElem->getAttribute("idlist") +
                  " is larger than the number of detectors listed in type = "
                  + pElem->getAttribute("type"));
                throw Kernel::Exception::InstrumentDefinitionError(
                  "Number of IDs listed in idlist (=" + ss1.str() + ") is larger than the number of detectors listed in type = "
                  + pElem->getAttribute("type") + " (=" + ss2.str() + ").", m_filename);
              }
            }
            else
            {	

              for (unsigned int i_loc = 0; i_loc < pNL_location_length; i_loc++)
              {
                appendLeaf(m_instrument, static_cast<Element*>(pNL_location->item(i_loc)), idList);
              }
            }
            std::vector<int>monitordetIdList=m_instrument->getMonitors();
            setProperty("MonitorList",monitordetIdList);
            pNL_location->release();
          }
        }

        pNL_comp->release();
        // Don't need this anymore (if it was even used) so empty it out to save memory
        m_tempPosHolder.clear();

        // Get cached file name
        // If the instrument directory is writable, put them there else use temporary directory
        std::string cacheFilename(m_filename.begin(),m_filename.end()-3);

        cacheFilename += "vtp";
        // check for the geometry cache
        Poco::File defFile(m_filename);
        Poco::File vtkFile(cacheFilename);
        Poco::File instrDir(Poco::Path(defFile.path()).parent());

        bool cacheAvailable = true;
        if ((!vtkFile.exists()) || defFile.getLastModified() > vtkFile.getLastModified())
        {
          g_log.information() << "Cache not available at " << cacheFilename << "\n";

          cacheAvailable = false;
        }

        std::string filestem = Poco::Path(cacheFilename).getFileName();
        Poco::Path fallback_dir(Kernel::ConfigService::Instance().getTempDir());
        Poco::File fallbackFile = Poco::File(fallback_dir.resolve(filestem));
        if( cacheAvailable == false )
        { 
          g_log.warning() << "Trying fallback " << fallbackFile.path() << "\n";
          if ((!fallbackFile.exists()) || defFile.getLastModified() > fallbackFile.getLastModified())
          {
            cacheAvailable = false;
          }
          else
          {
            cacheAvailable = true;
            cacheFilename = fallbackFile.path();
          }
        }

        if (cacheAvailable)
        {
          g_log.information("Loading geometry cache from " + cacheFilename);
          // create a vtk reader
          std::map<std::string, boost::shared_ptr<Geometry::Object> >::iterator objItr;
          boost::shared_ptr<Mantid::Geometry::vtkGeometryCacheReader> 
            reader(new Mantid::Geometry::vtkGeometryCacheReader(cacheFilename));
          for (objItr = mapTypeNameToShape.begin(); objItr != mapTypeNameToShape.end(); objItr++)
          {
            ((*objItr).second)->setVtkGeometryCacheReader(reader);
          }
        }
        else
        {
          g_log.information("Geometry cache is not available");
          try
          {
            if( !instrDir.canWrite() )
            {
              cacheFilename = fallbackFile.path();
              g_log.warning() << "Instrument directory is read only, writing cache to system temp.\n";
            }
          }
          catch(Poco::FileNotFoundException &)
          {
            g_log.error() << "Unable to find instrument definition while attempting to write cache.\n";
            throw std::runtime_error("Unable to find instrument definition while attempting to write cache.\n");
          }
          g_log.information() << "Creating cache in " << cacheFilename << "\n";
          // create a vtk writer
          std::map<std::string, boost::shared_ptr<Geometry::Object> >::iterator objItr;
          boost::shared_ptr<Mantid::Geometry::vtkGeometryCacheWriter> 
            writer(new Mantid::Geometry::vtkGeometryCacheWriter(cacheFilename));
          for (objItr = mapTypeNameToShape.begin(); objItr != mapTypeNameToShape.end(); objItr++)
          {
            ((*objItr).second)->setVtkGeometryCacheWriter(writer);
          }
          writer->write();
        }

        // Add/overwrite any instrument params with values specified in <component-link> XML elements
        setComponentLinks(m_instrument, pRootElem);
        // Add the instrument to the InstrumentDataService
        InstrumentDataService::Instance().add(instrumentFile,m_instrument);
      }

      // populate parameter map of workspace 
      m_workspace->populateInstrumentParameters();

      // check if default parameter file is also present
      runLoadParameterFile();

      // release XML document
      pDoc->release();
    }

    //-----------------------------------------------------------------------------------------------------------------------
    /** Reads the contents of the \<defaults\> element to set member variables,
    *  requires m_instrument to be already set
    *  @param defaults :: points to the data read from the \<defaults\> element
    */
    void LoadInstrument::readDefaults(Poco::XML::Element* defaults)
    {
      // Check whether spherical coordinates should be treated as offsets to parents position
      std::string offsets;
      Element* offsetElement = defaults->getChildElement("offsets");
      if (offsetElement) offsets = offsetElement->getAttribute("spherical");
      if ( offsets == "delta" ) m_deltaOffsets = true;

      // Check whether default facing is set
      Element* defaultFacingElement = defaults->getChildElement("components-are-facing");
      if (defaultFacingElement)
      {
        m_haveDefaultFacing = true;
        m_defaultFacing = parseFacingElementToV3D(defaultFacingElement);
      }

      // the default view is used by the instrument viewer to decide the angle to display the instrument from on start up
      Element* defaultView = defaults->getChildElement("default-view");
      if (defaultView)
      {
        m_instrument->setDefaultViewAxis(defaultView->getAttribute("axis-view"));
      }

      // check if angle=radian has been set
      Element* angleUnit = defaults->getChildElement("angle");
      if (angleUnit)
      {
        if (angleUnit->getAttribute("unit") == "radian")
        {
          m_angleConvertConst = 180.0/M_PI;
          std::map<std::string, std::string>& units = m_instrument->getLogfileUnit();
          units["angle"] = "radian";
        }
      }
    }


    //-----------------------------------------------------------------------------------------------------------------------
    /** Assumes second argument is a XML location element and its parent is a component element
    *  which is assigned to be an assembly. This method appends the parent component element of
    %  the location element to the CompAssembly passed as the 1st arg. Note this method may call
    %  itself, i.e. it may act recursively.
    *
    *  @param parent :: CompAssembly to append new component to
    *  @param pLocElem ::  Poco::XML element that points to a location element in an instrument description XML file
    *  @param idList :: The current IDList
    *  @param excludeList :: The exclude List
    */
    void LoadInstrument::appendAssembly(Geometry::ICompAssembly* parent, Poco::XML::Element* pLocElem, IdList& idList, 
      const std::vector<std::string> excludeList)
    {
      if (VERBOSE) std::cout << "appendAssembly() starting for parent " << parent->getName() << "\n";

      // The location element is required to be a child of a component element. Get this component element
      Element* pCompElem = getParentComponent(pLocElem);

      // Read detector IDs into idlist if required
      // Note idlist may be defined for any component
      // Note any new idlist found will take presedence. 

      if ( pCompElem->hasAttribute("idlist") )
      {
        std::string idlist = pCompElem->getAttribute("idlist");

        if ( idlist.compare(idList.idname) )
        {
          Element* pFound = pCompElem->ownerDocument()->getElementById(idlist, "idname");

          if ( pFound == NULL )
          {
            throw Kernel::Exception::InstrumentDefinitionError(
              "No <idlist> with name idname=\"" + idlist + "\" present in instrument definition file.", m_filename);
          }
          idList.reset(); 
          populateIdList(pFound, idList);
        }
      }

      //Create the assembly that will be appended into the parent.
      Geometry::ICompAssembly *ass;
      // The newly added component is required to have a type. Find out what this
      // type is and find all the location elements of this type. Finally loop over these
      // location elements

      Element* pType = getTypeElement[pCompElem->getAttribute("type")];
      if (pType->hasAttribute("outline") && pType->getAttribute("outline") != "no")
      {
        ass = new Geometry::ObjCompAssembly(getNameOfLocationElement(pLocElem));
      }
      else
      {
        ass = new Geometry::CompAssembly;
        ass->setName( getNameOfLocationElement(pLocElem) );
      }
      ass->setParent(parent);
      parent->add(ass);

      ass->setName( getNameOfLocationElement(pLocElem) );

      if (VERBOSE) std::cout << "appendAssembly() is creating an assembly called " << ass->getName() << "\n";

      // set location for this newly added comp and set facing if specified in instrument def. file. Also
      // check if any logfiles are referred to through the <parameter> element.

      setLocation(ass, pLocElem);
      setFacing(ass, pLocElem);
      setLogfile(ass, pCompElem, m_instrument->getLogfileCache());  // params specified within <component>
      setLogfile(ass, pLocElem, m_instrument->getLogfileCache());  // params specified within specific <location>


      NodeIterator it(pType, NodeFilter::SHOW_ELEMENT);

      Node* pNode = it.nextNode();
      while (pNode)
      {
        if ( pNode->nodeName().compare("location")==0 )
        {
          Element* pElem = static_cast<Element*>(pNode);

          // check if this location is in the exclude list
          std::vector<std::string>::const_iterator it = find(excludeList.begin(), excludeList.end(), getNameOfLocationElement(pElem));
          if ( it == excludeList.end() )
          {

            std::string typeName = (getParentComponent(pElem))->getAttribute("type");

            if (VERBOSE) std::cout << "appendAssembly() has found that its parent's type = " << typeName << "\n";

            if ( isAssembly(typeName) )
            {

              // check if <exclude> sub-elements for this location and create new exclude list to pass on 
              NodeList* pNLexclude = pElem->getElementsByTagName("exclude");
              unsigned int numberExcludeEle = pNLexclude->length();
              std::vector<std::string> newExcludeList;
              for (unsigned int i = 0; i < numberExcludeEle; i++)
              {
                Element* pExElem = static_cast<Element*>(pNLexclude->item(i));
                if ( pExElem->hasAttribute("sub-part") )
                  newExcludeList.push_back(pExElem->getAttribute("sub-part"));
              }
              pNLexclude->release();

              appendAssembly(ass, pElem, idList, newExcludeList);
            }
            else
              appendLeaf(ass, pElem, idList);
          }
        }
        pNode = it.nextNode();
      }

      // create outline object for the assembly
      if (pType->hasAttribute("outline") && pType->getAttribute("outline") != "no")
      {
        Geometry::ObjCompAssembly* objAss = dynamic_cast<Geometry::ObjCompAssembly*>(ass);
        if (pType->getAttribute("object_created") == "no")
        {
          pType->setAttribute("object_created","yes");
          boost::shared_ptr<Geometry::Object> obj = objAss->createOutline();
          if (obj)
          {
            mapTypeNameToShape[pType->getAttribute("name")] = obj;
          }
          else
          {// object failed to be created
            pType->setAttribute("outline","no");
            g_log.warning()<<"Failed to create outline object for assembly "<<pType->getAttribute("name")<<'\n';
          }
        }
        else
        {
          objAss->setOutline(mapTypeNameToShape[pType->getAttribute("name")]);
        }
      }

    }


    //-----------------------------------------------------------------------------------------------------------------------
    /** Assumes second argument is a XML location element and its parent is a component element
    *  which is assigned to be an assemble. This method appends the parent component element of
    %  the location element to the CompAssembly passed as the 1st arg. Note this method may call
    %  itself, i.e. it may act recursively.
    *
    *  @param parent :: CompAssembly to append new component to
    *  @param pLocElem ::  Poco::XML element that points to a location element in an instrument description XML file
    *  @param idList :: The current IDList
    *  @param excludeList :: The exclude List
    */
    void LoadInstrument::appendAssembly(boost::shared_ptr<Geometry::ICompAssembly> parent, Poco::XML::Element* pLocElem, IdList& idList,
      const std::vector<std::string> excludeList)
    {
      appendAssembly(parent.get(), pLocElem, idList, excludeList);
    }


    //-----------------------------------------------------------------------------------------------------------------------
    /** Assumes second argument is pointing to a leaf, which here means the location element (indirectly
    *  representing a component element) that contains no sub-components. This component is appended
    %  to the parent (1st argument).
    *
    *  @param parent :: CompAssembly to append component to
    *  @param pLocElem ::  Poco::XML element that points to the element in the XML doc we want to add
    *  @param idList :: The current IDList
    *
    *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
    */
    void LoadInstrument::appendLeaf(Geometry::ICompAssembly* parent, Poco::XML::Element* pLocElem, IdList& idList)
    {
      // The location element is required to be a child of a component element. Get this component element
      Element* pCompElem = getParentComponent(pLocElem);

      //--- Get the detector's X/Y pixel sizes (optional) ---
      if (VERBOSE) std::cout << "AppendLeaf: I am " << pLocElem->getAttribute("name") << " . " <<
          "Parent is=" << pCompElem->getAttribute("is") <<
          ". Parent type=" << pCompElem->getAttribute("type") <<
          "\n";

      // Read detector IDs into idlist if required
      // Note idlist may be defined for any component
      // Note any new idlist found will take presedence. 

      if ( pCompElem->hasAttribute("idlist") )
      {
        std::string idlist = pCompElem->getAttribute("idlist");

        if ( idlist.compare(idList.idname) )
        {
          Element* pFound = pCompElem->ownerDocument()->getElementById(idlist, "idname");

          if ( pFound == NULL )
          {
            throw Kernel::Exception::InstrumentDefinitionError(
              "No <idlist> with name idname=\"" + idlist + "\" present in instrument definition file.", m_filename);
          }

          idList.reset(); 
          populateIdList(pFound, idList);
        }
      }


      // get the type element of the component element in order to determine if the type
      // belong to the catogory: "detector", "SamplePos or "Source".

      std::string typeName = pCompElem->getAttribute("type");
      Element* pType = getTypeElement[typeName];

      std::string category = "";
      if (pType->hasAttribute("is"))
        category = pType->getAttribute("is");


      // do stuff a bit differently depending on which category the type belong to
      if ( category.compare("rectangular_detector") == 0  || category.compare("rectangularDetector") == 0  || category.compare("rectangulardetector") == 0 || category.compare("RectangularDetector") == 0 )
      {
        //-------------- Create a RectangularDetector ------------------------------------------------
        std::string name = getNameOfLocationElement(pLocElem);

        if (VERBOSE) std::cout << "AppendLeaf: Creating RectangularDetector " << name << ".\n";

        //Create the bank with the given parent.
        Geometry::RectangularDetector * bank = new Geometry::RectangularDetector(name, parent);

        // set location for this newly added comp and set facing if specified in instrument def. file. Also
        // check if any logfiles are referred to through the <parameter> element.
        setLocation(bank, pLocElem);
        setFacing(bank, pLocElem);
        setLogfile(bank, pCompElem, m_instrument->getLogfileCache()); // params specified within <component>
        setLogfile(bank, pLocElem, m_instrument->getLogfileCache());  // params specified within specific <location>

        //Extract all the parameters from the XML attributes
        int xpixels=0; double xstart=0.; double xstep=0.;
        int ypixels=0; double ystart=0.; double ystep=0.;
        int idstart=0; bool idfillbyfirst_y=true; int idstepbyrow=0;
        int idstep=1;

        //The shape!
        // Given that this leaf component is actually an assembly, its constituent component detector shapes comes from its type attribute.
        const std::string shapeType = pType->getAttribute("type");
        boost::shared_ptr<Geometry::Object> shape = mapTypeNameToShape[shapeType];

        //These parameters are in the TYPE defining RectangularDetector
        if ( pType->hasAttribute("xpixels") ) xpixels = atoi((pType->getAttribute("xpixels")).c_str());
        if ( pType->hasAttribute("xstart") )  xstart  = atof((pType->getAttribute("xstart")).c_str());
        if ( pType->hasAttribute("xstep") )   xstep   = atof((pType->getAttribute("xstep")).c_str());
        if ( pType->hasAttribute("ypixels") ) ypixels = atoi((pType->getAttribute("ypixels")).c_str());
        if ( pType->hasAttribute("ystart") )  ystart  = atof((pType->getAttribute("ystart")).c_str());
        if ( pType->hasAttribute("ystep") )   ystep   = atof((pType->getAttribute("ystep")).c_str());

        //THESE parameters are in the INSTANCE of this type - since they will change.
        if ( pCompElem->hasAttribute("idstart") ) idstart = atoi((pCompElem->getAttribute("idstart")).c_str());
        if ( pCompElem->hasAttribute("idfillbyfirst") )
          idfillbyfirst_y = (pCompElem->getAttribute("idfillbyfirst") == "y");
        //Default ID row step size
        if (idfillbyfirst_y) idstepbyrow = ypixels;
          else idstepbyrow = xpixels;
        if ( pCompElem->hasAttribute("idstepbyrow") ) idstepbyrow = atoi((pCompElem->getAttribute("idstepbyrow")).c_str());
        //Default ID row step size
        if ( pCompElem->hasAttribute("idstep") ) idstep = atoi((pCompElem->getAttribute("idstep")).c_str());

        if (VERBOSE) std::cout << "AppendLeaf: Initializing RectangularDetector with parameters : " <<
            "shape " << typeName << ", " << xpixels << ", " << xstart << ", " << xstep << ", " << ypixels << ", " << ystart << ", " << ystep << ", " << idstart << ", " << idfillbyfirst_y << ", " << idstepbyrow << ".\n";

        // Now, initialize all the pixels in the bank
        bank->initialize(shape, xpixels, xstart, xstep, ypixels, ystart, ystep, idstart, idfillbyfirst_y, idstepbyrow, idstep);

        //Loop through all detectors in the newly created bank and mark those in the instrument.
        try
        {
          for (int i=0; i < bank->nelements(); i++)
          {
            boost::shared_ptr<Geometry::Detector> detector = boost::dynamic_pointer_cast<Geometry::Detector>((*bank)[i]);
            if (detector)
            {
              //Make default facing for the pixel
              Geometry::IComponent* comp = (Geometry::IComponent*) detector.get();
              makeXYplaneFaceComponent(comp, m_defaultFacing);
              //Mark it as a detector (add to the instrument cache)
              m_instrument->markAsDetector(detector.get());
            }
          }
        }
        catch(Kernel::Exception::ExistsError&)
        {
          throw Kernel::Exception::InstrumentDefinitionError(
              "Duplicate detector ID found when adding RectangularDetector " + name + " in XML instrument file" + m_filename);
        }
      }
      else if ( category.compare("detector") == 0 )
      {
        //-------------- Create a Detector ------------------------------------------------
        std::string name = getNameOfLocationElement(pLocElem);

        // before setting detector ID check that the IDF satisfy the following 

        if (idList.counted >=  static_cast<int>(idList.vec.size()) )
        {
          std::stringstream ss1, ss2;
          ss1 << idList.vec.size(); ss2 << idList.counted;
          g_log.error("The number of detector IDs listed in idlist named "
            + idList.idname + " is less then the number of detectors");
          throw Kernel::Exception::InstrumentDefinitionError(
            "Number of IDs listed in idlist (=" + ss1.str() + ") is less than the number of detectors.", m_filename);
        }

        // Create detector and increment id. Finally add the detector to the parent
        Geometry::Detector* detector =
            new Geometry::Detector(name, idList.vec[idList.counted],mapTypeNameToShape[typeName], parent);
        idList.counted++;
        parent->add(detector);

        // set location for this newly added comp and set facing if specified in instrument def. file. Also
        // check if any logfiles are referred to through the <parameter> element.
        setLocation(detector, pLocElem);
        setFacing(detector, pLocElem);
        setLogfile(detector, pCompElem, m_instrument->getLogfileCache()); // params specified within <component>
        setLogfile(detector, pLocElem, m_instrument->getLogfileCache());  // params specified within specific <location>

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
        //-------------- Not a Detector nor a RectangularDetector ------------------------------
        std::string name = getNameOfLocationElement(pLocElem);

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
        setLogfile(comp, pCompElem, m_instrument->getLogfileCache()); // params specified within <component>
        setLogfile(comp, pLocElem, m_instrument->getLogfileCache());  // params specified within specific <location>
      }
    }



    //-----------------------------------------------------------------------------------------------------------------------
    /** Assumes second argument is pointing to a leaf, which here means the location element (indirectly
    *  representing a component element) that contains no sub-components. This component is appended
    %  to the parent (1st argument).
    *
    *  @param parent :: pointer to the CompAssembly to append component to
    *  @param pLocElem ::  Poco::XML element that points to the element in the XML doc we want to add
    *  @param idList :: The current IDList
    *
    *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
    */
    void LoadInstrument::appendLeaf(boost::shared_ptr<Geometry::ICompAssembly> parent, Poco::XML::Element* pLocElem, IdList& idList)
    {
      appendLeaf(parent.get(), pLocElem, idList);
    }


    //-----------------------------------------------------------------------------------------------------------------------
    /** Calculate the position of comp relative to its parent from info provided by \<location\> element.
    *
    *  @param comp :: To set position/location off
    *  @param pElem ::  Poco::XML element that points a location element in the XML doc
    *
    *  @return  Thrown if second argument is not a pointer to a 'location' XML element
    */
    Geometry::V3D LoadInstrument::getRelativeTranslation(const Geometry::IComponent* comp, const Poco::XML::Element* pElem)
    {

      Geometry::V3D retVal;  // position relative to parent

      // Polar coordinates can be labelled as (r,t,p) or (R,theta,phi)
      if ( pElem->hasAttribute("r") || pElem->hasAttribute("t") || pElem->hasAttribute("p") ||
        pElem->hasAttribute("R") || pElem->hasAttribute("theta") || pElem->hasAttribute("phi") )
      {
        double R=0.0, theta=0.0, phi=0.0;

        if ( pElem->hasAttribute("r") ) R = atof((pElem->getAttribute("r")).c_str());
        if ( pElem->hasAttribute("t") ) theta = m_angleConvertConst*atof((pElem->getAttribute("t")).c_str());
        if ( pElem->hasAttribute("p") ) phi = m_angleConvertConst*atof((pElem->getAttribute("p")).c_str());

        if ( pElem->hasAttribute("R") ) R = atof((pElem->getAttribute("R")).c_str());
        if ( pElem->hasAttribute("theta") ) theta = m_angleConvertConst*atof((pElem->getAttribute("theta")).c_str());
        if ( pElem->hasAttribute("phi") ) phi = m_angleConvertConst*atof((pElem->getAttribute("phi")).c_str());

        if ( m_deltaOffsets )
        {
          // In this case, locations given are radial offsets to the (radial) position of the parent,
          // so need to do some extra calculation before they're stored internally as x,y,z offsets.

          // Temporary vector to hold the parent's absolute position (will be 0,0,0 if no parent)
          Geometry::V3D parentPos;
          // Get the parent's absolute position (if the component has a parent)
          if ( comp->getParent() )
          {
            std::map<const Geometry::IComponent*, SphVec>::iterator it;
            it = m_tempPosHolder.find(comp);
            SphVec parent;
            if ( it == m_tempPosHolder.end() )
              parent = m_tempPosHolder[comp->getParent().get()];
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
          m_tempPosHolder[comp] = tmp;

          // Create a V3D and set its position to be the child's absolute position
          Geometry::V3D absPos;
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
    /** Set location (position) of comp as specified in XML location element.
    *
    *  @param comp :: To set position/location off
    *  @param pElem ::  Poco::XML element that points a location element in the XML doc
    *
    *  @throw logic_error Thrown if second argument is not a pointer to a 'location' XML element
    */
    void LoadInstrument::setLocation(Geometry::IComponent* comp, Poco::XML::Element* pElem)
    {
      // Require that pElem points to an element with tag name 'location'

      if ( (pElem->tagName()).compare("location") )
      {
        g_log.error("Second argument to function setLocation must be a pointer to an XML element with tag name location.");
        throw std::logic_error( "Second argument to function setLocation must be a pointer to an XML element with tag name location." );
      }

      //Geometry::V3D pos;  // position for <location>


      comp->translate(getRelativeTranslation(comp, pElem));
      


      // Rotate coordinate system of this component

      if ( pElem->hasAttribute("rot") )
      {
        double rotAngle = m_angleConvertConst*atof( (pElem->getAttribute("rot")).c_str() ); // assumed to be in degrees

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

          Geometry::V3D posTrans;

          if (tElem)
          {
	     posTrans = getRelativeTranslation(comp, tElem);

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
          double rotAngle = m_angleConvertConst*atof( (rElem->getAttribute("val")).c_str() ); // assumed to be in degrees

          double axis_x = 0.0;
          double axis_y = 0.0;
          double axis_z = 1.0;

          if ( rElem->hasAttribute("axis-x") )
              axis_x = atof( (rElem->getAttribute("axis-x")).c_str() );
          if ( rElem->hasAttribute("axis-y") )
              axis_y = atof( (rElem->getAttribute("axis-y")).c_str() );
          if ( rElem->hasAttribute("axis-z") )
              axis_z = atof( (rElem->getAttribute("axis-z")).c_str() );

          comp->rotate(Geometry::Quat(rotAngle, Geometry::V3D(axis_x,axis_y,axis_z)));      

          // for recursive action
          pRecursive = rElem; 
          }

      } // end while

    }


    //-----------------------------------------------------------------------------------------------------------------------
    /** Get parent component element of location element.
    *
    *  @param pLocElem ::  Poco::XML element that points a location element in the XML doc
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


    //-----------------------------------------------------------------------------------------------------------------------
    /** Method for populating IdList.
    *
    *  @param pE ::  Poco::XML element that points to an \<idlist\>
    *  @param idList :: The structure to populate with detector ID numbers
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

      // set name of idlist

      idList.idname = pE->getAttribute("idname");

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
        idList.vec.reserve(endID-startID/increment);
        for (int i = startID; i != endID+increment; i += increment)
          idList.vec.push_back(i);
      }
      else
      {
        // test first if any <id> elements

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
              idList.vec.reserve(endID-startID/increment);
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



    //-----------------------------------------------------------------------------------------------------------------------
    /** Returns True if the (string) type given is an assembly.
     *
     *  @param type ::  name of the type of a component in XML instrument definition
     *  @return True if the type is an assembly
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


    //-----------------------------------------------------------------------------------------------------------------------
    /** Make the shape defined in 1st argument face the component in the second argument,
    *  by rotating the z-axis of the component passed in 1st argument so that it points in the
    *  direction: from the component as specified 2nd argument to the component as specified in 1st argument.
    *
    *  @param in ::  Component to be rotated
    *  @param facing :: Object to face
    */
    void LoadInstrument::makeXYplaneFaceComponent(Geometry::IComponent* &in, const Geometry::ObjComponent* facing)
    {
      const Geometry::V3D facingPoint = facing->getPos();

      makeXYplaneFaceComponent(in, facingPoint);
    }



    //-----------------------------------------------------------------------------------------------------------------------
    /** Make the shape defined in 1st argument face the position in the second argument,
    *  by rotating the z-axis of the component passed in 1st argument so that it points in the
    *  direction: from the position (as specified 2nd argument) to the component (1st argument).
    *
    *  @param in ::  Component to be rotated
    *  @param facingPoint :: position to face
    */
    void LoadInstrument::makeXYplaneFaceComponent(Geometry::IComponent* &in, const Geometry::V3D& facingPoint)
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


    //-----------------------------------------------------------------------------------------------------------------------
    /** Parse position of facing element to V3D
    *
    *  @param pElem ::  Facing type element to parse
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
        if ( pElem->hasAttribute("t") ) theta = m_angleConvertConst*atof((pElem->getAttribute("t")).c_str());
        if ( pElem->hasAttribute("p") ) phi = m_angleConvertConst*atof((pElem->getAttribute("p")).c_str());

        if ( pElem->hasAttribute("R") ) R = atof((pElem->getAttribute("R")).c_str());
        if ( pElem->hasAttribute("theta") ) theta = m_angleConvertConst*atof((pElem->getAttribute("theta")).c_str());
        if ( pElem->hasAttribute("phi") ) phi = m_angleConvertConst*atof((pElem->getAttribute("phi")).c_str());

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


    //-----------------------------------------------------------------------------------------------------------------------
    /** Set facing of comp as specified in XML facing element (which must be sub-element of a location element).
    *
    *  @param comp :: To set facing of
    *  @param pElem ::  Poco::XML element that points a location element in the XML doc
    *
    *  @throw logic_error Thrown if second argument is not a pointer to a 'location' XML element
    */
    void LoadInstrument::setFacing(Geometry::IComponent* comp, Poco::XML::Element* pElem)
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
          double rotAngle = m_angleConvertConst*atof( (facingElem->getAttribute("rot")).c_str() ); // assumed to be in degrees
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


    //-----------------------------------------------------------------------------------------------------------------------
    /** Set parameter/logfile info (if any) associated with component
    *
    *  @param comp :: Some component
    *  @param pElem ::  Associated Poco::XML element to component that may hold a \<parameter\> element
    *  @param logfileCache :: Cache to add information about parameter to
    *
    *  @throw InstrumentDefinitionError Thrown if issues with the content of XML instrument file
    */
    void LoadInstrument::setLogfile(const Geometry::IComponent* comp, Poco::XML::Element* pElem, 
      std::multimap<std::string, boost::shared_ptr<Geometry::XMLlogfile> >& logfileCache)
    {
      // check first if pElem contains any <parameter> child elements, however not if this method is called through
      // setComponentLinks() for example by the LoadParameter algorithm

      if ( hasParameterElement_beenSet )
        if ( hasParameterElement.end() == std::find(hasParameterElement.begin(),hasParameterElement.end(),pElem) ) return;

      NodeList* pNL_comp = pElem->childNodes(); // here get all child nodes
      unsigned int pNL_comp_length = pNL_comp->length();

      for (unsigned int i = 0; i < pNL_comp_length; i++)
      {
        // we are only interest in the top level parameter elements hence
        // the reason for the if statement below
        if ( (pNL_comp->item(i))->nodeType() == Node::ELEMENT_NODE &&
          ((pNL_comp->item(i))->nodeName()).compare("parameter") == 0 )
        {
          Element* pParamElem = static_cast<Element*>(pNL_comp->item(i));

          if ( !pParamElem->hasAttribute("name") )
            throw Kernel::Exception::InstrumentDefinitionError("XML element with name or type = " + comp->getName() +
            " contain <parameter> element with no name attribute in XML instrument file", m_filename);

          std::string paramName = pParamElem->getAttribute("name");

          if ( paramName.compare("rot")==0 || paramName.compare("pos")==0  )
          {
            g_log.error() << "XML element with name or type = " << comp->getName() <<
              " contains <parameter> element with name=\"" << paramName << "\"." <<
              " This is a reserved Mantid keyword. Please use other name, " <<
              "and see www.mantidproject.org/IDF for list of reserved keywords." <<
              " This parameter is ignored";
            continue;
          }

          std::string logfileID = "";
          std::string value = "";

          std::string type = "double"; // default
          std::string extractSingleValueAs = "mean"; // default
          std::string eq = "";

          NodeList* pNLvalue = pParamElem->getElementsByTagName("value");
          unsigned int numberValueEle = pNLvalue->length();
          Element* pValueElem;

          NodeList* pNLlogfile = pParamElem->getElementsByTagName("logfile");
          unsigned int numberLogfileEle = pNLlogfile->length();
          Element* pLogfileElem;

          NodeList* pNLLookUp = pParamElem->getElementsByTagName("lookuptable");
          unsigned int numberLookUp = pNLLookUp->length();

          NodeList* pNLFormula = pParamElem->getElementsByTagName("formula");
          unsigned int numberFormula = pNLFormula->length();

          if ( numberValueEle+numberLogfileEle+numberLookUp+numberFormula > 1 )
          {
            g_log.warning() << "XML element with name or type = " << comp->getName() <<
              " contains <parameter> element where the value of the parameter has been specified" <<
              " more than once. See www.mantidproject.org/IDF for how the value" <<
              " of the parameter is set in this case.";
          }

          if ( numberValueEle+numberLogfileEle+numberLookUp+numberFormula == 0 )
          {
            g_log.error() << "XML element with name or type = " << comp->getName() <<
              " contains <parameter> for which no value is specified." <<
              " See www.mantidproject.org/IDF for how to set the value" <<
              " of a parameter. This parameter is ignored.";
            continue;
          }

          // if more than one <value> specified for a parameter use only the first <value> element
          if ( numberValueEle >= 1 )
          {
            pValueElem = static_cast<Element*>(pNLvalue->item(0));
            if ( !pValueElem->hasAttribute("val") )
              throw Kernel::Exception::InstrumentDefinitionError("XML element with name or type = " + comp->getName() +
              " contains <parameter> element with invalid syntax for its subelement <value>." +
              " Correct syntax is <value val=\"\"/>", m_filename);
            value = pValueElem->getAttribute("val");
          }
          else if ( numberLogfileEle >= 1 )
          {
            // <logfile > tag was used at least once.
            pLogfileElem = static_cast<Element*>(pNLlogfile->item(0));
            if ( !pLogfileElem->hasAttribute("id") )
              throw Kernel::Exception::InstrumentDefinitionError("XML element with name or type = " + comp->getName() +
              " contains <parameter> element with invalid syntax for its subelement logfile>." +
              " Correct syntax is <logfile id=\"\"/>", m_filename);
            logfileID = pLogfileElem->getAttribute("id");

            if ( pLogfileElem->hasAttribute("eq") )
              eq = pLogfileElem->getAttribute("eq");
            if ( pLogfileElem->hasAttribute("extract-single-value-as") )
              extractSingleValueAs = pLogfileElem->getAttribute("extract-single-value-as");      
          }
          pNLlogfile->release();
          pNLvalue->release();


          if ( pParamElem->hasAttribute("type") )
            type = pParamElem->getAttribute("type");


          // check if <fixed /> element present

          bool fixed = false;
          NodeList* pNLFixed = pParamElem->getElementsByTagName("fixed");
          unsigned int numberFixed = pNLFixed->length();
          if ( numberFixed >= 1 )
          {
            fixed = true;
          }
          pNLFixed->release();

          // some processing

          std::string fittingFunction = "";
          std::string tie = "";

          if ( type.compare("fitting") == 0 )
          {
            size_t found = paramName.find(":");
            if (found!=std::string::npos)
            {
              // check that only one : in name
              size_t index = paramName.find(":", found+1); 
              if (index!=std::string::npos)
              {
                g_log.error() << "Fitting <parameter> in instrument definition file defined with" 
                  << " more than one column character :. One must used.\n";
              }
              else
              {
                fittingFunction = paramName.substr(0,found);
                paramName = paramName.substr(found+1, paramName.size());
              }
            }
          }

          if ( fixed )
          {
            std::ostringstream str;
            str << paramName << "=" << value;
            tie = str.str();
          }

          // check if <min> or <max> elements present

          std::vector<std::string> constraint(2, ""); 

          NodeList* pNLMin = pParamElem->getElementsByTagName("min");
          unsigned int numberMin = pNLMin->length();
          NodeList* pNLMax = pParamElem->getElementsByTagName("max");
          unsigned int numberMax = pNLMax->length();

          if ( numberMin >= 1)
          {
            Element* pMin = static_cast<Element*>(pNLMin->item(0));
            constraint[0] = pMin->getAttribute("val"); 
          }
          if ( numberMax >= 1)
          {
            Element* pMax = static_cast<Element*>(pNLMax->item(0));
            constraint[1] = pMax->getAttribute("val");
          }
          pNLMin->release();
          pNLMax->release();


          // check if penalty-factor> elements present

          std::string penaltyFactor; 

          NodeList* pNL_penaltyFactor = pParamElem->getElementsByTagName("penalty-factor");
          unsigned int numberPenaltyFactor =  pNL_penaltyFactor->length();

          if ( numberPenaltyFactor>= 1)
          {
            Element* pPenaltyFactor = static_cast<Element*>(pNL_penaltyFactor->item(0));
            penaltyFactor = pPenaltyFactor->getAttribute("val"); 
          }
          pNL_penaltyFactor->release();


          // Check if look up table is specified

          std::vector<std::string> allowedUnits = UnitFactory::Instance().getKeys();

          boost::shared_ptr<Interpolation> interpolation(new Interpolation);

          if ( numberLookUp >= 1 )
          {
            Element* pLookUp = static_cast<Element*>(pNLLookUp->item(0));

            if ( pLookUp->hasAttribute("interpolation") )
              interpolation->setMethod(pLookUp->getAttribute("interpolation"));
            if ( pLookUp->hasAttribute("x-unit") )
            {
              std::vector<std::string>::iterator it;
              it = find(allowedUnits.begin(), allowedUnits.end(), pLookUp->getAttribute("x-unit"));
              if ( it == allowedUnits.end() )
              {
                g_log.warning() << "x-unit used with interpolation table must be one of the recognised units " <<
                  " see http://www.mantidproject.org/Unit_Factory"; 
              }
              else
                interpolation->setXUnit(pLookUp->getAttribute("x-unit"));    
            }
            if ( pLookUp->hasAttribute("y-unit") )
            {
              std::vector<std::string>::iterator it;
              it = find(allowedUnits.begin(), allowedUnits.end(), pLookUp->getAttribute("y-unit"));
              if ( it == allowedUnits.end() )
              {
                g_log.warning() << "y-unit used with interpolation table must be one of the recognised units " <<
                  " see http://www.mantidproject.org/Unit_Factory"; 
              }
              else
                interpolation->setYUnit(pLookUp->getAttribute("y-unit"));  
            }

            NodeList* pNLpoint = pLookUp->getElementsByTagName("point");
            unsigned int numberPoint = pNLpoint->length();

            for ( unsigned int i = 0; i < numberPoint; i++)
            {
              Element* pPoint = static_cast<Element*>(pNLpoint->item(i));
              double x = atof( pPoint->getAttribute("x").c_str() );
              double y = atof( pPoint->getAttribute("y").c_str() );
              interpolation->addPoint(x,y); 
            }
            pNLpoint->release();
          }
          pNLLookUp->release();


          // Check if formula is specified

          std::string formula = "";
          std::string formulaUnit = "TOF";
          std::string resultUnit = "TOF";

          if ( numberFormula >= 1 )
          {
            Element* pFormula = static_cast<Element*>(pNLFormula->item(0));
            formula = pFormula->getAttribute("eq");   
            if ( pFormula->hasAttribute("unit") )
            {
              std::vector<std::string>::iterator it;
              it = find(allowedUnits.begin(), allowedUnits.end(), pFormula->getAttribute("unit"));
              if ( it == allowedUnits.end() )
              {
                g_log.warning() << "unit attribute used with formula must be one of the recognised units " <<
                  " see http://www.mantidproject.org/Unit_Factory"; 
              }
              else
                formulaUnit = pFormula->getAttribute("unit");  
            }
            if ( pFormula->hasAttribute("result-unit") )
              resultUnit = pFormula->getAttribute("result-unit");
          }
          pNLFormula->release();


          boost::shared_ptr<XMLlogfile> temp(new XMLlogfile(logfileID, value, interpolation, formula, formulaUnit, resultUnit, 
	    paramName, type, tie, constraint, penaltyFactor, fittingFunction, extractSingleValueAs, eq, comp, m_angleConvertConst));
          logfileCache.insert( std::pair<std::string,boost::shared_ptr<XMLlogfile> >(logfileID,temp));
        } // end of if statement
      }
      pNL_comp->release();
    }


    //-----------------------------------------------------------------------------------------------------------------------
    /** Apply parameters specified in \<component-link\> XML elements.
    *
    *  @param instrument :: Instrument
    *  @param pRootElem ::  Associated Poco::XML element to component that may hold a \<parameter\> element
    */
    void LoadInstrument::setComponentLinks(boost::shared_ptr<Geometry::Instrument>& instrument, Poco::XML::Element* pRootElem)
    {
      NodeList* pNL_link = pRootElem->getElementsByTagName("component-link");
      unsigned int numberLinks = pNL_link->length();


      // check if any logfile cache units set. As of this writing the only unit to check is if "angle=radian"
      std::map<std::string, std::string>& units = instrument->getLogfileUnit();
      std::map<std::string, std::string>::iterator unit_it;
      unit_it = units.find("angle");
      if ( unit_it != units.end() )
        if ( unit_it->second == "radian" )
          m_angleConvertConst = 180.0/M_PI;
      

      for (unsigned int iLink = 0; iLink < numberLinks; iLink++)
      {
        Element* pLinkElem = static_cast<Element*>(pNL_link->item(iLink));
        std::string name = pLinkElem->getAttribute("name");
        std::vector<boost::shared_ptr<Geometry::IComponent> > sharedIComp = instrument->getAllComponentsWithName(name);

        for (unsigned int i = 0; i < sharedIComp.size(); i++)
        {
          boost::shared_ptr<Geometry::Component> sharedComp = boost::dynamic_pointer_cast<Geometry::Component>(sharedIComp[i]);
          if ( sharedComp )
          {
            //Not empty Component
            if (sharedComp->isParametrized())
            {
              setLogfile(sharedComp->base(), pLinkElem, instrument->getLogfileCache());
            }
            else
            {
              setLogfile(sharedIComp[i].get(), pLinkElem, instrument->getLogfileCache());
            }
          }
        }
      }
      pNL_link->release();
    }




    //-----------------------------------------------------------------------------------------------------------------------
    /// Run the sub-algorithm LoadInstrument (or LoadInstrumentFromRaw)
    void LoadInstrument::runLoadParameterFile()
    {
      g_log.debug("Loading the parameter definition...");
      // Determine the search directory for XML parameter definition files
      Kernel::ConfigServiceImpl & configService = Kernel::ConfigService::Instance();
      std::string directoryName = configService.getString("parameterDefinition.directory");
      if (directoryName.empty())
      {
        // This is the assumed deployment directory for parameter files, where we need to be 
        // relative to the directory of the executable, not the current working directory.
        directoryName = Poco::Path(configService.getPropertiesDir()).resolve("../Instrument").toString();
      }

      // Remove the path from the filename
      const int stripPath = m_filename.find_last_of("\\/");
      std::string instrumentFile = m_filename.substr(stripPath+1,m_filename.size());
      // the ID is the bit in front of _Definition
      const int getID = instrumentFile.find("_Definition");
      std::string instrumentID = instrumentFile.substr(0,getID);

      // force ID to upper case
      std::transform(instrumentID.begin(), instrumentID.end(), instrumentID.begin(), toupper);
      std::string fullPathIDF = directoryName + "/" + instrumentID + "_Parameters.xml";

      IAlgorithm_sptr loadInstParam = createSubAlgorithm("LoadParameterFile");

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      try
      {
        loadInstParam->setPropertyValue("Filename", fullPathIDF);
        loadInstParam->setProperty<MatrixWorkspace_sptr> ("Workspace", m_workspace);
        loadInstParam->execute();
      } catch (std::invalid_argument& e)
      {
        g_log.information("Invalid argument to LoadParameter sub-algorithm");
        g_log.information(e.what());
      } catch (std::runtime_error& e)
      {
        g_log.information("Unable to successfully run LoadParameter sub-algorithm");
        g_log.information(e.what());
      }
    }



    //-----------------------------------------------------------------------------------------------------------------------
    /** get name of location element. Will be the name attribute, or the
     * parent's name attribute, or the parent's type, if all else fails.
    *
    *  @param pElem ::  Poco::XML element that points to a location element
    *  @return name of location element
    */
    std::string LoadInstrument::getNameOfLocationElement(Poco::XML::Element* pElem)
    {
      Element* pCompElem = getParentComponent(pElem);

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
