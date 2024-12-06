// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "BayesFitting/StretchModel.h"
#include "BayesFitting/StretchView.h"

#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/IConfiguredAlgorithm.h"

#include <string>

using namespace MantidQt;
using namespace MantidQt::CustomInterfaces;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockStretchView : public IStretchView {
public:
  virtual ~MockStretchView() = default;

  MOCK_METHOD(void, subscribePresenter, (IStretchViewSubscriber * presenter), (override));
  MOCK_METHOD(void, loadSettings, (const QSettings &settings), (override));
  MOCK_METHOD(void, applySettings, ((std::map<std::string, QVariant> const &settings)), (override));
  MOCK_METHOD(void, validateUserInput, (IUserInputValidator * validator), (const, override));

  MOCK_METHOD(StretchRunData, getRunData, (), (const, override));
  MOCK_METHOD(CurrentPreviewData, getCurrentPreviewData, (), (const, override));
  MOCK_METHOD(std::string, getPlotType, (), (const, override));
  MOCK_METHOD(std::string, getPlotContour, (), (const, override));
  MOCK_METHOD(IRunView *, getRunWidget, (), (const, override));

  MOCK_METHOD(void, setupFitOptions, (), (override));
  MOCK_METHOD(void, setupPropertyBrowser, (), (override));
  MOCK_METHOD(void, setupPlotOptions, (), (override));

  MOCK_METHOD(void, setFileExtensionsByName, (bool filter), (override));
  MOCK_METHOD(void, setLoadHistory, (bool doLoadHistory), (override));

  MOCK_METHOD(void, resetPlotContourOptions, ((const std::vector<std::string> &contourNames)), (override));
  MOCK_METHOD(bool, displaySaveDirectoryMessage, (), (override));

  MOCK_METHOD(void, setPlotADSEnabled, (bool enabled), (override));
  MOCK_METHOD(void, setPlotResultEnabled, (bool enabled), (override));
  MOCK_METHOD(void, setPlotContourEnabled, (bool enabled), (override));
  MOCK_METHOD(void, setSaveResultEnabled, (bool enabled), (override));
  MOCK_METHOD(void, setButtonsEnabled, (bool enabled), (override));
  MOCK_METHOD(void, setPlotResultIsPlotting, (bool plotting), (override));
  MOCK_METHOD(void, setPlotContourIsPlotting, (bool plotting), (override));
};

class MockStretchModel : public IStretchModel {
public:
  virtual ~MockStretchModel() = default;

  MOCK_METHOD(MantidQt::API::IConfiguredAlgorithm_sptr, stretchAlgorithm,
              ((const StretchRunData &algParams), const std::string &fitWorkspaceName,
               const std::string &contourWorkspaceName, const bool useQuickBayes),
              (const, override));

  MOCK_METHOD(API::IConfiguredAlgorithm_sptr, setupSaveAlgorithm, (const std::string &wsName), (const, override));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
