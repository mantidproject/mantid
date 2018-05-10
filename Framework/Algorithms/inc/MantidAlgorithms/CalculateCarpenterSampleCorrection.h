#ifndef MANTID_ALGORITHM_MULTIPLE_SCATTERING_ABSORPTION_H_
#define MANTID_ALGORITHM_MULTIPLE_SCATTERING_ABSORPTION_H_

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidHistogramData/Points.h"
#include <vector>

namespace Mantid {
namespace Algorithms {
/** Multiple scattering absorption correction, originally used to
    correct vanadium spectrum at IPNS.  Algorithm originally worked
    out by Jack Carpenter and Asfia Huq and implmented in Java by
    Alok Chatterjee.  Translated to C++ by Dennis Mikkelson.

    Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
    National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at:
                  <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport CalculateCarpenterSampleCorrection
    : public API::DistributedDataProcessorAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override;

  /// Algorithm's version for identification overriding a virtual method
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CarpenterSampleCorrection", "CylinderAbsorption",
            "MonteCarloAbsorption",      "MayersSampleCorrection",
            "PearlMCAbsorption",         "VesuvioCalculateMS"};
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
  void calculate_abs_correction(const double angle_deg, const double radius,
                                const double coeff1, const double coeff2,
                                const double coeff3,
                                const HistogramData::Points &wavelength,
                                HistogramData::HistogramY &y_val);

  void calculate_ms_correction(const double angle_deg, const double radius,
                               const double coeff1, const double coeff2,
                               const double coeff3,
                               const HistogramData::Points &wavelength,
                               HistogramData::HistogramY &y_val);

  API::MatrixWorkspace_sptr
  createOutputWorkspace(const API::MatrixWorkspace_sptr &inputWS,
                        const std::string) const;
  void deleteWorkspace(API::MatrixWorkspace_sptr workspace);
  API::MatrixWorkspace_sptr
  setUncertainties(API::MatrixWorkspace_sptr workspace);
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_MULTIPLE_SCATTERING_ABSORPTION_H_*/
