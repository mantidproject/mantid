// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidNexusGeometry/NexusGeometryParser.h"

#include <nexus/NeXusException.hpp>
#include <nexus/NeXusFile.hpp>

#include <filesystem>

namespace Mantid::DataHandling {
// Register the algorithm into the algorithm factory as a file loading algorithm
DECLARE_FILELOADER_ALGORITHM(LoadEmptyInstrument)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;
using namespace HistogramData;

namespace {
const std::vector<std::string> validFilenameExtensions{".xml", ".hdf5", ".nxs", ".nxs.h5"};
enum class FilenameExtensionEnum { XML, HDF5, NXS, NXS_H5, enum_count };
typedef EnumeratedString<FilenameExtensionEnum, &validFilenameExtensions, &compareStringsCaseInsensitive>
    FilenameExtension;
} // namespace

// This method attempts to retrieve a valid, i.e. enumerated, instrument file name extension.
// Possible double and single extensions are considered. If no valid file name extension could
// be retrieved, an std::runtime_error will be thrown. For example, a file name might have a double
// extension such as ".nxs.h5", which is valid. It may also have a double extension such as .lite.nxs,
// which is invalid. In the latter case a single extension, .nxs, which is valid, should be retrieved.
std::string LoadEmptyInstrument::retrieveValidInstrumentFilenameExtension(const std::string &filename) {
  std::string ext{std::filesystem::path(filename).extension().string()};
  std::string stem{std::filesystem::path(filename).stem().string()};
  std::string pre_ext{std::filesystem::path(stem).extension().string()};

  // Test a possible double extension
  if (!pre_ext.empty()) {
    std::string double_ext{pre_ext + ext};
    try {
      FilenameExtension fne(double_ext);
      return fne.c_str();
    } catch (std::runtime_error &) {
    }
  }

  // Test a possible single extension
  try {
    FilenameExtension fne(ext);
    return fne.c_str();
  } catch (std::runtime_error &) {
  }

  std::ostringstream os;
  os << "Instrument file name \"" << filename << "\" has an invalid extension.";
  throw std::runtime_error(os.str());
}

// Return all valid instrument file name extensions
std::vector<std::string> LoadEmptyInstrument::getValidInstrumentFilenameExtensions() { return validFilenameExtensions; }

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadEmptyInstrument::confidence(Kernel::FileDescriptor &descriptor) const {
  const std::string &filePath = descriptor.filename();

  int confidence(0);
  if (descriptor.isAscii()) // Only consider an Ascii file
  {
    // Filename must contain "Definition"
    std::string::size_type stripPath = filePath.find_last_of("\\/");
    if (stripPath == std::string::npos)
      stripPath = 0;
    if (filePath.find("Definition", stripPath) != std::string::npos) {
      // We have some confidence and it depends on the filetype.
      if (descriptor.extension() == "xml") {
        confidence = 80;
      } else {
        confidence = 20;
      }
    } // Has "Definition"
  } // Ascii file
  return confidence;
}

/// Initialisation method.
void LoadEmptyInstrument::init() {

  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::OptionalLoad, validFilenameExtensions),
                  "Path to a file (full or relative) defining the instrument. The file could be an"
                  " IDF or a NeXus Geometry file. Note, Filename or InstrumentName must be specified, but not both.");
  declareProperty("InstrumentName", "",
                  "Name of instrument. Can be used instead of Filename to "
                  "specify an IDF");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace in which to store the imported instrument");

  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("DetectorValue", 1.0, mustBePositive,
                  "This value affects the colour of the detectors in the instrument\n"
                  "display window (default 1)");
  declareProperty("MonitorValue", 2.0, mustBePositive,
                  "This value affects the colour of the monitors in the instrument\n"
                  "display window (default 2)");

  declareProperty(std::make_unique<PropertyWithValue<bool>>("MakeEventWorkspace", false),
                  "Set to True to create an EventWorkspace (with no events) "
                  "instead of a Workspace2D.");
}

/** Executes the algorithm. Reading in the file and creating and populating
 *  the output workspace
 *
 *  @throw std::runtime_error If the instrument cannot be loaded by the
 *LoadInstrument ChildAlgorithm
 *  @throw InstrumentDefinitionError Thrown if issues with the content of XML
 *instrument file not covered by LoadInstrument
 *  @throw std::invalid_argument If the optional properties are set to invalid
 *values
 */
void LoadEmptyInstrument::exec() {
  Progress prog(this, 0.0, 1.0, 10);

  // load the instrument into this workspace
  MatrixWorkspace_sptr ws;
  const std::string instrumentName = getPropertyValue("InstrumentName");
  const std::string filename = getPropertyValue("Filename");
  if (!instrumentName.empty())
    ws = this->runLoadInstrument(filename, instrumentName);
  else {
    FilenameExtension enFilenameExtension{retrieveValidInstrumentFilenameExtension(filename)};
    switch (enFilenameExtension) {
    case FilenameExtensionEnum::XML:
    case FilenameExtensionEnum::HDF5:
      ws = this->runLoadInstrument(filename, instrumentName);
      break;
    case FilenameExtensionEnum::NXS:
    case FilenameExtensionEnum::NXS_H5:
      ws = this->runLoadIDFFromNexus(filename);
      break;
    default:
      std::ostringstream os;
      os << "Instrument file name has an invalid extension: "
         << "\"" << enFilenameExtension.c_str() << "\"";
      throw std::runtime_error(os.str());
    }
  }

  Instrument_const_sptr instrument = ws->getInstrument();

  // Get number of detectors stored in instrument
  const size_t number_spectra = instrument->getNumberDetectors();

  // Check that we have some spectra for the workspace
  if (number_spectra == 0) {
    g_log.error("Instrument has no detectors, unable to create workspace for it");
    throw Kernel::Exception::InstrumentDefinitionError("No detectors found in instrument");
  }

  Indexing::IndexInfo indexInfo(number_spectra);
  bool MakeEventWorkspace = getProperty("MakeEventWorkspace");
  prog.reportIncrement(5, "Creating Data");
  if (MakeEventWorkspace) {
    setProperty("OutputWorkspace",
                create<EventWorkspace>(instrument, indexInfo, BinEdges{0.0, std::numeric_limits<double>::min()}));
  } else {
    const double detector_value = getProperty("DetectorValue");
    const double monitor_value = getProperty("MonitorValue");
    auto ws2D = create<Workspace2D>(
        instrument, indexInfo,
        Histogram(BinEdges{0.0, 1.0}, Counts(1, detector_value), CountStandardDeviations(1, detector_value)));

    Counts v_monitor_y(1, monitor_value);
    CountStandardDeviations v_monitor_e(1, monitor_value);

    const auto &spectrumInfo = ws2D->spectrumInfo();
    const auto size = static_cast<int64_t>(spectrumInfo.size());
#pragma omp parallel for
    for (int64_t i = 0; i < size; i++) {
      if (spectrumInfo.isMonitor(i)) {
        ws2D->setCounts(i, v_monitor_y);
        ws2D->setCountStandardDeviations(i, v_monitor_e);
      }
    }
    setProperty("OutputWorkspace", std::move(ws2D));
  }
}

// Call LoadInstrument as a child algorithm
API::MatrixWorkspace_sptr LoadEmptyInstrument::runLoadInstrument(const std::string &filename,
                                                                 const std::string &instrumentname) {
  auto ws = WorkspaceFactory::Instance().create("Workspace2D", 1, 2, 1);

  auto loadInst = createChildAlgorithm("LoadInstrument", 0, 0.5);
  loadInst->setPropertyValue("Filename", filename);
  loadInst->setPropertyValue("InstrumentName", instrumentname);
  loadInst->setProperty("RewriteSpectraMap", OptionalBool(true));
  loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", ws);

  loadInst->execute();

  return ws;
}

// Call LoadIDFFromNexus as a child algorithm
API::MatrixWorkspace_sptr LoadEmptyInstrument::runLoadIDFFromNexus(const std::string &filename) {
  const std::string instrumentEntryName{"/instrument/instrument_xml"};
  // There are two possibilities for the parent of the instrument IDF entry
  const std::string instrumentParentEntryName_1{"mantid_workspace_1"};
  const std::string instrumentParentEntryName_2{"raw_data_1"};
  std::string instrumentParentEntryName{instrumentParentEntryName_1};

  // Test if instrument XML definition exists in the file
  bool foundIDF{false};
  try {
    ::NeXus::File nxsfile(filename);
    nxsfile.openPath(instrumentParentEntryName + instrumentEntryName);
    foundIDF = true;
  } catch (::NeXus::Exception &) {
  }

  if (!foundIDF) {
    instrumentParentEntryName = instrumentParentEntryName_2;
    try {
      ::NeXus::File nxsfile(filename);
      nxsfile.openPath(instrumentParentEntryName + instrumentEntryName);
      foundIDF = true;
    } catch (::NeXus::Exception &) {
    }
  }

  if (!foundIDF) {
    throw std::runtime_error("No instrument XML definition found in " + filename + " at " +
                             instrumentParentEntryName_1 + instrumentEntryName + " or at " +
                             instrumentParentEntryName_2 + instrumentEntryName);
  }

  auto loadInst = createChildAlgorithm("LoadIDFFromNexus");

  // Execute the child algorithm. Catch and log any error.
  auto ws = WorkspaceFactory::Instance().create("Workspace2D", 1, 2, 1);
  try {
    loadInst->setPropertyValue("Filename", filename);
    loadInst->setProperty<Mantid::API::MatrixWorkspace_sptr>("Workspace", ws);
    loadInst->setPropertyValue("InstrumentParentPath", instrumentParentEntryName);
    loadInst->execute();
  } catch (std::invalid_argument &) {
    getLogger().error("Invalid argument to LoadIDFFromNexus Child Algorithm ");
  } catch (std::runtime_error &) {
    getLogger().debug("No instrument definition found by LoadIDFFromNexus in " + filename + " at " +
                      instrumentParentEntryName + instrumentEntryName);
  }

  if (!loadInst->isExecuted())
    getLogger().information("No IDF loaded from the Nexus file " + filename);

  return ws;
}
} // namespace Mantid::DataHandling
