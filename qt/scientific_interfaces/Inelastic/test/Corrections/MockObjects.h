// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "Corrections/ContainerSubtractionModel.h"
#include "Corrections/ContainerSubtractionView.h"

#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/IConfiguredAlgorithm.h"

#include <optional>
#include <string>
#include <utility>

using namespace MantidQt;
using namespace MantidQt::CustomInterfaces;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockContainerSubtractionView : public IContainerSubtractionView {
public:
  virtual ~MockContainerSubtractionView() = default;
  MOCK_METHOD1(subscribe, void(IContainerSubtractionPresenter *presenter));
  MOCK_METHOD1(validate, void(IUserInputValidator *validator));
  MOCK_METHOD1(setSampleWSSuffixes, void(const QStringList &suffixes));
  MOCK_METHOD1(setSampleFBSuffixes, void(const QStringList &suffixes));
  MOCK_METHOD1(setCanWSSuffixes, void(const QStringList &suffixes));
  MOCK_METHOD1(setCanFBSuffixes, void(const QStringList &suffixes));
  MOCK_METHOD1(loadSettings, void(const QSettings &settings));
  MOCK_METHOD1(setLoadHistory, void(bool doLoadHistory));
  MOCK_METHOD1(enableSaveButton, void(bool enable));
  MOCK_CONST_METHOD0(getShift, double());
  MOCK_CONST_METHOD0(getScale, double());
  MOCK_CONST_METHOD0(getSpNo, int());
  MOCK_CONST_METHOD0(getSpMax, int());
  MOCK_METHOD1(setSpMax, void(int max));
  MOCK_CONST_METHOD0(clearPlot, void());
  MOCK_METHOD3(plotSpectrum, void(const CSCurves &curveName, const MatrixWorkspace_sptr &ws, size_t specNo));
  MOCK_METHOD0(requestRebinToSample, bool());
  MOCK_CONST_METHOD0(getPlotOptions, IOutputPlotOptionsView *());
  MOCK_CONST_METHOD0(getRunView, IRunView *());
  MOCK_CONST_METHOD0(getOutputNameView, IOutputNameView *());
};

class MockContainerSubtractionModel : public IContainerSubtractionModel {
public:
  virtual ~MockContainerSubtractionModel() = default;
  MOCK_METHOD1(setSampleWS, void(const std::string &name));
  MOCK_METHOD1(setCanWS, void(const std::string &name));
  MOCK_METHOD1(setSubtractedWS, void(const std::string &name));
  MOCK_METHOD0(removeSubtractedWS, void());

  MOCK_CONST_METHOD0(sampleWS, const MatrixWorkspace_sptr &());
  MOCK_CONST_METHOD0(canWS, const MatrixWorkspace_sptr &());
  MOCK_CONST_METHOD0(subtractedWS, const MatrixWorkspace_sptr &());
  MOCK_CONST_METHOD0(modCanWS, const MatrixWorkspace_sptr &());

  MOCK_METHOD3(prepareSubtraction, API::IConfiguredAlgorithm_sptr(double shiftX, double scale, bool doRebin));
  MOCK_METHOD2(updateContainer, void(double shiftX, double scale));

  MOCK_CONST_METHOD0(getAllValidWorkspaceNames, std::vector<std::string>());
  MOCK_METHOD1(addShiftLog, void(double shiftX));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
