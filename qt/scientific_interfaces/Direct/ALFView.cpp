// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFView.h"

#include "ALFAlgorithmManager.h"
#include "ALFAnalysisModel.h"
#include "ALFAnalysisView.h"
#include "ALFInstrumentModel.h"
#include "ALFInstrumentView.h"
#include "ALFInstrumentWidget.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/QtJobRunner.h"

#include <QSplitter>
#include <QString>
#include <QVBoxLayout>

namespace MantidQt::CustomInterfaces {

DECLARE_SUBWINDOW(ALFView)

ALFView::ALFView(QWidget *parent) : UserSubWindow(parent), m_instrumentPresenter(), m_analysisPresenter() {
  this->setWindowTitle("ALFView");

  // Algorithm manager for the instrument presenter
  auto jobRunnerInst = std::make_unique<MantidQt::API::QtJobRunner>();
  auto algorithmManagerInst = std::make_unique<ALFAlgorithmManager>(std::move(jobRunnerInst));

  m_instrumentPresenter = std::make_unique<ALFInstrumentPresenter>(
      new ALFInstrumentView(this), std::make_unique<ALFInstrumentModel>(), std::move(algorithmManagerInst));

  // Algorithm manager for the analysis presenter
  auto jobRunnerAnalysis = std::make_unique<MantidQt::API::QtJobRunner>();
  auto algorithmManagerAnalysis = std::make_unique<ALFAlgorithmManager>(std::move(jobRunnerAnalysis));

  m_analysisPresenter =
      std::make_unique<ALFAnalysisPresenter>(new ALFAnalysisView(-15.0, 15.0, this),
                                             std::make_unique<ALFAnalysisModel>(), std::move(algorithmManagerAnalysis));

  m_instrumentPresenter->subscribeAnalysisPresenter(m_analysisPresenter.get());
}

ALFView::~ALFView() { m_instrumentPresenter->saveSettings(); }

void ALFView::disable(std::string const &reason) {
  this->setEnabled(false);
  this->setWindowTitle("ALFView - " + QString::fromStdString(reason) + "...");
}

void ALFView::enable() {
  this->setEnabled(true);
  this->setWindowTitle("ALFView");
}

void ALFView::initLayout() {
  auto *splitter = new QSplitter(Qt::Horizontal);
  splitter->addWidget(m_instrumentPresenter->getInstrumentView());
  splitter->addWidget(m_analysisPresenter->getView());

  splitter->setCollapsible(0, false);
  splitter->setCollapsible(1, false);

  auto mainWidget = new QSplitter(Qt::Vertical);

  auto loadWidget = new QWidget();
  auto loadLayout = new QVBoxLayout(loadWidget);
  loadLayout->addWidget(m_instrumentPresenter->getSampleLoadWidget());
  loadLayout->addWidget(m_instrumentPresenter->getVanadiumLoadWidget());

  mainWidget->addWidget(loadWidget);
  mainWidget->addWidget(splitter);

  mainWidget->setCollapsible(0, false);
  mainWidget->setCollapsible(1, false);

  auto centralWidget = new QWidget();
  auto verticalLayout = new QVBoxLayout(centralWidget);
  verticalLayout->addWidget(mainWidget);
  verticalLayout->addWidget(createHelpWidget());

  this->setCentralWidget(centralWidget);

  m_instrumentPresenter->loadSettings();
}

QWidget *ALFView::createHelpWidget() {
  m_help = new QPushButton("?");
  m_help->setMaximumWidth(25);
  connect(m_help, SIGNAL(clicked()), this, SLOT(openHelp()));

  auto *helpWidget = new QWidget();
  auto helpLayout = new QHBoxLayout(helpWidget);
  helpLayout->addWidget(m_help);
  helpLayout->addItem(new QSpacerItem(1000, 20, QSizePolicy::Expanding, QSizePolicy::Expanding));
  return helpWidget;
}

void ALFView::openHelp() { MantidQt::API::HelpWindow::showCustomInterface(QString("direct/ALFView")); }

} // namespace MantidQt::CustomInterfaces
