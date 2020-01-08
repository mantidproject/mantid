// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFCustomInstrumentView.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"

#include <QMessageBox>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QVBoxLayout>

namespace MantidQt {
namespace CustomInterfaces {

ALFCustomInstrumentView::ALFCustomInstrumentView(const std::string &instrument,
                                                 QWidget *parent)
    : MantidWidgets::BaseCustomInstrumentView(instrument, parent),
      m_extractSingleTubeObservable(nullptr), m_averageTubeObservable(nullptr),
      m_extractAction(nullptr), m_averageAction(nullptr),
      m_analysisPane(nullptr) {
  m_helpPage = "ALF View";
}

void ALFCustomInstrumentView::setUpInstrument(
    const std::string &fileName,
    std::vector<std::function<bool(std::map<std::string, bool>)>> &binders) {

  m_extractSingleTubeObservable = new Observable();
  m_averageTubeObservable = new Observable();

  auto instrumentWidget =
      new MantidWidgets::InstrumentWidget(QString::fromStdString(fileName));
  instrumentWidget->removeTab("Instrument");
  instrumentWidget->removeTab("Draw");
  instrumentWidget->hideHelp();

  // set up extract single tube
  m_extractAction = new QAction("Extract Single Tube", this);
  connect(m_extractAction, SIGNAL(triggered()), this,
          SLOT(extractSingleTube())),
      instrumentWidget->getPickTab()->addToContextMenu(m_extractAction,
                                                       binders[0]);

  // set up add to average
  m_averageAction = new QAction("Add Tube To Average", this);
  connect(m_averageAction, SIGNAL(triggered()), this, SLOT(averageTube())),
      instrumentWidget->getPickTab()->addToContextMenu(m_averageAction,
                                                       binders[1]);

  setInstrumentWidget(instrumentWidget);
}

void ALFCustomInstrumentView::extractSingleTube() {
  MantidWidgets::InstrumentWidget *instrumentView = getInstrumentView();
  instrumentView->getPickTab()->savePlotToWorkspace();

  m_extractSingleTubeObservable->notify();
}

void ALFCustomInstrumentView::averageTube() {
  MantidWidgets::InstrumentWidget *instrumentView = getInstrumentView();
  instrumentView->getPickTab()->savePlotToWorkspace();
  m_averageTubeObservable->notify();
}

void ALFCustomInstrumentView::observeExtractSingleTube(Observer *listner) {
  m_extractSingleTubeObservable->attach(listner);
}
void ALFCustomInstrumentView::observeAverageTube(Observer *listner) {
  m_averageTubeObservable->attach(listner);
}

void ALFCustomInstrumentView::addObserver(
    std::tuple<std::string, Observer *> &listener) {
  if (std::get<0>(listener) == "singleTube") {
    observeExtractSingleTube(std::get<1>(listener));
  } else if (std::get<0>(listener) == "averageTube") {
    observeAverageTube(std::get<1>(listener));
  }
}

void ALFCustomInstrumentView::setupAnalysisPane(
    MantidWidgets::PlotFitAnalysisPaneView *analysis) {
  // keep a copy here so we can use a custom class
  m_analysisPane = analysis;
  // just adds it to the view
  BaseCustomInstrumentView::setupInstrumentAnalysisSplitters(analysis);
}

void ALFCustomInstrumentView::addSpectrum(std::string wsName) {
  m_analysisPane->addSpectrum(wsName);
}

} // namespace CustomInterfaces
} // namespace MantidQt