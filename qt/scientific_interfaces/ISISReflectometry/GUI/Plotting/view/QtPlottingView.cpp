// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtPlottingView.h"
#include "QtPlottingWorkspaceTreeViewAdapter.h"
#include <QCheckBox>
#include <QComboBox>
#include <QItemSelection>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <vector>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
int plotOutputTypeIndex(PlotOutputType outputType) { return static_cast<int>(outputType); }

template <typename Enum> int enumIndex(Enum value) { return static_cast<int>(value); }
} // namespace

QtPlottingView::QtPlottingView(QWidget *parent) : QWidget(parent), m_notifyee(nullptr) { initLayout(); }

QtPlottingView::~QtPlottingView() = default;

void QtPlottingView::initLayout() {
  m_ui.setupUi(this);
  m_ui.detectorMapYAxis->addItem("Detector Index", enumIndex(DetectorMapYAxis::DetectorId));
  m_ui.detectorMapYAxis->addItem("Detector angle, theta", enumIndex(DetectorMapYAxis::Theta));
  m_ui.detectorMapXAxis->addItem("Time of Flight", enumIndex(DetectorMapXAxis::TimeOfFlight));
  m_ui.detectorMapXAxis->addItem("Lambda", enumIndex(DetectorMapXAxis::Lambda));
  m_ui.alignmentXAxis->addItem("Detector Index", enumIndex(AlignmentXAxis::DetectorId));
  m_ui.alignmentXAxis->addItem("Detector angle, theta", enumIndex(AlignmentXAxis::Theta));
  m_plottingWorkspaceTreeViewAdapter =
      std::make_unique<QtPlottingWorkspaceTreeViewAdapter>(m_ui.plottingWorkspaceTree, this);
  connect(m_ui.plottingWorkspaceTree->selectionModel(), &QItemSelectionModel::selectionChanged, this,
          [this](QItemSelection const &selected, QItemSelection const &deselected) {
            m_plottingWorkspaceTreeViewAdapter->updateChildSelection(deselected, QItemSelectionModel::Deselect);
            m_plottingWorkspaceTreeViewAdapter->updateChildSelection(selected, QItemSelectionModel::Select);
            if (m_notifyee) {
              m_notifyee->notifyPlottingWorkspaceTreeSelectionChanged();
            }
          });
  connect(m_ui.plotPreset, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int) {
    clearPlottingWorkspaceTreeSelection();
    if (m_notifyee) {
      m_notifyee->notifyPlotOutputTypeChanged();
    }
  });
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
  connect(m_ui.addToExistingPlot, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState) {
    if (m_notifyee) {
      m_notifyee->notifyAddToExistingPlotChanged();
    }
  });
  setOutputSelectionControlsEnabled(false);
}

void QtPlottingView::subscribe(PlottingViewSubscriber *notifyee) { m_notifyee = notifyee; }

void QtPlottingView::setOutputSelectionEnabled(bool enabled) { setOutputSelectionControlsEnabled(enabled); }

void QtPlottingView::setAvailablePlotOutputTypes(std::vector<PlotOutputTypeViewItem> const &outputTypes) {
  auto const previouslySelected = selectedPlotOutputType();
  QSignalBlocker blocker(m_ui.plotPreset);
  m_ui.plotPreset->clear();
  for (auto const &outputType : outputTypes) {
    m_ui.plotPreset->addItem(QString::fromStdString(outputType.label), plotOutputTypeIndex(outputType.outputType));
  }
  auto const previousIndex =
      previouslySelected ? m_ui.plotPreset->findData(plotOutputTypeIndex(*previouslySelected)) : -1;
  if (previousIndex >= 0) {
    m_ui.plotPreset->setCurrentIndex(previousIndex);
  }
  if (previousIndex < 0 && !outputTypes.empty()) {
    clearPlottingWorkspaceTreeSelection();
  }
}

void QtPlottingView::setOutputSelectionControlsEnabled(bool enabled) {
  m_ui.plotPreset->setEnabled(enabled);
  m_ui.detectorMapYAxis->setEnabled(enabled);
  m_ui.detectorMapXAxis->setEnabled(enabled);
  m_ui.alignmentXAxis->setEnabled(enabled);
}

void QtPlottingView::setPlotActionState(PlotActionState const &state) {
  if (m_ui.addToExistingPlot->isChecked() != state.addToExistingPlotChecked) {
    QSignalBlocker blocker(m_ui.addToExistingPlot);
    m_ui.addToExistingPlot->setChecked(state.addToExistingPlotChecked);
  }
  m_ui.addToExistingPlot->setEnabled(state.addToExistingPlotEnabled);
  m_ui.plotIndividual->setEnabled(state.plotIndividualEnabled);
  m_ui.plotOverplot->setEnabled(state.plotOverplotEnabled);
  m_ui.plotTiled->setEnabled(state.plotTiledEnabled);
  m_ui.plotTiledVertically->setEnabled(state.plotTiledVerticallyEnabled);
}

void QtPlottingView::setPlotOutputControlsState(PlotOutputControlsState const &state) {
  m_ui.plotPropertiesTopSeparator->setVisible(state.plotPropertiesVisible);
  m_ui.plotPropertiesBottomSeparator->setVisible(state.plotPropertiesVisible);
  m_ui.detectorMapYAxisLabel->setVisible(state.detectorMapControlsVisible);
  m_ui.detectorMapYAxis->setVisible(state.detectorMapControlsVisible);
  m_ui.detectorMapXAxisLabel->setVisible(state.detectorMapControlsVisible);
  m_ui.detectorMapXAxis->setVisible(state.detectorMapControlsVisible);
  m_ui.alignmentXAxisLabel->setVisible(state.alignmentControlsVisible);
  m_ui.alignmentXAxis->setVisible(state.alignmentControlsVisible);
}

void QtPlottingView::clearPlottingWorkspaceTreeSelection() { m_plottingWorkspaceTreeViewAdapter->clearSelection(); }

void QtPlottingView::setPlottingWorkspaceTreeItemStates(std::vector<PlottingWorkspaceTreeItemState> const &itemStates) {
  m_plottingWorkspaceTreeViewAdapter->setPlottingWorkspaceTreeItemStates(itemStates);
}

std::vector<std::string> QtPlottingView::selectedPlottingWorkspaceNames() const {
  return m_plottingWorkspaceTreeViewAdapter->selectedPlottingWorkspaceNames();
}

size_t QtPlottingView::selectedPlottingWorkspaceGroupCount() const {
  return m_plottingWorkspaceTreeViewAdapter->selectedPlottingWorkspaceGroupCount();
}

std::optional<PlotOutputType> QtPlottingView::selectedPlotOutputType() const {
  auto const currentData = m_ui.plotPreset->currentData();
  return currentData.isValid() ? std::optional<PlotOutputType>{static_cast<PlotOutputType>(currentData.toInt())}
                               : std::nullopt;
}

PlotOutputSelection QtPlottingView::selectedPlotOutputSelection() const {
  return {*selectedPlotOutputType(), static_cast<DetectorMapXAxis>(m_ui.detectorMapXAxis->currentData().toInt()),
          static_cast<DetectorMapYAxis>(m_ui.detectorMapYAxis->currentData().toInt()),
          static_cast<AlignmentXAxis>(m_ui.alignmentXAxis->currentData().toInt())};
}

bool QtPlottingView::addToExistingPlot() const { return m_ui.addToExistingPlot->isChecked(); }

bool QtPlottingView::plotTiledVertically() const { return m_ui.plotTiledVertically->isChecked(); }

QWidget *QtPlottingView::plotParent() { return window(); }

bool QtPlottingView::confirmPlottingMultipleItems(size_t plotCount) const {
  auto const message = QString("This will plot %1 items. Continue?").arg(plotCount);
  return QMessageBox::warning(const_cast<QtPlottingView *>(this), "Create multiple plots", message,
                              QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok;
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
