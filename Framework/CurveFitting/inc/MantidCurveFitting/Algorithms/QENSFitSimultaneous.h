// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_QENSFITSIMULTANEOUS_H_
#define MANTID_ALGORITHMS_QENSFITSIMULTANEOUS_H_

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidCurveFitting/IFittingAlgorithm.h"
#include "MantidKernel/IValidator.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

/**
  QENSFitSimultaneous - Algorithm for performing a simultaneous QENS fit
*/
class DLLExport QENSFitSimultaneous : public IFittingAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override;

protected:
  virtual std::vector<API::MatrixWorkspace_sptr> getWorkspaces() const;
  virtual bool throwIfElasticQConversionFails() const;
  virtual bool isFitParameter(const std::string &name) const;
  std::set<std::string> getUniqueParameterNames() const;
  virtual std::vector<std::string> getFitParameterNames() const;
  virtual std::map<std::string, std::string> getAdditionalLogStrings() const;
  virtual std::map<std::string, std::string> getAdditionalLogNumbers() const;
  virtual API::ITableWorkspace_sptr
  processParameterTable(API::ITableWorkspace_sptr parameterTable);

private:
  void initConcrete() override;
  void execConcrete() override;
  std::vector<API::MatrixWorkspace_sptr> convertInputToElasticQ(
      const std::vector<API::MatrixWorkspace_sptr> &workspaces) const;
  std::pair<API::ITableWorkspace_sptr, API::Workspace_sptr>
  performFit(const std::vector<API::MatrixWorkspace_sptr> &workspaces,
             const std::string &output);
  API::WorkspaceGroup_sptr
  processIndirectFitParameters(API::ITableWorkspace_sptr parameterWorkspace,
                               const std::vector<std::size_t> &grouping);
  void copyLogs(API::WorkspaceGroup_sptr resultWorkspace,
                const std::vector<API::MatrixWorkspace_sptr> &workspaces);
  void copyLogs(API::MatrixWorkspace_sptr resultWorkspace,
                API::WorkspaceGroup_sptr resultGroup);
  void extractMembers(API::WorkspaceGroup_sptr resultGroupWs,
                      const std::vector<API::MatrixWorkspace_sptr> &workspaces,
                      const std::string &outputWsName);
  void addAdditionalLogs(API::WorkspaceGroup_sptr group);
  void addAdditionalLogs(API::Workspace_sptr result);

  API::IAlgorithm_sptr
  extractMembersAlgorithm(API::WorkspaceGroup_sptr resultGroupWs,
                          const std::string &outputWsName) const;

  std::string getOutputBaseName() const;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid

#endif
