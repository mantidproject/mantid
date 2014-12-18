// SaveNeXus
// @author Freddie Akeroyd, STFC ISIS Faility
// @author Ronald Fowler, STFC eScience. Modified to fit with SaveNexusProcessed
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveNexus.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/BoundedValidator.h"

#include <cmath>
#include <boost/shared_ptr.hpp>
#include <Poco/File.h>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveNexus)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

/// Empty default constructor
SaveNexus::SaveNexus() : Algorithm() {}

/** Initialisation method.
 *
 */
void SaveNexus::init() {
  // Declare required parameters, filename with ext {.nx,.nx5,xml} and input
  // workspac
  declareProperty(
      new WorkspaceProperty<Workspace>("InputWorkspace", "", Direction::Input),
      "Name of the workspace to be saved");
  std::vector<std::string> exts;
  exts.push_back(".nxs");
  exts.push_back(".nx5");
  exts.push_back(".xml");
  declareProperty(new FileProperty("Filename", "", FileProperty::Save, exts),
                  "The name of the Nexus file to write, as a full or relative\n"
                  "path");
  //
  // Declare optional input parameters
  // These are:
  // Title       - string to describe data
  // EntryNumber - integer >0 to be used in entry name "mantid_workspace_<n>"
  //                          Within a file the entries will be sequential from
  //                          1.
  //                          This option should allow overwrite of existing
  //                          entry,
  //                          *not* addition of out-of-sequence entry numbers.
  // spectrum_min, spectrum_max - range of "spectra" numbers to write
  // spectrum_list            list of spectra values to write
  //
  declareProperty("Title", "", boost::make_shared<NullValidator>(),
                  "A title to describe the saved workspace");

  auto mustBePositive = boost::make_shared<BoundedValidator<int>>();
  mustBePositive->setLower(0);
  // declareProperty("EntryNumber", Mantid::EMPTY_INT(), mustBePositive,
  //  "(Not implemented yet) The index number of the workspace within the Nexus
  //  file\n"
  // "(default leave unchanged)" );
  declareProperty(
      "WorkspaceIndexMin", 0, mustBePositive,
      "Number of first WorkspaceIndex to read, only for single period data.\n"
      "Not yet implemented");
  declareProperty(
      "WorkspaceIndexMax", Mantid::EMPTY_INT(), mustBePositive,
      "Number of last WorkspaceIndex to read, only for single period data.\n"
      "Not yet implemented.");
  declareProperty(
      new ArrayProperty<int>("WorkspaceIndexList"),
      "List of WorkspaceIndex numbers to read, only for single period data.\n"
      "Not yet implemented");
  declareProperty("Append", false, "Determines whether .nxs file needs to be\n"
                                   "over written or appended");
  // option which might be required in future - should be a choice e.g.
  // MantidProcessed/Muon1
  // declareProperty("Filetype","",new NullValidator<std::string>);
}

/** Execute the algorithm. Currently just calls SaveNexusProcessed but could
 *  call write other formats if support added
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void SaveNexus::exec() {
  // Retrieve the filename from the properties
  m_filename = getPropertyValue("FileName");
  m_inputWorkspace = getProperty("InputWorkspace");

  runSaveNexusProcessed();

  return;
}
/** virtual method to set the non workspace properties for this algorithm
 *  @param alg :: pointer to the algorithm
 *  @param propertyName :: name of the property
 *  @param propertyValue :: value  of the property
 *  @param perioidNum :: period number
 */
void SaveNexus::setOtherProperties(IAlgorithm *alg,
                                   const std::string &propertyName,
                                   const std::string &propertyValue,
                                   int perioidNum) {
  if (!propertyName.compare("Append")) {
    if (perioidNum != 1) {
      alg->setPropertyValue(propertyName, "1");
    } else
      alg->setPropertyValue(propertyName, propertyValue);
  } else
    Algorithm::setOtherProperties(alg, propertyName, propertyValue, perioidNum);
}
void SaveNexus::runSaveNexusProcessed() {
  IAlgorithm_sptr saveNexusPro =
      createChildAlgorithm("SaveNexusProcessed", 0.0, 1.0, true);
  // Pass through the same output filename
  saveNexusPro->setPropertyValue("Filename", m_filename);
  // Set the workspace property
  std::string inputWorkspace = "InputWorkspace";
  saveNexusPro->setProperty(inputWorkspace, m_inputWorkspace);
  //
  std::vector<int> specList = getProperty("WorkspaceIndexList");
  if (!specList.empty())
    saveNexusPro->setPropertyValue("WorkspaceIndexList",
                                   getPropertyValue("WorkspaceIndexList"));
  //
  int specMax = getProperty("WorkspaceIndexMax");
  if (specMax != Mantid::EMPTY_INT()) {
    saveNexusPro->setPropertyValue("WorkspaceIndexMax",
                                   getPropertyValue("WorkspaceIndexMax"));
    saveNexusPro->setPropertyValue("WorkspaceIndexMin",
                                   getPropertyValue("WorkspaceIndexMin"));
  }
  std::string title = getProperty("Title");
  if (!title.empty())
    saveNexusPro->setPropertyValue("Title", getPropertyValue("Title"));

  // Pass through the append property
  saveNexusPro->setProperty<bool>("Append", getProperty("Append"));

  // If we're tracking history, add the entry before we save it to file
  if (trackingHistory()) {
    m_history->fillAlgorithmHistory(
        this, Mantid::Kernel::DateAndTime::getCurrentTime(), -1,
        Algorithm::g_execCount);
    if (!isChild()) {
      m_inputWorkspace->history().addHistory(m_history);
    }
    // this is a child algorithm, but we still want to keep the history.
    else if (isRecordingHistoryForChild() && m_parentHistory) {
      m_parentHistory->addChildHistory(m_history);
    }
  }
  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  try {
    saveNexusPro->execute();
  } catch (std::runtime_error &) {
    g_log.error(
        "Unable to successfully run SaveNexusprocessed Child Algorithm");
  }
  if (!saveNexusPro->isExecuted())
    g_log.error(
        "Unable to successfully run SaveNexusProcessed Child Algorithm");
  //
  progress(1);
}

/**
Overriden process groups.
*/
bool SaveNexus::processGroups() {
  this->exec();

  // We finished successfully.
  setExecuted(true);
  notificationCenter().postNotification(
      new FinishedNotification(this, isExecuted()));

  return true;
}

} // namespace DataHandling
} // namespace Mantid
