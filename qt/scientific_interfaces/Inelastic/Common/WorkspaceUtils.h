// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "../DllConfig.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "QPair"
#include <optional>
#include <string>

namespace MantidQt {
namespace CustomInterfaces {
namespace WorkspaceUtils {

MANTIDQT_INELASTIC_DLL std::string getWorkspaceSuffix(const std::string &wsName);
MANTIDQT_INELASTIC_DLL std::string getWorkspaceBasename(const std::string &wsName);
MANTIDQT_INELASTIC_DLL std::unordered_map<std::string, size_t>
extractAxisLabels(const Mantid::API::MatrixWorkspace_const_sptr &workspace, const size_t &axisIndex);

MANTIDQT_INELASTIC_DLL std::string getEMode(const Mantid::API::MatrixWorkspace_sptr &ws);
MANTIDQT_INELASTIC_DLL std::optional<double> getEFixed(const Mantid::API::MatrixWorkspace_sptr &ws);

MANTIDQT_INELASTIC_DLL bool getResolutionRangeFromWs(const std::string &workspace, QPair<double, double> &res);
MANTIDQT_INELASTIC_DLL bool getResolutionRangeFromWs(const Mantid::API::MatrixWorkspace_const_sptr &workspace,
                                                     QPair<double, double> &res);

MANTIDQT_INELASTIC_DLL QPair<double, double>
getXRangeFromWorkspace(const Mantid::API::MatrixWorkspace_const_sptr &workspace, double precision = 0.00001);
MANTIDQT_INELASTIC_DLL QPair<double, double> getXRangeFromWorkspace(std::string const &workspaceName,
                                                                    double precision = 0.000001);
MANTIDQT_INELASTIC_DLL bool doesExistInADS(std::string const &workspaceName);

template <typename T = Mantid::API::MatrixWorkspace>
std::shared_ptr<T> getADSWorkspace(std::string const &workspaceName) {
  return Mantid::API::AnalysisDataService::Instance().retrieveWS<T>(workspaceName);
}

template MANTIDQT_INELASTIC_DLL std::shared_ptr<Mantid::API::MatrixWorkspace_sptr>
getADSWorkspace(std::string const &workspaceName);
template MANTIDQT_INELASTIC_DLL std::shared_ptr<Mantid::API::WorkspaceGroup_sptr>
getADSWorkspace(std::string const &workspaceName);
template MANTIDQT_INELASTIC_DLL std::shared_ptr<Mantid::API::ITableWorkspace_sptr>
getADSWorkspace(std::string const &workspaceName);

template <typename Iterator, typename Functor>
std::vector<std::string> transformElements(Iterator const fromIter, Iterator const toIter, Functor const &functor) {
  std::vector<std::string> newVector;
  newVector.reserve(toIter - fromIter);
  std::transform(fromIter, toIter, std::back_inserter(newVector), functor);
  return newVector;
}

template <typename T, typename Predicate> void removeElementsIf(std::vector<T> &vector, Predicate const &filter) {
  auto const iter = std::remove_if(vector.begin(), vector.end(), filter);
  if (iter != vector.end())
    vector.erase(iter, vector.end());
}

MANTIDQT_INELASTIC_DLL std::vector<std::string> attachPrefix(std::vector<std::string> const &strings,
                                                             std::string const &prefix);

} // namespace WorkspaceUtils
} // namespace CustomInterfaces
} // namespace MantidQt
