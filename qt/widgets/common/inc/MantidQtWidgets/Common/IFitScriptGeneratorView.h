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
#include "MantidQtWidgets/Common/IndexTypes.h"
#include "MantidQtWidgets/Common/MantidWidget.h"

#include <string>
#include <vector>

#include <QObject>
#include <QPushButton>
#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {

class AddWorkspaceDialog;
class FitScriptGeneratorDataTable;
class IFitScriptGeneratorPresenter;
struct GlobalParameter;
struct GlobalTie;

class EXPORT_OPT_MANTIDQT_COMMON IFitScriptGeneratorView : public API::MantidWidget {
  Q_OBJECT

public:
  enum class Event {
    RemoveClicked,
    AddClicked,
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
    FittingModeChanged
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

  virtual void removeWorkspaceDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex) = 0;
  virtual void addWorkspaceDomain(std::string const &workspaceName, WorkspaceIndex workspaceIndex, double startX,
                                  double endX) = 0;

  [[nodiscard]] virtual bool openAddWorkspaceDialog() = 0;
  [[nodiscard]] virtual std::vector<Mantid::API::MatrixWorkspace_const_sptr> getDialogWorkspaces() = 0;
  [[nodiscard]] virtual std::vector<WorkspaceIndex> getDialogWorkspaceIndices() const = 0;

  virtual void resetSelection() = 0;

  virtual bool applyFunctionChangesToAll() const = 0;

  virtual void clearFunction() = 0;
  virtual void setFunction(Mantid::API::IFunction_sptr const &function) const = 0;

  virtual void setSimultaneousMode(bool simultaneousMode) = 0;

  virtual void setGlobalTies(std::vector<GlobalTie> const &globalTies) = 0;
  virtual void setGlobalParameters(std::vector<GlobalParameter> const &globalParameter) = 0;

  virtual void displayWarning(std::string const &message) = 0;

public:
  /// Testing accessors
  virtual FitScriptGeneratorDataTable *tableWidget() const = 0;
  virtual QPushButton *removeButton() const = 0;
  virtual QPushButton *addWorkspaceButton() const = 0;
  virtual AddWorkspaceDialog *addWorkspaceDialog() const = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt
