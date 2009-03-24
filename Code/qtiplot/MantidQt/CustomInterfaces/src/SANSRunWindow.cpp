//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/SANSRunWindow.h"
#include "MantidQtCustomInterfaces/SANSUtilityDialogs.h"

#include <QLineEdit>
#include <QFileDialog>
#include <QHash>
#include <QTextStream>
#include <QTableWidget>
#include <QSettings>

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
  UserSubWindow(parent), m_last_dir(""), m_run_no_boxes(), m_unique_runs(), m_period_lbls()
{
}

SANSRunWindow::~SANSRunWindow()
{
  saveSettings();
}

//-----------------------------------------
// Private member functions
//-----------------------------------------
/**
 * Set up the dialog layout
 */
void SANSRunWindow::initLayout()
{
    m_uiForm.setupUi(this);

    //Button connections
    connect(m_uiForm.data_dirBtn, SIGNAL(clicked()), this, SLOT(selectDataDir()));
    connect(m_uiForm.userfileBtn, SIGNAL(clicked()), this, SLOT(selectUserFile()));

    connect(m_uiForm.load_dataBtn, SIGNAL(clicked()), this, SLOT(loadButtonClicked()));

    connect(m_uiForm.plotBtn, SIGNAL(clicked()), this, SLOT(plotButtonClicked()));

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

    // Combo boxes
    connect(m_uiForm.wav_dw_opt, SIGNAL(currentIndexChanged(int)), this, 
	    SLOT(stepComboChange(int)));
    connect(m_uiForm.q_dq_opt, SIGNAL(currentIndexChanged(int)), this, 
	    SLOT(stepComboChange(int)));
    connect(m_uiForm.qy_dqy_opt, SIGNAL(currentIndexChanged(int)), this, 
	    SLOT(stepComboChange(int)));

    connect(this, SIGNAL(destroyed(QObject*)), this, SLOT(saveSettings()));

    readSettings();
}    

/**
 * Restore previous input
 */
void SANSRunWindow::readSettings()
{
  QSettings value_store;
  value_store.beginGroup("CustomInterfaces/SANSRunWindow");
  m_uiForm.datadir_edit->setText(value_store.value("data_dir").toString());
  m_uiForm.userfile_edit->setText(value_store.value("user_file").toString());
  value_store.endGroup();
}


/**
 * Save input for future use
 */
void SANSRunWindow::saveSettings()
{
  std::cerr << "saveSettings called\n";
  QSettings value_store;
  value_store.beginGroup("CustomInterfaces");
  value_store.beginGroup("SANSRunWindow");
  value_store.setValue("data_dir", QDir(m_uiForm.datadir_edit->text()).absolutePath());
  value_store.setValue("user_file", m_uiForm.userfile_edit->text());
  value_store.endGroup();
  value_store.endGroup();
}

/**
 * Load the user file specified in the text field
 * @returns Boolean indicating whether we were successful or not
 */
bool SANSRunWindow::loadUserFile()
{
  QString filetext = m_uiForm.userfile_edit->text();
  if( filetext.isEmpty() ) return false;
  if( QFileInfo(filetext).isRelative() )
  {
    filetext = QDir(m_uiForm.datadir_edit->text()).absoluteFilePath(filetext);
  }

  if( !QFileInfo(filetext).exists() ) return false;

  QFile user_file(filetext);
  if( !user_file.open(QIODevice::ReadOnly) ) return false;
  
  QTextStream stream(&user_file);
  QString data;
  while( !stream.atEnd() )
  {
    //    data.append(stream.readLine() + "\n");
    QString com_line = stream.readLine();
    if( com_line.startsWith("L/") )
    {
      readLimits(com_line.section("/", 1));
    }
    else if( com_line.startsWith("MON") )
    {
      QString filepath = QFileInfo(filetext).absoluteDir().absoluteFilePath(com_line.section("]", 1));
      //Line has the form MON/FIELD=...
      QString field = com_line.section("/", 1).section("=", 0, 0);
      std::cerr << field.toStdString() << "\n";
      if( field.compare("direct", Qt::CaseInsensitive) == 0 )
      {
	m_uiForm.direct_file->setText(filepath);
      }
      else if( field.compare("hab", Qt::CaseInsensitive) == 0 )
      {
	m_uiForm.hab_file->setText(filepath);
      }
      else if( field.compare("flat", Qt::CaseInsensitive) == 0 )
      {
	m_uiForm.flat_file->setText(filepath);
      }
      else {}
    }
       
  }
  user_file.close();
  
  return true;
}

/**
 * Read a limit line from the user file
 * @param com_line A line from the LOQ user file that started with "L/" (note that the tag has been removed)
 */
void SANSRunWindow::readLimits(const QString & com_line)
{
  QStringList pieces = com_line.split('/');
  QString quantity = pieces[0].section(' ', 0, 0);
  QString min = pieces[0].section(' ', 1, 1);
  QString max = pieces[0].section(' ', 2, 2);
  QString step = pieces[0].section(' ', 3, 3);
  if( quantity == "R" )
  {
    m_uiForm.rad_min->setText(min);
    m_uiForm.rad_max->setText(max);
    m_uiForm.rad_dr->setText(step);
  }
  else if( quantity == "SP" )
  {
    m_uiForm.spec_min->setText(min);
    m_uiForm.spec_max->setText(max);
  }
  else
  {
    int opt_index(0);
    if( pieces[1].compare("log", Qt::CaseInsensitive) == 0 ) 
    { 
      opt_index = 1;
    }
    if( quantity == "WAV" )
    {
      m_uiForm.wav_min->setText(min);
      m_uiForm.wav_max->setText(max);
      m_uiForm.wav_dw->setText(step);
      m_uiForm.wav_dw_opt->setCurrentIndex(opt_index);
      if( opt_index == 0 ) m_uiForm.wav_step_lbl->setText("stepping");
      else  m_uiForm.wav_step_lbl->setText("dW / W");
    }
    else if( quantity == "Q" )
    {
      m_uiForm.q_min->setText(min);
      m_uiForm.q_max->setText(max);
      m_uiForm.q_dq->setText(step);
      m_uiForm.q_dq_opt->setCurrentIndex(opt_index);
      if( opt_index == 0 ) m_uiForm.q_step_lbl->setText("stepping");
      else  m_uiForm.q_step_lbl->setText("dQ / Q");
    }
    else if( quantity == "QXY" )
    {
      m_uiForm.qy_min->setText(min);
      m_uiForm.qy_max->setText(max);
      m_uiForm.qy_dqy->setText(step);
      m_uiForm.qy_dqy_opt->setCurrentIndex(opt_index);
      if( opt_index == 0 ) m_uiForm.qy_step_lbl->setText("stepping");
      else  m_uiForm.qy_step_lbl->setText("dQy / Qy");
    }
    else return;
  }
}

//-------------------------------------
// Private SLOTS
//------------------------------------
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
  QString origin_dir = QDir::currentPath();
  QString work_dir = QFileInfo(m_uiForm.datadir_edit->text()).absoluteFilePath();
    if( work_dir.isEmpty() || !QDir(work_dir).exists() )
  {
    showInformationBox("The specified data directory " + m_uiForm.datadir_edit->text() + " does not exist.");
    return;
  }

  // Check if we have loaded the data_file
  if( !loadUserFile() )
  {
    showInformationBox("Error loading user file '" + m_uiForm.userfile_edit->text() + "',  cannot continue.");
    return;
  }


  QString code = "setWorkingDirectory('" + work_dir + "')\n";
  //A load command for each box if there is anything in it and it has not already been loaded
  QMapIterator<int, QLineEdit*> itr(m_run_no_boxes);
  m_unique_runs.clear();  
  while( itr.hasNext() )
  {
    itr.next();
    QString run_no = itr.value()->text();
    if( run_no.isEmpty() || m_unique_runs.contains(run_no) ) continue;

    QString filename ="LOQ" + run_no + ".raw";
    if( !QFileInfo(work_dir + "/" + filename).exists() ) continue;
    code += writeLoadRawCmd(filename, run_no.split(".")[0]);
    
    m_unique_runs << run_no;
  }
  
  //We need to sort out the number of periods in each data set

  // Print the names of the workspaces
  code += "wksp_dict = {}\n"
    "for name in mtd.getWorkspaceNames():\n"
    "\tname = name.split('_')[0]\n"
    "\tif wksp_dict.has_key(name):\n"
    "\t\twksp_dict[name] += 1\n"
    "\telse:\n"
    "\t\twksp_dict[name] = 1\n\n"
    "for k,v in wksp_dict.iteritems():\n"
    "\tprint k + ':' + str(v)\n";

  // Get the min and max X values
  code += "\nwksp = mtd.getMatrixWorkspace(mtd.getWorkspaceNames()[0])\n"
    "print 'X:MIN:' + str(wksp.readX(0)[0])\n"
    "print 'X:MAX:' + str(wksp.readX(0)[len(wksp.readX(0))-1])\n";

  code += "setWorkingDirectory('" + origin_dir + "')\n";
  
  QString results = runPythonCode(code);
  if( results.isEmpty() ) 
  {
    showInformationBox("No data could be loaded.");
    return;
  }
  
  QStringList output_lines = results.split("\n");
  QStringListIterator sitr(output_lines);
  QHash<QString, int> period_nos;
  while( sitr.hasNext() )
  {
    QString line = sitr.next();
    if( line.startsWith("X:") )
    {
      QString value = line.section(':', 2, 2);
      if( line.section(':', 1, 1) == "MIN" )
      {
	m_uiForm.tof_min->setText(value);
      }
      else
      {
	m_uiForm.tof_max->setText(value);
      }
    }
    else
    {
      period_nos.insert(line.section(":", 0, 0), line.section(":",1, 1).toInt());
    }
  }

  //Now update the relevant boxes
  itr = m_run_no_boxes;
  while( itr.hasNext() )
  {
    itr.next();
    QString text = itr.value()->text();
    if( text.isEmpty() ) continue;
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
 * Plot button slot
 */
void SANSRunWindow::plotButtonClicked()
{
  SANSPlotDialog dialog(this);
  dialog.setAvailableData(m_unique_runs);
  connect(&dialog, SIGNAL(pythonCodeConstructed(const QString&)), this, SIGNAL(runAsPythonScript(const QString&)));
 
  dialog.exec();
}

/**
 * A ComboBox option change
 * @param new_index The new index that has been set
 */
void SANSRunWindow::stepComboChange(int new_index)
{
  if( !sender() ) return;

  QString origin = sender()->objectName();
  if( origin.startsWith("wav") )
  {
    if( new_index == 0 ) m_uiForm.wav_step_lbl->setText("stepping");
    else m_uiForm.wav_step_lbl->setText("dW / W");
  }
  else if( origin.startsWith("q_dq") )
  {
    if( new_index == 0 ) m_uiForm.q_step_lbl->setText("stepping");
    else m_uiForm.q_step_lbl->setText("dQ / Q");
  } 
  else
  {
    if( new_index == 0 ) m_uiForm.qy_step_lbl->setText("XY step");
    else m_uiForm.qy_step_lbl->setText("dQ / Q");
  }

}

//-----------------------------------------
// Python code utility functions
//-----------------------------------------
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
