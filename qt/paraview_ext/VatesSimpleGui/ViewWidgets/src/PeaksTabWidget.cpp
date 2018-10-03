// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesSimpleGuiViewWidgets/PeaksTabWidget.h"

#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidQtWidgets/SliceViewer/QPeaksTableModel.h"
#include "MantidVatesSimpleGuiViewWidgets/PeaksWidget.h"

#include <QItemSelectionModel>
#include <QModelIndex>
#include <QWidget>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace Mantid {
namespace Vates {
namespace SimpleGui {
/**
Constructor

@param ws : Peaks Workspace (MODEL)
@param coordinateSystem : Name of coordinate system used
@param parent : parent widget
*/
PeaksTabWidget::PeaksTabWidget(
    std::vector<Mantid::API::IPeaksWorkspace_sptr> ws,
    const std::string &coordinateSystem, QWidget *parent)
    : QWidget(parent), m_ws(ws), m_coordinateSystem(coordinateSystem) {
  ui.setupUi(this);

  // Add the tab widget
  m_tabWidget = new PeakCustomTabWidget();
  ui.tabLayout->addWidget(m_tabWidget);
}

/// Destructor
PeaksTabWidget::~PeaksTabWidget() {}

/**
 * Setup the Table model
 * @param visiblePeaks : A vector of lists of visible peaks for each peak
 * workspace
 */
void PeaksTabWidget::setupMvc(
    std::map<std::string, std::vector<bool>> visiblePeaks) {
  for (const auto &ws : m_ws) {
    // Create new tab
    const std::string &name = ws->getName();
    // Get visible peaks
    if (visiblePeaks.count(name) > 0) {
      addNewTab(ws, name, visiblePeaks[name]);
    }
  }
}

void PeaksTabWidget::addNewTab(Mantid::API::IPeaksWorkspace_sptr peaksWorkspace,
                               const std::string &tabName,
                               std::vector<bool> visiblePeaks) {
  PeaksWidget *widget =
      new PeaksWidget(std::move(peaksWorkspace), m_coordinateSystem, this);
  widget->setupMvc(std::move(visiblePeaks));

  // Connect to the output of the widget
  QObject::connect(
      widget, SIGNAL(zoomToPeak(Mantid::API::IPeaksWorkspace_sptr, int)), this,
      SLOT(onZoomToPeak(Mantid::API::IPeaksWorkspace_sptr, int)));

  // Connect to the sort functionality of the widget
  QObject::connect(widget,
                   SIGNAL(sortPeaks(const std::string &, const bool,
                                    Mantid::API::IPeaksWorkspace_sptr)),
                   this,
                   SIGNAL(sortPeaks(const std::string &, const bool,
                                    Mantid::API::IPeaksWorkspace_sptr)));

  // Add as a new tab
  m_tabWidget->addTab(widget, QString(tabName.c_str()));
}

/**
 * Zoom to the peak of interest
 * @param ws The workspace pointer.
 * @param row The row in the table.
 */
void PeaksTabWidget::onZoomToPeak(Mantid::API::IPeaksWorkspace_sptr ws,
                                  int row) {
  emit zoomToPeak(std::move(ws), row);
}

/**
 * Update the models and remove the model if it is not required anymore.
 * @param visiblePeaks A map with visible peaks for each workspace.
 * @param colors The color of the tabs
 */
void PeaksTabWidget::updateTabs(
    std::map<std::string, std::vector<bool>> &visiblePeaks,
    std::map<std::string, QColor> &colors) {
  // Iterate over all tabs
  for (int i = 0; i < m_tabWidget->count(); i++) {
    QString label = m_tabWidget->tabText(i);

    // Check if the peaks workspace still exists, if it does update, else delete
    // the tab.
    if (visiblePeaks.count(label.toStdString()) > 0 &&
        colors.count(label.toStdString()) > 0) {
      updateTab(visiblePeaks[label.toStdString()], colors[label.toStdString()],
                i);
    } else {
      m_tabWidget->removeTab(i);
    }
  }
}

/**
 * Update the tab
 * @param visiblePeaks Vector which determines which peaks are visible.
 * @param color
 * @param index The tab index.
 */
void PeaksTabWidget::updateTab(const std::vector<bool> &visiblePeaks,
                               const QColor &color, int index) {
  PeaksWidget *widget = qobject_cast<PeaksWidget *>(m_tabWidget->widget(index));
  widget->updateModel(std::move(visiblePeaks));
  m_tabWidget->tabBar()->setTabTextColor(index, color);
}

/**
 * Add a new tabs widget
 * @param peaksWorkspace A pointer to a peaksWorkspace
 * @param visiblePeaks Vector which determines which peaks are visible.
 */
void PeaksTabWidget::addNewPeaksWorkspace(
    Mantid::API::IPeaksWorkspace_sptr peaksWorkspace,
    std::vector<bool> visiblePeaks) {
  m_ws.push_back(std::move(peaksWorkspace));
  addNewTab(peaksWorkspace, peaksWorkspace->getName(), std::move(visiblePeaks));
}
} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid
