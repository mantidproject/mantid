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
/** @class PauseCommand

ProcessCommand defines the action "Pause"
*/
class PauseCommand : public CommandBase {
public:
  PauseCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  virtual ~PauseCommand(){};

  void execute() override { m_presenter->notify(DataProcessorPresenter::PauseFlag); };
  QString name() override { return QString("Pause"); }
  QString icon() override { return QString("://pause.png"); }
  QString tooltip() override { return QString("Pause processing runs"); }
  QString whatsthis() override {
    return QString("Pauses processing any selected runs. Processing may be "
                   "resumed by clicking on the 'Process' button.");
  }
  QString shortcut() override { return QString(); }
  bool modifiesSettings() override { return false; }
  bool modifiesRunningProcesses() override { return true; }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt