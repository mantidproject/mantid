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
/** @class OpenTableCommand

OpenTableCommand defines the action "Open Table"
*/
class OpenTableCommand : public CommandBase {
public:
  OpenTableCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  OpenTableCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~OpenTableCommand(){};

  void execute() override{
      // This action should do nothing
  };
  QString name() override { return QString("Open Table"); }
  QString icon() override { return QString("://multiload.png"); }
  QString tooltip() override { return QString("Open Table"); }
  QString whatsthis() override {
    return QString("Loads a table into the interface. Table must exist in "
                   "the ADS and be compatible in terms of the number and "
                   "type of columns");
  }
  QString shortcut() override { return QString(); }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt