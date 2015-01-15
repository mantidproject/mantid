#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataHandling/LoadTomoConfig.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/PropertyWithValue.h"

#include <nexus/NeXusException.hpp>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadTomoConfig);

using namespace Mantid::API;

LoadTomoConfig::LoadTomoConfig() {
}

LoadTomoConfig::~LoadTomoConfig() {
}

/** 
 * Standard Initialisation method. Declares properties.
 */
void LoadTomoConfig::init() {
  // Required input properties
  std::vector<std::string> exts;
  exts.push_back(".xml");
  exts.push_back(".nxs");
  exts.push_back(".nx5");

  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
                  "The name of the Nexus parameters file to read, as a full "
                  "or relative path.");

  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                                         Kernel::Direction::Output),
                  "The name of the workspace to be created as output of"
                  "the algorithm.  A workspace of this name will be created "
                  "and stored in the Analysis Data Service.");
}

/** 
 * Executes the algorithm: reads in the parameters file and creates
 * and fills the output workspace
 *
 * @throw runtime_error Thrown if execution fails
 */
void LoadTomoConfig::exec() {
  progress(0, "Opening file...");

  // Will throw an approriate exception if there is a problem with the
  // properties
  std::string fname = getPropertyValue("Filename");
  std::string wsName = getPropertyValue("OutputWorkspace");

  ITableWorkspace_sptr ws;
  try {
    // Do the real load. Throws exception if issues found
    ws =  loadFile(fname, wsName);
  } catch(std::exception& e) {
    g_log.error() << "Failed to load file: " << e.what();
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
bool LoadTomoConfig::checkOpenFile(std::string fname,
                                   boost::shared_ptr<::NeXus::File> &f) {
  try {
    f = boost::make_shared<::NeXus::File>(fname);
    if (f)
      f->getEntries();
  } catch (::NeXus::Exception &e) {
    g_log.error() << "Failed to open as a NeXus file: '" << fname
                  << "', error description: " << e.what() << std::endl;
    return false;
  }
  return true;
}

/**
 * Loads a tomography parameterization file into a newly created table
 * workspace.
 *
 * @param fname name of the parameterization file
 * @param wsName name of workspace where to load the file data
 *
 * @return table workspace with parameters (plugins) found in the
 * loaded file
 */
ITableWorkspace_sptr LoadTomoConfig::loadFile(std::string& fname,
                                              std::string& wsName) {
  // Throws an exception if there is a problem with file access
  //Mantid::NeXus::NXRoot root(fname);
  boost::shared_ptr<::NeXus::File> f = NULL;
  if (!checkOpenFile(fname, f)) {
    throw std::runtime_error(
        "Failed to recognize this file as a NeXus file, cannot continue.");
  }

  ITableWorkspace_sptr ws =
    API::WorkspaceFactory::Instance().createTable(wsName);
  if (!ws)
    throw std::runtime_error("Could not create TableWorkspace with name + '"
                             + wsName + "'");

  // init workspace
  ws->setTitle("Table with tomography parameters from file " +
               fname);
  ws->addColumn("str", "ID");
  ws->addColumn("str", "Params");
  ws->addColumn("str", "Name");
  ws->addColumn("str", "Cite");

  // a bit of file consistency check, check at least there's a
  // 'entry1'
  // it could be more strict and demand entries.size()==1
  std::map<std::string, std::string> entries = f->getEntries();
  auto it = entries.find("entry1");
  if (entries.end() == it) {
    throw std::runtime_error("Could not find the 'entry1' entry. "
      "Even though this file looks like a valid NeXus file, it is "
      "not in the correct format for tomography reconstruction "
      "parameterization files.");
  }

  // go through the input file plugin entries
  f->openGroup("entry1", "NXentry");
  f->openGroup("processing", "NXsubentry");
  size_t pluginsLen = f->getEntries().size();
  for (size_t j=0; j<pluginsLen; j++) {
    API::TableRow table = ws->appendRow();

    std::string entryIdx = boost::lexical_cast<std::string>(j);
    try {
      f->openGroup(entryIdx, "NXsubentry");
    } catch(::NeXus::Exception &e) {
      // detailed NeXus error message and throw...
      g_log.error() << "Failed to load plugin '" << j << "' from"
        "NeXus file. Error description: " << e.what();
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
      id = f->getStrData();
      params = f->getStrData();
      name = f->getStrData();
      cite = f->getStrData();
    } catch(::NeXus::Exception &e) {
      // permissive, just error message but carry on
      g_log.warning() << "Failed to read some fields in tomographic "
        "reconstruction plugin line. The file seems to be wrong. Error "
        "description: " << e.what();
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
