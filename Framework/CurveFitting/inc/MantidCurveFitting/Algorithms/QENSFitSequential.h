// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
  void addAdditionalLogs(API::WorkspaceGroup_sptr resultWorkspace);
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
                        std::vector<std::string> const &spectra,
                        std::string const &outputBaseName,
                        std::string const &endOfSuffix);
  void renameWorkspaces(API::WorkspaceGroup_sptr outputGroup,
                        std::vector<std::string> const &spectra,
                        std::string const &outputBaseName,
                        std::string const &endOfSuffix,
                        std::vector<API::MatrixWorkspace_sptr> const &names);
  void renameGroupWorkspace(std::string const &currentName,
                            std::vector<std::string> const &spectra,
                            std::string const &outputBaseName,
                            std::string const &endOfSuffix);
  void copyLogs(API::WorkspaceGroup_sptr resultWorkspaces,
                std::vector<API::MatrixWorkspace_sptr> const &workspaces);
  void copyLogs(API::Workspace_sptr resultWorkspace,
                std::vector<API::MatrixWorkspace_sptr> const &workspaces);
  void copyLogs(API::MatrixWorkspace_sptr resultWorkspace,
                API::WorkspaceGroup_sptr resultGroup);
  void copyLogs(API::MatrixWorkspace_sptr resultWorkspace,
                API::Workspace_sptr resultGroup);
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
