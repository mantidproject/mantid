#ifndef MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTER_GENERICDATAPROCESSORPRESENTERREDUCERWORKER_H
#define MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTER_GENERICDATAPROCESSORPRESENTERREDUCERWORKER_H

#include "MantidQtMantidWidgets/DataProcessorUI/GenericDataProcessorPresenter.h"

#include <QThread>

namespace MantidQt {
namespace MantidWidgets {

/**
Worker to run the reduction process for each row for the
GenericDataProcessorPresenter for the GUI it is attached to. It has
a finished() signal, and it is expected to emit it when the
hard/long-work methods finish.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
// EXPORT_OPT_MANTIDQT_MANTIDWIDGETS
class GenericDataProcessorPresenterReducerWorker : public QObject {
  Q_OBJECT

public:
  // for fitting (single peak fits)
  GenericDataProcessorPresenterReducerWorker(
      GenericDataProcessorPresenter *presenter)
      : m_presenter(presenter) {}

private slots:
  void reduceRow() {
    emit finished();
  }

signals:
  void finished();

private:
  GenericDataProcessorPresenter *m_presenter;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTER_GENERICDATAPROCESSORPRESENTERREDUCERWORKER_H
