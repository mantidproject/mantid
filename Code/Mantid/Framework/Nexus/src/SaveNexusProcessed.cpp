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

using namespace Mantid::API;

namespace Mantid
{
namespace NeXus
{

  using namespace Kernel;
  using namespace API;
  using namespace DataObjects;
  using Geometry::IInstrument_const_sptr;

  // Register the algorithm into the algorithm factory
  DECLARE_ALGORITHM(SaveNexusProcessed)


  /// Empty default constructor
  SaveNexusProcessed::SaveNexusProcessed() :
  Algorithm(),
  m_spec_list(), m_spec_max(Mantid::EMPTY_INT())
  {
  }

  //-----------------------------------------------------------------------------------------------
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


  //-----------------------------------------------------------------------------------------------
  /** Executes the algorithm.
   *
   *  @throw runtime_error Thrown if algorithm cannot execute
   */
  void SaveNexusProcessed::exec()
  {
    Progress prog_init(this, 0.0, 0.3, 5);

    // Retrieve the filename from the properties
    m_filename = getPropertyValue("Filename");
    //m_entryname = getPropertyValue("EntryName");
    m_title = getPropertyValue("Title");
    m_inputWorkspace = getProperty("InputWorkspace");
    m_eventWorkspace = boost::dynamic_pointer_cast<const EventWorkspace>(m_inputWorkspace);
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
    if ((workspaceID.find("Workspace2D") == std::string::npos) &&
        !m_eventWorkspace)
      throw Exception::NotImplementedError("SaveNexusProcessed passed invalid workspaces. Must be Workspace2D or EventWorkspace.");


    NexusFileIO *nexusFile= new NexusFileIO();
    if( nexusFile->openNexusWrite( m_filename ) != 0 )
    {
      g_log.error("Failed to open file");
      throw Exception::FileError("Failed to open file", m_filename);
    }
    prog_init.reportIncrement(1, "Opening file");
    if( nexusFile->writeNexusProcessedHeader( m_title ) != 0 )
    {
      g_log.error("Failed to write file");
      throw Exception::FileError("Failed to write to file", m_filename);
    }
    prog_init.reportIncrement(1, "Writing header");

    // write instrument data, if present and writer enabled
    IInstrument_const_sptr instrument = m_inputWorkspace->getInstrument();
    nexusFile->writeNexusInstrument(instrument);
    prog_init.reportIncrement(1, "Writing instrument");

    nexusFile->writeNexusParameterMap(m_inputWorkspace);
    prog_init.reportIncrement(1, "Writing parameter map");

    // write XML source file name, if it exists - otherwise write "NoNameAvailable"
    std::string instrumentName=instrument->getName();
    if(instrumentName != "")
    { //TODO: NO ONE SHOULD ASSUME THIS STRUCTURE IN THE INSTRUMENT FILENAME! Fix this and any other such hard-coded instrument file names.
      // force ID to upper case
      std::transform(instrumentName.begin(), instrumentName.end(), instrumentName.begin(), toupper);
      std::string instrumentXml(instrumentName+"_Definition.xml");
      // Determine the search directory for XML instrument definition files (IDFs)
      std::string directoryName = Kernel::ConfigService::Instance().getInstrumentDirectory();

      Poco::File file(directoryName+"/"+instrumentXml);
      if(!file.exists())
        instrumentXml="NoXmlFileFound";
      std::string version("1"); // these should be read from xml file
      std::string date("20081031");
      nexusFile->writeNexusInstrumentXmlName(instrumentXml,date,version);
    }
    else
      nexusFile->writeNexusInstrumentXmlName("NoNameAvailable","","");

    if( nexusFile->writeNexusProcessedSample(m_inputWorkspace->sample().getName(), m_inputWorkspace->sample(),
        m_inputWorkspace->run()) != 0 )
    {
      g_log.error("Failed to write NXsample");
      throw Exception::FileError("Failed to write NXsample", m_filename);
    }
    prog_init.reportIncrement(1, "Writing sample");


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

    // Write out the data (2D or event)
    if (m_eventWorkspace)
    {
      //nexusFile->writeNexusProcessedDataEvent(m_eventWorkspace);
      this->execEvent(nexusFile);
    }
    else
    {
      nexusFile->writeNexusProcessedData2D(m_inputWorkspace,uniformSpectra,spec);
    }

    nexusFile->writeNexusProcessedProcess(m_inputWorkspace);
    // MW 27/10/10 - don't try and save the spectra-detector map if there isn't one
    if ( m_inputWorkspace->getAxis(1)->isSpectra() )
    {
      nexusFile->writeNexusProcessedSpectraMap(m_inputWorkspace, spec);
    }
    nexusFile->closeNexusFile();

    delete nexusFile;

    return;
  }






  //-------------------------------------------------------------------------------------
  /** Append out each field of a vector of events to separate array.
   *
   * @param events :: vector of TofEvent or WeightedEvent, etc.
   * @param offset :: where the first event goes in the array
   * @param tofs, weights, errorSquareds, pulsetimes :: arrays to write to.
   *        Must be initialized and big enough,
   *        or NULL if they are not meant to be written to.
   */
  template<class T>
  void SaveNexusProcessed::appendEventListData( std::vector<T> events, size_t offset, double * tofs, float * weights, float * errorSquareds, int64_t * pulsetimes)
  {
    // Do nothing if there are no events.
    size_t num = events.size();
    if (num <= 0)
      return;

    typename std::vector<T>::const_iterator it;
    typename std::vector<T>::const_iterator it_end = events.end();
    size_t i = offset;

    // Fill the C-arrays with the fields from all the events, as requested.
    for (it = events.begin(); it != it_end; it++)
    {
      if (tofs) tofs[i] = it->tof();
      if (weights) weights[i] = it->weight();
      if (errorSquareds) errorSquareds[i] = it->errorSquared();
      if (pulsetimes) pulsetimes[i] = it->pulseTime().total_nanoseconds();
      i++;
    }
  }



  //-----------------------------------------------------------------------------------------------
  /** Execute the saving of event data
   * */
  void SaveNexusProcessed::execEvent(NexusFileIO * nexusFile)
  {
    prog = new Progress(this, 0.3, 1.0,m_eventWorkspace->getNumberEvents());

    // Make a super long list of tofs, weights, etc.
    std::vector<size_t> indices;
    indices.reserve( m_eventWorkspace->getNumberHistograms()+1 );
    // First we need to index the events in each spectrum
    size_t index = 0;
    for (int wi =0; wi < m_eventWorkspace->getNumberHistograms(); wi++)
    {
      indices.push_back(index);
      // Track the total # of events
      index += m_eventWorkspace->getEventList(wi).getNumberEvents();
    }
    indices.push_back(index);

    // Initialize all the arrays
    size_t num = index;
    double * tofs = NULL;
    float * weights = NULL;
    float * errorSquareds = NULL;
    int64_t * pulsetimes = NULL;

    // overall event type.
    std::string eventType;
    EventType type = m_eventWorkspace->getEventType();
    bool writeTOF = true;
    bool writePulsetime = false;
    bool writeWeight = false;
    bool writeError = false;

    switch (type)
    {
    case TOF:
      eventType = "TOF";
      writePulsetime = true;
      break;
    case WEIGHTED:
      eventType = "WEIGHTED";
      writePulsetime = true;
      writeWeight = true;
      writeError = true;
      break;
    case WEIGHTED_NOTIME:
      eventType = "WEIGHTED_NOTIME";
      writeWeight = true;
      writeError = true;
      break;
    }

    // --- Initialize the combined event arrays ----
    if (writeTOF)
      tofs = new double[num];
    if (writeWeight)
      weights = new float[num];
    if (writeError)
      errorSquareds = new float[num];
    if (writePulsetime)
      pulsetimes = new int64_t[num];

    // --- Fill in the combined event arrays ----
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int wi=0; wi < m_eventWorkspace->getNumberHistograms(); wi++)
    {
      PARALLEL_START_INTERUPT_REGION
      const DataObjects::EventList & el = m_eventWorkspace->getEventList(wi);

      // This is where it will land in the output array.
      // It is okay to write in parallel since none should step on each other.
      size_t offset = indices[wi];

      switch (el.getEventType())
      {
      case TOF:
        eventType = "TOF";
        appendEventListData( el.getEvents(), offset, tofs, weights, errorSquareds, pulsetimes);
        break;
      case WEIGHTED:
        eventType = "WEIGHTED";
        appendEventListData( el.getWeightedEvents(), offset, tofs, weights, errorSquareds, pulsetimes);
        break;
      case WEIGHTED_NOTIME:
        eventType = "WEIGHTED_NOTIME";
        appendEventListData( el.getWeightedEventsNoTime(), offset, tofs, weights, errorSquareds, pulsetimes);
        break;
      }
      prog->reportIncrement(el.getNumberEvents(), "Copying EventList");

      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    // Write out to the NXS file
    nexusFile->writeNexusProcessedDataEventCompressed(m_eventWorkspace, indices, tofs, weights, errorSquareds, pulsetimes);

    // Free mem.
    delete [] tofs;
    delete [] weights;
    delete [] errorSquareds;
    delete [] pulsetimes;
  }

  //-----------------------------------------------------------------------------------------------
  /** virtual method to set the non workspace properties for this algorithm
   *  @param alg :: pointer to the algorithm
   *  @param propertyName :: name of the property
   *  @param propertyValue :: value  of the property
   *  @param perioidNum :: period number
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
