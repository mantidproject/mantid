// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include <H5Cpp.h>
#include <vector>

namespace Mantid::DataHandling::NXcanSAS {
// Utils library for small helper classes, structures and functions used in either loading or saving NXcanSAS data.

enum class WorkspaceDimensionality : std::uint8_t { other = 0, oneD = 1, twoD = 2 };
class DataDimensions {
public:
  // Prepares size and shape vectors and variables for data to be stored in file
  explicit DataDimensions(const hsize_t numberOfPoints, const hsize_t numberOfHistograms,
                          const std::optional<std::pair<size_t, size_t>> &spinVecSize = std::nullopt);
  explicit DataDimensions(const Mantid::API::MatrixWorkspace_sptr &workspace,
                          const std::optional<std::pair<size_t, size_t>> &spinVecSize = std::nullopt);
  [[nodiscard]] const hsize_t &getNumberOfPoints() const;
  [[nodiscard]] const hsize_t &getNumberOfHistograms() const;
  [[nodiscard]] const std::vector<hsize_t> &getDataShape() const;
  [[nodiscard]] const std::vector<hsize_t> &getSlabShape() const;
  [[nodiscard]] const H5::DataSpace &getDataSpace() const;
  [[nodiscard]] const H5::DataType &getDataType() const;

private:
  hsize_t m_numberOfPoints;
  hsize_t m_numberOfHistograms;
  std::vector<hsize_t> m_dataShape;
  std::vector<hsize_t> m_slabShape;
  H5::DataSpace m_dataSpace;
  H5::DataType m_dataType;
};

struct DataSpaceInformation {
  explicit DataSpaceInformation(const size_t dimSpectrumAxis = 0, const size_t dimBin = 0)
      : dimSpectrumAxis(dimSpectrumAxis), dimBin(dimBin), spinStates(1) {}
  size_t dimSpectrumAxis;
  size_t dimBin;
  // 1 if data is not polarized
  size_t spinStates;
};

struct InstrumentNameInfo {
  explicit InstrumentNameInfo(const H5::Group &entry);
  static std::string getInstrumentNameFromFile(const H5::Group &entry);
  static std::string getIdfFromFile(const std::string &instrumentName);
  std::string instrumentName;
  std::string idf;
};

struct SpinVectorBuilder {
  explicit SpinVectorBuilder(const std::vector<std::string> &spinStateStr);
  std::vector<std::string> spinVec;
  std::vector<int> pIn;
  std::vector<int> pOut;
};

struct SpinState {
  std::string strSpinState{""};
  size_t indexPin{0};
  size_t indexPout{0};
};

std::optional<size_t> findWorkspaceIndexForSpinState(const std::vector<std::string> &spinStates,
                                                     const std::string &targetState);

DataSpaceInformation getDataSpaceInfo(const H5::DataSet &dataSet);
WorkspaceDimensionality getWorkspaceDimensionality(const Mantid::API::MatrixWorkspace_sptr &workspace);

} // namespace Mantid::DataHandling::NXcanSAS
