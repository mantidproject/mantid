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
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Points.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidNexus/NexusClasses.h"

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
}

/**
 * Executes the algorithm
 */
void LoadILLSALSA::exec() {
  const std::string filename = getPropertyValue("Filename");

  Mantid::NeXus::NXRoot dataRoot(filename);
  Mantid::NeXus::NXEntry dataFirstEntry = dataRoot.openFirstEntry();

  Mantid::NeXus::NXData monitorGroup = dataFirstEntry.openNXData("/monitor/data");
  Mantid::NeXus::NXInt monitor = monitorGroup.openIntData();
  monitor.load();

  Mantid::NeXus::NXData dataGroup = dataFirstEntry.openNXData("/data_scan/detector_data/data");
  Mantid::NeXus::NXInt data = dataGroup.openIntData();
  data.load();

  m_numberOfScans = data.dims(0);
  m_numberOfRows = data.dims(1);
  m_numberOfColumns = data.dims(2);

  m_outputWorkspace = DataObjects::create<DataObjects::Workspace2D>(m_numberOfRows * m_numberOfColumns + 1,
                                                                    HistogramData::Points(m_numberOfScans));

  auto loadInst = createChildAlgorithm("LoadInstrument");
  loadInst->setPropertyValue("InstrumentName", "SALSA");
  loadInst->setProperty<API::MatrixWorkspace_sptr>("Workspace", m_outputWorkspace);
  loadInst->setProperty("RewriteSpectraMap", Kernel::OptionalBool(true));
  loadInst->execute();
  setProperty("OutputWorkspace", m_outputWorkspace);

  int index = 0;
  for (int i = 0; i < m_numberOfRows; i++) {
    for (int j = 0; j < m_numberOfColumns; j++) {
      auto &spectrum = m_outputWorkspace->mutableY(index);
      auto &errors = m_outputWorkspace->mutableE(index);
      auto &axis = m_outputWorkspace->mutableX(index);
      for (int k = 0; k < m_numberOfScans; k++) {
        spectrum[k] = data(k, i, j);
        errors[k] = sqrt(data(k, i, j));
        axis[k] = k;
      }
      index++;
    }
  }
  for (int i = 0; i < m_numberOfScans; i++) {
    m_outputWorkspace->mutableY(index) = monitor(0, 0, 0);
    m_outputWorkspace->mutableE(index) = sqrt(monitor(0, 0, 0));
    m_outputWorkspace->mutableX(index) = i;
  }
}
} // namespace Mantid::DataHandling
