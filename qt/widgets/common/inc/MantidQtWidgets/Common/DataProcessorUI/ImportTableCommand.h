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
/** @class ImportTableCommand

ImportTableCommand defines the action "Import .TBL"
*/
class ImportTableCommand : public CommandBase {
public:
  ImportTableCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  ImportTableCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~ImportTableCommand(){};

  void execute() override { m_presenter->notify(DataProcessorPresenter::ImportTableFlag); };
  QString name() override { return QString("Import .TBL"); }
  QString icon() override { return QString("://open_template.png"); }
  QString tooltip() override { return QString("Import .TBL file"); }
  QString whatsthis() override { return QString("Opens a dialog to select a .TBL file to import"); }
  QString shortcut() override { return QString(); }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt