//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/DataComparison.h"

#include "MantidQtAPI/QwtWorkspaceSpectrumData.h"


namespace
{
  Mantid::Kernel::Logger g_log("DataComparison");
}

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomInterfaces
{
  DECLARE_SUBWINDOW(DataComparison);
}
}

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;


//----------------------
// Public member functions
//----------------------
///Constructor
DataComparison::DataComparison(QWidget *parent) :
  UserSubWindow(parent),
  m_plot(new QwtPlot(parent))
{
}


/// Set up the dialog layout
void DataComparison::initLayout()
{
  m_uiForm.setupUi(this);

  // Add the plot to the UI
  m_plot->setCanvasBackground(Qt::white);
  m_uiForm.loPlot->addWidget(m_plot);

  // Connect push buttons
  connect(m_uiForm.pbAddData, SIGNAL(clicked()), this, SLOT(addData()));

  connect(m_uiForm.pbRemoveSelectedData, SIGNAL(clicked()), this, SLOT(removeSelectedData()));
  connect(m_uiForm.pbRemoveAllData, SIGNAL(clicked()), this, SLOT(removeAllData()));

  connect(m_uiForm.pbDiffSelected, SIGNAL(clicked()), this, SLOT(diffSelected()));
  connect(m_uiForm.pbClearDiff, SIGNAL(clicked()), this, SLOT(clearDiff()));

  // Replot spectra when the spectrum index is changed
  connect(m_uiForm.sbSpectrum, SIGNAL(valueChanged(int)), this, SLOT(plotWorkspaces()));

  // Add headers to data table
  QStringList headerLabels;
  headerLabels << "Colour" << "Workspace" << "Offset" << "Spec.";
  m_uiForm.twCurrentData->setColumnCount(headerLabels.size());
  m_uiForm.twCurrentData->setHorizontalHeaderLabels(headerLabels);

  // Select entire rows when a cell is selected
  m_uiForm.twCurrentData->setSelectionBehavior(QAbstractItemView::SelectRows);

  // Fit columns
  m_uiForm.twCurrentData->resizeColumnsToContents();
}


/**
 * Adds the data currently selected by the data selector to the plot.
 */
void DataComparison::addData()
{
  QString dataName = m_uiForm.dsData->getCurrentDataName();

  m_uiForm.twCurrentData->blockSignals(true);

  // Append a new row to the data table
  int currentRows = m_uiForm.twCurrentData->rowCount();
  m_uiForm.twCurrentData->insertRow(currentRows);

  // Insert the colour selector
  QComboBox *colourCombo = new QComboBox();
  // Add colours
  colourCombo->addItem("Black", QVariant(Qt::black));
  colourCombo->addItem("Red", QVariant(Qt::red));
  colourCombo->addItem("Green", QVariant(Qt::green));
  colourCombo->addItem("Blue", QVariant(Qt::blue));
  colourCombo->addItem("Cyan", QVariant(Qt::cyan));
  colourCombo->addItem("Magenta", QVariant(Qt::magenta));
  colourCombo->addItem("Yellow", QVariant(Qt::yellow));
  colourCombo->addItem("Light Gray", QVariant(Qt::lightGray));
  colourCombo->addItem("Gray", QVariant(Qt::gray));
  colourCombo->addItem("Dark Red", QVariant(Qt::darkRed));
  colourCombo->addItem("Dark Green", QVariant(Qt::darkGreen));
  colourCombo->addItem("Dark Blue", QVariant(Qt::darkBlue));
  colourCombo->addItem("Dark Cyan", QVariant(Qt::darkCyan));
  colourCombo->addItem("Dark Magenta", QVariant(Qt::darkMagenta));
  colourCombo->addItem("Dark Yellow", QVariant(Qt::darkYellow));
  colourCombo->addItem("Dark Gray", QVariant(Qt::darkGray));
  // Set the initial colour
  colourCombo->setCurrentIndex(getInitialColourIndex());
  // Update plots when colour changed
  connect(colourCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(plotWorkspaces()));
  // Add widget to table
  m_uiForm.twCurrentData->setCellWidget(currentRows, COLOUR, colourCombo);

  // Insert the workspace name
  QTableWidgetItem *wsNameItem = new QTableWidgetItem(tr(dataName));
  wsNameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  m_uiForm.twCurrentData->setItem(currentRows, WORKSPACE_NAME, wsNameItem);

  // Insert the spectra offset
  QSpinBox *offsetSpin = new QSpinBox();
  offsetSpin->setMinimum(0);
  offsetSpin->setMaximum(INT_MAX);
  connect(offsetSpin, SIGNAL(valueChanged(int)), this, SLOT(updatePlot()));
  m_uiForm.twCurrentData->setCellWidget(currentRows, SPEC_OFFSET, offsetSpin);

  // Insert the current displayed spectra
  QTableWidgetItem *currentSpecItem = new QTableWidgetItem(tr("n/a"));
  currentSpecItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  m_uiForm.twCurrentData->setItem(currentRows, CURRENT_SPEC, currentSpecItem);

  m_uiForm.twCurrentData->blockSignals(false);

  // Fit columns
  m_uiForm.twCurrentData->resizeColumnsToContents();

  // Replot the workspaces
  plotWorkspaces();
}


/**
 * Gets a colour as an index for the combo box for a new workspace.
 * Looks for the lowest unused index, if all colours are used then returns 0.
 *
 * @return An index to set for the conbo box
 */
int DataComparison::getInitialColourIndex()
{
  int numRows = m_uiForm.twCurrentData->rowCount();

  // Just use the first colour if this is the first row
  if(numRows <= 1)
    return 0;

  // Build a list of used colours
  QList<int> usedColours;
  for(int row = 0; row < numRows - 1; row++)
  {
    QComboBox *colourSelector = dynamic_cast<QComboBox *>(m_uiForm.twCurrentData->cellWidget(row, COLOUR));
    int index = colourSelector->currentIndex();
    usedColours << index;
  }

  // Find the smallest unused colour
  int numColours = dynamic_cast<QComboBox *>(m_uiForm.twCurrentData->cellWidget(0, COLOUR))->count();
  for(int i = 0; i < numColours; i++)
  {
    if(!usedColours.contains(i))
      return i;
  }

  return 0;
}


/**
 * Removes the data currently selected in the table from the plot.
 */
void DataComparison::removeSelectedData()
{
  QList<QTableWidgetItem *> selectedItems = m_uiForm.twCurrentData->selectedItems();

  while(!selectedItems.isEmpty())
  {
    // Get the row number of the item
    int row = selectedItems[0]->row();

    // Get workspace name
    QString workspaceName = m_uiForm.twCurrentData->item(row, WORKSPACE_NAME)->text();

    // Remove from data tabel
    m_uiForm.twCurrentData->removeRow(row);

    // Detach the old curve from the plot if it exists
    if(m_curves.contains(workspaceName))
      m_curves[workspaceName]->attach(NULL);

    selectedItems = m_uiForm.twCurrentData->selectedItems();
  }

  // Replot the workspaces
  updatePlot();
}


/**
 * Removed all loaded data from the plot.
 */
void DataComparison::removeAllData()
{
  int numRows = m_uiForm.twCurrentData->rowCount();
  for(int row = 0; row < numRows; row++)
  {
    // Get workspace name
    QString workspaceName = m_uiForm.twCurrentData->item(0, WORKSPACE_NAME)->text();

    // Remove from data tabel
    m_uiForm.twCurrentData->removeRow(0);

    // Detach the old curve from the plot if it exists
    if(m_curves.contains(workspaceName))
      m_curves[workspaceName]->attach(NULL);
  }

  // Replot the workspaces
  plotWorkspaces();
}


/**
 * Replots the currently loaded workspaces.
 */
void DataComparison::plotWorkspaces()
{
  int globalSpecIndex = m_uiForm.sbSpectrum->value();
  int maxGlobalSpecIndex = 0;

  int numRows = m_uiForm.twCurrentData->rowCount();
  for(int row = 0; row < numRows; row++)
  {
    // Get workspace
    QString workspaceName = m_uiForm.twCurrentData->item(row, WORKSPACE_NAME)->text();
    MatrixWorkspace_const_sptr workspace =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName.toStdString());
    int numSpec = static_cast<int>(workspace->getNumberHistograms());

    // Calculate spectrum number
    QSpinBox *specOffsetSpin = dynamic_cast<QSpinBox *>(m_uiForm.twCurrentData->cellWidget(row, SPEC_OFFSET));
    int specOffset = specOffsetSpin->value();
    int specIndex = globalSpecIndex - specOffset;
    g_log.debug() << "Spectrum index for workspace " << workspaceName.toStdString()
                  << " is " << specIndex << ", with offset " << specOffset << std::endl;

    // See if this workspace extends the reach of the global spectrum selector
    int maxGlobalSpecIndexForWs = numSpec + specOffset - 1;
    if(maxGlobalSpecIndexForWs > maxGlobalSpecIndex)
      maxGlobalSpecIndex = maxGlobalSpecIndexForWs;

    // Check the spectrum index is in range
    if(specIndex >= numSpec || specIndex < 0)
    {
      g_log.debug() << "Workspace " << workspaceName.toStdString()
                    << ", spectrum index out of range." << std::endl;;

      // Give "n/a" in current spectrum display
      m_uiForm.twCurrentData->item(row, CURRENT_SPEC)->setText(tr("n/a"));

      // Detech the curve from the plot
      if(m_curves.contains(workspaceName))
        m_curves[workspaceName]->attach(NULL);

      continue;
    }

    // Update current spectrum display
    m_uiForm.twCurrentData->item(row, CURRENT_SPEC)->setText(tr(QString::number(specIndex)));

    // Create the curve data
    const bool logScale(false), distribution(false);
    QwtWorkspaceSpectrumData wsData(*workspace, static_cast<int>(specIndex), logScale, distribution);

    // Detach the old curve from the plot if it exists
    if(m_curves.contains(workspaceName))
      m_curves[workspaceName]->attach(NULL);

    QComboBox *colourSelector = dynamic_cast<QComboBox *>(m_uiForm.twCurrentData->cellWidget(row, COLOUR));
    QColor curveColour = colourSelector->itemData(colourSelector->currentIndex()).value<QColor>();

    // Create a new curve and attach it to the plot
    boost::shared_ptr<QwtPlotCurve> curve(new QwtPlotCurve);
    curve->setData(wsData);
    curve->setPen(curveColour);
    curve->attach(m_plot);
    m_curves[workspaceName] = curve;
  }

  // Update the plot
  m_plot->replot();

  // Set the max value for global spectrum spin box
  m_uiForm.sbSpectrum->setMaximum(maxGlobalSpecIndex);
  m_uiForm.sbSpectrum->setSuffix(" / " + QString::number(maxGlobalSpecIndex));
}


/**
 * Normalises the spectrum index offsets in the data table to zero.
 */
void DataComparison::normaliseSpectraOffsets()
{
  m_uiForm.twCurrentData->blockSignals(true);

  int numRows = m_uiForm.twCurrentData->rowCount();
  int lowestOffset = INT_MAX;

  // Find the lowest offset in the data table
  for(int row = 0; row < numRows; row++)
  {
    QSpinBox *specOffsetSpin = dynamic_cast<QSpinBox *>(m_uiForm.twCurrentData->cellWidget(row, SPEC_OFFSET));
    int specOffset = specOffsetSpin->value();
    if(specOffset < lowestOffset)
      lowestOffset = specOffset;
  }

  // Subtract the loest offset from all offsets to ensure at least one offset is zero
  for(int row = 0; row < numRows; row++)
  {
    QSpinBox *specOffsetSpin = dynamic_cast<QSpinBox *>(m_uiForm.twCurrentData->cellWidget(row, SPEC_OFFSET));
    int specOffset = specOffsetSpin->value();
    specOffset -= lowestOffset;
    specOffsetSpin->setValue(specOffset);
  }

  m_uiForm.twCurrentData->blockSignals(false);
}


/**
 * Handles updating the plot, i.e. normalising offsets and replotting spectra.
 */
void DataComparison::updatePlot()
{
  normaliseSpectraOffsets();
  plotWorkspaces();
}


/**
 * Creates a diff workspace of the two currently selected workspaces in the table
 * and plots it on the plot.
 *
 * Does nothing if there are not 2 workspaces selected.
 */
void DataComparison::diffSelected()
{
  //TODO
}


/**
 * Removes the diff workspace form the plot.
 *
 * Does not remove it from ADS.
 */
void DataComparison::clearDiff()
{
  //TODO
}
