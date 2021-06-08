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
/** @class CollapseGroupsCommand

CollapseGroupsCommand defines the action "Collapse All Groups"
*/
class CollapseGroupsCommand : public CommandBase {
public:
  CollapseGroupsCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter) {}
  virtual ~CollapseGroupsCommand() {}

  void execute() override { m_presenter->notify(DataProcessorPresenter::CollapseAllGroupsFlag); };
  QString name() override { return QString("Collapse All Groups"); }
  QString icon() override { return QString("://collapse_all.png"); }
  QString tooltip() override { return QString("Collapse all groups"); }
  QString whatsthis() override {
    return QString("If any groups in the table are currently expanded this will collapse "
                   "all expanded groups, hiding their individual runs.");
  }
  QString shortcut() override { return QString(); }
  bool modifiesSettings() override { return false; }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt