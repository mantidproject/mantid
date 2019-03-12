// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ALGORITHMPROGRESSPRESENTER_H
#define ALGORITHMPROGRESSPRESENTER_H
#include "AlgorithmProgressModel.h"
#include "AlgorithmProgressPresenterBase.h"
#include <QWidget>

/**
 * The AlgorithmProgressPresenter is the presenter for the progress bar always visible on the Workbench.
 * It will update the progress bar with the latest algorithm that has been run. If there are two or more
 * running simultaneously, only the latest one's progress will be displayed. More than one progress bar is
 * handled by the AlgorithmProgressDialogWidget.
 */
namespace MantidQt {
namespace MantidWidgets {
    class AlgorithmProgressWidget;

    class AlgorithmProgressPresenter : public AlgorithmProgressPresenterBase {
        Q_OBJECT

    public:
        AlgorithmProgressPresenter(QWidget* parent, AlgorithmProgressWidget*);

        void setCurrentAlgorithm() override;

        /// Updates the visible progress bar on the workbench
        void updateProgressBar(const Mantid::API::IAlgorithm* algorithm, double progress,
            const std::string& message) override;

        constexpr AlgorithmProgressModel& model()
        {
            return m_model;
        }

    private:
        AlgorithmProgressModel m_model;
        // The algorithm for which a progress bar is currently being controlled
        const Mantid::API::IAlgorithm* m_algorithm;
        // The creator of the view also owns the view (Python), not this presenter.
        AlgorithmProgressWidget* m_view;
    };
} // namespace MantidWidgets
} // namespace MantidQt

#endif // ALGORITHMPROGRESSPRESENTER_H