#ifndef MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTER_GENERICDATAPROCESSORPRESENTERROWREDUCERWORKER_H
#define MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTER_GENERICDATAPROCESSORPRESENTERROWREDUCERWORKER_H

#include "MantidQtMantidWidgets/DataProcessorUI/GenericDataProcessorPresenter.h"

#include <QThread>

namespace MantidQt {
namespace MantidWidgets {

/**
Worker to run the reduction process for each row for the
GenericDataProcessorPresenter for the GUI it is attached to. It has
a finished() signal which is expected to be emittted when one of
these when the hard/long-work methods finish.

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
class GenericDataProcessorPresenterRowReducerWorker : public QObject {
  Q_OBJECT

public:
  GenericDataProcessorPresenterRowReducerWorker(
      GenericDataProcessorPresenter *presenter, const RowData &rowData)
      : m_presenter(presenter), m_rowData(rowData) {}

private slots:
  void processRow() {
    emit finished();
  }

signals:
  void finished();

private:
  GenericDataProcessorPresenter *m_presenter;
  const RowData m_rowData;
  const GroupData m_groupData;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTER_GENERICDATAPROCESSORPRESENTERROWREDUCERWORKER_H
