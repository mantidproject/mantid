#include "MantidQtCustomInterfaces/MultiDatasetFit/MDFEditLocalParameterDialog.h"
#include "MantidQtCustomInterfaces/MultiDatasetFit/MultiDatasetFit.h"
#include "MantidQtCustomInterfaces/MultiDatasetFit/MDFLocalParameterItemDelegate.h"

#include <QMenu>
#include <QClipboard>

namespace{
  QString makeNumber(double d) {return QString::number(d,'g',16);}
  const int valueColumn = 0;
  const int roleColumn  = 1;
}

namespace MantidQt
{
namespace CustomInterfaces
{
namespace MDF
{

/// Constructor.
EditLocalParameterDialog::EditLocalParameterDialog(MultiDatasetFit *multifit, const QString &parName):
  QDialog(multifit),m_parName(parName)
{
  m_uiForm.setupUi(this);
  QHeaderView *header = m_uiForm.tableWidget->horizontalHeader();
  header->setResizeMode(0,QHeaderView::Stretch);
  connect(m_uiForm.tableWidget,SIGNAL(cellChanged(int,int)),this,SLOT(valueChanged(int,int)));
  m_uiForm.lblParameterName->setText("Parameter: " + parName);

  auto n = multifit->getNumberOfSpectra();
  for(int i = 0; i < n; ++i)
  {
    double value = multifit->getLocalParameterValue(parName,i);
    m_values.push_back(value);
    bool fixed = multifit->isLocalParameterFixed(parName,i);
    m_fixes.push_back(fixed);
    auto tie = multifit->getLocalParameterTie(parName,i);
    m_ties.push_back(tie);
    m_uiForm.tableWidget->insertRow(i);
    auto cell = new QTableWidgetItem( makeNumber(value) );
    m_uiForm.tableWidget->setItem(i, valueColumn, cell);
    auto headerItem = new QTableWidgetItem(
        multifit->getWorkspaceName(i) + " (" +
        QString::number(multifit->getWorkspaceIndex(i)) + ")");
    m_uiForm.tableWidget->setVerticalHeaderItem(i, headerItem);
    cell = new QTableWidgetItem("");
    auto flags = cell->flags();
    flags ^= Qt::ItemIsEditable;
    flags ^= Qt::ItemIsSelectable;
    flags ^= Qt::ItemIsEnabled;
    cell->setFlags(flags);
    m_uiForm.tableWidget->setItem(i, roleColumn, cell);
    updateRoleColumn(i);
  }
  auto deleg = new LocalParameterItemDelegate(this);
  m_uiForm.tableWidget->setItemDelegateForColumn(valueColumn, deleg);
  connect(deleg,SIGNAL(setAllValues(double)),this,SLOT(setAllValues(double)));
  connect(deleg,SIGNAL(fixParameter(int,bool)),this,SLOT(fixParameter(int,bool)));
  connect(deleg,SIGNAL(setAllFixed(bool)),this,SLOT(setAllFixed(bool)));
  connect(deleg,SIGNAL(setTie(int,QString)),this,SLOT(setTie(int,QString)));
  connect(deleg,SIGNAL(setTieAll(QString)),this,SLOT(setTieAll(QString)));

  m_uiForm.tableWidget->installEventFilter(this);
}

/// Slot. Called when a value changes.
/// @param row :: Row index of the changed cell.
/// @param col :: Column index of the changed cell.
void EditLocalParameterDialog::valueChanged(int row, int col)
{
  if ( col == valueColumn )
  {
    QString text = m_uiForm.tableWidget->item(row,col)->text();
    try
    {
      bool ok = false;
      double value = text.toDouble(&ok);
      if (ok) {
        m_values[row] = value;
      } else {
        m_ties[row] = text;
      }
    }
    catch(std::exception&)
    {
      // restore old value
      m_uiForm.tableWidget->item(row,col)->setText( makeNumber(m_values[row]) );
    }
  }
}

/// Set all parameters to the same value.
/// @param value :: A new value.
void EditLocalParameterDialog::setAllValues(double value)
{
  int n = m_values.size();
  for(int i = 0; i < n; ++i)
  {
    m_values[i] = value;
    m_uiForm.tableWidget->item(i, valueColumn)->setText( makeNumber(value) );
    updateRoleColumn(i);
  }
}

/// Get the list of new parameter values.
QList<double> EditLocalParameterDialog::getValues() const
{
  return m_values;
}

/// Get a list with the "fixed" attribute.
QList<bool> EditLocalParameterDialog::getFixes() const
{
  return m_fixes;
}

/// Get a list of the ties.
QStringList EditLocalParameterDialog::getTies() const
{
  return m_ties;
}

/// Fix/unfix a single parameter.
/// @param index :: Index of a paramter to fix or unfix.
/// @param fix :: Fix (true) or unfix (false).
void EditLocalParameterDialog::fixParameter(int index, bool fix)
{
  m_fixes[index] = fix;
  m_ties[index] = "";
  updateRoleColumn(index);
}

/// Set a new tie for a parameter
/// @param index :: Index of a paramter to tie.
/// @param tie :: A tie string.
void EditLocalParameterDialog::setTie(int index, QString tie)
{
  m_ties[index] = tie;
  m_fixes[index] = false;
  updateRoleColumn(index);
}

/// Set the same tie to all parameters.
/// @param tie :: A tie string.
void EditLocalParameterDialog::setTieAll(QString tie)
{
  for(int i = 0; i < m_ties.size(); ++i)
  {
    m_ties[i] = tie;
    m_fixes[i] = false;
    updateRoleColumn(i);
  }
  redrawCells();
}

/// Fix/unfix all parameters.
/// @param fix :: Fix (true) or unfix (false).
void EditLocalParameterDialog::setAllFixed(bool fix)
{
  if ( m_fixes.empty() ) return;
  for(int i = 0; i < m_fixes.size(); ++i)
  {
    m_fixes[i] = fix;
    m_ties[i] = "";
    updateRoleColumn(i);
  }
  redrawCells();
}

/// Event filter for managing the context menu.
bool EditLocalParameterDialog::eventFilter(QObject * obj, QEvent * ev)
{
  if ( obj == m_uiForm.tableWidget && ev->type() == QEvent::ContextMenu )
  {
    showContextMenu();
  }
  return QDialog::eventFilter(obj,ev);
}

/// Show the context menu.
void EditLocalParameterDialog::showContextMenu()
{
  auto selection = m_uiForm.tableWidget->selectionModel()->selectedColumns();

  bool hasSelection = false;

  for(auto index = selection.begin(); index != selection.end(); ++index)
  {
    if ( index->column() == valueColumn ) hasSelection = true;
  }

  if ( !hasSelection ) return;

  QMenu *menu = new QMenu(this);
  {
    QAction *action = new QAction("Copy",this);
    action->setToolTip("Copy data to clipboard.");
    connect(action,SIGNAL(activated()),this,SLOT(copy()));
    menu->addAction(action);
  }
  {
    QAction *action = new QAction("Paste",this);
    action->setToolTip("Paste data from clipboard.");
    connect(action,SIGNAL(activated()),this,SLOT(paste()));
    auto text = QApplication::clipboard()->text();
    action->setEnabled(!text.isEmpty());
    menu->addAction(action);
  }

  menu->exec(QCursor::pos());
}

/// Copy all parameter values to the clipboard.
/// Values will be separated by '\n'
void EditLocalParameterDialog::copy()
{
  QStringList text;
  auto n = m_values.size();
  for(int i = 0; i < n; ++i)
  {
    text << makeNumber(m_values[i]);
  }
  QApplication::clipboard()->setText( text.join("\n") );
}

/// Paste a list of values from the clipboard.
void EditLocalParameterDialog::paste()
{
  auto text = QApplication::clipboard()->text();
  auto vec = text.split(QRegExp("\\s|,"),QString::SkipEmptyParts);
  auto n = qMin(vec.size(), m_uiForm.tableWidget->rowCount());
  for(int i = 0; i < n; ++i)
  {
    auto str = vec[i];
    bool ok;
    m_values[i] = str.toDouble(&ok);
    if ( !ok ) str = "0";
    m_uiForm.tableWidget->item(i,1)->setText( str );
  }
}

/// Force the table to redraw its cells.
void EditLocalParameterDialog::redrawCells()
{
  for(int i = 0; i < m_values.size(); ++i)
  {
    // it's the only way I am able to make the table to repaint itself
    auto text = makeNumber(m_values[i]);
    m_uiForm.tableWidget->item(i, valueColumn)->setText( text + " " );
    m_uiForm.tableWidget->item(i, valueColumn)->setText( text );
  }
}

/// Update the text in the role column
void EditLocalParameterDialog::updateRoleColumn(int index)
{
  auto cell = m_uiForm.tableWidget->item(index, roleColumn);
  if (m_fixes[index])
  {
    cell->setText("fixed");
    cell->setForeground(QBrush(Qt::red));
  }
  else if (!m_ties[index].isEmpty())
  {
    cell->setText("tied");
    cell->setForeground(QBrush(Qt::blue));
  }
  else
  {
    cell->setText("fitted");
    cell->setForeground(QBrush(Qt::darkGreen));
  }
}

/// Check if there are any other fixed parameters
bool EditLocalParameterDialog::areOthersFixed(int i) const
{
  for (int j = 0; j < m_fixes.size(); ++j)
  {
    if (j != i && m_fixes[j]) return true;
  }
  return false;
}

/// Check if all other parameters are fixed
bool EditLocalParameterDialog::areAllOthersFixed(int i) const
{
  for (int j = 0; j < m_fixes.size(); ++j)
  {
    if (j != i && !m_fixes[j]) return false;
  }
  return true;
}

/// Check if there are any other tied parameters
bool EditLocalParameterDialog::areOthersTied(int i) const
{
  for (int j = 0; j < m_fixes.size(); ++j)
  {
    if (j != i && !m_ties[j].isEmpty()) return true;
  }
  return false;
}

} // MDF
} // CustomInterfaces
} // MantidQt
