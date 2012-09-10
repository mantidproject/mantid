/*WIKI* 


This algorithm loads all monitors found in a NeXus file into a single [[Workspace2D]]. The algorithm assumes that all of the monitors are histograms and have the same bin boundaries. '''NOTE:''' The entry is assumed to be in SNS format, so the loader is currently not generically applicable. It is also written for single entry files and will need tweaking to handle period data where the monitors are different.


*WIKI*/
#include "MantidDataHandling/LoadNexusMonitors.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/UnitFactory.h"

#include <Poco/File.h>
#include <Poco/Path.h>
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

namespace Mantid
{
namespace DataHandling
{

DECLARE_ALGORITHM(LoadNexusMonitors)

/// Sets documentation strings for this algorithm
void LoadNexusMonitors::initDocs()
{
  this->setWikiSummary(" Load all monitors from a NeXus file into a workspace. ");
  this->setOptionalMessage("Load all monitors from a NeXus file into a workspace.");
}


LoadNexusMonitors::LoadNexusMonitors() : Algorithm(),
nMonitors(0)
{
}

LoadNexusMonitors::~LoadNexusMonitors()
{
}

/// Initialisation method.
void LoadNexusMonitors::init()
{
  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load,
      ".nxs"),
      "The name (including its full or relative path) of the NeXus file to\n"
      "attempt to load. The file extension must either be .nxs or .NXS" );

  declareProperty(
    new API::WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace", "",
        Kernel::Direction::Output),
    "The name of the output workspace in which to load the NeXus monitors." );
}

/**
 * Executes the algorithm. Reading in the file and creating and populating
 * the output workspace
 */
void LoadNexusMonitors::exec()
{
  // Retrieve the filename from the properties
  this->filename = this->getPropertyValue("Filename");

  API::Progress prog1(this, 0.0, 0.2, 2);

  // top level file information
  ::NeXus::File file(this->filename);

  //Start with the base entry
  typedef std::map<std::string,std::string> string_map_t; 
  string_map_t::const_iterator it;
  string_map_t entries = file.getEntries();
  for (it = entries.begin(); it != entries.end(); ++it)
  {
	if ( ((it->first == "entry") || (it->first == "raw_data_1")) && (it->second == "NXentry") )
	{
		file.openGroup(it->first, it->second);
        m_top_entry_name = it->first;
		break;
	}
  }
  prog1.report();

  //Now we want to go through and find the monitors
  entries = file.getEntries();
  std::vector<std::string> monitorNames;
  prog1.report();

  API::Progress prog2(this, 0.2, 0.6, entries.size());

  it = entries.begin();
  for (; it != entries.end(); ++it)
  {
    std::string entry_name(it->first);
    std::string entry_class(it->second);
    if ((entry_class == "NXmonitor"))
    {
      monitorNames.push_back( entry_name );
    }
    prog2.report();
  }
  this->nMonitors = monitorNames.size();

  // Create the output workspace
  this->WS = API::WorkspaceFactory::Instance().create("Workspace2D",
      this->nMonitors, 1, 1);

  // a temporary place to put the spectra/detector numbers
  boost::scoped_array<specid_t> spectra_numbers(new specid_t[this->nMonitors]);
  boost::scoped_array<detid_t> detector_numbers(new detid_t[this->nMonitors]);

  API::Progress prog3(this, 0.6, 1.0, this->nMonitors);

  for (std::size_t i = 0; i < this->nMonitors; ++i)
  {
    g_log.information() << "Loading " << monitorNames[i] << std::endl;
    // Do not rely on the order in path list
    Poco::Path monPath(monitorNames[i]);
    std::string monitorName = monPath.getBaseName();

    // check for monitor name - in our case will be of the form either monitor1 or monitor_1
    std::string::size_type loc = monitorName.rfind('_');
    if (loc == std::string::npos)
    {
      loc = monitorName.rfind('r');
    }

    detid_t monIndex = -1 * boost::lexical_cast<int>(monitorName.substr(loc+1)); // SNS default
    file.openGroup(monitorNames[i], "NXmonitor");

    // Check if the spectra index is there
    specid_t spectrumNo(static_cast<specid_t>(i+1));
    try
    {
      file.openData("spectrum_index");
      file.getData(&spectrumNo);
      file.closeData();
    }
    catch(::NeXus::Exception &)
    {
      // Use the default as matching the workspace index
    }

    g_log.debug() << "monIndex = " << monIndex << std::endl;
    g_log.debug() << "spectrumNo = " << spectrumNo << std::endl;

    spectra_numbers[i] = spectrumNo;
    detector_numbers[i] = monIndex;
    // Default values, might change later.
    this->WS->getSpectrum(i)->setSpectrumNo(spectrumNo);
    this->WS->getSpectrum(i)->setDetectorID(monIndex);

    // Now, actually retrieve the necessary data
    file.openData("data");
    MantidVec data;
    MantidVec error;
    file.getDataCoerce(data);
    file.getDataCoerce(error);
    file.closeData();

    // Transform errors via square root
    std::transform(error.begin(), error.end(), error.begin(),
        (double(*)(double)) sqrt);

    // Get the TOF axis
    file.openData("time_of_flight");
    MantidVec tof;
    file.getDataCoerce(tof);
    file.closeData();
    file.closeGroup();

    this->WS->dataX(i) = tof;
    this->WS->dataY(i) = data;
    this->WS->dataE(i) = error;
    prog3.report();
  }

  // Fix the detector numbers if the defaults above are not correct
  fixUDets(detector_numbers, file, spectra_numbers, nMonitors);

  // Check for and ISIS compat block to get the detector IDs for the loaded spectrum numbers  
  // @todo: Find out if there is a better (i.e. more generic) way to do this
  try
  {
    file.openGroup("isis_vms_compat", "IXvms");
    
    file.closeGroup();
  }
  catch(::NeXus::Exception&)
  {
  }

  // Need to get the instrument name from the file
  std::string instrumentName;
  file.openGroup("instrument", "NXinstrument");
  file.openData("name");
  instrumentName = file.getStrData();
  g_log.debug() << "Instrument name read from NeXus file is " << instrumentName << std::endl;
  if (instrumentName.compare("POWGEN3") == 0) // hack for powgen b/c of bad long name
          instrumentName = "POWGEN";
  // Now let's close the file as we don't need it anymore to load the instrument.
  file.closeData();
  file.closeGroup(); // Close the NXentry
  file.close();

  this->WS->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
  this->WS->setYUnit("Counts");

  // Load the logs
  this->runLoadLogs(this->filename, this->WS);

  // The run_start will be loaded from the pulse times.
  Kernel::DateAndTime run_start(0,0);
  run_start = this->WS->getFirstPulseTime();
  this->WS->mutableRun().addProperty("run_start", run_start.toISO8601String(), true );

  // Load the instrument
  this->runLoadInstrument(instrumentName, this->WS);

  // Load the meta data, but don't stop on errors
  g_log.debug() << "Loading metadata" << std::endl;
  try
  {
      LoadEventNexus::loadEntryMetadata(this->filename, WS, m_top_entry_name);
  }
  catch (std::exception & e)
  {
      g_log.warning() << "Error while loading meta data: " << e.what() << std::endl;
  }

  // Fix the detector IDs/spectrum numbers
  for (size_t i=0; i < WS->getNumberHistograms(); i++)
  {
    WS->getSpectrum(i)->setSpectrumNo(spectra_numbers[i]);
    WS->getSpectrum(i)->setDetectorID(detector_numbers[i]);
  }
  WS->generateSpectraMap();


  this->setProperty("OutputWorkspace", this->WS);
}

/**
 * Fix the detector numbers if the defaults are not correct. Currently checks the isis_vms_compat
 * block and reads them from there if possible
 * @param det_ids :: An array of prefilled detector IDs
 * @param file :: A reference to the NeXus file opened at the root entry
 * @param spec_ids :: An array of spectrum numbers that the monitors have
 * @param nmonitors :: The size of the det_ids and spec_ids arrays
 */
void LoadNexusMonitors::fixUDets(boost::scoped_array<detid_t> &det_ids, ::NeXus::File & file, 
                                 const boost::scoped_array<specid_t> &spec_ids, 
                                 const size_t nmonitors) const
{
  try
  {
    file.openGroup("isis_vms_compat", "IXvms");
  }
  catch(::NeXus::Exception&)
  {
    return;
  }
  // UDET
  file.openData("UDET");
  std::vector<int32_t> udet;
  file.getData(udet);
  file.closeData();
  //SPEC
  file.openData("SPEC");
  std::vector<int32_t> spec;
  file.getData(spec);
  file.closeData();

  // This is a little complicated: Each value in the spec_id array is a value found in the
  // SPEC block of isis_vms_compat. The index that this value is found at then corresponds
  // to the index within the UDET block that holds the detector ID
  std::vector<int32_t>::const_iterator beg = spec.begin();
  for( size_t mon_index = 0; mon_index < nmonitors; ++mon_index )
  {
    std::vector<int32_t>::const_iterator itr = std::find(spec.begin(), spec.end(), spec_ids[mon_index]);
    if( itr == spec.end() ) 
    {
      det_ids[mon_index] = -1;
      continue;
    }
    std::vector<int32_t>::difference_type udet_index = std::distance(beg, itr);
    det_ids[mon_index] = udet[udet_index];
  }
  file.closeGroup();
}

void LoadNexusMonitors::runLoadLogs(const std::string filename, API::MatrixWorkspace_sptr localWorkspace)
{
    // do the actual work
    API::IAlgorithm_sptr loadLogs = createSubAlgorithm("LoadNexusLogs");
    // Now execute the sub-algorithm. Catch and log any error, but don't stop.
    try
    {
      g_log.information() << "Loading logs from NeXus file..." << std::endl;
      loadLogs->setPropertyValue("Filename", filename);
      loadLogs->setProperty<API::MatrixWorkspace_sptr> ("Workspace",localWorkspace);
      loadLogs->execute();
    }
    catch (...)
    {
      g_log.error() << "Error while loading Logs from Nexus. Some sample logs may be missing." << std::endl;
    }
}

/**
 * Load the instrument geometry File
 *  @param instrument :: instrument name.
 *  @param localWorkspace :: MatrixWorkspace in which to put the instrument geometry
 */
void LoadNexusMonitors::runLoadInstrument(const std::string &instrument,
    API::MatrixWorkspace_sptr localWorkspace)
{

  // do the actual work
  API::IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");

  // Now execute the sub-algorithm. Catch and log any error, but don't stop.
  bool executionSuccessful(true);
  try
  {
    loadInst->setPropertyValue("InstrumentName", instrument);
    loadInst->setProperty<API::MatrixWorkspace_sptr> ("Workspace", localWorkspace);
    loadInst->setProperty("RewriteSpectraMap", false); // We have a custom mapping
    loadInst->execute();

    // Populate the instrument parameters in this workspace - this works around a bug
    localWorkspace->populateInstrumentParameters();
  } catch (std::invalid_argument& e)
  {
    g_log.information() << "Invalid argument to LoadInstrument sub-algorithm : " << e.what()
        << std::endl;
    executionSuccessful = false;
  } catch (std::runtime_error& e)
  {
    g_log.information() << "Unable to successfully run LoadInstrument sub-algorithm : " << e.what()
        << std::endl;
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

} // end DataHandling
} // end Mantid
