// SaveNexusProcessed
// @author Ronald Fowler, based on SaveNexus
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/SaveNexusProcessed.h"
#include "MantidNexus/NexusFileIO.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ConfigService.h"

#include "Poco/File.h"
#include "Poco/Path.h"

#include <cmath>
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace NeXus
{

  // Register the algorithm into the algorithm factory
  DECLARE_ALGORITHM(SaveNexusProcessed)

  using namespace Kernel;
  using namespace API;
  using namespace DataObjects;

  Logger& SaveNexusProcessed::g_log = Logger::get("SaveNexusProcessed");

  /// Empty default constructor
  SaveNexusProcessed::SaveNexusProcessed() :
    Algorithm(),
    m_spec_list(), m_spec_max(unSetInt)
  {
  }

  /** Initialisation method.
   *
   */
  void SaveNexusProcessed::init()
  {
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
      "Name of the workspace to be saved");
    // Declare required input parameters for algorithm
    std::vector<std::string> exts;
    exts.push_back("NXS");
    exts.push_back("nxs");
    exts.push_back("nx5");
    exts.push_back("NX5");
    exts.push_back("xml");
    exts.push_back("XML");
    declareProperty("FileName","",new FileValidator(exts,false),
      "The name of the Nexus file to write, as a full or relative\n"
      "path");
    // Declare optional parameters (title now optional, was mandatory)
    declareProperty("Title", "", new NullValidator<std::string>,
      "A title to describe the saved workspace");
    BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
    mustBePositive->setLower(0);
    declareProperty("spectrum_min", 0, mustBePositive->clone(), 
      "Index number of first spectrum to read, only for single\n"
      "period data. Not yet implemented");
    declareProperty("spectrum_max", unSetInt, mustBePositive->clone(),
      "Index of last spectrum to read, only for single period\n"
      "data. Not yet implemented");
    declareProperty(new ArrayProperty<int>("spectrum_list"),
      "List of spectrum numbers to read, only for single period\n"
      "data. Not yet implemented");
    declareProperty("EntryNumber", 0, mustBePositive);
  }

  /** Executes the algorithm. Reading in the file and creating and populating
   *  the output workspace
   *
   *  @throw runtime_error Thrown if algorithm cannot execute
   */
  void SaveNexusProcessed::exec()
  {
    // Retrieve the filename from the properties
    m_filename = getPropertyValue("FileName");
    //m_entryname = getPropertyValue("EntryName");
    m_title = getPropertyValue("Title");
    m_inputWorkspace = getProperty("InputWorkspace");

    m_spec_list = getProperty("spectrum_list");
    m_spec_max = getProperty("spectrum_max");
    m_list = !m_spec_list.empty();
    m_interval = (m_spec_max != unSetInt);
    if ( m_spec_max == unSetInt ) m_spec_max = 0;

    const std::string workspaceID = m_inputWorkspace->id();
    if (workspaceID.find("Workspace2D") == std::string::npos )
        throw Exception::NotImplementedError("SaveNexusProcessed passed invalid workspaces.");

    NexusFileIO *nexusFile= new NexusFileIO();
    if( nexusFile->openNexusWrite( m_filename ) != 0 )
    {
       g_log.error("Failed to open file");
       throw Exception::FileError("Failed to open file", m_filename);
    }

    if( nexusFile->writeNexusProcessedHeader( m_title ) != 0 )
    {
       g_log.error("Failed to write file");
       throw Exception::FileError("Failed to write to file", m_filename);
    }

    // write instrument data, if present and writer enabled
    IInstrument_const_sptr instrument = m_inputWorkspace->getInstrument();
    nexusFile->writeNexusInstrument(instrument);

    nexusFile->writeNexusParameterMap(m_inputWorkspace);

    // write XML source file name, if it exists - otherwise write "NoNameAvailable"
    std::string instrumentName=instrument->getName();
    if(instrumentName != "")
    {
       std::string inst3Char = instrumentName.substr(0,3);  // get the first 3 letters of the name
       // force ID to upper case
       std::transform(inst3Char.begin(), inst3Char.end(), inst3Char.begin(), toupper);
       std::string instrumentXml(inst3Char+"_Definition.xml");
       // Determine the search directory for XML instrument definition files (IDFs)
       std::string directoryName = Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");
       if ( directoryName.empty() )
       {
	 // This is the assumed deployment directory for IDFs, where we need to be relative to the
	 // directory of the executable, not the current working directory.
	 directoryName = Poco::Path(Mantid::Kernel::ConfigService::Instance().getBaseDir()).resolve("../Instrument").toString();  
       }
       Poco::File file(directoryName+"/"+instrumentXml);
       if(!file.exists())
           instrumentXml="NoXmlFileFound";
       std::string version("1"); // these should be read from xml file
       std::string date("20081031");
       nexusFile->writeNexusInstrumentXmlName(instrumentXml,date,version);
    }
    else
       nexusFile->writeNexusInstrumentXmlName("NoNameAvailable","","");


    boost::shared_ptr<Mantid::API::Sample> sample=m_inputWorkspace->getSample();
    if( nexusFile->writeNexusProcessedSample( sample->getName(), sample) != 0 )
    {
       g_log.error("Failed to write NXsample");
       throw Exception::FileError("Failed to write NXsample", m_filename);
    }

    const int numberOfHist = m_inputWorkspace->getNumberHistograms();
    // check if all X() are in fact the same array
    bool uniformSpectra= API::WorkspaceHelpers::commonBoundaries(m_inputWorkspace);
    m_spec_min=0;
    m_spec_max=numberOfHist-1;
    if( m_interval )
    {
      m_spec_min = getProperty("spectrum_min");
      m_spec_max = getProperty("spectrum_max");
      if ( m_spec_max < m_spec_min || m_spec_max > numberOfHist-1 )
      {
        g_log.error("Invalid Spectrum min/max properties");
        throw std::invalid_argument("Inconsistent properties defined");
      }
    }
    nexusFile->writeNexusProcessedData(m_inputWorkspace,uniformSpectra,m_spec_min,m_spec_max);
    nexusFile->writeNexusProcessedProcess(m_inputWorkspace);
    const SpectraDetectorMap& spectraMap=m_inputWorkspace->spectraMap();
    nexusFile->writeNexusProcessedSpectraMap(spectraMap, m_spec_min, m_spec_max);
    nexusFile->closeNexusFile();
    
    delete nexusFile;

    return;
  }

} // namespace NeXus
} // namespace Mantid
