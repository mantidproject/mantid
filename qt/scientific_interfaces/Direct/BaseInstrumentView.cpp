// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "BaseInstrumentView.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"

#include <QMessageBox>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QSplitter>
#include <QVBoxLayout>

namespace MantidQt {
namespace CustomInterfaces {

BaseInstrumentView::BaseInstrumentView(const std::string &instrument,
                                       QWidget *parent)
    : QSplitter(Qt::Vertical, parent), m_helpPage(""),
      m_loadRunObservable(nullptr), m_files(nullptr),
      m_instrument(QString::fromStdString(instrument)),
      m_instrumentWidget(nullptr), m_help(nullptr) {
  auto loadWidget = generateLoadWidget();
  this->addWidget(loadWidget);
}

void MantidQt::CustomInterfaces::BaseInstrumentView::setUpInstrument(
    const std::string &fileName,
    std::vector<std::function<bool(std::map<std::string, bool>)>> &instrument) {

  (void)instrument;
  auto instrumentWidget =
      new MantidWidgets::InstrumentWidget(QString::fromStdString(fileName));
  instrumentWidget->hideHelp();
  setInstrumentWidget(instrumentWidget);
}

QWidget *BaseInstrumentView::generateLoadWidget() {
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

void BaseInstrumentView::setupInstrumentAnalysisSplitters(
    QWidget *analysisPane) {
  QSplitter *split = new QSplitter(Qt::Horizontal);
  split->addWidget(m_instrumentWidget);
  split->addWidget(analysisPane);
  this->addWidget(split);
}

void BaseInstrumentView::setupHelp() {
  QWidget *helpWidget = new QWidget();
  m_help = new QPushButton("?");
  m_help->setMaximumWidth(25);
  auto helpLayout = new QHBoxLayout(helpWidget);
  helpLayout->addWidget(m_help);

  helpLayout->addItem(new QSpacerItem(1000, 20, QSizePolicy::Expanding,
                                      QSizePolicy::Expanding));
  this->addWidget(helpWidget);
  connect(m_help, SIGNAL(clicked()), this, SLOT(openHelp()));
}

void BaseInstrumentView::openHelp() {
  if (m_helpPage == "") {
    return;
  }
  MantidQt::API::HelpWindow::showCustomInterface(
      nullptr, QString::fromStdString(m_helpPage));
}

std::string BaseInstrumentView::getFile() {
  auto name = m_files->getFilenames();
  if (name.size() > 0)
    return name[0].toStdString();
  return "";
}

void BaseInstrumentView::setRunQuietly(const std::string &runNumber) {
  m_files->setText(QString::fromStdString(runNumber));
}

void BaseInstrumentView::fileLoaded() {
  if (m_files->getText().isEmpty())
    return;

  if (!m_files->isValid()) {
    warningBox(m_files->getFileProblem());
    return;
  }
  m_loadRunObservable->notify();
}

void BaseInstrumentView::warningBox(const std::string &message) {
  warningBox(QString::fromStdString(message));
}

void BaseInstrumentView::warningBox(const QString &message) {
  QMessageBox::warning(this, m_instrument + " view", message);
}

} // namespace CustomInterfaces
} // namespace MantidQt
