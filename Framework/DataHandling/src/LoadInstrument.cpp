// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidDataHandling/LoadGeometry.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/Strings.h"
#include "MantidNexusGeometry/NexusGeometryParser.h"
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/Exception.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <fstream>
#include <sstream>

namespace Mantid {
namespace DataHandling {

DECLARE_ALGORITHM(LoadInstrument)

using namespace Kernel;
using namespace API;
using namespace Geometry;

std::recursive_mutex LoadInstrument::m_mutex;

/// Initialisation method.
void LoadInstrument::init() {
  // When used as a Child Algorithm the workspace name is not used - hence the
  // "Anonymous" to satisfy the validator
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "Workspace", "Anonymous", Direction::InOut),
                  "The name of the workspace to load the instrument definition "
                  "into. Any existing instrument will be replaced.");
  declareProperty(
      make_unique<FileProperty>("Filename", "", FileProperty::OptionalLoad,
                                LoadGeometry::validExtensions()),
      "The filename (including its full or relative path) of an instrument "
      "definition file. The file extension must either be .xml or .XML when "
      "specifying an instrument definition file. Files can also be .hdf5 or "
      ".nxs for usage with NeXus Geometry files. Note Filename or "
      "InstrumentName must be specified but not both.");
  declareProperty(
      make_unique<ArrayProperty<detid_t>>("MonitorList", Direction::Output),
      "Will be filled with a list of the detector ids of any "
      "monitors loaded in to the workspace.");
  declareProperty(
      "InstrumentName", "",
      "Name of instrument. Can be used instead of Filename to specify an IDF");
  declareProperty("InstrumentXML", "",
                  "The full XML instrument definition as a string.");
  declareProperty(
      make_unique<PropertyWithValue<OptionalBool>>(
          "RewriteSpectraMap", OptionalBool::Unset,
          boost::make_shared<MandatoryValidator<OptionalBool>>()),
      "If set to True then a 1:1 map between the spectrum numbers and "
      "detector/monitor IDs is set up such that the detector/monitor IDs in "
      "the IDF are ordered from smallest to largest number and then assigned "
      "in that order to the spectra in the workspace. For example if the IDF "
      "has defined detectors/monitors with IDs 1, 5, 10 and the workspace "
      "contains 3 spectra with numbers 1, 2, 3 (and workspace indices 0, 1, 2) "
      "then spectrum number 1 is associated with detector ID 1, spectrum "
      "number 2 with detector ID 5 and spectrum number 3 with detector ID 10."
      "If the number of spectra and detectors do not match then the operation "
      "is performed until the maximum number of either is reached. For example "
      "if there are 12 spectra and 50 detectors then the first 12 detectors "
      "are assigned to the 12 spectra in the workspace."
      "If set to False then the spectrum numbers and detector IDs of the "
      "workspace are not modified."
      "This property must be set to either True or False.");
}

//------------------------------------------------------------------------------------------------------------------------------
/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw FileError Thrown if unable to parse XML file
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML
 *instrument file
 */
void LoadInstrument::exec() {
  // Get the input workspace
  boost::shared_ptr<API::MatrixWorkspace> ws = getProperty("Workspace");
  std::string filename = getPropertyValue("Filename");
  std::string instname = getPropertyValue("InstrumentName");

  // Decide whether to use Nexus or IDF loading
  // Note: for now, if only the instrument name is provided, the IDF loader will
  // be used by default. This should be updated in the future, so that the
  // Nexus loader can also take in an instrument name. Possibly by adding
  // a "FileType" property to the LoadInstrument algorithm.
  if (LoadGeometry::isIDF(filename, instname))
    idfInstrumentLoader(ws, filename, instname);
  else if (LoadGeometry::isNexus(filename))
    nexusInstrumentLoader(ws, filename, instname);
  else
    throw Kernel::Exception::FileError("Instrument input cannot be read",
                                       filename);

  // Set the monitors output property
  setProperty("MonitorList", (ws->getInstrument())->getMonitors());

  // Rebuild the spectra map for this workspace so that it matches the
  // instrument, if required
  const OptionalBool RewriteSpectraMap = getProperty("RewriteSpectraMap");
  if (RewriteSpectraMap == OptionalBool::True)
    ws->rebuildSpectraMapping();
}

/// Load instrument from IDF XML file
void LoadInstrument::idfInstrumentLoader(
    boost::shared_ptr<API::MatrixWorkspace> &ws, std::string filename,
    std::string instname) {

  // We will parse the XML using the InstrumentDefinitionParser
  InstrumentDefinitionParser parser;

  // If the XML is passed in via the InstrumentXML property, use that.
  const Property *const InstrumentXML = getProperty("InstrumentXML");
  if (!InstrumentXML->isDefault()) {
    // We need the instrument name to be set as well because, for whatever
    // reason,
    //   this isn't pulled out of the XML.
    if (instname.empty())
      throw std::runtime_error("The InstrumentName property must be set when "
                               "using the InstrumentXML property.");
    // If the Filename property is not set, set it to the same as the instrument
    // name
    if (filename.empty())
      filename = instname;

    // Initialize the parser. Avoid copying the xmltext out of the property
    // here.
    const PropertyWithValue<std::string> *xml =
        dynamic_cast<const PropertyWithValue<std::string> *>(InstrumentXML);
    if (xml) {
      parser = InstrumentDefinitionParser(filename, instname, *xml);
    } else {
      throw std::invalid_argument("The instrument XML passed cannot be "
                                  "casted to a standard string.");
    }
  }
  // otherwise we need either Filename or InstrumentName to be set
  else {
    filename = checkAndRetrieveInstrumentFilename(ws, filename, instname,
                                                  FileType::Idf);
    // Remove the path from the filename for use with the InstrumentDataService
    const std::string::size_type stripPath = filename.find_last_of("\\/");
    std::string instrumentFile =
        filename.substr(stripPath + 1, filename.size());
    // Strip off "_Definition.xml"
    instname = instrumentFile.substr(0, instrumentFile.find("_Def"));

    // Initialize the parser with the the XML text loaded from the IDF file
    parser = InstrumentDefinitionParser(filename, instname,
                                        Strings::loadFile(filename));
  }

  // Find the mangled instrument name that includes the modified date
  std::string instrumentNameMangled = parser.getMangledName();

  Instrument_sptr instrument;
  // Check whether the instrument is already in the InstrumentDataService
  {
    // Make InstrumentService access thread-safe
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    if (InstrumentDataService::Instance().doesExist(instrumentNameMangled)) {
      // If it does, just use the one from the one stored there
      instrument =
          InstrumentDataService::Instance().retrieve(instrumentNameMangled);
    } else {
      // Really create the instrument
      Progress prog(this, 0.0, 1.0, 100);
      instrument = parser.parseXML(&prog);
      // Parse the instrument tree (internally create ComponentInfo and
      // DetectorInfo). This is an optimization that avoids duplicate parsing of
      // the instrument tree when loading multiple workspaces with the same
      // instrument. As a consequence less time is spent and less memory is
      // used. Note that this is only possible since the tree in `instrument`
      // will not be modified once we add it to the IDS.
      instrument->parseTreeAndCacheBeamline();
      // Add to data service for later retrieval
      InstrumentDataService::Instance().add(instrumentNameMangled, instrument);
    }
    ws->setInstrument(instrument);

    // populate parameter map of workspace
    ws->populateInstrumentParameters();

    // LoadParameterFile modifies the base instrument stored in the IDS so this
    // must also be protected by the lock until LoadParameterFile is fixed.
    // check if default parameter file is also present, unless loading from
    if (!filename.empty())
      runLoadParameterFile(ws, filename);
  }
}

void LoadInstrument::nexusInstrumentLoader(
    boost::shared_ptr<API::MatrixWorkspace> &ws, std::string filename,
    std::string instname) {
  filename = checkAndRetrieveInstrumentFilename(ws, filename, instname,
                                                FileType::Nexus);
  Instrument_const_sptr instrument =
      NexusGeometry::NexusGeometryParser::createInstrument(filename);
  ws->setInstrument(instrument);
  ws->populateInstrumentParameters();
}

/// Get the file name from the instrument name if it is not defined
std::string LoadInstrument::checkAndRetrieveInstrumentFilename(
    boost::shared_ptr<API::MatrixWorkspace> &ws, std::string filename,
    std::string instname, const FileType &filetype) {
  // Retrieve the filename from the properties
  std::string instrumentfname;
  if (filename.empty()) {
    // look to see if an Instrument name provided in which case create
    // filename on the fly
    if (instname.empty()) {
      g_log.error("Either the InstrumentName or Filename property of "
                  "LoadInstrument most be specified");
      throw Kernel::Exception::FileError(
          "Either the InstrumentName or Filename property of LoadInstrument "
          "must be specified to load an instrument",
          filename);
    } else {
      instrumentfname = ExperimentInfo::getInstrumentFilename(
          instname, ws->getWorkspaceStartDate(), filetype);
      setPropertyValue("Filename", instrumentfname);
    }
  } else {
    instrumentfname = filename;
  }
  if (instrumentfname.empty()) {
    throw Exception::NotFoundError(
        "Unable to find an Instrument Definition File for", instname);
  }
  return instrumentfname;
}

//-----------------------------------------------------------------------------------------------------------------------
/// Run the Child Algorithm LoadInstrument (or LoadInstrumentFromRaw)
void LoadInstrument::runLoadParameterFile(
    boost::shared_ptr<API::MatrixWorkspace> &ws, std::string filename) {
  g_log.debug("Loading the parameter definition...");

  // First search for XML parameter file in same folder as IDF file
  const std::string::size_type dir_end = filename.find_last_of("\\/");
  std::string directoryName =
      filename.substr(0, dir_end + 1); // include final '/'.
  std::string fullPathParamIDF = getFullPathParamIDF(directoryName, filename);

  if (fullPathParamIDF.empty()) {
    // Not found, so search the other places were it may occur
    Kernel::ConfigServiceImpl &configService =
        Kernel::ConfigService::Instance();
    std::vector<std::string> directoryNames =
        configService.getInstrumentDirectories();

    for (const auto &directoryName : directoryNames) {
      // This will iterate around the directories from user ->etc ->install, and
      // find the first beat file
      fullPathParamIDF = getFullPathParamIDF(directoryName, filename);
      // stop when you find the first one
      if (!fullPathParamIDF.empty())
        break;
    }
  }

  if (!fullPathParamIDF.empty()) {

    g_log.debug() << "Parameter file: " << fullPathParamIDF << '\n';
    // Now execute the Child Algorithm. Catch and log any error, but don't stop.
    try {
      // To allow the use of ExperimentInfo instead of workspace, we call it
      // manually
      Algorithm_sptr loadParamAlg = createChildAlgorithm("LoadParameterFile");
      loadParamAlg->setProperty("Filename", fullPathParamIDF);
      loadParamAlg->setProperty("Workspace", ws);
      loadParamAlg->execute();
      g_log.debug("Parameters loaded successfully.");
    } catch (std::invalid_argument &e) {
      g_log.information(
          "LoadParameterFile: No parameter file found for this instrument");
      g_log.information(e.what());
    } catch (std::runtime_error &e) {
      g_log.information(
          "Unable to successfully run LoadParameterFile Child Algorithm");
      g_log.information(e.what());
    }
  } else {
    g_log.information("No parameter file found for this instrument");
  }
}

//-----------------------------------------------------------------------------------------------------------------------
/// Search the directory for the Parameter IDF file and return full path name if
/// found, else return "".
//  directoryName must include a final '/'.
std::string LoadInstrument::getFullPathParamIDF(std::string directoryName,
                                                std::string filename) {
  Poco::Path directoryPath(directoryName);
  directoryPath.makeDirectory();
  // Remove the path from the filename
  Poco::Path filePath(filename);
  const std::string &instrumentFile = filePath.getFileName();

  // First check whether there is a parameter file whose name is the same as the
  // IDF file,
  // but with 'Parameters' instead of 'Definition'.
  std::string definitionPart("_Definition");
  const std::string::size_type prefix_end(instrumentFile.find(definitionPart));
  const std::string::size_type suffix_start =
      prefix_end + definitionPart.length();
  // Get prefix and leave case sensitive
  std::string prefix = instrumentFile.substr(0, prefix_end);
  // Make suffix ensuring it has positive length
  std::string suffix = ".xml";
  if (suffix_start < instrumentFile.length()) {
    suffix = instrumentFile.substr(suffix_start, std::string::npos);
  }

  // Assemble parameter file name
  std::string fullPathParamIDF =
      directoryPath.setFileName(prefix + "_Parameters" + suffix).toString();
  if (!Poco::File(fullPathParamIDF).exists()) { // No such file exists, so look
                                                // for file based on instrument
                                                // ID
                                                // given by the prefix
    fullPathParamIDF =
        directoryPath.setFileName(prefix + "_Parameters.xml").toString();
  }

  if (!Poco::File(fullPathParamIDF).exists()) { // No such file exists, indicate
                                                // none found in this directory.
    fullPathParamIDF = "";
  }

  return fullPathParamIDF;
}

} // namespace DataHandling
} // namespace Mantid
