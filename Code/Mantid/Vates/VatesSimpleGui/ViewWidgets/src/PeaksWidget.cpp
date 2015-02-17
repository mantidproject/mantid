#include "MantidVatesSimpleGuiViewWidgets/PeaksWidget.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidQtSliceViewer/QPeaksTableModel.h"
#include <QWidget>
#include <QItemSelectionModel>
#include <QModelIndex>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
/**
Constructor

@param ws : Peaks Workspace (MODEL)
@param coordinateSystem : Name of coordinate system used
@param parent : parent widget
*/
PeaksWidget::PeaksWidget(Mantid::API::IPeaksWorkspace_sptr ws, const std::string &coordinateSystem, QWidget *parent)  : QWidget(parent), m_ws(ws), m_coordinateSystem(coordinateSystem){
  ui.setupUi(this);
}

/**
 * Setup the Table model 
 * @param visiblePeaks : A list of visible peaks
 */
void PeaksWidget::setupMvc(std::vector<bool> visiblePeaks)
{
  MantidQt::SliceViewer::QPeaksTableModel* model = new  MantidQt::SliceViewer::QPeaksTableModel(this->m_ws);
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

  // Hide the rows which are invisible
  for (int i = 0; i < ui.tblPeaks->model()->rowCount(); i++)
  {
    if (visiblePeaks[i])
    {
      ui.tblPeaks->showRow(i);
    }
    else
    {
      ui.tblPeaks->hideRow(i);
    }  
  }

  QItemSelectionModel* selectionModel = ui.tblPeaks->selectionModel();
  connect(selectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(onCurrentChanged(QModelIndex, QModelIndex)));
}

/**
 * Detects a newly selectedd peaks workspace.
 * @param current The currently selected index.
 */
void PeaksWidget::onCurrentChanged(QModelIndex current, QModelIndex)
{
  if (current.isValid()) 
  {
    emit zoomToPeak(this->m_ws, current.row());
  }
}
} // namespace
}
}
