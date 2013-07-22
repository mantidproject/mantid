//----------------------
// Includes
//----------------------

#include "MantidQtCustomInterfaces/CreateMDWorkspace.h"
#include "MantidQtCustomInterfaces/WorkspaceMemento.h"
#include "MantidQtCustomInterfaces/WorkspaceInADS.h"
#include "MantidQtCustomInterfaces/RawFileMemento.h"
#include "MantidQtCustomInterfaces/EventNexusFileMemento.h"
#include "MantidQtCustomInterfaces/WorkspaceMemento.h"
#include "MantidQtCustomInterfaces/CreateMDWorkspaceAlgDialog.h"

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AnalysisDataService.h"


// Suppress a warning coming out of code that isn't ours
#if defined(__INTEL_COMPILER)
  #pragma warning disable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic push
  #endif
  #pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
#include "qttreepropertybrowser.h"
#include "qtpropertymanager.h"
#include "qteditorfactory.h"
#include "DoubleEditorFactory.h"
#if defined(__INTEL_COMPILER)
  #pragma warning enable 1125
#elif defined(__GNUC__)
  #if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6 )
    #pragma GCC diagnostic pop
  #endif
#endif

#include <QtCheckBoxFactory>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <QMessageBox>
#include <QFileDialog>
#include <QRadioButton>
#include <QCheckBox>
#include <QDesktopServices>
#include <QUrl>
#include <sstream>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

using namespace Mantid::API;

namespace MantidQt
{
namespace CustomInterfaces
{

//Add this class to the list of specialised dialogs in this namespace
DECLARE_SUBWINDOW(CreateMDWorkspace); 

  /**
  Helper type to perform comparisons between WorkspaceMementos
  */
  class IdComparitor : public std::unary_function<WorkspaceMemento_sptr,bool>
  {
  private:
    WorkspaceMemento_sptr m_benchmark;
  public:
    IdComparitor(WorkspaceMemento_sptr benchmark) : m_benchmark(benchmark){}
    bool operator()(WorkspaceMemento_sptr a) const
    {
      std::vector<std::string> strs;
      std::string id =  m_benchmark->getId();
      boost::split(strs, id, boost::is_any_of("/,\\"));

      std::stringstream streamPattern;
      streamPattern << "(" << strs.back() << ")$";
      boost::regex pattern(streamPattern.str(), boost::regex_constants::icase); 

      return boost::regex_search(a->getId(), pattern);

    }
  };

/*
Constructor taking a WorkspaceMementoCollection, which acts as the model.
*/
CreateMDWorkspace::CreateMDWorkspace(QWidget *) 
{
  //Generate memento view model.
  m_model = new QtWorkspaceMementoModel(m_data);
}

/*
Initalize the layout.
*/
void CreateMDWorkspace::initLayout()
{
  m_uiForm.setupUi(this);

  std::string location;
  if(!Mantid::Kernel::ConfigService::Instance().getValue("defaultsave.directory", location))
  {
    location = Mantid::Kernel::ConfigService::Instance().getTempDir().c_str();
  }
  m_uiForm.txt_location->setText(location.c_str());
  m_uiForm.txt_location->setEnabled(false);

  connect(m_uiForm.ck_merge, SIGNAL(clicked(bool)), this, SLOT(mergeClicked(bool)));
  connect(m_uiForm.btn_create, SIGNAL(clicked()), this, SLOT(createMDWorkspaceClicked()));
  connect(m_uiForm.btn_add_workspace, SIGNAL(clicked()), this, SLOT(addWorkspaceClicked()));
  connect(m_uiForm.btn_add_nexus_file, SIGNAL(clicked()), this, SLOT(addNexusFileClicked()));
  connect(m_uiForm.btn_add_event_nexus_file, SIGNAL(clicked()), this, SLOT(addEventNexusFileClicked()));
  connect(m_uiForm.btn_remove_workspace, SIGNAL(clicked()), this, SLOT(removeSelectedClicked()));
  connect(m_uiForm.btn_set_ub_matrix, SIGNAL(clicked()), this, SLOT(setUBMatrixClicked()));
  connect(m_uiForm.btn_find_ub_matrix, SIGNAL(clicked()), this, SLOT(findUBMatrixClicked()));
  connect(m_uiForm.btn_set_goniometer, SIGNAL(clicked()), this, SLOT(setGoniometerClicked()));
  connect(m_uiForm.btn_add_logs, SIGNAL(clicked()), this, SLOT(setLogValueClicked()));
  connect(m_uiForm.btn_help, SIGNAL(clicked()), this, SLOT(helpClicked()));
  connect(m_uiForm.btn_set_location, SIGNAL(clicked()), this, SLOT(setLocationClicked()));
  
  //Set MVC Model
  m_uiForm.tableView->setModel(m_model);
  this->setWindowTitle("Create MD Workspaces");
  m_uiForm.txt_merged_workspace_name->setEnabled(m_uiForm.ck_merge->isChecked());
  m_uiForm.lbl_merged_name->setEnabled(m_uiForm.ck_merge->isChecked());

  //Add existing ADS names into selector.
  m_uiForm.workspaceSelector->clear();
  typedef std::set<std::string> NameSet;
  NameSet names = AnalysisDataService::Instance().getObjectNames();
  NameSet::iterator it = names.begin();
  while(it != names.end())
  {
    m_uiForm.workspaceSelector->addItem((*it).c_str());
    ++it;
  }
  
}

/*
Event handler for finding the UBMatrix as part of a peak finding action.
*/
void CreateMDWorkspace::findUBMatrixClicked()
{
  QString command, args, result;
  QStringList bMatrixArgs;
  try
  {
    ScopedMemento memento(getFirstSelected());

    memento->fetchIt(Everything); 

    // Find the peaks workspace in detector space
    command = "from mantid.simpleapi import *\n"
      "try:\n"
      "    FindSXPeaksDialog(InputWorkspace='%1', OutputWorkspace='%1_peaks')\n"
      "    print 'SUCCESS'\n"
      "except:\n"
      "    print 'FAIL'\n"
      "    raise\n";
    args = QString(memento->getId().c_str());
    command = command.arg(args);
    result = runPythonCode(command).trimmed();
    if(result == "FAIL")
    {
      runConfirmation("Aborted during PeakFinding.");
      return;
    }

    // Calculate the u matrix and then copy the ub matrix result from the peaksworkspace to the matrix workspace
    command = "try:\n"
      "    alg = CalculateUMatrixDialog(PeaksWorkspace='%1_peaks')\n"
      "    a = alg.getProperty('a').value\n"
      "    b = alg.getProperty('b').value\n"
      "    c = alg.getProperty('c').value\n"
      "    alpha = alg.getProperty('alpha').value\n"
      "    beta = alg.getProperty('beta').value\n"
      "    gamma = alg.getProperty('gamma').value\n"
      "    CopySample(InputWorkspace='%1_peaks',OutputWorkspace='%1',CopyName='0',CopyMaterial='0',CopyEnvironment='0',CopyShape='0',CopyLattice='1')\n"
      "    print '%(a)s, %(b)s, %(c)s, %(alpha)s, %(beta)s, %(gamma)s' % {'a': a, 'b' : b, 'c' : c, 'alpha' : alpha, 'beta' : beta, 'gamma' : gamma}\n"
      "except:\n"
      "    print 'FAIL'\n"
      "    raise\n";

    command = command.arg(args);
    result = runPythonCode(command).trimmed();
    if(result == "FAIL")
    {
      runConfirmation("Aborted during calculating and copying the UB matrix.");
      return;
    }
    else
    {
      bMatrixArgs = result.split(',');
    }

    // Index the peaks on the workspace
    if(m_uiForm.ckIndexSXPeaks->isChecked())
    {
      command = "IndexSXPeaksDialog(PeaksWorkspace='%1_peaks', a='%2', b='%3', c='%4', alpha='%5', beta='%6', gamma='%7')";
      args = args + "," + result;
    }
    else
    {
      command = "IndexPeaksDialog(PeaksWorkspace='%1_peaks')";
    }

    command = command.arg(args);
    // Run peak indexing
    runPythonCode(command).trimmed();
  }
  catch(std::invalid_argument& ex)
  {
    runConfirmation(ex.what());
  }
}


/// Event handler for add log event.
void CreateMDWorkspace::setLogValueClicked()
{
  try
  {
    ScopedMemento memento(getFirstSelected());
    Mantid::API::MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>( memento->fetchIt(MinimalData) );
    QString id = QString(memento->getId().c_str());

    QString pyInput =
      "from mantid.simpleapi import *\n"
      "try:\n"
      "    wsName='%1'\n"
      "    alg = AddSampleLogDialog(Workspace=wsName)\n"
      "    logValue = alg.getProperty('LogText').value\n"
      "    logName = alg.getProperty('LogName').value\n"
      "    logType = alg.getProperty('LogType').value\n"
      "    print '%(LogName)s, %(LogValue)s, %(LogType)s' % {'LogName': logName, 'LogValue' : logValue, 'LogType' : logType}\n"
      "except:\n"
      "    print 'FAIL'\n"
      "    raise\n";

    pyInput = pyInput.arg(id);
    QString pyOutput = runPythonCode(pyInput).trimmed();
    QStringList logArgs = pyOutput.split(',');
    if (logArgs.size() == 3 )
    { 
      std::string name = logArgs[0].trimmed().toStdString();
      std::string value = logArgs[1].trimmed().toStdString();
      std::string logType = logArgs[2].trimmed().toStdString();
      memento->setLogValue(name, value, logType);
    }
    else
    {
      throw std::runtime_error("Could not set the log value!");
    }
  }
  catch(std::invalid_argument& ex)
  {
    runConfirmation(ex.what());
  }
}

/// Event handler for selecting merging.
void CreateMDWorkspace::mergeClicked(bool)
{
  bool isEnabled = m_uiForm.ck_merge->isChecked();
  m_uiForm.txt_merged_workspace_name->setEnabled(isEnabled);
  m_uiForm.lbl_merged_name->setEnabled(isEnabled);
}

/*
Getter for the first selected memento.
@return the first selected memento
@throw invalid argument if nothing is selected
*/
WorkspaceMemento_sptr CreateMDWorkspace::getFirstSelected()
{
  QTableView* view = m_uiForm.tableView;
  QModelIndexList indexes = view->selectionModel()->selection().indexes();
  if(indexes.size() > 0)
  {
    int index = indexes.front().row();
    return m_data[index];
  }
  else
  {
    throw std::invalid_argument("Nothing selected");
  }
}

/*
Event handler for setting the UB Matrix
*/
void CreateMDWorkspace::setUBMatrixClicked()
{
  
  try
  {
    ScopedMemento memento(getFirstSelected());
    Mantid::API::MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>( memento->fetchIt(MinimalData) );
    QString id = QString(memento->getId().c_str());

    QString pyInput =
      "from mantid.simpleapi import *\n"
      "wsName='%1'\n"
      "SetUBDialog(Workspace=wsName)\n"
      "msg = 'ws is: ' + wsName\n"
      "logger.notice(msg)\n"
      "ws = mtd[wsName]\n"
      "lattice = ws.sample().getOrientedLattice()\n"
      "ub = lattice.getUB()\n"
      "print '%(u00)d, %(u01)d, %(u02)d, %(u10)d, %(u11)d, %(u12)d, %(u20)d, %(u21)d, %(u22)d' "
      "% {'u00': ub[0][0], 'u01' : ub[0][1], 'u02' : ub[0][2], 'u10': ub[1][0], 'u11' : ub[1][1], 'u12' : ub[1][2], 'u20' : ub[2][0], 'u21' : ub[2][1], 'u22' : ub[2][2]}\n";

    pyInput = pyInput.arg(id);
    QString pyOutput = runPythonCode(pyInput).trimmed();

    QStringList ub = pyOutput.split(',');
    if (ub.size() == 9 )
    {
      memento->setUB(ub[0].toDouble(), ub[1].toDouble(), ub[2].toDouble(), ub[3].toDouble(), ub[4].toDouble(), ub[5].toDouble(), ub[6].toDouble(),  ub[7].toDouble(),  ub[8].toDouble());
      m_model->update();
    }
    else
    {
      throw std::runtime_error("Could not set the log value!");
    }
  }
  catch(std::invalid_argument& ex)
  {
    runConfirmation(ex.what());
  }
}

/*
Add a workspace from the ADS
*/
void CreateMDWorkspace::addWorkspaceClicked()
{
  std::string name = m_uiForm.workspaceSelector->currentText().toStdString();
  if(!name.empty())
  {
    WorkspaceMemento_sptr candidate(new WorkspaceInADS(name));
    addUniqueMemento(candidate);
  }
}

/**
Adds a memento to the existing data, if it's id has not already been used in the data list.
@param candidate : candidate memento to add.
*/
void CreateMDWorkspace::addUniqueMemento(WorkspaceMemento_sptr candidate)
{
  IdComparitor comparitor(candidate);
  WorkspaceMementoCollection::iterator pos = std::find_if(m_data.begin(), m_data.end(), comparitor);
  if(pos == m_data.end())
  {
    m_data.push_back(candidate);
    m_model->update();
  }
  else
  {
    std::string msg = "Already have a workspace by that name loaded. Cannot add another.";
    runConfirmation(msg);
  }
}

/*
Select and return files with a given file extension.
*/
QStringList CreateMDWorkspace::findFiles(const std::string fileType) const
{
  QFileDialog dialog;
  dialog.setDirectory(QDir::homePath());
  dialog.setFileMode(QFileDialog::ExistingFiles);
  dialog.setNameFilter(QString(fileType.c_str()));
  QStringList fileNames;
  if (dialog.exec())
  {
    fileNames = dialog.selectedFiles();
  }
  return fileNames;
}

/*
Add a nexus files on disk
*/
void CreateMDWorkspace::addNexusFileClicked()
{
  QStringList fileNames = findFiles("Nexus files (*.nxs)");

  QStringList::iterator it = fileNames.begin();
  QStringList::const_iterator end = fileNames.end();
  while(it != end)
  {
    std::string name = (*it).toStdString();
    if(!name.empty())
    {
      try
      {
        WorkspaceMemento_sptr candidate(new RawFileMemento(name));
        addUniqueMemento(candidate);
      }
      catch(std::invalid_argument& arg)
      {
        this->runConfirmation(arg.what());
      }
    }
    ++it;
  }
}

/*
Add an Event nexus files on disk
*/
void CreateMDWorkspace::addEventNexusFileClicked()
{
  QStringList fileNames = findFiles("Event Nexus files (*.nxs)");

  QStringList::iterator it = fileNames.begin();
  QStringList::const_iterator end = fileNames.end();
  while(it != end)
  {
    std::string name = (*it).toStdString();
    if(!name.empty())
    {
      try
      {
        WorkspaceMemento_sptr candidate(new EventNexusFileMemento(name));
        addUniqueMemento(candidate);
      }
      catch(std::invalid_argument& arg)
      {
        this->runConfirmation(arg.what());
      }
    }
    ++it;
  }
}

/**
Handler for setting the goniometer.
*/
void CreateMDWorkspace::setGoniometerClicked()
{
  try
  {
    ScopedMemento memento(getFirstSelected());
    Mantid::API::MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>( memento->fetchIt(MinimalData) );
    QString id = QString(memento->getId().c_str());

    QString pyInput =
      "from mantid.simpleapi import *\n"
      "import sys\n"
      "try:\n"
      "    wsName='%1'\n"
      "    alg = SetGoniometerDialog(Workspace=wsName)\n"
      "    axis0 = alg.getProperty('Axis0').value\n"
      "    axis1 = alg.getProperty('Axis1').value\n"
      "    axis2 = alg.getProperty('Axis2').value\n"
      "    axis3 = alg.getProperty('Axis3').value\n"
      "    axis4 = alg.getProperty('Axis4').value\n"
      "    axis5 = alg.getProperty('Axis5').value\n"
      "    print '%(Axis0)s; %(Axis1)s; %(Axis2)s; %(Axis3)s; %(Axis4)s; %(Axis5)s' % {'Axis0': axis0, 'Axis1' : axis1, 'Axis2' : axis2, 'Axis3' : axis3, 'Axis4' : axis4, 'Axis5' : axis5}\n"
      "except:\n"
      "    print 'FAIL'\n"
      "    raise\n";

    pyInput = pyInput.arg(id);
    QString pyOutput = runPythonCode(pyInput).trimmed();
    QStringList axis = pyOutput.split(';');
    if (axis.size() == 6 )
    { 
      std::string axis0 = axis[0].trimmed().toStdString();
      std::string axis1 = axis[1].trimmed().toStdString();
      std::string axis2 = axis[2].trimmed().toStdString();
      std::string axis3 = axis[3].trimmed().toStdString();
      std::string axis4 = axis[4].trimmed().toStdString();
      std::string axis5 = axis[5].trimmed().toStdString();

      memento->setGoniometer(axis0, axis1, axis2, axis3, axis4, axis5);
    }
  }
  catch(std::invalid_argument& ex)
  {
    runConfirmation(ex.what());
  }
}


/*
Remove any selected workspace mementos
*/
void CreateMDWorkspace::removeSelectedClicked()
{
  QTableView* view = m_uiForm.tableView;
  QModelIndexList indexes = view->selectionModel()->selection().indexes();

  //Copy the original collection
  WorkspaceMementoCollection temp(m_data.size());
  std::copy(m_data.begin(), m_data.end(), temp.begin());

  for (int i = 0; i < indexes.count(); i++)
  {
    QModelIndex index = indexes.at(i);
    int row = index.row();
    //Copy and swap trick.
    WorkspaceMemento_sptr dead = m_data[row];
    WorkspaceMementoCollection::iterator pos = std::find(temp.begin(), temp.end(), dead);
    if(pos != temp.end())
    {
      temp.erase(pos);
    }
  }
  //Assign using the copied/trimmed temp collection.
  m_data = temp;
  //Update the model.
  m_model->update();
}

void CreateMDWorkspace::initLocalPython()
{
}

/// Handler for setting the output location
void CreateMDWorkspace::setLocationClicked()
{
  QString temp = QFileDialog::getExistingDirectory(this, tr("Open Directory"), "/home");
  if(!temp.isEmpty())
  {
    m_uiForm.txt_location->setText(temp);
  }
}

/*
Run a generic confirmation dialog.
@param message : The message to display.
*/
int CreateMDWorkspace::runConfirmation(const std::string& message)
{
  QMessageBox msgBox;
  msgBox.setText(message.c_str());
  msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
  msgBox.setDefaultButton(QMessageBox::Cancel);
  return msgBox.exec();
}

void CreateMDWorkspace::createMDWorkspaceClicked()
{
  QString mergedWorkspaceName = m_uiForm.txt_merged_workspace_name->text().trimmed();
  
  //Output location.
  QString location = m_uiForm.txt_location->text();

  if(m_data.size() == 0)
  {
    runConfirmation("Nothing to process. Add workspaces first.");
    return;
  }
  if(m_uiForm.ck_merge->isChecked() && mergedWorkspaceName.isEmpty())
  {
    runConfirmation("A name must be provided for the merged workspace");
    return;
  }

  CreateMDWorkspaceAlgDialog algDlg;
  algDlg.setModal(true);
  int result = algDlg.exec();
  if(result != QDialog::Accepted)
  {
    runConfirmation("Aborted by user.");
    return;
  }
  
  QString qDimension = algDlg.getQDimension();
  QString maxExtents = algDlg.getMaxExtents();
  QString minExtents = algDlg.getMinExtents();
  QString analysisMode = algDlg.getAnalysisMode();
  QString otherDimensions = algDlg.getOtherDimensions();
  QString preProcessedDetectors = algDlg.getPreprocessedDetectors();
  if (preProcessedDetectors.toInt()==0) // no you do not need to preprocess detectors
  {
    preProcessedDetectors = QString("-");
  }
  else  // one wants to preprocess detectors.
  {
     preProcessedDetectors = QString("_PreprocDetectors");
  }
  
  //2) Run ConvertToMDEvents on each workspace.
  QString fileNames;
  for(WorkspaceMementoCollection::size_type i = 0; i < m_data.size(); i++)
  {
    ScopedMemento currentMemento(m_data[i]);
    Workspace_sptr ws = currentMemento->applyActions();
    QString command;

    bool keepWorkspaceInADS = m_uiForm.ck_keep->isChecked();
    if(keepWorkspaceInADS)
    {
      command = "try:\n"
        "    ConvertToMD(InputWorkspace='%1',OutputWorkspace='%1_md',MaxRecursionDepth=1,MinRecursionDepth=1,OtherDimensions='%2',dEAnalysisMode='%3',QDimensions='%4',MinValues='%5',MaxValues='%6',PreprocDetectorsWS='%7')\n"
        "    SaveMD(InputWorkspace='%1_md', Filename=r'%8/%1_md.nxs',MakeFileBacked='1')\n"
        "except:\n"
        "    print 'FAIL'\n"
        "    raise\n";
    }
    else
    {
      command = "try:\n"
        "    ConvertToMD(InputWorkspace='%1',OutputWorkspace='%1_md',MaxRecursionDepth=1,MinRecursionDepth=1,OtherDimensions='%2',dEAnalysisMode='%3',QDimensions='%4',MinValues='%5',MaxValues='%6',PreprocDetectorsWS='%7')\n"
        "    SaveMD(InputWorkspace='%1_md', Filename=r'%8/%1_md.nxs',MakeFileBacked='1')\n"
        "    DeleteWorkspace(Workspace='%1_md')\n"
        "except:\n"
        "    print 'FAIL'\n"
        "    raise\n";
    }

    QString id(currentMemento->getId().c_str());
    command = command.arg(id, otherDimensions, analysisMode, qDimension, minExtents, maxExtents, preProcessedDetectors, location);
    QString pyOutput = runPythonCode(command).trimmed();
    if(pyOutput == "FAIL")
    {
      runConfirmation("Aborted during conversion. See log messages.");
      return; //Abort.
    }

    if(i != 0)
    {
      fileNames += ",";
    }
    fileNames += location + "/" + id + "_md.nxs";
    
    currentMemento->cleanUp();
  }

  //3) Run Merge algorithm (if required)
  if(m_uiForm.ck_merge->isChecked())
  {
    QString command = "try:"
      "    MergeMDFiles(Filenames=r'%1',OutputFilename=r'%2/%3.nxs',OutputWorkspace='%3')\n"
      "except:\n"
      "    print 'FAIL'\n"
      "    raise\n";

    command = command.arg(fileNames, location, mergedWorkspaceName);
    QString pyOutput = runPythonCode(command).trimmed();
    if(pyOutput == "FAIL")
    {
      runConfirmation("Aborted during merge. See log messages.");
    }
  }
  //Report successful conversion.
  std::string msg = "Success. Ouput MD files have been written to : " + location.toStdString();
  runConfirmation(msg);
}

/**
 * Event handler for the help request. Launches browser to obtain help.
 */
void CreateMDWorkspace::helpClicked()
{
    QDesktopServices::openUrl(QUrl(QString("http://www.mantidproject.org/") +
				   "Create_MD_Workspace_GUI"));
}


/// Destructor
CreateMDWorkspace::~CreateMDWorkspace()
{
}

} //namespace CustomInterfaces
} //namespace MantidQt
