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
/** @class ClearSelectedCommand

ClearSelectedCommand defines the action "Clear Selected"
*/
class ClearSelectedCommand : public CommandBase {
public:
  ClearSelectedCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  ClearSelectedCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~ClearSelectedCommand(){};

  void execute() override { m_presenter->notify(DataProcessorPresenter::ClearSelectedFlag); };
  QString name() override { return QString("Clear Selected"); }
  QString icon() override { return QString("://erase.png"); }
  QString tooltip() override { return QString("Clear selected"); }
  QString whatsthis() override { return QString("Clears the contents of the selected rows"); }
  QString shortcut() override { return QString(); }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt