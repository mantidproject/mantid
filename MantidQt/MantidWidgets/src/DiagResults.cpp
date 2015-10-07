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
  const int NUMTESTS = 5;
  /// the list of tests that we display results for
  const QString TESTS[5] =
  { "Hard mask", "First detector vanadium test", "Second detector vanadium test", "Background test", "PSD Bleed test"};

  int find_test(const std::string &test_name){
    int found = -1;
    for(int i=0;i<5;i++){
      if (TESTS[i].toStdString()==test_name){
        found = i+1;
        return found;
      }
    }
    return found;
  }
}

//----------------------
// Public member functions
//----------------------
///Constructor
DiagResults::DiagResults(QWidget *parent): MantidDialog(parent),
  m_Grid(new QGridLayout)
{
  setWindowTitle("Failed detectors list");
  
  addRow("Test", "Number of failed spectra");
  // make one row for each set of results
  int row = 0;
  for ( int i = 0; i < NUMTESTS; i ++ )
  {
    QString col1 = TESTS[i];
    QString col2 = "N/A";
    row = addRow(col1, col2);
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
 * @param testSummary :: A string containing the test results
 */
void DiagResults::updateResults(const QString & testSummary)
{
  if( !testSummary.contains("Diagnostic Test Summary") )
  {
    throw std::runtime_error("Diagnostic results string does not have expected format.");
  }

  QStringList results = testSummary.split("\n");
  int nTestStrings = results.length();
  int end_count(0);
  // First result line is the header
  for(int i = 0; i <= nTestStrings; ++i)
  {
    QString testName = results[i].section(":", 0, 1);
    std::string tn = testName.toStdString();
    if (tn[0] == '='){
      end_count++;
      if (end_count>1)break;
      else            continue;
    }
    QStringList NameValPair = results[i].split(":");
    tn = NameValPair[0].toStdString();
    QStringList columns = NameValPair[1].split(QRegExp("\\s+"), QString::SkipEmptyParts);
    Q_ASSERT(columns.size() == 2);
    QString status;
    if( columns[0] == "None" ) status = "N/A";
    else status = columns[1];
    int test_ind = find_test(tn);
    if (test_ind<0)continue;
    updateRow(test_ind+1, status);
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
*  @param row :: the row where the data will be displayed
*  @param text :: the text that should be displayed in the first column
*/
void DiagResults::updateRow(int row, QString text)
{
  // Get the text label from the grid
  QWidget *widget = m_Grid->itemAtPosition(row, 1)->widget();
  QLabel *label = qobject_cast<QLabel*>(widget);
  label->setText(text);
}

/// enables the run button on the parent window so the user can do more analysis
void DiagResults::closeEvent(QCloseEvent *event)
{
  emit died();
  event->accept();
}
