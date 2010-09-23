//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadLogsFromSNSNexus.h"
#include "MantidAPI/Instrument.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidAPI/FileProperty.h"

#include <fstream>
#include <sstream>
#include <boost/algorithm/string/replace.hpp>

using std::cout;
using std::endl;
using std::map;
using std::string;
using std::vector;


namespace Mantid
{
namespace NeXus
{

DECLARE_ALGORITHM(LoadLogsFromSNSNexus)

using namespace Kernel;
using namespace API;

/// Empty default constructor
LoadLogsFromSNSNexus::LoadLogsFromSNSNexus()
{}

/// Initialisation method.
void LoadLogsFromSNSNexus::init()
{
  // When used as a sub-algorithm the workspace name is not used - hence the "Anonymous" to satisfy the validator
  declareProperty(
    new WorkspaceProperty<MatrixWorkspace>("Workspace","Anonymous",Direction::InOut),
    "The name of the workspace in which to inport the sample logs." );

  declareProperty(new FileProperty("Filename", "", FileProperty::Load, ".nxs"),
		  "The name (including its full or relative path) of the Nexus file to\n"
		  "attempt to load the instrument from. The file extension must either be\n"
		  ".nxs or .NXS" );
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
  NeXusAPI::File file(m_filename);
  g_log.information() << "NeXus file found: " << file.inquireFile() << endl;

  //Start with the base entry
  file.openGroup("entry", "NXentry");

  //TODO: Load run title and other info.

  //Now go to the DAS logs
  file.openGroup("DASlogs", "NXgroup");


  // print out the entry level fields
  map<string, string> entries = file.getEntries();

  NeXusAPI::Info info;
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


  return;
}


/** Loads an entry from a previously-open NXS file as a log entry
 * in the workspace's run.
 *
 * @param file: NXS file handle. MUST BE PASSED BY REFERENCE otherwise there
 *    occurs a segfault.
 * @param entry_name, entry_class: name and class of NXlog to open.
 */
void LoadLogsFromSNSNexus::loadSampleLog(NeXusAPI::File& file, std::string entry_name, std::string entry_class)
{
  file.openGroup(entry_name, entry_class);

  // Validate the NX log class.
  map<string, string> entries = file.getEntries();
  if ((entries.find("value") == entries.end()) ||
      (entries.find("time") == entries.end()) )
  {
    g_log.warning() << "Invalid NXlog entry " << entry_name << " found.\n";
    return;
  }


  NeXusAPI::Info info;
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
    //std::cout << entry_name << " has units " << units << "." << std::endl;
  }
  catch (NeXusAPI::Exception ex)
  {
    //Ignore missing units field.
    //std::cout << entry_name << " has no units " << std::endl;
    units = "";
  }

  //If there is more than one entry, it is a timeseries
  info = file.getInfo();
  isTimeSeries = (info.dims[0] > 1);

  //Get the data (convert types if necessary)
  if (file.isDataInt())
  {
    isInt = true;
    file.getDataCoerce(values_int);
    if (values_int.size() == 1)
    {
      WS->mutableRun().addProperty(entry_name, values_int[0], units);
    }
  }
  else
  {
    try
    {
      //Try to get as doubles.
      file.getDataCoerce(values);
      if (values.size() == 1)
      {
        WS->mutableRun().addProperty(entry_name, values[0], units);
      }
    }
    catch (NeXusAPI::Exception)
    {
      g_log.warning() << "NXlog entry " << entry_name << " has an unsupported 'value' data type.\n";
      file.closeData();
      file.closeGroup();
      return;

    }
  }

  file.closeData();

  if (isTimeSeries)
  {
    // --- Time series property ---

    //Get the times
    vector<double> time_double;
    vector<dateAndTime> times;

    file.openData("time");

    //Start time is an ISO8601 string date and time.
    std::string start;
    file.getAttr("start", start);

    //Convert to date and time
    Kernel::dateAndTime start_time = Kernel::DateAndTime::create_DateAndTime_FromISO8601_String(start);

    std::string time_units;
    file.getAttr("units", time_units);
    if (time_units != "second")
    {
      g_log.warning() << "NXlog entry " << entry_name << " has time units of '" << time_units << "', which are unsupported. 'second' is the only supported time unit.\n";
      file.closeData();
      file.closeGroup();
      return;
    }

    //Load the seconds into a double array
    file.getDataCoerce(time_double);
    file.closeData();

    if (isInt)
    {
      //Make an int TSP
      TimeSeriesProperty<int> * tsp = new TimeSeriesProperty<int>(entry_name);
      tsp->create(start_time, time_double, values_int);
      tsp->setUnits( units );
      WS->mutableRun().addLogData( tsp );
    }
    else
    {
      //Make a double TSP
      TimeSeriesProperty<double> * tsp = new TimeSeriesProperty<double>(entry_name);
      tsp->create(start_time, time_double, values);
      tsp->setUnits( units );
      WS->mutableRun().addLogData( tsp );
    }

  }

  file.closeGroup();
}


} // namespace NeXus
} // namespace Mantid
