// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidHistogramData/BinEdges.h"
#include <string>

#include <boost/variant.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>

namespace Mantid {
namespace IndirectFitDataCreationHelper {
/// Commonly used constant variables
int const START_X_COLUMN(2);
int const END_X_COLUMN(3);
int const EXCLUDE_REGION_COLUMN(4);

/// Functions used in the creation of workspaces
Mantid::API::MatrixWorkspace_sptr createWorkspace(int const &numberOfSpectra, int const &numberOfBins = 10);
Mantid::API::MatrixWorkspace_sptr createInstrumentWorkspace(int const &xLength, int const &yLength);
Mantid::API::MatrixWorkspace_sptr createWorkspaceWithTextAxis(int const &numberOfSpectra,
                                                              std::vector<std::string> const &labels,
                                                              int const &numberOfBins = 10);
Mantid::API::MatrixWorkspace_sptr createWorkspaceWithBinValues(int const &numberOfSpectra,
                                                               std::vector<double> const &labels,
                                                               int const &numberOfBins = 10);
Mantid::API::WorkspaceGroup_sptr createGroupWorkspace(std::size_t const &numberOfWorkspaces, int const &numberOfSpectra,
                                                      int const &numberOfBins = 10);
Mantid::API::WorkspaceGroup_sptr createGroupWorkspaceWithTextAxes(std::size_t const &numberOfWorkspaces,
                                                                  std::vector<std::string> const &labels,
                                                                  int const &numberOfSpectra,
                                                                  int const &numberOfBins = 10);

Mantid::API::TextAxis *getTextAxis(int const &numberOfSpectra, std::vector<std::string> const &labels);
Mantid::API::NumericAxis *getNumericAxis(int const &numberOfLabels, std::vector<double> const &values);

Mantid::API::MatrixWorkspace_sptr setWorkspaceEFixed(Mantid::API::MatrixWorkspace_sptr workspace, int const &xLength);
Mantid::API::MatrixWorkspace_sptr setWorkspaceBinEdges(Mantid::API::MatrixWorkspace_sptr workspace, int const &yLength,
                                                       Mantid::HistogramData::BinEdges const &binEdges);
Mantid::API::MatrixWorkspace_sptr setWorkspaceBinEdges(Mantid::API::MatrixWorkspace_sptr workspace, int const &xLength,
                                                       int const &yLength);
Mantid::API::MatrixWorkspace_sptr setWorkspaceProperties(Mantid::API::MatrixWorkspace_sptr workspace,
                                                         int const &xLength, int const &yLength);
Mantid::API::MatrixWorkspace_sptr createWorkspaceWithInstrument(int const &xLength, int const &yLength);
Mantid::API::MatrixWorkspace_sptr createWorkspaceWithInelasticInstrument(int const &yLength);
Mantid::API::MatrixWorkspace_sptr
createWorkspaceWithIndirectInstrumentAndParameters(std::string const &analyser = "graphite");

/// Simple struct used to access features of the ADS
/// No destructor so ensure you tearDown the ADS
struct SetUpADSWithWorkspace {

  template <typename T> SetUpADSWithWorkspace(std::string const &inputWSName, T const &workspace) {
    Mantid::API::AnalysisDataService::Instance().addOrReplace(inputWSName, workspace);
  }

  template <typename T> void addOrReplace(std::string const &workspaceName, T const &workspace) {
    Mantid::API::AnalysisDataService::Instance().addOrReplace(workspaceName, workspace);
  }

  bool doesExist(std::string const &workspaceName) {
    return Mantid::API::AnalysisDataService::Instance().doesExist(workspaceName);
  }

  Mantid::API::MatrixWorkspace_sptr retrieveWorkspace(std::string const &workspaceName) {
    return std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve(workspaceName));
  }
};

/// This is used to compare Spectra which is implemented as a boost::variant
struct AreSpectraEqual : public boost::static_visitor<bool> {

  template <typename T, typename U> bool operator()(const T &, const U &) const {
    return false; // cannot compare different types
  }

  template <typename T> bool operator()(const T &lhs, const T &rhs) const { return lhs == rhs; }
};

} // namespace IndirectFitDataCreationHelper
} // namespace Mantid
