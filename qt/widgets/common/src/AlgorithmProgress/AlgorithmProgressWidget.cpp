// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <QProgressBar>

#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressDialogWidget.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressPresenter.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressWidget.h"

namespace MantidQt {
namespace MantidWidgets {

    AlgorithmProgressWidget::AlgorithmProgressWidget(QWidget* parent)
        : QWidget(parent)
        , pb { new QProgressBar(this) }
        , layout { new QHBoxLayout(this) }
        , m_detailsButton { new QPushButton("Details") }
        , presenter { std::make_unique<AlgorithmProgressPresenter>(parent, this) }

    {
        setAttribute(Qt::WA_DeleteOnClose);
        pb->setAlignment(Qt::AlignHCenter);
        layout->addWidget(pb);
        layout->addWidget(m_detailsButton);
        this->setLayout(layout);

        connect(m_detailsButton, &QPushButton::clicked, this, &AlgorithmProgressWidget::showDetailsDialog);
    }

    void AlgorithmProgressWidget::deleteDetailsDialog()
    {
        // TODO delete the damn details somehow
        delete details;
    }
    void AlgorithmProgressWidget::showDetailsDialog()
    {
        if (!details) {
            details = std::make_unique<AlgorithmProgressDialogWidget>(dynamic_cast<QWidget*>(this->parent()), presenter->model);
            details->show();
        } else {
            details->show();
        }
    }

} // namespace MantidWidgets
} // namespace MantidQt