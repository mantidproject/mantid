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
#include "MantidAPI/FileProperty.h"

#include "Poco/File.h"
#include "Poco/Path.h"

#include <cmath>
#include <boost/shared_ptr.hpp>


namespace Mantid
{
namespace NeXus
{

  using namespace Kernel;
  using namespace API;
  using Geometry::IInstrument_const_sptr;

  // Register the algorithm into the algorithm factory
  DECLARE_ALGORITHM(SaveNexusProcessed)


  /// Empty default constructor
  SaveNexusProcessed::SaveNexusProcessed() :
  Algorithm(),
  m_spec_list(), m_spec_max(Mantid::EMPTY_INT())
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
    exts.push_back(".nxs");
    exts.push_back(".nx5");
    exts.push_back(".xml");

    declareProperty(new FileProperty("Filename", "", FileProperty::Save, exts),
        "The name of the Nexus file to write, as a full or relative\n"
        "path");

    // Declare optional parameters (title now optional, was mandatory)
    declareProperty("Title", "", new NullValidator<std::string>,
        "A title to describe the saved workspace");
    BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
    mustBePositive->setLower(0);

    declareProperty("WorkspaceIndexMin", 0, mustBePositive->clone(), 
        "Index number of first spectrum to write, only for single\n"
        "period data. Not yet implemented");
    declareProperty("WorkspaceIndexMax", Mantid::EMPTY_INT(), mustBePositive->clone(),
        "Index of last spectrum to write, only for single period\n"
        "data. Not yet implemented");
    declareProperty(new ArrayProperty<int>("WorkspaceIndexList"),
        "List of spectrum numbers to read, only for single period\n"
        "data. Not yet implemented");
    //declareProperty("EntryNumber", 0, mustBePositive);
    declareProperty("Append",false,"Determines whether .nxs file needs to be\n"
        "over written or appended");
  }

  /** Executes the algorithm.
   *
   *  @throw runtime_error Thrown if algorithm cannot execute
   */
  void SaveNexusProcessed::exec()
  {
    // Retrieve the filename from the properties
    m_filename = getPropertyValue("Filename");
    //m_entryname = getPropertyValue("EntryName");
    m_title = getPropertyValue("Title");
    m_inputWorkspace = getProperty("InputWorkspace");
    // If no title's been given, use the workspace title field
    if (m_title.empty()) m_title = m_inputWorkspace->getTitle();
    // If we don't want to append then remove the file if it already exists
    bool append_to_file = getProperty("Append");
    if( !append_to_file )
    {
      Poco::File file(m_filename);
      if( file.exists() )
      {
        file.remove();
      }
    }

    m_spec_list = getProperty("WorkspaceIndexList");
    m_spec_max = getProperty("WorkspaceIndexMax");
    m_list = !m_spec_list.empty();
    m_interval = (m_spec_max != Mantid::EMPTY_INT());
    if ( m_spec_max == Mantid::EMPTY_INT() ) m_spec_max = 0;

    const std::string workspaceID = m_inputWorkspace->id();
    if (workspaceID.find("Workspace2D") == std::string::npos )
      throw Exception::NotImplementedError("SaveNexusProcessed passed invalid workspaces.");


    NexusFileIO *nexusFile= new NexusFileIO();
    if( nexusFile->openNexusWrite( m_filename ) != 0 )
    {
      g_log.error("Failed to open file");
      throw Exception::FileError("Failed to open file", m_filename);
    }
    progress(0.2);
    if( nexusFile->writeNexusProcessedHeader( m_title ) != 0 )
    {
      g_log.error("Failed to write file");
      throw Exception::FileError("Failed to write to file", m_filename);
    }


    progress(0.3);
    // write instrument data, if present and writer enabled
    IInstrument_const_sptr instrument = m_inputWorkspace->getInstrument();
    nexusFile->writeNexusInstrument(instrument);

    nexusFile->writeNexusParameterMap(m_inputWorkspace);

    progress(0.5);


    // write XML source file name, if it exists - otherwise write "NoNameAvailable"
    std::string instrumentName=instrument->getName();
    if(instrumentName != "")
    {
      // force ID to upper case
      std::transform(instrumentName.begin(), instrumentName.end(), instrumentName.begin(), toupper);
      std::string instrumentXml(instrumentName+"_Definition.xml");
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

    progress(0.7);
    if( nexusFile->writeNexusProcessedSample(m_inputWorkspace->sample().getName(), m_inputWorkspace->sample(),
        m_inputWorkspace->run()) != 0 )
    {
      g_log.error("Failed to write NXsample");
      throw Exception::FileError("Failed to write NXsample", m_filename);
    }

    const int numberOfHist = m_inputWorkspace->getNumberHistograms();
    // check if all X() are in fact the same array
    bool uniformSpectra= API::WorkspaceHelpers::commonBoundaries(m_inputWorkspace);
    std::vector<int> spec;
    if( m_interval )
    {
      m_spec_min = getProperty("WorkspaceIndexMin");
      m_spec_max = getProperty("WorkspaceIndexMax");
      if ( m_spec_max < m_spec_min || m_spec_max > numberOfHist-1 )
      {
        g_log.error("Invalid WorkspaceIndex min/max properties");
        throw std::invalid_argument("Inconsistent properties defined");
      }
      spec.reserve(1+m_spec_max-m_spec_min);
      for(int i=m_spec_min;i<=m_spec_max;i++)
        spec.push_back(i);
      if (m_list)
      {
        for(size_t i=0;i<m_spec_list.size();i++)
        {
          int s = m_spec_list[i];
          if ( s < 0 ) continue;
          if (s < m_spec_min || s > m_spec_max)
            spec.push_back(s);
        }
      }
    }
    else if (m_list)
    {
      m_spec_max=0;
      m_spec_min=numberOfHist-1;
      for(size_t i=0;i<m_spec_list.size();i++)
      {
        int s = m_spec_list[i];
        if ( s < 0 ) continue;
        spec.push_back(s);
        if (s > m_spec_max) m_spec_max = s;
        if (s < m_spec_min) m_spec_min = s;
      }
    }
    else
    {
      m_spec_min=0;
      m_spec_max=numberOfHist-1;
      spec.reserve(1+m_spec_max-m_spec_min);
      for(int i=m_spec_min;i<=m_spec_max;i++)
        spec.push_back(i);
    }
    nexusFile->writeNexusProcessedData(m_inputWorkspace,uniformSpectra,spec);
    nexusFile->writeNexusProcessedProcess(m_inputWorkspace);
    nexusFile->writeNexusProcessedSpectraMap(m_inputWorkspace, spec);
    nexusFile->closeNexusFile();
    progress(1.0);

    delete nexusFile;

    return;
  }

  /** virtual method to set the non workspace properties for this algorithm
   *  @param alg pointer to the algorithm
   *  @param propertyName name of the property
   *  @param propertyValue value  of the property
   *  @param perioidNum period number
   */
  void SaveNexusProcessed::setOtherProperties(IAlgorithm* alg,const std::string& propertyName,const std::string& propertyValue,int perioidNum)
  {
    if(!propertyName.compare("Append"))
    {	if(perioidNum!=1)
    { alg->setPropertyValue(propertyName,"1");
    }
    else alg->setPropertyValue(propertyName,propertyValue);
    }
    else
      Algorithm::setOtherProperties(alg,propertyName,propertyValue,perioidNum);
  }

} // namespace NeXus
} // namespace Mantid
