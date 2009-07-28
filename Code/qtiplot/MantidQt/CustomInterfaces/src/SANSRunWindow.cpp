//----------------------
// Includes
//----------------------
#include "MantidQtCustomInterfaces/SANSRunWindow.h"
#include "MantidQtCustomInterfaces/SANSUtilityDialogs.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/IInstrument.h"
#include "MantidAPI/SpectraDetectorMap.h"
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

// Initialize the logger
Mantid::Kernel::Logger& SANSRunWindow::g_log = Mantid::Kernel::Logger::get("SANSRunWindow");

//----------------------
// Public member functions
//----------------------
///Constructor
SANSRunWindow::SANSRunWindow(QWidget *parent) :
  UserSubWindow(parent), m_data_dir(""), m_ins_defdir(""), m_last_dir(""), m_cfg_loaded(false), m_run_no_boxes(), 
  m_period_lbls(), m_pycode_loqreduce(""), m_pycode_viewmask(""), m_run_changed(false), 
  m_delete_observer(*this,&SANSRunWindow::handleMantidDeleteWorkspace)
{
  m_reducemapper = new QSignalMapper(this);
  Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_delete_observer);
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
    g_log.debug("Initializing interface layout");
    m_uiForm.setupUi(this);

    //Button connections
    connect(m_uiForm.data_dirBtn, SIGNAL(clicked()), this, SLOT(selectDataDir()));
    connect(m_uiForm.userfileBtn, SIGNAL(clicked()), this, SLOT(selectUserFile()));

    connect(m_uiForm.load_dataBtn, SIGNAL(clicked()), this, SLOT(handleLoadButtonClick()));
    connect(m_uiForm.plotBtn, SIGNAL(clicked()), this, SLOT(handlePlotButtonClick()));
 
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

    //Connect each box's edited signal to flag if the box's text has changed
    for( int idx = 0; idx < 9; ++idx )
    {
      connect(m_run_no_boxes.value(idx), SIGNAL(textEdited(const QString&)), this, SLOT(runBoxEdited()));
    }

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

    // Full workspace names as they appear in the service
    m_workspace_names.clear();

    // Combo boxes
    connect(m_uiForm.wav_dw_opt, SIGNAL(currentIndexChanged(int)), this, 
	    SLOT(handleStepComboChange(int)));
    connect(m_uiForm.q_dq_opt, SIGNAL(currentIndexChanged(int)), this, 
	    SLOT(handleStepComboChange(int)));
    connect(m_uiForm.qy_dqy_opt, SIGNAL(currentIndexChanged(int)), this, 
	    SLOT(handleStepComboChange(int)));

    connect(m_uiForm.inst_opt, SIGNAL(currentIndexChanged(int)), this, 
	    SLOT(handleInstrumentChange(int)));

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
  g_log.debug("Reading settings.");
  QSettings value_store;
  value_store.beginGroup("CustomInterfaces/SANSRunWindow");
  m_uiForm.datadir_edit->setText(value_store.value("data_dir").toString());
  m_uiForm.userfile_edit->setText(value_store.value("user_file").toString());
  
  m_uiForm.inst_opt->setCurrentIndex(value_store.value("instrument", 0).toInt());
  m_uiForm.file_opt->setCurrentIndex(value_store.value("fileextension", 0).toInt());
  value_store.endGroup();

  //The instrument definition directory
  m_ins_defdir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("instrumentDefinition.directory"));
  
  g_log.debug() << "Found previous data directory " << m_uiForm.datadir_edit->text().toStdString()
    << "\nFound previous user mask file" << m_uiForm.userfile_edit->text().toStdString() 
    << "\nFound instrument definition directory " << m_ins_defdir.toStdString() << std::endl;
}

/**
 * Save input for future use
 */
void SANSRunWindow::saveSettings()
{
  QSettings value_store;
  value_store.beginGroup("CustomInterfaces/SANSRunWindow");
  if( !m_data_dir.isEmpty() ) 
  {
    value_store.setValue("data_dir", m_data_dir);
  }
  if( !m_uiForm.userfile_edit->text().isEmpty() ) 
  {
    value_store.setValue("user_file", m_uiForm.userfile_edit->text());
  }
  value_store.setValue("instrument", m_uiForm.inst_opt->currentIndex());
  value_store.setValue("fileextension", m_uiForm.file_opt->currentIndex());
  
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
  QString reduce_script = scriptsdir.absoluteFilePath("SANS/SANSReduction.py");
    
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
  
  //Clear the def masking info table.
  int mask_table_count = m_uiForm.mask_table->rowCount();
  for( int i = mask_table_count - 1; i >= 0; --i )
  {
    m_uiForm.mask_table->removeRow(i);
  }

  //Set a couple of things to default values that will get overwritten if present in the file
  if( m_uiForm.inst_opt->currentIndex() == 0 ) 
  {
    m_uiForm.monitor_spec->setText("2");
    m_uiForm.bank_spec_min->setText("3");
    m_uiForm.bank_spec_max->setText("16386"); 
  }
  else
  { 
    m_uiForm.monitor_spec->setText("73730");
    m_uiForm.bank_spec_min->setText("36865");
    m_uiForm.bank_spec_max->setText("73728");
  }

  m_uiForm.dist_mod_mon->setText("0.0000");
  m_uiForm.smpl_offset->setText("0.0");

  QTextStream stream(&user_file);
  QString data;
  while( !stream.atEnd() )
  {
    QString com_line = stream.readLine();
    //Skip comments lines
    if( com_line.startsWith("!") ) continue;
    
    if( com_line.startsWith("L/") )
    {
      readLimits(com_line.section("/", 1));
    }
    else if( com_line.startsWith("MON") )
    {
      //Line has the form MON/FIELD=...
      QString field = com_line.section("/", 1).section("=", 0, 0);
      if( field.compare("length", Qt::CaseInsensitive) == 0 )
      {
        QStringList line_items = com_line.section('=',1).split(' ');
        if( line_items.count() == 2 )
        {
          m_uiForm.dist_mod_mon->setText(line_items[0]);
          m_uiForm.monitor_spec->setText(line_items[1]);
        }
      }
      else
      {
        QString filepath;
        if( com_line.contains(']') ) filepath = QFileInfo(filetext).absoluteDir().absoluteFilePath(com_line.section("]", 1));
        else filepath = com_line.section('=',1);

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
      if( type.startsWith('s', Qt::CaseInsensitive) )
      {
	col1_txt = "Spectrum";
	col2_txt = type;//.section('S', 1, -1, QString::SectionCaseInsensitiveSeps);
      }
      else if( type.startsWith('h', Qt::CaseInsensitive) )
      {
	col1_txt = "Strip";
	col2_txt = type;
      }
      else continue;
      
      int row = m_uiForm.mask_table->rowCount();
      //Insert line after last row
      m_uiForm.mask_table->insertRow(row);
      QTableWidgetItem *item1 = new QTableWidgetItem(col1_txt);
      QTableWidgetItem *item2 = new QTableWidgetItem(col2_txt);
      m_uiForm.mask_table->setItem(row, 0, item1);
      m_uiForm.mask_table->setItem(row, 1, item2);
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
  //  m_uiForm.tabWidget->setTabEnabled(1, true);
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
    int row = m_uiForm.mask_table->rowCount();
    //Insert line after last row
    m_uiForm.mask_table->insertRow(row);
    QTableWidgetItem *item1 = new QTableWidgetItem("Beam stop");
    QTableWidgetItem *item2 = new QTableWidgetItem("infinite-cylinder");
    m_uiForm.mask_table->setItem(row, 0, item1);
    m_uiForm.mask_table->setItem(row, 1, item2);
    m_uiForm.mask_table->insertRow(++row);
    item1 = new QTableWidgetItem("Corners");
    item2 = new QTableWidgetItem("infinite-cylinder");
    m_uiForm.mask_table->setItem(row, 0, item1);
    m_uiForm.mask_table->setItem(row, 1, item2);
  }
  else if( quantity == "SP" )
  {
    m_uiForm.all_spec_min->setText(min);
    m_uiForm.all_spec_max->setText(max);
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
 * @param lmm The moderator-monitor distance
 */
void SANSRunWindow::componentDistances(const QString & wsname, double & lms, double & lsda, double & lsdb, double & lmm)
{
  if( !workspaceExists(wsname) ) return;
  Mantid::API::MatrixWorkspace_sptr workspace_ptr = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
    (Mantid::API::AnalysisDataService::Instance().retrieve(wsname.toStdString()));

  Mantid::API::IInstrument_sptr instr = workspace_ptr->getInstrument();
  if( instr == boost::shared_ptr<Mantid::API::IInstrument>() ) return;

  Mantid::Geometry::IObjComponent_sptr source = instr->getSource();
  if( source == boost::shared_ptr<Mantid::Geometry::IObjComponent>() ) return;
  Mantid::Geometry::IObjComponent_sptr sample = instr->getSample();
  if( sample == boost::shared_ptr<Mantid::Geometry::IObjComponent>() ) return;

  lms = source->getPos().distance(sample->getPos());
   
  //Find the main detector bank
  std::string comp_name("main-detector-bank");
  bool isS2D(false);
  if( m_uiForm.inst_opt->currentIndex() == 1 ) 
  {
    isS2D = true;
    comp_name = "rear-detector";
  }
  boost::shared_ptr<Mantid::Geometry::IComponent> comp = instr->getComponentByName(comp_name);
  if( comp != boost::shared_ptr<Mantid::Geometry::IComponent>() )
  {
    lsda = sample->getPos().distance(comp->getPos());
  }

  comp_name = "HAB";
  if( isS2D ) comp_name = "front-detector";
  comp = instr->getComponentByName(comp_name);
  if( comp != boost::shared_ptr<Mantid::Geometry::IComponent>() )
  {
    lsdb = sample->getPos().distance(comp->getPos());
  }
  if( lmm < 0.0 ) return;

  int monitor_spectrum = m_uiForm.monitor_spec->text().toInt();
  std::vector<int> dets = workspace_ptr->spectraMap().getDetectors(monitor_spectrum);
  if( dets.empty() ) return;
  Mantid::Geometry::IDetector_sptr detector = instr->getDetector(dets[0]);
  lmm = detector->getDistance(*source);

//   //  dets = workspace_ptr->spectraMap().getDetectors(m_uiForm.bank_spec_min->text().toInt());
//   g_log.debug() << "main-detector  pos " << instr->getComponentByName("rear-detector")->getPos() << "\n";
  

//   dets = workspace_ptr->spectraMap().getDetectors(36865);
//   if( !dets.empty() ) 
//   {
//     g_log.debug() << "Spectrum 3 pos " << instr->getDetector(dets[0])->getPos() << "\n";
//   }
//  //   dets = workspace_ptr->spectraMap().getDetectors();
// //   if( !dets.empty() ) 
// //   {
// //     g_log.debug() << "Spectrum 8130 pos " << instr->getDetector(dets[0])->getPos() << "\n";
// //   }

// //   dets = workspace_ptr->spectraMap().getDetectors(8131);
// //   if( !dets.empty() ) 
// //   {
// //     g_log.debug() << "Spectrum 8131 pos " << instr->getDetector(dets[0])->getPos() << "\n";
// //   }
//   dets = workspace_ptr->spectraMap().getDetectors(73728);
//   if( !dets.empty() ) 
//   {
//     g_log.debug() << "Spectrum 16386 pos " << instr->getDetector(dets[0])->getPos() << "\n";
//   }
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
 * @param data_dir The data directory
 * @param run_no The run number to search for
 * @param ext The file extension
 */
QString SANSRunWindow::getRawFilePath(const QString & data_dir, const QString & run_no, const QString & ext) const
{
  //Do a quick check for the existence of the file with these exact credentials
  QDir directory(data_dir);
  QString prefix = m_uiForm.inst_opt->currentText();
  QString filename = directory.absoluteFilePath(prefix + run_no + ext);
  g_log.debug("Checking for run " + run_no.toStdString());
  if( QFileInfo(filename).exists() ) return filename;

  // If nothing pad the number and check
  QString padded_no = run_no.rightJustified(8, '0', true);
  filename = directory.absoluteFilePath(prefix + padded_no + ext);
  g_log.debug("Not found. Checking padded name " + filename.toStdString());
  if( QFileInfo(filename).exists() ) return filename;
  else return QString();
}
 
/**
 * Create the a comma separated list of masking values using the masking information from the Mask tab
 */
QString SANSRunWindow::createMaskString() const
{
  QString maskstring;
  int nrows = m_uiForm.mask_table->rowCount();
  for( int r = 0; r < nrows; ++r )
  {
    QString type = m_uiForm.mask_table->item(r, 1)->text();
    if( type == "infinite-cylinder" ) continue;
    
    maskstring += m_uiForm.mask_table->item(r, 1)->text() + ",";
  }
  maskstring += m_uiForm.user_maskEdit->text();
  return maskstring;
}

void SANSRunWindow::setupGeometryDetails()
{
  QString wsname = m_workspace_names.value(0);
  if( m_uiForm.sct_smp_prd->text() != "1" ) wsname += "_" + m_uiForm.sct_smp_prd->text();
    // Set up distance information
  double dist_ms_smp(0.0), dist_sd1_smp(0.0), dist_sd2_smp(0.0), dist_mm(-1.0);
  if( m_uiForm.dist_mod_mon->text() == "0.0000" ) dist_mm = 0.0;
  componentDistances(wsname, dist_ms_smp, dist_sd1_smp, dist_sd2_smp, dist_mm);
  const char format('f');
  const int prec(4);
  m_uiForm.dist_sample_ms->setText(QString::number(dist_ms_smp, format, prec));
  m_uiForm.dist_sample_sd1->setText(QString::number(dist_sd1_smp, format, prec));
  m_uiForm.dist_sample_sd2->setText(QString::number(dist_sd2_smp, format, prec));

  if( dist_mm > 0.0 ) m_uiForm.dist_mod_mon->setText(QString::number(dist_mm, format, prec));
  
  wsname = m_workspace_names.value(1);
  if( m_uiForm.sct_can_prd->text() != "1" ) wsname += "_" + m_uiForm.sct_can_prd->text();

  double dist_ms_can(0.0), dist_sd1_can(0.0), dist_sd2_can(0.0);
  // We only need the moderator-monitor from the sample so -1.0 flags not to calculate it
  dist_mm = -1.0;
  componentDistances(wsname, dist_ms_can, dist_sd1_can, dist_sd2_can, dist_mm);
  
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
  
  wsname = m_workspace_names.value(2);
  if( m_uiForm.sct_bkgd_prd->text() != "1" ) wsname += "_" + m_uiForm.sct_bkgd_prd->text();

  double dist_ms_bckd(0.0), dist_sd1_bckd(0.0), dist_sd2_bckd(0.0);
  componentDistances(wsname, dist_ms_bckd, dist_sd1_bckd, dist_sd2_bckd, dist_mm);
  m_uiForm.dist_bkgd_ms->setText(QString::number(dist_ms_bckd, format, prec));
  m_uiForm.dist_bkgd_sd1->setText(QString::number(dist_sd1_bckd, format, prec));
  m_uiForm.dist_bkgd_sd2->setText(QString::number(dist_sd2_bckd, format, prec));

  if( warn_user )
  {
    showInformationBox("Warning: Some component distances are inconsistent for the sample and can/background runs.\nSee the Geometry tab for details.");
  }
  return;
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
  if( !isUserFileLoaded() )
  {
    showInformationBox("Please load the relevant user file.");
    return;
  }
  
  if( !loadUserFile() )
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
    int key = itr.key();
    QString run_no = itr.value()->text();
    if( run_no.isEmpty() ) 
    {
      m_workspace_names.insert(key, "");
      continue;
    }
    //Construct a workspace name that will go into the ADS
    QString ws_name;
    if( key < 3 ) ws_name = run_no + "_sans_" + m_uiForm.file_opt->currentText();
    else ws_name = run_no + "_trans_" + m_uiForm.file_opt->currentText();
    //Check if we already have it and do nothing if that is so
    if( workspaceExists(ws_name) ) 
    {
      data_loaded = true;
      continue;
    }
    //Load the file. This checks for required padding of zeros etc
    int n_periods = runLoadData(work_dir, run_no, 
				m_uiForm.file_opt->itemData(m_uiForm.file_opt->currentIndex()).toString(), ws_name);
    // If this is zero then something went wrong with trying to load a file
    if( n_periods == 0 ) 
    {
      m_workspace_names.insert(key, "");
      showInformationBox("Error: Cannot load run " + run_no 
			 + ".\nPlease check that the correct instrument and file extension are selected");
      //Bail out completely now
      return;
    }

    //At this point we know the workspace exists
    m_workspace_names.insert(key, ws_name);
    if( key == 0 )
    {
      Mantid::API::MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
        (Mantid::API::AnalysisDataService::Instance().retrieve(ws_name.toStdString()));
        if( ws != boost::shared_ptr<Mantid::API::MatrixWorkspace>() && !ws->readX(0).empty() )
        {
          m_uiForm.tof_min->setText(QString::number(ws->readX(0).front())); 
          m_uiForm.tof_max->setText(QString::number(ws->readX(0).back()));
        }
    }

    QLabel *label = qobject_cast<QLabel*>(m_period_lbls.value(key));
    label->setText("/ " + QString::number(n_periods));
    QLineEdit *userentry = qobject_cast<QLineEdit*>(label->buddy());
    userentry->setText("1");

    data_loaded = true;
  }

  // Cannot do anything if nothing was loaded
  if( !data_loaded ) 
  {
    return;
  }
  m_run_changed = false;

  //Fill in the information on the geometry tab
  setupGeometryDetails();

  for( int index = 1; index < m_uiForm.tabWidget->count(); ++index )
  {
    m_uiForm.tabWidget->setTabEnabled(index, true);
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
  // Quick check that scattering sample number has been entered
  if( m_uiForm.sct_sample_edit->text().isEmpty() )
  {
    showInformationBox("Error: A scattering sample run number is required to continue.");
    return;
  } 

  if( m_run_changed )
  {
    g_log.debug("A run number has changed, running load routine.");
    handleLoadButtonClick();
  }

  int idtype(0);
  if( type.startsWith("2") ) idtype = 1;

  //Disable buttons so that interaction is limited while processing data
  setProcessingState(true, idtype);
  //Construct the code to execute
  QString py_code = m_pycode_loqreduce;
  py_code.replace("|INSTRUMENTPATH|", m_ins_defdir);
  py_code.replace("|INSTRUMENTNAME|", m_uiForm.inst_opt->currentText());

  //  py_code.replace("|SPECMIN|", m_uiForm.bank_spec_min->text());
  //  py_code.replace("|SPECMAX|", m_uiForm.bank_spec_max->text());

  py_code.replace("|SCATTERSAMPLE|", m_workspace_names.value(0));
  py_code.replace("|SCATTERCAN|", m_workspace_names.value(1));
  py_code.replace("|TRANSMISSIONSAMPLE|", m_workspace_names.value(3));
  py_code.replace("|TRANSMISSIONCAN|", m_workspace_names.value(4));
  py_code.replace("|DIRECTSAMPLE|", m_workspace_names.value(6));

  // Limit replacement
  QString radius = m_uiForm.rad_min->text();
  if( radius.isEmpty() ) radius = "-1.0";
  py_code.replace("|RADIUSMIN|", radius);

  radius = m_uiForm.rad_max->text();
  if( radius.isEmpty() ) radius = "-1.0";
  py_code.replace("|RADIUSMAX|", radius);

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
  py_code.replace("|SAMPLEZOFFSET|", m_uiForm.smpl_offset->text());
  py_code.replace("|FLATFILE|", m_uiForm.flat_file->text());
  
  py_code.replace("|SCALEFACTOR|", m_uiForm.scale_factor->text());
  py_code.replace("|MASKSTRING|", createMaskString());
  py_code.replace("|ANALYSISTYPE|", type);
  py_code.replace("|MONSPEC|", m_uiForm.monitor_spec->text());
  
  QString backmonstart(""), backmonend("");
  if( m_uiForm.inst_opt->currentText().startsWith("l", Qt::CaseInsensitive) )
  {
    backmonstart = "31000";
    backmonend = "39000";
  }
  else
  {
    backmonstart = "85000";
    backmonend = "100000";
  }

  py_code.replace("|BACKMONSTART|", backmonstart);
  py_code.replace("|BACKMONEND|", backmonend);

  py_code.replace("|DETBANK|", "front-detector");

  //  std::cerr << py_code.toStdString() << "\n";

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
  py_code.replace("|INSTRUMENTNAME|", m_uiForm.inst_opt->currentText());

  // Shape mask if applicable
  QString radius = m_uiForm.rad_min->text();
  if( radius.isEmpty() ) radius = "-1.0";
  g_log.debug("Min radius " + radius.toStdString());
  py_code.replace("|RADIUSMIN|", radius);

  radius = m_uiForm.rad_max->text();
  if( radius.isEmpty() ) radius = "-1.0";
  g_log.debug("Max radius " + radius.toStdString());
  py_code.replace("|RADIUSMAX|", radius);
  
  //Other masks
  py_code.replace("|SPECMIN|", m_uiForm.bank_spec_min->text());
  py_code.replace("|MASKLIST|", createMaskString());
  runPythonCode(py_code);
}

/**
 * Flip the flag saying that something has been edited in one of the run boxes
 */
void SANSRunWindow::runBoxEdited()
{
  m_run_changed = true;
}

/**
 * A different instrument has been selected
 */
void SANSRunWindow::handleInstrumentChange(int index)
{
  if( index == 0 ) 
  {
    m_uiForm.monitor_spec->setText("2");
    m_uiForm.bank_spec_min->setText("3");
    m_uiForm.bank_spec_max->setText("16386"); 
  }
  else
  { 
    m_uiForm.monitor_spec->setText("73730");
    m_uiForm.bank_spec_min->setText("1");
    m_uiForm.bank_spec_max->setText("36864");
  }
  m_cfg_loaded = false;
}

/**
 * Run the appropriate command to load the data
 * @param work_dir The directory
 * @param run_no The run number
 * @param ext The file extension
 * @param workspace The OutputWorkspace
 * @returns The number of periods in the workspace. Returns zero on failure
 */
int SANSRunWindow::runLoadData(const QString & work_dir, const QString & run_no, const QString & ext, const QString & workspace)  
{
  QString filepath = getRawFilePath(work_dir, run_no, ext);
  if( filepath.isEmpty() )
  {   
    return 0;
  }
  g_log.debug("Attempting to load " + filepath.toStdString());

  Mantid::API::FrameworkManagerImpl & f_mgr = Mantid::API::FrameworkManager::Instance();
  Mantid::API::IAlgorithm *loader(NULL);
  if( ext == ".raw" )
  {
    loader = f_mgr.createAlgorithm("LoadRaw");
  }
  else
  {
    loader = f_mgr.createAlgorithm("LoadISISNexus"); 
  }
  loader->setPropertyValue("Filename", filepath.toStdString());
  std::string workspace_name = workspace.toStdString();
  loader->setPropertyValue("OutputWorkspace", workspace_name);
  if( !loader->execute() )
  {
    return 0;
  }
  else
  {
    g_log.debug("Loading algorithm succeeded.");
    //Load succeeded so find the number of periods. (Here the number of workspaces in the group)
    //Retrieve shoudn't throw but lets wrap it just in case
     Mantid::API::Workspace_sptr wksp_ptr;
    try
    {
     wksp_ptr = Mantid::API::AnalysisDataService::Instance().retrieve(workspace_name);
    }
    catch(Mantid::Kernel::Exception::NotFoundError &)
    {
      g_log.error("Couldn't find workspace " + workspace_name + " in ADS.");
      return 0;
    }
    Mantid::API::WorkspaceGroup_sptr ws_group = boost::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(wksp_ptr);
    if( ws_group )
    {
      return ws_group->getNames().size();
    }
    else
    {
      return 1;
    }
  }
}

/**
 * Handle a delete notification from Mantid
 * @param p_dnf A Mantid delete notification
 */
void SANSRunWindow::handleMantidDeleteWorkspace(Mantid::API::WorkspaceDeleteNotification_ptr p_dnf)
{
  QString wksp_name = QString::fromStdString(p_dnf->object_name());
  QHashIterator<int, QString> itr(m_workspace_names);
  int names_count = m_workspace_names.count();
  for( int key = 0; key < names_count; ++key )
  {
    if( wksp_name == m_workspace_names.value(key) )
    {
      m_run_changed = true;
      return;
    }
  }
}
