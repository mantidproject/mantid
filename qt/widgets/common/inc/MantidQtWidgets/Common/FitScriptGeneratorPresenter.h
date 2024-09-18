// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/FittingMode.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorPresenter.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <string>
#include <tuple>
#include <vector>

#include <QStringList>

namespace MantidQt {
namespace MantidWidgets {

class IFitScriptGeneratorModel;
class IFitScriptGeneratorView;
struct GlobalParameter;
struct GlobalTie;

class EXPORT_OPT_MANTIDQT_COMMON FitScriptGeneratorPresenter : public IFitScriptGeneratorPresenter {
public:
  FitScriptGeneratorPresenter(IFitScriptGeneratorView *view, IFitScriptGeneratorModel *model,
                              QStringList const &workspaceNames = QStringList(), double startX = 0.0,
                              double endX = 0.0);
  ~FitScriptGeneratorPresenter() override;

  void notifyPresenter(ViewEvent const &event, [[maybe_unused]] std::string const &arg1 = "",
                       [[maybe_unused]] std::string const &arg2 = "") override;
  void notifyPresenter(ViewEvent const &event, std::vector<std::string> const &vec) override;
  void notifyPresenter(ViewEvent const &event, FittingMode fittingMode) override;
  void handleAddDomainAccepted(std::vector<Mantid::API::MatrixWorkspace_const_sptr> const &workspaces,
                               FunctionModelSpectra const &workspaceIndices) override;

  void openFitScriptGenerator() override;

  void setGlobalTies(std::vector<GlobalTie> const &globalTies) override;
  void setGlobalParameters(std::vector<GlobalParameter> const &globalParameters) override;

private:
  void handleADSDeleteEvent(std::string const &workspaceName);
  void handleADSClearEvent();
  void handleADSRenameEvent(std::string const &workspaceName, std::string const &newName);
  void handleRemoveDomainClicked();
  void handleAddDomainClicked();
  void handleSelectionChanged();
  void handleStartXChanged();
  void handleEndXChanged();
  void handleFunctionRemoved(std::string const &function);
  void handleFunctionAdded(std::string const &function);
  void handleFunctionReplaced(std::string const &function);
  void handleParameterChanged(std::string const &parameter);
  void handleAttributeChanged(std::string const &attribute);
  void handleParameterTieChanged(std::string const &parameter, std::string const &tie);
  void handleParameterConstraintRemoved(std::string const &parameter);
  void handleParameterConstraintChanged(std::string const &functionIndex, std::string const &constraint);
  void handleGlobalParametersChanged(std::vector<std::string> const &globalParameters);
  void handleEditLocalParameterClicked(std::string const &parameter);
  void handleEditLocalParameterFinished();
  void handleOutputBaseNameChanged(std::string const &outputBaseName);
  void handleFittingModeChanged(FittingMode fittingMode);
  void handleGenerateScriptToFileClicked();
  void handleGenerateScriptToClipboardClicked();

  void setWorkspaces(QStringList const &workspaceNames, double startX, double endX);
  void addWorkspaces(std::vector<Mantid::API::MatrixWorkspace_const_sptr> const &workspaces,
                     FunctionModelSpectra const &workspaceIndices);
  void addWorkspace(std::string const &workspaceName, double startX, double endX);
  void addWorkspace(Mantid::API::MatrixWorkspace_const_sptr const &workspace, double startX, double endX);
  void addWorkspace(Mantid::API::MatrixWorkspace_const_sptr const &workspace, WorkspaceIndex workspaceIndex,
                    double startX, double endX);
  void addWorkspace(std::string const &workspaceName, WorkspaceIndex workspaceIndex, double startX, double endX);

  void removeDomains(std::vector<FitDomainIndex> const &domainIndices);

  void updateStartX(std::string const &workspaceName, WorkspaceIndex workspaceIndex, double startX);
  void updateEndX(std::string const &workspaceName, WorkspaceIndex workspaceIndex, double endX);

  void updateParameterValue(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                            std::string const &parameter, double newValue);
  void updateAttributeValue(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                            std::string const &fullAttribute, Mantid::API::IFunction::Attribute const &newValue);

  void updateParameterTie(std::string const &workspaceName, WorkspaceIndex workspaceIndex, std::string const &parameter,
                          std::string const &tie);

  void removeParameterConstraint(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                 std::string const &parameter);
  void updateParameterConstraint(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                 std::string const &functionIndex, std::string const &constraint);

  void removeFunction(std::string const &workspaceName, WorkspaceIndex workspaceIndex, std::string const &function);
  void addFunction(std::string const &workspaceName, WorkspaceIndex workspaceIndex, std::string const &function);
  void setFunction(std::string const &workspaceName, WorkspaceIndex workspaceIndex, std::string const &function);

  void updateFunctionInViewFromModel(FitDomainIndex domainIndex);

  template <typename GetX, typename UpdateX> void updateXLimitForDomain(GetX &&getX, UpdateX &&updateX);

  template <typename UpdateFunction>
  void updateFunctionStructure(UpdateFunction &&updateFunction, std::string const &function);

  template <typename UpdateFunction, typename... Args>
  void updateFunctionsInModel(UpdateFunction &&updateFunction, Args... arguments);

  template <typename Function, typename... Args>
  void invokeFunctionForDomain(FitDomainIndex domainIndex, Function &&func, Args... arguments);

  [[nodiscard]] std::vector<FitDomainIndex> getRowIndices() const;

  void insertLocalParameterData(std::string const &parameter, std::vector<std::string> &workspaceNames,
                                std::vector<std::string> &domainNames, std::vector<double> &values,
                                std::vector<bool> &fixes, std::vector<std::string> &ties,
                                std::vector<std::string> &constraints) const;
  void insertLocalParameterDataForDomain(FitDomainIndex domainIndex, std::string const &parameter,
                                         std::vector<std::string> &workspaceNames,
                                         std::vector<std::string> &domainNames, std::vector<double> &values,
                                         std::vector<bool> &fixes, std::vector<std::string> &ties,
                                         std::vector<std::string> &constraints) const;

  void setLocalParameterDataForDomain(FitDomainIndex domainIndex, std::string const &parameter, double value, bool fix,
                                      std::string const &tie, std::string const &constraint);

  std::vector<FitDomainIndex> getDomainsWithLocalParameter(std::string const &parameter) const;

  std::tuple<std::string, std::string> convertFunctionIndexOfParameterTie(std::string const &workspaceName,
                                                                          WorkspaceIndex workspaceIndex,
                                                                          std::string const &parameter,
                                                                          std::string const &tie) const;

  void checkForWarningMessages();

  template <typename Generator> void generateFitScript(Generator &&func) const;
  void generateScriptToFile() const;
  void generateScriptToClipboard() const;

  std::vector<std::string> m_warnings;

  IFitScriptGeneratorView *m_view;
  IFitScriptGeneratorModel *m_model;
};

} // namespace MantidWidgets
} // namespace MantidQt
