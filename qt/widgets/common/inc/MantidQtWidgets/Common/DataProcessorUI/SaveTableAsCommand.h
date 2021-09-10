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
/** @class SaveTableAsCommand

SaveTableAsCommand defines the action "Save Table As"
*/
class SaveTableAsCommand : public CommandBase {
public:
  SaveTableAsCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  SaveTableAsCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~SaveTableAsCommand(){};

  void execute() override { m_presenter->notify(DataProcessorPresenter::SaveAsFlag); };
  QString name() override { return QString("Save Table As"); }
  QString icon() override { return QString("://filesaveas.png"); }
  QString tooltip() override { return QString("Save Table As"); }
  QString whatsthis() override {
    return QString("Saves current table as a table workspace. Asks for the "
                   "name of the ouput table");
  }
  QString shortcut() override { return QString(); }
  bool modifiesSettings() override { return false; }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt