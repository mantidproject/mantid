// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressDialogPresenter.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressDialogWidget.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressModel.h"

namespace MantidQt {
namespace MantidWidgets {

    AlgorithmProgressDialogPresenter::AlgorithmProgressDialogPresenter(QWidget* parent, AlgorithmProgressDialogWidget* view, AlgorithmProgressModel& model)
        : AlgorithmProgressPresenterBase(parent)
        , view { view }
        , model { model }
        , progressBars { std::vector<QProgressBar*>() }
    {
        model.addPresenter(this);
    }

    void AlgorithmProgressDialogPresenter::setCurrentAlgorithm()
    {
        progressBars.clear();
        view->updateRunningAlgorithms(model.runningAlgorithms());
    }

    void AlgorithmProgressDialogPresenter::addProgressBar(QProgressBar* pb)
    {
        progressBars.emplace_back(pb);
    }

    void AlgorithmProgressDialogPresenter::cancelAlgorithm(Mantid::API::IAlgorithm_sptr algorithm) const
    {
        algorithm->cancel();
    }

    void AlgorithmProgressDialogPresenter::removeFromModel()
    {
        model.removePresenter(this);
    }

    void AlgorithmProgressDialogPresenter::updateProgressBar(Mantid::API::IAlgorithm_sptr, double progress, const std::string& message)
    {
        for (const auto pb : progressBars) {
            emit progressBarNeedsUpdating(pb, progress, message);
        }
    }
}
}
