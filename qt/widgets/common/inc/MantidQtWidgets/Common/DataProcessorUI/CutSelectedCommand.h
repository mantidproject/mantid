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
/** @class CutSelectedCommand

CutSelectedCommand defines the action "Cut Selected"
*/
class CutSelectedCommand : public CommandBase {
public:
  CutSelectedCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  CutSelectedCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~CutSelectedCommand(){};

  void execute() override { m_presenter->notify(DataProcessorPresenter::CutSelectedFlag); };
  QString name() override { return QString("Cut Selected"); }
  QString icon() override { return QString("://cut.png"); }
  QString tooltip() override { return QString("Cut selected"); }
  QString whatsthis() override {
    return QString("Copies the selected rows to the clipboard, and then "
                   "deletes them. Each row is placed on a new line, and "
                   "each cell is separated by a tab");
  }
  QString shortcut() override { return QString("Ctrl+X"); }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt