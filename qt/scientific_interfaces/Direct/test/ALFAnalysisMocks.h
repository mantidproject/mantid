// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "ALFAnalysisModel.h"
#include "ALFAnalysisPresenter.h"
#include "ALFAnalysisView.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include "MantidKernel/WarningSuppressions.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

class MockALFAnalysisPresenter : public IALFAnalysisPresenter {
public:
  MOCK_METHOD0(getView, QWidget *());

  MOCK_METHOD2(setExtractedWorkspace,
               void(Mantid::API::MatrixWorkspace_sptr const &workspace, std::vector<double> const &twoThetas));

  MOCK_METHOD0(notifyPeakCentreEditingFinished, void());
  MOCK_METHOD0(notifyFitClicked, void());
  MOCK_METHOD0(notifyUpdateEstimateClicked, void());

  MOCK_CONST_METHOD0(numberOfTubes, std::size_t());

  MOCK_METHOD0(clear, void());
};

class MockALFAnalysisView : public IALFAnalysisView {
public:
  MOCK_METHOD0(getView, QWidget *());

  MOCK_METHOD1(subscribePresenter, void(IALFAnalysisPresenter *presenter));

  MOCK_CONST_METHOD0(getRange, std::pair<double, double>());

  MOCK_METHOD1(addSpectrum, void(Mantid::API::MatrixWorkspace_sptr const &workspace));
  MOCK_METHOD1(addFitSpectrum, void(Mantid::API::MatrixWorkspace_sptr const &workspace));

  MOCK_METHOD1(setPeakCentre, void(double const centre));
  MOCK_CONST_METHOD0(peakCentre, double());

  MOCK_METHOD1(setPeakCentreStatus, void(std::string const &status));

  MOCK_METHOD2(setAverageTwoTheta, void(std::optional<double> average, std::vector<double> const &all));

  MOCK_METHOD1(displayWarning, void(std::string const &message));
};

class MockALFAnalysisModel : public IALFAnalysisModel {
public:
  MOCK_METHOD0(clear, void());

  MOCK_METHOD2(setExtractedWorkspace,
               void(Mantid::API::MatrixWorkspace_sptr const &workspace, std::vector<double> const &twoThetas));
  MOCK_CONST_METHOD0(extractedWorkspace, Mantid::API::MatrixWorkspace_sptr());
  MOCK_CONST_METHOD0(isDataExtracted, bool());

  MOCK_METHOD1(doFit, Mantid::API::MatrixWorkspace_sptr(std::pair<double, double> const &range));
  MOCK_METHOD1(calculateEstimate, void(std::pair<double, double> const &range));

  MOCK_METHOD1(setPeakCentre, void(double const centre));
  MOCK_CONST_METHOD0(peakCentre, double());

  MOCK_CONST_METHOD0(fitStatus, std::string());

  MOCK_CONST_METHOD0(numberOfTubes, std::size_t());

  MOCK_CONST_METHOD0(averageTwoTheta, std::optional<double>());
  MOCK_CONST_METHOD0(allTwoThetas, std::vector<double>());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
