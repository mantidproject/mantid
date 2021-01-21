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
/** @class PlotRowCommand

PlotRowCommand defines the action "Plot Selected Rows"
*/
class PlotRowCommand : public CommandBase {
public:
  PlotRowCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  PlotRowCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~PlotRowCommand(){};

  void execute() override { m_presenter->notify(DataProcessorPresenter::PlotRowFlag); };
  QString name() override { return QString("Plot Selected Rows"); }
  QString icon() override { return QString("://graph.png"); }
  QString tooltip() override { return QString("Plot the selected runs"); }
  QString whatsthis() override {
    return QString("Creates a plot of the reduced workspaces produced by "
                   "the selected runs");
  }
  QString shortcut() override { return QString(); }
  bool modifiesSettings() override { return false; }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt