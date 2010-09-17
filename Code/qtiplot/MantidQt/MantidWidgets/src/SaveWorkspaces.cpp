//----------------------
// Includes
//----------------------
#include "MantidQtMantidWidgets/SaveWorkspaces.h"
#include "MantidQtAPI/FileDialogHandler.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/FileProperty.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QDoubleValidator>
#include <QCloseEvent>
#include <QShowEvent>
#include <QGroupBox>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::Kernel;
using namespace Mantid::API;

//----------------------
// Public member functions
//----------------------
/** 
 *  @param parent used by QT
 *  @param suggFname sets the initial entry in the filename box
 *  @param defSavs sets which boxes are ticked
 */
SaveWorkspaces::SaveWorkspaces(QWidget *parent, const QString & suggFname, QHash<const QCheckBox * const, QString> & defSavs) :
  API::MantidQtDialog(parent)
{
  setWindowTitle("Save Workspaces");

  //the form is split into 3 lines of controls in horizontal layouts
  QHBoxLayout *lineOne = new QHBoxLayout;
  setupLine1(lineOne, suggFname);
  QHBoxLayout *lineTwo = new QHBoxLayout;
  setupLine2(lineTwo, defSavs);

  QVBoxLayout *dialogLayout = new QVBoxLayout;
  dialogLayout->addLayout(lineOne);
  dialogLayout->addLayout(lineTwo);
   
  setLayout(dialogLayout);
  setAttribute(Qt::WA_DeleteOnClose);
}
/// Set up the dialog layout
void SaveWorkspaces::initLayout()
{
}
/** Puts the controls that go on the first line, the output
*  filename commands, on to the layout that's passed to it
*  @param lineOne the layout on to which the controls will be placed
*/
void SaveWorkspaces::setupLine1(QHBoxLayout * const lineOne, const QString defName)
{
  QLabel* fNameLabel = new QLabel("Filename:");
  m_fNameEdit = new QLineEdit();
  m_fNameEdit->setText(defName);
  QPushButton *fNameButton = new QPushButton("Browse");
  connect(fNameButton, SIGNAL(clicked()), this, SLOT(saveFileBrowse()));

  lineOne->addWidget(fNameLabel);
  lineOne->addWidget(m_fNameEdit);
  lineOne->addWidget(fNameButton);

  fNameLabel->setToolTip("Filename to save under");
  m_fNameEdit->setToolTip("Filename to save under");
  fNameButton->setToolTip("Filename to save under");
}
/** Puts the controls that go on the second line, the workspace
*  list and save commands, on to the layout that's passed to it
*  @param lineTwo the layout on to which the controls will be placed
*/
void SaveWorkspaces::setupLine2(QHBoxLayout * const lineTwo, const QHash<const QCheckBox * const, QString> & defSavs)
{
  m_workspaces = new QListWidget();
  std::set<std::string> ws = AnalysisDataService::Instance().getObjectNames();
  std::set<std::string>::const_iterator it = ws.begin(), wsEnd = ws.end();
  for( ; it != wsEnd; ++it)
  {
    Workspace *wksp =  FrameworkManager::Instance().getWorkspace(*it);
    if( dynamic_cast<MatrixWorkspace*>(wksp) )
    {//only include matrix workspaces, not groups or tables
      m_workspaces->addItem(QString::fromStdString(*it));
    }
  }
  //allow users to select more than one workspace in the list
  m_workspaces->setSelectionMode(QAbstractItemView::ExtendedSelection);

  QPushButton *save = new QPushButton("Save");
  connect(save, SIGNAL(clicked()), this, SLOT(saveSel()));
  QPushButton *cancel = new QPushButton("Cancel");
  connect(cancel, SIGNAL(clicked()), this, SLOT(close()));
  
  QCheckBox *saveNex = new QCheckBox("Nexus");
  QCheckBox *saveCan = new QCheckBox("CanSAS");
  QCheckBox *saveRKH = new QCheckBox("RKH");
  QCheckBox *saveCSV = new QCheckBox("CSV");
  //link the save option tick boxes to their save algorithm
  m_savFormats.insert(saveNex, "SaveNexus");
  m_savFormats.insert(saveCan, "SaveCanSAS1D");
  m_savFormats.insert(saveRKH, "SaveRKH");
  m_savFormats.insert(saveCSV, "SaveCSV");
  setupFormatTicks(defSavs);

  m_append = new QCheckBox("Append");
  
  // place controls into the layout, which places them on the form and takes care of deleting them
  QVBoxLayout *ly_saveConts = new QVBoxLayout;
  ly_saveConts->addWidget(save);
  ly_saveConts->addWidget(cancel);
  ly_saveConts->addWidget(m_append);
  ly_saveConts->addStretch();

  QVBoxLayout *ly_saveFormats = new QVBoxLayout;
  ly_saveFormats->addWidget(saveNex);
  ly_saveFormats->addWidget(saveCan);
  ly_saveFormats->addWidget(saveRKH);
  ly_saveFormats->addWidget(saveCSV);
  QGroupBox *gb_saveForms = new QGroupBox(tr("Save Formats"));
  gb_saveForms->setLayout(ly_saveFormats);
  ly_saveConts->addWidget(gb_saveForms);

  lineTwo->addWidget(m_workspaces);
  lineTwo->addLayout(ly_saveConts);

  m_workspaces->setToolTip("Select one or more workspaces");
  const QString formatsTip = "Some formats support appending multiple workspaces in one file";
  gb_saveForms->setToolTip(formatsTip);
  save->setToolTip(formatsTip);
  cancel->setToolTip(formatsTip);
  saveNex->setToolTip(formatsTip);
  saveCan->setToolTip(formatsTip);
  saveRKH->setToolTip(formatsTip);
  saveCSV->setToolTip(formatsTip);
  m_append->setToolTip(formatsTip);
}
/** For each save format tick box take the user setting from the
* main form
*/
void SaveWorkspaces::setupFormatTicks(const QHash<const QCheckBox * const, QString> & defSavs)
{
  for(SavFormatsConstIt i=m_savFormats.begin(); i != m_savFormats.end(); ++i)
  {
    //find the setting that has been passed for this save format
    QHash<const QCheckBox * const, QString>::const_iterator j = defSavs.begin();
    for ( ; j != defSavs.end(); ++j )
    {
      //the values are the algorithm names
      if ( j.value() == i.value() )
      {//copy over the checked status of the check box
        i.key()->setChecked(j.key()->isChecked());
      }
    }
  }
}
/**
 * Called in response to a close event
 * @parma event The event object
 */
void SaveWorkspaces::closeEvent(QCloseEvent* event)
{
  emit closing();
  event->accept();
}
QString SaveWorkspaces::saveList(const QList<QListWidgetItem*> & wspaces, const QString & algorithm, QString fileBase, bool toAppend)
{
  if ( wspaces.count() < 1 )
  {
    throw std::logic_error("");
  }

  if (toAppend && fileBase.isEmpty())
  {//no file name was given, use the name of the first workspace
    fileBase = wspaces[0]->text();
  }
  QString exten = getSaveAlgExt(algorithm);
  
  QString saveCommands;
  for (int j =0; j < wspaces.count(); ++j)
  {
    saveCommands += algorithm + "('"+wspaces[j]->text()+"','";

    QString outFile = fileBase;
    if (outFile.isEmpty())
    {//if no filename was given use the workspace names
      outFile = wspaces[j]->text();
    }
    else
    {//we have a file name
      if( ( wspaces.count() > 1 ) && ( ! toAppend ) )
      {//but multiple output files, number the files
        if (outFile.endsWith(exten))
        {//put the number before the extension
          outFile = outFile.split(exten)[0];
        }
        outFile += "-"+QString::number(j+1);
      }
    }
    
    if ( ! outFile.endsWith(exten) )
    {//code above sometimes removes the extension and the possiblity that one just wasn't added needs dealing with too
      outFile += exten;
    }
    saveCommands += outFile + "'";
    if (toAppend)
    {
      saveCommands += ", Append=True";
    }
    //finally finish the algorithm call
    saveCommands += ")\n";
  }
  return saveCommands;
}
/** Gets the first extension that the algorithm passed algorithm has in it's
*  FileProperty (the FileProperty must have the name "Filename"
*  @param algName name of the Mantid save algorithm
*  @return the first extension, if the algorithm's Filename property has an extension list or ""
*/
QString SaveWorkspaces::getSaveAlgExt(const QString & algName)
{
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create(
    algName.toStdString());
  Property *prop = alg->getProperty("Filename");
  FileProperty *fProp = dynamic_cast<FileProperty*>(prop);
  if (fProp)
  {
    return QString::fromStdString(fProp->getDefaultExt());
  }
  else
  {// the algorithm doesn't have a "Filename" file property which may indicate an error later on or maybe OK
    return "";
  }
}
/** Excutes the selected save algorithms on the workspaces that
*  have been selected to be saved
*/
void SaveWorkspaces::saveSel()
{
  QString saveCommands;
  for(SavFormatsConstIt i = m_savFormats.begin(); i != m_savFormats.end(); ++i)
  {//the key a pointer to the check box that the user may have clicked
    if (i.key()->isChecked())
    {//we need to save in this format
      
      bool toAppend = m_append->isChecked();
      if (toAppend)
      {//SaveCSV doesn't support appending
        if ( i.value() == "SaveCSV" )
        {
          toAppend = false;
        }
      }

      try
      {
        saveCommands += saveList(m_workspaces->selectedItems(), i.value(),
          m_fNameEdit->text(), toAppend);
      }
      catch(std::logic_error)
      {
        QMessageBox::information(this, "No workspace to save", "You must select at least one workspace to save");
        return;
      }
    }//end if save in this format
  }//end loop over formats
  emit runAsPythonScript(saveCommands);
}
/** Raises a browse dialog and inserts the selected file into the
*  save text edit box, outfile_edit
*/
void SaveWorkspaces::saveFileBrowse()
{
  QString title = "Save output workspace as";

  QSettings prevValues;
  prevValues.beginGroup("CustomInterfaces/SANSRunWindow/SaveWorkspaces");
  //use their previous directory first and go to their default if that fails
  QString prevPath = prevValues.value("dir", QString::fromStdString(
    ConfigService::Instance().getString("defaultsave.directory"))).toString();

  QString filter = ";;AllFiles (*.*)";
  QString oFile = API::FileDialogHandler::getSaveFileName(this, title, prevPath, filter);

  if( ! oFile.isEmpty() )
  {
    m_fNameEdit->setText(oFile);
    
    QString directory = QFileInfo(oFile).path();
    prevValues.setValue("dir", directory);
  }
}