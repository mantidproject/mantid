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
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Points.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidNexus/NexusClasses.h"

#include <H5Cpp.h>
#include <iterator>
#include <sstream>
#include <vector>

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
  if (descriptor.pathExists("/entry0/data_scan") && descriptor.pathExists("/entry0/instrument/Tx") &&
      descriptor.pathExists("/entry0/instrument/Ty") && descriptor.pathExists("/entry0/instrument/Tz"))
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
  declareProperty("DetectorDistance", 1.0, mustBePositive, "Distance between the sample and the detector");
}

/**
 * Executes the algorithm
 */
void LoadILLSALSA::exec() {
  const std::string filename = getPropertyValue("Filename");

  // load scanned variables names
  std::vector<std::string> scanVariableNames;
  {
    H5::H5File h5file(filename, H5F_ACC_RDONLY);
    H5::Group scanVariables = h5file.openGroup("entry0/data_scan/scanned_variables/variables_names");
    scanVariableNames = H5Util::readStringVector(scanVariables, "name");
    scanVariables.close();
    h5file.close();
  }

  // load data (detector and scanned variables)
  Mantid::NeXus::NXRoot dataRoot(filename);
  Mantid::NeXus::NXEntry dataFirstEntry = dataRoot.openFirstEntry();

  Mantid::NeXus::NXData scanVariablesGroup = dataFirstEntry.openNXData("/data_scan/scanned_variables/data");
  Mantid::NeXus::NXDouble scanVariables = scanVariablesGroup.openDoubleData();
  scanVariables.load();
  scanVariablesGroup.close();

  Mantid::NeXus::NXData dataGroup = dataFirstEntry.openNXData("/data_scan/detector_data/data");
  Mantid::NeXus::NXInt data = dataGroup.openIntData();
  data.load();
  dataGroup.close();

  int numberOfScans = data.dims(0);
  int numberOfRows = data.dims(1);
  int numberOfColumns = data.dims(2);

  if (scanVariables.dims(1) != numberOfScans) {
    std::ostringstream msg;
    msg << "Number of scans in detector data (" << numberOfScans << ") ";
    msg << "and scanned variables (" << scanVariables.dims(1) << ") do not match, please check your nexus file.";
    throw std::runtime_error(msg.str());
  }

  m_outputWorkspace = DataObjects::create<DataObjects::Workspace2D>(numberOfRows * numberOfColumns + 1,
                                                                    HistogramData::Points(numberOfScans));

  // create instrument
  auto loadInst = createChildAlgorithm("LoadInstrument");
  loadInst->setPropertyValue("InstrumentName", "SALSA");
  loadInst->setProperty<API::MatrixWorkspace_sptr>("Workspace", m_outputWorkspace);
  loadInst->setProperty("RewriteSpectraMap", Kernel::OptionalBool(true));
  loadInst->execute();
  setProperty("OutputWorkspace", m_outputWorkspace);

  // move detector
  Mantid::NeXus::NXFloat theta = dataFirstEntry.openNXFloat("/instrument/2theta/value");
  theta.load();
  double distance = getProperty("DetectorDistance");
  double thetaDeg = theta[0];
  double thetaRad = thetaDeg * M_PI / 180.0;
  double dx = -distance * sin(thetaRad);
  double dz = distance * cos(thetaRad);
  auto moveInst = createChildAlgorithm("MoveInstrumentComponent");
  moveInst->setProperty<API::MatrixWorkspace_sptr>("Workspace", m_outputWorkspace);
  moveInst->setPropertyValue("ComponentName", "detector");
  moveInst->setProperty<double>("X", dx);
  moveInst->setProperty<double>("Z", dz);
  moveInst->setProperty<bool>("RelativePosition", false);
  moveInst->execute();
  auto rotateInst = createChildAlgorithm("RotateInstrumentComponent");
  rotateInst->setProperty<API::MatrixWorkspace_sptr>("Workspace", m_outputWorkspace);
  rotateInst->setPropertyValue("ComponentName", "detector");
  rotateInst->setPropertyValue("Y", "1");
  rotateInst->setProperty<double>("Angle", -thetaDeg);
  rotateInst->execute();

  // fill detector data
  int index = 0;
  for (int i = 0; i < numberOfRows; i++) {
    for (int j = 0; j < numberOfColumns; j++) {
      auto &spectrum = m_outputWorkspace->mutableY(index);
      auto &errors = m_outputWorkspace->mutableE(index);
      auto &axis = m_outputWorkspace->mutableX(index);
      for (int k = 0; k < numberOfScans; k++) {
        spectrum[k] = data(k, i, j);
        errors[k] = sqrt(data(k, i, j));
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
  for (int i = 0; i < numberOfScans; i++) {
    m_outputWorkspace->mutableY(index)[i] = scanVariables((int)monitorIndex, i);
    m_outputWorkspace->mutableE(index)[i] = sqrt(scanVariables((int)monitorIndex, i));
    m_outputWorkspace->mutableX(index)[i] = i;
  }
}
} // namespace Mantid::DataHandling
