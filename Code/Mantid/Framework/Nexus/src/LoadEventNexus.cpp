//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadEventNexus.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/Instrument/CompAssembly.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Timer.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidAPI/LoadAlgorithmFactory.h" // For the DECLARE_LOADALGORITHM macro

#include <fstream>
#include <sstream>
#include <boost/algorithm/string/replace.hpp>
#include <Poco/File.h>
#include <Poco/Path.h>

using std::endl;
using std::map;
using std::string;
using std::vector;

using namespace ::NeXus;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

namespace Mantid
{
namespace NeXus
{

DECLARE_ALGORITHM(LoadEventNexus)
DECLARE_LOADALGORITHM(LoadEventNexus)

using namespace Kernel;
using namespace API;
using Geometry::Instrument;

/// Empty default constructor
LoadEventNexus::LoadEventNexus() : IDataFileChecker()
{}

/**
 * Do a quick file type check by looking at the first 100 bytes of the file 
 *  @param filePath :: path of the file including name.
 *  @param nread :: no.of bytes read
 *  @param header :: The first 100 bytes of the file as a union
 *  @return true if the given file is of type which can be loaded by this algorithm
 */
bool LoadEventNexus::quickFileCheck(const std::string& filePath,size_t nread, const file_header& header)
{
  std::string ext = this->extension(filePath);
  // If the extension is nxs then give it a go
  if( ext.compare("nxs") == 0 ) return true;

  // If not then let's see if it is a HDF file by checking for the magic cookie
  if ( nread >= sizeof(int32_t) && (ntohl(header.four_bytes) == g_hdf_cookie) ) return true;
  return false;
}

/**
 * Checks the file by opening it and reading few lines 
 *  @param filePath :: name of the file inluding its path
 *  @return an integer value how much this algorithm can load the file 
 */
int LoadEventNexus::fileCheck(const std::string& filePath)
{
  int confidence(0);
  try
  {
	// FIXME: We need a better test
    ::NeXus::File file = ::NeXus::File(filePath);
    // Open the base group called 'entry'
    file.openGroup("entry", "NXentry");
    // If all this succeeded then we'll assume this is an SNS Event NeXus file
    confidence = 80;
  }
  catch(::NeXus::Exception&)
  {
  }
  return confidence;
}

/// Initialisation method.
void LoadEventNexus::init()
{
  this->setOptionalMessage(
      "Loads an EventNeXus format file.\n"
      "Optionally, you can filter out events falling outside a\n"
      "range of times-of-flight and/or a time interval.");


  this->declareProperty(new FileProperty("Filename", "", FileProperty::Load, ".nxs"),
      "The name (including its full or relative path) of the Nexus file to\n"
      "attempt to load. The file extension must either be .nxs or .NXS" );

  this->declareProperty(
    new WorkspaceProperty<IEventWorkspace>("OutputWorkspace", "", Direction::Output),
    "The name of the output EventWorkspace in which to load the EventNexus file." );

  declareProperty(
      new PropertyWithValue<double>("FilterByTof_Min", EMPTY_DBL(), Direction::Input),
    "Optional: To exclude events that do not fall within a range of times-of-flight.\n"\
    "This is the minimum accepted value in microseconds." );

  declareProperty(
      new PropertyWithValue<double>("FilterByTof_Max", EMPTY_DBL(), Direction::Input),
    "Optional: To exclude events that do not fall within a range of times-of-flight.\n"\
    "This is the maximum accepted value in microseconds." );

  declareProperty(
      new PropertyWithValue<double>("FilterByTime_Start", EMPTY_DBL(), Direction::Input),
    "Optional: To only include events after the provided start time, in seconds (relative to the start of the run).");

  declareProperty(
      new PropertyWithValue<double>("FilterByTime_Stop", EMPTY_DBL(), Direction::Input),
    "Optional: To only include events before the provided stop time, in seconds (relative to the start of the run).");

  declareProperty(
      new PropertyWithValue<string>("BankName", "", Direction::Input),
    "Optional: To only include events from one bank. Any bank whose name does not match the given string will have no events.");

  declareProperty(
      new PropertyWithValue<bool>("SingleBankPixelsOnly", true, Direction::Input),
    "Optional: Only applies if you specified a single bank to load with BankName.\n"
    "Only pixels in the specified bank will be created if true; all of the instrument's pixels will be created otherwise.");

  declareProperty(
      new PropertyWithValue<bool>("LoadMonitors", false, Direction::Input),
      "Load the monitors from the file (optional, default False).");
//
//  declareProperty(
//      new PropertyWithValue<bool>("LoadLogs", true, Direction::Input),
//      "Load the sample logs from the file (optional, default True).");

  declareProperty(
      new PropertyWithValue<bool>("Precount", false, Direction::Input),
      "Pre-count the number of events in each pixel before allocating memory (optional, default False). \n"
      "This can significantly reduce memory use and memory fragmentation; it may also speed up loading.");
}



//------------------------------------------------------------------------------------------------
/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 */
void LoadEventNexus::exec()
{
  // Retrieve the filename from the properties
  m_filename = getPropertyValue("Filename");

  precount = getProperty("Precount");

  //loadlogs = getProperty("LoadLogs");
  loadlogs = true;

  //Get the limits to the filter
  filter_tof_min = getProperty("FilterByTof_Min");
  filter_tof_max = getProperty("FilterByTof_Max");
  if ( (filter_tof_min == EMPTY_DBL()) ||  (filter_tof_max == EMPTY_DBL()))
  {
    //Nothing specified. Include everything
    filter_tof_min = -1e20;
    filter_tof_max = +1e20;
  }
  else if ( (filter_tof_min != EMPTY_DBL()) ||  (filter_tof_max != EMPTY_DBL()))
  {
    //Both specified. Keep these values
  }
  else
    throw std::invalid_argument("You must specify both the min and max of time of flight to filter, or neither!");

  // Check to see if the monitors need to be loaded later
  bool load_monitors = this->getProperty("LoadMonitors");

  // Create the output workspace
  WS = EventWorkspace_sptr(new EventWorkspace());

  //Make sure to initialize.
  //   We can use dummy numbers for arguments, for event workspace it doesn't matter
  WS->initialize(1,1,1);

  // Set the units
  WS->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  WS->setYUnit("Counts");

  //Initialize progress reporting.
  //3 calls for the first part, 4 if monitors are loaded
  int reports = 4;
  if (load_monitors)
  {
    reports++;
  }
  Progress prog(this,0.0,0.3,  reports);


  if (loadlogs)
  {
    // --------------------- Load DAS Logs -----------------
    prog.report("Loading DAS logs");
    //The pulse times will be empty if not specified in the DAS logs.
    pulseTimes.clear();
    IAlgorithm_sptr loadLogs = createSubAlgorithm("LoadLogsFromSNSNexus");

    // Now execute the sub-algorithm. Catch and log any error, but don't stop.
    try
    {
      g_log.information() << "Loading logs from NeXus file..." << endl;
      loadLogs->setPropertyValue("Filename", m_filename);
      loadLogs->setProperty<MatrixWorkspace_sptr> ("Workspace", WS);
      loadLogs->execute();

      //If successful, we can try to load the pulse times
      Kernel::TimeSeriesProperty<double> * log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( WS->mutableRun().getProperty("proton_charge") );
      std::vector<Kernel::DateAndTime> temp = log->timesAsVector();
      for (size_t i =0; i < temp.size(); i++)
        pulseTimes.push_back( temp[i] );

      // Use the first pulse as the run_start time.
      if (temp.size() > 0)
      {
        // add the start of the run as a ISO8601 date/time string. The start = the first pulse.
        // (this is used in LoadInstrumentHelper to find the right instrument file to use).
        WS->mutableRun().addProperty("run_start", temp[0].to_ISO8601_string(), true );
      }
    }
    catch (...)
    {
      g_log.error() << "Error while loading Logs from SNS Nexus. Some sample logs may be missing." << std::endl;
    }
  }
  else
  {
    g_log.information() << "Skipping the loading of sample logs!" << endl;
  }
  prog.report("Loading instrument");

  //Load the instrument
  runLoadInstrument(m_filename, WS);

  if (!this->instrument_loaded_correctly)
      throw std::runtime_error("Instrument was not initialized correctly! Loading cannot continue.");

  if (load_monitors)
  {
    prog.report("Loading monitors");
    this->runLoadMonitors();
  }

  // Delete the output workspace name if it existed
  std::string outName = getPropertyValue("OutputWorkspace");
  if (AnalysisDataService::Instance().doesExist(outName))
    AnalysisDataService::Instance().remove( outName );

  // top level file information
  ::NeXus::File file(m_filename);

  //Start with the base entry
  file.openGroup("entry", "NXentry");

  //Now we want to go through all the bankN_event entries
  map<string, string> entries = file.getEntries();
  std::vector<string> bankNames;

  map<string,string>::const_iterator it = entries.begin();
  for (; it != entries.end(); it++)
  {
    std::string entry_name(it->first);
    std::string entry_class(it->second);
    if ((entry_class == "NXevent_data"))
    {
      bankNames.push_back( entry_name );
    }
  }

  //Close up the file
  file.closeGroup();
  file.close();

  // --------- Loading only one bank ----------------------------------
  std::string onebank = getProperty("BankName");
  bool doOneBank = (onebank != "");
  bool SingleBankPixelsOnly = getProperty("SingleBankPixelsOnly");
  if (doOneBank)
  {
    bankNames.clear();
    bankNames.push_back( onebank + "_events" );
  }


  prog.report("Initializing all pixels");

  //----------------- Pad Empty Pixels -------------------------------
  if (!this->instrument_loaded_correctly)
  {
    g_log.warning() << "Warning! Cannot pad empty pixels, since the instrument geometry did not load correctly or was not specified. Sorry!\n";
  }
  else
  {
    Timer tim1;
    //Pad pixels; parallel flag is off because it is actually slower :(
    if (doOneBank && SingleBankPixelsOnly)
    {
      // ---- Pad a pixel for each detector inside the bank -------
      int wi = 0;
      IInstrument_sptr inst = WS->getInstrument();
      boost::shared_ptr<IComponent> comp = inst->getComponentByName(onebank);
      boost::shared_ptr<ICompAssembly> bank = boost::dynamic_pointer_cast<ICompAssembly>(comp);
      if (bank)
      {
        // Get a vector of children (recursively)
        std::vector<boost::shared_ptr<IComponent> > children;
        bank->getChildren(children, true);
        std::vector<boost::shared_ptr<IComponent> >::iterator it;
        for (it = children.begin(); it != children.end(); it++)
        {
          IDetector_sptr det = boost::dynamic_pointer_cast<IDetector>(*it);
          if (det)
          {
            WS->getOrAddEventList(wi).addDetectorID( det->getID() );
            wi++;
          }
        }
        WS->doneAddingEventLists();
      }
      else
        throw std::runtime_error("Could not find the bank named " + onebank + " as a component assembly in the instrument tree.");
    }
    else
    {
      WS->padPixels( false );
    }
    //std::cout << tim1.elapsed() << "seconds to pad pixels.\n";
  }


  // -- Time filtering --
  double filter_time_start_sec, filter_time_stop_sec;
  filter_time_start_sec = getProperty("FilterByTime_Start");
  filter_time_stop_sec = getProperty("FilterByTime_Stop");

  //Default to ALL pulse times
  bool is_time_filtered = false;
  filter_time_start = Kernel::DateAndTime::minimum();
  filter_time_stop = Kernel::DateAndTime::maximum();

  if (pulseTimes.size() > 0)
  {
    //If not specified, use the limits of doubles. Otherwise, convert from seconds to absolute PulseTime
    if (filter_time_start_sec != EMPTY_DBL())
    {
      filter_time_start = pulseTimes[0] + filter_time_start_sec;
      is_time_filtered = true;
    }

    if (filter_time_stop_sec != EMPTY_DBL())
    {
      filter_time_stop = pulseTimes[0] + filter_time_stop_sec;
      is_time_filtered = true;
    }

    //Silly values?
    if (filter_time_stop < filter_time_start)
      throw std::invalid_argument("Your filter for time's Stop value is smaller than the Start value.");
  }


  //Count the limits to time of flight
  shortest_tof = static_cast<double>(std::numeric_limits<uint32_t>::max()) * 0.1;
  longest_tof = 0.;

  Progress prog2(this,0.3,1.0, bankNames.size());

  //This map will be used to find the workspace index
  IndexToIndexMap * pixelID_to_wi_map = WS->getDetectorIDToWorkspaceIndexMap(false);

  // Now go through each bank.
  // This'll be parallelized - but you can't run it in parallel if you couldn't pad the pixels.
  PARALLEL_FOR_IF( (this->instrument_loaded_correctly) )
  for (int i=0; i < static_cast<int>(bankNames.size()); i++)
  {
    PARALLEL_START_INTERUPT_REGION
    prog2.report("Loading " + bankNames[i]);
    this->loadBankEventData(bankNames[i], pixelID_to_wi_map);
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  //Don't need the map anymore.
  delete pixelID_to_wi_map;

  if (is_time_filtered)
  {
    //Now filter out the run, using the DateAndTime type.
    WS->mutableRun().filterByTime(filter_time_start, filter_time_stop);
  }

  //Info reporting
  g_log.information() << "Read " << WS->getNumberEvents() << " events"
      << ". Shortest TOF: " << shortest_tof << " microsec; longest TOF: "
      << longest_tof << " microsec." << std::endl;


  //Now, create a default X-vector for histogramming, with just 2 bins.
  Kernel::cow_ptr<MantidVec> axis;
  MantidVec& xRef = axis.access();
  xRef.resize(2);
  xRef[0] = shortest_tof - 1; //Just to make sure the bins hold it all
  xRef[1] = longest_tof + 1;
  //Set the binning axis using this.
  WS->setAllX(axis);

  // set more properties on the workspace
  this->loadEntryMetadata("entry");

  //Save output
  this->setProperty<IEventWorkspace_sptr>("OutputWorkspace", WS);

  // Clear any large vectors to free up memory.
  this->pulseTimes.clear();

  return;
}



void LoadEventNexus::loadEntryMetadata(const std::string &entry_name) {
  // Open the file
  ::NeXus::File file(m_filename);
  file.openGroup(entry_name, "NXentry");

  // get the title
  file.openData("title");
  if (file.getInfo().type == ::NeXus::CHAR) {
    string title = file.getStrData();
    if (!title.empty())
      WS->setTitle(title);
  }
  file.closeData();

  // TODO get the run number
  file.openData("run_number");
  string run("");
  if (file.getInfo().type == ::NeXus::CHAR) {
    run = file.getStrData();
  }
  if (!run.empty()) {
    WS->mutableRun().addProperty("run_number", run);
  }
  file.closeData();

  // close the file
  file.close();
}


//------------------------------------------------------------------------------------------------
/**
 * Load one of the banks' event data from the nexus file
 * @param entry_name :: The pathname of the bank to load
 * @param pixelID_to_wi_map :: a map where key = pixelID and value = the workpsace index to use.
 */
void LoadEventNexus::loadBankEventData(const std::string entry_name, IndexToIndexMap * pixelID_to_wi_map)
{
  //Local tof limits
  double my_shortest_tof, my_longest_tof;
  my_shortest_tof = static_cast<double>(std::numeric_limits<uint32_t>::max()) * 0.1;
  my_longest_tof = 0.;


  //The vectors we will be filling
  std::vector<uint64_t> event_index;
  std::vector<uint32_t> event_id;
  std::vector<float> event_time_of_flight;

  bool loadError = false ;

  PARALLEL_CRITICAL(LoadEventNexus_loadBankEventData_nexus_file_access)
  {
    // Open the file
    ::NeXus::File file(m_filename);
    file.openGroup("entry", "NXentry");

    //Open the bankN_event group
    file.openGroup(entry_name, "NXevent_data");

    // Get the event_index (a list of size of # of pulses giving the index in the event list for that pulse)
    file.openData("event_index");
    //Must be uint64
    if (file.getInfo().type == ::NeXus::UINT64)
      file.getData(event_index);
    else
    {
     g_log.warning() << "Entry " << entry_name << "'s event_index field is not UINT64! It will be skipped.\n";
     loadError = true;
    }
    file.closeData();

    // Look for the sign that the bank is empty
    if (event_index.size()==1)
    {
      if (event_index[0] == 0)
      {
        //One entry, only zero. This means NO events in this bank.
        loadError = true;
        g_log.debug() << "Bank " << entry_name << " is empty.\n";
      }
    }

    if (!loadError)
    {
      bool old_nexus_file_names = false;

      // Get the list of pixel ID's
      try
      {
        file.openData("event_id");
      }
      catch (::NeXus::Exception& )
      {
        //Older files (before Nov 5, 2010) used this field.
        file.openData("event_pixel_id");
        old_nexus_file_names = true;
      }

      //Must be uint32
      if (file.getInfo().type == ::NeXus::UINT32)
        file.getData(event_id);
      else
      {
        g_log.warning() << "Entry " << entry_name << "'s event_id field is not UINT32! It will be skipped.\n";
        loadError = true;
      }
      file.closeData();

      if (!loadError)
      {
        // Get the list of event_time_of_flight's
        if (!old_nexus_file_names)
          file.openData("event_time_offset");
        else
          file.openData("event_time_of_flight");

        //Check that the type is what it is supposed to be
        if (file.getInfo().type == ::NeXus::FLOAT32)
          file.getData(event_time_of_flight);
        else
        {
          g_log.warning() << "Entry " << entry_name << "'s event_time_offset field is not FLOAT32! It will be skipped.\n";
          loadError = true;
        }

        if (!loadError)
        {
          std::string units;
          file.getAttr("units", units);
          if (units != "microsecond")
          {
            g_log.warning() << "Entry " << entry_name << "'s event_time_offset field's units are not microsecond. It will be skipped.\n";
            loadError = true;
          }
          file.closeData();
        } //no error

      } //no error

    } //no error

    //Close up the file
    file.closeGroup();
    file.close();
  } // END of critical block.

  //Abort if anything failed
  if (loadError)
    return;

  // Two arrays must be the same size
  if (event_id.size() != event_time_of_flight.size())
  {
    g_log.warning() << "Entry " << entry_name << "'s event_time_offset and event_id vectors are not the same size! It will be skipped.\n";
    return;
  }

  // ---- Pre-counting events per pixel ID ----
  if (precount)
  {
    std::map<uint32_t, size_t> counts; // key = pixel ID, value = count
    std::vector<uint32_t>::const_iterator it;
    for (it = event_id.begin(); it != event_id.end(); it++)
    {
      std::map<uint32_t, size_t>::iterator map_found = counts.find(*it);
      if (map_found != counts.end())
      {
        map_found->second++;
      }
      else
      {
        counts[*it] = 1; // First entry
      }
    }

    // Now we pre-allocate (reserve) the vectors of events in each pixel counted
    std::map<uint32_t, size_t>::iterator pixID;
    for (pixID = counts.begin(); pixID != counts.end(); pixID++)
    {
      //Find the the workspace index corresponding to that pixel ID
      int wi((*pixelID_to_wi_map)[ pixID->first ]);
      // Allocate it
      WS->getEventList(wi).reserve( pixID->second );
    }
  }

  //Default pulse time (if none are found)
  Mantid::Kernel::DateAndTime pulsetime;

  // Index into the pulse array
  int pulse_i = 0;

  // And there are this many pulses
  int numPulses = static_cast<int>(pulseTimes.size());
  if (numPulses > static_cast<int>(event_index.size()))
  {
    g_log.warning() << "Entry " << entry_name << "'s event_index vector is smaller than the proton_charge DAS log. This is inconsistent, so we cannot find pulse times for this entry.\n";
    //This'll make the code skip looking for any pulse times.
    pulse_i = numPulses + 1;
  }

  //Go through all events in the list
  std::size_t numEvents = event_id.size();
  for (std::size_t i = 0; i < numEvents; i++)
  {
    //Find the pulse time for this event index
    if (pulse_i < numPulses-1)
    {
      //Go through event_index until you find where the index increases to encompass the current index. Your pulse = the one before.
      while ( !((i >= event_index[pulse_i]) && (i < event_index[pulse_i+1])))
      {
        pulse_i++;
        if (pulse_i >= (numPulses-1))
          break;
      }
      //Save the pulse time at this index for creating those events
      pulsetime = pulseTimes[pulse_i];
    }

    //Does this event pass the time filter?
    if ((pulsetime < filter_time_stop) && (pulsetime >= filter_time_start))
    {
      //Create the tofevent
      double tof = static_cast<double>( event_time_of_flight[i] );
      if ((tof >= filter_tof_min) && (tof <= filter_tof_max))
      {
        //The event TOF passes the filter.
        TofEvent event(tof, pulsetime);

        //Find the the workspace index corresponding to that pixel ID
        int wi((*pixelID_to_wi_map)[event_id[i]]);
        // Add it to the list at that workspace index
        WS->getEventList(wi).addEventQuickly( event );

        //Local tof limits
        if (tof < my_shortest_tof) { my_shortest_tof = tof;}
        if (tof > my_longest_tof) { my_longest_tof = tof;}
      }
    }

  }

  //Join back up the tof limits to the global ones
  PARALLEL_CRITICAL(tof_limits)
  {
    //This is not thread safe, so only one thread at a time runs this.
    if (my_shortest_tof < shortest_tof) { shortest_tof = my_shortest_tof;}
    if (my_longest_tof > longest_tof ) { longest_tof  = my_longest_tof;}
  }


}



//-----------------------------------------------------------------------------
/** Load the instrument geometry File
 *  @param nexusfilename :: Used to pick the instrument.
 *  @param localWorkspace :: MatrixWorkspace in which to put the instrument geometry
 */
void LoadEventNexus::runLoadInstrument(const std::string &nexusfilename, MatrixWorkspace_sptr localWorkspace)
{
  string instrument;

  // Get the instrument name
  ::NeXus::File nxfile(nexusfilename);
  //Start with the base entry
  nxfile.openGroup("entry", "NXentry");
  // Open the instrument
  nxfile.openGroup("instrument", "NXinstrument");
  nxfile.openData("name");
  instrument = nxfile.getStrData();
  g_log.debug() << "Instrument name read from NeXus file is " << instrument << std::endl;
  if (instrument.compare("POWGEN3") == 0) // hack for powgen b/c of bad long name
	  instrument = "POWGEN";
  // Now let's close the file as we don't need it anymore to load the instrument.
  nxfile.close();


  // do the actual work
  IAlgorithm_sptr loadInst= createSubAlgorithm("LoadInstrument");

  // Now execute the sub-algorithm. Catch and log any error, but don't stop.
  bool executionSuccessful(true);
  try
  {
    loadInst->setPropertyValue("InstrumentName", instrument);
    loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", localWorkspace);
    loadInst->execute();

    // Populate the instrument parameters in this workspace - this works around a bug
    localWorkspace->populateInstrumentParameters();
  } catch (std::invalid_argument& e)
  {
    g_log.information() << "Invalid argument to LoadInstrument sub-algorithm : " << e.what() << std::endl;
    executionSuccessful = false;
  } catch (std::runtime_error& e)
  {
    g_log.information("Unable to successfully run LoadInstrument sub-algorithm");
    g_log.information(e.what());
    executionSuccessful = false;
  }

  // If loading instrument definition file fails
  if (!executionSuccessful)
  {
    g_log.error() << "Error loading Instrument definition file\n";
  }
  else
  {
    this->instrument_loaded_correctly = true;
  }
}

/**
 * Load the Monitors from the NeXus file into a workspace. The original
 * workspace name is used and appended with _monitors.
 */
void LoadEventNexus::runLoadMonitors()
{
  IAlgorithm_sptr loadMonitors = this->createSubAlgorithm("LoadNexusMonitors");
  std::string mon_wsname = this->getProperty("OutputWorkspace");
  mon_wsname.append("_monitors");

  try
  {
    this->g_log.information() << "Loading monitors from NeXus file..."
        << std::endl;
    loadMonitors->setPropertyValue("Filename", m_filename);
    this->g_log.information() << "New workspace name for monitors: "
        << mon_wsname << std::endl;
    loadMonitors->setPropertyValue("OutputWorkspace", mon_wsname);
    loadMonitors->execute();
    MatrixWorkspace_sptr mons = loadMonitors->getProperty("OutputWorkspace");
    this->declareProperty(new WorkspaceProperty<>("MonitorWorkspace",
        mon_wsname, Direction::Output), "Monitors from the Event NeXus file");
    this->setProperty("MonitorWorkspace", mons);
  }
  catch (...)
  {
    this->g_log.error() << "Error while loading the monitors from the file. "
        << "File may contain no monitors." << std::endl;
  }
}

} // namespace NeXus
} // namespace Mantid
