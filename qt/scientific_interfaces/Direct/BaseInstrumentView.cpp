// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "BaseInstrumentView.h"
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
    : QSplitter(Qt::Vertical, parent), m_plot(nullptr), m_fitBrowser(nullptr),
      m_start(nullptr), m_end(nullptr), m_loadRunObservable(nullptr),
      m_files(nullptr), m_instrument(QString::fromStdString(instrument)),
      m_instrumentWidget(nullptr), m_fitPlotLayout(nullptr),
      m_fitButton(nullptr),m_fitObservable(nullptr) {
  auto loadWidget = generateLoadWidget();
  this->addWidget(loadWidget);
}

void MantidQt::CustomInterfaces::BaseInstrumentView::setUpInstrument(
    const std::string fileName,
    std::vector<std::function<bool(std::map<std::string, bool>)>> &instrument) {

  (void)instrument;
  m_instrumentWidget =
      new MantidWidgets::InstrumentWidget(QString::fromStdString(fileName));
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

void BaseInstrumentView::setupPlotFitSplitter() {
  m_fitPlotLayout = new QSplitter(Qt::Vertical);

  m_plot = new MantidWidgets::PreviewPlot();
  m_plot->setCanvasColour(Qt::white);
  m_fitPlotLayout->addWidget(m_plot);

  m_fitPlotLayout->addWidget(createFitPane());
}

QWidget *BaseInstrumentView::createFitPane() {
  auto fitPane = new QWidget();
  auto fitPaneLayout = new QVBoxLayout(fitPane);

  auto fitButtons = new QWidget();
  auto layout = new QHBoxLayout(fitButtons);
  m_fitButton = new QPushButton("Fit");
  m_fitObservable = new Observable();
  connect(m_fitButton, SIGNAL(clicked()), this, SLOT(doFit()));

  layout->addWidget(m_fitButton);
  layout->addItem(
      new QSpacerItem(80, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));

  fitPaneLayout->addWidget(fitButtons);

  m_fitBrowser = new MantidWidgets::FunctionBrowser(this);
  fitPaneLayout->addWidget(m_fitBrowser);

  QLabel *start = new QLabel("Fit from:");
  m_start = new QLineEdit("-15.0");
  QLabel *end = new QLabel("to:");
  m_end = new QLineEdit("15.0");

  QWidget *range = new QWidget();
  QHBoxLayout *rangeLayout = new QHBoxLayout(range);

  rangeLayout->addWidget(start);
  rangeLayout->addWidget(m_start);
  rangeLayout->addWidget(end);
  rangeLayout->addWidget(m_end);
  fitPaneLayout->addWidget(range);
  return fitPane;
}
  void BaseInstrumentView::setupInstrumentPlotFitSplitters() {

    setupPlotFitSplitter();
    QSplitter *split = new QSplitter(Qt::Horizontal);
    split->addWidget(m_instrumentWidget);
    split->addWidget(m_fitPlotLayout);
    this->addWidget(split);
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
 
  void BaseInstrumentView::doFit() {
    auto function = m_fitBrowser->getFunction();
    if (function->nFunctions() == 0) {
      return;
	}
    m_fitObservable->notify();
  }

  void BaseInstrumentView::warningBox(const std::string &message) {
    warningBox(QString::fromStdString(message));
  }

  void BaseInstrumentView::warningBox(const QString &message) {
    QMessageBox::warning(this, m_instrument + " view", message);
  }

} // namespace CustomInterfaces
} // namespace CustomInterfaces