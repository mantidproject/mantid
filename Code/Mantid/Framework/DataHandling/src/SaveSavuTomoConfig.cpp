#include "MantidAPI/FileProperty.h"
#include "MantidDataHandling/SaveSavuTomoConfig.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/MandatoryValidator.h"

#include <nexus/NeXusFile.hpp>
#include <Poco/File.h>

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveSavuTomoConfig)

using namespace Kernel;
using namespace API;

SaveSavuTomoConfig::SaveSavuTomoConfig() : API::Algorithm(), m_pluginInfoCount(4)
{ }

/**
 * Initialise the algorithm
 */
void SaveSavuTomoConfig::init() {
  // Get a list of table workspaces which contain the plugin information
  declareProperty(
      new ArrayProperty<std::string>("InputWorkspaces",
      boost::make_shared<MandatoryValidator<std::vector<std::string>>>()),
      "The names of the table workspaces containing plugin information.");

  declareProperty(new API::FileProperty("Filename", "", FileProperty::Save,
                                        std::vector<std::string>(1, ".nxs")),
                  "The name of the tomographic config file to write, as a full "
                  "or relative path. This will overwrite existing files.");
}

/**
 * Execute the algorithm
 */
void SaveSavuTomoConfig::exec() {
  // Prepare properties for writing to file
  std::string fileName = getPropertyValue("Filename");

  progress(0, "Checking workspace (tables)...");

  std::vector<ITableWorkspace_sptr> wss = checkTables(getProperty("InputWorkspaces"));

  try {
    saveFile(fileName, wss);
  } catch (std::exception &e) {
    g_log.error() << "Failed to save savu tomography reconstruction parameterization "
      "file, error description: " << e.what() << std::endl;
    return;
  }

  progress(0, "File saved.");
}

/**
 * Do a basic check that the table seems to be what we need to save a
 * savu configuration file.
 *
 * @param tws Table workspace that should contain plugin/processing
 * steps information (a savu configuration)
 *
 * @return True if the table passed seems to be a savu configuration table
 */
bool SaveSavuTomoConfig::tableLooksGenuine(const ITableWorkspace_sptr &tws) {
  // note that more columns might be added in the relatively near future
  if (!(tws->columnCount() >= m_pluginInfoCount))
    return false;

  std::vector<std::string> names = tws->getColumnNames();
  return ( 4 <= names.size() &&
           "ID" == names[0] &&
           "Parameters" == names[1] &&
           "Name" ==  names[2] &&
           "Cite" == names[3]);
}

/**
 * Check that the list of workspace names identifies table workspaces
 * with (approximately) the expected information (4 columns), and
 * return the corresponding (table) workspace objects.
 *
 * @param workspaces Table workspace names that should contain plugin/processing
 * steps information
 *
 * @return Table workspaces retrieved from the ADS, corresponding to
 * the names passed
 */
std::vector<ITableWorkspace_sptr> SaveSavuTomoConfig::checkTables(
    const std::vector<std::string> &workspaces) {
  std::vector<ITableWorkspace_sptr> wss;
  for (auto it = workspaces.begin(); it != workspaces.end(); ++it) {
    if (AnalysisDataService::Instance().doesExist(*it)) {
      ITableWorkspace_sptr table =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(*it);
      // Check it's valid. Very permissive check for the time being
      if (table && tableLooksGenuine(table)) {
        wss.push_back(table);
      } else {
        throw std::runtime_error("Invalid workspace provided: " +
                                 *it + ". This algorithm requires a table "
                                 "workspace with correct savu plugin/ "
                                 "pipeline process information.");
      }
    } else {
      throw std::runtime_error(
          "One or more specified table workspaces don't exist. I could not "
          "find this one: " + *it);
    }
  }
  return wss;
}

/**
 * Check that the list of workspace names identifies table workspaces
 * with (approximately) the expected information (4 columns), and
 * return the corresponding (table) workspace objects.
 *
 * @param fname Destination file name (can be ammended if needed and that
 * will be logged)
 * @param wss Table workspaces that apparently contain plugin/processing
 * steps information
 */
void SaveSavuTomoConfig::saveFile(const std::string fname,
              const std::vector<ITableWorkspace_sptr> &wss) {
  // Ensure it has a .nxs extension
  std::string fileName = fname;
  if (!boost::ends_with(fileName, ".nxs")) {
    const std::string ext = ".nxs";
    g_log.notice() << "Adding extension '" << ext << "' to the output "
      "file name given (it is a NeXus file). " << std::endl;
    fileName = fileName + ".nxs";
  }

  // If file exists, delete it.
  Poco::File f(fileName);
  if (f.exists()) {
    g_log.notice() << "Overwriting existing file: '" << fileName << "'" <<
      std::endl;
    f.remove();
  } else {
    g_log.notice() << "Creating file: '" << fileName << ";" << std::endl;
  }

  // Create the file handle
  NXhandle fileHandle;
  NXstatus status = NXopen(fileName.c_str(), NXACC_CREATE5, &fileHandle);

  if (status == NX_ERROR)
    throw std::runtime_error("Unable to create file.");

  NeXus::File nxFile(fileHandle);

  std::string topLevelEntry = "entry";
  nxFile.makeGroup(topLevelEntry, "NXentry", true);

  const std::string processingEntry = "processing";
  nxFile.makeGroup(processingEntry, "NXprocess", true);

  // Iterate through all plugin entries (number sub groups 0....n-1)
  size_t procCount = 0;
  for (size_t i = 0; i < wss.size(); ++i) {
    // Concatenate table contents, putting pipeline processing steps in the same sequence
    // as they come in the seq of table workspaces
    ITableWorkspace_sptr w = wss[i];
    for (size_t ti =0; ti < w->rowCount(); ++ti) {
      // Column info order is [ID / Params {as json string} / name {description} /
      // citation info]
      std::string id = w->cell<std::string>(ti, 0);
      std::string params = w->cell<std::string>(ti, 1);
      std::string name = w->cell<std::string>(ti, 2);
      // Unused for now, until file format is finalized/documented.
      // std::string cite = w->cell<std::string>(ti, 3);

      // but in the file it goes as: data (params), id, name
      nxFile.makeGroup(boost::lexical_cast<std::string>(procCount++), "NXnote", true);
      nxFile.writeData("data", params);
      nxFile.writeData("id", id);
      nxFile.writeData("name", name);
      // Ignore citation information for now. Format not fixed yet.
      // nxFile.writeData("cite", cite);
      nxFile.closeGroup(); // close NXnote
    }
  }

  nxFile.closeGroup(); // processing NXprocess

  // This seems to be required for certain extensions that can be appended
  // to these files. Not fixed for now.
  nxFile.makeGroup("intermediate", "NXcollection", false);

  nxFile.closeGroup(); // Top level entry (NXentry)

  nxFile.close();
}

} // namespace DataHandling
} // namespace Mantid
