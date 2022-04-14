// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadILLSALSA.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataHandling/H5Util.h"
#include "MantidDataHandling/LoadHelper.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Points.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidNexus/NexusClasses.h"

#include <iterator>
#include <sstream>

namespace Mantid::DataHandling {

DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLSALSA)

/**
 * Return the confidence with with this algorithm can load the file
 *
 * @param descriptor A descriptor for the file
 *
 * @return An integer specifying the confidence level. 0 indicates it will not be used
 */
int LoadILLSALSA::confidence(Kernel::NexusDescriptor &descriptor) const {
  if ((descriptor.pathExists("/entry0/data_scan") || descriptor.pathExists("/entry0/data")) &&
      descriptor.pathExists("/entry0/instrument/Tx") && descriptor.pathExists("/entry0/instrument/Ty") &&
      descriptor.pathExists("/entry0/instrument/Tz"))
    return 80;
  else
    return 0;
}

/**
 * Initialises the algorithm
 */
void LoadILLSALSA::init() {
  declareProperty(std::make_unique<API::FileProperty>("Filename", "", API::FileProperty::Load, ".nxs"),
                  "File path of the Data file to load");
  declareProperty(std::make_unique<API::WorkspaceProperty<>>("OutputWorkspace", "", Kernel::Direction::Output),
                  "The name to use for the output workspace");
  auto mustBePositive = std::make_shared<Kernel::BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("DetectorDistance", 1.0, mustBePositive, "Distance between the sample and the detector (meters)");
  declareProperty("ThetaOffset", 0.0, "Offset for the 2theta value (degrees)");
}

/**
 * Executes the algorithm
 */
void LoadILLSALSA::exec() {
  const std::string filename = getPropertyValue("Filename");
  H5::H5File h5file(filename, H5F_ACC_RDONLY);

  enum FileType { OLD, NEW, NONE };

  FileType fileType = NONE;
  // guess type of file
  try {
    H5::DataSet detectorDataset = h5file.openDataSet("entry0/data");
    detectorDataset.close();
  } catch (...) {
    fileType = OLD;
  }

  try {
    H5::DataSet detectorDataset = h5file.openDataSet("entry0/data_scan");
    detectorDataset.close();
  } catch (...) {
    fileType = NEW;
  }

  switch (fileType) {
  case NONE:
    throw std::runtime_error("pb");
    break;
  case OLD:
    loadOldNexus(h5file);
    break;
  case NEW:
    loadNewNexus(h5file);
    break;
  }

  h5file.close();
}

void LoadILLSALSA::setInstrument(double distance, double angle) {
  // load instrument
  auto loadInst = createChildAlgorithm("LoadInstrument");
  loadInst->setPropertyValue("InstrumentName", "SALSA");
  loadInst->setProperty<API::MatrixWorkspace_sptr>("Workspace", m_outputWorkspace);
  loadInst->setProperty("RewriteSpectraMap", Kernel::OptionalBool(true));
  loadInst->execute();

  // translation
  double angleRad = angle * M_PI / 180.0;
  double dx = -distance * sin(angleRad);
  double dz = distance * cos(angleRad);
  auto moveInst = createChildAlgorithm("MoveInstrumentComponent");
  moveInst->setProperty<API::MatrixWorkspace_sptr>("Workspace", m_outputWorkspace);
  moveInst->setPropertyValue("ComponentName", "detector");
  moveInst->setProperty<double>("X", dx);
  moveInst->setProperty<double>("Y", 0.);
  moveInst->setProperty<double>("Z", dz);
  moveInst->setProperty<bool>("RelativePosition", false);
  moveInst->execute();

  // rotation
  auto rotateInst = createChildAlgorithm("RotateInstrumentComponent");
  rotateInst->setProperty<API::MatrixWorkspace_sptr>("Workspace", m_outputWorkspace);
  rotateInst->setPropertyValue("ComponentName", "detector");
  rotateInst->setPropertyValue("Y", "1");
  rotateInst->setProperty<double>("Angle", -angle);
  rotateInst->execute();
}

/**
 * Load old Nexus files that contain a single point. In this case, data are in /entry0/data/Multi_data and there shape
 * is 256x256x1.
 * @param h5file reference to the opened hdf5/nexus file
 */
void LoadILLSALSA::loadOldNexus(const H5::H5File &h5file) {
  H5::DataSet detectorDataset = h5file.openDataSet("entry0/data/Multi_data");
  H5::DataSet monitorDataset = h5file.openDataSet("entry0/monitor/data");

  m_outputWorkspace = DataObjects::create<DataObjects::Workspace2D>(256 * 256 + 1, HistogramData::Points(1));
  setProperty("OutputWorkspace", m_outputWorkspace);

  std::vector<int> dataInt(256 * 256 + 1);
  detectorDataset.read(dataInt.data(), detectorDataset.getDataType());
  monitorDataset.read(dataInt.data() + 256 * 256, monitorDataset.getDataType());

  for (size_t i = 0; i < dataInt.size(); i++) {
    double count = dataInt[i];
    double error = sqrt(count);
    m_outputWorkspace->mutableY(i)[0] = count;
    m_outputWorkspace->mutableE(i)[0] = error;
  }

  detectorDataset.close();
  monitorDataset.close();
}

/**
 * Fill the output workspace with data coming from a scanning nexus.
 * @param h5file reference to the opened hdf5/nexus file
 */
void LoadILLSALSA::loadNewNexus(const H5::H5File &h5file) {
  H5::DataSet detectorDataset = h5file.openDataSet("entry0/data_scan/detector_data/data");
  H5::DataSpace detectorDataspace = detectorDataset.getSpace();

  int nDims = detectorDataspace.getSimpleExtentNdims();
  std::vector<hsize_t> dimsSize(nDims);
  detectorDataspace.getSimpleExtentDims(dimsSize.data(), NULL);

  m_numberOfScans = dimsSize[0];
  m_numberOfRows = dimsSize[1];
  m_numberOfColumns = dimsSize[2];

  m_outputWorkspace = DataObjects::create<DataObjects::Workspace2D>(m_numberOfRows * m_numberOfColumns + 1,
                                                                    HistogramData::Points(m_numberOfScans));
  setProperty("OutputWorkspace", m_outputWorkspace);

  std::vector<int> dataInt(m_numberOfScans * m_numberOfRows * m_numberOfColumns);
  detectorDataset.read(dataInt.data(), detectorDataset.getDataType());

  detectorDataset.close();

  // get scanned variable names
  H5::DataSet scanVarNames = h5file.openDataSet("entry0/data_scan/scanned_variables/variables_names/name");
  H5::DataSpace scanVarNamesSpace = scanVarNames.getSpace();

  nDims = scanVarNamesSpace.getSimpleExtentNdims();
  dimsSize = std::vector<hsize_t>(nDims);
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
  if ((nDims != 2) || (dimsSize[1] != m_numberOfScans))
    throw std::runtime_error("Scanned variables are not formatted properly. Check you nexus file.");

  std::vector<double> scanVarData(dimsSize[0] * dimsSize[1]);
  scanVar.read(scanVarData.data(), scanVar.getDataType());
  std::vector<double> monitorData(dimsSize[1]);
  for (size_t i = 0; i < monitorData.size(); i++)
    monitorData[i] = scanVarData[monitorIndex * dimsSize[1] + i];

  scanVar.close();

  // fill the workspace
  for (size_t j = 0; j < m_numberOfScans; j++) {
    for (size_t i = 0; i < m_numberOfRows * m_numberOfColumns; i++) {
      double count = dataInt[j * m_numberOfRows * m_numberOfColumns + i];
      double error = sqrt(count);
      m_outputWorkspace->mutableY(i)[j] = count;
      m_outputWorkspace->mutableE(i)[j] = error;
    }
    m_outputWorkspace->mutableY(m_numberOfRows * m_numberOfColumns)[j] = monitorData[j];
  }
}

void LoadILLSALSA::fillWorkspaceData(const Mantid::NeXus::NXInt &detectorData,
                                     const std::vector<std::string> &scanVariableNames,
                                     const Mantid::NeXus::NXDouble &scanVariables) {
  // fill detector data
  int index = 0;
  for (int i = 0; i < m_numberOfRows; i++) {
    for (int j = 0; j < m_numberOfColumns; j++) {
      auto &spectrum = m_outputWorkspace->mutableY(index);
      auto &errors = m_outputWorkspace->mutableE(index);
      auto &axis = m_outputWorkspace->mutableX(index);
      for (int k = 0; k < m_numberOfScans; k++) {
        spectrum[k] = detectorData(k, i, j);
        errors[k] = sqrt(detectorData(k, i, j));
        axis[k] = k;
      }
      index++;
    }
  }

  // fill monitor data
  auto it = std::find(scanVariableNames.cbegin(), scanVariableNames.cend(), "Monitor1");
  if (it == scanVariableNames.cend())
    throw std::runtime_error("Monitor was not found in scanned variable. Please check your nexus file.");
  auto monitorIndex = std::distance(scanVariableNames.cbegin(), it);
  for (int i = 0; i < m_numberOfScans; i++) {
    m_outputWorkspace->mutableY(index)[i] = scanVariables((int)monitorIndex, i);
    m_outputWorkspace->mutableE(index)[i] = sqrt(scanVariables((int)monitorIndex, i));
    m_outputWorkspace->mutableX(index)[i] = i;
  }
}

void LoadILLSALSA::fillWorkspaceMetadata(const std::string &filename) {
  API::Run &runDetails = m_outputWorkspace->mutableRun();
  NXhandle nxHandle;
  NXopen(filename.c_str(), NXACC_READ, &nxHandle);
  DataHandling::LoadHelper loadHelper;
  loadHelper.addNexusFieldsToWsRun(nxHandle, runDetails);
  NXclose(&nxHandle);
}
} // namespace Mantid::DataHandling
