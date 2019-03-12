// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenter.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressWidget.h"

namespace MantidQt {
namespace MantidWidgets {
    AlgorithmProgressPresenter::AlgorithmProgressPresenter(
        QWidget* parent,
        AlgorithmProgressWidget* view)
        : AlgorithmProgressPresenterBase(parent)
        , m_model { AlgorithmProgressModel() }
        , m_view(view)
    {
        m_model.addPresenter(this);
    }

    void AlgorithmProgressPresenter::setCurrentAlgorithm()
    {
        m_algorithm = m_model.latestRunningAlgorithm();
        if (!m_algorithm) {
            m_view->algorithmEnded();
        } else {
            m_view->algorithmStarted();
        }
    }

    void AlgorithmProgressPresenter::updateProgressBar(
        const Mantid::API::IAlgorithm* algorithm, const double progress,
        const std::string& message)
    {
        if (algorithm == this->m_algorithm) {
            emit progressBarNeedsUpdating(m_view->progressBar(), progress, message);
        }
    }

} // namespace MantidWidgets
} // namespace MantidQt
