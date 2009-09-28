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
  UserSubWindow(parent), m_data_dir(""), m_ins_defdir(""), m_last_dir(""), m_cfg_loaded(true), m_run_no_boxes(), 
  m_period_lbls(), m_pycode_loqreduce(""), m_pycode_viewmask(""), m_run_changed(false), m_force_reload(false),
  m_delete_observer(*this,&SANSRunWindow::handleMantidDeleteWorkspace),
  m_logvalues(), m_maskcorrections(), m_havescipy(true), m_lastreducetype(-1)
{
  m_reducemapper = new QSignalMapper(this);
  Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_delete_observer);
}

///Destructor
SANSRunWindow::~SANSRunWindow()
{
  // Seems to crash on destruction of if I don't do this 
  Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_delete_observer);
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
    connect(m_uiForm.runcentreBtn, SIGNAL(clicked()), this, SLOT(handleRunFindCentre()));
    connect(m_uiForm.saveBtn, SIGNAL(clicked()), this, SLOT(handleSaveButtonClick()));
 
    // Disable most things so that load is the only thing that can be done
    m_uiForm.oneDBtn->setEnabled(false);
    m_uiForm.twoDBtn->setEnabled(false);
    for( int i = 1; i < 4; ++i)
    {
      m_uiForm.tabWidget->setTabEnabled(i, false);
    }

    // Connect
    connect(m_uiForm.tabWidget, SIGNAL(currentChanged(int)), this, SLOT(handleTabChange(int)));

    // Reduction buttons
    connect(m_uiForm.oneDBtn, SIGNAL(clicked()), m_reducemapper, SLOT(map()));
    m_reducemapper->setMapping(m_uiForm.oneDBtn, "1D");
    connect(m_uiForm.twoDBtn, SIGNAL(clicked()), m_reducemapper, SLOT(map()));
    m_reducemapper->setMapping(m_uiForm.twoDBtn, "2D");
    connect(m_reducemapper, SIGNAL(mapped(const QString &)), this, SLOT(handleReduceButtonClick(const QString &)));
    
    connect(m_uiForm.showMaskBtn, SIGNAL(clicked()), this, SLOT(handleShowMaskButtonClick()));

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
      connect(m_run_no_boxes.value(idx), SIGNAL(textEdited(const QString&)), this, SLOT(forceDataReload()));
    }
    
    connect(m_uiForm.smpl_offset, SIGNAL(textEdited(const QString&)), this, SLOT(forceDataReload()));

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

  // Setup for instrument
  handleInstrumentChange(m_uiForm.inst_opt->currentIndex());
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
  QString mask_script = scriptsdir.absoluteFilePath("SANS/SANSViewMask.py");
    
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
  handleInstrumentChange(m_uiForm.inst_opt->currentIndex());

  m_uiForm.dist_mod_mon->setText("-");
  m_uiForm.smpl_offset->setText("0.0");

  //Setup mask file detector corrections
  m_maskcorrections.clear();
  m_maskcorrections["Front_Det_Z_corr"] = 0.0;
  m_maskcorrections["Front_Det_Y_corr"] = 0.0;
  m_maskcorrections["Front_Det_X_corr"] = 0.0;
  m_maskcorrections["Front_Det_Rot_corr"] = 0.0;
  m_maskcorrections["Rear_Det_Z_corr"] = 0.0;
  m_maskcorrections["Rear_Det_X_corr"] = 0.0;
 
  QDir work_dir = QDir(m_uiForm.datadir_edit->text());

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
        if( com_line.contains(']') ) filepath = com_line.section("]", 1);
        else filepath = com_line.section('=',1);

        //Check for relative or absolute path
        if( QFileInfo(filepath).isRelative() )
        {
          filepath = QFileInfo(user_file).absoluteDir().absoluteFilePath(filepath);
        }

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
	col2_txt = type;
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
    else if( com_line.startsWith("DET/CORR", Qt::CaseInsensitive) )
    {
      QString det = com_line.section(' ',1, 1);
      QString axis = com_line.section(' ',2, 2);
      double value = com_line.section(' ',3, 3).toDouble();
      QString key;


      if( det.compare("rear", Qt::CaseInsensitive) == 0 )
      {
	if( axis.compare("x", Qt::CaseInsensitive) == 0 )
	{
	  key = "Rear_Det_X_corr";
	}
	else
	{
	  key = "Rear_Det_Z_corr";
	}
      }
      else
      {
	if( axis.compare("x", Qt::CaseInsensitive) == 0 )
	{
	  key = "Front_Det_X_corr";
	}
	else if( axis.compare("y", Qt::CaseInsensitive) == 0 )
	{
	  key = "Front_Det_Y_corr";
	}
	else if( axis.compare("z", Qt::CaseInsensitive) == 0 )
	{
	  key = "Front_Det_Z_corr";
	}
	else
	{
	  key = "Front_Det_Rot_corr";
	}
      }
      m_maskcorrections[key] = value;
    }
    else if(com_line.startsWith("SAMPLE/OFFSET"))
    {
      QString txt = com_line.section(' ', 1, 1);
      m_uiForm.smpl_offset->setText(txt);
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

  lms = source->getPos().distance(sample->getPos()) * 1000.;
   
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
    lsda = sample->getPos().distance(comp->getPos()) * 1000.;
  }

  comp_name = "HAB";
  if( isS2D ) comp_name = "front-detector";
  comp = instr->getComponentByName(comp_name);
  if( comp != boost::shared_ptr<Mantid::Geometry::IComponent>() )
  {
    lsdb = sample->getPos().distance(comp->getPos()) * 1000.;
  }
  if( lmm < 0.0 ) return;

  int monitor_spectrum = m_uiForm.monitor_spec->text().toInt();
  std::vector<int> dets = workspace_ptr->spectraMap().getDetectors(monitor_spectrum);
  if( dets.empty() ) return;
  Mantid::Geometry::IDetector_sptr detector = instr->getDetector(dets[0]);
  lmm = detector->getDistance(*source) * 1000.;

}

/**
 * Set the state of processing.
 * @param running If we are processing then some interaction is disabled
 * @param type The reduction type, 0 = 1D and 1 = 2D
 */
void SANSRunWindow::setProcessingState(bool running, int type)
{
  m_uiForm.load_dataBtn->setEnabled(!running);
  m_uiForm.oneDBtn->setEnabled(!running);
  m_uiForm.twoDBtn->setEnabled(!running);
  m_uiForm.plotBtn->setEnabled(!running);
  m_uiForm.saveBtn->setEnabled(!running);

  if( m_havescipy )
  {
    m_uiForm.runcentreBtn->setEnabled(!running);
  }
  if( running )
  {
    if( type == 0 )
    {   
      m_uiForm.oneDBtn->setText("Running ...");
    }
    else if( type == 1 )
    {
      m_uiForm.twoDBtn->setText("Running ...");
    }
    else {}
  }
  else
  {
    m_uiForm.oneDBtn->setText("1D Reduce");
    m_uiForm.twoDBtn->setText("2D Reduce");
  }

  for( int i = 0; i < 4; ++i)
  {
    if( i == m_uiForm.tabWidget->currentIndex() ) continue;
    m_uiForm.tabWidget->setTabEnabled(i, !running);
  }

  QCoreApplication::processEvents();
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
  // Reset the geometry box
  resetGeometryDetailsBox();

  const char format('f');
  const int prec(3);
  bool warn_user(false);  
  

  // LOQ
  if( m_uiForm.inst_opt->currentIndex() == 0 )
  {
    QString wsname = m_workspace_names.value(0);
    if( m_uiForm.sct_smp_prd->text() != "1" ) wsname += "_" + m_uiForm.sct_smp_prd->text();

    // Set up distance information
    double dist_ms_smp(0.0), dist_sample_mdb(0.0), dist_smp_hab(0.0), dist_mm(-1.0);
    if( m_uiForm.dist_mod_mon->text() == "-" ) dist_mm = 0.0;
    componentDistances(wsname, dist_ms_smp, dist_sample_mdb, dist_smp_hab, dist_mm);
    m_uiForm.dist_sample_ms->setText(QString::number(dist_ms_smp, format, prec));
    m_uiForm.dist_smp_mdb->setText(QString::number(dist_sample_mdb, format, prec));
    m_uiForm.dist_smp_hab->setText(QString::number(dist_smp_hab, format, prec));

    if( dist_mm > 0.0 ) 
    {
      m_uiForm.dist_mod_mon->setText(QString::number(dist_mm, format, prec));
    }
    
    wsname = m_workspace_names.value(1);
    if( m_uiForm.sct_can_prd->text() != "1" ) wsname += "_" + m_uiForm.sct_can_prd->text();
    
    double dist_ms_can(0.0), dist_can_mdb(0.0), dist_sd2_can(0.0);
    // We only need the moderator-monitor from the sample so -1.0 flags not to calculate it
    dist_mm = -1.0;
    componentDistances(wsname, dist_ms_can, dist_can_mdb, dist_sd2_can, dist_mm);
  
    m_uiForm.dist_can_ms->setText(QString::number(dist_ms_can, format, prec));
    m_uiForm.dist_can_mdb->setText(QString::number(dist_can_mdb, format, prec));
    m_uiForm.dist_can_hab->setText(QString::number(dist_sd2_can, format, prec));

    if( dist_ms_can > 0.0 && abs(dist_ms_can - dist_ms_smp) > 5e-3 )
    {
      warn_user = true;
      m_uiForm.dist_sample_ms->setText("<font color='red'>" + m_uiForm.dist_sample_ms->text() + "</font>");
      m_uiForm.dist_can_ms->setText("<font color='red'>" + m_uiForm.dist_can_ms->text() + "</font>");
    }
    if( dist_can_mdb > 0.0  && abs(dist_can_mdb - dist_sample_mdb) > 5e-3 )
    {
      warn_user = true;
      m_uiForm.dist_smp_mdb->setText("<font color='red'>" + m_uiForm.dist_smp_mdb->text() + "</font>");
      m_uiForm.dist_can_mdb->setText("<font color='red'>" + m_uiForm.dist_can_mdb->text() + "</font>");
    }
    if( dist_sd2_can > 0.0 && abs(dist_sd2_can - dist_smp_hab) > 5e-3 )
    {
      warn_user = true;
      m_uiForm.dist_smp_hab->setText("<font color='red'>" + m_uiForm.dist_smp_hab->text() + "</font>");
      m_uiForm.dist_can_hab->setText("<font color='red'>" + m_uiForm.dist_can_hab->text() + "</font>");
    }
  
    wsname = m_workspace_names.value(2);
    if( m_uiForm.sct_bkgd_prd->text() != "1" ) wsname += "_" + m_uiForm.sct_bkgd_prd->text();
    
    double dist_ms_bckd(0.0), dist_sd1_bckd(0.0), dist_sd2_bckd(0.0);
    componentDistances(wsname, dist_ms_bckd, dist_sd1_bckd, dist_sd2_bckd, dist_mm);
    m_uiForm.dist_bkgd_ms->setText(QString::number(dist_ms_bckd, format, prec));
    m_uiForm.dist_bkgd_mdb->setText(QString::number(dist_sd1_bckd, format, prec));
    m_uiForm.dist_bkgd_hab->setText(QString::number(dist_sd2_bckd, format, prec));
  }
  //SANS2D
  else
  {
    QString wsname = m_workspace_names.value(0);
    if( m_uiForm.sct_smp_prd->text() != "1" ) wsname += "_" + m_uiForm.sct_smp_prd->text();
    double dummy(0.0), dist_ms_smp(0.0), dist_mm(-1.0);
    if( m_uiForm.dist_mon_s2d->text() == "-" ) dist_mm = 0.0;
    componentDistances(wsname, dist_ms_smp, dummy, dummy, dist_mm);
    m_uiForm.dist_sample_ms_s2d->setText(QString::number(dist_ms_smp, format, prec));

    if( dist_mm > 0.0 ) 
    {
      m_uiForm.dist_mon_s2d->setText(QString::number(dist_mm, format, prec));
    }


    //Sample run
    //rear X
    double smp_rearX = m_logvalues.value("Rear_Det_X") + m_maskcorrections.value("Rear_Det_X_corr");
    m_uiForm.dist_smp_rearX->setText(formatDouble(smp_rearX, format, prec, "black"));
    //rear Z
    double smp_rearZ  = m_logvalues.value("Rear_Det_Z") + m_maskcorrections.value("Rear_Det_Z_corr");
    m_uiForm.dist_smp_rearZ->setText(formatDouble(smp_rearZ, format, prec, "black"));
    //front X
    double smp_frontX = m_logvalues.value("Front_Det_X") + m_maskcorrections.value("Front_Det_X_corr");
    m_uiForm.dist_smp_frontX->setText(QString::number(smp_frontX, format, prec));
    //front Z
    double smp_frontZ = m_logvalues.value("Front_Det_Z") + m_maskcorrections.value("Front_Det_Z_corr");
    m_uiForm.dist_smp_frontZ->setText(QString::number(smp_frontZ, format, prec));
    //front rot
    double smp_rot = m_logvalues.value("Front_Det_Rot") + m_maskcorrections.value("Front_Det_Rot_corr");
    m_uiForm.smp_rot->setText(QString::number(smp_rot, format, prec));
    
    //Can
    wsname = m_workspace_names.value(1);
    if( !wsname.isEmpty() )
    {
      if( m_uiForm.sct_can_prd->text() != "1" ) wsname += "_" + m_uiForm.sct_can_prd->text();
      dist_ms_smp = 0.0;
      dummy = -1.0;
      componentDistances(wsname, dist_ms_smp, dummy, dummy, dummy);
      m_uiForm.dist_can_ms_s2d->setText(QString::number(dist_ms_smp, format, prec));

      //Get log values for this workspace
      QHash<QString, double> logs = loadDetectorLogs(QDir(m_uiForm.datadir_edit->text()).absolutePath(), m_uiForm.sct_can_edit->text());
      //rear X
      double can_rearX = logs.value("Rear_Det_X") + m_maskcorrections.value("Rear_Det_X_corr");
      //Check for differences above 5mm with sample
      if( std::fabs(smp_rearX - can_rearX) > 5e-3 )
      {
	warn_user = true;
	m_uiForm.dist_can_rearX->setText(formatDouble(can_rearX, format, prec, "red"));
	m_uiForm.dist_smp_rearX->setText(formatDouble(smp_rearX, format, prec, "red"));
      }
      else
      {
	m_uiForm.dist_can_rearX->setText(formatDouble(can_rearX, format, prec, "black"));
      }

      //rear Z
      double can_rearZ = logs.value("Rear_Det_Z") + m_maskcorrections.value("Rear_Det_Z_corr");
      if( std::fabs(smp_rearZ - can_rearZ) > 5e-3 )
      {
	warn_user = true;
	m_uiForm.dist_can_rearZ->setText(formatDouble(can_rearZ, format, prec, "red"));
	m_uiForm.dist_smp_rearZ->setText(formatDouble(smp_rearZ, format, prec, "red"));
      }
      else
      {
	m_uiForm.dist_can_rearZ->setText(formatDouble(can_rearZ, format, prec, "black"));
      }
      //front X
      double can_frontX = logs.value("Front_Det_X") + m_maskcorrections.value("Front_Det_X_corr");
      if( std::fabs(smp_frontX - can_frontX) > 5e-3 )
      {
	warn_user = true;
	m_uiForm.dist_can_frontX->setText(formatDouble(can_frontX, format, prec, "red"));
	m_uiForm.dist_smp_frontX->setText(formatDouble(smp_frontX, format, prec, "red"));
      }
      else
      {
	m_uiForm.dist_can_frontX->setText(formatDouble(can_frontX, format, prec, "black"));
      }
      //front Z
      double can_frontZ = logs.value("Front_Det_Z") + m_maskcorrections.value("Front_Det_Z_corr");
      if( std::fabs(smp_frontZ - can_frontZ) > 5e-3 )
      {
	warn_user = true;
	m_uiForm.dist_can_frontZ->setText(formatDouble(can_frontZ, format, prec, "red"));
	m_uiForm.dist_smp_frontZ->setText(formatDouble(smp_frontZ, format, prec, "red"));
      }
      else
      {
	m_uiForm.dist_can_frontZ->setText(formatDouble(can_frontZ, format, prec, "black"));
      }
      //front rot
      double can_rot = logs.value("Front_Det_Rot") + m_maskcorrections.value("Front_Det_Rot_corr");
      if( std::fabs(smp_rot - can_rot) > 5e-3 )
      {
	warn_user = true;
	m_uiForm.can_rot->setText(formatDouble(can_rot, format, prec, "red"));
	m_uiForm.smp_rot->setText(formatDouble(smp_rot, format, prec, "red"));
      }
      else
      {
	m_uiForm.can_rot->setText(formatDouble(can_rot, format, prec, "black"));
      }

    }
    // Background
    wsname = m_workspace_names.value(2);
    if( !wsname.isEmpty() )
    {
      if( m_uiForm.sct_bkgd_prd->text() != "1" ) wsname += "_" + m_uiForm.sct_bkgd_prd->text();
      dist_ms_smp = 0.0;
      componentDistances(wsname, dist_ms_smp, dummy, dummy, dummy);
      m_uiForm.dist_bkgd_ms_s2d->setText(QString::number(dist_ms_smp, format, prec));

      //Get log values for this workspace
      QHash<QString, double> logs = loadDetectorLogs(QDir(m_uiForm.datadir_edit->text()).absolutePath(), m_uiForm.sct_bkgd_edit->text());
      //rear X
      double bkgd_rearX = logs.value("Rear_Det_X") + m_maskcorrections.value("Rear_Det_X_corr");
      //Check for differences above 5mm with sample
      if( std::fabs(smp_rearX - bkgd_rearX) > 5e-3 )
      {
	warn_user = true;
	m_uiForm.dist_bkgd_rearX->setText(formatDouble(bkgd_rearX, format, prec, "red"));
	m_uiForm.dist_smp_rearX->setText(formatDouble(smp_rearX, format, prec, "red"));
      }
      else
      {
	m_uiForm.dist_bkgd_rearX->setText(formatDouble(bkgd_rearX, format, prec, "black"));
      }

      //rear Z
      double bkgd_rearZ = logs.value("Rear_Det_Z") + m_maskcorrections.value("Rear_Det_Z_corr");
      if( std::fabs(smp_rearZ - bkgd_rearZ) > 5e-3 )
      {
	warn_user = true;
	m_uiForm.dist_bkgd_rearZ->setText(formatDouble(bkgd_rearZ, format, prec, "red"));
	m_uiForm.dist_smp_rearZ->setText(formatDouble(smp_rearZ, format, prec, "red"));
      }
      else
      {
	m_uiForm.dist_bkgd_rearZ->setText(formatDouble(bkgd_rearZ, format, prec, "black"));
      }
      //front X
      double bkgd_frontX = logs.value("Front_Det_X") + m_maskcorrections.value("Front_Det_X_corr");
      if( std::fabs(smp_frontX - bkgd_frontX) > 5e-3 )
      {
	warn_user = true;
	m_uiForm.dist_bkgd_frontX->setText(formatDouble(bkgd_frontX, format, prec, "red"));
	m_uiForm.dist_smp_frontX->setText(formatDouble(smp_frontX, format, prec, "red"));
      }
      else
      {
	m_uiForm.dist_bkgd_frontX->setText(formatDouble(bkgd_frontX, format, prec, "black"));
      }
      //front Z
      double bkgd_frontZ = logs.value("Front_Det_Z") + m_maskcorrections.value("Front_Det_Z_corr");
      if( std::fabs(smp_frontZ - bkgd_frontZ) > 5e-3 )
      {
	warn_user = true;
	m_uiForm.dist_bkgd_frontZ->setText(formatDouble(bkgd_frontZ, format, prec, "red"));
	m_uiForm.dist_smp_frontZ->setText(formatDouble(smp_frontZ, format, prec, "red"));
      }
      else
      {
	m_uiForm.dist_bkgd_frontZ->setText(formatDouble(bkgd_frontZ, format, prec, "black"));
      }
      //front rot
      double bkgd_rot = logs.value("Front_Det_Rot") + m_maskcorrections.value("Front_Det_Rot_corr");
      if( std::fabs(smp_rot - bkgd_rot) > 5e-3 )
      {
	warn_user = true;
	m_uiForm.bkgd_rot->setText(formatDouble(bkgd_rot, format, prec, "red"));
	m_uiForm.smp_rot->setText(formatDouble(smp_rot, format, prec, "red"));
      }
      else
      {
	m_uiForm.bkgd_rot->setText(formatDouble(bkgd_rot, format, prec, "black"));
      }

    }
  }

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
  
  if( !loadUserFile() )
  {
    m_cfg_loaded = false;
    showInformationBox("Error loading user file '" + m_uiForm.userfile_edit->text() + "',  cannot continue.");
    return;
  }
  m_cfg_loaded = true;
  m_uiForm.tabWidget->setTabEnabled(1, true);
  m_uiForm.tabWidget->setTabEnabled(2, true);
  m_uiForm.tabWidget->setTabEnabled(3, true);
  

  //path() returns the directory
  m_last_dir = QFileInfo(file_path).path();
}

/**
 * Flip the flag to confirm whether data is reloaded
 * @param force If true, the data is reloaded when reduce is clicked
 */
void SANSRunWindow::forceDataReload(bool force)
{
  m_force_reload = force;
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
  setProcessingState(true, -1);

  if( m_force_reload ) cleanup();

  //A load command for each box if there is anything in it and it has not already been loaded
  QMapIterator<int, QLineEdit*> itr(m_run_no_boxes);
  bool load_success(false);
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
    if( workspaceExists(ws_name) && !m_force_reload ) 
    {
      load_success = true;
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
      //Bail out completely now and make sure that future load will try to reload
      forceDataReload();
      setProcessingState(false, -1);
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

      // Load log information
      m_logvalues = loadDetectorLogs(work_dir, run_no);

      // Set the geometry
      int geomid  = ws->getSample()->getGeometryFlag();
      m_uiForm.sample_geomid->setCurrentIndex(geomid - 1);
      double thick(0.0), width(0.0), height(0.0);
      ws->getSample()->getGeometry(thick, width, height);
      m_uiForm.sample_thick->setText(QString::number(thick));
      m_uiForm.sample_width->setText(QString::number(width));
      m_uiForm.sample_height->setText(QString::number(height));
    }

    QLabel *label = qobject_cast<QLabel*>(m_period_lbls.value(key));
    label->setText("/ " + QString::number(n_periods));
    QLineEdit *userentry = qobject_cast<QLineEdit*>(label->buddy());
    userentry->setText("1");

    load_success = true;
  }

  // Cannot do anything if nothing was loaded
  if( !load_success ) 
  {
   setProcessingState(false, -1);
   return;
  }

  forceDataReload(false);
  //Fill in the information on the geometry tab
  setupGeometryDetails();

  for( int index = 1; index < m_uiForm.tabWidget->count(); ++index )
  {
    m_uiForm.tabWidget->setTabEnabled(index, true);
  }
 
  setProcessingState(false, -1);
}

/** 
 * Construct the python code to perform the analysis based on the 
 * current settings
 * @ replacewsnames Whether to replace the data workspace names
 * @param checkchanges Whether to check that a data reload is necessary
 */
QString SANSRunWindow::constructReductionCode(bool , bool)
{
  if( !readPyReductionTemplate() ) return QString();
  if( m_ins_defdir.isEmpty() ) m_ins_defdir = m_data_dir;
  // Quick check that scattering sample number has been entered
  if( m_uiForm.sct_sample_edit->text().isEmpty() )
  {
    showInformationBox("Error: A scattering sample run number is required to continue.");
    return QString();
  } 

  //Construct the code to execute
  QString py_code = m_pycode_loqreduce;
  py_code.replace("|INSTRUMENTPATH|", m_ins_defdir);
  py_code.replace("|INSTRUMENTNAME|", m_uiForm.inst_opt->currentText());
  py_code.replace("|DETBANK|", m_uiForm.detbank_sel->currentText());
  
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

  //dQ
  step_prefix = "";
  py_code.replace("|QMIN|", m_uiForm.q_min->text());
  py_code.replace("|QMAX|", m_uiForm.q_max->text());
  if( m_uiForm.q_dq_opt->currentIndex() == 1 ) step_prefix = "-";
  py_code.replace("|QDELTA|", step_prefix  + m_uiForm.q_dq->text());

  // Qxy
  py_code.replace("|QXYMAX|", m_uiForm.qy_max->text());
  step_prefix = "";
  if( m_uiForm.qy_dqy_opt->currentIndex() == 1 ) step_prefix = "-";
  py_code.replace("|QXYDELTA|", step_prefix  + m_uiForm.qy_dqy->text());

  // Transmission behaviour
  if( m_uiForm.prev_trans->isChecked() )
  {
    py_code.replace("|USEPREVTRANS|", "True");
  }
  else
  {
    py_code.replace("|USEPREVTRANS|", "False");
  }

  // Efficiency
  py_code.replace("|DIRECTFILE|", m_uiForm.direct_file->text());
  py_code.replace("|FLATFILE|", m_uiForm.flat_file->text());

  
  py_code.replace("|SAMPLEZOFFSET|", m_uiForm.smpl_offset->text());
  py_code.replace("|MASKSTRING|", createMaskString());
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

  py_code.replace("|SCALEFACTOR|", m_uiForm.scale_factor->text());
  py_code.replace("|GEOMID|", m_uiForm.sample_geomid->currentText().at(0));
  py_code.replace("|SAMPLEWIDTH|", m_uiForm.sample_width->text());
  py_code.replace("|SAMPLEHEIGHT|", m_uiForm.sample_height->text());
  py_code.replace("|SAMPLETHICK|", m_uiForm.sample_thick->text());
  
  // Log information
  py_code.replace("|ZFRONTDET|", QString::number(m_logvalues["Front_Det_Z"]));
  py_code.replace("|XFRONTDET|", QString::number(m_logvalues["Front_Det_X"]));
  py_code.replace("|ROTFRONTDET|", QString::number(m_logvalues["Front_Det_Rot"]));
  py_code.replace("|ZREARDET|", QString::number(m_logvalues["Rear_Det_Z"]));
  py_code.replace("|XREARDET|", QString::number(m_logvalues["Rear_Det_X"]));
  //Mask file correction values
  py_code.replace("|ZCORRFRONTDET|", QString::number(m_maskcorrections["Front_Det_Z_corr"]));
  py_code.replace("|YCORRFRONTDET|", QString::number(m_maskcorrections["Front_Det_Y_corr"]));
  py_code.replace("|XCORRFRONTDET|", QString::number(m_maskcorrections["Front_Det_X_corr"]));
  py_code.replace("|ROTCORRFRONTDET|", QString::number(m_maskcorrections["Front_Det_Rot_corr"]));
  py_code.replace("|ZCORREARDET|", QString::number(m_maskcorrections["Rear_Det_Z_corr"]));
  py_code.replace("|XCORREARDET|", QString::number(m_maskcorrections["Rear_Det_X_corr"]));

  return py_code;
}


/**
 * Run the analysis script
 * @param type The data reduction type, 1D or 2D
 */
void SANSRunWindow::handleReduceButtonClick(const QString & type)
{
    
  //Currently the components are moved with each reduce click. Check if a load is necessary
  handleLoadButtonClick();

  int idtype(0);
  if( type.startsWith("2") ) idtype = 1;

  QString py_code = constructReductionCode();
  if( py_code.isEmpty() )
  {
    return;
  }

  //Disable buttons so that interaction is limited while processing data
  setProcessingState(true, idtype);
  m_lastreducetype = idtype;

  py_code.replace("|ANALYSISTYPE|", type);
  py_code += 
    "sample_setup = InitReduction(SCATTER_SAMPLE, [XBEAM_CENTRE, YBEAM_CENTRE], False)\n"
    "can_setup = InitReduction(SCATTER_CAN, [XBEAM_CENTRE, YBEAM_CENTRE], True)\n"
    "WavRangeReduction(sample_setup, can_setup, WAV1, WAV2)";

  //Execute the code
  runPythonCode(py_code, false);
  // Mark that a reload is necessary to rerun the same reduction
  forceDataReload();
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

void SANSRunWindow::handleRunFindCentre()
{

  // Start iteration
  updateCentreFindingStatus("::SANS::Loading data");
  handleLoadButtonClick();

  // Disable interaction
  setProcessingState(true, 0);

  // This checks whether we have a sample run and that it has been loaded
  QString py_code = constructReductionCode(false, false);
  if( py_code.isEmpty() )
  {
    setProcessingState(false, 0);
    return;
  }

  py_code.replace("|ANALYSISTYPE|", "1D");
  if( m_uiForm.beam_rmin->text().isEmpty() )
  {
    m_uiForm.beam_rmin->setText("60");
  }

  if( m_uiForm.beam_rmax->text().isEmpty() )
  {
    if( m_uiForm.inst_opt->currentIndex() == 0 )
    {
      m_uiForm.beam_rmax->setText("200");
    }
    else
    {
      m_uiForm.beam_rmax->setText("280");
    }
  }

  if( m_uiForm.beam_iter->text().isEmpty() )
  {
    m_uiForm.beam_iter->setText("15");
  }

  if( m_uiForm.beamstart_box->currentIndex() == 0 )
  {
    py_code += "Xstart = None; Ystart = None\n";
  }
  else
  {
    //
    if( m_uiForm.beam_x->text().isEmpty() || m_uiForm.beam_y->text().isEmpty() )
    {
      showInformationBox("Current centre postion is invalid.");
      return;
    }
    else
    {
      py_code += "Xstart = " + m_uiForm.beam_x->text() + "/1000.; Ystart = " + m_uiForm.beam_y->text() + "/1000\n";
    }
  }

  py_code += "\nbeamcoords = FindBeamCentre(";
  py_code += "rlow = " + m_uiForm.beam_rmin->text() + "/1000., rupp = " 
    + m_uiForm.beam_rmax->text() + "/1000., MaxIter = " + m_uiForm.beam_iter->text() 
    + ", xstart = Xstart, ystart = Ystart)\n"
    + "print '|' + str(beamcoords[0]) + '|' + str(beamcoords[1])\n";

  updateCentreFindingStatus("::SANS::Iteration 1");
  m_uiForm.beamstart_box->setFocus();

  //Execute the code
  //Connect up the logger to handle updating the centre finding status box
  connect(this, SIGNAL(logMessageReceived(const QString&)), this, 
	  SLOT(updateCentreFindingStatus(const QString&)));

  QString result = runPythonCode(py_code);
  
  disconnect(this, SIGNAL(logMessageReceived(const QString&)), this, 
	     SLOT(updateCentreFindingStatus(const QString&)));
  
  QTextStream reader(&result, QIODevice::ReadOnly);
  double x(0.0), y(0.0);
  bool found(false);
  while( !reader.atEnd() )
  {
    QString line = reader.readLine();
    if( line.startsWith('|') )
    {
      QStringList xycoords = result.split("|");
      if( xycoords.size() == 3 )
      {
        x = xycoords[1].toDouble();
        y = xycoords[2].toDouble();
        found = true;
      }
    }
  }

  if( found )
  {
    m_uiForm.beam_x->setText(QString::number(x*1000));
    m_uiForm.beam_y->setText(QString::number(y*1000));
    updateCentreFindingStatus("::SANS::Coordinates updated");
  }
  else
  {
    updateCentreFindingStatus("::SANS::Error with search");
  }
  forceDataReload();
  //Reenable stuff
  setProcessingState(false, 0);
}

/**
 * Save a workspace
 */
void SANSRunWindow::handleSaveButtonClick()
{
  runPythonCode("SaveRKHDialog()", false);
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

  py_code.replace("|XCENTRE|", m_uiForm.beam_x->text());
  py_code.replace("|YCENTRE|", m_uiForm.beam_y->text());

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
  py_code.replace("|MASKLIST|", createMaskString());
  runPythonCode(py_code);
}

/**
 * A different instrument has been selected
 */
void SANSRunWindow::handleInstrumentChange(int index)
{
  if( index == 0 ) 
  {
    m_uiForm.monitor_spec->setText("2");
    m_uiForm.detbank_sel->setItemText(0, "main-detector-bank");
    m_uiForm.detbank_sel->setItemText(1, "HAB");
    m_uiForm.beam_rmin->setText("60");
    m_uiForm.beam_rmax->setText("200");
    
    m_uiForm.geom_stack->setCurrentIndex(0);
  }
  else
  { 
    m_uiForm.monitor_spec->setText("2");
    m_uiForm.detbank_sel->setItemText(0, "rear-detector");
    m_uiForm.detbank_sel->setItemText(1, "front-detector");
    m_uiForm.beam_rmin->setText("60");
    m_uiForm.beam_rmax->setText("280");

    m_uiForm.geom_stack->setCurrentIndex(1);
  }
  m_cfg_loaded = false;
}

/**
 * Handles the change of current tab
 * @param index The new index
 */
void SANSRunWindow::handleTabChange(int index)
{
  if( index != 2 ) return;

  // Test for scipy optimize module and disable centre finding if it is not found
  QString scipycode = 
    "try:\n"
    "\timport scipy.optimize\n"
    "except(ImportError):\n"
    "\texit('scipy package is not installed')\n"
    "try:\n"
    "\timport numpy\n"
    "except(ImportError):\n"
    "\texit('scipy package is not installed')\n";

    QString result = runPythonCode(scipycode);
    m_havescipy = true;
    if( !result.isEmpty() )
    {
      showInformationBox("The centre-finding functionality requires the scipy and numpy Python packages to be installed,\n"
			 "please install them, taking care of to use the correct Python version, if you wish to use this function.");
      m_havescipy = false;
      m_uiForm.runcentreBtn->setEnabled(false);
    }
    disconnect(m_uiForm.tabWidget, SIGNAL(currentChanged(int)), this, SLOT(handleTabChange(int)));
}

/**
 * Update the centre finding status label
 * @param msg The message string
 */
void SANSRunWindow::updateCentreFindingStatus(const QString & msg)
{
  static QString prefix = "::SANS::";
  if( msg.startsWith(prefix) )
  {
    m_uiForm.centre_stat->setText(msg.split(prefix).value(1));
  }  
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
  if( workspace.isEmpty() )
  {
    return 0;
  }
  QString filepath = getRawFilePath(work_dir, run_no, ext);
  if( filepath.isEmpty() )
  {   
    return 0;
  }
  if( workspaceExists(workspace) )
  {
    runPythonCode(QString("mtd.deleteWorkspace('") + workspace + QString("')"),false);
  }

  g_log.debug("Attempting to load " + filepath.toStdString());
  QString py_code("");
  if( ext == ".raw" )
  {
    py_code = "LoadRaw";
  }
  else
  {
    py_code = "LoadISISNexus";
  }
  py_code += "(Filename='" + filepath + "', OutputWorkspace='" + workspace + "')"; 
  QString results = runPythonCode(py_code);
  if( !results.isEmpty() )
  {
    return 0;
  }
  g_log.debug("Loading algorithm succeeded.");

  if( ext == ".raw" )
  {
    py_code = "LoadSampleDetailsFromRaw(InputWorkspace='" + workspace 
      + "', Filename='" + filepath + "')";
    runPythonCode(py_code, false);
  }

  //Load succeeded so find the number of periods. (Here the number of workspaces in the group)
  //Retrieve shoudn't throw but lets wrap it just in case
  Mantid::API::Workspace_sptr wksp_ptr;
  try
  {
    wksp_ptr = Mantid::API::AnalysisDataService::Instance().retrieve(workspace.toStdString());
  }
  catch(Mantid::Kernel::Exception::NotFoundError &)
  {
    g_log.error("Couldn't find workspace " + workspace.toStdString() + " in ADS.");
    return 0;
  }

  // Find the number of periods
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

/**
 * Load log information. If the file has a raw extension then a log file with the same stem but .log is used
 * @param work_dir The directory
 * @param run_no The run number
 * @returns A map of log name to value
 */
QHash<QString, double> SANSRunWindow::loadDetectorLogs(const QString& work_dir, const QString & run_no)
{
  //Not necesary for LOQ for
  if( m_uiForm.inst_opt->currentIndex() == 0 ) return QHash<QString, double>();
  
  if( m_uiForm.file_opt->currentIndex() == 0 )
  {
    QString filepath = getRawFilePath(work_dir, run_no, ".raw");
    QString logname = QFileInfo(filepath).baseName();
    QString suffix = ".log";
    QString logpath = QFileInfo(filepath).path() + "/" + logname + suffix;
    QFile handle(logpath);
  
    if( !handle.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
      return QHash<QString, double>();
    }
    
    QHash<QString, double> logvalues;
    logvalues["Rear_Det_X"] = 0.0;
    logvalues["Rear_Det_Z"] = 0.0;
    logvalues["Front_Det_X"] = 0.0;
    logvalues["Front_Det_Z"] = 0.0;
    logvalues["Front_Det_Rot"] = 0.0;
    QList<QString> logkeys = logvalues.keys();
  
    QTextStream reader(&handle);
    while( !reader.atEnd() )
    {
      QString line = reader.readLine();
      QStringList items = line.split(QRegExp("\\s+"));
      QString entry = items.value(1);
      if( logkeys.contains(entry) )
      {
	// Log values are in mm 
	logvalues[entry] = items.value(2).toDouble();
      }
    }
    handle.close();
    return logvalues;
  }

  return QHash<QString, double>();
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

/**
 * Format a double as a string
 * @param value The double to convert to a string
 * @param format The format char
 * @param precision The precision
 * @param colour The colour
 */
QString SANSRunWindow::formatDouble(double value, char format, int precision, const QString & colour)
{
  return QString("<font color='") + colour + QString("'>") + QString::number(value, format, precision)  + QString("</font>");
}

/**
 * Rest the geometry details box 
 */
void SANSRunWindow::resetGeometryDetailsBox()
{
  QString blank("-");
  //LOQ
  m_uiForm.dist_mod_mon->setText(blank);
  m_uiForm.dist_sample_ms->setText(blank);
  m_uiForm.dist_smp_mdb->setText(blank);
  m_uiForm.dist_smp_hab->setText(blank);

  m_uiForm.dist_can_ms->setText(blank);
  m_uiForm.dist_can_mdb->setText(blank);
  m_uiForm.dist_can_hab->setText(blank);

  m_uiForm.dist_bkgd_ms->setText(blank);
  m_uiForm.dist_bkgd_mdb->setText(blank);
  m_uiForm.dist_bkgd_hab->setText(blank);
  

  //SANS2D
  m_uiForm.dist_mon_s2d->setText(blank);
  m_uiForm.dist_sample_ms_s2d->setText(blank);
  m_uiForm.dist_can_ms_s2d->setText(blank);
  m_uiForm.dist_bkgd_ms_s2d->setText(blank);

  m_uiForm.dist_smp_rearX->setText(blank);
  m_uiForm.dist_smp_rearZ->setText(blank);
  m_uiForm.dist_smp_frontX->setText(blank);
  m_uiForm.dist_smp_frontZ->setText(blank);
  m_uiForm.smp_rot->setText(blank);

  m_uiForm.dist_can_rearX->setText(blank);
  m_uiForm.dist_can_rearZ->setText(blank);
  m_uiForm.dist_can_frontX->setText(blank);
  m_uiForm.dist_can_frontZ->setText(blank);
  m_uiForm.can_rot->setText(blank);

  m_uiForm.dist_bkgd_rearX->setText(blank);
  m_uiForm.dist_bkgd_rearZ->setText(blank);
  m_uiForm.dist_bkgd_frontX->setText(blank);
  m_uiForm.dist_bkgd_frontZ->setText(blank);
  m_uiForm.bkgd_rot->setText(blank);

}

void SANSRunWindow::cleanup()
{
  Mantid::API::AnalysisDataServiceImpl & ads = Mantid::API::AnalysisDataService::Instance();
  std::vector<std::string> workspaces = ads.getObjectNames();
  std::vector<std::string>::const_iterator iend = workspaces.end();
  for( std::vector<std::string>::const_iterator itr = workspaces.begin(); itr != iend; ++itr )
  {
    if( QString::fromStdString(*itr).endsWith("_raw") )
    {
      ads.remove(*itr);
    }
  }
}
