// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidHistogramData/Points.h"
#include <vector>

namespace Mantid {
namespace Algorithms {
/** Multiple scattering absorption correction, originally used to
    correct vanadium spectrum at IPNS.  Algorithm originally worked
    out by Jack Carpenter and Asfia Huq and implmented in Java by
    Alok Chatterjee.  Translated to C++ by Dennis Mikkelson.
 */
class MANTID_ALGORITHMS_DLL CalculateCarpenterSampleCorrection : public API::DistributedDataProcessorAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override;

  /// Algorithm's version for identification overriding a virtual method
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CarpenterSampleCorrection", "CylinderAbsorption", "MonteCarloAbsorption",
            "MayersSampleCorrection",    "PearlMCAbsorption",  "VesuvioCalculateMS"};
  }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override;

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates both absorption  and multiple scattering corrections, "
           "originally used to correct vanadium spectrum at IPNS.";
  }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  /// CalculateCarpenterSampleCorrection correction calculation.
  void calculate_abs_correction(const double angle_deg, const double radius, const double coeff1, const double coeff2,
                                const double coeff3, const HistogramData::Points &wavelength,
                                HistogramData::HistogramY &y_val);

  void calculate_ms_correction(const double angle_deg, const double radius, const double coeff1, const double coeff2,
                               const double coeff3, const HistogramData::Points &wavelength,
                               HistogramData::HistogramY &y_val);

  API::MatrixWorkspace_sptr createOutputWorkspace(const API::MatrixWorkspace_sptr &inputWS, const std::string &) const;
  void deleteWorkspace(const API::MatrixWorkspace_sptr &workspace);
  API::MatrixWorkspace_sptr setUncertainties(const API::MatrixWorkspace_sptr &workspace);
};

} // namespace Algorithms
} // namespace Mantid
