//----------------------
// Includes
//----------------------
#include "MantidQtMantidWidgets/DiagResults.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/Exception.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QCloseEvent>
#include <boost/lexical_cast.hpp>

using namespace MantidQt::MantidWidgets;
DiagResults::TestSummary::TestSummary(QString name) :
  test(name), numBad(NORESULTS) {}
/** Reads the multi-line string created by the print statments in python scripts
*  @param pythonOut string as returned by runPythonCode()
*  @return either data (or if numBad is set to NORESULTS then diagnositic output, or possibly nothing at all)
*/
QString DiagResults::TestSummary::pythonResults(const QString &pyhtonOut)
{// set the number found bad to the not ready state incase there is an error below
  numBad = NORESULTS;

  QStringList results = pyhtonOut.split("\n");

  if ( results.count() < 2 )
  {// there was an error in the python, disregard these results
    status = "", outputWS = "", inputWS = "";
    return "Error \"" + pyhtonOut + "\" found, while executing scripts, there may be more details in the Mantid or python log.";
  }
  // each script should return 7 strings
  if ( results.count() != 7 || results[0] != "success" )
  {// there was an error in the python, disregard these results
    status = "", outputWS = "", inputWS = "";
    return "Error \"" + results[1] + "\" during " + results[2] + ", more details may be found in the Mantid and python logs.";
  }
  if ( test != results[1] )
  {//??STEVES, does this make sense, add to DeOxyGen?
    throw std::invalid_argument("Logic error:"+test.toStdString()+" is not "+results[1].toStdString());
  }
  
  //no errors, return the results
  std::string theBad = results[4].toStdString();
  
  status = results[2];
  outputWS = results[3];
  numBad = boost::lexical_cast<int>(theBad);
  inputWS = results[5];
  return "";
}

/// the total number of tests that results are reported for here
const int DiagResults::numTests = 3;
/// the list of tests that we display results for
const QString DiagResults::tests[DiagResults::numTests] =
  { "First white beam test", "Second white beam test", "Background test" };

//----------------------
// Public member functions
//----------------------
///Constructor
DiagResults::DiagResults(QWidget *parent): m_Grid(new QGridLayout),
  m_ListMapper(new QSignalMapper(this)), m_ViewMapper(new QSignalMapper(this))
{
  setWindowTitle("Failed detectors list");

  connect(m_ListMapper, SIGNAL(mapped(const QString &)), this, SLOT(tableList(const QString &)));
  connect(m_ViewMapper, SIGNAL(mapped(const QString &)), this, SLOT(instruView(const QString &)));
  
  // fill in the first row of controls and displays
  addRow("Step", "Bad detectors found");
    // make one row of buttons for each set of results
  int row = 0;
  for ( int i = 0; i < numTests; i ++ )
  {
    row = addRow(QString(tests[i]) + QString(" not done"), QString("     "));
    addButtonsDisab(row);
  }

  row ++;
  QPushButton *close = new QPushButton("Close");
  m_Grid->addWidget(close, row, 1);
  connect(close, SIGNAL(clicked()), this, SLOT(close()));

  setLayout(m_Grid);

  setAttribute(Qt::WA_DeleteOnClose);
}

void DiagResults::notifyDialog(const TestSummary &display)
{
  // store with the test the location of the data, outputWS could be an empty string
  outputWorkS[display.test] = display.outputWS;

  for ( int i = 0; i < numTests; i ++ )
  {
    if ( QString(tests[i]) == display.test )
    {
      int row = i + 1 + 1;
      updateRow(row, display.status, display.numBad);
      if ( display.numBad != TestSummary::NORESULTS )
      {
        showButtons(row, display.test);
      }
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

/// insert a row at the bottom of the grid
void DiagResults::updateRow(int row, QString firstColumn,
                                      int secondColumn)
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
  QPushButton *list = new QPushButton("List");
  list->setToolTip("List the detector IDs of the detectors found bad");
  m_Grid->addWidget(list, row, 2);
  list->setEnabled(false);

  QPushButton *view = new QPushButton("View");
  view->setToolTip("Show the locations of the bad detectors");
  m_Grid->addWidget(view, row, 3);
  view->setEnabled(false);
}

/// insert a row at the bottom of the grid
void DiagResults::showButtons(int row, QString test)
{
  QPushButton *bpList =
    dynamic_cast<QPushButton*>(m_Grid->itemAtPosition(row, 2)->widget());
  if ( ! bpList )
  {// this shouldn't happen but it appears we can't find the button so don't show it
    return;
  }
  bpList->setEnabled(true);
  connect(bpList, SIGNAL(clicked()), m_ListMapper, SLOT(map()));
  m_ListMapper->setMapping(bpList, test );
  
  QPushButton *bpView =
    dynamic_cast<QPushButton*>(m_Grid->itemAtPosition(row, 3)->widget());
  if ( ! bpView )
  {// no button lets leave
    return;
  }
  bpView->setEnabled(true);
  connect(bpView, SIGNAL(clicked()), m_ViewMapper, SLOT(map()));
  m_ViewMapper->setMapping(bpView, test);
}

/// enables the run button on the parent window so the user can do more analysis
void DiagResults::closeEvent(QCloseEvent *event)
{
  // remove all tempary workspaces
  std::map<QString, QString>::const_iterator it = outputWorkS.begin();
  // the last workspace is retained as the output workspace because all bad spectra are marked in this one
  // change the next line if you'd  like, I couldn't get it != it.end()-1 to work but this is equavilent as keys should be unique
  for ( ; it->first != outputWorkS.rbegin()->first; ++it )
  {
    std::string toDel = it->second.toStdString();
    Mantid::API::FrameworkManager::Instance().deleteWorkspace(toDel);
  }
  emit releaseParentWindow();
  event->accept();
}

void DiagResults::tableList(const QString &name)
{
  QString workspace = outputWorkS[name];
  QString tempOutp = QString("_FindBadDe") + workspace + QString("_temp");

  QString viewTablePy = "import BadDetectorTestFunctions as functions\n";
  viewTablePy.append("if functions.workspaceExists :\n");
  viewTablePy.append(
    "  bad = FindDetectorsOutsideLimits(InputWorkspace='"+workspace+"', OutputWorkspace='"+tempOutp+"', HighThreshold=10, LowThreshold=-1 )\n");
  viewTablePy.append("  mantid.deleteWorkspace('"+tempOutp+"')\n");
  viewTablePy.append("  stBad = bad.getPropertyValue('BadDetectorIDs')\n");
  viewTablePy.append("  liBad = stBad.split(',')\n");
  viewTablePy.append("else : liBad = ['The analysis data has been removed, run the detector efficiency tests again']\n");
  viewTablePy.append("tbBad = newTable('Failed Detector IDs -" + name + "', len(liBad), 1)\n");
//  viewTablePy.append("tbBad.setC)\n");
  viewTablePy.append("for i in range(0, len(liBad) ) :\n");
  viewTablePy.append("  tbBad.setText( 1, i+1, liBad[i] )\n");
  viewTablePy.append("tbBad.show()");

  emit runAsPythonScript(viewTablePy);
}

void DiagResults::instruView(const QString &name)
{
  QString workspace = outputWorkS[name];
  QString startInstruViewPy =
    "instrument_view = getInstrumentView(\"" +workspace+ "\")\n";
  // the colour map range should match the values set in the output workspace by the algorithms like FindDetectorsOutsideLimits
  startInstruViewPy.append("instrument_view.setWindowTitle('Failed detectors are marked 100 -" + name + "')\n");
  startInstruViewPy.append("instrument_view.setColorMapRange(0.,100.)\n");
  startInstruViewPy.append("instrument_view.showWindow()\n");
  emit runAsPythonScript(startInstruViewPy);
}
