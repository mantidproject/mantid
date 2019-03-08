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
        , presenters { std::vector<AlgorithmProgressPresenterBase*>() }
        , observers { std::vector<std::unique_ptr<ProgressObserver>>() }
    {
        // Start capturing the triggers from ALL algorithms starting
        // this allows us to attach an observer to the algorithm to track the
        // start, progress, finish and errors.
        observeStarting();
    }
    void AlgorithmProgressModel::addPresenter(
        AlgorithmProgressPresenterBase* newPresenter)
    {
        presenters.emplace_back(newPresenter);
    }

    void AlgorithmProgressModel::updatePresenters()
    {
        for (auto& presenter : presenters) {
            presenter->updateGui();
        }
    }
    void AlgorithmProgressModel::removePresenter(
        const AlgorithmProgressPresenterBase* presenter)
    {
        const auto it = std::find(presenters.begin(), presenters.end(), presenter);
        if (it != presenters.end()) {
            presenters.erase(it);
        }
    }
    void AlgorithmProgressModel::removeObserver(ProgressObserver* observer)
    {
        // find the observer that is being removed in the list of observers,
        // a custom comparison is used to compare the pointer inside the unique ptr
        const auto it = std::find_if(
            observers.begin(), observers.end(),
            [&observer](const std::unique_ptr<ProgressObserver>& currentObserver) {
                return currentObserver.get() == observer;
            });

        if (it != observers.end()) {
            observer->stopObservingCurrentAlgorithm();
            observers.erase(it);
            updatePresenters();
        }
    }
    void AlgorithmProgressModel::updateProgress(Mantid::API::IAlgorithm_sptr alg,
        const double progress,
        const std::string& msg)
    {
        for (auto& presenter : presenters) {
            presenter->updateProgressBar(alg, progress, msg);
        }
    }

    void AlgorithmProgressModel::startingHandle(Mantid::API::IAlgorithm_sptr alg)
    {
        auto po = std::make_unique<ProgressObserver>(this, alg);
        po->observeProgress(alg);
        po->observeFinish(alg);
        po->observeError(alg);
        std::cout << "Algorithms starting: " << alg->name() << '\n';
        // give ownership to the vector, when the observer
        // is removed from it, it will be freed
        observers.emplace_back(std::move(po));
        updatePresenters();
    }

    std::vector<Mantid::API::IAlgorithm_sptr>
    AlgorithmProgressModel::runningAlgorithms() const
    {
        auto vec { std::vector<Mantid::API::IAlgorithm_sptr>() };
        vec.reserve(observers.size());

        for (const auto& obs : observers) {
            vec.emplace_back(obs->currentAlgorithm());
        }

        return vec;
    }

    Mantid::API::IAlgorithm_sptr
    AlgorithmProgressModel::latestRunningAlgorithm() const
    {
        if (!observers.empty()) {
            return observers.back()->currentAlgorithm();
        } else {
            return nullptr;
        }
    }
} // namespace MantidWidgets
} // namespace MantidQt