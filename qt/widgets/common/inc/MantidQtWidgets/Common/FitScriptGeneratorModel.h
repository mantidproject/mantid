// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/FitDomain.h"
#include "MantidQtWidgets/Common/FittingGlobals.h"
#include "MantidQtWidgets/Common/FittingMode.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorModel.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

class IFitScriptGeneratorPresenter;

/**
 * This class stores the domain and fit data to be fitted to. This data is used
 * to generate a python script for complex Mantid fitting.
 */
class EXPORT_OPT_MANTIDQT_COMMON FitScriptGeneratorModel : public IFitScriptGeneratorModel {
public:
  FitScriptGeneratorModel();
  FitScriptGeneratorModel(FitScriptGeneratorModel const &model) = delete;
  FitScriptGeneratorModel &operator=(FitScriptGeneratorModel const &model) = delete;
  ~FitScriptGeneratorModel();

  void subscribePresenter(IFitScriptGeneratorPresenter *presenter) override;

  void removeDomain(FitDomainIndex domainIndex) override;
  void addWorkspaceDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex, double startX,
                          double endX) override;
  [[nodiscard]] bool hasWorkspaceDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex) const override;

  void renameWorkspace(std::string const &workspaceName, std::string const &newName) override;

  [[nodiscard]] bool updateStartX(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                  double startX) override;
  [[nodiscard]] bool updateEndX(std::string const &workspaceName, WorkspaceIndex workspaceIndex, double endX) override;

  void removeFunction(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                      std::string const &function) override;
  void addFunction(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                   std::string const &function) override;
  void setFunction(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                   std::string const &function) override;
  [[nodiscard]] Mantid::API::IFunction_sptr getFunction(std::string const &workspaceName,
                                                        WorkspaceIndex workspaceIndex) const override;

  [[nodiscard]] std::string getEquivalentFunctionIndexForDomain(std::string const &workspaceName,
                                                                WorkspaceIndex workspaceIndex,
                                                                std::string const &functionIndex) const override;
  [[nodiscard]] std::string getEquivalentFunctionIndexForDomain(FitDomainIndex domainIndex,
                                                                std::string const &functionIndex) const override;
  [[nodiscard]] std::string getEquivalentParameterTieForDomain(std::string const &workspaceName,
                                                               WorkspaceIndex workspaceIndex,
                                                               std::string const &fullParameter,
                                                               std::string const &fullTie) const override;
  [[nodiscard]] std::string getAdjustedFunctionIndex(std::string const &parameter) const override;
  [[nodiscard]] std::string getFullParameter(FitDomainIndex domainIndex, std::string const &parameter) const override;
  [[nodiscard]] std::string getFullTie(FitDomainIndex domainIndex, std::string const &tie) const override;

  void updateParameterValue(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                            std::string const &fullParameter, double newValue) override;
  void updateAttributeValue(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                            std::string const &fullAttribute,
                            Mantid::API::IFunction::Attribute const &newValue) override;

  void updateParameterTie(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                          std::string const &fullParameter, std::string const &tie) override;
  void removeParameterConstraint(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                 std::string const &fullParameter) override;
  void updateParameterConstraint(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                 std::string const &functionIndex, std::string const &constraint) override;

  [[nodiscard]] inline std::vector<GlobalTie> getGlobalTies() const noexcept override { return m_globalTies; }

  void setGlobalParameters(std::vector<std::string> const &parameters) override;

  [[nodiscard]] inline std::vector<GlobalParameter> getGlobalParameters() const noexcept override {
    return m_globalParameters;
  }

  void setOutputBaseName(std::string const &outputBaseName) override;

  void setFittingMode(FittingMode fittingMode) override;
  [[nodiscard]] inline FittingMode getFittingMode() const noexcept override { return m_fittingMode; }

  [[nodiscard]] bool isSimultaneousMode() const override;

  [[nodiscard]] bool hasParameter(FitDomainIndex domainIndex, std::string const &fullParameter) const override;

  void setParameterValue(FitDomainIndex domainIndex, std::string const &fullParameter, double value) override;
  void setParameterFixed(FitDomainIndex domainIndex, std::string const &fullParameter, bool fix) override;
  void setParameterTie(FitDomainIndex domainIndex, std::string const &fullParameter, std::string const &tie) override;
  void setParameterConstraint(FitDomainIndex domainIndex, std::string const &fullParameter,
                              std::string const &constraint) override;

  [[nodiscard]] std::string getDomainName(FitDomainIndex domainIndex) const override;
  [[nodiscard]] double getParameterValue(FitDomainIndex domainIndex, std::string const &fullParameter) const override;
  [[nodiscard]] bool isParameterFixed(FitDomainIndex domainIndex, std::string const &fullParameter) const override;
  [[nodiscard]] std::string getParameterTie(FitDomainIndex domainIndex,
                                            std::string const &fullParameter) const override;
  [[nodiscard]] std::string getParameterConstraint(FitDomainIndex domainIndex,
                                                   std::string const &fullParameter) const override;

  [[nodiscard]] inline std::size_t numberOfDomains() const noexcept override { return m_fitDomains.size(); }

  std::tuple<bool, std::string> isValid() const override;

  std::string generatePythonFitScript(
      std::tuple<std::string, std::string, std::string, std::string, std::string, bool> const &fitOptions,
      std::string const &filepath = "") override;

private:
  [[nodiscard]] FitDomainIndex findDomainIndex(std::string const &workspaceName, WorkspaceIndex workspaceIndex) const;
  [[nodiscard]] std::vector<std::unique_ptr<FitDomain>>::const_iterator
  findWorkspaceDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex) const;

  [[nodiscard]] std::string getEquivalentParameterTieForDomain(FitDomainIndex domainIndex,
                                                               std::string const &fullParameter,
                                                               std::string const &fullTie) const;

  void updateParameterTie(FitDomainIndex domainIndex, std::string const &fullParameter, std::string const &fullTie);
  void updateLocalParameterTie(FitDomainIndex domainIndex, std::string const &fullParameter,
                               std::string const &fullTie);
  void updateGlobalParameterTie(FitDomainIndex domainIndex, std::string const &fullParameter,
                                std::string const &fullTie);

  void updateParameterValuesWithLocalTieTo(FitDomainIndex domainIndex, std::string const &parameter, double newValue);
  void updateParameterValuesWithGlobalTieTo(std::string const &fullParameter, double newValue);
  void updateParameterValueInGlobalTie(GlobalTie const &globalTie, double newValue);

  [[nodiscard]] bool validParameter(std::string const &fullParameter) const;
  [[nodiscard]] bool validParameter(FitDomainIndex domainIndex, std::string const &fullParameter) const;
  [[nodiscard]] bool validTie(std::string const &fullTie) const;
  [[nodiscard]] bool validGlobalTie(std::string const &fullParameter, std::string const &fullTie) const;

  [[nodiscard]] bool isParameterValueWithinConstraints(FitDomainIndex domainIndex, std::string const &fullParameter,
                                                       double value) const;

  void clearGlobalTie(std::string const &fullParameter);
  [[nodiscard]] bool hasGlobalTie(std::string const &fullParameter) const;
  [[nodiscard]] std::vector<GlobalTie>::const_iterator findGlobalTie(std::string const &fullParameter) const;
  void checkGlobalTies();

  void checkParameterIsInAllDomains(std::string const &globalParameter) const;
  void checkGlobalParameterhasNoTies(std::string const &globalParameter) const;
  void checkParameterIsNotGlobal(std::string const &fullParameter) const;

  void tryToAdjustParameterInGlobalTieIfInvalidated(GlobalTie &globalTie);
  void tryToAdjustTieInGlobalTieIfInvalidated(GlobalTie &globalTie);

  template <typename Getter>
  auto getParameterProperty(Getter &&func, FitDomainIndex domainIndex, std::string const &fullParameter) const;

  [[nodiscard]] bool checkFunctionExistsInAllDomains() const;
  [[nodiscard]] bool checkFunctionIsSameForAllDomains() const;
  [[nodiscard]] std::string generatePermissibleWarnings() const;

  [[nodiscard]] std::vector<std::string> getInputWorkspaces() const;
  [[nodiscard]] std::vector<std::size_t> getWorkspaceIndices() const;
  [[nodiscard]] std::vector<double> getStartXs() const;
  [[nodiscard]] std::vector<double> getEndXs() const;

  template <typename T, typename Function> std::vector<T> transformDomains(Function const &func) const;

  [[nodiscard]] std::string getFittingType() const;
  [[nodiscard]] Mantid::API::IFunction_sptr getFunction() const;
  [[nodiscard]] Mantid::API::IFunction_sptr getMultiDomainFunction() const;

  void addGlobalParameterTies(Mantid::API::MultiDomainFunction_sptr &function) const;
  std::string constructGlobalParameterTie(GlobalParameter const &globalParameter) const;

  void addGlobalTies(Mantid::API::MultiDomainFunction_sptr &function) const;

  IFitScriptGeneratorPresenter *m_presenter;
  std::string m_outputBaseName;
  std::vector<std::unique_ptr<FitDomain>> m_fitDomains;
  // A vector of global parameters. E.g. f0.A0
  std::vector<GlobalParameter> m_globalParameters;
  // A vector of global ties. E.g. f0.f0.A0=f1.f0.A0
  std::vector<GlobalTie> m_globalTies;
  FittingMode m_fittingMode;
};

template <typename Getter>
auto FitScriptGeneratorModel::getParameterProperty(Getter &&func, FitDomainIndex domainIndex,
                                                   std::string const &fullParameter) const {
  auto const parameter = getAdjustedFunctionIndex(fullParameter);
  if (domainIndex.value < numberOfDomains())
    return std::invoke(std::forward<Getter>(func), m_fitDomains[domainIndex.value], parameter);

  throw std::runtime_error("The domain index provided does not exist.");
}

} // namespace MantidWidgets
} // namespace MantidQt
