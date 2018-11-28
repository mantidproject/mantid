// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadGeometry.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidNexusGeometry/NexusGeometryParser.h"

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory as a file loading algorithm
DECLARE_FILELOADER_ALGORITHM(LoadEmptyInstrument)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;
using namespace HistogramData;

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
  }   // Ascii file
  return confidence;
}

/// Initialisation method.
void LoadEmptyInstrument::init() {
  declareProperty(
      make_unique<FileProperty>("Filename", "", FileProperty::OptionalLoad,
                                LoadGeometry::validExtensions()),
      "The filename (including its full or relative path) of an instrument "
      "definition file. The file extension must either be .xml or .XML when "
      "specifying an instrument definition file. Files can also be .hdf5 or "
      ".nxs for usage with NeXus Geometry files. Note Filename or "
      "InstrumentName must be specified but not both.");
  declareProperty("InstrumentName", "",
                  "Name of instrument. Can be used instead of Filename to "
                  "specify an IDF");
  declareProperty(
      make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "",
                                                      Direction::Output),
      "The name of the workspace in which to store the imported instrument");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty(
      "DetectorValue", 1.0, mustBePositive,
      "This value affects the colour of the detectors in the instrument\n"
      "display window (default 1)");
  declareProperty(
      "MonitorValue", 2.0, mustBePositive,
      "This value affects the colour of the monitors in the instrument\n"
      "display window (default 2)");

  declareProperty(
      make_unique<PropertyWithValue<bool>>("MakeEventWorkspace", false),
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
  // load the instrument into this workspace
  const std::string filename = getPropertyValue("Filename");
  const std::string instrumentname = getPropertyValue("InstrumentName");
  Instrument_const_sptr instrument;
  Progress prog(this, 0.0, 1.0, 10);

  // Call LoadIstrument as a child algorithm
  MatrixWorkspace_sptr ws = this->runLoadInstrument(filename, instrumentname);
  instrument = ws->getInstrument();

  // Get number of detectors stored in instrument
  const size_t number_spectra = instrument->getNumberDetectors();

  // Check that we have some spectra for the workspace
  if (number_spectra == 0) {
    g_log.error(
        "Instrument has no detectors, unable to create workspace for it");
    throw Kernel::Exception::InstrumentDefinitionError(
        "No detectors found in instrument");
  }

  Indexing::IndexInfo indexInfo(number_spectra);
  bool MakeEventWorkspace = getProperty("MakeEventWorkspace");
  prog.reportIncrement(5, "Creating Data");
  if (MakeEventWorkspace) {
    setProperty("OutputWorkspace",
                create<EventWorkspace>(
                    std::move(instrument), indexInfo,
                    BinEdges{0.0, std::numeric_limits<double>::min()}));
  } else {
    const double detector_value = getProperty("DetectorValue");
    const double monitor_value = getProperty("MonitorValue");
    auto ws2D = create<Workspace2D>(
        std::move(instrument), indexInfo,
        Histogram(BinEdges{0.0, 1.0}, Counts(1, detector_value),
                  CountStandardDeviations(1, detector_value)));

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

/// Run the Child Algorithm LoadInstrument (or LoadInstrumentFromRaw)
API::MatrixWorkspace_sptr
LoadEmptyInstrument::runLoadInstrument(const std::string &filename,
                                       const std::string &instrumentname) {
  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument", 0, 0.5);
  loadInst->setPropertyValue("Filename", filename);
  loadInst->setPropertyValue("InstrumentName", instrumentname);
  loadInst->setProperty("RewriteSpectraMap", OptionalBool(true));
  auto ws = WorkspaceFactory::Instance().create("Workspace2D", 1, 2, 1);
  loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", ws);

  loadInst->execute();

  return ws;
}

} // namespace DataHandling
} // namespace Mantid
