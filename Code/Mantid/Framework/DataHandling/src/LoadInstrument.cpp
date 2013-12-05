/*WIKI* 

Loads an instrument definition file ([[InstrumentDefinitionFile|IDF]]) into a workspace, which contains information about detector positions, their geometric shape, slit properties, links between values stored in ISIS log-files and components of the instrument and so on. For more on IDFs see: [[InstrumentDefinitionFile]].

By default the algorithm will write a 1:1 map between the spectrum number and detector ID. Any custom loading algorithm that calls this as a Child Algorithm will therefore get this 1:1 map be default. If the custom loader is to write its own map then it is advised to set <code>RewriteSpectraMap</code> to false to avoid extra work. 

The instrument to load can be specified by either the InstrumentXML, Filename and InstrumentName properties (given here in order of precedence if more than one is set). At present, if the InstrumentXML is used the InstrumentName property should also be set.

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
      this->setWikiSummary("Loads an Instrument Definition File ([[InstrumentDefinitionFile|IDF]]) into a [[workspace]]. This algorithm is typically run as a child algorithm to the data loading algorithm, rather than as a top-level algorithm. After the IDF has been read this algorithm will attempt to run the child algorithm [[LoadParameterFile]]; where if IDF filename is of the form IDENTIFIER_Definition.xml then the instrument parameters in the file named IDENTIFIER_Parameters.xml would be loaded (in the directory specified by the ParameterDefinition.directory [[Properties_File|Mantid property]]).");
      this->setOptionalMessage("Loads an Instrument Definition File (IDF) into a workspace. After the IDF has been read this algorithm will attempt to run the Child Algorithm LoadParameterFile; where if IDF filename is of the form IDENTIFIER_Definition.xml then the instrument parameters in the file named IDENTIFIER_Parameters.xml would be loaded (in the directory specified by the parameterDefinition.directory Mantid property).");
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
      // When used as a Child Algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
      declareProperty(
        new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut),
        "The name of the workspace to load the instrument definition into. Any existing instrument will be replaced." );
      declareProperty(new FileProperty("Filename","", FileProperty::OptionalLoad, ".xml"),
        "The filename (including its full or relative path) of an instrument definition file. The file extension must either be .xml or .XML when specifying an instrument definition file. Note Filename or InstrumentName must be specified but not both.");
      declareProperty(new ArrayProperty<detid_t>("MonitorList",Direction::Output),
        "Will be filled with a list of the detector ids of any monitors loaded in to the workspace.");
      declareProperty("InstrumentName", "",
        "Name of instrument. Can be used instead of Filename to specify an IDF" );
      declareProperty("InstrumentXML","","The full XML instrument definition as a string.");
      declareProperty("RewriteSpectraMap", true, "If true then a 1:1 map between the spectrum numbers and detector/monitor IDs is set up as follows: the detector/monitor IDs in the IDF are ordered from smallest to largest number and then assigned in that order to the spectra in the workspace. For example if the IDF has defined detectors/monitors with ID = 1, 5 and 10 and the workspace contains 3 spectra with numbers 1,2,3 (and workspace indices 0,1, and 2) then spectrum number 1 is associated with det ID=1, spectrum number 2 with det ID=5 and spectrum number 3 with det ID=10");
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
          }
        }

        if ( m_filename.empty() )
        {
          throw Exception::NotFoundError("Unable to find an Instrument Definition File for",m_instName);
        }

        // Remove the path from the filename for use with the InstrumentDataService
        const std::string::size_type stripPath = m_filename.find_last_of("\\/");
        std::string instrumentFile = m_filename.substr(stripPath+1,m_filename.size());
        // Strip off "_Definition.xml"
        m_instName = instrumentFile.substr(0,instrumentFile.find("_Def"));

        // Initialize the parser with the the XML text loaded from the IDF file
        parser.initialize(m_filename, m_instName, Strings::loadFile(m_filename));
      }

      // Find the mangled instrument name that includes the modified date
      std::string instrumentNameMangled = parser.getMangledName();

      Instrument_sptr instrument;
      // Check whether the instrument is already in the InstrumentDataService
      if ( InstrumentDataService::Instance().doesExist(instrumentNameMangled) )
      {
        // If it does, just use the one from the one stored there
        instrument = InstrumentDataService::Instance().retrieve(instrumentNameMangled);
      }
      else
      {
          // Really create the instrument
          Progress * prog = new Progress(this, 0, 1, 100);
          instrument = parser.parseXML(prog);
          delete prog;
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
    /// Run the Child Algorithm LoadInstrument (or LoadInstrumentFromRaw)
    void LoadInstrument::runLoadParameterFile()
    {
      g_log.debug("Loading the parameter definition...");

      // First search for XML parameter file in same folder as IDF file
      const std::string::size_type dir_end = m_filename.find_last_of("\\/");
      std::string directoryName = m_filename.substr(0,dir_end+1); // include final '/'.
      std::string fullPathParamIDF = getFullPathParamIDF( directoryName );

      if( fullPathParamIDF.empty() )
      {
        // Not found, so search the other places were it may occur
        Kernel::ConfigServiceImpl & configService = Kernel::ConfigService::Instance();
        std::string directoryName = configService.getString("parameterDefinition.directory");
        if (directoryName.empty())
        {
          // This is the assumed deployment directory for parameter files, where we need to be 
          // relative to the directory of the executable, not the current working directory.
          directoryName = Poco::Path(configService.getPropertiesDir()).resolve("../instrument").toString();
        }
        fullPathParamIDF = getFullPathParamIDF( directoryName );
      }

      if(!fullPathParamIDF.empty()) {

        g_log.debug() << "Parameter file: " << fullPathParamIDF << std::endl;
        // Now execute the Child Algorithm. Catch and log any error, but don't stop.
        try
        {
          // To allow the use of ExperimentInfo instead of workspace, we call it manually
          LoadParameterFile::execManually(false, fullPathParamIDF, "", m_workspace);
          g_log.debug("Parameters loaded successfully.");
        } catch (std::invalid_argument& e)
        {
          g_log.information("LoadParameterFile: No parameter file found for this instrument");
          g_log.information(e.what());
        } catch (std::runtime_error& e)
        {
          g_log.information("Unable to successfully run LoadParameterFile Child Algorithm");
          g_log.information(e.what());
        }
      } else {
        g_log.information("No parameter file found for this instrument");
      }
    }

    //-----------------------------------------------------------------------------------------------------------------------
    /// Search the directory for the Parameter IDF file and return full path name if found, else return "".
    //  directoryName must include a final '/'.
    std::string LoadInstrument::getFullPathParamIDF( std::string directoryName )
    {
      // Remove the path from the filename
      const std::string::size_type stripPath = m_filename.find_last_of("\\/");
      std::string instrumentFile = m_filename.substr(stripPath+1,m_filename.size());

      // First check whether there is a parameter file whose name is the same as the IDF file,
      // but with 'Parameters' instead of 'Definition'.
      std::string definitionPart("_Definition");
      const std::string::size_type prefix_end(instrumentFile.find(definitionPart));
      const std::string::size_type suffix_start = prefix_end + definitionPart.length();
      // Make prefix and force it to be upper case
      std::string prefix = instrumentFile.substr(0, prefix_end);
      std::transform(prefix.begin(), prefix.end(), prefix.begin(), toupper); 
      // Make suffix ensuring it has positive length
      std::string suffix = ".xml";
      if( suffix_start < instrumentFile.length() )
      {
        suffix = instrumentFile.substr(suffix_start, std::string::npos );
      } 

      // Assemble parameter file name
      std::string fullPathParamIDF = directoryName + prefix + "_Parameters" + suffix;
      if( Poco::File(fullPathParamIDF).exists() == false) 
      { // No such file exists, so look for file based on instrument ID given by the prefix
        fullPathParamIDF = directoryName + "/" + prefix + "_Parameters.xml";
      }

      if( Poco::File(fullPathParamIDF).exists() == false) 
      { // No such file exists, indicate none found in this directory.
        fullPathParamIDF="";
      }

      return fullPathParamIDF;
    }

  } // namespace DataHandling
} // namespace Mantid
