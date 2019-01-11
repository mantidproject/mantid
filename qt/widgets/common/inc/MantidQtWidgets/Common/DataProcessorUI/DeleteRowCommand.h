// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORDELETEROWCOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORDELETEROWCOMMAND_H

#include "MantidQtWidgets/Common/DataProcessorUI/CommandBase.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/** @class DeleteRowCommand

DeleteRowCommand defines the action "Delete Row"
*/
class DeleteRowCommand : public CommandBase {
public:
  DeleteRowCommand(DataProcessorPresenter *tablePresenter)
      : CommandBase(tablePresenter){};
  DeleteRowCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~DeleteRowCommand(){};

  void execute() override {
    m_presenter->notify(DataProcessorPresenter::DeleteRowFlag);
  };
  QString name() override { return QString("Delete Row"); }
  QString icon() override { return QString("://delete_row.png"); }
  QString tooltip() override { return QString("Deletes a row"); }
  QString whatsthis() override { return QString("Deletes the selected row"); }
  QString shortcut() override { return QString(); }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORDELETEROWCOMMAND_H*/
