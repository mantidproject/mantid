// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorModel.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorView.h"
#include "MantidQtWidgets/Common/FittingGlobals.h"
#include "MantidQtWidgets/Common/FittingMode.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorModel.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorPresenter.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorView.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <string>
#include <vector>

#include <gmock/gmock.h>

using namespace MantidQt::MantidWidgets;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockFitScriptGeneratorPresenter : public IFitScriptGeneratorPresenter {

public:
  MockFitScriptGeneratorPresenter(FitScriptGeneratorView *view) {
    m_view = view;
    m_view->subscribePresenter(this);
  }

  MockFitScriptGeneratorPresenter(FitScriptGeneratorModel *model) {
    m_model = model;
    m_model->subscribePresenter(this);
  }

  void notifyPresenter(ViewEvent const &ev, std::string const &arg1 = "",
                       std::string const &arg2 = "") override {
    notifyPresenterImpl(ev, arg1, arg2);
  }

  MOCK_METHOD3(notifyPresenterImpl,
               void(ViewEvent const &ev, std::string const &arg1,
                    std::string const &arg2));
  MOCK_METHOD2(notifyPresenter,
               void(ViewEvent const &ev, std::vector<std::string> const &vec));
  MOCK_METHOD2(notifyPresenter,
               void(ViewEvent const &ev, FittingMode fittingMode));

  MOCK_METHOD0(openFitScriptGenerator, void());

  MOCK_METHOD1(setGlobalTies, void(std::vector<GlobalTie> const &globalTies));
  MOCK_METHOD1(setGlobalParameters,
               void(std::vector<GlobalParameter> const &globalParameters));

private:
  FitScriptGeneratorView *m_view;
  FitScriptGeneratorModel *m_model;
};

class MockFitScriptGeneratorView : public IFitScriptGeneratorView {

public:
  MOCK_METHOD1(subscribePresenter,
               void(IFitScriptGeneratorPresenter *presenter));

  MOCK_CONST_METHOD1(workspaceName, std::string(FitDomainIndex index));
  MOCK_CONST_METHOD1(workspaceIndex, MantidQt::MantidWidgets::WorkspaceIndex(
                                         FitDomainIndex index));
  MOCK_CONST_METHOD1(startX, double(FitDomainIndex index));
  MOCK_CONST_METHOD1(endX, double(FitDomainIndex index));

  MOCK_CONST_METHOD0(allRows, std::vector<FitDomainIndex>());
  MOCK_CONST_METHOD0(selectedRows, std::vector<FitDomainIndex>());

  MOCK_CONST_METHOD1(parameterValue, double(std::string const &parameter));
  MOCK_CONST_METHOD1(attributeValue, Mantid::API::IFunction::Attribute(
                                         std::string const &attribute));

  MOCK_METHOD2(removeWorkspaceDomain,
               void(std::string const &workspaceName,
                    MantidQt::MantidWidgets::WorkspaceIndex workspaceIndex));
  MOCK_METHOD4(addWorkspaceDomain,
               void(std::string const &workspaceName,
                    MantidQt::MantidWidgets::WorkspaceIndex workspaceIndex,
                    double startX, double endX));

  MOCK_METHOD0(openAddWorkspaceDialog, bool());
  MOCK_METHOD0(getDialogWorkspaces,
               std::vector<Mantid::API::MatrixWorkspace_const_sptr>());
  MOCK_CONST_METHOD0(getDialogWorkspaceIndices,
                     std::vector<MantidQt::MantidWidgets::WorkspaceIndex>());

  MOCK_METHOD0(resetSelection, void());

  MOCK_CONST_METHOD0(isAddRemoveFunctionForAllChecked, bool());

  MOCK_METHOD0(clearFunction, void());
  MOCK_CONST_METHOD1(setFunction,
                     void(Mantid::API::IFunction_sptr const &function));

  MOCK_METHOD1(setSimultaneousMode, void(bool simultaneousMode));

  MOCK_METHOD1(setGlobalTies, void(std::vector<GlobalTie> const &globalTies));
  MOCK_METHOD1(setGlobalParameters,
               void(std::vector<GlobalParameter> const &globalParameter));

  MOCK_METHOD1(displayWarning, void(std::string const &message));

  MOCK_CONST_METHOD0(tableWidget, FitScriptGeneratorDataTable *());
  MOCK_CONST_METHOD0(removeButton, QPushButton *());
  MOCK_CONST_METHOD0(addWorkspaceButton, QPushButton *());
  MOCK_CONST_METHOD0(addWorkspaceDialog, AddWorkspaceDialog *());
};

class MockFitScriptGeneratorModel : public IFitScriptGeneratorModel {

public:
  MOCK_METHOD1(subscribePresenter,
               void(IFitScriptGeneratorPresenter *presenter));

  MOCK_METHOD2(removeWorkspaceDomain,
               void(std::string const &workspaceName,
                    MantidQt::MantidWidgets::WorkspaceIndex workspaceIndex));
  MOCK_METHOD4(addWorkspaceDomain,
               void(std::string const &workspaceName,
                    MantidQt::MantidWidgets::WorkspaceIndex workspaceIndex,
                    double startX, double endX));
  MOCK_CONST_METHOD2(
      hasWorkspaceDomain,
      bool(std::string const &workspaceName,
           MantidQt::MantidWidgets::WorkspaceIndex workspaceIndex));

  MOCK_METHOD3(updateStartX,
               bool(std::string const &workspaceName,
                    MantidQt::MantidWidgets::WorkspaceIndex workspaceIndex,
                    double startX));
  MOCK_METHOD3(updateEndX,
               bool(std::string const &workspaceName,
                    MantidQt::MantidWidgets::WorkspaceIndex workspaceIndex,
                    double endX));

  MOCK_METHOD3(removeFunction,
               void(std::string const &workspaceName,
                    MantidQt::MantidWidgets::WorkspaceIndex workspaceIndex,
                    std::string const &function));
  MOCK_METHOD3(addFunction,
               void(std::string const &workspaceName,
                    MantidQt::MantidWidgets::WorkspaceIndex workspaceIndex,
                    std::string const &function));
  MOCK_METHOD3(setFunction,
               void(std::string const &workspaceName,
                    MantidQt::MantidWidgets::WorkspaceIndex workspaceIndex,
                    std::string const &function));
  MOCK_CONST_METHOD2(
      getFunction, Mantid::API::IFunction_sptr(
                       std::string const &workspaceName,
                       MantidQt::MantidWidgets::WorkspaceIndex workspaceIndex));

  MOCK_CONST_METHOD3(
      getEquivalentFunctionIndexForDomain,
      std::string(std::string const &workspaceName,
                  MantidQt::MantidWidgets::WorkspaceIndex workspaceIndex,
                  std::string const &functionIndex));
  MOCK_CONST_METHOD4(
      getEquivalentParameterTieForDomain,
      std::string(std::string const &workspaceName,
                  MantidQt::MantidWidgets::WorkspaceIndex workspaceIndex,
                  std::string const &fullParameter,
                  std::string const &fullTie));

  MOCK_METHOD4(updateParameterValue,
               void(std::string const &workspaceName,
                    MantidQt::MantidWidgets::WorkspaceIndex workspaceIndex,
                    std::string const &fullParameter, double newValue));
  MOCK_METHOD4(updateAttributeValue,
               void(std::string const &workspaceName,
                    MantidQt::MantidWidgets::WorkspaceIndex workspaceIndex,
                    std::string const &fullAttribute,
                    Mantid::API::IFunction::Attribute const &newValue));

  MOCK_METHOD4(updateParameterTie,
               void(std::string const &workspaceName,
                    MantidQt::MantidWidgets::WorkspaceIndex workspaceIndex,
                    std::string const &fullParameter, std::string const &tie));

  MOCK_METHOD3(removeParameterConstraint,
               void(std::string const &workspaceName,
                    MantidQt::MantidWidgets::WorkspaceIndex workspaceIndex,
                    std::string const &fullParameter));
  MOCK_METHOD4(updateParameterConstraint,
               void(std::string const &workspaceName,
                    MantidQt::MantidWidgets::WorkspaceIndex workspaceIndex,
                    std::string const &functionIndex,
                    std::string const &constraint));

  MOCK_METHOD1(setGlobalParameters,
               void(std::vector<std::string> const &parameters));

  MOCK_METHOD1(setFittingMode, void(FittingMode fittingMode));
  MOCK_CONST_METHOD0(getFittingMode, FittingMode());

  MOCK_CONST_METHOD0(getGlobalTies, std::vector<GlobalTie>());
  MOCK_CONST_METHOD0(getGlobalParameters, std::vector<GlobalParameter>());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
