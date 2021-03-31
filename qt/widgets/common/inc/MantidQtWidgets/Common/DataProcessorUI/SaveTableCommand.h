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
/** @class SaveTableCommand

SaveTableCommand defines the action "Save Table"
*/
class SaveTableCommand : public CommandBase {
public:
  SaveTableCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  SaveTableCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~SaveTableCommand(){};

  void execute() override { m_presenter->notify(DataProcessorPresenter::SaveFlag); };
  QString name() override { return QString("Save Table"); }
  QString icon() override { return QString("://filesave.png"); }
  QString tooltip() override { return QString("Save Table"); }
  QString whatsthis() override { return QString("Saves current table as a table workspace"); }
  QString shortcut() override { return QString(); }
  bool modifiesSettings() override { return false; }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt