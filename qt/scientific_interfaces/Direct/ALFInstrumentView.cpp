// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFInstrumentView.h"

#include "ALFInstrumentPresenter.h"
#include "MantidQtWidgets/Common/FileFinderWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"

#include <map>
#include <string>

#include <QMessageBox>
#include <QSizePolicy>
#include <QSpacerItem>

namespace {

bool hasCurve(std::map<std::string, bool> properties) {
  auto const stored = properties.find("plotStored");
  auto const curve = properties.find("hasCurve");
  return (stored != properties.cend() && stored->second) || (curve != properties.cend() && curve->second);
}

std::function<bool(std::map<std::string, bool>)> canExtractTube = [](std::map<std::string, bool> properties) -> bool {
  return (properties.find("isTube")->second && hasCurve(properties));
};

} // namespace

namespace MantidQt::CustomInterfaces {

ALFInstrumentView::ALFInstrumentView(QWidget *parent)
    : QWidget(parent), m_files(), m_instrumentWidget(), m_extractAction(), m_averageAction() {}

void ALFInstrumentView::setUpInstrument(std::string const &fileName) {
  m_instrumentWidget =
      new MantidWidgets::InstrumentWidget(QString::fromStdString(fileName), nullptr, true, true, 0.0, 0.0, true,
                                          MantidWidgets::InstrumentWidget::Dependencies(), false);
  m_instrumentWidget->removeTab("Instrument");
  m_instrumentWidget->removeTab("Draw");
  m_instrumentWidget->hideHelp();

  auto pickTab = m_instrumentWidget->getPickTab();
  pickTab->expandPlotPanel();

  connect(pickTab->getSelectTubeButton(), SIGNAL(clicked()), this, SLOT(selectWholeTube()));

  // set up extract single tube
  m_extractAction = new QAction("Extract Single Tube", this);
  connect(m_extractAction, SIGNAL(triggered()), this, SLOT(extractSingleTube()));
  pickTab->addToContextMenu(m_extractAction, canExtractTube);

  std::function<bool(std::map<std::string, bool>)> canAverageTube =
      [&](std::map<std::string, bool> properties) -> bool {
    return (m_presenter->checkDataIsExtracted() && properties.find("isTube")->second && hasCurve(properties));
  };

  // set up add to average
  m_averageAction = new QAction("Add Tube To Average", this);
  connect(m_averageAction, SIGNAL(triggered()), this, SLOT(averageTube()));
  pickTab->addToContextMenu(m_averageAction, canAverageTube);
}

QWidget *ALFInstrumentView::generateLoadWidget() {
  m_files = new API::FileFinderWidget(this);
  m_files->setLabelText("ALF");
  m_files->allowMultipleFiles(false);
  m_files->setInstrumentOverride("ALF");
  m_files->isForRunFiles(true);
  connect(m_files, SIGNAL(fileFindingFinished()), this, SLOT(fileLoaded()));

  auto loadWidget = new QWidget();
  auto loadLayout = new QHBoxLayout(loadWidget);

  loadLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
  loadLayout->addWidget(m_files);
  loadLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

  return loadWidget;
}

void ALFInstrumentView::subscribePresenter(IALFInstrumentPresenter *presenter) { m_presenter = presenter; }

std::optional<std::string> ALFInstrumentView::getFile() {
  auto name = m_files->getFilenames();
  if (name.size() > 0)
    return name[0].toStdString();
  return std::nullopt;
}

void ALFInstrumentView::setRunQuietly(std::string const &runNumber) {
  m_files->setText(QString::fromStdString(runNumber));
}

void ALFInstrumentView::fileLoaded() {
  if (m_files->getText().isEmpty())
    return;

  if (!m_files->isValid()) {
    warningBox(m_files->getFileProblem().toStdString());
    return;
  }
  m_presenter->loadRunNumber();
}

void ALFInstrumentView::selectWholeTube() {
  auto pickTab = m_instrumentWidget->getPickTab();
  pickTab->setPlotType(MantidQt::MantidWidgets::IWPickPlotType::TUBE_INTEGRAL);
  pickTab->setTubeXUnits(MantidQt::MantidWidgets::IWPickXUnits::OUT_OF_PLANE_ANGLE);
}

void ALFInstrumentView::extractSingleTube() {
  m_instrumentWidget->getPickTab()->savePlotToWorkspace();
  m_presenter->extractSingleTube();
}

void ALFInstrumentView::averageTube() {
  m_instrumentWidget->getPickTab()->savePlotToWorkspace();
  m_presenter->averageTube();
}

void ALFInstrumentView::warningBox(std::string const &message) {
  QMessageBox::warning(this, "ALFView", QString::fromStdString(message));
}

} // namespace MantidQt::CustomInterfaces
