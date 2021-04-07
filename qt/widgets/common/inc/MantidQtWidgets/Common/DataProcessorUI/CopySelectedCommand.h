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
/** @class CopySelectedCommand

CopySelectedCommand defines the action "Copy Selected"
*/
class CopySelectedCommand : public CommandBase {
public:
  CopySelectedCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  CopySelectedCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~CopySelectedCommand(){};

  void execute() override { m_presenter->notify(DataProcessorPresenter::CopySelectedFlag); };
  QString name() override { return QString("Copy Selected"); }
  QString icon() override { return QString("://copy.png"); }
  QString tooltip() override { return QString("Copy selected"); }
  QString whatsthis() override {
    return QString("Copies the selected rows to the clipboard. Each row is "
                   "placed on a new line, and each cell is separated by a "
                   "tab");
  }
  QString shortcut() override { return QString("Ctrl+C"); }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt