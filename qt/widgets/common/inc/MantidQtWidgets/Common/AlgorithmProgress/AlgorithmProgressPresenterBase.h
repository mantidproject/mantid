// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ALGORITHMPROGRESSPRESENTERBASE_H
#define ALGORITHMPROGRESSPRESENTERBASE_H

#include "MantidAPI/Algorithm.h"
#include <QWidget>
#include <string>

namespace Mantid {
namespace API {
    class IAlgorithm;
}
} // namespace Mantid

class QProgressBar;

/**
 * The AlgorithmProgressPresenterBase is the base class that presenters showing progress bars use.
 * It sets up the common connections for events from the model, e.g. when an algorithm has progressed
 * and the progress bar value needs to be updated.
 */

namespace MantidQt {
namespace MantidWidgets {
    class ProgressObserverData;
    class AlgorithmProgressPresenterBase : public QWidget {
        Q_OBJECT
    public:
        using AlgorithmInfo = QList<std::tuple<Mantid::API::AlgorithmID, std::string, std::vector<Mantid::Kernel::Property*>>>;
        AlgorithmProgressPresenterBase(QWidget* parent);

        /// Signals to the presenters that an algorithm has started.
        void algorithmStarted(Mantid::API::AlgorithmID);
        /// Signals to the presenters that an algorithm has ended.
        void algorithmEnded(Mantid::API::AlgorithmID);
        /// Signals to the presenters that there has been progress in one of the algorithms
        void updateProgressBar(Mantid::API::AlgorithmID, double, const std::string&);
        /// Sets the parameter progress bar to show the progress and message
        void setProgressBar(QProgressBar*, double, QString);

    public slots:
        virtual void algorithmStartedSlot(Mantid::API::AlgorithmID) = 0;
        virtual void updateProgressBarSlot(Mantid::API::AlgorithmID, double, QString) = 0;
        virtual void algorithmEndedSlot(Mantid::API::AlgorithmID) = 0;

    signals:
        // When there has been a change in the observers, this signal
        // will trigger an update of the current algorithm being watched
        void algorithmStartedSignal(Mantid::API::AlgorithmID);
        void updateProgressBarSignal(Mantid::API::AlgorithmID, double, QString);
        void algorithmEndedSignal(Mantid::API::AlgorithmID);
    };
} // namespace MantidWidgets
} // namespace MantidQt

#endif // ALGORITHMPROGRESSPRESENTERBASE_H