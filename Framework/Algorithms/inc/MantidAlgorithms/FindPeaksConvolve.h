// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidHistogramData/Histogram.h"

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

namespace Mantid::Algorithms {

/** FindPeaksConvolve : TODO: DESCRIPTION
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
  bool m_performBinaryClosing;

  void init() override;
  void exec() override;

  void initiateValidators();
  std::pair<std::string, int> secondaryValidation() const;
  const std::string validatePeakExtentInput() const;
  void storeClassProperties();

  void performConvolution(const size_t dataIndex);
  Tensor1D createKernel(const int binCount);
  Tensor1D createSmoothKernel(const size_t kernelSize);
  size_t getKernelBinCount(const HistogramData::HistogramX *xData) const;
  Eigen::VectorXd centreBinsXData(const HistogramData::HistogramX *xData);
  void extractPeaks(const size_t dataIndex, const Tensor1D &iOverSigma, const EigenMap_const &xData,
                    const TensorMap_const &yData, const size_t peakExtentBinNumber);
  std::vector<std::pair<const int, const double>> filterDataForSignificantPoints(const Tensor1D &iOverSigma);
  size_t findPeakInRawData(const int xIndex, const TensorMap_const &yData, size_t peakExtentBinNumber);
  void storePeakResults(const size_t dataIndex, std::vector<FindPeaksConvolve::PeakResult> &peakCentres);
  Eigen::VectorXd generateNormalPDF(const int peakExtentBinNumber);
  void createIntermediateWorkspaces(const size_t dataIndex, const Tensor1D &kernel, const Tensor1D &iOverSigma,
                                    const EigenMap_const &xData);
  void outputResults();
};

} // namespace Mantid::Algorithms