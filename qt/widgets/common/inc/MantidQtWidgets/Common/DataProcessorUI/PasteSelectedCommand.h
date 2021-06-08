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
/** @class PasteSelectedCommand

PasteSelectedCommand defines the action "Paste Selected"
*/
class PasteSelectedCommand : public CommandBase {
public:
  PasteSelectedCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  PasteSelectedCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~PasteSelectedCommand(){};

  void execute() override { m_presenter->notify(DataProcessorPresenter::PasteSelectedFlag); };
  QString name() override { return QString("Paste Selected"); }
  QString icon() override { return QString("://paste.png"); }
  QString tooltip() override { return QString("Paste selected"); }
  QString whatsthis() override {
    return QString("Pastes the contents of the clipboard into the selected "
                   "rows. If no rows are selected, new ones are added at "
                   "the end");
  }
  QString shortcut() override { return QString("Ctrl+V"); }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt