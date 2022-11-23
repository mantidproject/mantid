// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFView.h"

#include "ALFAnalysisModel.h"
#include "ALFAnalysisView.h"
#include "ALFInstrumentModel.h"
#include "ALFInstrumentView.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"

#include <QString>
#include <QVBoxLayout>

namespace MantidQt::CustomInterfaces {

DECLARE_SUBWINDOW(ALFView)

ALFView::ALFView(QWidget *parent) : UserSubWindow(parent), m_instrumentPresenter(), m_analysisPresenter() {
  this->setWindowTitle("ALF View");

  m_instrumentPresenter =
      std::make_unique<ALFInstrumentPresenter>(new ALFInstrumentView(this), std::make_unique<ALFInstrumentModel>());

  m_analysisPresenter = std::make_unique<ALFAnalysisPresenter>(new ALFAnalysisView(-15.0, 15.0, this),
                                                               std::make_unique<ALFAnalysisModel>());

  m_instrumentPresenter->subscribeAnalysisPresenter(m_analysisPresenter.get());
}

void ALFView::initLayout() {
  auto *splitter = new QSplitter(Qt::Horizontal);
  splitter->addWidget(m_instrumentPresenter->getInstrumentView());
  splitter->addWidget(m_analysisPresenter->getView());

  auto mainWidget = new QSplitter(Qt::Vertical);
  mainWidget->addWidget(m_instrumentPresenter->getLoadWidget());
  mainWidget->addWidget(splitter);

  auto centralWidget = new QWidget();
  auto verticalLayout = new QVBoxLayout(centralWidget);
  verticalLayout->addWidget(mainWidget);
  verticalLayout->addWidget(createHelpWidget());

  this->setCentralWidget(centralWidget);
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

void ALFView::openHelp() { MantidQt::API::HelpWindow::showCustomInterface(QString("direct/ALF View")); }

} // namespace MantidQt::CustomInterfaces
