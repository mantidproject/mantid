// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/NXcanSASUtil.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/InstrumentFileFinder.h"
#include "MantidDataHandling/NXcanSASDefinitions.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidNexus/H5Util.h"

using namespace Mantid::API;
using namespace Mantid::NeXus;

namespace Mantid::DataHandling::NXcanSAS {

// Prepares size and shape vectors and variables for data to be stored in file
DataDimensions::DataDimensions(const MatrixWorkspace_sptr &workspace,
                               const std::optional<std::pair<size_t, size_t>> &spinVecSize)
    : DataDimensions(static_cast<hsize_t>(workspace->blocksize()),
                     static_cast<hsize_t>(workspace->getNumberHistograms()), spinVecSize) {}

DataDimensions::DataDimensions(const hsize_t numberOfPoints, const hsize_t numberOfHistograms,
                               const std::optional<std::pair<size_t, size_t>> &spinVecSize)
    : m_numberOfPoints(numberOfPoints), m_numberOfHistograms(numberOfHistograms),
      m_dataShape(std::vector<hsize_t>({m_numberOfPoints})), m_slabShape(std::vector<hsize_t>({m_numberOfPoints})) {
  if (m_numberOfHistograms > 1) {
    m_dataShape.insert(m_dataShape.cbegin(), m_numberOfHistograms);
    m_slabShape.insert(m_slabShape.cbegin(), 1);
  }
  // If data is polarized only
  if (spinVecSize.has_value()) {
    const auto &[PinSize, PoutSize] = *spinVecSize;
    m_dataShape.insert(m_dataShape.cbegin(), {PinSize, PoutSize});
    m_slabShape.insert(m_slabShape.cbegin(), {1, 1});
  }

  m_dataSpace = H5::DataSpace(static_cast<int>(m_dataShape.size()), m_dataShape.data());
  m_dataType = H5::DataType(H5Util::getType<double>());
}

const hsize_t &DataDimensions::getNumberOfPoints() const { return m_numberOfPoints; }
const hsize_t &DataDimensions::getNumberOfHistograms() const { return m_numberOfHistograms; }
const std::vector<hsize_t> &DataDimensions::getDataShape() const { return m_dataShape; }
const std::vector<hsize_t> &DataDimensions::getSlabShape() const { return m_slabShape; }
const H5::DataSpace &DataDimensions::getDataSpace() const { return m_dataSpace; }
const H5::DataType &DataDimensions::getDataType() const { return m_dataType; }

std::string InstrumentNameInfo::getInstrumentNameFromFile(const H5::Group &entry) {
  // Get instrument name
  const auto instrument = entry.openGroup(sasInstrumentGroupName);
  return H5Util::readString(instrument, sasInstrumentName);
}

std::string InstrumentNameInfo::getIdfFromFile(const std::string &instrumentName) {
  std::string idf;
  try {
    idf = InstrumentFileFinder::getInstrumentFilename(instrumentName);
  } catch (std::runtime_error &) {
    return "";
  }
  return idf;
}

InstrumentNameInfo::InstrumentNameInfo(const H5::Group &entry)
    : instrumentName(getInstrumentNameFromFile(entry)), idf(getIdfFromFile(instrumentName)) {}

SpinVectorBuilder::SpinVectorBuilder(const std::vector<std::string> &spinStateStr) : spinVec(spinStateStr) {
  // If there is polarized data, we set the default state vector as -1,1 and then arrange workspaces accordingly
  // when storing the polarized data set.
  const auto stateVector = std::vector<int>({-1, 1});
  if (spinStateStr.size() == 4) {
    pIn = stateVector;
    pOut = stateVector;
  } else if (spinStateStr.size() == 2) {
    if (spinStateStr.begin()->starts_with("0")) {
      pIn = std::vector<int>(1, 0);
      pOut = stateVector;
    } else {
      pIn = stateVector;
      pOut = std::vector<int>(1, 0);
    }
  }
}

DataSpaceInformation getDataSpaceInfo(const H5::DataSet &dataSet) {
  DataSpaceInformation dataSpaceInfo;
  const auto dataSpace = dataSet.getSpace();
  const auto rank = dataSpace.getSimpleExtentNdims();
  auto dims = std::vector<hsize_t>(rank);
  dataSpace.getSimpleExtentDims(dims.data());
  switch (rank) {
  case 1:
    dataSpaceInfo.dimSpectrumAxis = 1;
    dataSpaceInfo.dimBin = dims[0];
    break;
  case 2:
    dataSpaceInfo.dimSpectrumAxis = dims[0];
    dataSpaceInfo.dimBin = dims[1];
    break;
  case 3:
    dataSpaceInfo.spinStates = dims[0] * dims[1];
    dataSpaceInfo.dimSpectrumAxis = 1;
    dataSpaceInfo.dimBin = dims[2];
    break;
  case 4:
    dataSpaceInfo.spinStates = dims[0] * dims[1];
    dataSpaceInfo.dimSpectrumAxis = dims[2];
    dataSpaceInfo.dimBin = dims[3];
    break;
  default:
    throw std::invalid_argument("LoadNXcanSAS:: Cannot load a data set with " + std::to_string(rank) + " m_dataDims.");
  }
  return dataSpaceInfo;
}

/**
 * Retrieves workspace dimensionality enum value: oneD , twoD, other (error)
 * @param workspace: The workspace from which to get the data dimensionality
 *
 */
WorkspaceDimensionality getWorkspaceDimensionality(const MatrixWorkspace_sptr &workspace) {
  const auto numberOfHists = workspace->getNumberHistograms();
  return static_cast<WorkspaceDimensionality>(numberOfHists > 1 ? 2 : numberOfHists);
}

} // namespace Mantid::DataHandling::NXcanSAS
