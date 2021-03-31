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
#include "MantidQtWidgets/Common/FitDomain.h"
#include "MantidQtWidgets/Common/FittingGlobals.h"
#include "MantidQtWidgets/Common/FittingMode.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorModel.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <memory>
#include <string>
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

  void removeWorkspaceDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex) override;
  void addWorkspaceDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex, double startX,
                          double endX) override;
  [[nodiscard]] bool hasWorkspaceDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex) const override;

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
  [[nodiscard]] std::string getEquivalentParameterTieForDomain(std::string const &workspaceName,
                                                               WorkspaceIndex workspaceIndex,
                                                               std::string const &fullParameter,
                                                               std::string const &fullTie) const override;

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

  void setFittingMode(FittingMode fittingMode) override;
  [[nodiscard]] inline FittingMode getFittingMode() const noexcept override { return m_fittingMode; }

  [[nodiscard]] bool isSimultaneousMode() const override;

private:
  [[nodiscard]] FitDomainIndex findDomainIndex(std::string const &workspaceName, WorkspaceIndex workspaceIndex) const;
  [[nodiscard]] std::vector<std::unique_ptr<FitDomain>>::const_iterator
  findWorkspaceDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex) const;

  [[nodiscard]] std::string getEquivalentParameterTieForDomain(FitDomainIndex domainIndex,
                                                               std::string const &fullParameter,
                                                               std::string const &fullTie) const;
  [[nodiscard]] std::string getAdjustedFunctionIndex(std::string const &parameter) const;

  void updateParameterTie(FitDomainIndex domainIndex, std::string const &fullParameter, std::string const &fullTie);
  void updateLocalParameterTie(FitDomainIndex domainIndex, std::string const &fullParameter,
                               std::string const &fullTie);
  void updateGlobalParameterTie(FitDomainIndex domainIndex, std::string const &fullParameter,
                                std::string const &fullTie);

  void updateParameterValuesWithLocalTieTo(FitDomainIndex domainIndex, std::string const &parameter, double newValue);
  void updateParameterValuesWithGlobalTieTo(std::string const &fullParameter, double newValue);
  void updateParameterValueInGlobalTie(GlobalTie const &globalTie, double newValue);

  [[nodiscard]] double getParameterValue(FitDomainIndex domainIndex, std::string const &fullParameter) const;

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

  [[nodiscard]] inline std::size_t numberOfDomains() const noexcept { return m_fitDomains.size(); }

  void checkParameterIsInAllDomains(std::string const &globalParameter) const;
  void checkGlobalParameterhasNoTies(std::string const &globalParameter) const;
  void checkParameterIsNotGlobal(std::string const &fullParameter) const;

  void tryToAdjustParameterInGlobalTieIfInvalidated(GlobalTie &globalTie);
  void tryToAdjustTieInGlobalTieIfInvalidated(GlobalTie &globalTie);

  IFitScriptGeneratorPresenter *m_presenter;
  std::vector<std::unique_ptr<FitDomain>> m_fitDomains;
  // A vector of global parameters. E.g. f0.A0
  std::vector<GlobalParameter> m_globalParameters;
  // A vector of global ties. E.g. f0.f0.A0=f1.f0.A0
  std::vector<GlobalTie> m_globalTies;
  FittingMode m_fittingMode;
};

} // namespace MantidWidgets
} // namespace MantidQt
