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

namespace MantidQt {
namespace MantidWidgets {
    class AlgorithmProgressModel;
    class AlgorithmProgressDialogWidget;

    class AlgorithmProgressDialogPresenter : public AlgorithmProgressPresenterBase {
        Q_OBJECT
    public:
        AlgorithmProgressDialogPresenter(QWidget* parent, AlgorithmProgressDialogWidget* view, AlgorithmProgressModel& model);

        void setCurrentAlgorithm() override;

        /// Add a progress bar that this presenter will update.
        /// The presenter does NOT own the progress bar
        void addProgressBar(QProgressBar* pb);
        void cancelAlgorithm(Mantid::API::IAlgorithm_sptr algorithm) const;
        void removeFromModel();

        /// Updates the list of progress bar currently managed by the presenter
        void updateProgressBar(Mantid::API::IAlgorithm_sptr, double progress, const std::string& message) override;

    private:
        AlgorithmProgressDialogWidget* view;
        AlgorithmProgressModel& model;
        std::vector<QProgressBar*> progressBars;
    };
} // namespace MantidWidgets
} // namespace MantidQt

#endif // ALGORITHMPROGRESSDIALOGPRESENTER_H