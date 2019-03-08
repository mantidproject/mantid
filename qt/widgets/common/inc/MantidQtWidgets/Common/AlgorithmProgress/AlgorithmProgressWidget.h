// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ALGORITHMPROGRESSWIDGET_H
#define ALGORITHMPROGRESSWIDGET_H
//----------------------------------
// Includes
//----------------------------------
#include "MantidAPI/AlgorithmObserver.h"

#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/Message.h"

#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressDialogWidget.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenter.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QWidget>
#include <memory>

class QProgressBar;
namespace MantidQt {
namespace MantidWidgets {
    class EXPORT_OPT_MANTIDQT_COMMON AlgorithmProgressWidget : public QWidget {
        Q_OBJECT
    public:
        AlgorithmProgressWidget(QWidget* parent = nullptr);

        QProgressBar* pb;
        void deleteDetailsDialog();

    public slots:
        void showDetailsDialog();

    private:
        QHBoxLayout* layout;
        QPushButton* m_detailsButton;
        Mantid::API::IAlgorithm_const_sptr alggggggggggg;

        std::unique_ptr<AlgorithmProgressDialogWidget> details;
        std::unique_ptr<AlgorithmProgressPresenter> presenter;
    };

} // namespace MantidWidgets
} // namespace MantidQt
#endif // ALGORITHMPROGRESSWIDGET_H