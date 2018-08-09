#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidGeometry/Instrument.h"
#include "MantidDataHandling/LoadEmptyNexusGeometry.h"
#include "MantidNexusGeometry/NexusGeometryParser.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidTypes/SpectrumDefinition.h"
#include <algorithm>

namespace Mantid {
namespace DataHandling {

using namespace API;
using namespace Kernel;
using namespace DataObjects;
using namespace HistogramData;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadEmptyNexusGeometry)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadEmptyNexusGeometry::name() const {
  return "LoadEmptyNexusGeometry";
}

/// Algorithm's version for identification. @see Algorithm::version
int LoadEmptyNexusGeometry::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadEmptyNexusGeometry::category() const {
  return "DataHandling\\Nexus";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadEmptyNexusGeometry::summary() const {
  return "Loads an instrument from OFF nexus geometry file into an empty "
         "workspace. Much like LoadEmptyInstrument.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadEmptyNexusGeometry::init() {
  const std::vector<std::string> extensions{".nxs", ".hdf5"};
  declareProperty(Kernel::make_unique<FileProperty>(
                      "Filename", "", FileProperty::Load, extensions),
                  "The name of the Nexus file to read geometry from, as a full "
                  "or relative path.");

  declareProperty(Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An empty output workspace with an instrument attached.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadEmptyNexusGeometry::exec() {
  std::string fileName = getProperty("Filename");
  Progress prog(this, 0.0, 1.0, 10);

  prog.reportIncrement(0, "Loading geometry from file");
  auto instrument =
      NexusGeometry::NexusGeometryParser::createInstrument(fileName);

  const size_t number_spectra = instrument->getNumberDetectors();

  // Check that we have some spectra for the workspace
  if (number_spectra == 0) {
    g_log.error(
        "Instrument has no detectors, unable to create workspace for it");
    throw Kernel::Exception::InstrumentDefinitionError(
        "No detectors found in instrument");
  }

  prog.reportIncrement(3, "Building in-memory cache");
  std::vector<Mantid::Indexing::SpectrumNumber> spectrumNumbers(number_spectra);
  std::iota(spectrumNumbers.begin(), spectrumNumbers.end(),
            0 /*starting spectrum number*/);
  Indexing::IndexInfo indexInfo(spectrumNumbers);

  prog.reportIncrement(7, "Building spectrum definitions");
  size_t detIndex = 0;
  std::vector<SpectrumDefinition> spectrumDefinitions(number_spectra);
  std::generate(spectrumDefinitions.begin(), spectrumDefinitions.end(),
                [&]() { return SpectrumDefinition(detIndex++); });
  indexInfo.setSpectrumDefinitions(spectrumDefinitions);

  prog.reportIncrement(9, "Creating empty workspace.");
  auto workspace = create<Workspace2D>(
      std::move(instrument), indexInfo,
      Histogram(BinEdges{0.0, 1.0}, Counts{}, CountStandardDeviations{}));

  setProperty("OutputWorkspace", std::move(workspace));
}

} // namespace DataHandling
} // namespace Mantid
