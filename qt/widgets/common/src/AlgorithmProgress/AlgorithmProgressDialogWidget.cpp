// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressDialogWidget.h"
#include "MantidQtWidgets/Common/AlgorithmProgress/AlgorithmProgressModel.h"
#include "MantidQtWidgets/Common/MantidWidget.h"
#include <QHeaderView>
#include <QIcon>
#include <QProgressBar>
#include <QPushButton>
#include <QStringList>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace MantidQt {
namespace MantidWidgets {

AlgorithmProgressDialogWidget::AlgorithmProgressDialogWidget(
    QWidget *parent, AlgorithmProgressModel &model)
    : QDialog(parent),
      m_presenter{std::make_unique<AlgorithmProgressDialogPresenter>(
          parent, this, model)},
      m_tree{new QTreeWidget(this)} {

  m_tree->setColumnCount(3);
  m_tree->setSelectionMode(QTreeWidget::NoSelection);
  m_tree->setColumnWidth(0, 220);
  m_tree->setHeaderLabels({"Algorithm", "Progress", ""});
  auto header = m_tree->header();
  header->setSectionResizeMode(1, QHeaderView::Stretch);
  header->setStretchLastSection(false);

  auto buttonLayout = new QHBoxLayout();
  const auto button = new QPushButton("Close", this);

  connect(button, &QPushButton::clicked, this,
          &AlgorithmProgressDialogWidget::close);

  buttonLayout->addStretch();
  buttonLayout->addWidget(button);

  auto layout = new QVBoxLayout();
  layout->addWidget(m_tree);
  layout->addLayout(buttonLayout);

  setLayout(layout);
  setWindowTitle("Mantid - Algorithm Progress");
  setWindowIcon(QIcon(":/MantidPlot_Icon_32offset.png"));
  resize(500, 300);
}

std::pair<QTreeWidgetItem *, QProgressBar *>
AlgorithmProgressDialogWidget::addAlgorithm(Mantid::API::IAlgorithm_sptr alg) {
  const auto name = alg->name();
  const auto properties = alg->getProperties();
  const auto item =
      new QTreeWidgetItem(m_tree, QStringList{QString::fromStdString(name)});

  m_tree->addTopLevelItem(item);
  auto progressBar = new QProgressBar(m_tree);
  progressBar->setAlignment(Qt::AlignHCenter);

  auto cancelButton = new AlgorithmProgressDialogWidgetCancelButton(
      alg, QString::fromStdString("Cancel"), m_tree);

  m_tree->setItemWidget(item, 1, progressBar);
  m_tree->setItemWidget(item, 2, cancelButton);

  for (const auto &prop : properties) {
    const auto &propAsString = prop->value();
    if (!propAsString.empty()) {
      item->addChild(new QTreeWidgetItem(
          item, QStringList{QString::fromStdString(propAsString)}));
    }
  }

  return std::make_pair(item, progressBar);
}

void AlgorithmProgressDialogWidget::closeEvent(QCloseEvent *event) {
  // stops the any incoming signals for widget making/updating/deleting
  // the window is closing so we don't care about processing them
  m_presenter->blockSignals(true);
  QWidget::closeEvent(event);
}

} // namespace MantidWidgets
} // namespace MantidQt
