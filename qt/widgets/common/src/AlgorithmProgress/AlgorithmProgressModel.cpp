// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressModel.h"

namespace MantidQt {
namespace MantidWidgets {

    AlgorithmProgressModel::AlgorithmProgressModel()
        : AlgorithmObserver()
        , m_presenters { std::vector<AlgorithmProgressPresenterBase*>() }
        , m_observers { std::vector<std::unique_ptr<ProgressObserver>>() }
    {
        // Start capturing the triggers from ALL algorithms starting
        // this allows us to attach an observer to the algorithm to track the
        // start, progress, finish and errors.
        observeStarting();
    }
    void AlgorithmProgressModel::addPresenter(
        AlgorithmProgressPresenterBase* newPresenter)
    {
        m_presenters.emplace_back(newPresenter);
    }

    void AlgorithmProgressModel::updatePresenters()
    {
        for (auto& presenter : m_presenters) {
            presenter->updateGui();
        }
    }
    void AlgorithmProgressModel::removePresenter(
        const AlgorithmProgressPresenterBase* presenter)
    {
        const auto it = std::find(m_presenters.begin(), m_presenters.end(), presenter);
        if (it != m_presenters.end()) {
            m_presenters.erase(it);
        }
    }
    void AlgorithmProgressModel::removeObserver(ProgressObserver* observer)
    {
        std::lock_guard<std::mutex> guard(*howdoesMutex);
        // find the observer that is being removed in the list of observers,
        // a custom comparison is used to compare the pointer inside the unique ptr
        const auto it = std::find_if(
            m_observers.begin(), m_observers.end(),
            [&observer](const std::unique_ptr<ProgressObserver>& currentObserver) {
                return currentObserver.get() == observer;
            });
        // FIXME can't delete without locking because multiple things can delete simultaneously zzz
        if (it != m_observers.end()) {
            observer->stopObservingCurrentAlgorithm();
            m_observers.erase(it);
            updatePresenters();
        }
    }
    void AlgorithmProgressModel::updateProgress(Mantid::API::IAlgorithm_sptr alg,
        const double progress,
        const std::string& msg)
    {
        std::lock_guard<std::mutex> guard(*howdoesMutex);

        for (auto& presenter : m_presenters) {
            presenter->updateProgressBar(alg, progress, msg);
        }
    }

    void AlgorithmProgressModel::startingHandle(Mantid::API::IAlgorithm_sptr alg)
    {
        std::lock_guard<std::mutex> guard(*howdoesMutex);

        progressObserver = std::make_unique<ProgressObserver>(this, alg);
        progressObserver->observeProgress(alg);
        progressObserver->observeFinish(alg);
        progressObserver->observeError(alg);
        // give ownership to the vector, when the observer
        // is removed from it, it will be freed
        //m_observers.emplace_back(std::move(progressObserver));
        updatePresenters();
    }

    std::vector<Mantid::API::IAlgorithm_sptr>
    AlgorithmProgressModel::runningAlgorithms()
    {
        std::lock_guard<std::mutex> guard(*howdoesMutex);

        auto runningAlgs { std::vector<Mantid::API::IAlgorithm_sptr>() };
        runningAlgs.reserve(m_observers.size());

        for (const auto& obs : m_observers) {
            runningAlgs.emplace_back(obs->currentAlgorithm());
        }

        return runningAlgs;
    }

    Mantid::API::IAlgorithm_sptr AlgorithmProgressModel::latestRunningAlgorithm()
    {
        std::lock_guard<std::mutex> guard(*howdoesMutex);

        if (!m_observers.empty()) {
            return m_observers.back()->currentAlgorithm();
        } else {
            return nullptr;
        }
    }
} // namespace MantidWidgets
} // namespace MantidQt