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
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

/**
 * This class is used to store all data relating to a single domain to be
 * fitted. This includes the location of the domain (workspace Name & index),
 * the fit range (start and end X), and the function to be fitted over.
 */
class EXPORT_OPT_MANTIDQT_COMMON FitDomain {

public:
  FitDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex, double startX, double endX);

  void setWorkspaceName(std::string const &workspaceName);

  [[nodiscard]] std::string domainName() const;
  [[nodiscard]] std::string workspaceName() const noexcept { return m_workspaceName; }
  [[nodiscard]] WorkspaceIndex workspaceIndex() const noexcept { return m_workspaceIndex; }

  [[nodiscard]] bool setStartX(double startX);
  [[nodiscard]] bool setEndX(double startX);

  [[nodiscard]] double startX() const noexcept { return m_startX; }
  [[nodiscard]] double endX() const noexcept { return m_endX; }

  void setFunction(Mantid::API::IFunction_sptr const &function);
  [[nodiscard]] Mantid::API::IFunction_sptr getFunctionCopy() const;
  void removeFunction(std::string const &function);
  void addFunction(Mantid::API::IFunction_sptr const &function);

  void setParameterValue(std::string const &parameter, double newValue);
  [[nodiscard]] double getParameterValue(std::string const &parameter) const;

  void setParameterFixed(std::string const &parameter, bool fix) const;
  [[nodiscard]] bool isParameterFixed(std::string const &parameter) const;

  void setAttributeValue(std::string const &attribute, Mantid::API::IFunction::Attribute newValue);
  [[nodiscard]] Mantid::API::IFunction::Attribute getAttributeValue(std::string const &attribute) const;

  [[nodiscard]] bool hasParameter(std::string const &parameter) const;
  [[nodiscard]] bool isParameterActive(std::string const &parameter) const;
  [[nodiscard]] std::string getParameterTie(std::string const &parameter) const;
  [[nodiscard]] std::string getParameterConstraint(std::string const &parameter) const;

  void clearParameterTie(std::string const &parameter);
  [[nodiscard]] bool updateParameterTie(std::string const &parameter, std::string const &tie);

  void removeParameterConstraint(std::string const &parameter);
  void updateParameterConstraint(std::string const &functionIndex, std::string const &parameter,
                                 std::string const &constraint);

  [[nodiscard]] std::vector<std::string> getParametersTiedTo(std::string const &parameter) const;

  [[nodiscard]] bool isParameterValueWithinConstraints(std::string const &parameter, double value) const;

private:
  [[nodiscard]] bool setParameterTie(std::string const &parameter, std::string const &tie);

  [[nodiscard]] double getTieValue(std::string const &tie) const;

  [[nodiscard]] bool isValidParameterTie(std::string const &parameter, std::string const &tie) const;
  [[nodiscard]] bool isValidParameterConstraint(std::string const &parameter, std::string const &constraint) const;

  [[nodiscard]] bool isValidStartX(double startX) const;
  [[nodiscard]] bool isValidEndX(double endX) const;
  [[nodiscard]] std::pair<double, double> xLimits() const;
  [[nodiscard]] std::pair<double, double> xLimits(Mantid::API::MatrixWorkspace_const_sptr const &workspace,
                                                  WorkspaceIndex workspaceIndex) const;

  void removeFunctionFromIFunction(std::string const &function, Mantid::API::IFunction_sptr &iFunction);
  void removeFunctionFromComposite(std::string const &function, Mantid::API::CompositeFunction_sptr &composite);
  void addFunctionToExisting(Mantid::API::IFunction_sptr const &function);

  void updateParameterConstraint(Mantid::API::CompositeFunction_sptr &composite, std::string const &functionIndex,
                                 std::string const &parameter, std::string const &constraint);

  void appendParametersTiedTo(std::vector<std::string> &tiedParameters, std::string const &parameter,
                              std::size_t const &parameterIndex) const;

  void removeInvalidatedTies();

  std::string m_workspaceName;
  WorkspaceIndex m_workspaceIndex;
  double m_startX;
  double m_endX;
  Mantid::API::IFunction_sptr m_function;
};

} // namespace MantidWidgets
} // namespace MantidQt
