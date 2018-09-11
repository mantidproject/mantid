#ifndef MANTID_ALGORITHMS_EXRACTMEMBERS_H_
#define MANTID_ALGORITHMS_EXRACTMEMBERS_H_

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

namespace Mantid {
namespace Algorithms {

/** ExtractQENSMembers : Extracts the fit members from a QENS fit

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport ExtractQENSMembers : public API::DataProcessorAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;
  std::map<std::string, std::string> validateInputs() override;

  std::vector<Mantid::API::MatrixWorkspace_sptr> getInputWorkspaces() const;

  std::vector<double>
  getQValues(const std::vector<Mantid::API::MatrixWorkspace_sptr> &workspaces);

  std::vector<std::string>
  getAxisLabels(Mantid::API::MatrixWorkspace_sptr workspace,
                size_t axisIndex) const;

  std::vector<std::string>
  renameConvolvedMembers(const std::vector<std::string> &members,
                         const std::vector<std::string> &newNames) const;

  Mantid::API::MatrixWorkspace_sptr
  extractSpectrum(Mantid::API::MatrixWorkspace_sptr inputWS, size_t spectrum);

  Mantid::API::MatrixWorkspace_sptr
  appendSpectra(Mantid::API::MatrixWorkspace_sptr inputWS,
                Mantid::API::MatrixWorkspace_sptr spectraWorkspace);

  Mantid::API::WorkspaceGroup_sptr
  groupWorkspaces(const std::vector<std::string> &workspaceNames);

  std::vector<Mantid::API::MatrixWorkspace_sptr>
  createMembersWorkspaces(Mantid::API::MatrixWorkspace_sptr initialWS,
                          const std::vector<std::string> &members);

  void appendToMembers(Mantid::API::MatrixWorkspace_sptr resultWS,
                       std::vector<Mantid::API::MatrixWorkspace_sptr> &members);

  void setNumericAxis(
      const std::vector<Mantid::API::MatrixWorkspace_sptr> &workspaces,
      const std::vector<double> &values, size_t axisIndex) const;

  std::vector<std::string> addMembersToADS(
      const std::vector<std::string> &members,
      const std::vector<Mantid::API::MatrixWorkspace_sptr> &memberWorkspaces,
      const std::string &outputWSName);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_EXRACTMEMBERS_H_ */
