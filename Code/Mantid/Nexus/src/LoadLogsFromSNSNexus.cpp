//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidNexus/LoadLogsFromSNSNexus.h"
#include "MantidAPI/Instrument.h"

#include "MantidKernel/ConfigService.h"
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
    new WorkspaceProperty<Workspace>("Workspace","Anonymous",Direction::InOut),
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
  localWorkspace = getProperty("Workspace");


//  // top level file information
//  NeXusAPI::File file(m_filename);
//  g_log.information() << "NeXus file found: " << file.inquireFile() << endl;
//
//  //Start with the base entry
//  file.openGroup("entry", "NXentry");
//
//  //TODO: Load run title and other info.
//
//  //Now go to the DAS logs
//  file.openGroup("DASlogs", "NXgroup");
//
//
//  // print out the entry level fields
//  map<string, string> entries = file.getEntries();
//  cout << "DASlogs contains " << entries.size() << " items" << endl;
//
//  NeXusAPI::Info info;
//  for (map<string,string>::const_iterator it = entries.begin();
//       it != entries.end(); it++)
//  {
//    std::string entry_name = it->first;
//    std::string entry_class = it->second;
//    if (entry_class == "NXlog")
//    {
//      loadSampleLog(file, entry_name, entry_class);
//    }
//  }
//  file.closeGroup();


  return;
}


/** Loads an entry from a previously-open NXS file as a log entry
 * in the workspace's run.
 */
//void LoadLogsFromSNSNexus::loadSampleLog(NeXusAPI::File file, std::string entry_name, std::string entry_class)
//{
//  cout << entry_name << "\n";
//  file.openGroup(entry_name, entry_class);
//
//  NeXusAPI::Info info;
//  file.openData("value");
//  info = file.getInfo();
//  vector<float> * result = file.getData<float>();
//
//  if (result->size() > 0)
//    cout << entry_name << (*result)[0] << "\n";
//
//  delete result;
//  file.closeGroup();
//}


} // namespace NeXus
} // namespace Mantid
