// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <string>
#include <utility>

namespace MantidQt {
namespace MantidWidgets {

/**
 * This class is used to store all data relating to a single domain to be
 * fitted. This includes the location of the domain (workspace Name & index),
 * the fit range (start and end X), and the function to be fitted over.
 */
class EXPORT_OPT_MANTIDQT_COMMON FitDomain {

public:
  FitDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
            double startX, double endX);

  [[nodiscard]] bool setStartX(double startX);
  [[nodiscard]] bool setEndX(double startX);

  inline std::string workspaceName() const noexcept { return m_workspaceName; }
  inline WorkspaceIndex workspaceIndex() const noexcept {
    return m_workspaceIndex;
  }

  void setFunction(Mantid::API::IFunction_sptr const &function);
  [[nodiscard]] Mantid::API::IFunction_sptr getFunction() const;
  void removeFunction(std::string const &function);
  void addFunction(Mantid::API::IFunction_sptr const &function);

  void setParameterValue(std::string const &parameter, double newValue);
  void setAttributeValue(std::string const &attribute,
                         Mantid::API::IFunction::Attribute newValue);

  [[nodiscard]] bool hasParameter(std::string const &parameter) const;
  [[nodiscard]] double getParameterValue(std::string const &parameter) const;

  void clearParameterTie(std::string const &parameter);
  [[nodiscard]] bool updateParameterTie(std::string const &parameter,
                                        std::string const &tie);

  void removeParameterConstraint(std::string const &parameter);
  void updateParameterConstraint(std::string const &functionIndex,
                                 std::string const &parameter,
                                 std::string const &constraint);

private:
  bool setParameterTie(std::string const &parameter, std::string const &tie);

  [[nodiscard]] bool isValidParameterValue(std::string const &parameter,
                                           double value) const;
  [[nodiscard]] bool isValidStartX(double startX) const;
  [[nodiscard]] bool isValidEndX(double endX) const;
  std::pair<double, double> xLimits() const;
  std::pair<double, double>
  xLimits(Mantid::API::MatrixWorkspace_const_sptr const &workspace,
          WorkspaceIndex workspaceIndex) const;

  void removeFunctionFromIFunction(std::string const &function,
                                   Mantid::API::IFunction_sptr &iFunction);
  void
  removeFunctionFromComposite(std::string const &function,
                              Mantid::API::CompositeFunction_sptr &composite);
  void addFunctionToExisting(Mantid::API::IFunction_sptr const &function);

  void updateParameterConstraint(Mantid::API::CompositeFunction_sptr &composite,
                                 std::string const &functionIndex,
                                 std::string const &parameter,
                                 std::string const &constraint);

  std::string m_workspaceName;
  WorkspaceIndex m_workspaceIndex;
  double m_startX;
  double m_endX;
  Mantid::API::IFunction_sptr m_function;
};

} // namespace MantidWidgets
} // namespace MantidQt
