#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenterBase.h"
#include <QProgressBar>

namespace MantidQt {
namespace MantidWidgets {

AlgorithmProgressPresenterBase::AlgorithmProgressPresenterBase() : QObject() {
  connect(this, SIGNAL(updateWatchedAlgorithm()), this,
          SLOT(setCurrentAlgorithm()));
  connect(this,
          SIGNAL(progressBarNeedsUpdating(QProgressBar *, double,
                                          const std::string &)),
          this, SLOT(setProgressBar(*, double, const std::string &)));
}

void AlgorithmProgressPresenterBase::update() { emit updateWatchedAlgorithm(); }

void AlgorithmProgressPresenterBase::setProgressBar(
    QProgressBar *progressBar, const double progress,
    const std::string &message) {
  progressBar->setValue(static_cast<int>(progress * 100));
}
} // namespace MantidWidgets
} // namespace MantidQt