// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFCustomInstrumentView.h"
#include "MantidQtWidgets/InstrumentView/BaseCustomInstrumentPresenter.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"

#include <QMessageBox>
#include <QSizePolicy>

namespace MantidQt::CustomInterfaces {

ALFCustomInstrumentView::ALFCustomInstrumentView(const std::string &instrument, QWidget *parent)
    : MantidWidgets::BaseCustomInstrumentView(instrument, parent), m_extractAction(nullptr), m_averageAction(nullptr),
      m_analysisPane(nullptr) {
  m_helpPage = "direct/ALF View";
}

void ALFCustomInstrumentView::setUpInstrument(const std::string &fileName,
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

void ALFCustomInstrumentView::selectWholeTube() {
  auto pickTab = getInstrumentView()->getPickTab();
  pickTab->setPlotType(MantidQt::MantidWidgets::IWPickPlotType::TUBE_INTEGRAL);
  pickTab->setTubeXUnits(MantidQt::MantidWidgets::IWPickXUnits::OUT_OF_PLANE_ANGLE);
}

void ALFCustomInstrumentView::extractSingleTube() {
  MantidWidgets::InstrumentWidget *instrumentView = getInstrumentView();
  instrumentView->getPickTab()->savePlotToWorkspace();

  m_presenter->extractSingleTube();
}

void ALFCustomInstrumentView::averageTube() {
  MantidWidgets::InstrumentWidget *instrumentView = getInstrumentView();
  instrumentView->getPickTab()->savePlotToWorkspace();

  m_presenter->averageTube();
}

void ALFCustomInstrumentView::addSpectrum(const std::string &wsName) { m_analysisPane->addSpectrum(wsName); }

} // namespace MantidQt::CustomInterfaces
