#include "MantidDataHandling/LoadILLDiffraction.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <nexus/napi.h>

namespace Mantid {
namespace DataHandling {

using namespace API;
using namespace Kernel;
using namespace NeXus;

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLDiffraction)

/// Returns confidence. @see IFileLoader::confidence
int LoadILLDiffraction::confidence(NexusDescriptor &descriptor) const {

  // fields existent only at the ILL Diffraction
  if (descriptor.pathExists("/entry0/data_scan")) {
    return 80;
  } else {
    return 0;
  }
}

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadILLDiffraction::name() const {
  return "LoadILLDiffraction";
}

/// Algorithm's version for identification. @see Algorithm::version
int LoadILLDiffraction::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadILLDiffraction::category() const {
  return "DataHandling\\Nexus";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadILLDiffraction::summary() const {
  return "Loads ILL diffraction nexus files.";
}

/**
 * Constructor
 */
LoadILLDiffraction::LoadILLDiffraction()
    : IFileLoader<NexusDescriptor>(), m_instNames({"D1B", "D2B", "D4C", "D20"}),
      m_isDetectorScan(false) {}

/**
 * Initialize the algorithm's properties.
 */
void LoadILLDiffraction::init() {
  declareProperty(
      make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
      "File path of the Data file to load");
  declareProperty(make_unique<WorkspaceProperty<Workspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

/**
 * Execute the algorithm.
 */
void LoadILLDiffraction::exec() {

  m_progress = make_unique<Progress>(this, 0, 1, 2);

  m_fileName = getPropertyValue("Filename");

  loadDataScan();
  m_progress->report("Loaded data scan");

  loadMetadata();
  m_progress->report("Loaded the metadata");

  // set the output
  setProperty("OutputWorkspace", m_outWorkspace);
}

/**
* Load the data scan
*/
void LoadILLDiffraction::loadDataScan() {

  // open the root node
  NXRoot dataRoot(m_fileName);
  NXEntry firstEntry = dataRoot.openFirstEntry();

  m_numberScanPoints = firstEntry.getInt("data_scan/total_steps");

  // read in the actual data
  NXData dataGroup = firstEntry.openNXData("data_scan/detector_data");
  NXUInt data = dataGroup.openUIntData();
  data.load();

  m_numberDetectorsRead = data.dim1();

  resolveInstrument(firstEntry.getString("instrument/name"));

  // read the scanned variables
  NXInt scannedVar = firstEntry.openNXInt(
      "data_scan/scanned_variables/variables_names/scanned");
  scannedVar.load();

  // resolveScanType();

  // read the scan points
  NXData scanValues = firstEntry.openNXData("data_scan/scanned_variables");
  NXDouble scanVal = scanValues.openDoubleData();
  scanVal.load();

  initWorkspace();

  std::vector<int> scannedVarIndices;
  std::vector<double> xAxis;
  std::vector<double> monitor;

  for (int i = 0; i < scannedVar.dim0(); ++i) {
    if (scannedVar[i] == 1) {
      scannedVarIndices.emplace_back(i);
    }
  }

  if (!scannedVarIndices.empty()) {
    xAxis.assign(scanVal() + m_numberScanPoints * scannedVarIndices[0],
                 scanVal() + m_numberScanPoints * (scannedVarIndices[0] + 1));
    monitor.assign(scanVal() + 3 * m_numberScanPoints,
                   scanVal() + 4 * m_numberScanPoints);
  }

  // Assign monitor counts
  m_outWorkspace->mutableX(0) = xAxis;
  m_outWorkspace->mutableY(0) = monitor;
  std::transform(monitor.begin(), monitor.end(),
                 m_outWorkspace->mutableE(0).begin(), sqrt);

  // Assign detector counts
  for (int i = 1; i <= data.dim1(); ++i) {
    for (int j = 0; j < data.dim0(); ++j) {
      unsigned int y = data(j, i - 1);
      m_outWorkspace->mutableY(i)[j] = y;
      m_outWorkspace->mutableE(i)[j] = sqrt(y);
    }
    m_outWorkspace->mutableX(i) = xAxis;
  }

  dataRoot.close();
  loadStaticInstrument();
}

/**
 * Resolves the instrument based on instrument name and resolution mode
 * @param inst Instrument name read from the file
 * @throws runtime_error, if the instrument name is not supported
 */
void LoadILLDiffraction::resolveInstrument(const std::string &inst) {
  if (m_instNames.find(inst) == m_instNames.end()) {
    throw std::runtime_error("Instrument " + inst + " not supported.");
  } else {
    if (inst == "D20") {
      switch (m_numberDetectorsRead) {
      case 1600: {
        m_instName = "D20a";
        m_numberDetectorsActual = 1536;
        break;
      }
      case 3200: {
        m_instName = "D20";
        m_numberDetectorsActual = 3200; // 3072
        break;
      }
      case 4800: {
        m_instName = "D20c";
        m_numberDetectorsActual = 4608;
        break;
      }
      default:
        throw std::runtime_error("Unknown resolution mode for instrument " +
                                 inst);
      }
    } else {
      m_instName = inst;
      m_numberDetectorsActual = m_numberDetectorsRead;
    }
  }
}

/**
 * Initializes the output workspace based on the resolved instrument, scan
 * points, and scan type
 */
void LoadILLDiffraction::initWorkspace() {
  // create empty workspace
  int nSpectra = 0, nBins = 0;

  if (m_numberScanPoints == 0) {
    nSpectra = m_numberDetectorsActual + 1;
    nBins = 1;
  } else {
    if (!m_isDetectorScan) {
      nSpectra = m_numberDetectorsActual + 1;
      nBins = m_numberScanPoints;
    } else {
      nSpectra = m_numberDetectorsActual * m_numberScanPoints + 1;
      nBins = 1;
    }
  }
  m_outWorkspace = WorkspaceFactory::Instance().create("Workspace2D", nSpectra,
                                                       nBins, nBins);
}

/**
* Loads the metadata. Segfaults on recursion!
*/
void LoadILLDiffraction::loadMetadata() {

  Run &run = m_outWorkspace->mutableRun();

  run.addProperty("Facility", std::string("ILL"));

  // Open NeXus file
  NXhandle nxHandle;
  NXstatus nxStat = NXopen(m_fileName.c_str(), NXACC_READ, &nxHandle);

  if (nxStat == NX_ERROR) {
    throw Exception::FileError("Unable to open nexus file for metadata:",
                               m_fileName);
  }
  m_loadHelper.addNexusFieldsToWsRun(nxHandle, run);

  nxStat = NXclose(&nxHandle);
}

/**
* Runs LoadInstrument as child to link the non-moving instrument to workspace
*/
void LoadILLDiffraction::loadStaticInstrument() {
  IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");
  loadInst->setPropertyValue("InstrumentName", m_instName);
  loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", m_outWorkspace);
  loadInst->setProperty("RewriteSpectraMap", OptionalBool(true));
  loadInst->execute();
}

} // namespace DataHandling
} // namespace Mantid
