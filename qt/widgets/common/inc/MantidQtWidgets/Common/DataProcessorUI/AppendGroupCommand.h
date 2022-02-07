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
/** @class AppendGroupCommand

AppendGroupCommand defines the action "Insert Group"
*/
class AppendGroupCommand : public CommandBase {
public:
  AppendGroupCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  AppendGroupCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~AppendGroupCommand(){};

  void execute() override { m_presenter->notify(DataProcessorPresenter::AppendGroupFlag); };
  QString name() override { return QString("Insert Group After"); }
  QString icon() override { return QString("://insert_group.png"); }
  QString tooltip() override { return QString("Inserts group after"); }
  QString whatsthis() override {
    return QString("Inserts a new group after the first selected group. If "
                   "no groups are selected then a new group is added at "
                   "the end of the table");
  }
  QString shortcut() override { return QString(); }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt