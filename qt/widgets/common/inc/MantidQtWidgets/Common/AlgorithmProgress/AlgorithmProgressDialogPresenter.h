// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ALGORITHMPROGRESSDIALOGPRESENTER_H
#define ALGORITHMPROGRESSDIALOGPRESENTER_H
#include "AlgorithmProgressModel.h"
#include "AlgorithmProgressPresenterBase.h"
#include "MantidAPI/IAlgorithm.h"
#include <unordered_map>

/**
 * The AlgorithmProgressDialogPresenter keeps track of the active progress bars that need updating whenever their
 * algorithms report any progress.
 */
namespace MantidQt {
namespace MantidWidgets {
    class AlgorithmProgressModel;
    class AlgorithmProgressDialogWidget;

    class AlgorithmProgressDialogPresenter : public AlgorithmProgressPresenterBase {
        Q_OBJECT
    public:
        AlgorithmProgressDialogPresenter(QWidget* parent, AlgorithmProgressDialogWidget* view, AlgorithmProgressModel& model);

        /// Sets the current algorithms being show in the window
        void setCurrentAlgorithm() override;

        /// Add a progress bar that this presenter will update.
        /// The presenter does NOT own the progress bar
        void addProgressBar(const Mantid::API::IAlgorithm* alg, QProgressBar* pb);
        /// Removes this presenter from the model, so that it can stop getting
        /// updates for algorithms' progress
        void removeFromModel();

        /// Updates the list of progress bar currently managed by the presenter
        void updateProgressBar(const Mantid::API::IAlgorithm* alg, double progress, const std::string& message) override;

    private:
        AlgorithmProgressDialogWidget* m_view;
        /// Reference to the model of the visible progress bar
        AlgorithmProgressModel& m_model;
        /// Container for all the progress bar that are currently being displayed
        /// This container does NOT own any of the progress bars
        std::unordered_map<const Mantid::API::IAlgorithm*, QProgressBar*> m_progressBars;

        std::unique_ptr<std::mutex> howdoesMutex = std::make_unique<std::mutex>();
    };
} // namespace MantidWidgets
} // namespace MantidQt

#endif // ALGORITHMPROGRESSDIALOGPRESENTER_H