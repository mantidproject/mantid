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

namespace Mantid {
namespace DataHandling {

DECLARE_ALGORITHM(LoadInstrument)

using namespace Kernel;
using namespace API;
using namespace Geometry;

std::recursive_mutex LoadInstrument::m_mutex;

// This enum class is used to remember easily which instrument loader should be
// used. There are 3 different loaders: from XML string, from an IDF file and
// from a Nexus file. Some parts of the exec() function are common to Xml and
// Idf, some parts are common to Idf and Nxs, and some parts are common to all
// three. Assigning numbers to the 3 types allows us to write statements like
// if (loader_type < LoaderType::Nxs) then do all things common to Xml and Idf.
enum class LoaderType { Xml = 1, Idf = 2, Nxs = 3 };

/// Initialisation method.
void LoadInstrument::init() {
  // When used as a Child Algorithm the workspace name is not used - hence the
  // "Anonymous" to satisfy the validator
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "Workspace", "Anonymous", Direction::InOut),
                  "The name of the workspace to load the instrument definition "
                  "into. Any existing instrument will be replaced.");
  declareProperty(
      std::make_unique<FileProperty>("Filename", "", FileProperty::OptionalLoad,
                                     LoadGeometry::validExtensions()),
      "The filename (including its full or relative path) of an instrument "
      "definition file. The file extension must either be .xml or .XML when "
      "specifying an instrument definition file. Files can also be .hdf5 or "
      ".nxs for usage with NeXus Geometry files. Note Filename or "
      "InstrumentName must be specified but not both.");
  declareProperty(std::make_unique<ArrayProperty<detid_t>>("MonitorList",
                                                           Direction::Output),
                  "Will be filled with a list of the detector ids of any "
                  "monitors loaded in to the workspace.");
  declareProperty(
      "InstrumentName", "",
      "Name of instrument. Can be used instead of Filename to specify an"
      "instrument definition.");
  declareProperty("InstrumentXML", "",
                  "The full XML instrument definition as a string.");
  declareProperty(
      std::make_unique<PropertyWithValue<OptionalBool>>(
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
  // std::pair<std::string, std::string> loader_type;
  LoaderType loader_type;

  // If instrumentXML is not default (i.e. it has been defined), then use that
  // Note: this is part of the IDF loader
  const Property *const InstrumentXML = getProperty("InstrumentXML");
  if (!InstrumentXML->isDefault()) {
    // We need the instrument name to be set as well because, for whatever
    // reason, this isn't pulled out of the XML.
    if (instname.empty())
      throw std::runtime_error("The InstrumentName property must be set when "
                               "using the InstrumentXML property.");
    // If the Filename property is not set, set it to the same as the instrument
    // name
    if (filename.empty())
      filename = instname;

    // Assign the loader type to Xml
    loader_type = LoaderType::Xml;

  } else {
    // This part of the loader searches through the instrument directories for
    // a valid IDF or Nexus geometry file

    // The first step is to define a valid filename

    // If the filename is empty, try to find a file from the instrument name
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
        filename = ExperimentInfo::getInstrumentFilename(
            instname, ws->getWorkspaceStartDate());
        setPropertyValue("Filename", filename);
      }
    }
    if (filename.empty()) {
      throw Exception::NotFoundError(
          "Unable to find an Instrument File for instrument: ", instname);
    }

    // Remove the path from the filename for use with the InstrumentDataService
    const std::string::size_type stripPath = filename.find_last_of("\\/");
    std::string instrumentFile =
        filename.substr(stripPath + 1, filename.size());
    // Strip off "_Definition.xml"
    instname = instrumentFile.substr(0, instrumentFile.find("_Def"));

    // Now that we have a file name, decide whether to use Nexus or IDF loading
    if (LoadGeometry::isIDF(filename)) {
      // Assign the loader type to Idf
      loader_type = LoaderType::Idf;
    } else if (LoadGeometry::isNexus(filename)) {
      // Assign the loader type to Nxs
      loader_type = LoaderType::Nxs;
    } else {
      throw Kernel::Exception::FileError(
          "No valid loader found for instrument file ", filename);
    }
  }

  InstrumentDefinitionParser parser;
  std::string instrumentNameMangled;
  Instrument_sptr instrument;

  // Define a parser if using IDFs
  if (loader_type == LoaderType::Xml)
    parser =
        InstrumentDefinitionParser(filename, instname, InstrumentXML->value());
  else if (loader_type == LoaderType::Idf)
    parser = InstrumentDefinitionParser(filename, instname,
                                        Strings::loadFile(filename));

  // Find the mangled instrument name that includes the modified date
  if (loader_type < LoaderType::Nxs)
    instrumentNameMangled = parser.getMangledName();
  else if (loader_type == LoaderType::Nxs)
    instrumentNameMangled =
        NexusGeometry::NexusGeometryParser::getMangledName(filename, instname);
  else
    throw std::runtime_error("Unknown instrument LoaderType");

  {
    // Make InstrumentService access thread-safe
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // Check whether the instrument is already in the InstrumentDataService
    if (InstrumentDataService::Instance().doesExist(instrumentNameMangled)) {
      // If it does, just use the one from the one stored there
      instrument =
          InstrumentDataService::Instance().retrieve(instrumentNameMangled);
    } else {

      if (loader_type < LoaderType::Nxs) {
        // Really create the instrument
        Progress prog(this, 0.0, 1.0, 100);
        instrument = parser.parseXML(&prog);
        // Parse the instrument tree (internally create ComponentInfo and
        // DetectorInfo). This is an optimization that avoids duplicate parsing
        // of the instrument tree when loading multiple workspaces with the same
        // instrument. As a consequence less time is spent and less memory is
        // used. Note that this is only possible since the tree in `instrument`
        // will not be modified once we add it to the IDS.
        instrument->parseTreeAndCacheBeamline();
      } else {
        Instrument_const_sptr ins =
            NexusGeometry::NexusGeometryParser::createInstrument(
                filename, NexusGeometry::makeLogger(&m_log));
        instrument = boost::const_pointer_cast<Instrument>(ins);
      }
      // Add to data service for later retrieval
      InstrumentDataService::Instance().add(instrumentNameMangled, instrument);
    }
    ws->setInstrument(instrument);

    // populate parameter map of workspace
    ws->populateInstrumentParameters();

    // LoadParameterFile modifies the base instrument stored in the IDS so this
    // must also be protected by the lock until LoadParameterFile is fixed.
    // check if default parameter file is also present, unless loading from
    if (!filename.empty() && (loader_type < LoaderType::Nxs))
      runLoadParameterFile(ws, filename);
  } // end of mutex scope

  // Set the monitors output property
  setProperty("MonitorList", (ws->getInstrument())->getMonitors());

  // Rebuild the spectra map for this workspace so that it matches the
  // instrument, if required
  const OptionalBool RewriteSpectraMap = getProperty("RewriteSpectraMap");
  if (RewriteSpectraMap == OptionalBool::True)
    ws->rebuildSpectraMapping();
}

//-----------------------------------------------------------------------------------------------------------------------
/// Run the Child Algorithm LoadInstrument (or LoadInstrumentFromRaw)
void LoadInstrument::runLoadParameterFile(
    const boost::shared_ptr<API::MatrixWorkspace> &ws, std::string filename) {
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

    for (const auto &name : directoryNames) {
      // This will iterate around the directories from user ->etc ->install, and
      // find the first beat file
      fullPathParamIDF = getFullPathParamIDF(name, filename);
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
