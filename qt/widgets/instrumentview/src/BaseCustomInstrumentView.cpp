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
#include <QVBoxLayout>

namespace MantidQt::MantidWidgets {

BaseCustomInstrumentView::BaseCustomInstrumentView(const std::string &instrument, QWidget *parent)
    : QWidget(parent), m_extractAction(nullptr), m_averageAction(nullptr), m_helpPage("direct/ALF View"),
      m_files(nullptr), m_instrument(QString::fromStdString(instrument)), m_instrumentWidget(nullptr), m_help(nullptr) {
}

void BaseCustomInstrumentView::subscribePresenter(MantidQt::MantidWidgets::BaseCustomInstrumentPresenter *presenter) {
  m_presenter = presenter;
}

void BaseCustomInstrumentView::setUpInstrument(const std::string &fileName,
                                               std::vector<std::function<bool(std::map<std::string, bool>)>> &binders) {
  auto instrumentWidget =
      new MantidWidgets::InstrumentWidget(QString::fromStdString(fileName), nullptr, true, true, 0.0, 0.0, true,
                                          MantidWidgets::InstrumentWidget::Dependencies(), false);
  instrumentWidget->removeTab("Instrument");
  instrumentWidget->removeTab("Draw");
  instrumentWidget->hideHelp();

  auto pickTab = instrumentWidget->getPickTab();

  connect(pickTab->getSelectTubeButton(), SIGNAL(clicked()), this, SLOT(selectWholeTube()));

  // set up extract single tube
  m_extractAction = new QAction("Extract Single Tube", this);
  connect(m_extractAction, SIGNAL(triggered()), this, SLOT(extractSingleTube()));
  pickTab->addToContextMenu(m_extractAction, binders[0]);

  // set up add to average
  m_averageAction = new QAction("Add Tube To Average", this);
  connect(m_averageAction, SIGNAL(triggered()), this, SLOT(averageTube()));
  pickTab->addToContextMenu(m_averageAction, binders[1]);

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
  // this->addWidget(helpWidget);
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

void BaseCustomInstrumentView::selectWholeTube() {
  auto pickTab = getInstrumentView()->getPickTab();
  pickTab->setPlotType(MantidQt::MantidWidgets::IWPickPlotType::TUBE_INTEGRAL);
  pickTab->setTubeXUnits(MantidQt::MantidWidgets::IWPickXUnits::OUT_OF_PLANE_ANGLE);
}

void BaseCustomInstrumentView::extractSingleTube() {
  MantidWidgets::InstrumentWidget *instrumentView = getInstrumentView();
  instrumentView->getPickTab()->savePlotToWorkspace();

  m_presenter->extractSingleTube();
}

void BaseCustomInstrumentView::averageTube() {
  MantidWidgets::InstrumentWidget *instrumentView = getInstrumentView();
  instrumentView->getPickTab()->savePlotToWorkspace();

  m_presenter->averageTube();
}

void BaseCustomInstrumentView::warningBox(const std::string &message) { warningBox(QString::fromStdString(message)); }

void BaseCustomInstrumentView::warningBox(const QString &message) {
  QMessageBox::warning(this, m_instrument + " view", message);
}

} // namespace MantidQt::MantidWidgets
