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
/** @class ExportTableCommand

ExportTableCommand defines the action "Export .TBL"

processor interface presenter needs to support.
*/
class ExportTableCommand : public CommandBase {
public:
  ExportTableCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  ExportTableCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~ExportTableCommand(){};

  void execute() override { m_presenter->notify(DataProcessorPresenter::ExportTableFlag); };
  QString name() override { return QString("Export .TBL"); }
  QString icon() override { return QString("://save_template.png"); }
  QString tooltip() override { return QString("Export .TBL file"); }
  QString whatsthis() override { return QString("Opens a dialog to export a table as .TBL file"); }
  QString shortcut() override { return QString(); }
  bool modifiesSettings() override { return false; }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt