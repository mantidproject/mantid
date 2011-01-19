//----------------------
// Includes
//----------------------
#include "MantidQtMantidWidgets/DiagResults.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QCloseEvent>
#include <QHashIterator>
#include <QRegExp>

#include <boost/lexical_cast.hpp>

using namespace MantidQt::API;
using namespace MantidQt::MantidWidgets;


namespace
{
  /// the total number of tests that results are reported for here
  const int NUMTESTS = 4;
  /// the list of tests that we display results for
  const QString TESTS[NUMTESTS] =
  { "First white beam test", "Second white beam test", "Background test", "PSD Bleed test"};
}

//----------------------
// Public member functions
//----------------------
///Constructor
DiagResults::DiagResults(QWidget *parent): MantidDialog(parent),
  m_Grid(new QGridLayout), m_ListMapper(new QSignalMapper(this)),
  m_ViewMapper(new QSignalMapper(this))
{
  setWindowTitle("Failed detectors list");

  connect(m_ListMapper, SIGNAL(mapped(int)), this, SLOT(tableList(int)));
  connect(m_ViewMapper, SIGNAL(mapped(int)), SLOT(instruView(int)));
  
  addRow("Test", "Number of failed spectra");
  // make one row of buttons for each set of results
  int row = 0;
  for ( int i = 0; i < NUMTESTS; i ++ )
  {
    QString col1 = TESTS[i];
    QString col2 = "N/A";
    row = addRow(col1, col2);
    addButtons(row);
  }
  row++;
  QPushButton *close = new QPushButton("Close");
  m_Grid->addWidget(close, row, 1);
  connect(close, SIGNAL(clicked()), this, SLOT(close()));

  setLayout(m_Grid);

  setAttribute(Qt::WA_DeleteOnClose, false);
}

/**
 * Update the results on the dialog
 * @param testSummary A string containing the test results
 */
void DiagResults::updateResults(const QString & testSummary)
{
  if( !testSummary.contains("Diagnostic Test Summary") )
  {
    throw std::runtime_error("Diagnostic results string does not have expected format.");
  }

  QStringList results = testSummary.split("\n");
  // First result line is the header
  for(int i = 1; i <= NUMTESTS; ++i)
  {
    QString testName = results[i].section(":", 0, 1);
    QString fieldValues = results[i].section(":", 1);
    QStringList columns = fieldValues.split(QRegExp("\\s+"), QString::SkipEmptyParts);
    Q_ASSERT(columns.size() == 2);
    QString status;
    if( columns[0] == "None" ) status = "N/A";
    else status = columns[1];
    // Store the name of the test workspace
    m_diagWS[i+1] = columns[0];
    updateRow(i+1, status);
  }
  
}

/** Enable or disable the buttons used to run Python scripts
*  @param show the buttons are enabled if this is set to true and disabled otherwise
*/
void DiagResults::showButtons(bool show)
{
  for( int i = 0; i < NUMTESTS; i ++ )
  {
    const int row = i + 1 + 1;
    QPushButton *bpList =
      qobject_cast<QPushButton*>(m_Grid->itemAtPosition(row, 2)->widget());

    if ( bpList && bpList->text() == "List" )
    {
      bpList->setEnabled(show);
    }
    QPushButton *bpView =
      qobject_cast<QPushButton*>(m_Grid->itemAtPosition(row, 3)->widget());
    if ( bpView && bpView->text() == "View" )
    {
      bpView->setEnabled(show);
    }
  }
}
//----------------------
// Private member functions
//----------------------
/// insert a row at the bottom of the grid
int DiagResults::addRow(QString firstColumn, QString secondColumn)
{
  // set row to one past the end of the number of rows that currently exist
  int row = m_Grid->rowCount();
  m_Grid->addWidget(new QLabel(firstColumn), row, 0);
  m_Grid->addWidget(new QLabel(secondColumn), row, 1);
  return row;
}

/** Displays a summary of the results of tests in to text labels
*  @param row the row where the data will be displayed
*  @param text the text that should be displayed in the first column
*/
void DiagResults::updateRow(int row, QString text)
{
  // Get the text label from the grid
  QWidget *widget = m_Grid->itemAtPosition(row, 1)->widget();
  QLabel *label = qobject_cast<QLabel*>(widget);
  label->setText(text);
  // Update the buttons depending on the status of text
  QPushButton *listButton = qobject_cast<QPushButton*>(m_Grid->itemAtPosition(row, 2)->widget());
  QPushButton *viewButton = qobject_cast<QPushButton*>(m_Grid->itemAtPosition(row, 3)->widget());
  if(text == "N/A")
  {
    listButton->setEnabled(false);
    viewButton->setEnabled(false);
  }
  else
  {
    listButton->setEnabled(true);
    viewButton->setEnabled(true);
  }

}

/// insert a row at the bottom of the grid
void DiagResults::addButtons(int row)
{
  QPushButton *list = new QPushButton("List");
  connect(list, SIGNAL(clicked()), m_ListMapper, SLOT(map()));
  m_ListMapper->setMapping(list, row);
  list->setToolTip("List the detector IDs of the detectors found bad");
  m_Grid->addWidget(list, row, 2);
  list->setEnabled(false);

  QPushButton *view = new QPushButton("View");
  connect(view, SIGNAL(clicked()), m_ViewMapper, SLOT(map()));
  m_ViewMapper->setMapping(view, row);
  view->setToolTip("Show the locations of the bad detectors");
  m_Grid->addWidget(view, row, 3);
  view->setEnabled(false);
}

/** Enable the controls on the row and conect the buttons to the slots
*  from which their Python script is executed
*  @param row the index number of the row to enable
*  @param test the name of the test that row corrosponds to
*/
void DiagResults::setupButtons(int row, QString test)
{
  QPushButton *bpList =
    qobject_cast<QPushButton*>(m_Grid->itemAtPosition(row, 2)->widget());
  if ( bpList && ( bpList->text() != "List" ) )
  {
    bpList->setText("List");
    connect(bpList, SIGNAL(clicked()), m_ListMapper, SLOT(map()));
    m_ListMapper->setMapping(bpList, test );
  }

  QPushButton *bpView =
    qobject_cast<QPushButton*>(m_Grid->itemAtPosition(row, 3)->widget());
  if ( bpView && ( bpView->text() != "View" ) )
  {
    bpView->setText("View");
    connect(bpView, SIGNAL(clicked()), m_ViewMapper, SLOT(map()));
    m_ViewMapper->setMapping(bpView, test);
  }
}

/// enables the run button on the parent window so the user can do more analysis
void DiagResults::closeEvent(QCloseEvent *event)
{
  // Remove all tempary workspaces
  QHashIterator<int,QString> itr(m_diagWS);
  while(itr.hasNext() )
  {
    QString remove = itr.next().value();
    if( remove != "None" ) 
    {
      Mantid::API::FrameworkManager::Instance().deleteWorkspace(remove.toStdString());
    }
  }
  emit died();
  event->accept();
}

void DiagResults::tableList(int row)
{
  QString wsName = m_diagWS[row];
  QString testName = TESTS[row - 2];
  QString pyCode = 
    "import diagnostics\n"
    "failed_spectra = diagnostics.get_failed_spectra_list('%1')\n"
    "num_failed = len(failed_spectra)\n"
    "failed_table = newTable('Failed Spectra - %2 ', num_failed, 1)\n"
    "for i in range(num_failed):\n"
    "    failed_table.setText(1, i+1, str(failed_spectra[i]))\n"
    "failed_table.show()";

  //Set the workspace name and testName
  pyCode = pyCode.arg(wsName,testName);
  runPythonCode(pyCode, true);
}

void DiagResults::instruView(int row)
{
  QString wsName = m_diagWS[row];
  QString pyCode = 
    "inst_view = getInstrumentView('%1')\n"
    "inst_view.setWindowTitle('Failed detectors')\n"
    "inst_view.setColorMapRange(0.0, 1.0)\n"
    "inst_view.show()";
  pyCode = pyCode.arg(wsName);
  runPythonCode(pyCode, true);
}
