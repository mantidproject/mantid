#ifndef MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACE2_H_
#define MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACE2_H_

#include "MantidAlgorithms/ReflectometryWorkflowBase2.h"

namespace Mantid {
namespace Algorithms {

/** CreateTransmissionWorkspace2 : Create a transmission run workspace in
 Wavelength given one or more TOF workspaces. Version 2 of the algorithm.

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
class DLLExport CreateTransmissionWorkspace2
    : public ReflectometryWorkflowBase2 {
public:
  const std::string name() const override;
  const std::string summary() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CreateTransmissionWorkspaceAuto"};
  }
  const std::string category() const override;

private:
  /// Initialize
  void init() override;
  /// Execute
  void exec() override;
  /// Validate inputs
  std::map<std::string, std::string> validateInputs() override;

  /// Normalize by monitors
  API::MatrixWorkspace_sptr
  normalizeDetectorsByMonitors(API::MatrixWorkspace_sptr IvsTOF);
  /// Get the run numbers of the input workspaces
  void getRunNumbers();
  /// Store a transition run in ADS
  void storeTransitionRun(int which, API::MatrixWorkspace_sptr ws);
  /// Store the stitched transition workspace run in ADS
  void storeOutputWorkspace(API::MatrixWorkspace_sptr ws);

  std::string m_firstTransmissionRunNumber;
  std::string m_secondTransmissionRunNumber;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CREATETRANSMISSIONWORKSPACE2_H_ */
