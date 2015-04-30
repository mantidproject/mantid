//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidQtCustomDialogs/SortTableWorkspaceDialog.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"


using namespace MantidQt::API;

namespace MantidQt
{
namespace CustomDialogs
{

// Declare the dialog. Name must match the class name
DECLARE_DIALOG(SortTableWorkspaceDialog)


/// Default constructor
SortTableWorkspaceDialog::SortTableWorkspaceDialog(QWidget *parent) 
  : API::AlgorithmDialog(parent), m_form()
{
}

/// Initialize the layout
void SortTableWorkspaceDialog::initLayout()
{
  // set up the GUI elements
  m_form.setupUi(this);

  // add the Run/Cancel buttons
  m_form.dialogLayout->addLayout(this->createDefaultButtonLayout());

  // correct the tab order
  QWidget::setTabOrder( m_form.groupBox, m_form.cbColumnName );
  QWidget::setTabOrder( m_form.cbColumnName, m_form.cbAscending );

  // disable Add/Remove buttons in case there are no table workspaces at all
  m_form.btnRemoveColumn->setEnabled(false);
  m_form.btnAddColumn->setEnabled(false);

  // connect the slots
  connect( m_form.workspace, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(workspaceChanged(const QString&)) );
  connect( m_form.workspace, SIGNAL(emptied()), this, SLOT(clearGUI()) );
  connect( m_form.cbColumnName, SIGNAL(currentIndexChanged(int)), this, SLOT(changedColumnName(int)) );
  connect( m_form.btnAddColumn, SIGNAL(clicked()), this, SLOT(addColumn()) );
  connect( m_form.btnRemoveColumn, SIGNAL(clicked()), this, SLOT(removeColumn()) );

  tieStaticWidgets(true);
}

/**
 * Parse input
 */
void SortTableWorkspaceDialog::parseInput()
{
  QStringList columns;
  QStringList ascending;

  // extract column names to sort by from the controls, sort order too
  auto n = m_sortColumns.size();
  for(int i = 0; i < n; ++i)
  {
    auto itemColumn = m_form.columnsLayout->itemAtPosition(i, 1);
    auto itemAscending = m_form.columnsLayout->itemAtPosition(i, 2);
    if ( !itemColumn || !itemColumn->widget() || !itemAscending || !itemAscending->widget() )
    {
      throw std::logic_error("Logic error in SortTableWorkspaceDialog: internal inconsistency.");
    }
    
    auto name = dynamic_cast<QComboBox*>(itemColumn->widget())->currentText();
    auto ia = dynamic_cast<QComboBox*>(itemAscending->widget())->currentIndex();
    columns << name;
    ascending << QString::number( ia == 0 ? 1 : 0 );
  }

  // pass the properties to the algorithm
  storePropertyValue( "Columns", columns.join(",") );
  storePropertyValue( "Ascending", ascending.join(",") );
}

/**
* Tie static widgets to their properties
* @param readHistory :: If true then the history will be re read.
*/
void SortTableWorkspaceDialog::tieStaticWidgets(const bool)
{
  QStringList allowedTypes;
  allowedTypes << "TableWorkspace";
  m_form.workspace->setWorkspaceTypes(allowedTypes);
  tie( m_form.workspace, "InputWorkspace" );
  tie( m_form.output, "OutputWorkspace" );
  if ( !m_form.workspace->currentText().isEmpty() )
  {
    // start with output == input
    m_form.output->setText( m_form.workspace->currentText() );
  }
}

/**
 * Call to update the interface when the input workspace changes.
 * @param wsName :: Name of the new input workspace.
 */
void SortTableWorkspaceDialog::workspaceChanged(const QString& wsName)
{
  // start with output == input
  m_form.output->setText( wsName );
  // prepare the controls for new values
  clearGUI();
  if ( wsName.isEmpty() ) return;
  try
  {
    auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::ITableWorkspace>( wsName.toStdString() );
    if ( !ws ) return;
    m_columnNames.clear();
    // get and cache the column names from the workspace
    auto columnNames = ws->getColumnNames();
    for(auto name = columnNames.begin(); name != columnNames.end(); ++name)
    {
      m_columnNames << QString::fromStdString(*name);
    }
    m_form.cbColumnName->addItems( m_columnNames );
    // the GUI already has the controls to set the first column
    // if there are no other columns in the table no more names can be added
    if ( m_columnNames.size() <= 1 )
    {
      m_form.btnAddColumn->setEnabled(false);
    }
    else
    {
      m_form.btnAddColumn->setEnabled(true);
    }
    // cache the selected column name
    m_sortColumns[0] = m_form.cbColumnName->currentText();
  }
  catch(Mantid::Kernel::Exception::NotFoundError&)
  {
    return;
  }
}

/**
 * Clear the UI
 */
void SortTableWorkspaceDialog::clearGUI()
{
  m_columnNames.clear();
  m_form.lblColumnName->setText( "Column" );
  m_form.cbColumnName->clear();
  m_form.cbAscending->setCurrentIndex(0);
  m_form.btnAddColumn->setEnabled(false);
  m_form.btnRemoveColumn->setEnabled(false);
  // remove controls for any additional columns
  auto nRows = m_form.columnsLayout->rowCount();
  for(auto row = nRows - 1; row > 0; --row)
  {
    for(int col = 0; col < 3; ++col)
    {
      auto item = m_form.columnsLayout->itemAtPosition( row, col );
      if ( item )
      {
        auto index = m_form.columnsLayout->indexOf( item->widget() );
        m_form.columnsLayout->takeAt( index );
        item->widget()->deleteLater();
      }
    }
  }
  // leave a space for one column name
  // size of m_sortColumns is used to get the number of selected columns
  m_sortColumns.clear();
  m_sortColumns << "";
}

/**
 * Add elements to set additional column name and sort order
 */
void SortTableWorkspaceDialog::addColumn()
{
  m_form.lblColumnName->setText( "Column 1" );
  // create controls for setting new column
  auto newRow = m_sortColumns.size();
  assert( newRow <= m_columnNames.size() );
  QLabel *label = new QLabel( QString("Column %1").arg(newRow+1) );
  QComboBox *columnName = new QComboBox();
  columnName->addItems( m_columnNames );
  columnName->setToolTip( m_form.cbColumnName->toolTip() );
  connect( columnName, SIGNAL(currentIndexChanged(int)), this, SLOT(changedColumnName(int)) );
  QComboBox *ascending = new QComboBox();
  ascending->addItem("Ascending");
  ascending->addItem("Descending");
  ascending->setToolTip( m_form.cbAscending->toolTip() );
  // add them to the layout
  m_form.columnsLayout->addWidget(label,newRow,0);
  m_form.columnsLayout->addWidget(columnName,newRow,1);
  m_form.columnsLayout->addWidget(ascending,newRow,2);
  // correct the tab order
  QWidget::setTabOrder( m_form.columnsLayout->itemAtPosition( newRow - 1, 2 )->widget(), columnName );
  QWidget::setTabOrder( columnName, ascending );

  // suggest a name for the new column: one that hasn't been used in 
  // other sort columns
  QString newColumnName;
  foreach(QString name, m_columnNames)
  {
    if ( !m_sortColumns.contains(name) )
    {
      columnName->setCurrentText( name );
      break;
    }
  }
  // cache the column name
  m_sortColumns << columnName->currentText();
  // set the Add/Remove buttons into a correct state
  if ( m_sortColumns.size() == m_columnNames.size() )
  {
    m_form.btnAddColumn->setEnabled(false);
  }
  m_form.btnRemoveColumn->setEnabled(true);
}

/**
 * Sync column names in the combo-boxes and m_sortColumns.
 */
void SortTableWorkspaceDialog::changedColumnName(int)
{
  // don't try to figure out which column changed - just reset all cached names
  auto n = m_sortColumns.size();
  for(int i = 0; i < n; ++i)
  {
    auto item = m_form.columnsLayout->itemAtPosition(i, 1);
    if ( !item || !item->widget() || !dynamic_cast<QComboBox*>(item->widget()) )
    {
      throw std::logic_error("Logic error in SortTableWorkspaceDialog: internal inconsistency.");
    }
    
    auto name = dynamic_cast<QComboBox*>(item->widget())->currentText();
    m_sortColumns[i] = name;
  }
}

/**
 * Remove GUI elements for additional column to sort by.
 */
void SortTableWorkspaceDialog::removeColumn()
{
  assert( m_columnNames.size() > 1 );
  // remove the last column
  m_sortColumns.removeLast();
  auto row = m_sortColumns.size();
  for(int col = 0; col < 3; ++col)
  {
    auto item = m_form.columnsLayout->itemAtPosition( row, col );
    if ( item )
    {
      auto index = m_form.columnsLayout->indexOf( item->widget() );
      m_form.columnsLayout->takeAt( index );
      item->widget()->deleteLater();
    }
  }
  // leave the Add/Remove buttons in a correct state
  if ( m_sortColumns.size() == 1 )
  {
    m_form.btnRemoveColumn->setEnabled(false);
    m_form.lblColumnName->setText( "Column" );
  }
  m_form.btnAddColumn->setEnabled(true);
}

} // CustomDialogs
} // MantidQt
