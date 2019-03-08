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
namespace MantidQt {
namespace MantidWidgets {
    class AlgorithmProgressDialogWidget : public QDialog {
        Q_OBJECT
    public:
        AlgorithmProgressDialogWidget(QWidget* parent, AlgorithmProgressModel& model);
        void updateRunningAlgorithms(std::vector<Mantid::API::IAlgorithm_sptr> algorithmsData);

        void closeEvent(QCloseEvent* event) override;

    private:
        std::unique_ptr<AlgorithmProgressDialogPresenter> presenter;
        QTreeWidget* tree;
    };

} // namespace MantidWidgets
} // namespace MantidQt

#endif // ALGORITHMPROGRESSDIALOGWIDGET_H