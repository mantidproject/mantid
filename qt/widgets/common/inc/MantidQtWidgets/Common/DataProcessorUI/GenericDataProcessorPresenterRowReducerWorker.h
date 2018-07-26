#ifndef MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTER_GENERICDATAPROCESSORPRESENTERROWREDUCERWORKER_H
#define MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTER_GENERICDATAPROCESSORPRESENTERROWREDUCERWORKER_H

#include "MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenter.h"

#include <QThread>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

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
      GenericDataProcessorPresenter *presenter, RowData_sptr rowData,
      const int rowIndex, const int groupIndex)
      : m_presenter(presenter), m_rowData(rowData), m_rowIndex(rowIndex),
        m_groupIndex(groupIndex) {}

private slots:
  void startWorker() {
    try {
      m_presenter->reduceRow(m_rowData);
      m_presenter->m_manager->update(m_groupIndex, m_rowIndex,
                                     m_rowData->data());
      m_presenter->m_manager->setProcessed(true, m_rowIndex, m_groupIndex);
      emit finished(0);
    } catch (std::exception &ex) {
      handleError(ex.what());
    } catch (...) {
      handleError("Unexpected exception");
    }
  }

signals:
  void finished(const int exitCode);
  void reductionErrorSignal(QString ex);

private:
  GenericDataProcessorPresenter *m_presenter;
  RowData_sptr m_rowData;
  int m_rowIndex;
  int m_groupIndex;

  void handleError(const std::string &errorMessage) {
    m_presenter->m_manager->setProcessed(true, m_rowIndex, m_groupIndex);
    m_presenter->m_manager->setError(std::string("Row reduction failed: ") +
                                         errorMessage,
                                     m_rowIndex, m_groupIndex);
    emit reductionErrorSignal(QString::fromStdString(errorMessage));
    emit finished(1);
  }
};

} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQTMANTIDWIDGETS_GENERICDATAPROCESSORPRESENTER_GENERICDATAPROCESSORPRESENTERROWREDUCERWORKER_H
