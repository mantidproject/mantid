// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadIDFFromNexus.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Strings.h"
#include "MantidNexusCpp/NeXusFile.hpp"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/File.h>
#include <Poco/Path.h>

using Poco::XML::Document;
using Poco::XML::DOMParser;
using Poco::XML::Element;
using Poco::XML::NodeList;

namespace Mantid::DataHandling {

DECLARE_ALGORITHM(LoadIDFFromNexus)

using namespace Kernel;
using namespace API;
using Types::Core::DateAndTime;

/// Empty default constructor
LoadIDFFromNexus::LoadIDFFromNexus() = default;

/// Initialisation method.
void LoadIDFFromNexus::init() {
  // When used as a Child Algorithm the workspace name is not used - hence the
  // "Anonymous" to satisfy the validator
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("Workspace", "Anonymous", Direction::InOut),
                  "The name of the workspace in which to attach the imported instrument");

  const std::vector<std::string> exts{".nxs", ".nxs.h5"};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
                  "The name (including its full or relative path) of the Nexus file to "
                  "attempt to load the instrument from.");

  declareProperty("InstrumentParentPath", std::string(""),
                  "Path name within the Nexus tree of the folder containing "
                  "the instrument folder. "
                  "For example it is 'raw_data_1' for an ISIS raw Nexus file "
                  "and 'mantid_workspace_1' for a processed nexus file. "
                  "Only a one level path is curently supported",
                  Direction::Input);
  declareProperty("ParameterCorrectionFilePath", std::string(""),
                  "Full path name of Parameter Correction file. "
                  "This should only be used in a situation,"
                  "where the default full file path is inconvenient.",
                  Direction::Input);
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw FileError Thrown if unable to parse XML file
 */
void LoadIDFFromNexus::exec() {
  // Retrieve the filename from the properties
  const std::string filename = getPropertyValue("Filename");

  // Get the input workspace
  const MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");

  // Get the instrument path
  std::string instrumentParentPath = getPropertyValue("InstrumentParentPath");

  // Get the instrument group in the Nexus file
  ::NeXus::File nxfile(filename);
  // Assume one level in instrument path
  nxfile.openPath(instrumentParentPath);

  // Take instrument info from nexus file.
  localWorkspace->loadInstrumentInfoNexus(filename, &nxfile);

  // Look for parameter correction file
  std::string parameterCorrectionFile = getPropertyValue("ParameterCorrectionFilePath");
  if (parameterCorrectionFile.empty()) {
    parameterCorrectionFile = getParameterCorrectionFile(localWorkspace->getInstrument()->getName());
  }
  g_log.debug() << "Parameter correction file: " << parameterCorrectionFile << "\n";

  // Read parameter correction file, if found
  std::string correctionParameterFile;
  bool append = false;
  if (!parameterCorrectionFile.empty()) {
    // Read parameter correction file
    // to find out which parameter file to use
    // and whether it is appended to default parameters.
    g_log.notice() << "Using parameter correction file: " << parameterCorrectionFile << ".\n";
    readParameterCorrectionFile(parameterCorrectionFile, localWorkspace->getAvailableWorkspaceStartDate(),
                                correctionParameterFile, append);
  }

  // Load default parameters if either there is no correction parameter file or
  // it is to be appended.
  if (correctionParameterFile.empty() || append) {
    LoadParameters(&nxfile, localWorkspace);
  } else { // Else clear the parameters
    g_log.notice() << "Parameters to be replaced are cleared.\n";
    localWorkspace->getInstrument()->getParameterMap()->clear();
  }

  // Load parameters from correction parameter file, if it exists
  if (!correctionParameterFile.empty()) {
    Poco::Path corrFilePath(parameterCorrectionFile);
    g_log.debug() << "Correction file path: " << corrFilePath.toString() << "\n";
    Poco::Path corrDirPath = corrFilePath.parent();
    g_log.debug() << "Correction directory path: " << corrDirPath.toString() << "\n";
    Poco::Path corrParamFile(corrDirPath, correctionParameterFile);
    if (append) {
      g_log.notice() << "Using correction parameter file: " << corrParamFile.toString() << " to append parameters.\n";
    } else {
      g_log.notice() << "Using correction parameter file: " << corrParamFile.toString() << " to replace parameters.\n";
    }
    loadParameterFile(corrParamFile.toString(), localWorkspace);
  } else {
    g_log.notice() << "No correction parameter file applies to the date for "
                      "correction file.\n";
  }
}

/*  Gets the full pathname of the parameter correction file, if it exists
 * @param instName :: short name of instrument as it appears in IDF filename
 * etc.
 * @returns  full path name of correction file if found else ""
 */
std::string LoadIDFFromNexus::getParameterCorrectionFile(const std::string &instName) {

  std::vector<std::string> directoryNames = ConfigService::Instance().getInstrumentDirectories();
  for (auto &directoryName : directoryNames) {
    // This will iterate around the directories from user ->etc ->install, and
    // find the first appropriate file
    Poco::Path iPath(directoryName,
                     "embedded_instrument_corrections"); // Go to correction file subfolder
    // First see if the directory exists
    Poco::File ipDir(iPath);
    if (ipDir.exists() && ipDir.isDirectory()) {
      iPath.append(instName + "_Parameter_Corrections.xml"); // Append file name to pathname
      Poco::File ipFile(iPath);
      if (ipFile.exists() && ipFile.isFile()) {
        return ipFile.path(); // Return first found
      }
    } // Directory
  } // Loop
  return ""; // No file found
}

/* Reads the parameter correction file and if a correction is needed output the
 *parameterfile needed
 *  and whether it is to be appended.
 * @param correction_file :: path nsame of correction file as returned by
 *getParameterCorrectionFile()
 * @param date :: IS8601 date string applicable: Must be full timestamp
 *(timezone optional)
 * @param parameter_file :: output parameter file to use or "" if none
 * @param append :: output whether the parameters from parameter_file should be
 *appended.
 *
 *  @throw FileError Thrown if unable to parse XML file
 */
void LoadIDFFromNexus::readParameterCorrectionFile(const std::string &correction_file, const std::string &date,
                                                   std::string &parameter_file, bool &append) {
  using namespace Poco::XML;
  // Set output arguments to default
  parameter_file = "";
  append = false;

  // Check the date.
  if (date.empty()) {
    g_log.notice() << "No date is supplied for parameter correction file " << correction_file
                   << ". Correction file is ignored.\n";
    return;
  }

  // Get contents of correction file
  const std::string xmlText = Kernel::Strings::loadFile(correction_file);

  // Set up the DOM parser and parse xml file
  DOMParser pParser;
  Poco::AutoPtr<Document> pDoc;
  try {
    pDoc = pParser.parseString(xmlText);
  } catch (Poco::Exception &exc) {
    throw Kernel::Exception::FileError(exc.displayText() + ". Unable to parse parameter correction file:",
                                       correction_file);
  } catch (...) {
    throw Kernel::Exception::FileError("Unable to parse parameter correction file:", correction_file);
  }
  // Get pointer to root element
  Element *pRootElem = pDoc->documentElement();
  if (!pRootElem->hasChildNodes()) {
    g_log.error("Parameter correction file: " + correction_file + "contains no XML root element.");
    throw Kernel::Exception::InstrumentDefinitionError("No root element in XML parameter correction file",
                                                       correction_file);
  }

  // Convert date to Mantid object
  g_log.notice() << "Date for correction file " << date << "\n";
  DateAndTime externalDate(date);

  // Examine the XML structure obtained by parsing
  Poco::AutoPtr<NodeList> correctionNodeList = pRootElem->getElementsByTagName("correction");
  for (unsigned long i = 0; i < correctionNodeList->length(); ++i) {
    // For each correction element
    auto *corr = dynamic_cast<Element *>(correctionNodeList->item(i));
    if (corr) {
      DateAndTime start(corr->getAttribute("valid-from"));
      DateAndTime end(corr->getAttribute("valid-to"));
      if (start <= externalDate && externalDate <= end) {
        parameter_file = corr->getAttribute("file");
        append = (corr->getAttribute("append") == "true");
        break;
      }
    } else {
      g_log.error("Parameter correction file: " + correction_file + "contains an invalid correction element.");
      throw Kernel::Exception::InstrumentDefinitionError("Invalid element in XML parameter correction file",
                                                         correction_file);
    }
  }
}

/** Loads the parameters from the Nexus file if possible, else from a parameter
 *file
 *  into the specified workspace
 * @param nxfile :: open NeXus file
 * @param localWorkspace :: workspace into which loading occurs
 *
 *  @throw FileError Thrown if unable to parse XML file
 */
void LoadIDFFromNexus::LoadParameters(::NeXus::File *nxfile, const MatrixWorkspace_sptr &localWorkspace) {

  std::string parameterString;

  // First attempt to load parameters from nexus file.
  nxfile->openGroup("instrument", "NXinstrument");
  localWorkspace->loadInstrumentParametersNexus(nxfile, parameterString);
  nxfile->closeGroup();

  // loadInstrumentParametersNexus does not populate any instrument params
  // so we do it here.
  localWorkspace->populateInstrumentParameters();

  if (parameterString.empty()) {
    // No parameters have been found in Nexus file, so we look for them in a
    // parameter file.
    std::vector<std::string> directoryNames = ConfigService::Instance().getInstrumentDirectories();
    const std::string instrumentName = localWorkspace->getInstrument()->getName();
    for (const auto &directoryName : directoryNames) {
      // This will iterate around the directories from user ->etc ->install, and
      // find the first appropriate file
      const std::string paramFile = directoryName + instrumentName + "_Parameters.xml";

      // Attempt to load specified file, if successful, use file and stop
      // search.
      if (loadParameterFile(paramFile, localWorkspace))
        break;
    }
  } else { // We do have parameters from the Nexus file
    g_log.notice() << "Found Instrument parameter map entry in Nexus file, "
                      "which is loaded.\n\n";
    // process parameterString into parameters in workspace
    localWorkspace->readParameterMap(parameterString);
  }
}

// Private function to load parameter file specified by a full path name into
// given workspace, returning success.
bool LoadIDFFromNexus::loadParameterFile(const std::string &fullPathName, const MatrixWorkspace_sptr &localWorkspace) {

  try {
    // load and also populate instrument parameters from this 'fallback'
    // parameter file
    Algorithm_sptr loadParamAlg = createChildAlgorithm("LoadParameterFile");
    loadParamAlg->setProperty("Filename", fullPathName);
    loadParamAlg->setProperty("Workspace", localWorkspace);
    loadParamAlg->execute();
    g_log.notice() << "Instrument parameter file: " << fullPathName << " has been loaded.\n\n";
    return true; // Success
  } catch (std::invalid_argument &e) {
    g_log.information("LoadParameterFile: No parameter file found for this instrument");
    g_log.information(e.what());
  } catch (std::runtime_error &e) {
    g_log.information("Unable to successfully run LoadParameterFile Child Algorithm");
    g_log.information(e.what());
  }
  return false;
}

} // namespace Mantid::DataHandling
