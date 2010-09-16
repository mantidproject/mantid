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

#include <boost/lexical_cast.hpp>

using namespace MantidQt::API;
using namespace MantidQt::MantidWidgets;

DiagResults::TestSummary::TestSummary(QString name) :
  test(name), status("Error"), numBad(NORESULTS) {}
/** Reads the multi-line string created by the print statments in python scripts
*  @param pythonOut string as returned by runPythonCode()
*  @return either data (or if numBad is set to NORESULTS then diagnositic output, or possibly nothing at all)
*  @throw invalid_argument if output from the wrong test is passed to this function
*/
QString DiagResults::TestSummary::pythonResults(const QString &pythonOut)
{// set the number found bad to the not ready state incase there is an error below
  numBad = NORESULTS;

  QStringList results = pythonOut.split("\n");

  if ( results.count() < 2 )
  {// there was an error in the python, disregard these results
    status = "Error", outputWS = "", inputWS = "", listBad = "";
    return "Error \"" + pythonOut + "\" found, while executing scripts, there may be more details in the Mantid or python log.";
  }
  if ( results[0] != "Created the workspaces:" )
  {// there was an error in the python, disregard these results
    status = "Error", outputWS = "", inputWS = "", listBad = "";
    return results[0] + " '"+results[1]+"' " + results[2] + ". Diagnostic information may be found in the Mantid and python logs.";
  }
  if ( ! results[3].contains(test) )
  {
    throw std::invalid_argument("Logic error: test '"+test.toStdString()+"' was excuted out of order");
  }
  
  //no errors, return the results
  std::string theBad = results[4].toStdString();
  
  status = "success";
  outputWS = results[1];
  numBad = boost::lexical_cast<int>(theBad);
  inputWS = results[2];
  listBad = results[5];
  return "";
}

/// the total number of tests that results are reported for here
const int DiagResults::NUMTESTS = 3;
/// the list of tests that we display results for
const QString DiagResults::TESTS[DiagResults::NUMTESTS] =
  { "First white beam test", "Second white beam test", "Background test" };

//----------------------
// Public member functions
//----------------------
///Constructor
DiagResults::DiagResults(QWidget *parent): MantidQtDialog(parent),
  m_Grid(new QGridLayout), m_ListMapper(new QSignalMapper(this)),
  m_ViewMapper(new QSignalMapper(this))
{
  setWindowTitle("Failed detectors list");

  connect(m_ListMapper, SIGNAL(mapped(const QString &)), this, SLOT(tableList(const QString &)));
  connect(m_ViewMapper, SIGNAL(mapped(const QString &)), this, SLOT(instruView(const QString &)));
  
  // fill in the first row of controls and displays
  addRow("Step", "Bad detectors found");
    // make one row of buttons for each set of results
  int row = 0;
  for ( int i = 0; i < NUMTESTS; i ++ )
  {
    row = addRow(QString(TESTS[i]) + QString(" not done"), QString("     "));
    addButtonsDisab(row);
  }

  row ++;
  QPushButton *close = new QPushButton("Close");
  m_Grid->addWidget(close, row, 1);
  connect(close, SIGNAL(clicked()), this, SLOT(close()));

  setLayout(m_Grid);

  setAttribute(Qt::WA_DeleteOnClose);
}

void DiagResults::addResults(const TestSummary &display)
{
  // store with the test the location of the data, outputWS could be an empty string
  m_outputWorkS.insert(display.test,display.outputWS);

  for ( int i = 0; i < NUMTESTS; i ++ )
  {
    if ( QString(TESTS[i]) == display.test )
    {
      int row = i + 1 + 1;
      updateRow(row, display.status, display.numBad);
      if ( display.numBad != TestSummary::NORESULTS )
      {
        setupButtons(row, display.test);
      }
    }
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
*  @param firstColumn the text that should be displayed in the first column
*  @param secondColumn the text that should be displayed in the second column
*/
void DiagResults::updateRow(int row, QString firstColumn, int secondColumn)
{
  QWidget *oldLabel = m_Grid->itemAtPosition(row, 0)->widget();
  if ( !oldLabel ) return;
  oldLabel->hide();
  m_Grid->removeWidget( oldLabel );
  delete oldLabel;
  m_Grid->addWidget(new QLabel(firstColumn), row, 0);

  oldLabel = m_Grid->itemAtPosition(row, 1)->widget();
  if ( !oldLabel ) return;
  oldLabel->hide();
  m_Grid->removeWidget( oldLabel );
  delete oldLabel;
  QString secondLabel;
  if ( secondColumn != TestSummary::NORESULTS )
  {
    secondLabel = QString("   ") + QString::number(secondColumn);
  }
  else secondLabel = "";
  m_Grid->addWidget(new QLabel(secondLabel), row, 1);
}
/// insert a row at the bottom of the grid
void DiagResults::addButtonsDisab(int row)
{
  QPushButton *list = new QPushButton("");
  list->setToolTip("List the detector IDs of the detectors found bad");
  m_Grid->addWidget(list, row, 2);
  list->setEnabled(false);

  QPushButton *view = new QPushButton("");
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
  //// remove all tempary workspaces
  QHashIterator<QString,QString> itr(m_outputWorkS);
  while(itr.hasNext() )
  {
    QString remove = itr.next().value();
    Mantid::API::FrameworkManager::Instance().deleteWorkspace(remove.toStdString());
  }
  emit died();
  event->accept();
}

void DiagResults::tableList(const QString &name)
{
  QString workspace = m_outputWorkS[name];
  QString tempOutp = QString("_FindBadDe") + workspace + QString("_temp");
  QString temp_key = name + "_temp";
  m_outputWorkS.insert(temp_key,tempOutp);

  QString viewTablePy = "import DetectorTestLib as functions\n";
  viewTablePy.append("if mtd.workspaceExists('" + workspace + "'):\n");
  viewTablePy.append(
    "  bad = FindDetectorsOutsideLimits(InputWorkspace='"+workspace+"', OutputWorkspace='"+tempOutp+"', HighThreshold=10, LowThreshold=-1 )\n");
  viewTablePy.append("  stBad = bad.getPropertyValue('BadSpectraNums')\n");
  viewTablePy.append("  liBad = stBad.split(',')\n");
  viewTablePy.append("else : liBad = ['The analysis data has been removed, run the detector efficiency tests again']\n");
  viewTablePy.append("tbBad = newTable('Failed Detector IDs -" + name + "', len(liBad), 1)\n");
  viewTablePy.append("for i in range(0, len(liBad) ) :\n");
  viewTablePy.append("  tbBad.setText( 1, i+1, liBad[i] )\n");
  viewTablePy.append("tbBad.show()");

  emit runAsPythonScript(viewTablePy);
}

void DiagResults::instruView(const QString &name)
{
  QString workspace = m_outputWorkS[name];
  QString startInstruViewPy =
    "instrument_view = getInstrumentView(\"" +workspace+ "\")\n";
  // the colour map range should match the values set in the output workspace by the algorithms like FindDetectorsOutsideLimits
  startInstruViewPy.append("instrument_view.setWindowTitle('Failed detectors are marked 100 -" + name + "')\n");
  startInstruViewPy.append("instrument_view.setColorMapRange(0.,100.)\n");
  startInstruViewPy.append("instrument_view.showWindow()\n");
  
  emit runAsPythonScript(startInstruViewPy);
}
