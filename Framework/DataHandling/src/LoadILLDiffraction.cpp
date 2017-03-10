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
  return "DataHandling\\Nexus;ILL\\Diffraction";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadILLDiffraction::summary() const {
  return "Loads ILL diffraction nexus files.";
}

/**
 * Constructor
 */
LoadILLDiffraction::LoadILLDiffraction()
    : IFileLoader<NexusDescriptor>(),
      m_instNames({"D20"}) {}

/**
 * Initialize the algorithm's properties.
 */
void LoadILLDiffraction::init() {
  declareProperty(
      make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
      "File path of the data file to load");
  declareProperty(make_unique<WorkspaceProperty<Workspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The output workspace.");
}

/**
 * Execute the algorithm.
 */
void LoadILLDiffraction::exec() {

  Progress progress(this, 0, 1, 2);

  m_fileName = getPropertyValue("Filename");

  // open the root node
  NXRoot dataRoot(m_fileName);
  NXEntry firstEntry = dataRoot.openFirstEntry();

  loadDataScan(firstEntry);
  dataRoot.close();

  progress.report("Loaded the data scan block");

  loadMetadata();
  progress.report("Loaded the metadata");

  // set the output
  setProperty("OutputWorkspace", m_outWorkspace);
}

/**
* Load the data scan block
* @param firstEntry : First nexus entry opened
*/
void LoadILLDiffraction::loadDataScan(NXEntry &firstEntry) {

  // read the actual data
  NXData dataGroup = firstEntry.openNXData("data_scan/detector_data");
  NXUInt data = dataGroup.openUIntData();
  data.load();

  // read the scan points
  NXData scanValues = firstEntry.openNXData("data_scan/scanned_variables");
  NXDouble scanVal = scanValues.openDoubleData();
  scanVal.load();

  m_numberDetectorsRead = data.dim1();
  m_numberScanPoints = data.dim0();

  // figure out the IDF to load
  resolveInstrument(firstEntry.getString("instrument/name"));

  // read the scanned variables
  loadScannedVariables(firstEntry);

  // figure out the scan type
  resolveScanType();

  // Init the output workspace
  initWorkspace();

  if (m_scanType == DetectorScan) {
    fillMovingInstrumentScan(data, scanVal);
  } else {
    fillStaticInstrumentScan(data, scanVal);
  }
}

/**
 * Resolves the scan type
 */
void LoadILLDiffraction::resolveScanType() {
    if (m_numberScanPoints == 1) {
        m_scanType = NoScan;
    }
    else {
        // TODO check the detector scan
        m_scanType = OtherScan;
    }
}

/**
 * Loads the scanned_variables/variables_names block
 */
void LoadILLDiffraction::loadScannedVariables(NXEntry& firstEntry) {

    // read which variables are scanned
    NXInt scanned = firstEntry.openNXInt(
        "data_scan/scanned_variables/variables_names/scanned");
    scanned.load();

    // read what is going to be the axis axis
    NXInt axis = firstEntry.openNXInt(
        "data_scan/scanned_variables/variables_names/axis");
    axis.load();

}


/**
 * Fills the loaded data to the workspace for non-moving instrument
 * @param data : pointer to data
 * @param scan : pointer to scan points
 */
void LoadILLDiffraction::fillStaticInstrumentScan(const NXUInt &data,
                                                  const NXDouble &scan) {

  std::vector<double> xAxis{0.};
  std::vector<double> monitor{1.};

  /*
  if (!m_scannedVarIndices.empty()) {
    xAxis.assign(scan() + m_numberScanPoints * m_scannedVarIndices[0],
                 scan() + m_numberScanPoints * (m_scannedVarIndices[0] + 1));
    monitor.assign(scan() + 3 * m_numberScanPoints,
                   scan() + 4 * m_numberScanPoints);
  }
  */

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

  // Link the instrument
  // loadStaticInstrument();
}

/**
 * Resolves the instrument based on instrument name and resolution mode
 * @param inst Instrument name read from the file
 * @throws runtime_error, if the instrument is not supported
 */
void LoadILLDiffraction::resolveInstrument(const std::string &inst) {
  if (m_instNames.find(inst) == m_instNames.end()) {
    throw std::runtime_error("Instrument " + inst + " not supported.");
  } else {
    if (inst == "D20") {
      m_instName = inst;
      switch (m_numberDetectorsRead) {
      case 1600: {
        m_instName += "_lr"; // low resolution mode
        m_numberDetectorsActual = 1536;
        break;
      }
      case 3200: {
        m_numberDetectorsActual = 3200; // 3072
        break;
      }
      case 4800: {
        m_instName += "_hr"; // high resolution mode
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

  int nSpectra = m_numberDetectorsActual + 1, nBins = 1;

  if (m_scanType == DetectorScan) {
    nSpectra *= m_numberScanPoints;
  } else if (m_scanType == OtherScan) {
    nBins = m_numberScanPoints;
  }

  m_outWorkspace = WorkspaceFactory::Instance().create("Workspace2D", nSpectra,
                                                       nBins, nBins);
}

/**
* Loads the metadata to SampleLogs
*/
void LoadILLDiffraction::loadMetadata() {

  Run &run = m_outWorkspace->mutableRun();

  run.addProperty("Facility", std::string("ILL"));

  // Open NeXus file
  NXhandle nxHandle;
  NXstatus nxStat = NXopen(m_fileName.c_str(), NXACC_READ, &nxHandle);

  if (nxStat != NX_ERROR) {
    m_loadHelper.addNexusFieldsToWsRun(nxHandle, run);
    nxStat = NXclose(&nxHandle);
  }  

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
