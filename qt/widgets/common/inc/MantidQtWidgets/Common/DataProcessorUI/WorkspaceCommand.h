// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DataProcessorUI/CommandBase.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** @class WorkspaceCommand

WorkspaceCommand defines a workspace action
*/
class WorkspaceCommand : public CommandBase {
public:
  WorkspaceCommand(DataProcessorPresenter *tablePresenter, const QString &name)
      : CommandBase(tablePresenter), m_name(name){};
  WorkspaceCommand(const QDataProcessorWidget &widget, const QString &name) : CommandBase(widget), m_name(name){};
  virtual ~WorkspaceCommand(){};

  void execute() override {
    // Tell the presenter which of the available workspaces was selected
    m_presenter->setModel(m_name);
  };
  QString name() override { return m_name; }
  QString icon() override { return QString("://worksheet.png"); }
  QString tooltip() override { return QString("Table Workspace"); }
  QString whatsthis() override { return QString("Table Workspace"); }
  QString shortcut() override { return QString(); }

private:
  QString m_name;
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt