//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/SANSRunWindow.h"

#include <QLineEdit>
#include <QFileDialog>
#include <QHash>

//Add this class to the list of specialised dialogs in this namespace
namespace MantidQt
{
namespace CustomInterfaces
{
  DECLARE_SUBWINDOW(SANSRunWindow);
}
}

using namespace MantidQt::CustomInterfaces;

//----------------------
// Public member functions
//----------------------
///Constructor
SANSRunWindow::SANSRunWindow(QWidget *parent) :
  UserSubWindow(parent), m_last_dir(""), m_run_no_boxes(), m_period_lbls()
{
}

/// Set up the dialog layout
void SANSRunWindow::initLayout()
{
    m_uiForm.setupUi(this);

    //Button connections
    connect(m_uiForm.data_dirBtn, SIGNAL(clicked()), this, SLOT(selectDataDir()));
    connect(m_uiForm.userfileBtn, SIGNAL(clicked()), this, SLOT(selectUserFile()));

    connect(m_uiForm.load_dataBtn, SIGNAL(clicked()), this, SLOT(loadButtonClicked()));

    //Text edit map
    m_run_no_boxes.insert(0, m_uiForm.sct_sample_edit);
    m_run_no_boxes.insert(1, m_uiForm.sct_can_edit);
    m_run_no_boxes.insert(2, m_uiForm.sct_bkgd_edit);
    m_run_no_boxes.insert(3, m_uiForm.tra_sample_edit);
    m_run_no_boxes.insert(4, m_uiForm.tra_can_edit);
    m_run_no_boxes.insert(5, m_uiForm.tra_bkgd_edit);
    m_run_no_boxes.insert(6, m_uiForm.direct_sample_edit);
    m_run_no_boxes.insert(7, m_uiForm.direct_can_edit);
    m_run_no_boxes.insert(8, m_uiForm.direct_bkgd_edit);

    //Period label hash. Each label has a buddy set to its corresponding text edit field
    m_period_lbls.insert(0, m_uiForm.sct_prd_tot1);
    m_period_lbls.insert(1, m_uiForm.sct_prd_tot2);
    m_period_lbls.insert(2, m_uiForm.sct_prd_tot3);
    m_period_lbls.insert(3, m_uiForm.tra_prd_tot1);
    m_period_lbls.insert(4, m_uiForm.tra_prd_tot2);
    m_period_lbls.insert(5, m_uiForm.tra_prd_tot3);
    m_period_lbls.insert(6, m_uiForm.direct_prd_tot1);
    m_period_lbls.insert(7, m_uiForm.direct_prd_tot2);   
    m_period_lbls.insert(8, m_uiForm.direct_prd_tot3);

}

/**
 * Select the base directory for the data
 */
void SANSRunWindow::selectDataDir()
{
  QString data_dir = QFileDialog::getExistingDirectory(this, tr("Choose a directory"), m_last_dir);
  if( QDir(data_dir).exists() ) 
  {
    m_last_dir = data_dir;
    m_uiForm.datadir_edit->setText(data_dir);
  }
}


/**
 * Select the user file
 */
void SANSRunWindow::selectUserFile()
{
  QString file_path = QFileDialog::getOpenFileName(this, tr("Select a user file"), m_last_dir,
						   "SANS user files (*.com)");
  if( QFileInfo(file_path).exists() ) 
  {
    m_last_dir = QFileInfo(file_path).absoluteDir().path();
    m_uiForm.userfile_edit->setText(file_path);
  }
}

/**
 * Receive a load button click signal
 */
void SANSRunWindow::loadButtonClicked()
{
  QString work_dir = QFileInfo(m_uiForm.datadir_edit->text()).absoluteFilePath();
  if( work_dir.isEmpty() || !QDir(work_dir).exists() )
  {
    showInformationBox("The specified data directory " + m_uiForm.datadir_edit->text() + " does not exist.");
    return;
  }
  
  QString code = "setWorkingDirectory('" + work_dir + "')\n";
  //A load command for each box if there is anything in it and it has not already been loaded
  QMapIterator<int, QLineEdit*> itr(m_run_no_boxes);
  QStringList unique_nos;
  while( itr.hasNext() )
  {
    itr.next();
    QString run_no = itr.value()->text();
    itr.value()->setReadOnly(true);
    if( run_no.isEmpty() || unique_nos.contains(run_no) ) continue;

    //QString filename ="LOQ" + run_no + ".raw";
    QString filename = run_no;
    if( !QFileInfo(work_dir + "/" + filename).exists() ) continue;
    code += writeLoadRawCmd(filename, run_no.split(".")[0]);
    
    unique_nos << run_no;
  }
  
  //We need to sort out the number of periods in each data set

  // Print the names of the workspaces
  code += "wksp_dict = {}\n"
    "for name in getWorkspaceNames():\n"
    "\tname = name.split('_')[0]\n"
    "\tif wksp_dict.has_key(name):\n"
    "\t\twksp_dict[name] += 1\n"
    "\telse:\n"
    "\t\twksp_dict[name] = 1\n\n"
    "for k,v in wksp_dict.iteritems():\n"
    "\tprint k + ':' + str(v)\n";

    
  QString results = runPythonCode(code);
  showInformationBox(results);

  QStringList output_lines = results.split("\n");
  QStringListIterator sitr(output_lines);
  QHash<QString, int> period_nos;
  while( sitr.hasNext() )
  {
    QString line = sitr.next();
    period_nos.insert(line.section(":", 0, 0), line.section(":",1, 1).toInt());
  }

  //Now update the relevant boxes
  itr = m_run_no_boxes;
  while( itr.hasNext() )
  {
    itr.next();
    QString text = itr.value()->text().section(".",0,0);
    int total_periods = period_nos.value(text);
    QLabel *label = qobject_cast<QLabel*>(m_period_lbls.value(itr.key()));
    if( !label ) continue;

    label->setText("/ " + QString::number(total_periods));
    QLineEdit *userentry = qobject_cast<QLineEdit*>(label->buddy());
    if( !userentry ) continue;

    userentry->setText("1");
  }
}

/**
 * Write a Python LoadRaw command. This assumes that the filename has already been validated
 * @param filename The Filename property value
 * @param workspace The OutputWorkspace property value
 * @param spec_min The spectrum_min property value (optional)
 * @param spec_max The spectrum_max property value (optional)
 * @param spec_list The spectrum_list property value (optional)
 * @param cache_opt The cache option (optional)
 */
QString SANSRunWindow::writeLoadRawCmd(const QString & filename, const QString & workspace, 
				       const QString & spec_min, const QString & spec_max,
				       const QString & spec_list, const QString & cache_opt)
{
  QString command = "LoadRaw(Filename = '" + filename + "', OutputWorkspace = '" + workspace + "'";
  //Now the optional properties
  if( !spec_min.isEmpty() )
  {
    command += ", spectrum_min = '" + spec_min + "'";
  }
  if( !spec_max.isEmpty() )
  {
    command += ", spectrum_max = '" + spec_max + "'";
  }
  if( !spec_list.isEmpty() )
  {
    command += ", spectrum_list = '" + spec_list + "'";
  }
  if( !cache_opt.isEmpty() )
  {
    command += ", Cache = '" + cache_opt + "'";
  }
  command += ")\n";
  return command;
}
