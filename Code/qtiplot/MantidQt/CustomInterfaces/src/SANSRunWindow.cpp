//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/SANSRunWindow.h"
#include "MantidQtCustomInterfaces/SANSUtilityDialogs.h"

#include "MantidKernel/ConfigService.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IInstrument.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/V3D.h"

#include <QLineEdit>
#include <QFileDialog>
#include <QHash>
#include <QTextStream>
#include <QTreeWidgetItem>
#include <QSettings>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QInputDialog>
#include <QRegExp>
#include <QRegExpValidator>
#include <QSignalMapper>

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
  UserSubWindow(parent), m_data_dir(""), m_ins_defdir(""), m_last_dir(""), m_cfg_loaded(false), m_run_no_boxes(), m_period_lbls(), m_pycode_loqreduce("")
{
  m_reducemapper = new QSignalMapper(this);
}

///Destructor
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

    connect(m_uiForm.load_dataBtn, SIGNAL(clicked()), this, SLOT(handleLoadButtonClick()));
    connect(m_uiForm.plotBtn, SIGNAL(clicked()), this, SLOT(handlePlotButtonClick()));
    //    m_uiForm.plotBtn->setEnabled(false);

    // Disable most things so that load is the only thing that can be done
    m_uiForm.oneDBtn->setEnabled(false);
    m_uiForm.twoDBtn->setEnabled(false);
    for( int index = 1; index < m_uiForm.tabWidget->count(); ++index )
    {
      m_uiForm.tabWidget->setTabEnabled(index, false);
    }
    
    // Reduction buttons
    connect(m_uiForm.oneDBtn, SIGNAL(clicked()), m_reducemapper, SLOT(map()));
    m_reducemapper->setMapping(m_uiForm.oneDBtn, "1D");
    connect(m_uiForm.twoDBtn, SIGNAL(clicked()), m_reducemapper, SLOT(map()));
    m_reducemapper->setMapping(m_uiForm.twoDBtn, "2D");
    connect(m_reducemapper, SIGNAL(mapped(const QString &)), this, SLOT(handleReduceButtonClick(const QString &)));
    
    connect(m_uiForm.showMaskBtn, SIGNAL(clicked()), this, SLOT(handleShowMaskButtonClick()));
    connect(this, SIGNAL(dataReadyToProcess(bool)), m_uiForm.oneDBtn, SLOT(setEnabled(bool)));
    connect(this, SIGNAL(dataReadyToProcess(bool)), m_uiForm.twoDBtn, SLOT(setEnabled(bool)));

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
	    SLOT(handleStepComboChange(int)));
    connect(m_uiForm.q_dq_opt, SIGNAL(currentIndexChanged(int)), this, 
	    SLOT(handleStepComboChange(int)));
    connect(m_uiForm.qy_dqy_opt, SIGNAL(currentIndexChanged(int)), this, 
	    SLOT(handleStepComboChange(int)));

    // file extensions
    m_uiForm.file_opt->setItemData(0, ".raw");
    m_uiForm.file_opt->setItemData(1, ".nxs");

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

  //The instrument definition directory
  m_ins_defdir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("instrumentDefinition.directory"));
  
}

/**
 * Save input for future use
 */
void SANSRunWindow::saveSettings()
{
  QSettings value_store;
  value_store.beginGroup("CustomInterfaces/SANSRunWindow");
  if( !m_data_dir.isEmpty() ) value_store.setValue("data_dir", m_data_dir);
  if( !m_uiForm.userfile_edit->text().isEmpty() ) value_store.setValue("user_file", m_uiForm.userfile_edit->text());
  value_store.endGroup();
}

/**
 * Load the data reduction template for the LOQ analysis. It is
 * currently assumed that this resides in the SANS subdirectory
 * pointed to by the pythonscripts.directory config varibale in
 * Mantid.properties
 */
bool SANSRunWindow::readPyReductionTemplate()
{
  QDir scriptsdir(QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("pythonscripts.directory")));
  QString reduce_script = scriptsdir.absoluteFilePath("SANS/LOQ_ReduceData.py");
    
  if( !QFileInfo(reduce_script).exists() ) 
  {
    showInformationBox("Error: Unable to load template script, " + reduce_script + " does not exist");
    return false;
  }
  
  QFile py_script(reduce_script);
  if( !py_script.open(QIODevice::ReadOnly) ) 
  {
    showInformationBox("Error: Unable to access template script, " + reduce_script);
    return false;
  }
  QTextStream stream(&py_script);
  m_pycode_loqreduce.clear();
  while( !stream.atEnd() )
  {
    m_pycode_loqreduce.append(stream.readLine() + "\n");
  }
  py_script.close();
  return true;
}

/**
 * Load the mask template script for LOQ. It is
 * currently assumed that this resides in the SANS subdirectory
 * pointed to by the pythonscripts.directory config varibale in
 * Mantid.properties
 */
bool SANSRunWindow::readPyViewMaskTemplate()
{
  QDir scriptsdir(QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("pythonscripts.directory")));
  QString mask_script = scriptsdir.absoluteFilePath("SANS/LOQ_ViewMask.py");
    
  if( !QFileInfo(mask_script).exists() ) 
  {
    showInformationBox("Error: Unable to load template script, " + mask_script + " does not exist");
    return false;
  }
  
  QFile py_script(mask_script);
  if( !py_script.open(QIODevice::ReadOnly) ) 
  {
    showInformationBox("Error: Unable to access template script, " + mask_script);
    return false;
  }
  QTextStream stream(&py_script);
  m_pycode_viewmask.clear();
  while( !stream.atEnd() )
  {
    m_pycode_viewmask.append(stream.readLine() + "\n");
  }
  py_script.close();
  return true;
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
    filetext = QDir(m_data_dir).absoluteFilePath(filetext);
  }
  
  if( !QFileInfo(filetext).exists() ) return false;
  
  QFile user_file(filetext);
  if( !user_file.open(QIODevice::ReadOnly) ) return false;
  
  //Clear the def masking info table
  m_uiForm.def_mask_table->clear();
  QTextStream stream(&user_file);
  QString data;
  while( !stream.atEnd() )
  {
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
    else if( com_line.startsWith("set centre") )
    {
      m_uiForm.beam_x->setText(com_line.section(' ', 2, 2));
      m_uiForm.beam_y->setText(com_line.section(' ', 3, 3));
    }
    else if( com_line.startsWith("set scales") )
    {
      m_uiForm.scale_factor->setText(com_line.section(' ', 2, 2));
    }
    else if( com_line.startsWith("mask", Qt::CaseInsensitive) )
    {
      QString type = com_line.section(' ',1, 1);
      QString col1_txt(""), col2_txt("");
      if( type.startsWith('S', Qt::CaseInsensitive) )
      {
	col1_txt = "Spectrum";
	col2_txt = type.section('S', 1, -1, QString::SectionCaseInsensitiveSeps);
      }
      else if( type.startsWith('h', Qt::CaseInsensitive) )
      {
	col1_txt = "Strip";
	col2_txt = type;
      }
      else continue;
      
      int row = m_uiForm.def_mask_table->rowCount();
      //Insert line after last row
      m_uiForm.def_mask_table->insertRow(row);
      QTableWidgetItem *item1 = new QTableWidgetItem(col1_txt);
      QTableWidgetItem *item2 = new QTableWidgetItem(col2_txt);
      m_uiForm.def_mask_table->setItem(row, 0, item1);
      m_uiForm.def_mask_table->setItem(row, 1, item2);

    }
    else {}
       
  }
  user_file.close();

  // Phi values default to -90 and 90
  m_uiForm.phi_min->setText("-90");
  m_uiForm.phi_max->setText("90");
  
  m_cfg_loaded = true;
  m_uiForm.userfileBtn->setText("Reload");
  m_uiForm.tabWidget->setTabEnabled(m_uiForm.tabWidget->count() - 1, true);
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

  //Ensure all doubles come out with a '0.' not just '.' prefix
  if( min.startsWith('.') ) min.prepend('0');
  if( max.startsWith('.') ) max.prepend('0');
  if( step.startsWith('.') ) step.prepend('0');

  if( quantity == "R" )
  {
    m_uiForm.rad_min->setText(min);
    m_uiForm.rad_max->setText(max);
    m_uiForm.rad_dr->setText(step);
    //Add mask values to table
    int row = m_uiForm.def_mask_table->rowCount();
    //Insert line after last row
    m_uiForm.def_mask_table->insertRow(row);
    QTableWidgetItem *item1 = new QTableWidgetItem("Beam stop");
    QTableWidgetItem *item2 = new QTableWidgetItem("Shape");
    m_uiForm.def_mask_table->setItem(row, 0, item1);
    m_uiForm.def_mask_table->setItem(row, 1, item2);
    m_uiForm.def_mask_table->insertRow(++row);
    item1 = new QTableWidgetItem("Corners");
    item2 = new QTableWidgetItem("Shape");
    m_uiForm.def_mask_table->setItem(row, 0, item1);
    m_uiForm.def_mask_table->setItem(row, 1, item2);
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
      m_uiForm.qy_max->setText(max);
      m_uiForm.qy_dqy->setText(step);
      m_uiForm.qy_dqy_opt->setCurrentIndex(opt_index);
      if( opt_index == 0 ) m_uiForm.qy_step_lbl->setText("stepping");
      else  m_uiForm.qy_step_lbl->setText("dQy / Qy");
    }
    else return;
  }
}

/**
 * Retrieve and set the component distances
 * @param wsname The name of the workspace
 * @param lms The result of the moderator-sample distance
 * @param lsda The result of the sample-detector bank 1 distance
 * @param lsdb The result of the sample-detector bank 2 distance
 */
void SANSRunWindow::componentDistances(const QString & wsname, double & lms, double & lsda, double & lsdb)
{
  if( !workspaceExists(wsname) ) return;
  Mantid::API::MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
    (Mantid::API::AnalysisDataService::Instance().retrieve(wsname.toStdString()));

  Mantid::API::IInstrument_sptr instr = ws->getInstrument();
  if( instr == boost::shared_ptr<Mantid::API::IInstrument>() ) return;

  Mantid::Geometry::IObjComponent_sptr source =  instr->getSource();
  if( source == boost::shared_ptr<Mantid::Geometry::IObjComponent>() ) return;
  Mantid::Geometry::IObjComponent_sptr sample =  instr->getSample();
  if( sample == boost::shared_ptr<Mantid::Geometry::IObjComponent>() ) return;

  lms = source->getPos().distance(sample->getPos());
   
  //Find the main detector bank
  boost::shared_ptr<Mantid::Geometry::IComponent> comp = instr->getComponentByName("main-detector-bank");
  if( comp != boost::shared_ptr<Mantid::Geometry::IComponent>() )
  {
    lsda = sample->getPos().distance(comp->getPos());
  }

  comp = instr->getComponentByName("HAB");
  if( comp != boost::shared_ptr<Mantid::Geometry::IComponent>() )
  {
    lsdb = sample->getPos().distance(comp->getPos());
  }

}

/**
 * Set the state of processing.
 * @param running If we are processing then some interaction is disabled
 * @param type The reduction type, 0 = 1D and 1 = 2D
 */
void SANSRunWindow::setProcessingState(bool running, int type)
{
  if( running )
  {
    m_uiForm.load_dataBtn->setEnabled(false);
    if( type == 0 )
    {   
      m_uiForm.oneDBtn->setText("Running ...");
    }
    else 
    {
      m_uiForm.twoDBtn->setText("Running ...");
    }
    m_uiForm.oneDBtn->setEnabled(false);
    m_uiForm.twoDBtn->setEnabled(false);
  }
  else
  {
    m_uiForm.oneDBtn->setText("1D Reduce");
    m_uiForm.twoDBtn->setText("2D Reduce");
    m_uiForm.oneDBtn->setEnabled(true);
    m_uiForm.twoDBtn->setEnabled(true);
    m_uiForm.load_dataBtn->setEnabled(true);
  }
}

/**
 * Does the workspace exist in the AnalysisDataService
 * @param ws_name The name of the workspace
 * @returns A boolean indicatingif the given workspace exists in the AnalysisDataService
 */
bool SANSRunWindow::workspaceExists(const QString & ws_name) const
{
  return Mantid::API::AnalysisDataService::Instance().doesExist(ws_name.toStdString());
}

/**
 * @returns A list of the currently available workspaces
 */
QStringList SANSRunWindow::currentWorkspaceList() const
{
  std::vector<std::string> ws_list = Mantid::API::AnalysisDataService::Instance().getObjectNames();
  std::vector<std::string>::const_iterator iend = ws_list.end();
  QStringList current_list;
  for( std::vector<std::string>::const_iterator itr = ws_list.begin(); itr != iend; ++itr )
  {
    current_list.append(QString::fromStdString(*itr));
  }
  return current_list;
}

/**
 * Is the user file loaded
 * @returns A boolean indicating whether the user file has been parsed in to the details tab
 */
bool SANSRunWindow::isUserFileLoaded() const
{
  return m_cfg_loaded;
}

/**
 * Get the path the the raw file indicated by the run number.This checks the given directory for the number 
 * given. Left-padding of zeroes is done as required.
 * @param run_no The run number to search for
 */
QString SANSRunWindow::getRawFilePath(const QString & data_dir, const QString & run_no) const
{
  //Do a quick check for the existence of the file with these exact credentials
  QDir directory(data_dir);
  QString prefix = m_uiForm.inst_opt->currentText();
  QString ext = m_uiForm.file_opt->itemData(m_uiForm.file_opt->currentIndex()).toString();
  QString filename = directory.absoluteFilePath(prefix + run_no + ext);
  if( QFileInfo(filename).exists() ) return filename;
  
  //Otherwise check entries with padded
  //RegExp2 will be default for Qt >= 5. It matches more like other regex engines
  QRegExp rx(prefix + "[0]*" + run_no + ".raw", Qt::CaseInsensitive, QRegExp::RegExp2);
  QRegExpValidator matcher(rx, 0);
  QStringList files = directory.entryList( QStringList(prefix + "*"), QDir::Files | QDir::NoSymLinks );
  QStringListIterator itr(files);
  filename.clear();
  while( itr.hasNext() )
  {
    filename = itr.next();
    int pos(0);
    if( matcher.validate(filename, pos) == QValidator::Acceptable )
    {
      break;
    }
    else 
    {
      filename.clear();
    }
  }
  if( filename.isEmpty() ) return QString();
  else return directory.absoluteFilePath(filename);
}
 
/**
 * Create the a comma separated list of masking values using the masking information from the Mask tab
 */
QString SANSRunWindow::createMaskString() const
{
  QString maskstring;
  int nrows = m_uiForm.def_mask_table->rowCount();
  for( int r = 0; r < nrows; ++r )
  {
    QString type = m_uiForm.def_mask_table->item(r, 1)->text();
    if( type == "Shape" ) continue;
    
    maskstring += m_uiForm.def_mask_table->item(r, 1)->text() + ",";
  }
  maskstring += m_uiForm.user_maskEdit->text();
  return maskstring;
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
  if( !data_dir.isEmpty() && QDir(data_dir).exists() ) 
  {
    m_last_dir = data_dir;
    m_data_dir = data_dir;
    m_uiForm.datadir_edit->setText(data_dir);
  }
}


/**
 * Select and load the user file
 */
void SANSRunWindow::selectUserFile()
{
  QString box_text = m_uiForm.userfile_edit->text();
  QString start_path = box_text;
  if( box_text.isEmpty() )
  {
    start_path = m_last_dir;
  }
  
  QString file_path = QFileDialog::getOpenFileName(this, "Select a user file", start_path, "AllFiles (*.*)");    
  if( file_path.isEmpty() || QFileInfo(file_path).isDir() ) return;
  m_uiForm.userfile_edit->setText(file_path);
  
  loadUserFile();
  //path() returns the directory
  m_last_dir = QFileInfo(file_path).path();
}

/**
 * Receive a load button click signal
 */
void SANSRunWindow::handleLoadButtonClick()
{
  QString origin_dir = QDir::currentPath();
  QString work_dir = QDir(m_uiForm.datadir_edit->text()).absolutePath();
  if( work_dir.isEmpty() || !QDir(work_dir).exists() )
  {
    showInformationBox("The specified data directory " + m_uiForm.datadir_edit->text() + " does not exist.");
    return;
  }
  if( !work_dir.endsWith('/') ) work_dir += "/";
  m_data_dir = work_dir;

  // Check if we have loaded the data_file
  if( !isUserFileLoaded() && !loadUserFile() )
  {
    showInformationBox("Error loading user file '" + m_uiForm.userfile_edit->text() + "',  cannot continue.");
    return;
  }
  
  //A load command for each box if there is anything in it and it has not already been loaded
  QMapIterator<int, QLineEdit*> itr(m_run_no_boxes);
  bool data_loaded(false);
  while( itr.hasNext() )
  {
    itr.next();
    QString run_no = itr.value()->text();
    if( run_no.isEmpty() ) continue;

    QString ws_name;
    if( itr.key() < 3 ) ws_name = run_no + "_sans";
    else ws_name = run_no + "_trans";
    
    if( workspaceExists(ws_name) ) continue;
    //Check for the correct number of digits
    QString filepath = getRawFilePath(work_dir, run_no);
    if( filepath.isEmpty() ) continue;
    //Load the file
    runPythonCode(writeLoadRawCmd(filepath, ws_name), true);
    data_loaded = true;
  }

  if( !data_loaded ) 
  {
    showInformationBox("Warning: No data could be loaded for " + m_uiForm.inst_opt->currentText() + 
		       " with a " + m_uiForm.file_opt->itemData(m_uiForm.file_opt->currentIndex()).toString() +
		       " file extension.");
    return;
  }

  //We need to sort out the number of periods in each data set
  QString code = "wksp_dict = {}\n"
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

  QString results = runPythonCode(code);
  if( results.isEmpty() ) return;
  
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

  for( int index = 1; index < m_uiForm.tabWidget->count(); ++index )
  {
    m_uiForm.tabWidget->setTabEnabled(index, true);
  }

  QString wsname = m_uiForm.sct_sample_edit->text() + "_sans";
  if( m_uiForm.sct_smp_prd->text() != "1" ) wsname += "_" + m_uiForm.sct_smp_prd->text();
    // Set up distance information
  double dist_ms_smp(0.0), dist_sd1_smp(0.0), dist_sd2_smp(0.0);
  componentDistances(wsname, dist_ms_smp, dist_sd1_smp, dist_sd2_smp);
  const char format('f');
  const int prec(4);
  m_uiForm.dist_sample_ms->setText(QString::number(dist_ms_smp, format, prec));
  m_uiForm.dist_sample_sd1->setText(QString::number(dist_sd1_smp, format, prec));
  m_uiForm.dist_sample_sd2->setText(QString::number(dist_sd2_smp, format, prec));
  
  wsname = m_uiForm.sct_can_edit->text() + "_sans";
  if( m_uiForm.sct_can_prd->text() != "1" ) wsname += "_" + m_uiForm.sct_can_prd->text();

  double dist_ms_can(0.0), dist_sd1_can(0.0), dist_sd2_can(0.0);
  componentDistances(wsname, dist_ms_can, dist_sd1_can, dist_sd2_can);
  
  m_uiForm.dist_can_ms->setText(QString::number(dist_ms_can, format, prec));
  m_uiForm.dist_can_sd1->setText(QString::number(dist_sd1_can, format, prec));
  m_uiForm.dist_can_sd2->setText(QString::number(dist_sd2_can, format, prec));
  bool warn_user(false);  
  if( dist_ms_can > 0.0 && abs(dist_ms_can - dist_ms_smp) > 5e-3 )
  {
    warn_user = true;
    m_uiForm.dist_sample_ms->setText("<font color='red'>" + m_uiForm.dist_sample_ms->text() + "</font>");
    m_uiForm.dist_can_ms->setText("<font color='red'>" + m_uiForm.dist_can_ms->text() + "</font>");
  }
  if( dist_sd1_can > 0.0  && abs(dist_sd1_can - dist_sd1_smp) > 5e-3 )
  {
    warn_user = true;
    m_uiForm.dist_sample_sd1->setText("<font color='red'>" + m_uiForm.dist_sample_sd1->text() + "</font>");
    m_uiForm.dist_can_sd1->setText("<font color='red'>" + m_uiForm.dist_can_sd1->text() + "</font>");
  }
  if( dist_sd2_can > 0.0 && abs(dist_sd2_can - dist_sd2_smp) > 5e-3 )
  {
    warn_user = true;
    m_uiForm.dist_sample_sd2->setText("<font color='red'>" + m_uiForm.dist_sample_sd2->text() + "</font>");
    m_uiForm.dist_can_sd2->setText("<font color='red'>" + m_uiForm.dist_can_sd2->text() + "</font>");
  }
  
  wsname = m_uiForm.sct_bkgd_edit->text() + "_sans";
  if( m_uiForm.sct_bkgd_prd->text() != "1" ) wsname += "_" + m_uiForm.sct_bkgd_prd->text();

  double dist_ms_bckd(0.0), dist_sd1_bckd(0.0), dist_sd2_bckd(0.0);
  componentDistances(wsname, dist_ms_bckd, dist_sd1_bckd, dist_sd2_bckd);
  m_uiForm.dist_bkgd_ms->setText(QString::number(dist_ms_bckd, format, prec));
  m_uiForm.dist_bkgd_sd1->setText(QString::number(dist_sd1_bckd, format, prec));
  m_uiForm.dist_bkgd_sd2->setText(QString::number(dist_sd2_bckd, format, prec));

  if( warn_user )
  {
    showInformationBox("Warning: Some component distances are inconsistent for the sample and can/background runs.\nSee the Geometry tab for details");
  } 
  //We can now process some data
  emit dataReadyToProcess(true);
}

/**
 * Run the LOQ analysis script
 * @param type The data reduction type, 1D or 2D
 */
void SANSRunWindow::handleReduceButtonClick(const QString & type)
{
  if( !readPyReductionTemplate() ) return;

  if( m_ins_defdir.isEmpty() ) m_ins_defdir = m_data_dir;

  QStringList wslist(m_uiForm.sct_sample_edit->text() + "_sans");
  wslist << m_uiForm.sct_can_edit->text() + "_sans" <<  m_uiForm.tra_sample_edit->text() + "_trans"
	 << m_uiForm.tra_can_edit->text() + "_trans" << m_uiForm.direct_sample_edit->text() + "_trans";
  QStringListIterator itr(wslist);
  while( itr.hasNext() )
  {
    // Quick check that the workspaces we need actually exist
    QString testws = itr.next();
    if( !workspaceExists(testws) )
    {
      showInformationBox("Error: " + testws + " does not exist. Please check that the relevant data has been loaded.");
      return;
    }
  }

  int idtype(0);
  if( type.startsWith("2") ) idtype = 1;

  //Disable buttons so that interaction is limited while processing data
  setProcessingState(true, idtype);
  
  //Construct the code to execute
  QString py_code = m_pycode_loqreduce;
  py_code.replace("|INSTRUMENTPATH|", m_ins_defdir);
  py_code.replace("|SCATTERSAMPLE|", wslist.at(0));
  py_code.replace("|SCATTERCAN|", wslist.at(1));
  py_code.replace("|TRANSMISSIONSAMPLE|", wslist.at(2));
  py_code.replace("|TRANSMISSIONCAN|", wslist.at(3));
  py_code.replace("|DIRECTSAMPLE|", wslist.at(4));

  //Limit replacement
  py_code.replace("|RADIUSMIN|", m_uiForm.rad_min->text());
  py_code.replace("|RADIUSMAX|", m_uiForm.rad_max->text());
  py_code.replace("|XBEAM|", m_uiForm.beam_x->text());
  py_code.replace("|YBEAM|", m_uiForm.beam_y->text());
  py_code.replace("|WAVMIN|", m_uiForm.wav_min->text());
  py_code.replace("|WAVMAX|", m_uiForm.wav_max->text());
  //Need to check for linear/log steps. If log then need to prepend a '-' to 
  //the front so that the Rebin algorithm recognises this
  QString step_prefix("");
  if( m_uiForm.wav_dw_opt->currentIndex() == 1 ) 
  {
    step_prefix = "-";
  }
  py_code.replace("|WAVDELTA|", step_prefix + m_uiForm.wav_dw->text());
    
  step_prefix = "";
  if( idtype == 0 )
  {
    if( m_uiForm.q_dq_opt->currentIndex() == 1 ) step_prefix = "-";
    py_code.replace("|QMIN|", m_uiForm.q_min->text());
    py_code.replace("|QMAX|", m_uiForm.q_max->text());
    py_code.replace("|QDELTA|", step_prefix  + m_uiForm.q_dq->text());
    py_code.replace("|QXYMAX|", "0");
    py_code.replace("|QXYDELTA|", "0");
  }
  else
  {
    if( m_uiForm.qy_dqy_opt->currentIndex() == 1 ) step_prefix = "-";
    py_code.replace("|QMIN|", "0");
    py_code.replace("|QMAX|", "0");
    py_code.replace("|QDELTA|", "0");
    py_code.replace("|QXYMAX|", m_uiForm.qy_max->text());
    py_code.replace("|QXYDELTA|", step_prefix  + m_uiForm.qy_dqy->text());
  }
  py_code.replace("|DIRECTFILE|", m_uiForm.direct_file->text());
  py_code.replace("|FLATFILE|", m_uiForm.flat_file->text());
  
  py_code.replace("|SCALEFACTOR|", m_uiForm.scale_factor->text());
  py_code.replace("|MASKSTRING|", createMaskString());
  py_code.replace("|ANALYSISTYPE|", type);
  
  //Execute the code
  runPythonCode(py_code);
  //Reenable stuff
  setProcessingState(false, idtype);
}

/**
 * Plot button slot
 */
void SANSRunWindow::handlePlotButtonClick()
{
  SANSPlotDialog dialog(this);
  dialog.setAvailableData(currentWorkspaceList());
  connect(&dialog, SIGNAL(pythonCodeConstructed(const QString&)), this, SIGNAL(runAsPythonScript(const QString&)));
  dialog.exec();
}

/**
 * A ComboBox option change
 * @param new_index The new index that has been set
 */
void SANSRunWindow::handleStepComboChange(int new_index)
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

/**
 * Called when the show mask button has been clicked
 */
void SANSRunWindow::handleShowMaskButtonClick()
{
  if( !readPyViewMaskTemplate() ) return;

  QString py_code = m_pycode_viewmask;
  py_code.replace("|INSTRUMENTPATH|", m_ins_defdir);
  //Shape mask defaults
  py_code.replace("|RADIUSMIN|", m_uiForm.rad_min->text());
  py_code.replace("|RADIUSMAX|", m_uiForm.rad_max->text());

  //Other masks
  py_code.replace("|MASKLIST|", createMaskString());
  runPythonCode(py_code);
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
