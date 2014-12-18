//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidSINQ/PoldiLoadLog.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FileProperty.h"
#include <boost/shared_ptr.hpp>

#include <nexus/NeXusFile.hpp>
#include <nexus/NeXusException.hpp>

#include <fstream>
#include <string>

using namespace std;

namespace Mantid {
namespace Poldi {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(PoldiLoadLog)

using namespace Kernel;
using namespace API;
using Geometry::Instrument;
using namespace ::NeXus;

/// Initialisation method.
void PoldiLoadLog::init() {

  std::vector<std::string> exts;
  exts.push_back(".hdf");
  exts.push_back(".h5");
  exts.push_back("");

  // Input workspace containing the data raw to treat.
  declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>(
                      "InputWorkspace", "", Direction::InOut),
                  "Input workspace of the raw data.");

  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
                  "The raw data NeXus file");

  std::vector<std::string> exts2;
  exts2.push_back(".txt");
  exts2.push_back(".dic");
  exts2.push_back("");

  declareProperty(new FileProperty("Dictionary", "", FileProperty::Load, exts2),
                  "A Dictionary for controlling NeXus loading");

  // Data
  declareProperty(
      new WorkspaceProperty<ITableWorkspace>("PoldiLog", "", Direction::Output),
      "The output Tableworkspace"
      "with columns containing key summary information about the Poldi "
      "spectra.");
}

/** ***************************************************************** */

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw Exception::FileError If the Nexus file cannot be found/opened
 *  @throw std::invalid_argument If the optional properties are set to invalid
 *values
 */
void PoldiLoadLog::exec() {

  ////////////////////////////////////////////////////////////////////////
  // About the workspace
  ////////////////////////////////////////////////////////////////////////

  DataObjects::Workspace2D_sptr localWorkspace =
      this->getProperty("InputWorkspace");

  std::string filename = getProperty("Filename");
  std::string dictname = getProperty("Dictionary");
  g_log.information() << "_Poldi -        log     Running PoldiLoadLog "
                      << filename << " with  " << dictname << std::endl;
  g_log.information() << "_Poldi -        log          with dictionary "
                      << dictname << std::endl;

  loadDictionary(getProperty("Dictionary"));
  g_log.information() << "_Poldi -        log           dictionary has "
                      << dictionary.size() << " entries  " << std::endl;
  //	setProperty("DictionaryNbOfEntries", dictionary.size());

  ////////////////////////////////////////////////////////////////////////
  // Load the data into the workspace
  ////////////////////////////////////////////////////////////////////////

  ITableWorkspace_sptr outputws = WorkspaceFactory::Instance().createTable();

  outputws->addColumn("str", "param");
  outputws->addColumn("str", "path");
  outputws->addColumn("str", "value");

  std::map<std::string, std::string>::const_iterator it;
  File fin(filename);

  for (it = dictionary.begin(); it != dictionary.end(); ++it) {

    //		g_log.error() << "_Poldi -   dico       " <<  it->first  << " "
    //<<
    // it->second << std::endl;

    TableRow t10 = outputws->appendRow();

    if (it->second.find("/") == 0) {
      (&fin)->openPath(it->second);

      Info inf = (&fin)->getInfo();
      if (inf.type == ::NeXus::CHAR) {
        std::string data = (&fin)->getStrData();
        t10 << it->first << it->second << data;
        g_log.debug() << "_Poldi -        log     " << it->first << " "
                      << it->second << " " << data << std::endl;
      } else if (inf.type == ::NeXus::FLOAT32 || inf.type == ::NeXus::FLOAT64) {
        std::vector<double> data;
        (&fin)->getDataCoerce(data);
        std::ostringstream s;
        s << data[0];
        t10 << it->first << it->second << s.str();
        g_log.debug() << "_Poldi -        log     " << it->first << " "
                      << it->second << " " << data[0] << std::endl;
      } else {
        std::vector<int> data;
        (&fin)->getDataCoerce(data);
        std::ostringstream s;
        s << data[0];
        t10 << it->first << it->second << s.str();
        g_log.debug() << "_Poldi -        log     " << it->first << " "
                      << it->second << " " << data[0] << std::endl;
      }
    } else {
      t10 << it->first << "" << it->second;
      g_log.information() << "_Poldi -        log     " << it->first << " = "
                          << it->second << std::endl;
    }
  }

  // Open the hdf file
  try {
    (&fin)->openPath(dictionary["StartTime"]);
    //		localWorkspace->mutableRun().addProperty("run_start",
    // root.getString(dictionary["StartTime"]), true );

    //		NeXus::NXRoot theroot(filename);
    //	    Kernel::DateAndTime
    // run_start(root.getString(dictionary["StartTime"]));
    Kernel::DateAndTime run_start((&fin)->getStrData());
    //		localWorkspace->mutableRun().addProperty("run_start",
    // run_start.toISO8601String(), true );
    localWorkspace->mutableRun().addProperty("run_start", (&fin)->getStrData(),
                                             true);
  } catch (::NeXus::Exception &) {
  }

  (&fin)->close();

  setProperty("PoldiLog", outputws);
}

void PoldiLoadLog::loadDictionary(std::string dictFile) {
  std::ifstream in(dictFile.c_str(), std::ifstream::in);
  std::string line, key, value;

  while (std::getline(in, line)) {
    // skip comments
    if (line[0] == '#') {
      continue;
    }
    // skip empty lines
    if (line.length() < 2) {
      continue;
    }
    std::istringstream l(line);
    std::getline(l, key, '=');
    std::getline(l, value, '\n');
    boost::algorithm::trim(key);
    boost::algorithm::trim(value);
    dictionary[key] = value;
  }
  in.close();
}

} // namespace DataHandling
} // namespace Mantid
