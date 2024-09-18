// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/Common/FittingMode.h"
#include "MantidQtWidgets/Common/FunctionModelSpectra.h"
#include "MantidQtWidgets/Common/IndexTypes.h"
#include "MantidQtWidgets/Common/MantidWidget.h"

#include <string>
#include <tuple>
#include <vector>

#include <QObject>
#include <QPushButton>
#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {

class AddWorkspaceDialog;
class FitScriptGeneratorDataTable;
class IAddWorkspaceDialog;
class IFitScriptGeneratorPresenter;
struct GlobalParameter;
struct GlobalTie;

class EXPORT_OPT_MANTIDQT_COMMON IFitScriptGeneratorView : public API::MantidWidget {
  Q_OBJECT

public:
  enum class Event {
    ADSDeleteEvent,
    ADSClearEvent,
    ADSRenameEvent,
    RemoveDomainClicked,
    AddDomainClicked,
    StartXChanged,
    EndXChanged,
    SelectionChanged,
    FunctionRemoved,
    FunctionAdded,
    FunctionReplaced,
    ParameterChanged,
    AttributeChanged,
    ParameterTieChanged,
    ParameterConstraintRemoved,
    ParameterConstraintChanged,
    GlobalParametersChanged,
    EditLocalParameterClicked,
    EditLocalParameterFinished,
    OutputBaseNameChanged,
    FittingModeChanged,
    GenerateScriptToFileClicked,
    GenerateScriptToClipboardClicked
  };

  IFitScriptGeneratorView(QWidget *parent = nullptr) : API::MantidWidget(parent) {}
  virtual ~IFitScriptGeneratorView() = default;

  virtual void subscribePresenter(IFitScriptGeneratorPresenter *presenter) = 0;

  [[nodiscard]] virtual std::string workspaceName(FitDomainIndex index) const = 0;
  [[nodiscard]] virtual WorkspaceIndex workspaceIndex(FitDomainIndex index) const = 0;
  [[nodiscard]] virtual double startX(FitDomainIndex index) const = 0;
  [[nodiscard]] virtual double endX(FitDomainIndex index) const = 0;

  [[nodiscard]] virtual std::vector<FitDomainIndex> allRows() const = 0;
  [[nodiscard]] virtual std::vector<FitDomainIndex> selectedRows() const = 0;
  [[nodiscard]] virtual FitDomainIndex currentRow() const = 0;

  [[nodiscard]] virtual bool hasLoadedData() const = 0;

  [[nodiscard]] virtual double parameterValue(std::string const &parameter) const = 0;
  [[nodiscard]] virtual Mantid::API::IFunction::Attribute attributeValue(std::string const &attribute) const = 0;

  virtual void renameWorkspace(std::string const &workspaceName, std::string const &newName) = 0;

  virtual void removeDomain(FitDomainIndex domainIndex) = 0;
  virtual void addWorkspaceDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex, double startX,
                                  double endX) = 0;

  virtual void openAddWorkspaceDialog() = 0;
  [[nodiscard]] virtual std::vector<Mantid::API::MatrixWorkspace_const_sptr>
  getDialogWorkspaces(MantidWidgets::IAddWorkspaceDialog *dialog) = 0;

  virtual void openEditLocalParameterDialog(std::string const &parameter,
                                            std::vector<std::string> const &workspaceNames,
                                            std::vector<std::string> const &domainNames,
                                            std::vector<double> const &values, std::vector<bool> const &fixes,
                                            std::vector<std::string> const &ties,
                                            std::vector<std::string> const &constraints) = 0;
  virtual std::tuple<std::string, std::vector<double>, std::vector<bool>, std::vector<std::string>,
                     std::vector<std::string>>
  getEditLocalParameterResults() const = 0;

  [[nodiscard]] virtual std::tuple<std::string, std::string, std::string, std::string, std::string, bool>
  fitOptions() const = 0;
  [[nodiscard]] virtual std::string filepath() const = 0;

  virtual void resetSelection() = 0;

  virtual bool applyFunctionChangesToAll() const = 0;

  virtual void clearFunction() = 0;
  virtual void setFunction(Mantid::API::IFunction_sptr const &function) const = 0;

  virtual void setSimultaneousMode(bool simultaneousMode) = 0;

  virtual void setGlobalTies(std::vector<GlobalTie> const &globalTies) = 0;
  virtual void setGlobalParameters(std::vector<GlobalParameter> const &globalParameter) = 0;

  virtual void displayWarning(std::string const &message) = 0;

  virtual void setSuccessText(std::string const &text) = 0;
  virtual void saveTextToClipboard(std::string const &text) const = 0;

public:
  /// Testing accessors
  virtual FitScriptGeneratorDataTable *tableWidget() const = 0;
  virtual QPushButton *removeButton() const = 0;
  virtual QPushButton *addWorkspaceButton() const = 0;
  virtual QPushButton *generateScriptToFileButton() const = 0;
  virtual QPushButton *generateScriptToClipboardButton() const = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt
