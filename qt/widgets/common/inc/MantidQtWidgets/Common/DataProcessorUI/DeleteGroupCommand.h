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

/** @class DeleteGroupCommand

DeleteGroupCommand defines the action "Delete Group"
*/
class DeleteGroupCommand : public CommandBase {
public:
  DeleteGroupCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  DeleteGroupCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~DeleteGroupCommand(){};

  void execute() override { m_presenter->notify(DataProcessorPresenter::DeleteGroupFlag); };
  QString name() override { return QString("Delete Group"); }
  QString icon() override { return QString("://delete_group.png"); }
  QString tooltip() override { return QString("Deletes selected group"); }
  QString whatsthis() override { return QString("Deletes the selected groups"); }
  QString shortcut() override { return QString(); }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt