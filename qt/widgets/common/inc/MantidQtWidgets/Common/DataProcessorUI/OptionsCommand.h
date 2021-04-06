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
/** @class OptionsCommand

OptionsCommand defines the action "Import .TBL"
*/
class OptionsCommand : public CommandBase {
public:
  OptionsCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  OptionsCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~OptionsCommand(){};

  void execute() override { m_presenter->notify(DataProcessorPresenter::OptionsDialogFlag); };
  QString name() override { return QString("Options"); }
  QString icon() override { return QString("://configure.png"); }
  QString tooltip() override { return QString("Options"); }
  QString whatsthis() override { return QString("Opens a dialog with some options for the table"); }
  QString shortcut() override { return QString(); }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt