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
/** @class SeparatorCommand

SeparatorCommand defines a separator. It has no name, no icon and
empty
execute() method
*/
class SeparatorCommand : public CommandBase {
public:
  SeparatorCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  SeparatorCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~SeparatorCommand(){};

  void execute() override{};
  QString name() override { return QString(); }
  QString icon() override { return QString(); }
  QString tooltip() override { return QString(); }
  QString whatsthis() override { return QString(); }
  QString shortcut() override { return QString(); }
  bool modifiesSettings() override { return false; }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt