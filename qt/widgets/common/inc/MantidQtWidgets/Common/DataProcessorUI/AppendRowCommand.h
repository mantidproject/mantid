// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DataProcessorUI/CommandBase.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPresenter.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** @class AppendRowCommand

AppendRowCommand defines the action "Insert Row After"
*/
class AppendRowCommand : public CommandBase {
public:
  AppendRowCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  AppendRowCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~AppendRowCommand(){};

  void execute() override { m_presenter->notify(DataProcessorPresenter::AppendRowFlag); };
  QString name() override { return QString("Insert Row After"); }
  QString icon() override { return QString("://insert_row.png"); }
  QString tooltip() override { return QString("Inserts row after"); }
  QString whatsthis() override {
    return QString("Inserts a new row after the last selected row. If "
                   "groups exist and a group is selected, the new row is "
                   "appended to the selected group. If nothing is selected "
                   "then a new row is added to the last group");
  }
  QString shortcut() override { return QString(); }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt