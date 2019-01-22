// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECTFITOUTPUTOPTIONSPRESENTERTEST_H_
#define MANTIDQT_INDIRECTFITOUTPUTOPTIONSPRESENTERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "IIndirectFitOutputOptionsModel.h"
#include "IIndirectFitOutputOptionsView.h"
#include "IndirectFitOutputOptionsPresenter.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/WarningSuppressions.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces::IDA;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock the view
//class MockIndirectFitOutputOptionsView : public IIndirectFitOutputOptionsView {
//public:
//  /// Signals
//  void emitGroupWorkspaceChanged(std::string const &group) {
//    emit groupWorkspaceChanged(group);
//  }
//
//  void emitPlotClicked() { emit plotClicked(); }
//
//  void emitSaveClicked() { emit saveClicked(); }
//
//  /// Public Methods
//  MOCK_METHOD1(setGroupWorkspaceComboBoxVisible, void(bool visible));
//  MOCK_METHOD1(setWorkspaceComboBoxVisible, void(bool visible));
//
//  MOCK_METHOD0(clearPlotWorkspaces, void());
//  MOCK_METHOD0(clearPlotTypes, void());
//  MOCK_METHOD1(setAvailablePlotWorkspaces,
//               void(std::vector<std::string> const &workspaceNames));
//  MOCK_METHOD1(setAvailablePlotTypes,
//               void(std::vector<std::string> const &parameterNames));
//
//  MOCK_METHOD1(setPlotGroupWorkspaceIndex, void(int index));
//  MOCK_METHOD1(setPlotWorkspacesIndex, void(int index));
//  MOCK_METHOD1(setPlotTypeIndex, void(int index));
//
//  MOCK_CONST_METHOD0(getSelectedGroupWorkspace, std::string());
//  MOCK_CONST_METHOD0(getSelectedWorkspace, std::string());
//  MOCK_CONST_METHOD0(getSelectedPlotType, std::string());
//
//  MOCK_METHOD1(setPlotText, void(QString const &text));
//  MOCK_METHOD1(setSaveText, void(QString const &text));
//
//  MOCK_METHOD1(setPlotExtraOptionsEnabled, void(bool enable));
//  MOCK_METHOD1(setPlotEnabled, void(bool enable));
//  MOCK_METHOD1(setSaveEnabled, void(bool enable));
//
//  MOCK_METHOD1(displayWarning, void(std::string const &message));
//};
//
///// Mock object to mock the model
//class MockIndirectFitOutputOptionsModel
//    : public IIndirectFitOutputOptionsModel {
//  /// Public Methods
//  MOCK_METHOD1(setResultWorkspace, void(WorkspaceGroup_sptr groupWorkspace));
//  MOCK_METHOD1(setPDFWorkspace, void(WorkspaceGroup_sptr groupWorkspace));
//  MOCK_METHOD0(getResultWorkspace, WorkspaceGroup_sptr());
//  MOCK_METHOD0(getPDFWorkspace, WorkspaceGroup_sptr());
//
//  MOCK_METHOD0(removePDFWorkspace, void());
//
//  MOCK_CONST_METHOD0(isResultGroupPlottable, bool());
//  MOCK_CONST_METHOD0(isPDFGroupPlottable, bool());
//
//  MOCK_METHOD0(clearSpectraToPlot, void());
//  MOCK_CONST_METHOD0(getSpectraToPlot, std::vector<SpectrumToPlot>());
//
//  MOCK_METHOD1(plotResult, void(std::string const &plotType));
//  MOCK_METHOD2(plotPDF, void(std::string const &workspaceName,
//                             std::string const &plotType));
//
//  MOCK_CONST_METHOD0(saveResult, void());
//
//  MOCK_CONST_METHOD1(
//      getWorkspaceParameters,
//      std::vector<std::string>(std::string const &selectedGroup));
//  MOCK_CONST_METHOD0(getPDFWorkspaceNames, std::vector<std::string>());
//
//  MOCK_CONST_METHOD1(isResultGroupSelected,
//                     bool(std::string const &selectedGroup));
//};
//
//GNU_DIAG_ON_SUGGEST_OVERRIDE

class IndirectFitOutputOptionsPresenterTest : public CxxTest::TestSuite {
public:
  /// Needed to make sure everything is initialized
  IndirectFitOutputOptionsPresenterTest() { FrameworkManager::Instance(); }

  static IndirectFitOutputOptionsPresenterTest *createSuite() {
    return new IndirectFitOutputOptionsPresenterTest();
  }

  static void destroySuite(IndirectFitOutputOptionsPresenterTest *suite) {
    delete suite;
  }

  void setUp() override {}

  void test_test() {}
};
#endif
