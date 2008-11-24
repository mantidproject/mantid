// SaveNexusProcessed
// @author Ronald Fowler, based on SaveNexus
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/SaveNexusProcessed.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidNexus/NexusFileIO.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ConfigService.h"

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
  SaveNexusProcessed::SaveNexusProcessed()
  {
  }

  /** Initialisation method.
   *
   */
  void SaveNexusProcessed::init()
  {
    // Declare required input parameters for algorithm
    std::vector<std::string> exts;
    exts.push_back("NXS");
    exts.push_back("nxs");
    exts.push_back("nx5");
    exts.push_back("NX5");
    exts.push_back("xml");
    exts.push_back("XML");
    //declareProperty("FileName","",new FileValidator(exts));
    declareProperty("FileName","",new MandatoryValidator<std::string>);

    declareProperty("Title","",new MandatoryValidator<std::string>);
    declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace","",Direction::Input));
    // declare optional parameters
    // Declare optional input parameters
    BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
    mustBePositive->setLower(0);
    declareProperty("EntryNumber",0,mustBePositive);
    declareProperty("spectrum_min",0, mustBePositive->clone());
    declareProperty("spectrum_max",0, mustBePositive->clone());
    declareProperty(new ArrayProperty<int>("spectrum_list"));
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

    Property *specList = getProperty("spectrum_list");
    m_list = !(specList->isDefault());
    Property *specMax = getProperty("spectrum_max");
    m_interval = !(specMax->isDefault());
    const std::string workspaceID = m_inputWorkspace->id();
    if (workspaceID != "Workspace2D")
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
    boost::shared_ptr<Instrument> instrument = m_inputWorkspace->getInstrument();
    nexusFile->writeNexusInstrument(instrument);

    // write XML source file name, if it exists - otherwise write "NoNameAvailable"
    std::string instrumentName=instrument->getName();
    if(instrumentName != "")
    {
       std::string inst3Char = instrumentName.substr(0,3);  // get the first 3 letters of the name
       // force ID to upper case
       std::transform(inst3Char.begin(), inst3Char.end(), inst3Char.begin(), toupper);
       std::string instrumentXml(inst3Char+"_definition.xml");
       // Determine the search directory for XML instrument definition files (IDFs)
       std::string directoryName = Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");      
       if ( directoryName.empty() ) directoryName = "../Instrument";
       boost::filesystem::path file(directoryName+"/"+instrumentXml);
       if(!boost::filesystem::exists(file))
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

    const Workspace2D_sptr localworkspace = boost::dynamic_pointer_cast<Workspace2D>(m_inputWorkspace);
    const int numberOfHist = localworkspace->getNumberHistograms();
    // check if all X() are in fact the same array
    bool uniformSpectra= API::WorkspaceHelpers::commonBoundaries(localworkspace);
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
    nexusFile->writeNexusProcessedData(localworkspace,uniformSpectra,m_spec_min,m_spec_max);
    nexusFile->writeNexusProcessedProcess(localworkspace);
	nexusFile->closeNexusFile();

    return;
  }

} // namespace NeXus
} // namespace Mantid
