// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenterBase.h"
#include <QProgressBar>

namespace MantidQt {
namespace MantidWidgets {

    AlgorithmProgressPresenterBase::AlgorithmProgressPresenterBase(QWidget* parent)
        : QWidget(parent)
    {
        connect(this, &AlgorithmProgressPresenterBase::updateWatchedAlgorithm, this,
            &AlgorithmProgressPresenterBase::setCurrentAlgorithm, Qt::QueuedConnection);

        connect(this, &AlgorithmProgressPresenterBase::progressBarNeedsUpdating, this,
            &AlgorithmProgressPresenterBase::setProgressBar, Qt::QueuedConnection);
    }

    void AlgorithmProgressPresenterBase::updateGui()
    {
        emit updateWatchedAlgorithm();
    }

    void AlgorithmProgressPresenterBase::setProgressBar(
        QProgressBar* progressBar, double progress, const std::string& message)
    {
        progressBar->setValue(static_cast<int>(progress * 100));
        if (!message.empty()) {
            progressBar->setFormat(QString("%1 - %2").arg(QString::fromStdString(message), "%p%"));
        } else {
            progressBar->setFormat("%p%");
        }
    }
} // namespace MantidWidgets
} // namespace MantidQt