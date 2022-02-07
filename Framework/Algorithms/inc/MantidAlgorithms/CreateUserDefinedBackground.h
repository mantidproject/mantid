// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidHistogramData/Histogram.h"
namespace Mantid {
namespace Kernel {
class Interpolation;
}
namespace Algorithms {

/** CreateUserDefinedBackground : Given an input workspace containing data with
  a background and a table of user-selected points defining the background,
  creates a new workspace containing background data that can be subtracted
  from the original data.
*/
class MANTID_ALGORITHMS_DLL CreateUserDefinedBackground : public API::Algorithm {
public:
  /// Return name of algorithm
  const std::string name() const override;
  /// Version number
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"RemoveBackground", "CalculatePolynomialBackground"};
  }
  /// Category algorithm belongs to
  const std::string category() const override;
  /// Description of algorithm
  const std::string summary() const override;

private:
  /// Declare properties and initialise algorithm
  void init() override;
  /// Execute algorithm
  void exec() override;
  /// Verify input properties and return errors
  std::map<std::string, std::string> validateInputs() override;
  /// Sort table and remove blank rows
  void cleanUpTable(API::ITableWorkspace_sptr &table) const;
  /// Extend background to cover range of data
  void extendBackgroundToData(API::ITableWorkspace_sptr &background, const API::MatrixWorkspace_const_sptr &data) const;
  /// Create new background workspace with data interpolated from table
  API::MatrixWorkspace_sptr createBackgroundWorkspace(const API::ITableWorkspace_const_sptr &background,
                                                      const API::MatrixWorkspace_const_sptr &data) const;
  /// Set up and return an interpolator object ready for use
  Kernel::Interpolation getInterpolator(const API::ITableWorkspace_const_sptr &background,
                                        const API::MatrixWorkspace_const_sptr &workspace) const;
  /// Get Y storage mode for background data
  HistogramData::Histogram::YMode getBackgroundYMode(const API::MatrixWorkspace_const_sptr &data,
                                                     std::vector<double> &yBackground) const;
  /// Multiply y data by bin width
  void multiplyByBinWidth(const API::MatrixWorkspace_const_sptr &data, std::vector<double> &yBackground) const;
  /// Key name for "normalize histogram to bin width" option on plots
  static const std::string AUTODISTRIBUTIONKEY;
};

} // namespace Algorithms
} // namespace Mantid
