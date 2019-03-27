// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/SliceViewer/PeaksWorkspaceWidget.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/SliceViewer/PeaksViewer.h"
#include "MantidQtWidgets/SliceViewer/QPeaksTableModel.h"
#include <QColorDialog>
#include <QPlastiqueStyle>
#include <QSortFilterProxyModel>

namespace {
QColor getSelectedColor() {
  QColorDialog colourDlg;
  colourDlg.result();
  return colourDlg.getColor();
}
} // namespace

namespace MantidQt {
namespace SliceViewer {

using MantidQt::API::SignalBlocker;

/**
Constructor

@param ws : Peaks Workspace (MODEL)
@param coordinateSystem : Name of coordinate system used
@param defaultForegroundPeakViewColor : Default peak foreground colour
@param defaultBackgroundPeakViewColor : Default peak background colour
@param parent : parent widget
*/
PeaksWorkspaceWidget::PeaksWorkspaceWidget(
    Mantid::API::IPeaksWorkspace_const_sptr ws,
    const std::string &coordinateSystem,
    PeakViewColor defaultForegroundPeakViewColor,
    PeakViewColor defaultBackgroundPeakViewColor, PeaksViewer *parent)
    : QWidget(parent), m_ws(ws), m_coordinateSystem(coordinateSystem),
      m_foregroundPeakViewColor(defaultForegroundPeakViewColor),
      m_backgroundPeakViewColor(defaultBackgroundPeakViewColor),
      m_parent(parent) {

  ui.setupUi(this);

  // Connect internal signals-slots.
  connect(ui.ckShowBackground, SIGNAL(clicked(bool)), this,
          SLOT(onShowBackgroundChanged(bool)));

  // Colors for Cross -- there is no background color
  connect(ui.btnPeakColor, SIGNAL(clicked()), this,
          SLOT(onForegroundColorCrossClicked()));

  // Colors for Sphere
  connect(ui.btnBackgroundColorSphere, SIGNAL(clicked()), this,
          SLOT(onBackgroundColorSphereClicked()));
  connect(ui.btnPeakColorSphere, SIGNAL(clicked()), this,
          SLOT(onForegroundColorSphereClicked()));

  // Colors for Ellipsoid
  connect(ui.btnBackgroundColorEllipsoid, SIGNAL(clicked()), this,
          SLOT(onBackgroundColorEllipsoidClicked()));
  connect(ui.btnPeakColorEllipsoid, SIGNAL(clicked()), this,
          SLOT(onForegroundColorEllipsoidClicked()));

  connect(ui.btnRemove, SIGNAL(clicked()), this,
          SLOT(onRemoveWorkspaceClicked()));
  connect(ui.btnHide, SIGNAL(clicked()), this, SLOT(onToggleHideInPlot()));
  connect(ui.btnAddPeak, SIGNAL(toggled(bool)), this,
          SLOT(onAddPeaksToggled(bool)));
  connect(ui.btnRemovePeak, SIGNAL(toggled(bool)), this,
          SLOT(onClearPeaksToggled(bool)));

  // Override the styles for the colour buttons, because with some inherited
  // styles, the button background colour will be hidden.
  ui.btnPeakColor->setStyle(new QPlastiqueStyle);

  ui.btnBackgroundColorSphere->setStyle(new QPlastiqueStyle);
  ui.btnPeakColorSphere->setStyle(new QPlastiqueStyle);
  ui.btnBackgroundColorEllipsoid->setStyle(new QPlastiqueStyle);
  ui.btnPeakColorEllipsoid->setStyle(new QPlastiqueStyle);

  ui.ckShowBackground->setVisible(true);
  ui.lblShowBackgroundColour->setVisible(true);
  ui.btnAddPeak->setEnabled(true);

  // Populate controls with data.
  populate();

  QItemSelectionModel *selectionModel = ui.tblPeaks->selectionModel();
  connect(selectionModel, SIGNAL(currentChanged(QModelIndex, QModelIndex)),
          this, SLOT(onCurrentChanged(QModelIndex, QModelIndex)));
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

void PeaksWorkspaceWidget::setShownColumns(const std::set<QString> &cols) {
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
  QPeaksTableModel *model = new QPeaksTableModel(this->m_ws);

  m_tableModel = new QSortFilterProxyModel(this);
  m_tableModel->setSourceModel(model);
  m_tableModel->setDynamicSortFilter(true);
  m_tableModel->setSortRole(Qt::UserRole);
  ui.tblPeaks->setModel(m_tableModel);

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
  m_nameText = QString(m_ws->getName().c_str());
  ui.lblWorkspaceName->setText(m_nameText);
  ui.lblWorkspaceName->setToolTip(m_nameText);

  const QString integratedText =
      "Integrated: " + QString(m_ws->hasIntegratedPeaks() ? "Yes" : "No");

  ui.lblWorkspaceState->setText(integratedText);
  ui.lblWorkspaceState->setToolTip(integratedText);

  const QString coordinateText = QString(m_coordinateSystem.c_str());
  ui.lblWorkspaceCoordinates->setText("Coords: " + coordinateText);
  ui.lblWorkspaceCoordinates->setToolTip(coordinateText);

  // Set the default colors
  QPalette palette;
  palette.setColor(ui.btnPeakColor->backgroundRole(),
                   m_foregroundPeakViewColor.colorCross);
  ui.btnPeakColor->setPalette(palette);

  palette.setColor(ui.btnPeakColorSphere->backgroundRole(),
                   m_foregroundPeakViewColor.colorSphere);
  ui.btnPeakColorSphere->setPalette(palette);

  palette.setColor(ui.btnPeakColorEllipsoid->backgroundRole(),
                   m_foregroundPeakViewColor.colorEllipsoid);
  ui.btnPeakColorEllipsoid->setPalette(palette);

  palette.setColor(ui.btnBackgroundColorSphere->backgroundRole(),
                   m_backgroundPeakViewColor.colorSphere);
  ui.btnBackgroundColorSphere->setPalette(palette);

  palette.setColor(ui.btnBackgroundColorEllipsoid->backgroundRole(),
                   m_backgroundPeakViewColor.colorSphere);
  ui.btnBackgroundColorEllipsoid->setPalette(palette);

  // Setup table
  createTableMVC();
}

/// Destructor
PeaksWorkspaceWidget::~PeaksWorkspaceWidget() {}

/**
 * Handler for changing the foreground colour of an integrated peak.
 */
void PeaksWorkspaceWidget::onForegroundPeakViewColorClicked() {
  auto foregroundColorCross = ui.btnPeakColor->palette().button().color();
  auto foregroundColorSphere =
      ui.btnPeakColorSphere->palette().button().color();
  auto foregroundColorEllipsoid =
      ui.btnPeakColorEllipsoid->palette().button().color();
  PeakViewColor color(foregroundColorCross, foregroundColorSphere,
                      foregroundColorEllipsoid);
  emit peakColorchanged(this->m_ws, color);
}

/**
 * Handler for changing the background colour of an integrated peak.
 */
void PeaksWorkspaceWidget::onBackgroundPeakViewColorClicked() {
  auto backgroundColorSphere =
      ui.btnBackgroundColorSphere->palette().button().color();
  auto backgroundColorEllipsoid =
      ui.btnBackgroundColorEllipsoid->palette().button().color();
  // The first entry will be negelected as there is no background color for
  // cross
  PeakViewColor color(backgroundColorSphere, backgroundColorSphere,
                      backgroundColorEllipsoid);
  emit backgroundColorChanged(this->m_ws, color);
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
 * Get the workspace model.
 * @return workspace around which this is built.
 */
Mantid::API::IPeaksWorkspace_const_sptr
PeaksWorkspaceWidget::getPeaksWorkspace() const {
  return m_ws;
}

/**
 * Set the background color
 * @param backgroundColor: a peak view color for the background buttons
 */
void PeaksWorkspaceWidget::setBackgroundColor(
    const PeakViewColor &backgroundColor) {
  // Need to set the three buttons sepearately
  auto backgroundColorSphere = backgroundColor.colorSphere;
  auto backgroundColorEllipsoid = backgroundColor.colorEllipsoid;

  QPalette palette;
  palette.setColor(ui.btnBackgroundColorSphere->backgroundRole(),
                   backgroundColorSphere);
  ui.btnBackgroundColorSphere->setPalette(palette);

  palette.setColor(ui.btnBackgroundColorEllipsoid->backgroundRole(),
                   backgroundColorEllipsoid);
  ui.btnBackgroundColorEllipsoid->setPalette(palette);
}

/**
 * Set the foreground color
 * @param foregroundColor
 */
void PeaksWorkspaceWidget::setForegroundColor(
    const PeakViewColor &foregroundColor) {
  // Need to set the three buttons sepearately
  auto foregroundColorCross = foregroundColor.colorCross;
  auto foregroundColorSphere = foregroundColor.colorSphere;
  auto foregroundColorEllipsoid = foregroundColor.colorEllipsoid;

  QPalette palette;

  palette.setColor(ui.btnPeakColor->backgroundRole(), foregroundColorCross);
  ui.btnPeakColor->setPalette(palette);

  palette.setColor(ui.btnPeakColorSphere->backgroundRole(),
                   foregroundColorSphere);
  ui.btnPeakColorSphere->setPalette(palette);

  palette.setColor(ui.btnPeakColorEllipsoid->backgroundRole(),
                   foregroundColorEllipsoid);
  ui.btnPeakColorEllipsoid->setPalette(palette);
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
  ui.tblPeaks->setCurrentIndex(m_tableModel->index(index, 0));
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
  auto model = static_cast<QPeaksTableModel *>(m_tableModel->sourceModel());
  model->setPeaksWorkspace(m_ws);

  // Update the display name of the workspace.
  m_nameText = m_ws->getName().c_str();
  this->ui.lblWorkspaceName->setText(m_nameText);
}

/**
 * @brief PeaksWorkspaceWidget::onCurrentChanged
 * @param index : Index of the table newly selected
 */
void PeaksWorkspaceWidget::onCurrentChanged(QModelIndex index, QModelIndex /*unused*/) {
  if (index.isValid()) {
    index = m_tableModel->mapToSource(index);
    emit zoomToPeak(this->m_ws, index.row());
  }
}

/**
 * @brief PeaksWorkspaceWidget::onClearPeaksToggled
 * @param on : Enter mode
 */
void PeaksWorkspaceWidget::onClearPeaksToggled(bool on) {
  // We should now tell the PeaksViewer about this.
  m_parent->clearPeaksModeRequest(this, on);
}

/**
 * @brief PeaksWorkspaceWidget::onAddPeaksToggled
 * @param on : Enter mode
 */
void PeaksWorkspaceWidget::onAddPeaksToggled(bool on) {
  // We should now tell the PeaksViewer about this. It should have a global mode
  // for AddingPeaks it merely needs to know the destination workspace
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

void PeaksWorkspaceWidget::onForegroundColorCrossClicked() {
  auto selectedColor = getSelectedColor();
  if (selectedColor.isValid()) {
    QPalette palette;
    palette.setColor(ui.btnPeakColor->backgroundRole(), selectedColor);
    ui.btnPeakColor->setPalette(palette);
    onForegroundPeakViewColorClicked();
  }
}

void PeaksWorkspaceWidget::onBackgroundColorSphereClicked() {
  auto selectedColor = getSelectedColor();
  if (selectedColor.isValid()) {
    QPalette palette;
    palette.setColor(ui.btnBackgroundColorSphere->backgroundRole(),
                     selectedColor);
    ui.btnBackgroundColorSphere->setPalette(palette);
    onBackgroundPeakViewColorClicked();
  }
}

void PeaksWorkspaceWidget::onForegroundColorSphereClicked() {
  auto selectedColor = getSelectedColor();
  if (selectedColor.isValid()) {
    QPalette palette;
    palette.setColor(ui.btnPeakColorSphere->backgroundRole(), selectedColor);
    ui.btnPeakColorSphere->setPalette(palette);
    onForegroundPeakViewColorClicked();
  }
}

void PeaksWorkspaceWidget::onBackgroundColorEllipsoidClicked() {
  auto selectedColor = getSelectedColor();
  if (selectedColor.isValid()) {
    QPalette palette;
    palette.setColor(ui.btnBackgroundColorEllipsoid->backgroundRole(),
                     selectedColor);
    ui.btnBackgroundColorEllipsoid->setPalette(palette);
    onBackgroundPeakViewColorClicked();
  }
}

void PeaksWorkspaceWidget::onForegroundColorEllipsoidClicked() {
  auto selectedColor = getSelectedColor();
  if (selectedColor.isValid()) {
    QPalette palette;
    palette.setColor(ui.btnPeakColorEllipsoid->backgroundRole(), selectedColor);
    ui.btnPeakColorEllipsoid->setPalette(palette);
    onForegroundPeakViewColorClicked();
  }
}

} // namespace SliceViewer
} // namespace MantidQt
