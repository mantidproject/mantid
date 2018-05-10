#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORPLOTROWCOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORPLOTROWCOMMAND_H

#include "MantidQtWidgets/Common/DataProcessorUI/CommandBase.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
/** @class PlotRowCommand

PlotRowCommand defines the action "Plot Selected Rows"

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
class PlotRowCommand : public CommandBase {
public:
  PlotRowCommand(DataProcessorPresenter *tablePresenter)
      : CommandBase(tablePresenter){};
  PlotRowCommand(const QDataProcessorWidget &widget) : CommandBase(widget){};
  virtual ~PlotRowCommand(){};

  void execute() override {
    m_presenter->notify(DataProcessorPresenter::PlotRowFlag);
  };
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
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORPLOTROWCOMMAND_H*/
