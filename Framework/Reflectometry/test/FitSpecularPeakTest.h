// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidReflectometry/FitSpecularPeak.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/EmptyValues.h"

#include <cxxtest/TestSuite.h>

#include <cmath>
#include <limits>
#include <vector>

using namespace Mantid;

class FitSpecularPeakTest : public CxxTest::TestSuite {
public:
  static FitSpecularPeakTest *createSuite() { return new FitSpecularPeakTest(); }
  static void destroySuite(FitSpecularPeakTest *suite) { delete suite; }

  FitSpecularPeakTest() { API::FrameworkManager::Instance(); }

  void test_init() {
    Reflectometry::FitSpecularPeak algorithm;
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize())
    TS_ASSERT(algorithm.isInitialized())
    bool const useFittedCentreOnFailure = algorithm.getProperty("UseFittedPeakCentreOnFailure");
    bool const acceptChangesInFunction = algorithm.getProperty("AcceptChangesInFunctionTooSmall");
    bool const acceptChangesInParameters = algorithm.getProperty("AcceptChangesInParameterTooSmall");
    TS_ASSERT(!useFittedCentreOnFailure)
    TS_ASSERT(acceptChangesInFunction)
    TS_ASSERT(acceptChangesInParameters)
  }

  void test_fit_status_acceptance_is_configurable() {
    auto const &success = API::MinimizerStatus::SUCCESS;
    auto const &changesInFunction = API::MinimizerStatus::CHANGES_IN_FUNCTION_TOO_SMALL;
    auto const &changesInParameters = API::MinimizerStatus::CHANGES_IN_PARAMETER_TOO_SMALL;

    TS_ASSERT(Reflectometry::FitSpecularPeak::fitStatusIsAccepted(success, false, false))
    TS_ASSERT(Reflectometry::FitSpecularPeak::fitStatusIsAccepted(changesInFunction, true, false))
    TS_ASSERT(!Reflectometry::FitSpecularPeak::fitStatusIsAccepted(changesInFunction, false, true))
    TS_ASSERT(Reflectometry::FitSpecularPeak::fitStatusIsAccepted(changesInParameters, false, true))
    TS_ASSERT(!Reflectometry::FitSpecularPeak::fitStatusIsAccepted(changesInParameters, true, false))
    TS_ASSERT(!Reflectometry::FitSpecularPeak::fitStatusIsAccepted("Failed to converge", true, true))
  }

  void test_fits_peak_and_returns_absolute_workspace_index() {
    auto workspace = gaussianWorkspace(80, 20, 43.25, 3.5, 12.0, 2.0);
    auto algorithm = configuredAlgorithm(workspace);
    algorithm->setProperty("StartWorkspaceIndex", 20);
    algorithm->setProperty("EndWorkspaceIndex", 70);
    algorithm->setPropertyValue("OutputProfileWorkspace", "unused_profile");
    algorithm->setPropertyValue("OutputFitWorkspace", "unused_fit");

    TS_ASSERT_THROWS_NOTHING(algorithm->execute())
    TS_ASSERT(algorithm->isExecuted())
    double const peakCentre = algorithm->getProperty("PeakCentre");
    double const peakCentreError = algorithm->getProperty("PeakCentreError");
    TS_ASSERT_DELTA(peakCentre, 43.25, 0.05)
    TS_ASSERT(std::isfinite(peakCentreError))
    TS_ASSERT_EQUALS(algorithm->getPropertyValue("OutputStatus"), "success")
    API::MatrixWorkspace_sptr profile = algorithm->getProperty("OutputProfileWorkspace");
    API::MatrixWorkspace_sptr fit = algorithm->getProperty("OutputFitWorkspace");
    TS_ASSERT(profile)
    TS_ASSERT(fit)
    if (!profile || !fit) {
      return;
    }
    TS_ASSERT_EQUALS(profile->getNumberHistograms(), 1)
    TS_ASSERT_EQUALS(profile->x(0).front(), 20.0)
    TS_ASSERT_EQUALS(profile->x(0).back(), 70.0)
    TS_ASSERT_EQUALS(fit->getNumberHistograms(), 3)
  }

  void test_supports_flat_background_model() {
    auto workspace = gaussianWorkspace(60, 20, 28.4, 3.0, 10.0, 3.0);
    auto algorithm = configuredAlgorithm(workspace);
    algorithm->setPropertyValue("BackgroundType", "Flat");

    TS_ASSERT_THROWS_NOTHING(algorithm->execute())
    double const peakCentre = algorithm->getProperty("PeakCentre");
    double const peakCentreError = algorithm->getProperty("PeakCentreError");
    TS_ASSERT_DELTA(peakCentre, 28.4, 0.1)
    TS_ASSERT(std::isfinite(peakCentreError))
    TS_ASSERT_LESS_THAN(0.0, peakCentreError)
    TS_ASSERT_EQUALS(algorithm->getPropertyValue("OutputStatus"), "success")
  }

  void test_fit_failure_returns_initial_peak_centre_without_fit_output_or_error() {
    auto workspace = gaussianWorkspace(3, 10, 1.0, 0.15, 10.0, 1.0);
    auto algorithm = configuredAlgorithm(workspace);
    algorithm->setPropertyValue("OutputFitWorkspace", "unused_fit");

    TS_ASSERT_THROWS_NOTHING(algorithm->execute())
    double const peakCentre = algorithm->getProperty("PeakCentre");
    double const peakCentreError = algorithm->getProperty("PeakCentreError");
    API::MatrixWorkspace_sptr fit = algorithm->getProperty("OutputFitWorkspace");
    TS_ASSERT_EQUALS(peakCentre, 1.0)
    TS_ASSERT_EQUALS(peakCentreError, EMPTY_DBL())
    TS_ASSERT_EQUALS(algorithm->getPropertyValue("OutputStatus"), "Fit failed; using initial peak centre")
    TS_ASSERT(!fit)
  }

  void test_unsuccessful_fit_returns_initial_peak_centre_by_default() {
    auto algorithm = configuredAlgorithm(nonConvergingWorkspace());

    TS_ASSERT_THROWS_NOTHING(algorithm->execute())
    double const peakCentre = algorithm->getProperty("PeakCentre");
    TS_ASSERT_EQUALS(peakCentre, 3.0)
    TS_ASSERT_EQUALS(algorithm->getPropertyValue("OutputStatus"), "Fit failed; using initial peak centre")
  }

  void test_unsuccessful_fit_returns_fitted_peak_centre_when_requested() {
    auto algorithm = configuredAlgorithm(nonConvergingWorkspace());
    algorithm->setProperty("UseFittedPeakCentreOnFailure", true);
    algorithm->setPropertyValue("OutputFitWorkspace", "unused_fit");

    TS_ASSERT_THROWS_NOTHING(algorithm->execute())
    double const peakCentre = algorithm->getProperty("PeakCentre");
    API::MatrixWorkspace_sptr fit = algorithm->getProperty("OutputFitWorkspace");
    TS_ASSERT_DELTA(peakCentre, 2.687066, 1e-6)
    TS_ASSERT_EQUALS(algorithm->getPropertyValue("OutputStatus"), "Failed to converge after 500 iterations.")
    TS_ASSERT(fit)
  }

  void test_fit_exception_returns_initial_peak_centre_when_fitted_centre_on_failure_is_requested() {
    auto workspace = gaussianWorkspace(3, 10, 1.0, 0.15, 10.0, 1.0);
    auto algorithm = configuredAlgorithm(workspace);
    algorithm->setProperty("UseFittedPeakCentreOnFailure", true);

    TS_ASSERT_THROWS_NOTHING(algorithm->execute())
    double const peakCentre = algorithm->getProperty("PeakCentre");
    TS_ASSERT_EQUALS(peakCentre, 1.0)
    TS_ASSERT_EQUALS(algorithm->getPropertyValue("OutputStatus"), "Fit failed; using initial peak centre")
  }

  void test_out_of_range_fitted_centre_is_not_used_when_fitted_centre_on_failure_is_requested() {
    auto workspace = workspaceFromProfile({-4.238311484671598, 18.674709133562637, 16.99615560143591,
                                           -10.010405371038509, 17.468765109562575, 17.946402536620198});
    auto algorithm = configuredAlgorithm(workspace);
    algorithm->setProperty("UseFittedPeakCentreOnFailure", true);

    TS_ASSERT_THROWS_NOTHING(algorithm->execute())
    double const peakCentre = algorithm->getProperty("PeakCentre");
    TS_ASSERT_EQUALS(peakCentre, 1.0)
    TS_ASSERT_EQUALS(algorithm->getPropertyValue("OutputStatus"), "Fit failed; using initial peak centre")
  }

  void test_ragged_integration_limits_are_normalized_before_transposing() {
    auto workspace = gaussianWorkspace(40, 20, 21.5, 2.5, 9.0, 1.0);
    for (size_t workspaceIndex = 0; workspaceIndex < workspace->getNumberHistograms(); ++workspaceIndex) {
      auto &x = workspace->mutableX(workspaceIndex);
      auto const offset = 0.01 * static_cast<double>(workspaceIndex);
      std::transform(x.cbegin(), x.cend(), x.begin(), [offset](double const value) { return value + offset; });
    }
    auto algorithm = configuredAlgorithm(workspace);

    TS_ASSERT_THROWS_NOTHING(algorithm->execute())
    double const peakCentre = algorithm->getProperty("PeakCentre");
    TS_ASSERT_DELTA(peakCentre, 21.5, 0.1)
  }

  void test_range_limits_select_the_peak_from_the_requested_x_region() {
    auto workspace = gaussianWorkspace(60, 40, 31.5, 3.0, 10.0, 1.0, 5.0, 8.0);
    addPeak(*workspace, 45.0, 2.5, 14.0, 12.0, 16.0);
    auto algorithm = configuredAlgorithm(workspace);
    algorithm->setProperty("RangeLower", 5.0);
    algorithm->setProperty("RangeUpper", 8.0);

    TS_ASSERT_THROWS_NOTHING(algorithm->execute())
    double const peakCentre = algorithm->getProperty("PeakCentre");
    TS_ASSERT_DELTA(peakCentre, 31.5, 0.1)
  }

  void test_throws_when_an_initial_peak_cannot_be_found() {
    auto workspace = gaussianWorkspace(20, 10, 10.0, 2.0, 0.0, 0.0);
    for (size_t workspaceIndex = 0; workspaceIndex < workspace->getNumberHistograms(); ++workspaceIndex) {
      std::fill(workspace->mutableY(workspaceIndex).begin(), workspace->mutableY(workspaceIndex).end(),
                std::numeric_limits<double>::quiet_NaN());
    }
    auto algorithm = configuredAlgorithm(workspace);

    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error const &)
    TS_ASSERT(!algorithm->isExecuted())
  }

  void test_invalid_workspace_index_range_is_rejected() {
    auto workspace = gaussianWorkspace(20, 10, 10.0, 2.0, 8.0, 1.0);
    auto algorithm = configuredAlgorithm(workspace);
    algorithm->setProperty("StartWorkspaceIndex", 12);
    algorithm->setProperty("EndWorkspaceIndex", 8);

    TS_ASSERT_THROWS(algorithm->execute(), std::runtime_error const &)
  }

private:
  static std::unique_ptr<Reflectometry::FitSpecularPeak>
  configuredAlgorithm(const API::MatrixWorkspace_sptr &workspace) {
    auto algorithm = std::make_unique<Reflectometry::FitSpecularPeak>();
    algorithm->initialize();
    algorithm->setChild(true);
    algorithm->setRethrows(true);
    algorithm->setProperty("InputWorkspace", workspace);
    return algorithm;
  }

  static API::MatrixWorkspace_sptr gaussianWorkspace(size_t const numberOfSpectra, size_t const numberOfBins,
                                                     double const centre, double const sigma, double const height,
                                                     double const background, double const peakXMin = 0.0,
                                                     double const peakXMax = 20.0) {
    HistogramData::BinEdges const edges{numberOfBins + 1,
                                        HistogramData::LinearGenerator(0.0, 20.0 / static_cast<double>(numberOfBins))};
    HistogramData::Counts const counts(numberOfBins, 0.0);
    auto workspace =
        DataObjects::create<DataObjects::Workspace2D>(numberOfSpectra, HistogramData::Histogram(edges, counts));
    for (size_t workspaceIndex = 0; workspaceIndex < numberOfSpectra; ++workspaceIndex) {
      auto const offset = (static_cast<double>(workspaceIndex) - centre) / sigma;
      for (size_t bin = 0; bin < numberOfBins; ++bin) {
        auto const x = workspace->points(workspaceIndex)[bin];
        auto const peak = x >= peakXMin && x <= peakXMax ? height * std::exp(-0.5 * offset * offset) : 0.0;
        workspace->mutableY(workspaceIndex)[bin] = background + peak;
        workspace->mutableE(workspaceIndex)[bin] = std::sqrt(background + peak + 1.0);
      }
    }
    return workspace;
  }

  static API::MatrixWorkspace_sptr workspaceFromProfile(std::vector<double> const &profile) {
    HistogramData::BinEdges const edges{0.0, 1.0};
    HistogramData::Counts const counts(1, 0.0);
    auto workspace =
        DataObjects::create<DataObjects::Workspace2D>(profile.size(), HistogramData::Histogram(edges, counts));
    for (size_t index = 0; index < profile.size(); ++index) {
      workspace->mutableY(index)[0] = profile[index];
      workspace->mutableE(index)[0] = 1.0;
    }
    return workspace;
  }

  static API::MatrixWorkspace_sptr nonConvergingWorkspace() {
    return workspaceFromProfile({-8.062627323198912, 0.09656098269096347, -1.1052904296664998, 60.17173237063975,
                                 0.5011382967410345, 5.544182065326});
  }

  static void addPeak(API::MatrixWorkspace &workspace, double const centre, double const sigma, double const height,
                      double const peakXMin, double const peakXMax) {
    for (size_t workspaceIndex = 0; workspaceIndex < workspace.getNumberHistograms(); ++workspaceIndex) {
      auto const offset = (static_cast<double>(workspaceIndex) - centre) / sigma;
      for (size_t bin = 0; bin < workspace.y(workspaceIndex).size(); ++bin) {
        auto const x = workspace.points(workspaceIndex)[bin];
        if (x >= peakXMin && x <= peakXMax) {
          workspace.mutableY(workspaceIndex)[bin] += height * std::exp(-0.5 * offset * offset);
        }
      }
    }
  }
};
