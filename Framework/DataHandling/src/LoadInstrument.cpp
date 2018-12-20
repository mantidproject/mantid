#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/Strings.h"

#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
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
                                ".xml"),
      "The filename (including its full or relative path) of an instrument "
      "definition file. The file extension must either be .xml or .XML when "
      "specifying an instrument definition file. Note Filename or "
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
  m_workspace = getProperty("Workspace");
  m_filename = getPropertyValue("Filename");
  m_instName = getPropertyValue("InstrumentName");

  // We will parse the XML using the InstrumentDefinitionParser
  InstrumentDefinitionParser parser;

  // If the XML is passed in via the InstrumentXML property, use that.
  const Property *const InstrumentXML = getProperty("InstrumentXML");
  if (!InstrumentXML->isDefault()) {
    // We need the instrument name to be set as well because, for whatever
    // reason,
    //   this isn't pulled out of the XML.
    if (m_instName.empty())
      throw std::runtime_error("The InstrumentName property must be set when "
                               "using the InstrumentXML property.");
    // If the Filename property is not set, set it to the same as the instrument
    // name
    if (m_filename.empty())
      m_filename = m_instName;

    // Initialize the parser. Avoid copying the xmltext out of the property
    // here.
    const PropertyWithValue<std::string> *xml =
        dynamic_cast<const PropertyWithValue<std::string> *>(InstrumentXML);
    if (xml) {
      parser = InstrumentDefinitionParser(m_filename, m_instName, *xml);
    } else {
      throw std::invalid_argument("The instrument XML passed cannot be "
                                  "casted to a standard string.");
    }
  }
  // otherwise we need either Filename or InstrumentName to be set
  else {
    // Retrieve the filename from the properties
    if (m_filename.empty()) {
      // look to see if an Instrument name provided in which case create
      // IDF filename on the fly
      if (m_instName.empty()) {
        g_log.error("Either the InstrumentName or Filename property of "
                    "LoadInstrument most be specified");
        throw Kernel::Exception::FileError(
            "Either the InstrumentName or Filename property of LoadInstrument "
            "most be specified to load an IDF",
            m_filename);
      } else {
        const std::string date = m_workspace->getWorkspaceStartDate();
        m_filename = ExperimentInfo::getInstrumentFilename(m_instName, date);
      }
    }

    if (m_filename.empty()) {
      throw Exception::NotFoundError(
          "Unable to find an Instrument Definition File for", m_instName);
    }

    // Remove the path from the filename for use with the InstrumentDataService
    const std::string::size_type stripPath = m_filename.find_last_of("\\/");
    std::string instrumentFile =
        m_filename.substr(stripPath + 1, m_filename.size());
    // Strip off "_Definition.xml"
    m_instName = instrumentFile.substr(0, instrumentFile.find("_Def"));

    // Initialize the parser with the the XML text loaded from the IDF file
    parser = InstrumentDefinitionParser(m_filename, m_instName,
                                        Strings::loadFile(m_filename));
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
    m_workspace->setInstrument(instrument);

    // populate parameter map of workspace
    m_workspace->populateInstrumentParameters();

    // LoadParameterFile modifies the base instrument stored in the IDS so this
    // must also be protected by the lock until LoadParameterFile is fixed.
    // check if default parameter file is also present, unless loading from
    if (!m_filename.empty())
      runLoadParameterFile();
  }

  // Set the monitors output property
  setProperty("MonitorList", instrument->getMonitors());

  // Rebuild the spectra map for this workspace so that it matches the
  // instrument
  // if required
  const OptionalBool RewriteSpectraMap = getProperty("RewriteSpectraMap");
  if (RewriteSpectraMap == OptionalBool::True)
    m_workspace->rebuildSpectraMapping();
}

//-----------------------------------------------------------------------------------------------------------------------
/// Run the Child Algorithm LoadInstrument (or LoadInstrumentFromRaw)
void LoadInstrument::runLoadParameterFile() {
  g_log.debug("Loading the parameter definition...");

  // First search for XML parameter file in same folder as IDF file
  const std::string::size_type dir_end = m_filename.find_last_of("\\/");
  std::string directoryName =
      m_filename.substr(0, dir_end + 1); // include final '/'.
  std::string fullPathParamIDF = getFullPathParamIDF(directoryName);

  if (fullPathParamIDF.empty()) {
    // Not found, so search the other places were it may occur
    Kernel::ConfigServiceImpl &configService =
        Kernel::ConfigService::Instance();
    std::vector<std::string> directoryNames =
        configService.getInstrumentDirectories();

    for (const auto &directoryName : directoryNames) {
      // This will iterate around the directories from user ->etc ->install, and
      // find the first beat file
      fullPathParamIDF = getFullPathParamIDF(directoryName);
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
      loadParamAlg->setProperty("Workspace", m_workspace);
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
std::string LoadInstrument::getFullPathParamIDF(std::string directoryName) {
  Poco::Path directoryPath(directoryName);
  directoryPath.makeDirectory();
  // Remove the path from the filename
  Poco::Path filePath(m_filename);
  const std::string &instrumentFile = filePath.getFileName();

  // First check whether there is a parameter file whose name is the same as the
  // IDF file,
  // but with 'Parameters' instead of 'Definition'.
  std::string definitionPart("_Definition");
  const std::string::size_type prefix_end(instrumentFile.find(definitionPart));
  const std::string::size_type suffix_start =
      prefix_end + definitionPart.length();
  // Make prefix and force it to be upper case
  std::string prefix = instrumentFile.substr(0, prefix_end);
  std::transform(prefix.begin(), prefix.end(), prefix.begin(), toupper);
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
