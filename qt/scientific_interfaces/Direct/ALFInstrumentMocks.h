// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "ALFInstrumentModel.h"
#include "ALFInstrumentPresenter.h"
#include "ALFInstrumentView.h"
#include "MantidKernel/WarningSuppressions.h"

#include <QWidget>

#include <optional>
#include <string>

namespace MantidQt {

namespace MantidWidgets {
class InstrumentWidget;
class PlotFitAnalysisPanePresenter;
} // namespace MantidWidgets

namespace CustomInterfaces {

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockALFInstrumentView : public IALFInstrumentView {
public:
  MOCK_METHOD1(setUpInstrument, void(const std::string &fileName));

  MOCK_METHOD0(generateLoadWidget, QWidget *());
  MOCK_METHOD0(getInstrumentView, MantidWidgets::InstrumentWidget *());

  MOCK_METHOD1(subscribePresenter, void(IALFInstrumentPresenter *presenter));

  MOCK_METHOD0(getFile, std::optional<std::string>());
  MOCK_METHOD1(setRunQuietly, void(std::string const &runNumber));

  MOCK_METHOD1(warningBox, void(std::string const &message));
};

class MockALFInstrumentModel : public IALFInstrumentModel {
public:
  MOCK_METHOD1(loadAndTransform, std::optional<std::string>(std::string const &filename));

  MOCK_CONST_METHOD0(instrumentName, std::string());
  MOCK_CONST_METHOD0(loadedWsName, std::string());
  MOCK_CONST_METHOD0(extractedWsName, std::string());
  MOCK_CONST_METHOD0(runNumber, std::size_t());

  MOCK_METHOD0(extractSingleTube, std::optional<double>());
  MOCK_METHOD1(averageTube, std::optional<double>(std::size_t const numberOfTubes));

  MOCK_CONST_METHOD0(checkDataIsExtracted, bool());
};

class MockALFInstrumentPresenter : public IALFInstrumentPresenter {
public:
  MOCK_METHOD0(getLoadWidget, QWidget *());
  MOCK_METHOD0(getInstrumentView, MantidWidgets::InstrumentWidget *());

  MOCK_METHOD1(subscribeAnalysisPresenter, void(IALFAnalysisPresenter *presenter));

  MOCK_METHOD0(loadRunNumber, void());

  MOCK_METHOD0(extractSingleTube, void());
  MOCK_METHOD0(averageTube, void());

  MOCK_CONST_METHOD0(checkDataIsExtracted, bool());

  MOCK_CONST_METHOD0(extractedWsName, std::string());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

} // namespace CustomInterfaces
} // namespace MantidQt