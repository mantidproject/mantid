// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace Algorithms {

/** WienerSmooth algorithm performes smoothing data in a spectrum of a matrix
  workspace
  using the Wiener filter smoothing.
*/
class MANTID_ALGORITHMS_DLL WienerSmooth : public API::Algorithm {
public:
  const std::string name() const override { return "WienerSmooth"; }
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"FFTSmooth"}; }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  std::pair<double, double> getStartEnd(const Mantid::HistogramData::HistogramX &X, bool isHistogram) const;
  API::MatrixWorkspace_sptr copyInput(const API::MatrixWorkspace_sptr &inputWS, size_t wsIndex);
  API::MatrixWorkspace_sptr smoothSingleSpectrum(API::MatrixWorkspace_sptr inputWS, size_t wsIndex);
};

} // namespace Algorithms
} // namespace Mantid
