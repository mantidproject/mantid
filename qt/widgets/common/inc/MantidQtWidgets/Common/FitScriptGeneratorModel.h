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
#include "MantidQtWidgets/Common/FittingMode.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorModel.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <string>
#include <utility>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

struct GlobalTie {

  GlobalTie(std::string const &parameter, std::string const &tie) {
    m_parameter = parameter;
    m_tie = tie;
  }

  std::string m_parameter;
  std::string m_tie;
};

/**
 * This class stores the domain and fit data to be fitted to. This data is used
 * to generate a python script for complex Mantid fitting.
 */
class EXPORT_OPT_MANTIDQT_COMMON FitScriptGeneratorModel
    : public IFitScriptGeneratorModel {
public:
  FitScriptGeneratorModel();
  ~FitScriptGeneratorModel();

  void removeWorkspaceDomain(std::string const &workspaceName,
                             WorkspaceIndex workspaceIndex) override;
  void addWorkspaceDomain(std::string const &workspaceName,
                          WorkspaceIndex workspaceIndex, double startX,
                          double endX) override;

  [[nodiscard]] bool updateStartX(std::string const &workspaceName,
                                  WorkspaceIndex workspaceIndex,
                                  double startX) override;
  [[nodiscard]] bool updateEndX(std::string const &workspaceName,
                                WorkspaceIndex workspaceIndex,
                                double endX) override;

  void removeFunction(std::string const &workspaceName,
                      WorkspaceIndex workspaceIndex,
                      std::string const &function) override;
  void addFunction(std::string const &workspaceName,
                   WorkspaceIndex workspaceIndex,
                   std::string const &function) override;
  void setFunction(std::string const &workspaceName,
                   WorkspaceIndex workspaceIndex,
                   std::string const &function) override;
  Mantid::API::IFunction_sptr
  getFunction(std::string const &workspaceName,
              WorkspaceIndex workspaceIndex) override;

  std::string getEquivalentParameterForDomain(
      std::string const &workspaceName, WorkspaceIndex workspaceIndex,
      std::string const &fullParameter) const override;

  void updateParameterValue(std::string const &workspaceName,
                            WorkspaceIndex workspaceIndex,
                            std::string const &parameter,
                            double newValue) override;
  void updateAttributeValue(
      std::string const &workspaceName, WorkspaceIndex workspaceIndex,
      std::string const &attribute,
      Mantid::API::IFunction::Attribute const &newValue) override;
  void updateParameterTie(std::string const &workspaceName,
                          WorkspaceIndex workspaceIndex,
                          std::string const &parameter,
                          std::string const &tie) override;

  void setFittingMode(FittingMode const &fittingMode) override;
  [[nodiscard]] inline FittingMode getFittingMode() const noexcept override {
    return m_fittingMode;
  }

private:
  [[nodiscard]] std::size_t
  findDomainIndex(std::string const &workspaceName,
                  WorkspaceIndex workspaceIndex) const;
  [[nodiscard]] std::vector<FitDomain>::const_iterator
  findWorkspaceDomain(std::string const &workspaceName,
                      WorkspaceIndex workspaceIndex) const;
  [[nodiscard]] bool hasWorkspaceDomain(std::string const &workspaceName,
                                        WorkspaceIndex workspaceIndex) const;

  void updateParameterTie(std::size_t const &domainIndex,
                          std::string const &fullParameter,
                          std::string const &fullTie);
  void updateLocalParameterTie(std::size_t const &domainIndex,
                               std::string const &fullParameter,
                               std::string const &fullTie);
  void updateGlobalParameterTie(std::size_t const &domainIndex,
                                std::string const &fullParameter,
                                std::string const &fullTie);

  [[nodiscard]] bool validParameter(std::string const &fullParameter) const;

  [[nodiscard]] bool validGlobalTie(std::string const &fullTie) const;
  void clearGlobalTie(std::string const &fullParameter);
  [[nodiscard]] std::vector<GlobalTie>::const_iterator
  findGlobalTie(std::string const &fullParameter) const;

  [[nodiscard]] inline std::size_t numberOfDomains() const noexcept {
    return m_fitDomains.size();
  }

  std::vector<FitDomain> m_fitDomains;
  // A vector of global ties. E.g. f0.f0.A0=f1.f0.A0
  std::vector<GlobalTie> m_globalTies;
  FittingMode m_fittingMode;
};

} // namespace MantidWidgets
} // namespace MantidQt
