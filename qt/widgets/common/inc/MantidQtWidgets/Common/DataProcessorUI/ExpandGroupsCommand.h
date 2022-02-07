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
/** @class ExpandGroupsCommand

ExpandGroupsCommand defines the action "Expand All Groups"
*/
class ExpandGroupsCommand : public CommandBase {
public:
  ExpandGroupsCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter) {}
  virtual ~ExpandGroupsCommand() {}

  void execute() override { m_presenter->notify(DataProcessorPresenter::ExpandAllGroupsFlag); };
  QString name() override { return QString("Expand All Groups"); }
  QString icon() override { return QString("://expand_all.png"); }
  QString tooltip() override { return QString("Expands all groups"); }
  QString whatsthis() override {
    return QString("If any groups in the table are currently collapsed this will expand "
                   "all collapsed groups, revealing their individual runs.");
  }
  QString shortcut() override { return QString(); }
  bool modifiesSettings() override { return false; }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt