// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DataProcessorUI/GenericDataProcessorPresenter.h"

#include <QThread>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/**
Worker to run the reduction process for each group of the
GenericDataProcessorPresenter for the GUI it is attached to. It has
a finished() signal which is expected to be emittted when one of
these when the hard/long-work methods finish.
*/
class GenericDataProcessorPresenterGroupReducerWorker : public QObject {
  Q_OBJECT

public:
  GenericDataProcessorPresenterGroupReducerWorker(GenericDataProcessorPresenter *presenter, const GroupData &groupData,
                                                  int groupIndex)
      : m_presenter(presenter), m_groupData(groupData), m_groupIndex(groupIndex) {}

private slots:
  void startWorker() {
    try {
      m_presenter->postProcessGroup(m_groupData);
      // Group is set processed if all constituent rows are processed
      if (m_presenter->m_manager->rowCount(m_groupIndex) == static_cast<int>(m_groupData.size()))
        m_presenter->m_manager->setProcessed(true, m_groupIndex);
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
  const GroupData m_groupData;
  int m_groupIndex;

  void handleError(const std::string &errorMessage) {
    m_presenter->m_manager->setError(std::string("Group processing failed: ") + errorMessage, m_groupIndex);
    if (m_presenter->m_manager->rowCount(m_groupIndex) == static_cast<int>(m_groupData.size()))
      m_presenter->m_manager->setProcessed(true, m_groupIndex);
    emit reductionErrorSignal(QString::fromStdString(errorMessage));
    emit finished(1);
  }
};
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt