#include "MantidQtCustomInterfaces/SANSAddFiles.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/AlgorithmManager.h"

#include <QStringList>
#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>
#include <QVariant>

#include <Poco/Path.h>

namespace MantidQt
{
namespace CustomInterfaces
{

using namespace MantidQt::CustomInterfaces;
using namespace Mantid::Kernel;
using namespace Mantid::API;

// Initialize the logger
Logger& SANSAddFiles::g_log = Logger::get("SANSAddFiles");

SANSAddFiles::SANSAddFiles(QWidget *parent, Ui::SANSRunWindow *ParWidgets) :
  m_SANSForm(ParWidgets), parForm(parent), m_pythonRunning(false)
{
  initLayout();
  
  //get lists of suported extentions
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("Load");
  Property *prop = alg->getProperty("Filename");
  m_exts = prop->allowedValues();
  //a log file must be copied across if it was a raw file, find out from the extention if a raw file was selected
  alg = AlgorithmManager::Instance().create("LoadRaw");
  prop = alg->getProperty("Filename");
  m_rawExts = prop->allowedValues();
}

SANSAddFiles::~SANSAddFiles()
{
  saveSettings();
}

//Connect signals and setup widgets
void SANSAddFiles::initLayout()
{
  connect(m_SANSForm->new2Add_edit, SIGNAL(returnPressed()), this,
    SLOT(add2Runs2Add()));
  
  //the runAsPythonScript() signal needs to get to Qtiplot, here it is connected to the parent, which is connected to Qtiplot
  connect(this, SIGNAL(runAsPythonScript(const QString&)),
          parForm, SIGNAL(runAsPythonScript(const QString&)));

  insertListFront("");

  connect(m_SANSForm->toAdd_List, SIGNAL(itemChanged(QListWidgetItem *)),
    this, SLOT(setCellData(QListWidgetItem *)));

  //buttons on the Add Runs tab
  connect(m_SANSForm->add_Btn, SIGNAL(clicked()), this, SLOT(add2Runs2Add()));
  connect(m_SANSForm->sum_Btn, SIGNAL(clicked()), this, SLOT(runPythonAddFiles()));
  connect(m_SANSForm->summedPath_Btn, SIGNAL(clicked()), this, SLOT(summedPathBrowse()));
  connect(m_SANSForm->browse_to_add_Btn, SIGNAL(clicked()), this, SLOT(new2AddBrowse()));
  connect(m_SANSForm->clear_Btn, SIGNAL(clicked()), this, SLOT(clearClicked()));
  connect(m_SANSForm->remove_Btn, SIGNAL(clicked()), this, SLOT(removeSelected()));

  readSettings();
}
/**
 * Restore previous input
 */
void SANSAddFiles::readSettings()
{
  QSettings value_store;
  value_store.beginGroup("CustomInterfaces/SANSRunWindow");
  std::string defOut =
    ConfigService::Instance().getString("defaultsave.directory");
  
  QString qDefOut = QString::fromStdString(defOut);
  //this string may be passed to python, so convert any '\' to '/' to make it compatible on all systems
  Poco::Path defOutComp = Poco::Path(defOut);
  if ( defOutComp.separator() == '\\' )
  {
    qDefOut.replace('\\', '/');
  }
  
  m_SANSForm->summedPath_edit->setText(
    value_store.value("AddRuns/OutPath", qDefOut).toString());

  value_store.endGroup();
}
/**
 * Save input for future use
 */
void SANSAddFiles::saveSettings()
{
  QSettings value_store;
  value_store.beginGroup("CustomInterfaces/SANSRunWindow");
  value_store.setValue("AddRuns/OutPath", m_SANSForm->summedPath_edit->text());
}
/** Creates a QListWidgetItem with the given text and inserts it
*  into the list box
*  @param[in] text the text to insert
*  @return a pointer to the inserted widget
*/
QListWidgetItem* SANSAddFiles::insertListFront(const QString &text)
{
  QListWidgetItem *newItem = new QListWidgetItem(text);
  newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);
  m_SANSForm->toAdd_List->insertItem(0, newItem);
  return newItem;
}
/**Moves the entry in the line edit new2Add_edit to the
*  listbox toAdd_List, expanding any run number lists
*/
void SANSAddFiles::add2Runs2Add()
{
  //split comma separated file names or run numbers into a list
  ArrayProperty<std::string> commaSep("unusedName",
    m_SANSForm->new2Add_edit->text().toStdString() );
  const std::vector<std::string> nam = commaSep;

  for(std::vector<std::string>::const_iterator i=nam.begin();i!=nam.end();++i)
  {//each comma separated item could be a range of run numbers specified with a ':' or '-' 
    QStringList ranges;
    std::vector<int> runNumRanges;
    try
    {//if the entry is in the form 454:456, runNumRanges will be filled with the integers ({454, 455, 456}) otherwise it will throw
      appendValue(*i, runNumRanges);
      std::vector<int>::const_iterator num = runNumRanges.begin();
      for( ; num != runNumRanges.end(); ++num)
      {
        ranges.append(QString::number(*num));
      }
    }
    catch(boost::bad_lexical_cast &)
    {//this means that we don't have a list of integers, treat it as full (and valid) filename
      ranges.append(QString::fromStdString(*i));
    }

    for(QStringList::const_iterator k = ranges.begin(); k != ranges.end(); ++k)
    {
      //Don't display the full file path in the box, it's too long
      QListWidgetItem *newL = insertListFront(QFileInfo(*k).fileName());
      newL->setData(Qt::WhatsThisRole, QVariant(*k));
      //Put the full path in the tooltip so people can see it if they want to
      //do this with the file finding functionality of the FileProperty
      FileProperty search("dummy", k->toStdString(), FileProperty::Load,
        std::vector<std::string>(), Direction::Input);
      if ( search.isValid() == "" )
      {//this means the file was found
        newL->setToolTip(QString::fromStdString(search.value()));
      }
    }
  }
  m_SANSForm->new2Add_edit->clear();
}
/** Executes the add_runs() function inside the SANSadd2 script
*/
void SANSAddFiles::runPythonAddFiles()
{
  if (m_pythonRunning)
  {//it is only possible to run one python script at a time
    return;
  }

  add2Runs2Add();

  QString code_torun = "import SANSadd2\n";
  code_torun += "print SANSadd2.add_runs('";
  code_torun += m_SANSForm->summedPath_edit->text()+"', (";
  //there are multiple file list inputs that can be filled in loop through them
  for(int i = 0; i < m_SANSForm->toAdd_List->count(); ++i )
  {
    const QString filename =
      m_SANSForm->toAdd_List->item(i)->data(Qt::WhatsThisRole).toString();
    //allow but do nothing with empty entries
    if ( ! filename.isEmpty() )
    {
      code_torun += "'"+filename+"',";
    }
  }
  if ( code_torun.endsWith(',') )
  {//we've made a comma separated list, there can be no comma at the end
    code_torun.truncate(code_torun.size()-1);
  }
  //pass the current instrument
  code_torun += "),'"+m_SANSForm->inst_opt->currentText()+"', '";
  QString ext = m_SANSForm->file_opt->itemData(
    m_SANSForm->file_opt->currentIndex()).toString();
  code_torun += ext+"'";
  
  code_torun += ", rawTypes=(";
  std::set<std::string>::const_iterator end = m_rawExts.end();
  for(std::set<std::string>::const_iterator j=m_rawExts.begin(); j != end; ++j)
  {
    code_torun += "'"+QString::fromStdString(*j)+"',";
  }
  //remove the comma that would remain at the end of the list
  code_torun.truncate(code_torun.length()-1);
  code_torun += ")";

  code_torun += ")\n";

  g_log.debug() << "Executing Python: \n" << code_torun.toStdString() << std::endl;

  m_SANSForm->sum_Btn->setEnabled(false);
  m_pythonRunning = true;
  QString status = runPythonCode(code_torun, false);
  m_SANSForm->sum_Btn->setEnabled(true);
  m_pythonRunning = false;

  if( ! status.startsWith("The following file has been created:") )
  {
    if (status.isEmpty())
    {
      status = "Could not sum files, there may be more\ninformation in the Results Log window";
    }
    QMessageBox::critical(this, "Error adding files", status);
  }
  else
  {
    QMessageBox::information(this, "Files summed", status);
  }
}
/** This slot opens a file browser allowing a user select a path, which
* is copied into the summedPath_edit
*/
void SANSAddFiles::summedPathBrowse()
{
  QString dir = m_SANSForm->summedPath_edit->text();
  
  QString oPath = QFileDialog::getExistingDirectory(parForm, "Output path", dir);
  if( ! oPath.trimmed().isEmpty() )
  {
    m_SANSForm->summedPath_edit->setText(oPath);
    QSettings prevVals;
    prevVals.beginGroup("CustomInterfaces/SANSRunWindow/AddRuns");
    prevVals.setValue("OutPath", oPath);
  }
}
/** This slot opens a file browser allowing a user select files, which is
* copied into the new2Add_edit ready to be copied to the listbox (toAdd_List)
*/
void SANSAddFiles::new2AddBrowse()
{
  QSettings prevVals;
  prevVals.beginGroup("CustomInterfaces/SANSRunWindow/AddRuns");
  //get the previous data input directory or, if there wasn't one, use the directory entered on the main form
  QString dir =
    prevVals.value("InPath", m_SANSForm->datadir_edit->text()).toString();
  
	QString fileFilter = "Files (";

  std::set<std::string>::const_iterator end = m_exts.end();
  for(std::set<std::string>::const_iterator i = m_exts.begin(); i != end; ++i)
  {
    fileFilter += " *"+QString::fromStdString(*i);
  }

  fileFilter += ")";
  const QStringList files =
    QFileDialog::getOpenFileNames(parForm, "Select files", dir, fileFilter);

  if( ! files.isEmpty() )
  {
    // next time the user clicks browse they will see the directory that they last loaded a file from
    QFileInfo defPath(files[0]);
    prevVals.setValue("InPath", defPath.absoluteDir().absolutePath());
    //join turns the list into a single string with the entries seperated, in this case, by ,
    m_SANSForm->new2Add_edit->setText(files.join(", "));
  }
}
/** Normally in responce to an edit this sets data associated with the cell
*  to the cells text and removes the tooltip
*/
void SANSAddFiles::setCellData(QListWidgetItem *)
{
  QListWidgetItem* editting = m_SANSForm->toAdd_List->currentItem();
  if (editting)
  {
    editting->setData(Qt::WhatsThisRole, QVariant(editting->text()));
    editting->setToolTip("");
  }
}
/** Called when the clear button is clicked it clears the list of file
* names to add table
*/
void SANSAddFiles::clearClicked()
{
  m_SANSForm->toAdd_List->clear();
  insertListFront("");
}

void SANSAddFiles::removeSelected()
{
  QList<QListWidgetItem*> sels = m_SANSForm->toAdd_List->selectedItems();
  while( sels.count() > 0 )
  {
    int selRow = m_SANSForm->toAdd_List->row(sels.front());
    delete m_SANSForm->toAdd_List->takeItem(selRow);
    sels = m_SANSForm->toAdd_List->selectedItems();
  }
}


}//namespace CustomInterfaces
}//namespace MantidQt
