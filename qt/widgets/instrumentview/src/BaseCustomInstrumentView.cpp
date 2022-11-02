// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentView.h"

#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentPresenter.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"

#include <QMessageBox>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QSplitter>
#include <QVBoxLayout>

namespace MantidQt::MantidWidgets {

BaseCustomInstrumentView::BaseCustomInstrumentView(const std::string &instrument, QWidget *parent)
    : QSplitter(Qt::Vertical, parent), m_helpPage(""), m_files(nullptr),
      m_instrument(QString::fromStdString(instrument)), m_instrumentWidget(nullptr), m_help(nullptr) {
  auto loadWidget = generateLoadWidget();
  this->addWidget(loadWidget);
}

void BaseCustomInstrumentView::subscribePresenter(MantidQt::MantidWidgets::BaseCustomInstrumentPresenter *presenter) {
  m_presenter = presenter;
}

void BaseCustomInstrumentView::setUpInstrument(
    const std::string &fileName, std::vector<std::function<bool(std::map<std::string, bool>)>> &instrument) {

  (void)instrument;
  auto instrumentWidget = new InstrumentWidget(QString::fromStdString(fileName));
  instrumentWidget->hideHelp();
  setInstrumentWidget(instrumentWidget);
}

QWidget *BaseCustomInstrumentView::generateLoadWidget() {
  m_files = new API::FileFinderWidget(this);
  m_files->setLabelText(m_instrument);
  m_files->allowMultipleFiles(false);
  m_files->setInstrumentOverride(m_instrument);
  m_files->isForRunFiles(true);
  connect(m_files, SIGNAL(fileFindingFinished()), this, SLOT(fileLoaded()));

  auto loadWidget = new QWidget();
  auto loadLayout = new QHBoxLayout(loadWidget);

  loadLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
  loadLayout->addWidget(m_files);
  loadLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

  return loadWidget;
}

void BaseCustomInstrumentView::setupHelp() {
  auto *helpWidget = new QWidget();
  m_help = new QPushButton("?");
  m_help->setMaximumWidth(25);
  auto helpLayout = new QHBoxLayout(helpWidget);
  helpLayout->addWidget(m_help);

  helpLayout->addItem(new QSpacerItem(1000, 20, QSizePolicy::Expanding, QSizePolicy::Expanding));
  this->addWidget(helpWidget);
  connect(m_help, SIGNAL(clicked()), this, SLOT(openHelp()));
}

void BaseCustomInstrumentView::openHelp() {
  if (m_helpPage == "") {
    return;
  }
  MantidQt::API::HelpWindow::showCustomInterface(QString::fromStdString(m_helpPage));
}

std::string BaseCustomInstrumentView::getFile() {
  auto name = m_files->getFilenames();
  if (name.size() > 0)
    return name[0].toStdString();
  return "";
}

void BaseCustomInstrumentView::setRunQuietly(const std::string &runNumber) {
  m_files->setText(QString::fromStdString(runNumber));
}

void BaseCustomInstrumentView::fileLoaded() {
  if (m_files->getText().isEmpty())
    return;

  if (!m_files->isValid()) {
    warningBox(m_files->getFileProblem());
    return;
  }
  m_presenter->loadRunNumber();
}

void BaseCustomInstrumentView::warningBox(const std::string &message) { warningBox(QString::fromStdString(message)); }

void BaseCustomInstrumentView::warningBox(const QString &message) {
  QMessageBox::warning(this, m_instrument + " view", message);
}

} // namespace MantidQt::MantidWidgets
