// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressDialogPresenter.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressDialogWidget.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressModel.h"
#include <boost/detail/compressed_pair.hpp>

namespace MantidQt {
namespace MantidWidgets {

    AlgorithmProgressDialogPresenter::AlgorithmProgressDialogPresenter(QWidget* parent, AlgorithmProgressDialogWidget* view, AlgorithmProgressModel& model)
        : AlgorithmProgressPresenterBase(parent)
        , m_view { view }
        , m_model { model }
        , m_progressBars { new std::unordered_map<Mantid::API::IAlgorithm*, std::pair<Mantid::API::IAlgorithm_sptr, QProgressBar*>>() }
    {
        model.addPresenter(this);
    }

    void AlgorithmProgressDialogPresenter::setCurrentAlgorithm()
    {
        // clear all held progress bars, this container owns nothing so nothing is leaked
        m_progressBars->clear();
        m_view->updateRunningAlgorithms(m_model.runningAlgorithms());
    }

    void AlgorithmProgressDialogPresenter::addProgressBar(Mantid::API::IAlgorithm_sptr alg, QProgressBar* pb)
    {
        m_progressBars->insert(std::make_pair(alg.get(), std::make_pair(alg, pb)));
    }

    void AlgorithmProgressDialogPresenter::removeFromModel()
    {
        m_model.removePresenter(this);
    }

    void AlgorithmProgressDialogPresenter::updateProgressBar(Mantid::API::IAlgorithm_sptr alg, double progress, const std::string& message)
    {
        emit progressBarNeedsUpdating(m_progressBars->at(alg.get()).second, progress, message);
    }
}
}
