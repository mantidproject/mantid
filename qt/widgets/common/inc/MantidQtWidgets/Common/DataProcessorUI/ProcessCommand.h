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

/** @class ProcessCommand

ProcessCommand defines the action "Process"
*/
class ProcessCommand : public CommandBase {
public:
  ProcessCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  ProcessCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~ProcessCommand(){};

  void execute() override { m_presenter->notify(DataProcessorPresenter::ProcessFlag); };
  QString name() override { return QString("Process"); }
  QString icon() override { return QString("://stat_rows.png"); }
  QString tooltip() override { return QString("Processes selected runs"); }
  QString whatsthis() override {
    return QString("Processes the selected runs. Selected runs are reduced "
                   "sequentially and independently. If nothing is "
                   "selected, the behaviour is as if all "
                   "runs were selected.");
  }
  QString shortcut() override { return QString(); }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt