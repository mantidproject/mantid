// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFView_view.h"

#include <QFileDialog>
#include <QGridLayout>
#include <QLineEdit>
#include <QRegExpValidator>
#include <QSplitter>
#include <QVBoxLayout>

namespace MantidQt {
namespace CustomInterfaces {

ALFView_view::ALFView_view(QWidget *parent)
    : QWidget(parent), m_run(nullptr),
      m_loadRunObservable(nullptr), m_browseObservable(nullptr) {
  QSplitter *MainLayout = new QSplitter(Qt::Vertical, this);
  QWidget *loadBar = new QWidget();
  m_loadRunObservable = new observable();
  m_browseObservable = new observable();
  generateLoadWidget(loadBar);

  MainLayout->addWidget(loadBar);
  //  MainLayout->addWidget(widgetSplitter);
  // this->addWidget(MainLayout);
}

void ALFView_view::generateLoadWidget(QWidget *loadBar) {
  m_run = new QLineEdit("0");
  m_run->setValidator(new QRegExpValidator(QRegExp("[0-9]*"), m_run));
  connect(m_run, SIGNAL(editingFinished()), this, SLOT(runChanged()));

  m_browse = new QPushButton("Browse");
  connect(m_browse, SIGNAL(clicked()), this, SLOT(browse()));

  QHBoxLayout *loadLayout = new QHBoxLayout(loadBar);
  loadLayout->addWidget(m_run);
  loadLayout->addWidget(m_browse);
}

int ALFView_view::getRunNumber() { return m_run->text().toInt(); }

void ALFView_view::setRunQuietly(const QString runNumber) {
  m_run->blockSignals(true);
  m_run->setText(runNumber);
  m_run->blockSignals(false);
}

void ALFView_view::runChanged() {
  m_loadRunObservable->notify();
} // emit newRun(); }

void ALFView_view::browse() {
  auto file = QFileDialog::getOpenFileName(this, "Open a file",
                                           "", "File (*.nxs)");
  if (file.isEmpty()) {
    return;
  }
  //emit browsedToRun(file.toStdString());
  m_browseObservable->notify(file.toStdString());
}

} // namespace CustomInterfaces
} // namespace MantidQt