#ifndef MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE2_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE2_H_

#include "MantidKernel/System.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/ReflectometryWorkflowBase2.h"

namespace Mantid {
namespace Algorithms {

/** ReflectometryReductionOne2 : Reflectometry reduction of a single input TOF
 workspace to an IvsQ workspace. Version 2 of the algorithm.

 Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

 File change history is stored at: <https://github.com/mantidproject/mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport ReflectometryReductionOne2 : public ReflectometryWorkflowBase2 {
public:
  /// Algorithm's name for identification
  const std::string name() const override {
    return "ReflectometryReductionOne";
  };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Reduces a single TOF/Lambda reflectometry run into a mod Q vs I/I0 "
           "workspace. Performs monitor normalization and transmission "
           "corrections.";
  }
  /// Algorithm's version for identification.
  int version() const override { return 2; };
  /// Algorithm's category for identification.
  const std::string category() const override { return "Reflectometry"; };

private:
  /** Overridden Algorithm methods **/

  // Initialize the algorithm
  void init() override;
  // Execute the algorithm
  void exec() override;
  // Validate inputs
  std::map<std::string, std::string> validateInputs() override;
  // Create a direct beam workspace from input workspace in wavelength
  Mantid::API::MatrixWorkspace_sptr
  makeDirectBeamWS(Mantid::API::MatrixWorkspace_sptr inputWS);
  // Performs transmission corrections
  Mantid::API::MatrixWorkspace_sptr
  transmissionCorrection(Mantid::API::MatrixWorkspace_sptr detectorWS);
  // Performs transmission corrections using alternative correction algorithms
  Mantid::API::MatrixWorkspace_sptr
  algorithmicCorrection(Mantid::API::MatrixWorkspace_sptr detectorWS);
  // convert to momentum transfer, rebin and scale
  Mantid::API::MatrixWorkspace_sptr
  convertToQ(Mantid::API::MatrixWorkspace_sptr inputWS);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONE2_H_ */
