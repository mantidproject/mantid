// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressDialogWidget.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressModel.h"

#include "MantidQtWidgets/Common/MantidWidget.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QProgressBar>
#include <QPushButton>
#include <QStringList>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace MantidQt {
namespace MantidWidgets {

    AlgorithmProgressDialogWidget::AlgorithmProgressDialogWidget(QWidget* parent, AlgorithmProgressModel& model)
        : QDialog(parent)
        , presenter { std::make_unique<AlgorithmProgressDialogPresenter>(parent, this, model) }
        , tree { new QTreeWidget(this) }
    {
        setAttribute(Qt::WA_DeleteOnClose);

        tree->setColumnCount(3);
        tree->setSelectionMode(QTreeWidget::NoSelection);
        tree->setColumnWidth(0, 220);
        tree->setHeaderLabels({ "Algorithm", "Progress", "" });
        auto header = tree->header();
        header->setSectionResizeMode(1, QHeaderView::Stretch);
        header->setSectionResizeMode(2, QHeaderView::Fixed);
        header->setStretchLastSection(false);

        auto buttonLayout = new QHBoxLayout();
        const auto button = new QPushButton("Close", this);
        buttonLayout->addStretch();
        buttonLayout->addWidget(button);

        auto layout = new QVBoxLayout();
        layout->addWidget(tree);
        layout->addLayout(buttonLayout);

        setLayout(layout);
        setWindowTitle("Mantid - Algorithm Progress");
        setWindowIcon(QIcon(":/MantidPlot_Icon_32offset.png"));
        resize(500, 300);

        presenter->updateGui();
    }

    void AlgorithmProgressDialogWidget::updateRunningAlgorithms(std::vector<Mantid::API::IAlgorithm_sptr> algorithms)
    {
        tree->clear();
        for (const auto& alg : algorithms) {
            const auto item = new QTreeWidgetItem(QStringList { QString::fromStdString(alg->name()) });

            tree->addTopLevelItem(item);
            auto pb = new QProgressBar(tree);
            pb->setAlignment(Qt::AlignHCenter);
            presenter->addProgressBar(pb);

            // OH BOY does this work? inb4 undef behaviour for accessing a deleted variable
            const auto cancel_btn = new QPushButton("Cancel", tree);
            connect(cancel_btn, &QPushButton::clicked, [=]() {
                alg->cancel();
            });

            tree->setItemWidget(item, 1, pb);
            tree->setItemWidget(item, 2, cancel_btn);

            for (const auto& prop : alg->getProperties()) {
                const auto& propAsString = prop->value();
                if (!propAsString.empty()) {
                    item->addChild(new QTreeWidgetItem(QStringList(QString::fromStdString(propAsString))));
                }
            }
        }
    }

    void AlgorithmProgressDialogWidget::closeEvent(QCloseEvent* event)
    {
        presenter->removeFromModel();
        QDialog::closeEvent(event);
    }
}
}
