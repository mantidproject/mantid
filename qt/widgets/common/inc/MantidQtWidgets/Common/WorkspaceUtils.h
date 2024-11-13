// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "DllOption.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "QPair"
#include <optional>
#include <string>

namespace MantidQt {
namespace MantidWidgets {
namespace WorkspaceUtils {

EXPORT_OPT_MANTIDQT_COMMON std::string getWorkspaceSuffix(const std::string &wsName);
EXPORT_OPT_MANTIDQT_COMMON std::string getWorkspaceBasename(const std::string &wsName);
EXPORT_OPT_MANTIDQT_COMMON std::unordered_map<std::string, size_t>
extractAxisLabels(const Mantid::API::MatrixWorkspace_const_sptr &workspace, const size_t &axisIndex);

EXPORT_OPT_MANTIDQT_COMMON std::string getEMode(const Mantid::API::MatrixWorkspace_sptr &ws);
EXPORT_OPT_MANTIDQT_COMMON std::optional<double> getEFixed(const Mantid::API::MatrixWorkspace_sptr &ws);

EXPORT_OPT_MANTIDQT_COMMON bool getResolutionRangeFromWs(const std::string &workspace, std::pair<double, double> &res);
EXPORT_OPT_MANTIDQT_COMMON bool getResolutionRangeFromWs(const Mantid::API::MatrixWorkspace_const_sptr &workspace,
                                                         std::pair<double, double> &res);

EXPORT_OPT_MANTIDQT_COMMON std::pair<double, double>
getXRangeFromWorkspace(const Mantid::API::MatrixWorkspace_const_sptr &workspace, double precision = 0.00001);
EXPORT_OPT_MANTIDQT_COMMON std::pair<double, double> getXRangeFromWorkspace(std::string const &workspaceName,
                                                                            double precision = 0.000001);

EXPORT_OPT_MANTIDQT_COMMON std::optional<std::size_t> maximumIndex(const Mantid::API::MatrixWorkspace_sptr &workspace);
EXPORT_OPT_MANTIDQT_COMMON std::string getIndexString(const std::string &workspaceName);

EXPORT_OPT_MANTIDQT_COMMON bool doesExistInADS(std::string const &workspaceName);
EXPORT_OPT_MANTIDQT_COMMON bool doAllWsExistInADS(std::vector<std::string> const &workspaceNames);

template <typename T = Mantid::API::MatrixWorkspace>
std::shared_ptr<T> getADSWorkspace(std::string const &workspaceName) {
  return Mantid::API::AnalysisDataService::Instance().retrieveWS<T>(workspaceName);
}

template EXPORT_OPT_MANTIDQT_COMMON std::shared_ptr<Mantid::API::MatrixWorkspace_sptr>
getADSWorkspace(std::string const &workspaceName);
template EXPORT_OPT_MANTIDQT_COMMON std::shared_ptr<Mantid::API::WorkspaceGroup_sptr>
getADSWorkspace(std::string const &workspaceName);
template EXPORT_OPT_MANTIDQT_COMMON std::shared_ptr<Mantid::API::ITableWorkspace_sptr>
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

EXPORT_OPT_MANTIDQT_COMMON std::vector<std::string> attachPrefix(std::vector<std::string> const &strings,
                                                                 std::string const &prefix);

EXPORT_OPT_MANTIDQT_COMMON std::string parseRunNumbers(std::vector<std::string> const &workspaceNames);
} // namespace WorkspaceUtils
} // namespace MantidWidgets
} // namespace MantidQt
