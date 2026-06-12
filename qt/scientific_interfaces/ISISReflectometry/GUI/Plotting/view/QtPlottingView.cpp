// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtPlottingView.h"
#include "GUI/Plotting/model/PlotOutputTypeProperties.h"
#include "WorkspaceTreeController.h"
#include <QCheckBox>
#include <QComboBox>
#include <QItemSelection>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
int plotOutputTypeIndex(PlotOutputType outputType) { return static_cast<int>(outputType); }

auto constexpr minimumSelectedItemsForMultiPlot = size_t{2};

template <typename Enum> int enumIndex(Enum value) { return static_cast<int>(value); }

bool hasSelectedItems(size_t selectedItemCount) { return selectedItemCount > 0; }

/// Return true if the selection satisfies the output type's normal multi-plot requirements.
bool hasEnoughSelectedItemsForMultiPlot(size_t selectedItemCount, size_t selectedWorkspaceGroupCount,
                                        PlotOutputTypeProperties const &plotProperties) {
  if (plotProperties.requiresWorkspaceGroupsForMultiPlot()) {
    return selectedWorkspaceGroupCount >= minimumSelectedItemsForMultiPlot;
  }
  return selectedItemCount >= minimumSelectedItemsForMultiPlot;
}

/// Return true if the selection can produce a multi-output plot for the current target.
bool hasEnoughSelectedItemsForMultiOutputPlot(size_t selectedItemCount, size_t selectedWorkspaceGroupCount,
                                              PlotOutputTypeProperties const &plotProperties, bool addToExistingPlot) {
  return addToExistingPlot
             ? hasSelectedItems(selectedItemCount)
             : hasEnoughSelectedItemsForMultiPlot(selectedItemCount, selectedWorkspaceGroupCount, plotProperties);
}

/// Return true if the individual-plot button should be enabled.
bool shouldEnablePlotIndividual(bool outputSelectionEnabled, size_t selectedItemCount, bool addToExistingPlot) {
  return outputSelectionEnabled && !addToExistingPlot && hasSelectedItems(selectedItemCount);
}

/// Return true if the overplot button should be enabled.
bool shouldEnablePlotOverplot(bool outputSelectionEnabled, size_t selectedItemCount, size_t selectedWorkspaceGroupCount,
                              PlotOutputTypeProperties const &plotProperties, bool addToExistingPlot,
                              bool activePlotOverplotCompatible) {
  return outputSelectionEnabled && plotProperties.supportsOverplot() &&
         (!addToExistingPlot || activePlotOverplotCompatible) &&
         hasEnoughSelectedItemsForMultiOutputPlot(selectedItemCount, selectedWorkspaceGroupCount, plotProperties,
                                                  addToExistingPlot);
}

/// Return true if the tiled-plot button should be enabled.
bool shouldEnablePlotTiled(bool outputSelectionEnabled, size_t selectedItemCount, size_t selectedWorkspaceGroupCount,
                           PlotOutputTypeProperties const &plotProperties, bool addToExistingPlot) {
  return outputSelectionEnabled &&
         hasEnoughSelectedItemsForMultiOutputPlot(selectedItemCount, selectedWorkspaceGroupCount, plotProperties,
                                                  addToExistingPlot);
}

/// Return true if the vertical tiling option should be enabled.
bool shouldEnablePlotTiledVertically(bool outputSelectionEnabled, size_t selectedItemCount) {
  return outputSelectionEnabled && hasSelectedItems(selectedItemCount);
}

/// Return true if the add-to-existing option should be enabled.
bool shouldEnableAddToExistingPlot(bool outputSelectionEnabled, bool activePlotAvailable,
                                   bool activePlotOverplotCompatible, PlotOutputTypeProperties const &plotProperties) {
  return outputSelectionEnabled && activePlotAvailable && activePlotOverplotCompatible &&
         plotProperties.supportsAddToExistingPlot();
}
} // namespace

QtPlottingView::QtPlottingView(QWidget *parent)
    : QWidget(parent), m_notifyee(nullptr), m_outputSelectionEnabled(false), m_activePlotAvailable(false),
      m_activePlotOverplotCompatible(false) {
  initLayout();
}

QtPlottingView::~QtPlottingView() = default;

void QtPlottingView::initLayout() {
  m_ui.setupUi(this);
  m_ui.detectorMapYAxis->addItem("Detector Index", enumIndex(DetectorMapYAxis::DetectorId));
  m_ui.detectorMapYAxis->addItem("Detector angle, theta", enumIndex(DetectorMapYAxis::Theta));
  m_ui.detectorMapXAxis->addItem("Time of Flight", enumIndex(DetectorMapXAxis::TimeOfFlight));
  m_ui.detectorMapXAxis->addItem("Lambda", enumIndex(DetectorMapXAxis::Lambda));
  m_ui.alignmentXAxis->addItem("Detector Index", enumIndex(AlignmentXAxis::DetectorId));
  m_ui.alignmentXAxis->addItem("Detector angle, theta", enumIndex(AlignmentXAxis::Theta));
  m_workspaceTree = std::make_unique<WorkspaceTreeController>(m_ui.workspaceTree, this);
  connect(m_ui.workspaceTree->selectionModel(), &QItemSelectionModel::selectionChanged, this,
          [this](QItemSelection const &selected, QItemSelection const &deselected) {
            m_workspaceTree->updateChildSelection(deselected, QItemSelectionModel::Deselect);
            m_workspaceTree->updateChildSelection(selected, QItemSelectionModel::Select);
            if (m_notifyee) {
              m_notifyee->notifyActivePlotAvailabilityChanged();
            }
            updatePlotButtonEnabledStates();
          });
  connect(m_ui.plotPreset, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int) {
    clearWorkspaceSelection();
    updatePlotOutputProperties();
  });
  setAvailablePlotOutputTypes({PlotOutputType::ReflectivityCurve});
  connect(m_ui.plotTiled, &QPushButton::clicked, this, [this]() {
    if (m_notifyee) {
      m_notifyee->notifyPlotTiledClicked();
    }
  });
  connect(m_ui.plotOverplot, &QPushButton::clicked, this, [this]() {
    if (m_notifyee) {
      m_notifyee->notifyPlotOverplotClicked();
    }
  });
  connect(m_ui.plotIndividual, &QPushButton::clicked, this, [this]() {
    if (m_notifyee) {
      m_notifyee->notifyPlotIndividualClicked();
    }
  });
  connect(m_ui.addToExistingPlot, &QCheckBox::stateChanged, this, [this](int) {
    if (m_notifyee) {
      m_notifyee->notifyAddToExistingPlotChanged();
    }
    updatePlotButtonEnabledStates();
  });
  auto *activePlotAvailabilityTimer = new QTimer(this);
  activePlotAvailabilityTimer->setInterval(1000);
  connect(activePlotAvailabilityTimer, &QTimer::timeout, this, [this]() {
    if (m_notifyee) {
      m_notifyee->notifyActivePlotAvailabilityChanged();
    }
  });
  activePlotAvailabilityTimer->start();
  setOutputSelectionControlsEnabled(false);
}

void QtPlottingView::subscribe(PlottingViewSubscriber *notifyee) { m_notifyee = notifyee; }

void QtPlottingView::setOutputSelectionEnabled(bool enabled) { setOutputSelectionControlsEnabled(enabled); }

void QtPlottingView::setAvailablePlotOutputTypes(std::vector<PlotOutputType> const &outputTypes) {
  auto const previouslySelected = selectedPlotOutputType();
  QSignalBlocker blocker(m_ui.plotPreset);
  m_ui.plotPreset->clear();
  for (auto const outputType : outputTypes) {
    m_ui.plotPreset->addItem(plotOutputTypeProperties(outputType).displayName(), plotOutputTypeIndex(outputType));
  }
  auto const previousIndex = m_ui.plotPreset->findData(plotOutputTypeIndex(previouslySelected));
  if (previousIndex >= 0) {
    m_ui.plotPreset->setCurrentIndex(previousIndex);
  }
  if (previousIndex < 0 && !outputTypes.empty()) {
    clearWorkspaceSelection();
  }
  updatePlotOutputProperties();
}

void QtPlottingView::setOutputSelectionControlsEnabled(bool enabled) {
  m_outputSelectionEnabled = enabled;
  m_ui.plotPreset->setEnabled(enabled);
  m_ui.detectorMapYAxis->setEnabled(enabled);
  m_ui.detectorMapXAxis->setEnabled(enabled);
  m_ui.alignmentXAxis->setEnabled(enabled);
  updatePlotButtonEnabledStates();
}

void QtPlottingView::updatePlotButtonEnabledStates() {
  auto const selectedWorkspaceCount = selectedWorkspaceNames().size();
  auto const selectedWorkspaceGroupCount = m_workspaceTree->selectedWorkspaceGroupCount();
  auto const &plotProperties = plotOutputTypeProperties(selectedPlotOutputType());
  auto const addToExistingEnabled = shouldEnableAddToExistingPlot(m_outputSelectionEnabled, m_activePlotAvailable,
                                                                  m_activePlotOverplotCompatible, plotProperties);
  if (!addToExistingEnabled && m_ui.addToExistingPlot->isChecked()) {
    QSignalBlocker blocker(m_ui.addToExistingPlot);
    m_ui.addToExistingPlot->setChecked(false);
  }
  auto const addToExisting = addToExistingPlot();
  m_ui.addToExistingPlot->setEnabled(addToExistingEnabled);
  m_ui.plotIndividual->setEnabled(
      shouldEnablePlotIndividual(m_outputSelectionEnabled, selectedWorkspaceCount, addToExisting));
  m_ui.plotOverplot->setEnabled(shouldEnablePlotOverplot(m_outputSelectionEnabled, selectedWorkspaceCount,
                                                         selectedWorkspaceGroupCount, plotProperties, addToExisting,
                                                         m_activePlotOverplotCompatible));
  auto const plotTiledEnabled = shouldEnablePlotTiled(m_outputSelectionEnabled, selectedWorkspaceCount,
                                                      selectedWorkspaceGroupCount, plotProperties, addToExisting);
  m_ui.plotTiled->setEnabled(plotTiledEnabled);
  m_ui.plotTiledVertically->setEnabled(
      shouldEnablePlotTiledVertically(m_outputSelectionEnabled, selectedWorkspaceCount));
}

void QtPlottingView::updatePlotOutputProperties() {
  auto const &plotProperties = plotOutputTypeProperties(selectedPlotOutputType());

  m_ui.plotPropertiesTopSeparator->setVisible(plotProperties.showsPlotProperties());
  m_ui.plotPropertiesBottomSeparator->setVisible(plotProperties.showsPlotProperties());
  m_ui.detectorMapYAxisLabel->setVisible(plotProperties.showsDetectorMapProperties());
  m_ui.detectorMapYAxis->setVisible(plotProperties.showsDetectorMapProperties());
  m_ui.detectorMapXAxisLabel->setVisible(plotProperties.showsDetectorMapProperties());
  m_ui.detectorMapXAxis->setVisible(plotProperties.showsDetectorMapProperties());
  m_ui.alignmentXAxisLabel->setVisible(plotProperties.showsAlignmentProperties());
  m_ui.alignmentXAxis->setVisible(plotProperties.showsAlignmentProperties());
  m_workspaceTree->setCurrentPlotOutputType(selectedPlotOutputType());
  updatePlotButtonEnabledStates();
}

void QtPlottingView::clearWorkspaceSelection() {
  m_workspaceTree->clearSelection();
  updatePlotButtonEnabledStates();
}

void QtPlottingView::setWorkspaceItems(std::vector<PlottingWorkspaceTreeItem> const &items) {
  m_workspaceTree->setItems(items);
  updatePlotButtonEnabledStates();
}

std::vector<std::string> QtPlottingView::selectedWorkspaceNames() const {
  return m_workspaceTree->selectedWorkspaceNames();
}

PlotOutputType QtPlottingView::selectedPlotOutputType() const {
  return static_cast<PlotOutputType>(m_ui.plotPreset->currentData().toInt());
}

PlotOutputSelection QtPlottingView::selectedPlotOutputSelection() const {
  return {selectedPlotOutputType(), static_cast<DetectorMapXAxis>(m_ui.detectorMapXAxis->currentData().toInt()),
          static_cast<DetectorMapYAxis>(m_ui.detectorMapYAxis->currentData().toInt()),
          static_cast<AlignmentXAxis>(m_ui.alignmentXAxis->currentData().toInt())};
}

bool QtPlottingView::addToExistingPlot() const { return m_ui.addToExistingPlot->isChecked(); }

bool QtPlottingView::plotTiledVertically() const { return m_ui.plotTiledVertically->isChecked(); }

void QtPlottingView::setActivePlotAvailable(bool available) {
  m_activePlotAvailable = available;
  if (!available) {
    QSignalBlocker blocker(m_ui.addToExistingPlot);
    m_ui.addToExistingPlot->setChecked(false);
  }
  updatePlotButtonEnabledStates();
}

void QtPlottingView::setActivePlotOverplotCompatible(bool compatible) {
  m_activePlotOverplotCompatible = compatible;
  updatePlotButtonEnabledStates();
}

QWidget *QtPlottingView::plotParent() { return window(); }

bool QtPlottingView::confirmPlottingMultipleItems(size_t plotCount) const {
  auto const message = QString("This will plot %1 items. Continue?").arg(plotCount);
  return QMessageBox::warning(const_cast<QtPlottingView *>(this), "Create multiple plots", message,
                              QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok;
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
