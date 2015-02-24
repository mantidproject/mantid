#include "MantidAPI/FileProperty.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadSavuTomoConfig.h"
#include "MantidKernel/PropertyWithValue.h"

#include <nexus/NeXusException.hpp>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadSavuTomoConfig);

using namespace Mantid::API;

LoadSavuTomoConfig::LoadSavuTomoConfig() {
}

LoadSavuTomoConfig::~LoadSavuTomoConfig() {
}

/** 
 * Standard Initialisation method. Declares properties.
 */
void LoadSavuTomoConfig::init() {
  // Required input properties
  std::vector<std::string> exts;
  exts.push_back(".nxs");
  exts.push_back(".nx5");
  exts.push_back(".xml");

  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
                  "The name of the Nexus parameterization file to read, as a full"
                  "or relative path.");

  declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace",
                                                         "savuTomoConfig",
                                                         Kernel::Direction::Output,
                                                         PropertyMode::Mandatory),
                  "The name of the workspace to be created as output of"
                  "the algorithm, a workspace with this name will be created "
                  "and stored in the Analysis Data Service.");
}

/** 
 * Executes the algorithm: reads in the parameters file and creates
 * and fills the output workspace
 *
 * @throw runtime_error Thrown if execution fails
 */
void LoadSavuTomoConfig::exec() {
  progress(0, "Opening file...");

  // Will throw an approriate exception if there is a problem with the
  // properties
  std::string fname = getPropertyValue("Filename");
  std::string wsName = getPropertyValue("OutputWorkspace");

  ITableWorkspace_sptr ws;
  try {
    // Do the real load. Throws exception if issues found
    ws =  loadFile(fname, wsName);
    if (ws) {
      setProperty("OutputWorkspace", ws);
    }
  } catch(std::exception& e) {
    g_log.error() << "Failed to load savu tomography reconstruction "
      "parameterization file: " << e.what() << std::endl;
    return;
  }

  progress(1.0, "Loading finished.");
}

/**
 * Can it be openend in the expected format, and does it seem to have
 * sensible contents?
 *
 * @param fname name of file
 * @param f nexus file object, created if file is fine
 *
 * @return true if everything seems fine, false otherwise
 */
bool LoadSavuTomoConfig::checkOpenFile(std::string fname,
                                   boost::shared_ptr<NeXus::File> &f) {
  try {
    f = boost::make_shared<NeXus::File>(fname);
    if (f)
      f->getEntries();
  } catch (NeXus::Exception &e) {
    g_log.error() << "Failed to open as a NeXus file: '" << fname
                  << "', error description: " << e.what() << std::endl;
    return false;
  }
  return true;
}

/**
 * Loads a tomography parameterization file into a newly created table
 * workspace. The file must have the following syntax:
 *
 * <NXentry name="entry1">
 *   <NXprocess name="processing">
 *     <NXnote name="id">
 *       <values id="ID VALUE" params="..." name="..." cite="...">
 *       </values>
 *     </NXnote>
 *   </NXprocess>
 * </NXentry>
 *
 * @param fname name of the parameterization file
 * @param wsName name of workspace where to load the file data
 *
 * @return table workspace with parameters (plugins) found in the
 * loaded file
 */
ITableWorkspace_sptr LoadSavuTomoConfig::loadFile(std::string& fname,
                                              std::string& wsName) {
  // Throws an exception if there is a problem with file access
  //Mantid::NeXus::NXRoot root(fname);
  boost::shared_ptr<NeXus::File> f;
  if (!checkOpenFile(fname, f)) {
    throw std::runtime_error(
        "Failed to recognize this file as a NeXus file, cannot continue.");
  }

  ITableWorkspace_sptr ws =
    API::WorkspaceFactory::Instance().createTable();
  if (!ws)
    throw std::runtime_error("Could not create TableWorkspace for "
                             "workspace with name '" + wsName + "'");

  // init workspace
  ws->setTitle("Table with tomography parameters from file " +
               fname);
  ws->addColumn("str", "ID");
  ws->addColumn("str", "Parameters");
  ws->addColumn("str", "Name");
  ws->addColumn("str", "Cite");

  // a bit of file consistency check, check at least there's a
  // 'entry1'
  // it could be more strict and demand entries.size()==1
  std::map<std::string, std::string> entries = f->getEntries();
  std::string mainEntryName = "entry";
  auto it = entries.find(mainEntryName);
  if (entries.end() == it) {
    throw std::runtime_error("Could not find the '" + mainEntryName + "' "
      "entry. Even though this file looks like a valid NeXus file, it is "
      "not in the correct format for tomography reconstruction "
      "parameterization files.");
  }

  // go through the input file plugin entries
  f->openGroup(mainEntryName, "NXentry");
  f->openGroup("process", "NXprocess");
  size_t pluginsLen = f->getEntries().size();
  for (size_t j=0; j<pluginsLen; j++) {
    API::TableRow table = ws->appendRow();

    std::string entryIdx = boost::lexical_cast<std::string>(j);
    try {
      f->openGroup(entryIdx, "NXnote");
    } catch(NeXus::Exception &e) {
      // detailed NeXus error message and throw...
      g_log.error() << "Failed to load plugin '" << j << "' from"
        "NeXus file. Error description: " << e.what() << std::endl;
      throw std::runtime_error("Could not load one or more plugin "
        "entries from the tomographic reconstruction parameterization "
        "file. Please check that the file is correct.");
    }

    // TODO: check final 'schema', get these 4 fields from the file
    std::string id = "";
    std::string params = "";
    std::string name = "";
    std::string cite = "";
    try {
      f->readData("data", params);
      f->readData("id", id);
      f->readData("name", name);
      // cite not available for now
      // f->readData("cite", cite);
      // This might be extended to an NXcite group that would be included
      // not here but inside an "intermediate" NXcollection group. That
      // NXcite would have 4 arrays: description, doi, endnote, bibtex.
      // But this is what we have so far.
      cite = "Not available";
    } catch(NeXus::Exception &e) {
      // permissive, just error message but carry on
      g_log.warning() << "Failed to read some fields in tomographic "
        "reconstruction plugin line. The file seems to be wrong. Error "
        "description: " << e.what() << std::endl;
    }

    table << id << params << name << cite;
    f->closeGroup();
    progress(static_cast<double>(j)/static_cast<double>(pluginsLen));
  }
  f->close();

  return ws;
}

} // namespace DataHandling
} // namespace Mantid
