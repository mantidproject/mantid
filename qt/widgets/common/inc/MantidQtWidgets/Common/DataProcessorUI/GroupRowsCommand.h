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
/** @class GroupRowsCommand

GroupRowsCommand defines the action "Group Selected"
*/
class GroupRowsCommand : public CommandBase {
public:
  GroupRowsCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  GroupRowsCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~GroupRowsCommand(){};

  void execute() override { m_presenter->notify(DataProcessorPresenter::GroupRowsFlag); };
  QString name() override { return QString("Group Selected"); }
  QString icon() override { return QString("://drag_curves.png"); }
  QString tooltip() override { return QString("Group selected rows"); }
  QString whatsthis() override { return QString("Places all selected runs into the same group"); }
  QString shortcut() override { return QString(); }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt