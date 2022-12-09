// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ALFInstrumentView.h"

#include "ALFInstrumentPresenter.h"
#include "ALFInstrumentWidget.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidQtWidgets/Common/FileFinderWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedSurface.h"

#include <string>

#include <QMessageBox>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QString>

namespace MantidQt::CustomInterfaces {

ALFInstrumentView::ALFInstrumentView(QWidget *parent) : QWidget(parent), m_files(), m_instrumentWidget() {}

void ALFInstrumentView::setUpInstrument(std::string const &fileName) {
  m_instrumentWidget = new ALFInstrumentWidget(QString::fromStdString(fileName));

  connect(m_instrumentWidget, SIGNAL(instrumentActorReset()), this, SLOT(reconnectInstrumentActor()));
  reconnectInstrumentActor();

  auto surface = m_instrumentWidget->getInstrumentDisplay()->getSurface().get();
  connect(surface, SIGNAL(shapeCreated()), this, SLOT(notifyShapeChanged()));
  connect(surface, SIGNAL(shapeChangeFinished()), this, SLOT(notifyShapeChanged()));
  connect(surface, SIGNAL(shapesRemoved()), this, SLOT(notifyShapeChanged()));
  connect(surface, SIGNAL(shapesCleared()), this, SLOT(notifyShapeChanged()));

  auto pickTab = m_instrumentWidget->getPickTab();
  connect(pickTab->getSelectTubeButton(), SIGNAL(clicked()), this, SLOT(selectWholeTube()));
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

void ALFInstrumentView::reconnectInstrumentActor() {
  connect(&m_instrumentWidget->getInstrumentActor(), SIGNAL(refreshView()), this, SLOT(notifyShapeChanged()));
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

void ALFInstrumentView::notifyShapeChanged() { m_presenter->notifyShapeChanged(); }

MantidWidgets::IInstrumentActor const &ALFInstrumentView::getInstrumentActor() const {
  return m_instrumentWidget->getInstrumentActor();
}

Mantid::Geometry::ComponentInfo const &ALFInstrumentView::componentInfo() const {
  auto &actor = m_instrumentWidget->getInstrumentActor();
  return actor.componentInfo();
}

std::vector<std::size_t> ALFInstrumentView::getSelectedDetectors() const {
  auto const surface = std::dynamic_pointer_cast<MantidQt::MantidWidgets::UnwrappedSurface>(
      m_instrumentWidget->getInstrumentDisplay()->getSurface());

  std::vector<size_t> detectorIndices;
  // Find the detectors which are being intersected by the "masked" shapes.
  surface->getIntersectingDetectors(detectorIndices);
  return detectorIndices;
}

void ALFInstrumentView::selectWholeTube() {
  auto pickTab = m_instrumentWidget->getPickTab();
  pickTab->setPlotType(MantidQt::MantidWidgets::IWPickPlotType::TUBE_INTEGRAL);
  pickTab->setTubeXUnits(MantidQt::MantidWidgets::IWPickXUnits::OUT_OF_PLANE_ANGLE);
}

void ALFInstrumentView::warningBox(std::string const &message) {
  QMessageBox::warning(this, "ALFView", QString::fromStdString(message));
}

} // namespace MantidQt::CustomInterfaces
