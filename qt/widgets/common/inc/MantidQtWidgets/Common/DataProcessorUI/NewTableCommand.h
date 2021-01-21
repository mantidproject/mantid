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
/** @class NewTableCommand

NewTableCommand defines the action "New Table"
*/
class NewTableCommand : public CommandBase {
public:
  NewTableCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  NewTableCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~NewTableCommand(){};

  void execute() override { m_presenter->notify(DataProcessorPresenter::NewTableFlag); };
  QString name() override { return QString("New Table"); }
  QString icon() override { return QString("://new.png"); }
  QString tooltip() override { return QString("New Table"); }
  QString whatsthis() override { return QString("Loads a blank table into the interface"); }
  QString shortcut() override { return QString(); }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt