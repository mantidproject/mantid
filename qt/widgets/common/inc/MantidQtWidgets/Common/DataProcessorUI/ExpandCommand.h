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
/** @class ExpandCommand

ExpandCommand defines the action "Expand Selection"
*/
class ExpandCommand : public CommandBase {
public:
  ExpandCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  ExpandCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~ExpandCommand(){};

  void execute() override { m_presenter->notify(DataProcessorPresenter::ExpandSelectionFlag); };
  QString name() override { return QString("Expand Selection"); }
  QString icon() override { return QString("://fit_frame.png"); }
  QString tooltip() override { return QString("Selects an entire group"); }
  QString whatsthis() override {
    return QString("Expands the current selection to include any runs that "
                   "are in the same group as any selected run. This "
                   "effectively means selecting the group to which the "
                   "selected run belongs");
  }
  QString shortcut() override { return QString(); }
  bool modifiesSettings() override { return false; }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt