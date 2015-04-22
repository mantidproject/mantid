#include "MantidVatesSimpleGuiViewWidgets/PeaksWidget.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidQtSliceViewer/QPeaksTableModel.h"

#include <QWidget>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <vector>
#include <string>
#include <map>

namespace Mantid {
namespace Vates {
namespace SimpleGui {
/**
Constructor

@param ws : Peaks Workspace (MODEL)
@param coordinateSystem : Name of coordinate system used
@param parent : parent widget
*/
PeaksWidget::PeaksWidget(Mantid::API::IPeaksWorkspace_sptr ws,
                         const std::string &coordinateSystem, QWidget *parent)
    : QWidget(parent), m_ws(ws), m_coordinateSystem(coordinateSystem), m_originalTableWidth(1) {
  ui.setupUi(this);
}

/**
 * Setup the Table model
 * @param visiblePeaks : A vector of lists of visible peaks for each peak
 * workspace
 */
void PeaksWidget::setupMvc(std::vector<bool> visiblePeaks) {
  // Create new table view
  MantidQt::SliceViewer::QPeaksTableModel *model =
      new MantidQt::SliceViewer::QPeaksTableModel(m_ws);
  QObject::connect(model, SIGNAL(peaksSorted(const std::string &, const bool)),
                   this, SLOT(onPeaksSorted(const std::string &, const bool)));
  ui.tblPeaks->setModel(model);
  const std::vector<int> hideCols = model->defaultHideCols();
  for (auto it = hideCols.begin(); it != hideCols.end(); ++it)
    ui.tblPeaks->setColumnHidden(*it, true);
  ui.tblPeaks->verticalHeader()->setResizeMode(QHeaderView::Interactive);
  ui.tblPeaks->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
  m_originalTableWidth = ui.tblPeaks->horizontalHeader()->length();
  // calculate the average width (in pixels) of numbers
  QString allNums("0123456789");
  double char_width =
      static_cast<double>(
          ui.tblPeaks->fontMetrics().boundingRect(allNums).width()) /
      static_cast<double>(allNums.size());
  // set the starting width of each column
  for (int i = 0; i < m_originalTableWidth; ++i) {
    double width =
        static_cast<double>(model->numCharacters(i) + 3) * char_width;
    ui.tblPeaks->horizontalHeader()->resizeSection(i, static_cast<int>(width));
  }

  // Set the visible rows
  for (size_t i = 0; i < visiblePeaks.size(); i++) {
    if (visiblePeaks[i]) {
      ui.tblPeaks->showRow(static_cast<int>(i));
    } else {
      ui.tblPeaks->hideRow(static_cast<int>(i));
    }
  }
  QItemSelectionModel *selectionModel = ui.tblPeaks->selectionModel();
  connect(selectionModel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
          this, SLOT(onCurrentChanged(QModelIndex, QModelIndex)));
}

/**
 * Detects a newly selectedd peaks workspace.
 * @param current The currently selected index.
 */
void PeaksWidget::onCurrentChanged(QModelIndex current, QModelIndex) {
  if (current.isValid()) {
    emit zoomToPeak(m_ws, current.row());
  }
}

/**
 * Update the visibility of the underlying model
 * @param visiblePeaks A vector indicating which peaks are visible.
 */
void PeaksWidget::updateModel(std::vector<bool> visiblePeaks) {
  for (size_t i = 0; i < visiblePeaks.size(); i++) {
    if (visiblePeaks[i]) {
      ui.tblPeaks->showRow(static_cast<int>(i));
    } else {
      ui.tblPeaks->hideRow(static_cast<int>(i));
    }
  }
}

/**
 * Sort the peak workspace according to the a selected column
 * @param columnToSortBy The column by which to sort.
 * @param sortAscending If sort ascending or descending.
 */
void PeaksWidget::onPeaksSorted(const std::string &columnToSortBy,
                                const bool sortAscending) {
  emit sortPeaks(columnToSortBy, sortAscending, m_ws);
}

} // namespace
}
}
