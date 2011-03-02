//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadLogsFromSNSNexus.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Timer.h"
#include "MantidAPI/FileProperty.h"

#include <fstream>
#include <sstream>
#include <boost/algorithm/string/replace.hpp>

using std::cout;
using std::endl;
using std::map;
using std::string;
using std::vector;

static bool VERBOSE = false;

namespace Mantid
{
namespace NeXus
{

DECLARE_ALGORITHM(LoadLogsFromSNSNexus)

/// Sets documentation strings for this algorithm
void LoadLogsFromSNSNexus::initDocs()
{
  this->setWikiSummary("Loads sample logs (temperature, pulse charges, etc.) from a SNS NeXus file and adds it to the run information in a [[workspace]]. This is useful when using [[LoadEventPreNeXus]], to add sample logs after loading. ");
  this->setOptionalMessage("Loads sample logs (temperature, pulse charges, etc.) from a SNS NeXus file and adds it to the run information in a workspace. This is useful when using LoadEventPreNeXus, to add sample logs after loading.");
}


using namespace Kernel;
using namespace API;
using Geometry::Instrument;

/// Empty default constructor
LoadLogsFromSNSNexus::LoadLogsFromSNSNexus()
{}

/// Initialisation method.
void LoadLogsFromSNSNexus::init()
{
  // When used as a sub-algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut),
    "The name of the workspace in which to import the sample logs." );

  declareProperty(new FileProperty("Filename", "", FileProperty::Load, ".nxs"),
		  "The name (including its full or relative path) of the Nexus file to\n"
		  "attempt to load the instrument from. The file extension must either be\n"
		  ".nxs or .NXS" );
  declareProperty(new PropertyWithValue<bool>("OverwriteLogs", true, Direction::Input));
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 */
void LoadLogsFromSNSNexus::exec()
{
  // Retrieve the filename from the properties
  m_filename = getPropertyValue("Filename");

  // Get the input workspace
  WS = getProperty("Workspace");


  // top level file information
  ::NeXus::File file(m_filename);
  g_log.information() << "NeXus file found: " << file.inquireFile() << endl;

  //Start with the base entry
  file.openGroup("entry", "NXentry");

  //Now go to the DAS logs
  file.openGroup("DASlogs", "NXgroup");

  // print out the entry level fields
  map<string, string> entries = file.getEntries();

  ::NeXus::Info info;
  map<string,string>::const_iterator it = entries.begin();
  for (; it != entries.end(); it++)
  {
    std::string entry_name(it->first);
    std::string entry_class(it->second);
    if ((entry_class == "NXlog") ||(entry_class == "NXpositioner"))
    {
      loadSampleLog(file, entry_name, entry_class);
    }
  }

  file.closeGroup();

  try
  {
    //Use the DAS logs to integrate the proton charge (if any).
    WS->mutableRun().integrateProtonCharge();
  }
  catch (Exception::NotFoundError &)
  {
    //Ignore not found property error.
  }

  return;
}


/** Loads an entry from a previously-open NXS file as a log entry
 * in the workspace's run.
 *
 * @param file: NXS file handle. MUST BE PASSED BY REFERENCE otherwise there
 *    occurs a segfault.
 * @param entry_name, entry_class: name and class of NXlog to open.
 */
void LoadLogsFromSNSNexus::loadSampleLog(::NeXus::File& file, std::string entry_name, std::string entry_class)
{
  // whether or not to overwrite logs on workspace
  bool overwritelogs = this->getProperty("OverwriteLogs");

  file.openGroup(entry_name, entry_class);

  // Validate the NX log class.
  map<string, string> entries = file.getEntries();
  if ((entries.find("value") == entries.end()) ||
      (entries.find("time") == entries.end()) )
  {
    g_log.warning() << "Invalid NXlog entry " << entry_name << " found. Did not contain 'value' and 'time'.\n";
    file.closeGroup();
    return;
  }


  ::NeXus::Info info;
  //Two possible types of properties:
  vector<double> values;
  vector<int> values_int;

  bool isTimeSeries = false;
  bool isInt = false;

  file.openData("value");

  //Get the units of the property
  std::string units("");
  try
  {
    file.getAttr("units", units);
  }
  catch (::NeXus::Exception &)
  {
    //Ignore missing units field.
    units = "";
  }

  //If there is more than one entry, it is a timeseries
  info = file.getInfo();
  //isTimeSeries = (info.dims[0] > 1);
  isTimeSeries = true;

  Timer timer1;
  try
  {
    //Get the data (convert types if necessary)
    if (file.isDataInt())
    {
      isInt = true;
      file.getDataCoerce(values_int);
//      if (values_int.size() == 1)
//      {
//        WS->mutableRun().addProperty(entry_name, values_int[0], units);
//      }

    }
    else
    {
      //Try to get as doubles.
      file.getDataCoerce(values);
//      if (values.size() == 1)
//      {
//        WS->mutableRun().addProperty(entry_name, values[0], units);
//      }
    }
  }
  catch (::NeXus::Exception &e)
  {
    g_log.warning() << "NXlog entry " << entry_name << " gave an error when loading 'value' data:'" << e.what() << "'.\n";
    file.closeData();
    file.closeGroup();
    return;
  }
  if (VERBOSE) std::cout << "getDataCoerce took " << timer1.elapsed() << " sec.\n";


  file.closeData();

  if (isTimeSeries)
  {
    // --- Time series property ---

    //Get the times
    vector<double> time_double;
    vector<DateAndTime> times;

    try {
      file.openData("time");
    }
    catch (::NeXus::Exception &e)
    {
      g_log.warning() << "NXlog entry " << entry_name << " gave an error when opening the time field '" << e.what() << "'.\n";
      file.closeGroup();
      return;
    }

    //----- Start time is an ISO8601 string date and time. ------
    std::string start;
    try {
      file.getAttr("start", start);
    }
    catch (::NeXus::Exception &)
    {
      //Some logs have "offset" instead of start
      try {
        file.getAttr("offset", start);
      }
      catch (::NeXus::Exception &)
      {
        g_log.warning() << "NXlog entry " << entry_name << " has no start time indicated.\n";
        file.closeData();
        file.closeGroup();
        return;
      }
    }

    //Convert to date and time
    Kernel::DateAndTime start_time = Kernel::DateAndTime(start);

    std::string time_units;
    file.getAttr("units", time_units);
    if (time_units != "second")
    {
      g_log.warning() << "NXlog entry " << entry_name << " has time units of '" << time_units << "', which are unsupported. 'second' is the only supported time unit.\n";
      file.closeData();
      file.closeGroup();
      return;
    }

    Timer timer2;
    //--- Load the seconds into a double array ---
    try {
      file.getDataCoerce(time_double);
    }
    catch (::NeXus::Exception &e)
    {
      g_log.warning() << "NXlog entry " << entry_name << "'s time field could not be loaded: '" << e.what() << "'.\n";
      file.closeData();
      file.closeGroup();
      return;
    }
    file.closeData();

    if (VERBOSE) std::cout << "getDataCoerce for the seconds field took " << timer2.elapsed() << " sec.\n";

    if (isInt)
    {
      //Make an int TSP
      TimeSeriesProperty<int> * tsp = new TimeSeriesProperty<int>(entry_name);
      tsp->create(start_time, time_double, values_int);
      tsp->setUnits( units );
      WS->mutableRun().addProperty( tsp, overwritelogs );
    }
    else
    {
      //Make a double TSP
      TimeSeriesProperty<double> * tsp = new TimeSeriesProperty<double>(entry_name);

      Timer timer3;
      tsp->create(start_time, time_double, values);
      if (VERBOSE) std::cout << "creating a TSP took  " << timer3.elapsed() << " sec.\n";

      tsp->setUnits( units );
      WS->mutableRun().addProperty( tsp, overwritelogs );
      // Trick to free memory?
      std::vector<double>().swap(time_double);
      std::vector<double>().swap(values);

    }

  }

  file.closeGroup();
}


} // namespace NeXus
} // namespace Mantid
