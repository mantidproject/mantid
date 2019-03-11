// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ALGORITHMPROGRESSDIALOGWIDGET_H
#define ALGORITHMPROGRESSDIALOGWIDGET_H
#include <memory>

#include <QDialog>
#include <QTreeWidget>

#include "AlgorithmProgressDialogPresenter.h"

class QProgressBar;

/**
 * The AlgorithmProgressDialogWidget displays multiple progress bars for algorithms running
 * simultaneously. This widget shares its model with the main Workbench progress bar (AlgorithmProgressWidget).
 */
namespace MantidQt {
namespace MantidWidgets {
    class AlgorithmProgressDialogWidget : public QDialog {
        Q_OBJECT
    public:
        AlgorithmProgressDialogWidget(QWidget* parent, AlgorithmProgressModel& model);

        /// Displays the list of currently running algorithms. New progress bars are added here
        void updateRunningAlgorithms(std::vector<Mantid::API::IAlgorithm_sptr> algorithmsData);

    private:
        void closeEvent(QCloseEvent* event) override;

        std::unique_ptr<AlgorithmProgressDialogPresenter> m_presenter;
        QTreeWidget* m_tree;
    };

} // namespace MantidWidgets
} // namespace MantidQt

#endif // ALGORITHMPROGRESSDIALOGWIDGET_H