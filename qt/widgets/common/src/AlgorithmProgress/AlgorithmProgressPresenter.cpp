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
        , view(view)
        , model { AlgorithmProgressModel() }
    {
        model.addPresenter(this);
    }

    void AlgorithmProgressPresenter::setCurrentAlgorithm()
    {
        algorithm = model.latestRunningAlgorithm();
    }

    void AlgorithmProgressPresenter::updateProgressBar(
        Mantid::API::IAlgorithm_sptr alg,
        const double progress,
        const std::string& message)
    {
        if (alg == this->algorithm) {
            emit progressBarNeedsUpdating(view->pb, progress, message);
        }
    }

} // namespace MantidWidgets
} // namespace MantidQt
