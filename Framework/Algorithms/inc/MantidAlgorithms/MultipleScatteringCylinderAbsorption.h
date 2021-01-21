// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DistributedAlgorithm.h"
#include <vector>

namespace Mantid {
namespace HistogramData {
class HistogramX;
class HistogramY;
class HistogramE;
} // namespace HistogramData
namespace Algorithms {
/** Multiple scattering absorption correction, originally used to
    correct vanadium spectrum at IPNS.  Algorithm originally worked
    out by Jack Carpenter and Asfia Huq and implmented in Java by
    Alok Chatterjee.  Translated to C++ by Dennis Mikkelson.

    @author Dennis Mikkelson
    @date 17/08/2010
 */
class MANTID_ALGORITHMS_DLL MultipleScatteringCylinderAbsorption : public API::DistributedAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override;

  /// Algorithm's version for identification overriding a virtual method
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"MonteCarloAbsorption", "MayersSampleCorrection", "PearlMCAbsorption", "VesuvioCalculateMS"};
  }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override;

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Multiple scattering absorption correction, originally used to "
           "correct vanadium spectrum at IPNS.";
  }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  /// MultipleScatteringCylinderAbsorption correction calculation.
  void apply_msa_correction(const double angle_deg, const double radius, const double coeff1, const double coeff2,
                            const double coeff3, const HistogramData::HistogramX &wavelength,
                            HistogramData::HistogramY &y_val, HistogramData::HistogramE &errors);
};

} // namespace Algorithms
} // namespace Mantid
