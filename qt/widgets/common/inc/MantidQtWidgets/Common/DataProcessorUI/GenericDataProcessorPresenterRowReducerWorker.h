// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenter.h"

#include <QThread>
#include <utility>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/**
Worker to run the reduction process for each row for the
GenericDataProcessorPresenter for the GUI it is attached to. It has
a finished() signal which is expected to be emittted when one of
these when the hard/long-work methods finish.
*/
class GenericDataProcessorPresenterRowReducerWorker : public QObject {
  Q_OBJECT

public:
  GenericDataProcessorPresenterRowReducerWorker(GenericDataProcessorPresenter *presenter, RowData_sptr rowData,
                                                const int rowIndex, const int groupIndex)
      : m_presenter(presenter), m_rowData(std::move(rowData)), m_rowIndex(rowIndex), m_groupIndex(groupIndex) {}

private slots:
  void startWorker() {
    try {
      m_presenter->reduceRow(m_rowData);
      m_presenter->m_manager->update(m_groupIndex, m_rowIndex, m_rowData->data());
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
    m_presenter->m_manager->setError(std::string("Row reduction failed: ") + errorMessage, m_rowIndex, m_groupIndex);
    emit reductionErrorSignal(QString::fromStdString(errorMessage));
    emit finished(1);
  }
};

} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
