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
#include "MantidDataHandling/LoadHelper.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Points.h"
#include "MantidKernel/BoundedValidator.h"

#include <iterator>
#include <sstream>

namespace Mantid::DataHandling {

DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLSALSA)

const size_t LoadILLSALSA::VERTICAL_NUMBER_PIXELS = 256;
const size_t LoadILLSALSA::HORIZONTAL_NUMBER_PIXELS = 256;

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

  enum FileType { NONE, V1, V2 };

  FileType fileType = NONE;
  // guess type of file
  if (!h5file.nameExists("entry0"))
    throw std::runtime_error(
        "The Nexus file your are trying to open is incorrectly formatted, 'entry0' group does not exist");
  H5::Group entryGroup = h5file.openGroup("entry0");
  if (entryGroup.nameExists("data"))
    fileType = V1;
  else if (entryGroup.nameExists("data_scan"))
    fileType = V2;
  entryGroup.close();

  switch (fileType) {
  case NONE:
    throw std::runtime_error("The Nexus file your are trying to open is not supported by the SALSA loader.");
    break;
  case V1:
    loadNexusV1(h5file);

    break;
  case V2:
    loadNexusV2(h5file);
    break;
  }

  // set the instrument
  double sampleToDetectorDistance = getProperty("DetectorDistance");
  H5::DataSet thetaDataset = h5file.openDataSet("entry0/instrument/2theta/value");
  float theta;
  thetaDataset.read(&theta, thetaDataset.getDataType());
  double twoThetaAngle = theta + static_cast<double>(getProperty("ThetaOffset"));
  setInstrument(sampleToDetectorDistance, twoThetaAngle);
  thetaDataset.close();

  h5file.close();

  fillWorkspaceMetadata(filename);
}

/**
 * Load the instrument and set its position.
 * @param distance Sample to detector distance (in meters)
 * @param angle Beam to detector angle (2 theta, in degrees)
 */
void LoadILLSALSA::setInstrument(double distance, double angle) {
  // load instrument
  LoadHelper::loadEmptyInstrument(m_outputWorkspace, "SALSA");

  // rotation due to the IDF (channels are created along Y)
  auto rotateInst = createChildAlgorithm("RotateInstrumentComponent");
  rotateInst->setProperty<API::MatrixWorkspace_sptr>("Workspace", m_outputWorkspace);
  rotateInst->setPropertyValue("ComponentName", "detector");
  rotateInst->setPropertyValue("Z", "1");
  rotateInst->setProperty<double>("Angle", 90);
  rotateInst->execute();

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
  rotateInst = createChildAlgorithm("RotateInstrumentComponent");
  rotateInst->setProperty<API::MatrixWorkspace_sptr>("Workspace", m_outputWorkspace);
  rotateInst->setPropertyValue("ComponentName", "detector");
  rotateInst->setPropertyValue("X", "1"); // Y->X with the first rotation
  rotateInst->setProperty<double>("Angle", -angle);
  rotateInst->execute();
}

/**
 * Load V1 Nexus file. In this case, data are in /entry0/data/Multi_data and there shape is 256x256x1.
 * @param h5file reference to the opened hdf5/nexus file
 */
void LoadILLSALSA::loadNexusV1(const H5::H5File &h5file) {
  H5::DataSet detectorDataset = h5file.openDataSet("entry0/data/Multi_data");
  H5::DataSet monitorDataset = h5file.openDataSet("entry0/monitor/data");

  m_outputWorkspace = DataObjects::create<DataObjects::Workspace2D>(
      VERTICAL_NUMBER_PIXELS * HORIZONTAL_NUMBER_PIXELS + 1, HistogramData::Points(1));
  setProperty("OutputWorkspace", m_outputWorkspace);

  std::vector<int> dataInt(VERTICAL_NUMBER_PIXELS * HORIZONTAL_NUMBER_PIXELS + 1);
  detectorDataset.read(dataInt.data(), detectorDataset.getDataType());
  monitorDataset.read(dataInt.data() + VERTICAL_NUMBER_PIXELS * HORIZONTAL_NUMBER_PIXELS, monitorDataset.getDataType());

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
 * Load V2 Nexus file. In this case, data are in entry0/data_scan/detector_data/data and there shape is nx256x256 (with
 * n the number of scans).
 * @param h5file reference to the opened hdf5/nexus file
 */
void LoadILLSALSA::loadNexusV2(const H5::H5File &h5file) {
  H5::DataSet detectorDataset = h5file.openDataSet("entry0/data_scan/detector_data/data");
  H5::DataSpace detectorDataspace = detectorDataset.getSpace();

  int nDims = detectorDataspace.getSimpleExtentNdims();
  std::vector<hsize_t> dimsSize(nDims);
  detectorDataspace.getSimpleExtentDims(dimsSize.data(), NULL);

  size_t numberOfScans = dimsSize[0];

  if ((dimsSize[1] != VERTICAL_NUMBER_PIXELS) || (dimsSize[2] != HORIZONTAL_NUMBER_PIXELS)) {
    std::ostringstream oss;
    oss << "Unexpected data shape, got " << dimsSize[1] << "x" << dimsSize[2] << "pixels ";
    oss << "instead of " << VERTICAL_NUMBER_PIXELS << "x" << HORIZONTAL_NUMBER_PIXELS;
    throw std::runtime_error(oss.str());
  }

  m_outputWorkspace = DataObjects::create<DataObjects::Workspace2D>(
      VERTICAL_NUMBER_PIXELS * HORIZONTAL_NUMBER_PIXELS + 1, HistogramData::Points(numberOfScans));
  setProperty("OutputWorkspace", m_outputWorkspace);

  std::vector<int> dataInt(numberOfScans * VERTICAL_NUMBER_PIXELS * HORIZONTAL_NUMBER_PIXELS);
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
  if ((nDims != 2) || (dimsSize[1] != numberOfScans))
    throw std::runtime_error("Scanned variables are not formatted properly. Check you nexus file.");

  std::vector<double> scanVarData(dimsSize[0] * dimsSize[1]);
  scanVar.read(scanVarData.data(), scanVar.getDataType());
  std::vector<double> monitorData(dimsSize[1]);
  for (size_t i = 0; i < monitorData.size(); i++)
    monitorData[i] = scanVarData[monitorIndex * dimsSize[1] + i];

  scanVar.close();

  // fill the workspace
  for (size_t j = 0; j < numberOfScans; j++) {
    for (size_t i = 0; i < VERTICAL_NUMBER_PIXELS * HORIZONTAL_NUMBER_PIXELS; i++) {
      double count = dataInt[j * VERTICAL_NUMBER_PIXELS * HORIZONTAL_NUMBER_PIXELS + i];
      double error = sqrt(count);
      m_outputWorkspace->mutableY(i)[j] = count;
      m_outputWorkspace->mutableE(i)[j] = error;
    }
    m_outputWorkspace->mutableY(VERTICAL_NUMBER_PIXELS * HORIZONTAL_NUMBER_PIXELS)[j] = monitorData[j];
  }
}

void LoadILLSALSA::fillWorkspaceMetadata(const std::string &filename) {
  API::Run &runDetails = m_outputWorkspace->mutableRun();

  ::NeXus::File nxHandle(filename, NXACC_READ);
  LoadHelper::addNexusFieldsToWsRun(nxHandle, runDetails);
}
} // namespace Mantid::DataHandling
