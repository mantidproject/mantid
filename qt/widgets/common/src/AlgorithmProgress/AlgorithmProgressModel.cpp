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
    {
        // Start capturing the triggers from ALL algorithms starting
        // this allows us to attach an observer to the algorithm to track the
        // start, progress, finish and errors.
        observeStarting();
        progressObserver = std::make_unique<ProgressObserver>(this);
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

    void AlgorithmProgressModel::updateProgress(const Mantid::API::IAlgorithm* alg,
        const double progress,
        const std::string& msg)
    {

        for (auto& presenter : m_presenters) {
            presenter->updateProgressBar(alg, progress, msg);
        }
    }

    void AlgorithmProgressModel::startingHandle(Mantid::API::IAlgorithm_sptr alg)
    {
        progressObserver->observeStart(alg);
        progressObserver->observeProgress(alg);
        progressObserver->observeFinish(alg);
        progressObserver->observeError(alg);
    }

    const std::vector<const Mantid::API::IAlgorithm*>&
    AlgorithmProgressModel::runningAlgorithms()
    {
        return progressObserver->managedAlgs;
    }

    const Mantid::API::IAlgorithm* AlgorithmProgressModel::latestRunningAlgorithm()
    {
        if (!progressObserver->managedAlgs.empty()) {
            return progressObserver->managedAlgs.back();
        } else {
            return nullptr;
        }
    }
} // namespace MantidWidgets
} // namespace MantidQt