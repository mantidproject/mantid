#ifndef MANTIDQTMANTIDWIDGETS_DATAPROCESSORPAUSECOMMAND_H
#define MANTIDQTMANTIDWIDGETS_DATAPROCESSORPAUSECOMMAND_H

#include "MantidQtWidgets/Common/DataProcessorUI/TableQueryCommandBase.h"

namespace MantidQt {
namespace MantidWidgets {
/** @class DataProcessorPauseCommand

DataProcessorProcessCommand defines the action "Pause"

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
class DataProcessorPauseCommand : public TableQueryCommandBase {
public:
  using TableQueryCommandBase::TableQueryCommandBase;
  virtual ~DataProcessorPauseCommand(){};

  void execute() override {
    m_presenter->notify(DataProcessorPresenter::PauseFlag);
  };
  QString name() const override { return QString("Pause"); }
  QString icon() const override { return QString("://pause.png"); }
  QString tooltip() const override { return QString("Pause processing runs"); }
  QString whatsthis() const override {
    return QString("Pauses processing any selected runs. Processing may be "
                   "resumed by clicking on the 'Process' button.");
  }
  QString shortcut() const override { return QString(); }
};
}
}
#endif /*MANTIDQTMANTIDWIDGETS_DATAPROCESSORPAUSECOMMAND_H*/
