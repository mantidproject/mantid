#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORPLOTGROUPCOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORPLOTGROUPCOMMAND_H

#include "MantidQtWidgets/Common/DataProcessorUI/CommandBase.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** @class PlotGroupCommand

PlotGroupCommand defines the action "Plot Selected Groups"

Copyright &copy; 2011-16 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class PlotGroupCommand : public CommandBase {
public:
  PlotGroupCommand(DataProcessorPresenter *tablePresenter)
      : CommandBase(tablePresenter){};
  PlotGroupCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~PlotGroupCommand(){};

  void execute() override {
    m_presenter->notify(DataProcessorPresenter::PlotGroupFlag);
  };
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
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORPLOTGROUPCOMMAND_H*/
