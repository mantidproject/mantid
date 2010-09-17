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

//TODO: Is that the right path for the include?
#include "../../../Third_Party/include/NeXusFile.hpp"

//We have to rename the namespace since there is a conflict with Mantid::NeXus
namespace NeXusAPI = NeXus;

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
  const Workspace_sptr localWorkspace = getProperty("Workspace");



  // top level file information
  NeXusAPI::File file(m_filename);
  g_log.information() << "NeXus file found: " << file.inquireFile() << endl;

//  vector<NeXusAPI::AttrInfo> attr_infos = file.getAttrInfos();
//  cout << "Number of global attributes: " << attr_infos.size() << endl;
//  for (vector<NeXusAPI::AttrInfo>::iterator it = attr_infos.begin();
//      it != attr_infos.end(); it++) {
//    if (it->name != "file_time" && it->name != "HDF_version" && it->name !=  "HDF5_Version" && it->name != "XML_version") {
//      cout << "   " << it->name << " = ";
//      if (it->type == NeXusAPI::CHAR) {
//        cout << file.getStrAttr(*it);
//      }
//      cout << endl;
//    }
//  }
//
//  // check group attributes
//  file.openGroup("entry", "NXentry");
//  attr_infos = file.getAttrInfos();
//  cout << "Number of group attributes: " << attr_infos.size() << endl;
//  for (vector<NeXusAPI::AttrInfo>::iterator it = attr_infos.begin();
//      it != attr_infos.end(); it++) {
//    cout << "   " << it->name << " = ";
//    if (it->type == NeXusAPI::CHAR) {
//      cout << file.getStrAttr(*it);
//    }
//    cout << endl;
//  }


  return;
}


} // namespace NeXus
} // namespace Mantid
