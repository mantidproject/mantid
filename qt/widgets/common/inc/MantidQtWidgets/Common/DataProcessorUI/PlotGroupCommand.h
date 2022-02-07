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
/** @class PlotGroupCommand

PlotGroupCommand defines the action "Plot Selected Groups"
*/
class PlotGroupCommand : public CommandBase {
public:
  PlotGroupCommand(DataProcessorPresenter *tablePresenter) : CommandBase(tablePresenter){};
  PlotGroupCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~PlotGroupCommand(){};

  void execute() override { m_presenter->notify(DataProcessorPresenter::PlotGroupFlag); };
  QString name() override { return QString("Plot Selected Groups"); }
  QString icon() override { return QString("://trajectory.png"); }
  QString tooltip() override { return QString("Plots the selected group"); }
  QString whatsthis() override {
    return QString("Creates a plot of the post-processed workspaces "
                   "produced by any groups any selected runs are in");
  }
  QString shortcut() override { return QString(); }
  bool modifiesSettings() override { return false; }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt