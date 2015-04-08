#include "MantidQtCustomInterfaces/MultiDatasetFit/MDFDataController.h"
#include "MantidQtCustomInterfaces/MultiDatasetFit/MultiDatasetFit.h"
#include "MantidQtCustomInterfaces/MultiDatasetFit/MDFAddWorkspaceDialog.h"

#include <QTableWidget>
#include <QMessageBox>

namespace{
  // columns in the data table
  const int wsColumn      = 0;
  const int wsIndexColumn = 1;
  const int startXColumn  = 2;
  const int endXColumn    = 3;

  QString makeNumber(double d) {return QString::number(d,'g',16);}
}

namespace MantidQt
{
namespace CustomInterfaces
{
namespace MDF
{

/// Constructor.
DataController::DataController(MultiDatasetFit *parent, QTableWidget *dataTable):
  QObject(parent),m_dataTable(dataTable),m_isFittingRangeGlobal(false)
{
  connect(dataTable,SIGNAL(itemSelectionChanged()), this,SLOT(workspaceSelectionChanged()));
  connect(dataTable,SIGNAL(cellChanged(int,int)),this,SLOT(updateDataset(int,int)));
}

/// Show a dialog to select a workspace from the ADS.
void DataController::addWorkspace()
{
  AddWorkspaceDialog dialog(owner());
  if ( dialog.exec() == QDialog::Accepted )
  {
    QString wsName = dialog.workspaceName().stripWhiteSpace();
    // if name is empty assume that there are no workspaces in the ADS
    if ( wsName.isEmpty() ) return;
    if ( Mantid::API::AnalysisDataService::Instance().doesExist( wsName.toStdString()) )
    {
      auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>( wsName.toStdString() );
      auto indices = dialog.workspaceIndices();
      for(auto i = indices.begin(); i != indices.end(); ++i)
      {
        addWorkspaceSpectrum( wsName, *i, *ws );
      }
      emit spectraAdded(static_cast<int>(indices.size()));
      emit dataTableUpdated();
    }
    else
    {
      QMessageBox::warning(owner(),"MantidPlot - Warning",QString("Workspace \"%1\" doesn't exist.").arg(wsName));
    }
  }
}

/// Add a spectrum from a workspace to the table.
/// @param wsName :: Name of a workspace.
/// @param wsIndex :: Index of a spectrum in the workspace (workspace index).
/// @param ws :: The workspace.
void DataController::addWorkspaceSpectrum(const QString &wsName, int wsIndex, const Mantid::API::MatrixWorkspace& ws)
{
  int row = m_dataTable->rowCount();
  m_dataTable->insertRow(row);

  auto cell = new QTableWidgetItem( wsName );
  m_dataTable->setItem( row, wsColumn, cell );
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);

  cell = new QTableWidgetItem( QString::number(wsIndex) );
  m_dataTable->setItem( row, wsIndexColumn, cell );
  flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);

  const double startX = ws.readX(wsIndex).front();
  cell = new QTableWidgetItem( makeNumber(startX) );
  m_dataTable->setItem( row, startXColumn, cell );

  const double endX = ws.readX(wsIndex).back();
  cell = new QTableWidgetItem( makeNumber(endX) );
  m_dataTable->setItem( row, endXColumn, cell );
}

/// Slot. Called when selection in the data table changes.
void DataController::workspaceSelectionChanged()
{
  auto selection = m_dataTable->selectionModel();
  bool enableRemoveButton = selection->hasSelection();
  if ( enableRemoveButton )
  {
    enableRemoveButton = selection->selectedRows().size() > 0;
  }

  emit hasSelection(enableRemoveButton);
}

/// Slot. Called when "Remove" button is pressed.
void DataController::removeSelectedSpectra()
{
  auto ranges = m_dataTable->selectedRanges();
  if ( ranges.isEmpty() ) return;
  QList<int> rows;
  for(auto range = ranges.begin(); range != ranges.end(); ++range)
  {
    for(int row = range->topRow(); row <= range->bottomRow(); ++row)
    {
      rows.push_back( row );
    }
  }
  removeSpectra( rows );
}

/// Remove some spectra from fitting.
/// @param rows :: A list of indices of the spacetra to remove.
void DataController::removeSpectra( QList<int> rows )
{
  if ( rows.isEmpty() ) return;
  qSort(rows);
  for(int i = rows.size() - 1; i >= 0; --i)
  {
    m_dataTable->removeRow( rows[i] );
  }
  emit spectraRemoved(rows);
  emit dataTableUpdated();
}

/// Check that the data sets in the table are valid and remove invalid ones.
void DataController::checkSpectra()
{
  QList<int> rows;
  int nrows = getNumberOfSpectra();
  auto& ADS = Mantid::API::AnalysisDataService::Instance();
  for( int row = 0; row < nrows; ++row)
  {
    auto wsName = getWorkspaceName( row );
    auto i = getWorkspaceIndex( row );
    if ( !ADS.doesExist( wsName ) )
    {
      rows.push_back( row );
      continue;
    }
    auto ws = ADS.retrieveWS<Mantid::API::MatrixWorkspace>( wsName );
    if ( !ws || i >= static_cast<int>( ws->getNumberHistograms() ) )
    {
      rows.push_back( row );
      continue;
    }
  }

  removeSpectra( rows );
}

/// Get the workspace name of the i-th spectrum.
/// @param i :: Index of a spectrum in the data table.
std::string DataController::getWorkspaceName(int i) const
{
  return m_dataTable->item(i, wsColumn)->text().toStdString();
}

/// Get the workspace index of the i-th spectrum.
/// @param i :: Index of a spectrum in the data table.
int DataController::getWorkspaceIndex(int i) const
{
  return m_dataTable->item(i, wsIndexColumn)->text().toInt();
}

/// Get the number of spectra to fit to.
int DataController::getNumberOfSpectra() const
{
  return m_dataTable->rowCount();
}

/// Enable global setting of fitting range (calls to setFittingRage(...)
/// will set ranges of all datasets.)
/// @param on :: True for global setting, false for individual.
void DataController::setFittingRangeGlobal(bool on)
{
  m_isFittingRangeGlobal = on;
}

/// Set the fitting range for a data set or all data sets.
/// @param i :: Index of a data set (spectrum). If m_isFittingRangeGlobal == true
///   the index is ignored and fitting range is set for all spectra.
/// @param startX :: Start of the fitting range.
/// @param endX :: End of the fitting range.
void DataController::setFittingRange(int i, double startX, double endX)
{
  if ( i < 0 || i >= m_dataTable->rowCount() ) return;
  auto start = makeNumber(startX);
  auto end = makeNumber(endX);
  if ( m_isFittingRangeGlobal )
  {
    for(int k = 0; k < getNumberOfSpectra(); ++k)
    {
      m_dataTable->item(k, startXColumn)->setText(start);
      m_dataTable->item(k, endXColumn)->setText(end);
    }
  }
  else
  {
    m_dataTable->item(i, startXColumn)->setText(start);
    m_dataTable->item(i, endXColumn)->setText(end);
  }
}

/// Get the fitting range for a i-th data set.
/// @param i :: Index of a dataset.
std::pair<double,double> DataController::getFittingRange(int i) const
{
  double startX = m_dataTable->item(i, startXColumn)->text().toDouble();
  double endX = m_dataTable->item(i, endXColumn)->text().toDouble();
  return std::make_pair(startX,endX);
}

/// Inform the others that a dataset was updated.
void DataController::updateDataset(int row, int)
{
  emit dataSetUpdated(row);
}

/// Object's parent cast to MultiDatasetFit.
MultiDatasetFit *DataController::owner() const {return static_cast<MultiDatasetFit*>(parent());}

} // MDF
} // CustomInterfaces
} // MantidQt
