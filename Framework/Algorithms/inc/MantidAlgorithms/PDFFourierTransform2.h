// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** PDFFourierTransform : TODO: DESCRIPTION
 */
class MANTID_ALGORITHMS_DLL PDFFourierTransform2 : public API::Algorithm {
public:
  ~PDFFourierTransform2() override = default;
  /// Algorithm's name for identification
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Fourier transform from S(Q) to G(r), which is paired distribution "
           "function (PDF). G(r) will be stored in another named workspace.";
  }

  /// Algorithm's version for identification
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"FFT"}; }
  /// Algorithm's category for identification
  const std::string category() const override;
  /// @copydoc Algorithm::validateInputs()
  std::map<std::string, std::string> validateInputs() override;
  void convertToLittleGRMinus1(std::vector<double> &FOfR, const std::vector<double> &R, std::vector<double> &DFOfR,
                               const std::vector<double> &DR, const std::string &PDFType, const double &rho0,
                               const double &cohScatLen);

protected:
  size_t determineMinIndex(double min, const std::vector<double> &X, const std::vector<double> &Y);
  size_t determineMaxIndex(double max, const std::vector<double> &X, const std::vector<double> &Y);

private:
  /// Initialize the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  double determineRho0();
  void convertToSQMinus1(std::vector<double> &FOfQ, std::vector<double> &Q, std::vector<double> &DFOfQ,
                         const std::vector<double> &DQ);
  void convertFromSQMinus1(HistogramData::HistogramY &FOfQ, const HistogramData::HistogramX &Q,
                           HistogramData::HistogramE &DFOfQ);
  void convertFromLittleGRMinus1(HistogramData::HistogramY &FOfR, const HistogramData::HistogramX &R,
                                 HistogramData::HistogramE &DFOfR, const std::string &PDFType, const double &rho0,
                                 const double &cohScatLen);
};

} // namespace Algorithms
} // namespace Mantid
