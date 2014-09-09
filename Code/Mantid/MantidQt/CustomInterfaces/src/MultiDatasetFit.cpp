#include "MantidQtCustomInterfaces/MultiDatasetFit.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ArrayBoundedValidator.h"

#include <QDialog>
#include <QHeaderView>
#include <QMessageBox>

#include <boost/make_shared.hpp>

#include <vector>
#include <algorithm>

namespace MantidQt
{
namespace CustomInterfaces
{

/*==========================================================================================*/
/*                              AddWorkspaceDialog                                          */
/*==========================================================================================*/
AddWorkspaceDialog::AddWorkspaceDialog(QWidget *parent):QDialog(parent)
{
  m_uiForm.setupUi(this);
  // populate the combo box with names of eligible workspaces
  QStringList workspaceNames;
  auto wsNames = Mantid::API::AnalysisDataService::Instance().getObjectNames();
  for(auto name = wsNames.begin(); name != wsNames.end(); ++name)
  {
    auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>( *name );
    if ( ws )
    {
      workspaceNames << QString::fromStdString( *name );
    }
  }
  connect(m_uiForm.cbWorkspaceName,SIGNAL(currentIndexChanged(const QString&)),this,SLOT(workspaceNameChanged(const QString&)));
  m_uiForm.cbWorkspaceName->addItems( workspaceNames );

  connect(m_uiForm.cbAllSpectra,SIGNAL(stateChanged(int)),this,SLOT(selectAllSpectra(int)));
}

/**
 * Slot. Reacts on change of workspace name in the selection combo box.
 * @param wsName :: Name of newly selected workspace.
 */
void AddWorkspaceDialog::workspaceNameChanged(const QString& wsName)
{
  auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>( wsName.toStdString() );
  if ( ws )
  {
    int maxValue = static_cast<int>(ws->getNumberHistograms()) - 1;
    if ( maxValue < 0 ) maxValue = 0;
    m_maxIndex = maxValue;
    if ( m_uiForm.cbAllSpectra->isChecked() )
    {
      m_uiForm.leWSIndices->setText(QString("0-%1").arg(m_maxIndex));
    }
    else
    {
      m_uiForm.leWSIndices->clear();
    }
  }
  else
  {
    m_maxIndex = 0;
    m_uiForm.leWSIndices->clear();
    m_uiForm.cbAllSpectra->setChecked(false);
  }
}

/**
 * Slot. Called when "All Spectra" check box changes its state
 */
void AddWorkspaceDialog::selectAllSpectra(int state)
{
  if ( state == Qt::Checked )
  {
    m_uiForm.leWSIndices->setText(QString("0-%1").arg(m_maxIndex));
    m_uiForm.leWSIndices->setEnabled(false);
  }
  else
  {
    m_uiForm.leWSIndices->setEnabled(true);
  }

}

/**
 * Called on close if selection accepted.
 */
void AddWorkspaceDialog::accept()
{
  m_workspaceName = m_uiForm.cbWorkspaceName->currentText();
  m_wsIndices.clear();
  QString indexInput = m_uiForm.leWSIndices->text();
  if ( !indexInput.isEmpty() )
  {
    auto validator = boost::make_shared<Mantid::Kernel::ArrayBoundedValidator<int>>(0,m_maxIndex);
    Mantid::Kernel::ArrayProperty<int> prop("Indices",validator);
    std::string err = prop.setValue( indexInput.toStdString() );
    if ( err.empty() )
    {
      m_wsIndices = prop;
    }
    else
    {
      QMessageBox::warning(this, "MantidPlot - Error", QString("Some of the indices are outside the allowed range [0,%1]").arg(m_maxIndex));
    }
  }
  QDialog::accept();
}

/**
 * Called on close if selection rejected.
 */
void AddWorkspaceDialog::reject()
{
  m_workspaceName.clear();
  m_wsIndices.clear();
  QDialog::reject();
}

/*==========================================================================================*/
/*                                PlotController                                            */
/*==========================================================================================*/

PlotController::PlotController(QObject *parent,QwtPlot *plot, QTableWidget *table):
  QObject(parent),m_plot(plot),m_table(table)
{
}

PlotController::~PlotController()
{
  std::cerr << "Plot controller destroyed." << std::endl;
}

/*==========================================================================================*/
/*                                MultiDatasetFit                                           */
/*==========================================================================================*/

//Register the class with the factory
DECLARE_SUBWINDOW(MultiDatasetFit);

/**
 * Constructor
 * @param parent :: The parent widget
 */
MultiDatasetFit::MultiDatasetFit(QWidget *parent)
:UserSubWindow(parent)
{
}

/**
 * Initilize the layout.
 */
void MultiDatasetFit::initLayout()
{
  m_uiForm.setupUi(this);
  m_uiForm.hSplitter->setStretchFactor(0,0);
  m_uiForm.hSplitter->setStretchFactor(1,1);
  m_uiForm.vSplitter->setStretchFactor(0,0);
  m_uiForm.vSplitter->setStretchFactor(1,1);

  QHeaderView *header = m_uiForm.dataTable->horizontalHeader();
  header->setResizeMode(0,QHeaderView::Stretch);
  header->setResizeMode(1,QHeaderView::Fixed);

  m_uiForm.btnRemove->setEnabled( false );

  connect(m_uiForm.btnAddWorkspace,SIGNAL(clicked()),this,SLOT(addWorkspace()));
  connect(m_uiForm.btnRemove,SIGNAL(clicked()),this,SLOT(removeSelectedSpectra()));
  connect(m_uiForm.dataTable,SIGNAL(itemSelectionChanged()), this,SLOT(workspaceSelectionChanged()));

  m_plotController = new PlotController(this,m_uiForm.plot,m_uiForm.dataTable);
}

/**
 * Show a dialog to select a workspace.
 */
void MultiDatasetFit::addWorkspace()
{
  AddWorkspaceDialog dialog(this);
  if ( dialog.exec() == QDialog::Accepted )
  {
    QString wsName = dialog.workspaceName().stripWhiteSpace();
    if ( Mantid::API::AnalysisDataService::Instance().doesExist( wsName.toStdString()) )
    {
      auto indices = dialog.workspaceIndices();
      for(auto i = indices.begin(); i != indices.end(); ++i)
      {
        addWorkspaceSpectrum( wsName, *i );
      }
    }
    else
    {
      QMessageBox::warning(this,"MantidPlot - Warning",QString("Workspace \"%1\" doesn't exist.").arg(wsName));
    }
  }
}

/**
 * Add a spectrum from a workspace to the table.
 * @param wsName :: Name of a workspace.
 * @param wsIndex :: Index of a spectrum in the workspace (workspace index).
 */
void MultiDatasetFit::addWorkspaceSpectrum(const QString &wsName, int wsIndex)
{
  int row = m_uiForm.dataTable->rowCount();
  m_uiForm.dataTable->insertRow(row);

  auto cell = new QTableWidgetItem( wsName );
  m_uiForm.dataTable->setItem( row, 0, cell );
  cell = new QTableWidgetItem( QString::number(wsIndex) );
  m_uiForm.dataTable->setItem( row, 1, cell );
}

/**
 * Slot. Called when selection in the data table changes.
 */
void MultiDatasetFit::workspaceSelectionChanged()
{
  auto selection = m_uiForm.dataTable->selectionModel();
  bool enableRemoveButton = selection->hasSelection();
  if ( enableRemoveButton )
  {
    enableRemoveButton = selection->selectedRows().size() > 0;
  }

  m_uiForm.btnRemove->setEnabled( enableRemoveButton );
}

/**
 * Slot. Called when "Remove" button is pressed.
 */
void MultiDatasetFit::removeSelectedSpectra()
{
  auto ranges = m_uiForm.dataTable->selectedRanges();
  if ( ranges.isEmpty() ) return;
  std::vector<int> rows;
  for(auto range = ranges.begin(); range != ranges.end(); ++range)
  {
    for(int row = range->topRow(); row <= range->bottomRow(); ++row)
    {
      rows.push_back( row );
    }
  }
  std::sort( rows.begin(), rows.end() );
  for(auto row = rows.rbegin(); row != rows.rend(); ++row)
  {
    m_uiForm.dataTable->removeRow( *row );
  }
}

/*==========================================================================================*/
} // CustomInterfaces
} // MantidQt
