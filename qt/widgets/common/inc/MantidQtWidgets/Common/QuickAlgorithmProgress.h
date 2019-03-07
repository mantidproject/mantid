// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef QUICKALGPROGRESS_H
#define QUICKALGPROGRESS_H
//----------------------------------
// Includes
//----------------------------------
#include "DllOption.h"
#include "MantidAPI/AlgorithmObserver.h"

#include "MantidAPI/IAlgorithm.h"

#include "MantidQtWidgets/Common/Configurable.h"
#include "MantidQtWidgets/Common/Message.h"
#include "MantidQtWidgets/Common/QtSignalChannel.h"

#include <QHBoxLayout>
#include <QHash>
#include <QProgressBar>
#include <QPushButton>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QWidget>
#include <iostream>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON QuickAlgorithmProgress
    : public QWidget,
      public Mantid::API::AlgorithmObserver {
  Q_OBJECT
public:
  QuickAlgorithmProgress(QWidget *parent = nullptr)
      : QWidget(parent), AlgorithmObserver() {

    this->observeStarting();
    std::cout << "making QuickAlgorithmProgress ..." << std::endl;

    // m_detailsButton = new QPushButton("Details");
    layout = new QHBoxLayout(this);
    pb = new QProgressBar(this);
    pb->setAlignment(Qt::AlignHCenter);
    layout->insertWidget(0, pb);

    layout->addStretch();
    // layout->addWidget(m_detailsButton);
    this->setLayout(layout);

    connect(this, SIGNAL(update_progress_bar(double)), this,
            SLOT(slot_upgrade_progress_bar(double)));
  }

  void set_value(const double val) { pb->setValue(val * 100); }

  void startingHandle(const Mantid::API::IAlgorithm_sptr alg) {
    std::cout << "Starting observing...\n";
    alggggggggggg = alg;
    observeProgress(alg);
    observeFinish(alg);
    observeError(alg);
  }

  void progressHandle(const Mantid::API::IAlgorithm *alg, double val,
                      const std::string &msg) {
    emit update_progress_bar(val, msg);
  }

  void finishHandle() {
    std::cout << "STOPPING observing...\n";

    stopObserving(alggggggggggg);
    pb->reset();
  }

  QHBoxLayout *layout;
  QProgressBar *pb;
  QPushButton *m_detailsButton;
  Mantid::API::IAlgorithm_const_sptr alggggggggggg;

public slots:
  void slot_upgrade_progress_bar(double val) { set_value(val); }
signals:
  void update_progress_bar(double val, std::string msg);
};

} // namespace MantidWidgets
} // namespace MantidQt
#endif