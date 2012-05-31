/*WIKI* 

Loads an instrument definition file ([[InstrumentDefinitionFile|IDF]]) into a workspace, which contains information about detector positions, their geometric shape, slit properties, links between values stored in ISIS log-files and components of the instrument and so on. For more on IDFs see: [[InstrumentDefinitionFile]].

By default the algorithm will write a 1:1 map between the spectrum number and detector ID. Any custom loading algorithm that calls this as a sub algorithm will therefore get this 1:1 map be default. If the custom loader is to write its own map then it is advised to set <code>RewriteSpectraMap</code> to false to avoid extra work. 


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/Progress.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadParameterFile.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheReader.h"
#include "MantidGeometry/Rendering/vtkGeometryCacheWriter.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Interpolation.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"

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
#include <fstream>
#include <iostream>
#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"

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
    
    /// Sets documentation strings for this algorithm
    void LoadInstrument::initDocs()
    {
      this->setWikiSummary(" Loads an Instrument Definition File ([[InstrumentDefinitionFile|IDF]]) into a [[workspace]]. After the IDF has been read this algorithm will attempt to run the sub-algorithm [[LoadParameterFile]]; where if IDF filename is of the form IDENTIFIER_Definition.xml then the instrument parameters in the file named IDENTIFIER_Parameters.xml would be loaded (in the directory specified by the parameterDefinition.directory [[Properties_File|Mantid property]]). ");
      this->setOptionalMessage("Loads an Instrument Definition File (IDF) into a workspace. After the IDF has been read this algorithm will attempt to run the sub-algorithm LoadParameterFile; where if IDF filename is of the form IDENTIFIER_Definition.xml then the instrument parameters in the file named IDENTIFIER_Parameters.xml would be loaded (in the directory specified by the parameterDefinition.directory Mantid property).");
    }
    

    using namespace Kernel;
    using namespace API;
    using namespace Geometry;

    /// Empty default constructor
    LoadInstrument::LoadInstrument() : Algorithm()
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
      declareProperty(new ArrayProperty<detid_t>("MonitorList",Direction::Output),
        "List of detector ids of monitors loaded in to the workspace");
      declareProperty("InstrumentName", "",
        "Name of instrument. Can be used instead of Filename to specify an IDF" );
      declareProperty("InstrumentXML","","The full XML description of the instrument."
                      "If given, the instrument is constructed from this instead of from an IDF."
                      "The InstrumentName property must also be set.");
      declareProperty("RewriteSpectraMap", true, "If true then the spectra-detector mapping "
                      "for the input workspace will be overwritten with a 1:1 map of spectrum "
                      "number to detector ID");
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
      m_filename = getPropertyValue("Filename");
      m_instName = getPropertyValue("InstrumentName");

      // We will parse the XML using the InstrumentDefinitionParser
      InstrumentDefinitionParser parser;

      // If the XML is passed in via the InstrumentXML property, use that.
      const Property * const InstrumentXML = getProperty("InstrumentXML");
      if ( ! InstrumentXML->isDefault() )
      {
        // We need the instrument name to be set as well because, for whatever reason,
        //   this isn't pulled out of the XML.
        if ( m_instName.empty() )
          throw std::runtime_error("The InstrumentName property must be set when using the InstrumentXML property.");
        // If the Filename property is not set, set it to the same as the instrument name
        if ( m_filename.empty() ) m_filename = m_instName;

        // Initialize the parser. Avoid copying the xmltext out of the property here.
        parser.initialize(m_filename, m_instName,
            *dynamic_cast<const PropertyWithValue<std::string>*>(InstrumentXML) );
      }
      // otherwise we need either Filename or InstrumentName to be set
      else
      {
        // Retrieve the filename from the properties
        if ( m_filename.empty() )
        {
          g_log.debug() << "=====> A1" << std::endl;

          // look to see if an Instrument name provided in which case create
          // IDF filename on the fly
          if ( m_instName.empty() )
          {
            g_log.error("Either the InstrumentName or Filename property of LoadInstrument most be specified");
            throw Kernel::Exception::FileError("Either the InstrumentName or Filename property of LoadInstrument most be specified to load an IDF" , m_filename);
          }
          else
          {
            const std::string date = m_workspace->getWorkspaceStartDate();
            m_filename = ExperimentInfo::getInstrumentFilename(m_instName,date);

            g_log.debug() << "=====> A2 - Filename is " << m_filename << std::endl;
          }
        }

        if (!m_filename.empty())
        {
          // Remove the path from the filename for use with the InstrumentDataService
          const std::string::size_type stripPath = m_filename.find_last_of("\\/");
          std::string instrumentFile = m_filename.substr(stripPath+1,m_filename.size());
          // Strip off "_Definition.xml"
          m_instName = instrumentFile.substr(0,instrumentFile.find("_Def"));

          g_log.debug() << "=====> A3 - Instrument is " << m_instName << std::endl;
        }

        g_log.debug() << "=====> m_filename = " << m_filename << std::endl;

        // Initialize the parser with the the XML text loaded from the IDF file
        parser.initialize(m_filename, m_instName, Strings::loadFile(m_filename));
      }

      // Find the mangled instrument name that includes the modified date
      std::string instrumentNameMangled = parser.getMangledName();
      g_log.debug() << "Instrument Mangled Name = " << instrumentNameMangled << std::endl;

      Instrument_sptr instrument;
      // Check whether the instrument is already in the InstrumentDataService
      if ( InstrumentDataService::Instance().doesExist(instrumentNameMangled) )
      {
          // If it does, just use the one from the one stored there
          g_log.debug() << "Instrument definition already loaded, using cached version.";
          instrument = InstrumentDataService::Instance().retrieve(instrumentNameMangled);
      }
      else
      {
          g_log.debug() << "Loading instrument XML...";
          // Really create the instrument
          Progress * prog = new Progress(this, 0, 1, 100);
          instrument = parser.parseXML(prog);
          delete prog;
          g_log.debug() << "...done!" << std::endl;
          // Add to data service for later retrieval
          InstrumentDataService::Instance().add(instrumentNameMangled, instrument);
      }


      // Add the instrument to the workspace
      m_workspace->setInstrument(instrument);

      // populate parameter map of workspace 
      m_workspace->populateInstrumentParameters();

      // check if default parameter file is also present, unless loading from
      if (!m_filename.empty())
        runLoadParameterFile();
      
      // Set the monitors output property
      setProperty("MonitorList",instrument->getMonitors());

      // Rebuild the spectra map for this workspace so that it matches the instrument
      // if required
      const bool rewriteSpectraMap = getProperty("RewriteSpectraMap");
      if( rewriteSpectraMap )
        m_workspace->rebuildSpectraMapping();
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
        directoryName = Poco::Path(configService.getPropertiesDir()).resolve("../instrument").toString();
      }

      // Remove the path from the filename
      const std::string::size_type stripPath = m_filename.find_last_of("\\/");
      std::string instrumentFile = m_filename.substr(stripPath+1,m_filename.size());
      // the ID is the bit in front of _Definition
      const std::string::size_type getID(instrumentFile.find("_Definition"));
      std::string instrumentID = instrumentFile.substr(0,getID);

      // force ID to upper case
      std::transform(instrumentID.begin(), instrumentID.end(), instrumentID.begin(), toupper);
      std::string fullPathIDF = directoryName + "/" + instrumentID + "_Parameters.xml";

      g_log.debug() << "Parameter file: " << fullPathIDF << std::endl;
      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      try
      {
        // To allow the use of ExperimentInfo instead of workspace, we call it manually
        LoadParameterFile::execManually(fullPathIDF, m_workspace);
        g_log.debug("Parameters loaded successfully.");
      } catch (std::invalid_argument& e)
      {
        g_log.information("LoadParameterFile: No parameter file found for this instrument");
        g_log.information(e.what());
      } catch (std::runtime_error& e)
      {
        g_log.information("Unable to successfully run LoadParameterFile sub-algorithm");
        g_log.information(e.what());
      }
    }

  } // namespace DataHandling
} // namespace Mantid
