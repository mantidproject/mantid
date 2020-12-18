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
#include "MantidQtWidgets/Common/IFitScriptGeneratorModel.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <string>
#include <utility>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

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

private:
  [[nodiscard]] std::size_t
  findDomainIndex(std::string const &workspaceName,
                  WorkspaceIndex workspaceIndex) const;
  [[nodiscard]] std::vector<FitDomain>::const_iterator
  findWorkspaceDomain(std::string const &workspaceName,
                      WorkspaceIndex workspaceIndex) const;
  [[nodiscard]] bool hasWorkspaceDomain(std::string const &workspaceName,
                                        WorkspaceIndex workspaceIndex) const;

  void updateParameterTie(Mantid::API::IFunction_sptr const &function,
                          std::string const &parameter, std::string const &tie);

  [[nodiscard]] inline std::size_t numberOfDomains() const noexcept {
    return m_fitDomains.size();
  }

  std::vector<FitDomain> m_fitDomains;
};

} // namespace MantidWidgets
} // namespace MantidQt
