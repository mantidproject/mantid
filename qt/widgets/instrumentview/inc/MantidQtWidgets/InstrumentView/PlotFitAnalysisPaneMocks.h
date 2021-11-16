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
  explicit MockPlotFitAnalysisPanePresenter(IPlotFitAnalysisPaneView *view, PlotFitAnalysisPaneModel *model) {
    (void)model;
    (void)view;
    m_addFunc = 0;
  };
  ~MockPlotFitAnalysisPanePresenter(){};
  MOCK_METHOD0(destructor, void());
  MOCK_METHOD0(getView, IPlotFitAnalysisPaneView *());
  MOCK_METHOD0(getCurrentWS, std::string());
  MOCK_METHOD0(clearCurrentWS, void());
  MOCK_METHOD0(doFit, void());
  MOCK_METHOD0(updateEstimateAfterExtraction, void());
  MOCK_METHOD0(updateEstimate, void());
  MOCK_METHOD1(addSpectrum, void(const std::string &name));
  // at runtime the cast is done, so mock it ourselves
  void addFunction(IFunction_sptr func) override {
    (void)func;
    m_addFunc += 1;
  };
  int getAddCount() { return m_addFunc; };

private:
  int m_addFunc;
};

class MockPlotFitAnalysisPaneView : public MantidQt::MantidWidgets::IPlotFitAnalysisPaneView {
public:
  explicit MockPlotFitAnalysisPaneView(const double &start = 1., const double &end = 5., QWidget *parent = nullptr) {
    (void)start;
    (void)end;
    (void)parent;
  };
  ~MockPlotFitAnalysisPaneView(){};
  MOCK_METHOD1(observeFitButton, void(Observer *listener));
  MOCK_METHOD1(observeUpdateEstimateButton, void(Observer *listener));
  MOCK_METHOD0(getRange, std::pair<double, double>());
  MOCK_METHOD0(getFunction, Mantid::API::IFunction_sptr());
  MOCK_METHOD1(addSpectrum, void(const std::string &name));
  MOCK_METHOD1(addFitSpectrum, void(const std::string &name));
  MOCK_METHOD1(addFunction, void(Mantid::API::IFunction_sptr));
  MOCK_METHOD1(updateFunction, void(const Mantid::API::IFunction_sptr));
  MOCK_METHOD1(displayWarning, void(const std::string &message));

  MOCK_METHOD0(getQWidget, QWidget *());
  MOCK_METHOD2(setupPlotFitSplitter, void(const double &start, const double &end));
  MOCK_METHOD2(createFitPane, QWidget *(const double &start, const double &end));
};

class MockPlotFitAnalysisPaneModel : public MantidQt::MantidWidgets::PlotFitAnalysisPaneModel {
public:
  MockPlotFitAnalysisPaneModel() {
    m_fitCount = 0;
    m_estimateCount = 0;
  };

  ~MockPlotFitAnalysisPaneModel(){};
  IFunction_sptr doFit(const std::string &wsName, const std::pair<double, double> &range,
                       const IFunction_sptr func) override {
    m_fitCount += 1;
    (void)wsName;
    (void)range;
    return func;
  };

  IFunction_sptr calculateEstimate(const std::string &workspaceName, const std::pair<double, double> &range) override {
    (void)workspaceName;
    (void)range;
    m_estimateCount += 1;
    return FunctionFactory::Instance().createFunction("Gaussian");
  };

  bool hasEstimate() const override { return m_estimateCount > 0; };

  int getFitCount() { return m_fitCount; };
  int getEstimateCount() { return m_estimateCount; };

private:
  int m_fitCount;
  int m_estimateCount;
};
