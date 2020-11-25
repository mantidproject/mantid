// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "ui_FitScriptGenerator.h"

#include "MantidQtWidgets/Common/FitOptionsBrowser.h"
#include "MantidQtWidgets/Common/FunctionBrowser.h"
#include "MantidQtWidgets/Common/MantidWidget.h"

#include <memory>

#include <QMap>
#include <QString>
#include <QStringList>
#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {

class FitScriptGeneratorPresenter;

class EXPORT_OPT_MANTIDQT_COMMON FitScriptGeneratorView
    : public API::MantidWidget {
  Q_OBJECT

  enum ColumnIndex {
    WorkspaceName = 0,
    WorkspaceIndex = 1,
    StartX = 2,
    EndX = 3
  } const;

public:
  enum class Event { RemoveClicked, StartXChanged, EndXChanged } const;

  FitScriptGeneratorView(
      QWidget *parent = nullptr,
      QStringList const &workspaceNames = QStringList(), double startX = 0.0,
      double endX = 0.0,
      QMap<QString, QString> const &fitOptions = QMap<QString, QString>());
  ~FitScriptGeneratorView() override;

  void subscribePresenter(FitScriptGeneratorPresenter *presenter);

private slots:
  void onRemoveClicked();

private:
  void connectUiSignals();
  void setWorkspaces(QStringList const &workspaceNames, double startX,
                     double endX);
  void setFitBrowserOptions(QMap<QString, QString> const &fitOptions);
  void setFitBrowserOption(QString const &name, QString const &value);
  void setFittingType(QString const &fitType);

  void addWorkspaceDomain(QString const &workspaceName, int workspaceIndex,
                          double startX, double endX);

  void setItemInDomainTable(int rowIndex, int columnIndex,
                            QVariant const &value);

  FitScriptGeneratorPresenter *m_presenter;
  std::unique_ptr<FunctionBrowser> m_functionBrowser;
  std::unique_ptr<FitOptionsBrowser> m_fitOptionsBrowser;
  Ui::FitScriptGenerator m_ui;
};

} // namespace MantidWidgets
} // namespace MantidQt
