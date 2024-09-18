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

  void notifyPresenter(ViewEvent const &ev, std::string const &arg1 = "", std::string const &arg2 = "") override {
    notifyPresenterImpl(ev, arg1, arg2);
  }

  MOCK_METHOD3(notifyPresenterImpl, void(ViewEvent const &ev, std::string const &arg1, std::string const &arg2));
  MOCK_METHOD2(notifyPresenter, void(ViewEvent const &ev, std::vector<std::string> const &vec));
  MOCK_METHOD2(notifyPresenter, void(ViewEvent const &ev, FittingMode fittingMode));
  MOCK_METHOD2(handleAddDomainAccepted, void(std::vector<Mantid::API::MatrixWorkspace_const_sptr> const &workspaces,
                                             FunctionModelSpectra const &workspaceIndices));

  MOCK_METHOD0(openFitScriptGenerator, void());

  MOCK_METHOD1(setGlobalTies, void(std::vector<GlobalTie> const &globalTies));
  MOCK_METHOD1(setGlobalParameters, void(std::vector<GlobalParameter> const &globalParameters));

private:
  FitScriptGeneratorView *m_view;
  FitScriptGeneratorModel *m_model;
};

class MockFitScriptGeneratorView : public IFitScriptGeneratorView {

public:
  MOCK_METHOD1(subscribePresenter, void(IFitScriptGeneratorPresenter *presenter));

  MOCK_CONST_METHOD1(workspaceName, std::string(FitDomainIndex index));
  MOCK_CONST_METHOD1(workspaceIndex, WorkspaceIndex(FitDomainIndex index));
  MOCK_CONST_METHOD1(startX, double(FitDomainIndex index));
  MOCK_CONST_METHOD1(endX, double(FitDomainIndex index));

  MOCK_CONST_METHOD0(allRows, std::vector<FitDomainIndex>());
  MOCK_CONST_METHOD0(selectedRows, std::vector<FitDomainIndex>());
  MOCK_CONST_METHOD0(currentRow, FitDomainIndex());

  MOCK_CONST_METHOD0(hasLoadedData, bool());

  MOCK_CONST_METHOD1(parameterValue, double(std::string const &parameter));
  MOCK_CONST_METHOD1(attributeValue, Mantid::API::IFunction::Attribute(std::string const &attribute));

  MOCK_METHOD2(renameWorkspace, void(std::string const &workspaceName, std::string const &newName));

  MOCK_METHOD1(removeDomain, void(MantidQt::MantidWidgets::FitDomainIndex domainIndex));
  MOCK_METHOD4(addWorkspaceDomain,
               void(std::string const &workspaceName, WorkspaceIndex workspaceIndex, double startX, double endX));

  MOCK_METHOD0(openAddWorkspaceDialog, void());
  MOCK_METHOD1(getDialogWorkspaces, std::vector<Mantid::API::MatrixWorkspace_const_sptr>(
                                        MantidQt::MantidWidgets::IAddWorkspaceDialog *dialog));

  MOCK_METHOD7(openEditLocalParameterDialog,
               void(std::string const &parameter, std::vector<std::string> const &workspaceNames,
                    std::vector<std::string> const &domainNames, std::vector<double> const &values,
                    std::vector<bool> const &fixes, std::vector<std::string> const &ties,
                    std::vector<std::string> const &constraints));
  MOCK_CONST_METHOD0(getEditLocalParameterResults, std::tuple<std::string, std::vector<double>, std::vector<bool>,
                                                              std::vector<std::string>, std::vector<std::string>>());

  MOCK_CONST_METHOD0(fitOptions, std::tuple<std::string, std::string, std::string, std::string, std::string, bool>());
  MOCK_CONST_METHOD0(outputBaseName, std::string());
  MOCK_CONST_METHOD0(filepath, std::string());

  MOCK_METHOD0(resetSelection, void());

  MOCK_CONST_METHOD0(applyFunctionChangesToAll, bool());

  MOCK_METHOD0(clearFunction, void());
  MOCK_CONST_METHOD1(setFunction, void(Mantid::API::IFunction_sptr const &function));

  MOCK_METHOD1(setSimultaneousMode, void(bool simultaneousMode));

  MOCK_METHOD1(setGlobalTies, void(std::vector<GlobalTie> const &globalTies));
  MOCK_METHOD1(setGlobalParameters, void(std::vector<GlobalParameter> const &globalParameter));

  MOCK_METHOD1(displayWarning, void(std::string const &message));

  MOCK_CONST_METHOD0(tableWidget, FitScriptGeneratorDataTable *());
  MOCK_CONST_METHOD0(removeButton, QPushButton *());
  MOCK_CONST_METHOD0(addWorkspaceButton, QPushButton *());
  MOCK_CONST_METHOD0(addWorkspaceDialog, AddWorkspaceDialog *());
  MOCK_CONST_METHOD0(generateScriptToFileButton, QPushButton *());
  MOCK_CONST_METHOD0(generateScriptToClipboardButton, QPushButton *());

  MOCK_METHOD1(setSuccessText, void(std::string const &text));
  MOCK_CONST_METHOD1(saveTextToClipboard, void(std::string const &text));
};

class MockFitScriptGeneratorModel : public IFitScriptGeneratorModel {

public:
  MOCK_METHOD1(subscribePresenter, void(IFitScriptGeneratorPresenter *presenter));

  MOCK_METHOD1(removeDomain, void(MantidQt::MantidWidgets::FitDomainIndex domainIndex));
  MOCK_METHOD4(addWorkspaceDomain,
               void(std::string const &workspaceName, WorkspaceIndex workspaceIndex, double startX, double endX));
  MOCK_CONST_METHOD2(hasWorkspaceDomain, bool(std::string const &workspaceName, WorkspaceIndex workspaceIndex));

  MOCK_METHOD2(renameWorkspace, void(std::string const &workspaceName, std::string const &newName));

  MOCK_METHOD3(updateStartX, bool(std::string const &workspaceName, WorkspaceIndex workspaceIndex, double startX));
  MOCK_METHOD3(updateEndX, bool(std::string const &workspaceName, WorkspaceIndex workspaceIndex, double endX));

  MOCK_METHOD3(removeFunction,
               void(std::string const &workspaceName, WorkspaceIndex workspaceIndex, std::string const &function));
  MOCK_METHOD3(addFunction,
               void(std::string const &workspaceName, WorkspaceIndex workspaceIndex, std::string const &function));
  MOCK_METHOD3(setFunction,
               void(std::string const &workspaceName, WorkspaceIndex workspaceIndex, std::string const &function));
  MOCK_CONST_METHOD2(getFunction,
                     Mantid::API::IFunction_sptr(std::string const &workspaceName, WorkspaceIndex workspaceIndex));

  MOCK_CONST_METHOD3(getEquivalentFunctionIndexForDomain,
                     std::string(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                 std::string const &functionIndex));
  MOCK_CONST_METHOD2(getEquivalentFunctionIndexForDomain,
                     std::string(MantidQt::MantidWidgets::FitDomainIndex, std::string const &functionIndex));
  MOCK_CONST_METHOD4(getEquivalentParameterTieForDomain,
                     std::string(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                 std::string const &fullParameter, std::string const &fullTie));
  MOCK_CONST_METHOD1(getAdjustedFunctionIndex, std::string(std::string const &parameter));
  MOCK_CONST_METHOD2(getFullParameter,
                     std::string(MantidQt::MantidWidgets::FitDomainIndex, std::string const &parameter));
  MOCK_CONST_METHOD2(getFullTie, std::string(MantidQt::MantidWidgets::FitDomainIndex, std::string const &tie));

  MOCK_METHOD4(updateParameterValue, void(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                          std::string const &fullParameter, double newValue));
  MOCK_METHOD4(updateAttributeValue,
               void(std::string const &workspaceName, WorkspaceIndex workspaceIndex, std::string const &fullAttribute,
                    Mantid::API::IFunction::Attribute const &newValue));

  MOCK_METHOD4(updateParameterTie, void(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                        std::string const &fullParameter, std::string const &tie));

  MOCK_METHOD3(removeParameterConstraint,
               void(std::string const &workspaceName, WorkspaceIndex workspaceIndex, std::string const &fullParameter));
  MOCK_METHOD4(updateParameterConstraint, void(std::string const &workspaceName, WorkspaceIndex workspaceIndex,
                                               std::string const &functionIndex, std::string const &constraint));

  MOCK_METHOD1(setGlobalParameters, void(std::vector<std::string> const &parameters));

  MOCK_METHOD1(setOutputBaseName, void(std::string const &outputBaseName));

  MOCK_METHOD1(setFittingMode, void(FittingMode fittingMode));
  MOCK_CONST_METHOD0(getFittingMode, FittingMode());
  MOCK_CONST_METHOD0(isSimultaneousMode, bool());

  MOCK_CONST_METHOD2(hasParameter,
                     bool(MantidQt::MantidWidgets::FitDomainIndex domainIndex, std::string const &parameter));

  MOCK_METHOD3(setParameterValue, void(MantidQt::MantidWidgets::FitDomainIndex domainIndex,
                                       std::string const &fullParameter, double value));
  MOCK_METHOD3(setParameterFixed,
               void(MantidQt::MantidWidgets::FitDomainIndex domainIndex, std::string const &fullParameter, bool fix));
  MOCK_METHOD3(setParameterTie, void(MantidQt::MantidWidgets::FitDomainIndex domainIndex,
                                     std::string const &fullParameter, std::string const &tie));
  MOCK_METHOD3(setParameterConstraint, void(MantidQt::MantidWidgets::FitDomainIndex domainIndex,
                                            std::string const &fullParameter, std::string const &constraint));

  MOCK_CONST_METHOD1(getDomainName, std::string(MantidQt::MantidWidgets::FitDomainIndex domainIndex));
  MOCK_CONST_METHOD2(getParameterValue,
                     double(MantidQt::MantidWidgets::FitDomainIndex domainIndex, std::string const &fullParameter));
  MOCK_CONST_METHOD2(isParameterFixed,
                     bool(MantidQt::MantidWidgets::FitDomainIndex domainIndex, std::string const &fullParameter));
  MOCK_CONST_METHOD2(getParameterTie, std::string(MantidQt::MantidWidgets::FitDomainIndex domainIndex,
                                                  std::string const &fullParameter));
  MOCK_CONST_METHOD2(getParameterConstraint, std::string(MantidQt::MantidWidgets::FitDomainIndex domainIndex,
                                                         std::string const &fullParameter));

  MOCK_CONST_METHOD0(numberOfDomains, std::size_t());

  MOCK_CONST_METHOD0(getGlobalTies, std::vector<GlobalTie>());
  MOCK_CONST_METHOD0(getGlobalParameters, std::vector<GlobalParameter>());

  MOCK_CONST_METHOD0(isValid, std::tuple<bool, std::string>());

  std::string generatePythonFitScript([[maybe_unused]] std::tuple<std::string, std::string, std::string, std::string,
                                                                  std::string, bool> const &fitOptions,
                                      [[maybe_unused]] std::string const &filepath = "") override {
    return "# mock python script";
  }
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
