// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidAPI/IFunction.h"
#include "MantidQtWidgets/Common/FittingMode.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <string>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

class IFitScriptGeneratorPresenter;

class EXPORT_OPT_MANTIDQT_COMMON IFitScriptGeneratorModel {

public:
  virtual ~IFitScriptGeneratorModel() = default;

  virtual void subscribePresenter(IFitScriptGeneratorPresenter *presenter) = 0;

  virtual void removeWorkspaceDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex) = 0;
  virtual void addWorkspaceDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex, double startX,
                                  double endX) = 0;
  [[nodiscard]] virtual bool hasWorkspaceDomain(std::string const &workspaceName,
                                                WorkspaceIndex workspaceIndex) const = 0;

  [[nodiscard]] virtual bool updateStartX(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                          double startX) = 0;
  [[nodiscard]] virtual bool updateEndX(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                        double endX) = 0;

  virtual void removeFunction(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                              std::string const &function) = 0;
  virtual void addFunction(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                           std::string const &function) = 0;
  virtual void setFunction(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                           std::string const &function) = 0;
  [[nodiscard]] virtual Mantid::API::IFunction_sptr getFunction(std::string const &workspaceName,
                                                                WorkspaceIndex workspaceIndex) const = 0;

  [[nodiscard]] virtual std::string getEquivalentFunctionIndexForDomain(std::string const &workspaceName,
                                                                        WorkspaceIndex workspaceIndex,
                                                                        std::string const &functionIndex) const = 0;
  [[nodiscard]] virtual std::string getEquivalentParameterTieForDomain(std::string const &workspaceName,
                                                                       WorkspaceIndex workspaceIndex,
                                                                       std::string const &fullParameter,
                                                                       std::string const &fullTie) const = 0;

  virtual void updateParameterValue(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                    std::string const &fullParameter, double newValue) = 0;
  virtual void updateAttributeValue(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                    std::string const &fullAttribute,
                                    Mantid::API::IFunction::Attribute const &newValue) = 0;

  virtual void updateParameterTie(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                  std::string const &fullParameter, std::string const &tie) = 0;

  virtual void removeParameterConstraint(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                         std::string const &fullParameter) = 0;
  virtual void updateParameterConstraint(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                         std::string const &functionIndex, std::string const &constraint) = 0;

  virtual void setGlobalParameters(std::vector<std::string> const &parameters) = 0;

  virtual void setFittingMode(FittingMode fittingMode) = 0;
  [[nodiscard]] virtual FittingMode getFittingMode() const = 0;

  [[nodiscard]] virtual std::vector<GlobalTie> getGlobalTies() const = 0;
  [[nodiscard]] virtual std::vector<GlobalParameter> getGlobalParameters() const = 0;

  [[nodiscard]] virtual bool isSimultaneousMode() const = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt
