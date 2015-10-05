#include "MantidQtSliceViewer/PeaksWorkspaceWidget.h"
#include "MantidQtSliceViewer/PeaksViewer.h"
#include "MantidQtSliceViewer/QPeaksTableModel.h"
#include "MantidQtAPI/SignalBlocker.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include <QColorDialog>
#include <QPlastiqueStyle>



namespace MantidQt {
namespace SliceViewer {

using MantidQt::API::SignalBlocker;

/**
Constructor

@param ws : Peaks Workspace (MODEL)
@param coordinateSystem : Name of coordinate system used
@param defaultForegroundColour : Default peak foreground colour
@param defaultBackgroundColour : Default peak background colour
@param canAddPeaks : Flag to indicate that peaks can be added. False for no add mode.
@param parent : parent widget
*/
PeaksWorkspaceWidget::PeaksWorkspaceWidget(
    Mantid::API::IPeaksWorkspace_const_sptr ws,
    const std::string &coordinateSystem, const QColor &defaultForegroundColour,
    const QColor &defaultBackgroundColour, const bool canAddPeaks, PeaksViewer *parent)
    : QWidget(parent), m_ws(ws), m_coordinateSystem(coordinateSystem),
      m_foregroundColour(defaultForegroundColour),
      m_backgroundColour(defaultBackgroundColour),
      m_parent(parent) {

  ui.setupUi(this);

  // Connect internal signals-slots.
  connect(ui.ckShowBackground, SIGNAL(clicked(bool)), this,
          SLOT(onShowBackgroundChanged(bool)));
  connect(ui.btnBackgroundColor, SIGNAL(clicked()), this,
          SLOT(onBackgroundColourClicked()));
  connect(ui.btnPeakColor, SIGNAL(clicked()), this,
          SLOT(onForegroundColourClicked()));
  connect(ui.btnRemove, SIGNAL(clicked()), this,
          SLOT(onRemoveWorkspaceClicked()));
  connect(ui.btnHide, SIGNAL(clicked()), this, SLOT(onToggleHideInPlot()));
  connect(ui.btnAddPeak, SIGNAL(toggled(bool)), this, SLOT(onAddPeaksToggled(bool)));
  connect(ui.btnRemovePeak, SIGNAL(toggled(bool)), this, SLOT(onClearPeaksToggled(bool)));

  // Override the styles for the colour buttons, because with some inherited
  // styles, the button background colour will be hidden.
  ui.btnBackgroundColor->setStyle(new QPlastiqueStyle);
  ui.btnPeakColor->setStyle(new QPlastiqueStyle);

  // Hide controls that don't apply when peaks are integrated.
  const bool integratedPeaks = m_ws->hasIntegratedPeaks();
  ui.btnBackgroundColor->setVisible(integratedPeaks);
  ui.ckShowBackground->setVisible(integratedPeaks);
  ui.lblShowBackgroundColour->setVisible(integratedPeaks);

  // Don't allow peaks to be added if it has been forbidden
  ui.btnAddPeak->setEnabled(canAddPeaks);

  // Populate controls with data.
  populate();

  QItemSelectionModel* selectionModel = ui.tblPeaks->selectionModel();
  connect(selectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(onCurrentChanged(QModelIndex, QModelIndex)));

}

std::set<QString> PeaksWorkspaceWidget::getShownColumns() {

  std::set<QString> result;
  auto numCols = ui.tblPeaks->model()->columnCount();
  for (auto i = 0; i < numCols; ++i) {
    if (!ui.tblPeaks->isColumnHidden(i))
      result.insert(ui.tblPeaks->model()
                        ->headerData(i, Qt::Horizontal, Qt::DisplayRole)
                        .toString());
  }
  return result;
}

void PeaksWorkspaceWidget::setShownColumns(std::set<QString> &cols) {
  auto numCols = ui.tblPeaks->model()->columnCount();
  for (auto i = 0; i < numCols; ++i) {
    const QString name = ui.tblPeaks->model()
                             ->headerData(i, Qt::Horizontal, Qt::DisplayRole)
                             .toString();
    bool hide(cols.find(name) == cols.end());
    ui.tblPeaks->setColumnHidden(i, hide);
  }
}

/**
Populate controls with data ready for rendering.
*/
void PeaksWorkspaceWidget::createTableMVC() {
  QPeaksTableModel* model = new QPeaksTableModel(this->m_ws);
  connect(model, SIGNAL(peaksSorted(const std::string &, const bool)), this,
          SLOT(onPeaksSorted(const std::string &, const bool)));
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
}

void PeaksWorkspaceWidget::populate() {
  m_nameText = QString(m_ws->name().c_str());
  ui.lblWorkspaceName->setText(m_nameText);
  ui.lblWorkspaceName->setToolTip(m_nameText);

  const QString integratedText =
      "Integrated: " + QString(m_ws->hasIntegratedPeaks() ? "Yes" : "No");

  ui.lblWorkspaceState->setText(integratedText);
  ui.lblWorkspaceState->setToolTip(integratedText);

  const QString coordinateText = QString(m_coordinateSystem.c_str());
  ui.lblWorkspaceCoordinates->setText("Coords: " + coordinateText);
  ui.lblWorkspaceCoordinates->setToolTip(coordinateText);

  ui.btnBackgroundColor->setBackgroundColor(m_backgroundColour);
  ui.btnPeakColor->setBackgroundColor(m_foregroundColour);

  // Setup table
  createTableMVC();
}

/// Destructor
PeaksWorkspaceWidget::~PeaksWorkspaceWidget() {}

/**
Handler for changing the foreground colour of an integrated peak.
*/
void PeaksWorkspaceWidget::onForegroundColourClicked() {
  QColorDialog colourDlg;
  colourDlg.result();
  QColor selectedColour = colourDlg.getColor();
  if (selectedColour.isValid()) {
    ui.btnPeakColor->setBackgroundColor(selectedColour);
    emit peakColourChanged(this->m_ws, selectedColour);
  }
}

/**
Handler for changing the background colour of an integrated peak.
*/
void PeaksWorkspaceWidget::onBackgroundColourClicked() {
  QColorDialog colourDlg;
  QColor selectedColour = colourDlg.getColor();
  if (selectedColour.isValid()) {
    ui.btnBackgroundColor->setBackgroundColor(selectedColour);
    emit backgroundColourChanged(this->m_ws, selectedColour);
  }
}

/**
Handler or showing/hiding the background radius of integrated peaks.
@param show : TRUE if the background radius is to be shown.
*/
void PeaksWorkspaceWidget::onShowBackgroundChanged(bool show) {
  emit backgroundRadiusShown(this->m_ws, show);
}

/**
Handler for removing a workspace from the plotting tools.
*/
void PeaksWorkspaceWidget::onRemoveWorkspaceClicked() {
  emit removeWorkspace(this->m_ws);
}

/**
Handler to hide/show the widget on request.
*/
void PeaksWorkspaceWidget::onToggleHideInPlot() {
  emit hideInPlot(this->m_ws, ui.btnHide->isChecked());
}

/**
 * Handler for sorting of the peaks workspace.
 * @param columnToSortBy
 * @param sortAscending
 */
void PeaksWorkspaceWidget::onPeaksSorted(const std::string &columnToSortBy,
                                         const bool sortAscending) {
  emit peaksSorted(columnToSortBy, sortAscending, this->m_ws);
}

/**
 * Get the workspace model.
 * @return workspace around which this is built.
 */
Mantid::API::IPeaksWorkspace_const_sptr
PeaksWorkspaceWidget::getPeaksWorkspace() const {
  return m_ws;
}

/**
 * Set the background color
 * @param backgroundColor
 */
void PeaksWorkspaceWidget::setBackgroundColor(const QColor &backgroundColor) {
  ui.btnBackgroundColor->setBackgroundColor(backgroundColor);
}

/**
 * Set the foreground color
 * @param foregroundColor
 */
void PeaksWorkspaceWidget::setForegroundColor(const QColor &foregroundColor) {
  ui.btnPeakColor->setBackgroundColor(foregroundColor);
}

/**
 * Set show/hide background
 * @param showBackground
 */
void PeaksWorkspaceWidget::setShowBackground(bool showBackground) {
  ui.ckShowBackground->setChecked(showBackground);
}

/**
 * @brief Handler for hiding/showing the peaks
 * @param isHidden : true to hide
 */
void PeaksWorkspaceWidget::setHidden(bool isHidden) {
  ui.btnHide->setChecked(isHidden);
}

/**
 * @brief Set the selected peak
 * @param index : index to set as selected
 */
void PeaksWorkspaceWidget::setSelectedPeak(int index) {
  ui.tblPeaks->clearSelection();
  ui.tblPeaks->setCurrentIndex(ui.tblPeaks->model()->index(index, 0));
}

/**
 * @brief PeaksWorkspaceWidget::getWSName
 * @return return the workspace name
 */
std::string PeaksWorkspaceWidget::getWSName() const {
  return m_nameText.toStdString();
}

/**
 * @brief PeaksWorkspaceWidget::workspaceUpdate
 * @param ws : Workspace to redisplay with
 */
void PeaksWorkspaceWidget::workspaceUpdate(
    Mantid::API::IPeaksWorkspace_const_sptr ws) {
  // Only if we provide a peaks workspace for replacement.
  if (ws) {
    m_ws = ws;
  }
  // Set at new representation for the model.
  static_cast<QPeaksTableModel*>(this->ui.tblPeaks->model())->setPeaksWorkspace(m_ws);
  // Update the display name of the workspace.
  m_nameText = m_ws->getName().c_str();
  this->ui.lblWorkspaceName->setText(m_nameText);
}

/**
 * @brief PeaksWorkspaceWidget::onCurrentChanged
 * @param index : Index of the table newly selected
 */
void PeaksWorkspaceWidget::onCurrentChanged(QModelIndex index, QModelIndex)
{
    if (index.isValid()) {
      emit zoomToPeak(this->m_ws, index.row());
    }
}

/**
 * @brief PeaksWorkspaceWidget::onClearPeaksToggled
 * @param on : Enter mode
 */
void PeaksWorkspaceWidget::onClearPeaksToggled(bool on)
{
    //We should now tell the PeaksViewer about this.
    m_parent->clearPeaksModeRequest(this, on);
}

/**
 * @brief PeaksWorkspaceWidget::onAddPeaksToggled
 * @param on : Enter mode
 */
void PeaksWorkspaceWidget::onAddPeaksToggled(bool on)
{
    // We should now tell the PeaksViewer about this. It should have a global mode for AddingPeaks it merely needs to know the destination workspace
    m_parent->addPeaksModeRequest(this, on);
}

void PeaksWorkspaceWidget::exitClearPeaksMode() {
    SignalBlocker<QPushButton> scopedBlocker(ui.btnRemovePeak);
    scopedBlocker->setChecked(false);
}

void PeaksWorkspaceWidget::exitAddPeaksMode() {
    SignalBlocker<QPushButton> scopedBlocker(ui.btnAddPeak);
    scopedBlocker->setChecked(false);
}

} // namespace
}
