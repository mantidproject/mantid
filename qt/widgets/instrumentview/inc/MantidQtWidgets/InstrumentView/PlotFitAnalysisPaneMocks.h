// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidAPI/NumericAxis.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidQtWidgets/Common/ObserverPattern.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneModel.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPanePresenter.h"
#include "MantidQtWidgets/InstrumentView/PlotFitAnalysisPaneView.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/WarningSuppressions.h"

#include <string>
#include <utility>

GNU_DIAG_OFF_SUGGEST_OVERRIDE

using namespace Mantid::API;
using Mantid::Geometry::Instrument;
using namespace MantidQt::MantidWidgets;

class MockPlotFitAnalysisPanePresenter : public MantidQt::MantidWidgets::IPlotFitAnalysisPanePresenter {
public:
  explicit MockPlotFitAnalysisPanePresenter(IPlotFitAnalysisPaneView *view, IPlotFitAnalysisPaneModel *model) {
    (void)model;
    (void)view;
  };
  ~MockPlotFitAnalysisPanePresenter(){};
  MOCK_METHOD0(destructor, void());
  MOCK_METHOD0(getView, IPlotFitAnalysisPaneView *());
  MOCK_METHOD0(getCurrentWS, std::string());
  MOCK_METHOD0(clearCurrentWS, void());
  MOCK_METHOD0(peakCentreEditingFinished, void());
  MOCK_METHOD0(fitClicked, void());
  MOCK_METHOD0(updateEstimateClicked, void());
  MOCK_METHOD1(addSpectrum, void(const std::string &name));
};

class MockPlotFitAnalysisPaneView : public MantidQt::MantidWidgets::IPlotFitAnalysisPaneView {
public:
  explicit MockPlotFitAnalysisPaneView(const double &start = 1., const double &end = 5., QWidget *parent = nullptr) {
    (void)start;
    (void)end;
    (void)parent;
  };
  ~MockPlotFitAnalysisPaneView(){};
  MOCK_METHOD1(observePeakCentreLineEdit, void(Observer *listener));
  MOCK_METHOD1(observeFitButton, void(Observer *listener));
  MOCK_METHOD1(observeUpdateEstimateButton, void(Observer *listener));
  MOCK_METHOD0(getRange, std::pair<double, double>());
  MOCK_METHOD1(addSpectrum, void(const std::string &name));
  MOCK_METHOD1(addFitSpectrum, void(const std::string &name));
  MOCK_METHOD1(displayWarning, void(const std::string &message));

  MOCK_METHOD0(getQWidget, QWidget *());
  MOCK_METHOD2(setupPlotFitSplitter, void(const double &start, const double &end));
  MOCK_METHOD2(createFitPane, QWidget *(const double &start, const double &end));

  MOCK_METHOD1(setPeakCentre, void(const double centre));
  MOCK_CONST_METHOD0(peakCentre, double());
  MOCK_METHOD1(setPeakCentreStatus, void(const std::string &status));
};

class MockPlotFitAnalysisPaneModel : public MantidQt::MantidWidgets::IPlotFitAnalysisPaneModel {
public:
  MOCK_METHOD2(doFit, void(const std::string &wsName, const std::pair<double, double> &range));
  MOCK_METHOD2(calculateEstimate, void(const std::string &workspaceName, const std::pair<double, double> &range));

  MOCK_METHOD1(setPeakCentre, void(const double centre));
  MOCK_CONST_METHOD0(peakCentre, double());

  MOCK_CONST_METHOD0(fitStatus, std::string());
};
