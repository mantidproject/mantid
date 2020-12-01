// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <string>
#include <vector>

#include <QString>

namespace MantidQt {
namespace MantidWidgets {

struct FitDomain {

  FitDomain(std::string const &prefix, std::string const &workspaceName,
            WorkspaceIndex workspaceIndex, double startX, double endX) {
    m_multiDomainFunctionPrefix = prefix;
    m_workspaceName = workspaceName;
    m_workspaceIndex = workspaceIndex;
    m_startX = startX;
    m_endX = endX;
  }

  std::string m_multiDomainFunctionPrefix;
  std::string m_workspaceName;
  WorkspaceIndex m_workspaceIndex;
  double m_startX;
  double m_endX;
};

class EXPORT_OPT_MANTIDQT_COMMON FitScriptGeneratorModel {
public:
  FitScriptGeneratorModel();
  ~FitScriptGeneratorModel();

  void removeWorkspaceDomain(std::string const &workspaceName,
                             WorkspaceIndex workspaceIndex);
  void addWorkspaceDomain(std::string const &workspaceName,
                          WorkspaceIndex workspaceIndex, double startX,
                          double endX);

private:
  void removeWorkspaceDomain(
      std::size_t const &removeIndex,
      std::vector<FitDomain>::const_iterator const &removeIter);
  void addWorkspaceDomain(std::string const &prefix,
                          std::string const &workspaceName,
                          WorkspaceIndex workspaceIndex, double startX,
                          double endX);

  bool hasWorkspaceDomain(std::string const &workspaceName,
                          WorkspaceIndex workspaceIndex);
  std::vector<FitDomain>::const_iterator
  findWorkspaceDomain(std::string const &workspaceName,
                      WorkspaceIndex workspaceIndex) const;

  void removeCompositeAtPrefix(std::string const &prefix);

  void addEmptyCompositeAtPrefix(std::string const &prefix);
  void addEmptyCompositeAtPrefix(std::string const &compositePrefix,
                                 std::size_t const &compositeIndex);

  void removeCompositeAtIndex(std::size_t const &compositeIndex);

  bool hasCompositeAtPrefix(std::string const &prefix) const;

  std::string nextAvailablePrefix() const;

  [[nodiscard]] inline std::size_t numberOfDomains() const noexcept {
    return m_fitDomains.size();
  }

  std::vector<FitDomain> m_fitDomains;
  Mantid::API::MultiDomainFunction_sptr m_function;
};

} // namespace MantidWidgets
} // namespace MantidQt
