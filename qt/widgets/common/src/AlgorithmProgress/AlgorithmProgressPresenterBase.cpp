#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenterBase.h"
#include <QProgressBar>
#include <iostream>

namespace MantidQt {
namespace MantidWidgets {

    AlgorithmProgressPresenterBase::AlgorithmProgressPresenterBase(QWidget* parent)
        : QWidget(parent)
    {
        connect(this, &AlgorithmProgressPresenterBase::updateWatchedAlgorithm, this,
            &AlgorithmProgressPresenterBase::setCurrentAlgorithm);

        connect(this, &AlgorithmProgressPresenterBase::progressBarNeedsUpdating, this,
            &AlgorithmProgressPresenterBase::setProgressBar);
    }

    void AlgorithmProgressPresenterBase::updateGui()
    {
        emit updateWatchedAlgorithm();
    }

    void AlgorithmProgressPresenterBase::setProgressBar(
        QProgressBar* progressBar, double progress, const std::string& message)
    {
        progressBar->setValue(static_cast<int>(progress * 100));
    }
} // namespace MantidWidgets
} // namespace MantidQt