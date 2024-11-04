// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

// Disable warning 4554 on windows which is inherent in the Eigen::Tensor library.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4554)
#include "Eigen/../unsupported/Eigen/CXX11/Tensor"
#pragma warning(pop)
#else
#include "Eigen/../unsupported/Eigen/CXX11/Tensor"
#endif

#include "Eigen/Core"

typedef Eigen::Tensor<double, 1> Tensor1D;
typedef Eigen::TensorMap<const Eigen::Tensor<double, 1>> TensorMap_const;
typedef Eigen::TensorMap<Eigen::Tensor<double, 1>> TensorMap;
typedef Eigen::Map<const Eigen::VectorXd> EigenMap_const;

namespace Mantid::HistogramData {
class HistogramX;
}

namespace Mantid::Algorithms {

/** FindPeaksConvolve : Finds peak centres using convolution with a shoebox kernel to approximate the second derivative,
 * taking maxima above an I/Sigma threshold. Algorithm designed by Richard Waite and implemented by Mial Lewis.
 */
class MANTID_ALGORITHMS_DLL FindPeaksConvolve : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  struct PeakResult {
    const double centre;
    const double height;
    const double iOverSigma;
    double getAttribute(const std::string &attrString) const;
  };

  std::unordered_map<std::string, Kernel::IValidator_sptr> m_validators;
  API::MatrixWorkspace_sptr m_inputDataWS;
  size_t m_specCount = -1;
  std::vector<int> m_specNums;
  std::vector<std::vector<FindPeaksConvolve::PeakResult>> m_peakResults;
  std::atomic<size_t> m_maxPeakCount;

  bool m_createIntermediateWorkspaces;
  bool m_findHighestDatapointInPeak;
  double m_iOverSigmaThreshold;
  bool m_mergeNearbyPeaks;
  bool m_centreBins;
  Eigen::VectorXd m_pdf;
  std::vector<std::string> m_intermediateWsNames;
  std::mutex mtx;

  void init() override;
  void exec() override;

  void initiateValidators();
  std::pair<std::string, int> secondaryValidation() const;
  const std::string validatePeakExtentInput() const;
  const std::string validateWorkspaceIndexInput() const;
  void storeClassProperties();

  void performConvolution(const size_t dataIndex);
  Tensor1D createKernel(const int binCount) const;
  Tensor1D createSmoothKernel(const size_t kernelSize) const;
  std::pair<size_t, bool> getKernelBinCount(const HistogramData::HistogramX *xData) const;
  double getXDataValue(const HistogramData::HistogramX *xData, const size_t xIndex) const;
  Eigen::VectorXd centreBinsXData(const HistogramData::HistogramX *xData) const;
  void extractPeaks(const size_t dataIndex, const Tensor1D &iOverSigma, const HistogramData::HistogramX *xData,
                    const TensorMap_const &yData, const size_t peakExtentBinNumber);
  size_t findPeakInRawData(const int xIndex, const TensorMap_const &yData, size_t peakExtentBinNumber);
  void storePeakResults(const size_t dataIndex, std::vector<FindPeaksConvolve::PeakResult> &peakCentres);
  void generateNormalPDF(const int peakExtentBinNumber);
  std::vector<std::string> createIntermediateWorkspaces(const size_t dataIndex, const Tensor1D &kernel,
                                                        const Tensor1D &iOverSigma,
                                                        const HistogramData::HistogramX *xData);
  void outputIntermediateWorkspace(const std::string &wsName, const std::vector<double> &xData,
                                   const std::vector<double> &yData);
  void outputResults();
  void outputResultsTable(const std::string &resultHeader);
  std::unordered_map<std::string, API::ITableWorkspace_sptr>
  createOutputTables(const std::vector<std::string> &outputTblNames);
  std::string populateOutputWorkspaces(const std::vector<std::string> &outputTblNames,
                                       const std::unordered_map<std::string, API::ITableWorkspace_sptr> &outputTbls);
  API::WorkspaceGroup_sptr groupOutputWorkspaces(const std::string &outputName,
                                                 const std::vector<std::string> &outputTblNames);
};

} // namespace Mantid::Algorithms
