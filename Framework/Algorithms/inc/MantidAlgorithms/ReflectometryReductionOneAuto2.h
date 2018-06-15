#ifndef MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTO2_H_
#define MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTO2_H_

#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "ReflectometryWorkflowBase2.h"

namespace Mantid {
namespace Algorithms {

/** ReflectometryReductionOneAuto2 : Algorithm to run ReflectometryReductionOne,
attempting to pick instrument parameters for missing properties. Version 2.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport ReflectometryReductionOneAuto2
    : public ReflectometryWorkflowBase2 {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"ReflectometryReductionOne"};
  }
  const std::string category() const override;
  const std::string summary() const override;

  /// Validate inputs
  std::map<std::string, std::string> validateInputs() override;

  /// For (multiperiod) workspace groups
  bool checkGroups() override;
  bool processGroups() override;

  /// Sums transmission workspaces belonging to a group
  Mantid::API::MatrixWorkspace_sptr
  sumTransmissionWorkspaces(Mantid::API::WorkspaceGroup_sptr &transGroup);

private:
  void init() override;
  void exec() override;
  // Set default names for output workspaces
  void setDefaultOutputWorkspaceNames();
  /// Get the name of the detectors of interest based on processing instructions
  std::vector<std::string>
  getDetectorNames(const std::string &instructions,
                   Mantid::API::MatrixWorkspace_sptr inputWS);
  /// Correct detector positions vertically
  Mantid::API::MatrixWorkspace_sptr
  correctDetectorPositions(const std::string &instructions,
                           Mantid::API::MatrixWorkspace_sptr inputWS,
                           const double twoTheta);
  /// Calculate theta
  double calculateTheta(const std::string &instructions,
                        Mantid::API::MatrixWorkspace_sptr inputWS);
  /// Rebin and scale a workspace in Q
  Mantid::API::MatrixWorkspace_sptr
  rebinAndScale(Mantid::API::MatrixWorkspace_sptr inputWS, double theta,
                std::vector<double> &params);
  /// Populate algorithmic correction properties
  void populateAlgorithmicCorrectionProperties(
      Mantid::API::IAlgorithm_sptr alg,
      Mantid::Geometry::Instrument_const_sptr instrument);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_REFLECTOMETRYREDUCTIONONEAUTO2_H_ */
