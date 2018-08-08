#ifndef MANTID_ALGORITHMS_QENSFITSEQUENTIAL_H_
#define MANTID_ALGORITHMS_QENSFITSEQUENTIAL_H_

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/IValidator.h"

#include <set>

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

/**
  QENSFitSequential - Performs a sequential QENS fit

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
class DLLExport QENSFitSequential : public API::DataProcessorAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override;

protected:
  std::map<std::string, std::string> validateInputs() override;
  virtual std::vector<API::MatrixWorkspace_sptr> getWorkspaces() const;
  virtual std::map<std::string, std::string> getAdditionalLogStrings() const;
  virtual std::map<std::string, std::string> getAdditionalLogNumbers() const;
  virtual API::ITableWorkspace_sptr
  processParameterTable(API::ITableWorkspace_sptr parameterTable);
  virtual std::vector<std::string> getFitParameterNames() const;

private:
  void init() override;
  void exec() override;
  API::ITableWorkspace_sptr performFit(const std::string &input,
                                       const std::string &output);
  void deleteTemporaryWorkspaces(const std::string &outputBaseName);
  void addAdditionalLogs(API::Workspace_sptr result);

  virtual bool throwIfElasticQConversionFails() const;
  virtual bool isFitParameter(const std::string &parameterName) const;
  std::set<std::string> getUniqueParameterNames() const;
  std::string getOutputBaseName() const;
  std::string getInputString(
      const std::vector<API::MatrixWorkspace_sptr> &workspaces) const;
  std::vector<std::size_t> getDatasetGrouping(
      const std::vector<API::MatrixWorkspace_sptr> &workspaces) const;
  API::WorkspaceGroup_sptr
  processIndirectFitParameters(API::ITableWorkspace_sptr parameterWorkspace,
                               const std::vector<std::size_t> &grouping);

  std::vector<API::MatrixWorkspace_sptr> convertInputToElasticQ(
      const std::vector<API::MatrixWorkspace_sptr> &workspaces) const;

  void renameWorkspaces(API::WorkspaceGroup_sptr outputGroup,
                        const std::vector<std::string> &spectra);
  void renameWorkspaces(API::WorkspaceGroup_sptr outputGroup,
                        const std::vector<std::string> &spectra,
                        const std::vector<API::MatrixWorkspace_sptr> &names);
  void copyLogs(API::WorkspaceGroup_sptr resultWorkspace,
                const std::vector<API::MatrixWorkspace_sptr> &workspaces);
  void copyLogs(API::MatrixWorkspace_sptr resultWorkspace,
                API::WorkspaceGroup_sptr resultGroup);
  void extractMembers(API::WorkspaceGroup_sptr resultGroupWs,
                      const std::vector<API::MatrixWorkspace_sptr> &workspaces,
                      const std::string &outputWsName);

  API::IAlgorithm_sptr
  extractMembersAlgorithm(API::WorkspaceGroup_sptr resultGroupWs,
                          const std::string &outputWsName) const;

  std::string getTemporaryName() const;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid

#endif
