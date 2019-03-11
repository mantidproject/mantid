// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ALGORITHMPROGRESSMODEL_H
#define ALGORITHMPROGRESSMODEL_H

#include "MantidAPI/AlgorithmObserver.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressObserver.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenterBase.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

/**
 * The AlgorithmProgressModel keeps track of all active presenters that are managing
 * views with progress bars, and notifies them whenever they need to update due to
 * an algorithm's event. It tracks the starting of algorithms, and will create
 * ProgressObservers for each algorithm that starts.
 */
namespace MantidQt {
namespace MantidWidgets {
    class AlgorithmProgressModel : public Mantid::API::AlgorithmObserver {
    public:
        AlgorithmProgressModel();

        /// Add a new presenter that is displaying a progress bar
        void addPresenter(AlgorithmProgressPresenterBase* presenter);
        /// Remove a new presenter to stop it from getting updates for algorithm progress
        void removePresenter(const AlgorithmProgressPresenterBase* presenter);
        /// Update the progress bar in all live presenters
        void updatePresenters();
        /// Remove a currently active observer
        void removeObserver(ProgressObserver* observer);
        /// Update the progress using data from the observer
        void updateProgress(Mantid::API::IAlgorithm_sptr, double progress,
            const std::string& msg);

        /// Start an observer that tracks the execution of the parameter algorithm
        void startingHandle(Mantid::API::IAlgorithm_sptr alg) override;

        std::vector<Mantid::API::IAlgorithm_sptr> runningAlgorithms();
        Mantid::API::IAlgorithm_sptr latestRunningAlgorithm();

    private:
        std::unique_ptr<std::mutex> howdoesMutex = std::make_unique<std::mutex>();
        std::vector<AlgorithmProgressPresenterBase*> m_presenters;
        std::vector<std::unique_ptr<ProgressObserver>> m_observers;
    };

} // namespace MantidWidgets
} // namespace MantidQt

#endif // ALGORITHMPROGRESSMODEL_H