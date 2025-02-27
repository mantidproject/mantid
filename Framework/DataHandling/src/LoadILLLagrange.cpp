// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadILLLagrange.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/UnitLabelTypes.h"
#include "MantidNexus/H5Util.h"
#include "MantidNexus/NeXusException.hpp"
#include "MantidNexus/NeXusFile.hpp"

#include <Poco/Path.h>

namespace Mantid::DataHandling {

using namespace API;
using namespace Geometry;
using namespace Kernel;
using namespace NeXus;
using Types::Core::DateAndTime;

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLLagrange)

/// Returns confidence. @see IFileLoader::confidence
int LoadILLLagrange::confidence(NexusDescriptor &descriptor) const {

  // fields existent only at the ILL Diffraction
  if (descriptor.isEntry("/entry0/IN1")) {
    return 80;
  } else {
    return 0;
  }
}

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadILLLagrange::name() const { return "LoadILLLagrange"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadILLLagrange::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadILLLagrange::category() const { return "DataHandling\\Nexus;ILL\\Lagrange"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadILLLagrange::summary() const { return "Loads ILL Lagrange instrument nexus files."; }

/**
 * Constructor
 */
LoadILLLagrange::LoadILLLagrange() : IFileLoader<NexusDescriptor>() {}

/**
 * Initialize the algorithm's properties.
 */
void LoadILLLagrange::init() {
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
                  "File path of the data file to load");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The output workspace.");
  declareProperty("InitialEnergyOffset", 0.0, "Offset for the initial energy (meV)");
}

/**
 * Executes the algorithm.
 */
void LoadILLLagrange::exec() {

  Progress progress(this, 0, 1, 2);

  progress.report("Loading the detector data");
  loadData();

  progress.report("Loading the metadata");
  loadMetaData();

  setProperty("OutputWorkspace", m_outputWorkspace);
}

/**
 *  Sets up the workspace, loads the mockup instrument,
 *  the data and scanned variables for proper data labelling.
 */
void LoadILLLagrange::loadData() {

  // open the H5 file
  H5::H5File h5file(getPropertyValue("Filename"), H5F_ACC_RDONLY, NeXus::H5Util::defaultFileAcc());

  H5::DataSet dataset = h5file.openDataSet("entry0/data_scan/detector_data/data");

  // init the workspace with proper number of histograms and number of channels
  initWorkspace(dataset);

  // load the instrument
  LoadHelper::loadEmptyInstrument(m_outputWorkspace, "Lagrange");

  // load data from file
  std::vector<int> dataInt(m_nScans);
  dataset.read(dataInt.data(), dataset.getDataType());
  dataset.close();

  // get scanned variable names
  H5::DataSet scanVarNames = h5file.openDataSet("entry0/data_scan/scanned_variables/variables_names/name");
  H5::DataSpace scanVarNamesSpace = scanVarNames.getSpace();

  int nDims = scanVarNamesSpace.getSimpleExtentNdims();
  auto dimsSize = std::vector<hsize_t>(nDims);
  scanVarNamesSpace.getSimpleExtentDims(dimsSize.data(), nullptr);

  std::vector<char *> rdata(dimsSize[0]);
  scanVarNames.read(rdata.data(), scanVarNames.getDataType());
  size_t monitorIndex = 0;
  while (monitorIndex < rdata.size()) {
    if (std::string(rdata[monitorIndex]) == "Monitor1")
      break;
    monitorIndex++;
  }
  if (monitorIndex == rdata.size())
    throw std::runtime_error("Monitor count not found. Please check your nexus file.");
  scanVarNames.vlenReclaim(rdata.data(), scanVarNames.getDataType(), scanVarNamesSpace);
  scanVarNames.close();

  // get scanned variable values and extract monitor count
  H5::DataSet scanVar = h5file.openDataSet("entry0/data_scan/scanned_variables/data");
  H5::DataSpace scanVarSpace = scanVar.getSpace();

  nDims = scanVarSpace.getSimpleExtentNdims();
  dimsSize = std::vector<hsize_t>(nDims);
  scanVarSpace.getSimpleExtentDims(dimsSize.data(), nullptr);
  if ((nDims != 2) || (dimsSize[1] != m_nScans))
    throw std::runtime_error("Scanned variables are not formatted properly. Check you nexus file.");

  std::vector<double> scanVarData(dimsSize[0] * dimsSize[1]);
  scanVar.read(scanVarData.data(), scanVar.getDataType());
  std::vector<double> monitorData(dimsSize[1]);
  std::vector<double> scanVariableData(dimsSize[1]);
  for (size_t i = 0; i < monitorData.size(); i++) {
    monitorData[i] = scanVarData[monitorIndex * dimsSize[1] + i];
    scanVariableData[i] = scanVarData[i];
  }
  scanVar.close();

  double energyAxisOffset = getProperty("InitialEnergyOffset");
  // fill the workspace
  for (size_t j = 0; j < m_nScans; j++) {
    double count = dataInt[j];
    double error = sqrt(count);
    m_outputWorkspace->mutableX(0)[j] = scanVariableData[j] - energyAxisOffset;
    m_outputWorkspace->mutableY(0)[j] = count;
    m_outputWorkspace->mutableE(0)[j] = error;
    m_outputWorkspace->mutableX(1)[j] = scanVariableData[j] - energyAxisOffset;
    m_outputWorkspace->mutableY(1)[j] = monitorData[j];
    m_outputWorkspace->mutableE(1)[j] = sqrt(monitorData[j]);
  }
  h5file.close();
}

/**
 * Dumps the metadata from the file for each entry separately
 */
void LoadILLLagrange::loadMetaData() {

  // Open NeXus file
  try {
    ::NeXus::File nxHandle(getPropertyValue("Filename"), NXACC_READ);
    LoadHelper::addNexusFieldsToWsRun(nxHandle, m_outputWorkspace->mutableRun(), "entry0");
  } catch (const ::NeXus::Exception &e) {
    g_log.debug() << "Failed to open nexus file \"" << getPropertyValue("Filename") << "\" in read mode: " << e.what()
                  << "\n";
  }

  // Add scanned variable: energy to the sample logs so it can be used for merging workspaces as X axis
  TimeSeriesProperty<double> *prop = new TimeSeriesProperty<double>("Ei");
  int index = 0;
  for (auto energy : m_outputWorkspace->readX(0)) {
    prop->addValue(index++, energy);
  }
  m_outputWorkspace->mutableRun().addProperty(prop);
}

/**
 * Initializes the output workspace for LAGRANGE
 * @param dataset : dataset to be loaded into the returned workspace
 */
void LoadILLLagrange::initWorkspace(const H5::DataSet &dataset) {
  // The number of spectra is always 1 + 1 monitor, for consistency with ASCII data loader
  const size_t nSpectra = 2;

  // Set number of scans
  H5::DataSpace detectorDataspace = dataset.getSpace();
  int nDims = detectorDataspace.getSimpleExtentNdims();
  std::vector<hsize_t> dimsSize(nDims);
  detectorDataspace.getSimpleExtentDims(dimsSize.data(), NULL);
  m_nScans = dimsSize[0];

  m_outputWorkspace = WorkspaceFactory::Instance().create("Workspace2D", nSpectra, m_nScans, m_nScans);

  // Set x axis units
  m_outputWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create("Energy");
  // Set y axis unit
  m_outputWorkspace->setYUnit("Counts");
}

} // namespace Mantid::DataHandling
