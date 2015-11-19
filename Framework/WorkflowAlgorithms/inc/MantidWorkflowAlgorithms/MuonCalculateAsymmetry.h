#ifndef MANTID_WORKFLOWALGORITHMS_MUONCALCULATEASYMMETRY_H_
#define MANTID_WORKFLOWALGORITHMS_MUONCALCULATEASYMMETRY_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace WorkflowAlgorithms {
/** MuonCalculateAsymmetry : converts loaded/prepared Muon data to a data
  suitable for analysis.

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
class DLLExport MuonCalculateAsymmetry : public API::Algorithm {
public:
  MuonCalculateAsymmetry();
  virtual ~MuonCalculateAsymmetry();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Converts loaded/prepared Muon data to a data suitable for "
           "analysis.";
  }

  virtual int version() const;
  virtual const std::string category() const;

private:
  void init();
  void exec();

  // Calculates raw counts
  API::MatrixWorkspace_sptr
  calculateGroupCounts(const API::WorkspaceGroup_const_sptr &inputWSGroup,
                       int groupIndex, const std::vector<int> &summedPeriods,
                       const std::vector<int> &subtractedPeriods);
  // Calculates asymmetry for specified spectrum
  API::MatrixWorkspace_sptr
  calculateGroupAsymmetry(const API::WorkspaceGroup_const_sptr &inputWSGroup,
                          int groupIndex, const std::vector<int> &summedPeriods,
                          const std::vector<int> &subtractedPeriods);
  // Calculates asymmetry for a pair of spectra
  API::MatrixWorkspace_sptr
  calculatePairAsymmetry(const API::WorkspaceGroup_const_sptr &inputWSGroup,
                         int firstPairIndex, int secondPairIndex, double alpha,
                         const std::vector<int> &summedPeriods,
                         const std::vector<int> &subtractedPeriods);
  /// Checks if the supplied properties are valid or not
  std::map<std::string, std::string> validateInputs() override;
  /// Checks if periods in set are valid
  std::vector<int> findInvalidPeriods(const std::vector<int> &periodSet) const;
  /// Builds error string for invalid period numbers
  std::string buildErrorString(const std::vector<int> &invalidPeriods) const;
  /// Sums the specified periods in the supplied workspace group
  API::MatrixWorkspace_sptr
  sumPeriods(const API::WorkspaceGroup_const_sptr &inputWSGroup,
             const std::vector<int> &periodsToSum);
  /// Subtracts one workspace from another (lhs - rhs)
  API::MatrixWorkspace_sptr
  subtractWorkspaces(const API::MatrixWorkspace_sptr &lhs,
                     const API::MatrixWorkspace_sptr &rhs);
  /// Extracts a single spectrum from a workspace
  API::MatrixWorkspace_sptr extractSpectrum(const API::Workspace_sptr &inputWS,
                                            const int index);
  /// Removes exponential decay from the workspace
  API::MatrixWorkspace_sptr removeExpDecay(const API::Workspace_sptr &inputWS,
                                           const int index);
  /// Performs asymmetry calculation on the workspace
  API::MatrixWorkspace_sptr asymmetryCalc(const API::Workspace_sptr &inputWS,
                                          const int firstPairIndex,
                                          const int secondPairIndex,
                                          const double alpha);
};

} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /* MANTID_WORKFLOWALGORITHMS_MUONCALCULATEASYMMETRY_H_ */
