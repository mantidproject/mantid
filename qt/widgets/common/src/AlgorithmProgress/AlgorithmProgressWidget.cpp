// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressWidget.h"

namespace MantidQt {
namespace MantidWidgets {

    AlgorithmProgressWidget::AlgorithmProgressWidget(QWidget* parent)
        : QWidget(parent)
        , presenter { std::make_unique<AlgorithmProgressPresenter>(parent, this) }
    {
        setAttribute(Qt::WA_DeleteOnClose);

        m_detailsButton = new QPushButton("Details");
        layout = new QHBoxLayout(this);
        pb = new QProgressBar(this);
        pb->setAlignment(Qt::AlignHCenter);
        layout->addWidget(pb);
        layout->addWidget(m_detailsButton);
        this->setLayout(layout);
    }
} // namespace MantidWidgets
} // namespace MantidQt