// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFView_view.h"

#include <QMessageBox>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QVBoxLayout>

namespace MantidQt {
namespace CustomInterfaces {

ALFView_view::ALFView_view(const std::string &instrument, QWidget *parent)
    : QSplitter(Qt::Vertical, parent), m_loadRunObservable(nullptr),
      m_files(nullptr), m_instrument(QString::fromStdString(instrument)) {
  auto loadWidget = generateLoadWidget();

  this->addWidget(loadWidget);
}

QWidget *ALFView_view::generateLoadWidget() {
  m_loadRunObservable = new Observable();

  m_files = new API::MWRunFiles(this);
  m_files->setLabelText(m_instrument);
  m_files->allowMultipleFiles(false);
  m_files->setInstrumentOverride(m_instrument);
  m_files->isForRunFiles(true);
  connect(m_files, SIGNAL(fileFindingFinished()), this, SLOT(fileLoaded()));

  auto loadWidget = new QWidget();
  auto loadLayout = new QHBoxLayout(loadWidget);

  loadLayout->addItem(
      new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
  loadLayout->addWidget(m_files);
  loadLayout->addItem(
      new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

  return loadWidget;
}

std::string ALFView_view::getFile() {
  auto name = m_files->getFilenames();
  if (name.size() > 0)
    return name[0].toStdString();
  return "";
}

void ALFView_view::setRunQuietly(const std::string &runNumber) {
  m_files->setText(QString::fromStdString(runNumber));
}

void ALFView_view::fileLoaded() {
  if (m_files->getText().isEmpty())
    return;

  if (!m_files->isValid()) {
    warningBox(m_files->getFileProblem());
    return;
  }
  m_loadRunObservable->notify();
}

void ALFView_view::warningBox(const std::string &message) {
  warningBox(QString::fromStdString(message));
}
void ALFView_view::warningBox(const QString &message) {
  QMessageBox::warning(this, m_instrument + " view", message);
}

} // namespace CustomInterfaces
} // namespace MantidQt