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
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/V3D.h"

#include <QLineEdit>
#include <QFileDialog>
#include <QHash>
#include <QTextStream>
#include <QTreeWidgetItem>
#include <QSettings>
#include <QMessageBox>
#include <QInputDialog>
#include <QSignalMapper>
#include <QHeaderView>
#include <QApplication>
#include <QClipboard>
#include <QTemporaryFile>
#include <QDateTime>

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

//----------------------------------------------
// Public member functions
//----------------------------------------------
///Constructor
SANSRunWindow::SANSRunWindow(QWidget *parent) :
  UserSubWindow(parent), m_data_dir(""), m_ins_defdir(""), m_last_dir(""), m_cfg_loaded(true), m_run_no_boxes(), 
  m_period_lbls(), m_warnings_issued(false), m_force_reload(false), m_log_warnings(false),
  m_delete_observer(*this,&SANSRunWindow::handleMantidDeleteWorkspace), m_s2d_detlabels(), 
  m_loq_detlabels(), m_allowed_batchtags(), m_lastreducetype(-1), m_have_reducemodule(false), 
  m_dirty_batch_grid(false), m_tmp_batchfile("")
{
  m_reducemapper = new QSignalMapper(this);
  m_mode_mapper = new QSignalMapper(this);
  Mantid::API::AnalysisDataService::Instance().notificationCenter.addObserver(m_delete_observer);
  
  m_allowed_batchtags.insert("sample_sans",0);
  m_allowed_batchtags.insert("sample_trans",1);
  m_allowed_batchtags.insert("sample_direct_beam",2);
  m_allowed_batchtags.insert("can_sans",3);
  m_allowed_batchtags.insert("can_trans",4);
  m_allowed_batchtags.insert("can_direct_beam",5);
  m_allowed_batchtags.insert("background_sans",-1);
  m_allowed_batchtags.insert("background_trans",-1);
  m_allowed_batchtags.insert("background_direct_beam",-1);
  m_allowed_batchtags.insert("output_as",6);

}

///Destructor
SANSRunWindow::~SANSRunWindow()
{
  // Seems to crash on destruction of if I don't do this 
  Mantid::API::AnalysisDataService::Instance().notificationCenter.removeObserver(m_delete_observer);
  saveSettings();
}

//--------------------------------------------
// Private member functions
//--------------------------------------------
/**
 * Set up the dialog layout
 */
void SANSRunWindow::initLayout()
{
    g_log.debug("Initializing interface layout");
    m_uiForm.setupUi(this);

    //Set column stretch on the mask table
    m_uiForm.mask_table->horizontalHeader()->setStretchLastSection(true);

    //Button connections
    connect(m_uiForm.data_dirBtn, SIGNAL(clicked()), this, SLOT(selectDataDir()));
    connect(m_uiForm.userfileBtn, SIGNAL(clicked()), this, SLOT(selectUserFile()));
    connect(m_uiForm.csv_browse_btn,SIGNAL(clicked()), this, SLOT(selectCSVFile()));

    connect(m_uiForm.load_dataBtn, SIGNAL(clicked()), this, SLOT(handleLoadButtonClick()));
    //connect(m_uiForm.plotBtn, SIGNAL(clicked()), this, SLOT(handlePlotButtonClick()));
    connect(m_uiForm.runcentreBtn, SIGNAL(clicked()), this, SLOT(handleRunFindCentre()));
    connect(m_uiForm.saveBtn, SIGNAL(clicked()), this, SLOT(handleSaveButtonClick()));


    // Disable most things so that load is the only thing that can be done
    m_uiForm.oneDBtn->setEnabled(false);
    m_uiForm.twoDBtn->setEnabled(false);
    for( int i = 1; i < 4; ++i)
    {
      m_uiForm.tabWidget->setTabEnabled(i, false);
    }

    // Reduction buttons
    connect(m_uiForm.oneDBtn, SIGNAL(clicked()), m_reducemapper, SLOT(map()));
    m_reducemapper->setMapping(m_uiForm.oneDBtn, "1D");
    connect(m_uiForm.twoDBtn, SIGNAL(clicked()), m_reducemapper, SLOT(map()));
    m_reducemapper->setMapping(m_uiForm.twoDBtn, "2D");
    connect(m_reducemapper, SIGNAL(mapped(const QString &)), this, SLOT(handleReduceButtonClick(const QString &)));
    
    connect(m_uiForm.showMaskBtn, SIGNAL(clicked()), this, SLOT(handleShowMaskButtonClick()));
    connect(m_uiForm.clear_log, SIGNAL(clicked()), m_uiForm.centre_logging, SLOT(clear()));

    //Mode switches
    connect(m_uiForm.single_mode_btn, SIGNAL(clicked()), m_mode_mapper, SLOT(map()));
    m_mode_mapper->setMapping(m_uiForm.single_mode_btn, SANSRunWindow::SingleMode);
    connect(m_uiForm.batch_mode_btn, SIGNAL(clicked()), m_mode_mapper, SLOT(map()));
    m_mode_mapper->setMapping(m_uiForm.batch_mode_btn, SANSRunWindow::BatchMode);
    connect(m_mode_mapper, SIGNAL(mapped(int)), this, SLOT(switchMode(int)));

    //Set a custom context for the batch table
    m_uiForm.batch_table->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_batch_paste = new QAction(tr("&Paste"),m_uiForm.batch_table);
    m_batch_paste->setShortcut(tr("Ctrl+P"));
    connect(m_batch_paste, SIGNAL(activated()), this, SLOT(pasteToBatchTable()));
    m_uiForm.batch_table->addAction(m_batch_paste);

    m_batch_clear = new QAction(tr("&Clear"),m_uiForm.batch_table);    
    m_uiForm.batch_table->addAction(m_batch_clear);
    connect(m_batch_clear, SIGNAL(activated()), this, SLOT(clearBatchTable()));

    //Logging
    connect(this, SIGNAL(logMessageReceived(const QString&)), this, SLOT(updateLogWindow(const QString&)));
    connect(m_uiForm.logger_clear, SIGNAL(clicked()), m_uiForm.logging_field, SLOT(clear()));
    m_uiForm.logging_field->ensureCursorVisible();

    connect(m_uiForm.verbose_check, SIGNAL(stateChanged(int)), this, SLOT(verboseMode(int)));

    //Create the widget hash maps
    initWidgetMaps();

    //Connect each box's edited signal to flag if the box's text has changed
    for( int idx = 0; idx < 9; ++idx )
    {
      connect(m_run_no_boxes.value(idx), SIGNAL(textEdited(const QString&)), this, SLOT(runChanged()));
    }
    
    connect(m_uiForm.smpl_offset, SIGNAL(textEdited(const QString&)), this, SLOT(runChanged()));

    // Combo boxes
    connect(m_uiForm.wav_dw_opt, SIGNAL(currentIndexChanged(int)), this, 
	    SLOT(handleStepComboChange(int)));
    connect(m_uiForm.q_dq_opt, SIGNAL(currentIndexChanged(int)), this, 
	    SLOT(handleStepComboChange(int)));
    connect(m_uiForm.qy_dqy_opt, SIGNAL(currentIndexChanged(int)), this, 
	    SLOT(handleStepComboChange(int)));

    connect(m_uiForm.inst_opt, SIGNAL(currentIndexChanged(int)), this, 
	    SLOT(handleInstrumentChange(int)));

    // Add Python set functions as underlying data 
    m_uiForm.inst_opt->setItemData(0, "LOQ()");
    m_uiForm.inst_opt->setItemData(1, "SANS2D()");

    //Add shortened forms of step types to step boxes
    m_uiForm.wav_dw_opt->setItemData(0, "LIN");
    m_uiForm.wav_dw_opt->setItemData(1, "LOG");
    m_uiForm.q_dq_opt->setItemData(0, "LIN");
    m_uiForm.q_dq_opt->setItemData(1, "LOG");
    m_uiForm.qy_dqy_opt->setItemData(0, "LIN");

    readSettings();
}    

/**
 * Initialize the widget maps
 */
void SANSRunWindow::initWidgetMaps()
{
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

    // SANS2D det names/label map
    QHash<QString, QLabel*> labelsmap;
    labelsmap.insert("Front_Det_Z", m_uiForm.dist_smp_frontZ);
    labelsmap.insert("Front_Det_X", m_uiForm.dist_smp_frontX);
    labelsmap.insert("Front_Det_Rot", m_uiForm.smp_rot);
    labelsmap.insert("Rear_Det_X", m_uiForm.dist_smp_rearX);
    labelsmap.insert("Rear_Det_Z", m_uiForm.dist_smp_rearZ);
    m_s2d_detlabels.append(labelsmap);
  
    labelsmap.clear();
    labelsmap.insert("Front_Det_Z", m_uiForm.dist_can_frontZ);
    labelsmap.insert("Front_Det_X", m_uiForm.dist_can_frontX);
    labelsmap.insert("Front_Det_Rot", m_uiForm.can_rot);
    labelsmap.insert("Rear_Det_X", m_uiForm.dist_can_rearX);
    labelsmap.insert("Rear_Det_Z", m_uiForm.dist_can_rearZ);
    m_s2d_detlabels.append(labelsmap);

    labelsmap.clear();
    labelsmap.insert("Front_Det_Z", m_uiForm.dist_bkgd_frontZ);
    labelsmap.insert("Front_Det_X", m_uiForm.dist_bkgd_frontX);
    labelsmap.insert("Front_Det_Rot", m_uiForm.bkgd_rot);
    labelsmap.insert("Rear_Det_X", m_uiForm.dist_bkgd_rearX);
    labelsmap.insert("Rear_Det_Z", m_uiForm.dist_bkgd_rearZ);
    m_s2d_detlabels.append(labelsmap);

    //LOQ labels
    labelsmap.clear();
    labelsmap.insert("moderator-sample", m_uiForm.dist_sample_ms);
    labelsmap.insert("sample-main-detector-bank", m_uiForm.dist_smp_mdb);
    labelsmap.insert("sample-HAB",m_uiForm.dist_smp_hab);
    m_loq_detlabels.append(labelsmap);
  
    labelsmap.clear();
    labelsmap.insert("moderator-sample", m_uiForm.dist_can_ms);
    labelsmap.insert("sample-main-detector-bank", m_uiForm.dist_can_mdb);
    labelsmap.insert("sample-HAB",m_uiForm.dist_can_hab);
    m_loq_detlabels.append(labelsmap);

    labelsmap.clear();
    labelsmap.insert("moderator-sample", m_uiForm.dist_bkgd_ms);
    labelsmap.insert("sample-main-detector-bank", m_uiForm.dist_bkgd_mdb);
    labelsmap.insert("sample-HAB", m_uiForm.dist_bkgd_hab);
    m_loq_detlabels.append(labelsmap);

    // Full workspace names as they appear in the service
    m_workspace_names.clear();

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
  m_last_dir = value_store.value("last_dir", "").toString();

  m_uiForm.inst_opt->setCurrentIndex(value_store.value("instrument", 0).toInt());
  
  int mode_flag = value_store.value("runmode", 0).toInt();
  if( mode_flag == SANSRunWindow::SingleMode )
  {
    m_uiForm.single_mode_btn->click();
  }
  else
  {
    m_uiForm.batch_mode_btn->click();
  }

  //The instrument definition directory
  m_ins_defdir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("instrumentDefinition.directory"));

  // Setup for instrument
  handleInstrumentChange(m_uiForm.inst_opt->currentIndex());
  //Set old file extension
  m_uiForm.file_opt->setCurrentIndex(value_store.value("fileextension", 0).toInt());
  value_store.endGroup();

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

  value_store.setValue("last_dir", m_last_dir);

  value_store.setValue("instrument", m_uiForm.inst_opt->currentIndex());
  value_store.setValue("fileextension", m_uiForm.file_opt->currentIndex());
  unsigned int mode_id(0);
  if( m_uiForm.single_mode_btn->isChecked() )
  {
    mode_id = SANSRunWindow::SingleMode;
  }
  else
  {
    mode_id = SANSRunWindow::BatchMode;
  }
  value_store.setValue("runmode",mode_id);
  value_store.endGroup();
}

/**
 * Run a function from the SANS reduction script, ensuring that the first call imports the module
 * @param pycode The code to execute
 * @returns A trimmed string containing the output of the code execution
 */
QString SANSRunWindow::runReduceScriptFunction(const QString & pycode)
{
  if( !m_have_reducemodule )
  {
    // Imoprt the SANS module and set the correct instrument
    runPythonCode("from SANSReduction import *\n" , false);
    m_have_reducemodule = true;
  }
  //Ensure the correct instrument is set
  QString code_torun =  "SetNoPrintMode(True)\n" + pycode + "\nSetNoPrintMode(False)";
  return runPythonCode(code_torun).trimmed();
}

/**
 * Trim off Python markers surrounding things like strings or lists that have been 
 * printed by Python
 */
void SANSRunWindow::trimPyMarkers(QString & txt)
{
 txt.remove(0,1);
 txt.chop(1);
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

  user_file.close();
  
  //Clear the def masking info table.
  int mask_table_count = m_uiForm.mask_table->rowCount();
  for( int i = mask_table_count - 1; i >= 0; --i )
  {
    m_uiForm.mask_table->removeRow(i);
  }

  // Use python function to read the file and then extract the fields
  runReduceScriptFunction("MaskFile(r'" + filetext + "')");

  double unit_conv(1000.);
  // Radius
  double dbl_param = runReduceScriptFunction("printParameter('RMIN'),").toDouble();
  m_uiForm.rad_min->setText(QString::number(dbl_param*unit_conv));
  dbl_param = runReduceScriptFunction("printParameter('RMAX'),").toDouble();
  m_uiForm.rad_max->setText(QString::number(dbl_param*unit_conv));
  //Wavelength
  m_uiForm.wav_min->setText(runReduceScriptFunction("printParameter('WAV1'),"));
  m_uiForm.wav_max->setText(runReduceScriptFunction("printParameter('WAV2'),"));
  setLimitStepParameter("wavelength", runReduceScriptFunction("printParameter('DWAV'),"), m_uiForm.wav_dw, m_uiForm.wav_dw_opt);
  //Q
  m_uiForm.q_min->setText(runReduceScriptFunction("printParameter('Q1'),"));
  m_uiForm.q_max->setText(runReduceScriptFunction("printParameter('Q2'),"));
  setLimitStepParameter("Q", runReduceScriptFunction("printParameter('DQ'),"), m_uiForm.q_dq, m_uiForm.q_dq_opt);
  //Qxy
  m_uiForm.qy_max->setText(runReduceScriptFunction("printParameter('QXY2'),"));
  setLimitStepParameter("Qxy", runReduceScriptFunction("printParameter('DQXY'),"), m_uiForm.qy_dqy, m_uiForm.qy_dqy_opt);

  //Monitor spectrum
  m_uiForm.monitor_spec->setText(runReduceScriptFunction("printParameter('MONITORSPECTRUM'),"));

  //Direct efficiency correction
  m_uiForm.direct_file->setText(runReduceScriptFunction("printParameter('DIRECT_BEAM_FILE_R'),"));
  m_uiForm.front_direct_file->setText(runReduceScriptFunction("printParameter('DIRECT_BEAM_FILE_F'),"));

  //Scale factor
  dbl_param = runReduceScriptFunction("printParameter('RESCALE'),").toDouble();
  m_uiForm.scale_factor->setText(QString::number(dbl_param/100.));

  //Sample offset if one has been specified
  dbl_param = runReduceScriptFunction("printParameter('SAMPLE_Z_CORR'),").toDouble();
  m_uiForm.smpl_offset->setText(QString::number(dbl_param*unit_conv));

  //Centre coordinates
  dbl_param = runReduceScriptFunction("printParameter('XBEAM_CENTRE'),").toDouble();
  m_uiForm.beam_x->setText(QString::number(dbl_param*1000.0));
  dbl_param = runReduceScriptFunction("printParameter('YBEAM_CENTRE'),").toDouble();
  m_uiForm.beam_y->setText(QString::number(dbl_param*1000.0));

  //Gravity switch
  QString param = runReduceScriptFunction("printParameter('GRAVITY')");
  if( param == "True" )
  {
    m_uiForm.gravity_check->setChecked(true);
  }
  else
  {
    m_uiForm.gravity_check->setChecked(false);
  }
  
  ////Detector bank
  param = runReduceScriptFunction("printParameter('DETBANK')");
  int index = m_uiForm.detbank_sel->findText(param);  
  if( index >= 0 && index < 2 )
  {
    m_uiForm.detbank_sel->setCurrentIndex(index);
  }

  //Masking table
  updateMaskTable();
 
  // Phi values 
  m_uiForm.phi_min->setText(runReduceScriptFunction("printParameter('PHIMIN')"));
  m_uiForm.phi_max->setText(runReduceScriptFunction("printParameter('PHIMAX')"));
  
  m_cfg_loaded = true;
  m_uiForm.userfileBtn->setText("Reload");
  m_uiForm.tabWidget->setTabEnabled(m_uiForm.tabWidget->count() - 1, true);
  return true;
}

/**
 * Load a CSV file specifying information run numbers and populate the batch mode grid
 */
bool SANSRunWindow::loadCSVFile()
{
  QString filename = m_uiForm.csv_filename->text(); 
  QFile csv_file(filename);
  if( !csv_file.open(QIODevice::ReadOnly | QIODevice::Text) )
  {
    showInformationBox("Error: Cannot open CSV file \"" + filename + "\"");
    return false;
  }
  
  //Clear the current table
  clearBatchTable();
  QTextStream file_in(&csv_file);
  int errors(0);
  while( !file_in.atEnd() )
  {
    QString line = file_in.readLine().simplified();
    if( !line.isEmpty() )
    {
      errors += addBatchLine(line, ",");
    }
  }
  if( errors > 0 )
  {
    showInformationBox("Warning: " + QString::number(errors) + " malformed lines detected in \"" + filename + "\". Lines skipped.");
  }
  return true;
}

/**
 * Set a pair of an QLineEdit field and type QComboBox using the parameter given
 * @param pname The name of the parameter
 * @param param A string representing a value that maybe prefixed with a minus to indicate a different step type
 * @param step_value The field to store the actual value
 * @param step_type The combo box with the type options
 */
void SANSRunWindow::setLimitStepParameter(const QString& pname, QString param, QLineEdit* step_value,  QComboBox* step_type)
{
  if( param.startsWith("-") )
  {
    int index = step_type->findText("Logarithmic");
    if( index < 0 )
    {
     raiseOneTimeMessage("Warning: Unable to find logarithmic scale option for " + pname + ", setting as linear.", 1);
     index = step_type->findText("Linear");
    }
    step_type->setCurrentIndex(index);
    step_value->setText(param.remove(0,1));
  }
  else
  {
    step_type->setCurrentIndex(step_type->findText("Linear"));
    step_value->setText(param);
  }
}

/**
 * Construct the mask table on the Mask tab 
 */
void SANSRunWindow::updateMaskTable()
{
  //Clear the current contents
  for( int i = m_uiForm.mask_table->rowCount() - 1; i >= 0; --i )
  {
	  m_uiForm.mask_table->removeRow(i);
	}

  QString reardet_name("rear-detector"), frontdet_name("front-detector");
  if( m_uiForm.inst_opt->currentIndex() == 0 )
  {
    reardet_name = "main-detector-bank";
    frontdet_name = "HAB";
  }
  
  // First create 2 default mask cylinders at min and max radius for the beam stop and 
  // corners
  m_uiForm.mask_table->insertRow(0);
  m_uiForm.mask_table->setItem(0, 0, new QTableWidgetItem("beam stop"));
  m_uiForm.mask_table->setItem(0, 1, new QTableWidgetItem(reardet_name));
  m_uiForm.mask_table->setItem(0, 2, new QTableWidgetItem("infinite-cylinder, r = rmin"));
  if( m_uiForm.rad_max->text() != "-1" )
  {  
    m_uiForm.mask_table->insertRow(1);
    m_uiForm.mask_table->setItem(1, 0, new QTableWidgetItem("corners"));
    m_uiForm.mask_table->setItem(1, 1, new QTableWidgetItem(reardet_name));
    m_uiForm.mask_table->setItem(1, 2, new QTableWidgetItem("infinite-cylinder, r = rmax"));
  }

  //Now add information from the mask file
  //Spectrum mask
  QString mask_string = runReduceScriptFunction("printParameter('SPECMASKSTRING')");
  addSpectrumMasksToTable(mask_string, "-");
  //"Rear" det
  mask_string = runReduceScriptFunction("printParameter('SPECMASKSTRING_R')");
  addSpectrumMasksToTable(mask_string, reardet_name);
  //"Front" det
  mask_string = runReduceScriptFunction("printParameter('SPECMASKSTRING_F')");
  addSpectrumMasksToTable(mask_string, frontdet_name);

  //Time masks
  mask_string = runReduceScriptFunction("printParameter('TIMEMASKSTRING')");
  addTimeMasksToTable(mask_string, "-");
  //Rear detector
  mask_string = runReduceScriptFunction("printParameter('TIMEMASKSTRING_R')");
  addTimeMasksToTable(mask_string, reardet_name);
  //Front detectors
  mask_string = runReduceScriptFunction("printParameter('TIMEMASKSTRING_F')");
  addTimeMasksToTable(mask_string, frontdet_name);
}

/**
 * Add a spectrum mask string to the mask table
 * @param mask_string The string of mask information
 * @param det_name The detector it relates to 
 */
void SANSRunWindow::addSpectrumMasksToTable(const QString & mask_string, const QString & det_name)
{
  QStringList elements = mask_string.split(",", QString::SkipEmptyParts);
  QStringListIterator sitr(elements);
  while(sitr.hasNext())
  {
    QString item = sitr.next();
    QString col1_txt;
    if( item.startsWith('s', Qt::CaseInsensitive) )
    {
      col1_txt = "Spectrum";
    }
    else if( item.startsWith('h', Qt::CaseInsensitive) || item.startsWith('v', Qt::CaseInsensitive) )
    {
      if( item.contains('+') )
      {
        col1_txt = "Box";
      }
      else
      {
        col1_txt = "Strip";
      }
    }
    else continue;

    int row = m_uiForm.mask_table->rowCount();
    //Insert line after last row
    m_uiForm.mask_table->insertRow(row);
    m_uiForm.mask_table->setItem(row, 0, new QTableWidgetItem(col1_txt));
    m_uiForm.mask_table->setItem(row, 1, new QTableWidgetItem(det_name));
    m_uiForm.mask_table->setItem(row, 2, new QTableWidgetItem(item));
  }
}

/**
 * Add a time mask string to the mask table
 * @param mask_string The string of mask information
 * @param det_name The detector it relates to 
 */
void SANSRunWindow::addTimeMasksToTable(const QString & mask_string, const QString & det_name)
{
  QStringList elements = mask_string.split(";",QString::SkipEmptyParts);
  QStringListIterator sitr(elements);
  while(sitr.hasNext())
  {
    int row = m_uiForm.mask_table->rowCount();
    m_uiForm.mask_table->insertRow(row);
    m_uiForm.mask_table->setItem(row, 0, new QTableWidgetItem("time"));
    m_uiForm.mask_table->setItem(row, 1, new QTableWidgetItem(det_name));
    m_uiForm.mask_table->setItem(row, 2, new QTableWidgetItem(sitr.next()));
  }
}

/**
 * Retrieve and set the component distances
 * @param workspace The workspace pointer
 * @param lms The result of the moderator-sample distance
 * @param lsda The result of the sample-detector bank 1 distance
 * @param lsdb The result of the sample-detector bank 2 distance
 */
void SANSRunWindow::componentLOQDistances(Mantid::API::MatrixWorkspace_sptr workspace, double & lms, double & lsda, double & lsdb)
{
  Mantid::API::IInstrument_sptr instr = workspace->getInstrument();
  if( instr == boost::shared_ptr<Mantid::API::IInstrument>() ) return;

  Mantid::Geometry::IObjComponent_sptr source = instr->getSource();
  if( source == boost::shared_ptr<Mantid::Geometry::IObjComponent>() ) return;
  Mantid::Geometry::IObjComponent_sptr sample = instr->getSample();
  if( sample == boost::shared_ptr<Mantid::Geometry::IObjComponent>() ) return;

  lms = source->getPos().distance(sample->getPos()) * 1000.;
   
  //Find the main detector bank
  boost::shared_ptr<Mantid::Geometry::IComponent> comp = instr->getComponentByName("main-detector-bank");
  if( comp != boost::shared_ptr<Mantid::Geometry::IComponent>() )
  {
    lsda = sample->getPos().distance(comp->getPos()) * 1000.;
  }

  comp = instr->getComponentByName("HAB");
  if( comp != boost::shared_ptr<Mantid::Geometry::IComponent>() )
  {
    lsdb = sample->getPos().distance(comp->getPos()) * 1000.;
  }

}

/**
 * Set the state of processing.
 * @param running If we are processing then some interaction is disabled
 * @param type The reduction type, 0 = 1D and 1 = 2D
 */
void SANSRunWindow::setProcessingState(bool running, int type)
{
  if( m_uiForm.single_mode_btn->isChecked() )
  {
    m_uiForm.load_dataBtn->setEnabled(!running);
  }
  else
  {
    m_uiForm.load_dataBtn->setEnabled(false);
  }

  m_uiForm.oneDBtn->setEnabled(!running);
  m_uiForm.twoDBtn->setEnabled(!running);
  //m_uiForm.plotBtn->setEnabled(!running);
  m_uiForm.saveBtn->setEnabled(!running);
  m_uiForm.runcentreBtn->setEnabled(!running);

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
  std::set<std::string> ws_list = Mantid::API::AnalysisDataService::Instance().getObjectNames();
  std::set<std::string>::const_iterator iend = ws_list.end();
  QStringList current_list;
  for( std::set<std::string>::const_iterator itr = ws_list.begin(); itr != iend; ++itr )
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
 * Create the mask strings for spectra and times
 */
void SANSRunWindow::addUserMaskStrings(QString & exec_script)
{
  //Clear current
  exec_script += "Mask('MASK/CLEAR')\nMask('MASK/CLEAR/TIME')\n";

  //Pull in the table details first, skipping the first two rows
  int nrows = m_uiForm.mask_table->rowCount();
  for(int row = 0; row <  nrows; ++row)
  {
    if( m_uiForm.mask_table->item(row, 2)->text().startsWith("inf") )
    {
      continue;
    }
    //Details are in the third column
    exec_script += "Mask('MASK";
    if( m_uiForm.mask_table->item(row, 0)->text() == "time")
    {
      exec_script += "/TIME";
    }
    QString details = m_uiForm.mask_table->item(row, 2)->text();
    QString detname = m_uiForm.mask_table->item(row, 1)->text();
    if( detname == "-" )
    {
      exec_script += " " + details;
    }
    else if( detname == "rear-detector" || detname == "main-detector-bank" )
    {
      exec_script += "/REAR " + details;
    }
    else
    {
      exec_script += "/FRONT " + details;
    }
    exec_script += "')\n";    
  }

  //Spectra mask first
  QStringList mask_params = m_uiForm.user_spec_mask->text().split(",", QString::SkipEmptyParts);
  QStringListIterator sitr(mask_params);
  QString bad_masks;
  while(sitr.hasNext())
  {
    QString item = sitr.next().trimmed();
    if( item.startsWith("REAR", Qt::CaseInsensitive) || item.startsWith("FRONT", Qt::CaseInsensitive) )
    {
      exec_script += "Mask('MASK/" + item + "')\n";
    }
    else if( item.startsWith('S', Qt::CaseInsensitive) || item.startsWith('H', Qt::CaseInsensitive) || 
        item.startsWith('V', Qt::CaseInsensitive) )
    {
      exec_script += "Mask('MASK " + item + "')\n";
    }
    else
    {
      bad_masks += item + ",";
    }
  }
  if( !bad_masks.isEmpty() )
  {
    m_uiForm.tabWidget->setCurrentIndex(3);
    showInformationBox(QString("Warning: Could not parse the following spectrum masks: ") + bad_masks + ". Values skipped.");
  }
  
  //Time masks
  mask_params = m_uiForm.user_time_mask->text().split(",", QString::SkipEmptyParts);
  sitr = QStringListIterator(mask_params);
  bad_masks = "";
  while(sitr.hasNext())
  {
    QString item = sitr.next().trimmed();
    if( item.startsWith("REAR", Qt::CaseInsensitive) || item.startsWith("FRONT", Qt::CaseInsensitive) )
    {
      int ndetails = item.split(" ").count();
      if( ndetails == 3 || ndetails == 2 )
      {
        exec_script += "Mask('/TIME" + item + "')\n";
      }
      else
      {
        bad_masks += item + ",";
      }
    }
  }
  if( !bad_masks.isEmpty() )
  {
    m_uiForm.tabWidget->setCurrentIndex(3);
    showInformationBox(QString("Warning: Could not parse the following time masks: ") + bad_masks + ". Values skipped.");
  }
}

/**
 * Set the information about component distances on the geometry tab
 */
void SANSRunWindow::setGeometryDetails(const QString & sample_logs, const QString & can_logs)
{
  resetGeometryDetailsBox();

  double unit_conv(1000.);
  
  QString workspace_name = getWorkspaceName(0);
  if( workspace_name.isEmpty() ) return;

  Mantid::API::Workspace_sptr workspace_ptr = Mantid::API::AnalysisDataService::Instance().retrieve(workspace_name.toStdString());
  Mantid::API::MatrixWorkspace_sptr sample_workspace = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(workspace_ptr);
  Mantid::API::IInstrument_sptr instr = sample_workspace->getInstrument();
  boost::shared_ptr<Mantid::Geometry::IComponent> source = instr->getSource();

  // Moderator-monitor distance is common to LOQ and S2D
  int monitor_spectrum = m_uiForm.monitor_spec->text().toInt();
  std::vector<int> dets = sample_workspace->spectraMap().getDetectors(monitor_spectrum);
  if( dets.empty() ) return;
  double dist_mm(0.0);
  QString colour("black");
  try
  {
    Mantid::Geometry::IDetector_sptr detector = instr->getDetector(dets[0]);  
    dist_mm = detector->getDistance(*source) * unit_conv;
  }
  catch(std::runtime_error&)
  {
    colour = "red";
  }

  //LOQ
  if( m_uiForm.inst_opt->currentIndex() == 0 )
  {
    if( colour == "red" )
    {
      m_uiForm.dist_mod_mon->setText("<font color='red'>error<font>");
    }
    else
    {
      m_uiForm.dist_mod_mon->setText(formatDouble(dist_mm, colour));
    }
    setLOQGeometry(sample_workspace, 0);
    QString can = getWorkspaceName(1);
    if( !can.isEmpty() )
    {
      Mantid::API::Workspace_sptr workspace_ptr = Mantid::API::AnalysisDataService::Instance().retrieve(can.toStdString());
      Mantid::API::MatrixWorkspace_sptr can_workspace = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(workspace_ptr);
      setLOQGeometry(can_workspace, 1);
    }
  }
  else
  {
    if( colour == "red" )
    {
      m_uiForm.dist_mon_s2d->setText("<font color='red'>error<font>");
    }
    else
    {
      m_uiForm.dist_mon_s2d->setText(formatDouble(dist_mm, colour));
    }

    //SANS2D - Sample
    setSANS2DGeometry(sample_workspace, sample_logs, 0);
    //Get the can workspace if there is one
    QString can = getWorkspaceName(1);
    if( can.isEmpty() ) 
    {
      return;
    }
    Mantid::API::Workspace_sptr workspace_ptr;
    try 
    { 
      workspace_ptr = Mantid::API::AnalysisDataService::Instance().retrieve(can.toStdString());
    }
    catch(std::runtime_error&)
    {
      return;
    }
    Mantid::API::MatrixWorkspace_sptr can_workspace = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(workspace_ptr);
    setSANS2DGeometry(can_workspace, can_logs, 1);

    //Check for discrepancies
    bool warn_user(false);
    double lms_sample(m_uiForm.dist_sample_ms_s2d->text().toDouble()), lms_can(m_uiForm.dist_can_ms_s2d->text().toDouble());
    if( std::fabs(lms_sample - lms_can) > 5e-03 )
    {
      warn_user = true;
      markError(m_uiForm.dist_sample_ms_s2d);
      markError(m_uiForm.dist_can_ms_s2d);
    }

    QString marked_dets = runReduceScriptFunction("print GetMismatchedDetList(),").trimmed();
    trimPyMarkers(marked_dets);
    if( !marked_dets.isEmpty() )
    {
      QStringList detnames = marked_dets.split(",");
      QStringListIterator itr(detnames);
      while( itr.hasNext() )
      {
        QString name = itr.next().trimmed();
        trimPyMarkers(name);
        for( int i = 0; i < 2; ++i )
        {
          markError(m_s2d_detlabels[i].value(name));
          warn_user = true;
        }
      }
    }
    if( warn_user )
    {
      raiseOneTimeMessage("Warning: Some detector distances do not match for the assigned Sample/Can runs, see Geometry tab for details.");
    }
  }
}

/**
 * Set SANS2D geometry info
 * @param workspace The workspace
 * @param logs The log information
*/
void SANSRunWindow::setSANS2DGeometry(Mantid::API::MatrixWorkspace_sptr workspace, const QString & logs, int wscode)
{
  if( m_uiForm.inst_opt->currentIndex() == 0 ) return;
  
  double unitconv = 1000.;

  Mantid::API::IInstrument_sptr instr = workspace->getInstrument();
  boost::shared_ptr<Mantid::Geometry::IComponent> sample = instr->getSample();
  boost::shared_ptr<Mantid::Geometry::IComponent> source = instr->getSource();
  double distance = source->getDistance(*sample) * unitconv;
  //Moderator-sample
  QLabel *dist_label(NULL); 
  if( wscode == 0 )
  {
    dist_label = m_uiForm.dist_sample_ms_s2d;
  }
  else if( wscode == 1 )
  {
    dist_label = m_uiForm.dist_can_ms_s2d;
  }
  else
  {
    dist_label = m_uiForm.dist_bkgd_ms_s2d;
  }
  dist_label->setText(formatDouble(distance, "black"));

  //Detectors
  QStringList det_info = logs.split(",");
  QStringListIterator itr(det_info);
  while( itr.hasNext() )
  {
    QString line = itr.next();
    QStringList values = line.split(":");
    QString detname = values[0].trimmed();
    QString distance = values[1].trimmed();
    trimPyMarkers(detname);
    trimPyMarkers(distance);
  
    QLabel *lbl = m_s2d_detlabels[wscode].value(detname);
    if( lbl ) lbl->setText(distance);
  }
}

/**
 * Set LOQ geometry information
 * @param workspace The workspace to operate on
 */
void SANSRunWindow::setLOQGeometry(Mantid::API::MatrixWorkspace_sptr workspace, int wscode)
{
  if( m_uiForm.inst_opt->currentIndex() == 1 ) return;

  double dist_ms(0.0), dist_mdb(0.0), dist_hab(0.0);
  //Sample
  componentLOQDistances(workspace, dist_ms, dist_mdb, dist_hab);
  
  QHash<QString, QLabel*> & labels = m_loq_detlabels[wscode];
  QLabel *detlabel = labels.value("moderator-sample");
  if( detlabel )
  {
    detlabel->setText(QString::number(dist_ms));
  }

  detlabel = labels.value("sample-main-detector-bank");
  if( detlabel )
  {
    detlabel->setText(QString::number(dist_mdb));
  }

  detlabel = labels.value("sample-HAB");
  if( detlabel )
  {
    detlabel->setText(QString::number(dist_hab));
  }

}

/**
 * Mark an error on a label
 * @param label A pointer to a QLabel instance
 */
void SANSRunWindow::markError(QLabel* label)
{
  if( label )
  {
    label->setText("<font color=\"red\">" + label->text() + "</font>");
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
  if( !browseForFile("Select a user file", m_uiForm.userfile_edit) )
  {
    return;
  }
  
  runReduceScriptFunction("UserPath('" + QFileInfo(m_uiForm.userfile_edit->text()).path() + "')");
  //Set the correct instrument
  runReduceScriptFunction(m_uiForm.inst_opt->itemData(m_uiForm.inst_opt->currentIndex()).toString());

  if( !loadUserFile() )
  {
    m_cfg_loaded = false;
    showInformationBox("Error loading user file '" + m_uiForm.userfile_edit->text() + "',  cannot continue.");
    return;
  }
  //Check for warnings
  checkLogFlags();

  m_cfg_loaded = true;
  m_uiForm.tabWidget->setTabEnabled(1, true);
  m_uiForm.tabWidget->setTabEnabled(2, true);
  m_uiForm.tabWidget->setTabEnabled(3, true);
  

  //path() returns the directory
  m_last_dir = QFileInfo(m_uiForm.userfile_edit->text()).path();
}

/**
 * Select and load a CSV file
 */
void SANSRunWindow::selectCSVFile()
{
  if( !m_cfg_loaded )
  {
    showInformationBox("Please load the relevant user file.");
    return;
  }

  if( !browseForFile("Select CSV file",m_uiForm.csv_filename, "CSV files (*.csv)") )
  {
    return;
  }

  if( !loadCSVFile() )
  {
    return;
  }
  //path() returns the directory
  m_last_dir = QFileInfo(m_uiForm.csv_filename->text()).path();
  if( m_cfg_loaded ) setProcessingState(false, -1);
}

/**
 * Mark that a run number has changed
*/
void SANSRunWindow::runChanged()
{
  m_warnings_issued = false;
  forceDataReload(true);
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
 * Browse for a file and set the text of the given edit box
 * @param box_title The title field for the display box
 * @param A QLineEdit box to use for the file path
 * @param file_filter An optional file filter
 */
bool SANSRunWindow::browseForFile(const QString & box_title, QLineEdit* file_field, QString file_filter)
{
  QString box_text = file_field->text();
  QString start_path = box_text;
  if( box_text.isEmpty() )
  {
    start_path = m_last_dir;
  }
  file_filter += ";;AllFiles (*.*)";
  QString file_path = QFileDialog::getOpenFileName(this, box_title, start_path, file_filter);    
  if( file_path.isEmpty() || QFileInfo(file_path).isDir() ) return false;
  file_field->setText(file_path);
  return true;
}

/**
 * Receive a load button click signal
 */
bool SANSRunWindow::handleLoadButtonClick()
{
  QString origin_dir = QDir::currentPath();
  QString work_dir = QDir(m_uiForm.datadir_edit->text()).absolutePath();
  if( work_dir.isEmpty() || !QDir(work_dir).exists() )
  {
    showInformationBox("The specified data directory " + m_uiForm.datadir_edit->text() + " does not exist.");
    return false;
  }
  if( !work_dir.endsWith('/') ) work_dir += "/";
  m_data_dir = work_dir;
  runReduceScriptFunction("DataPath('" + m_data_dir + "')");

  // Check if we have loaded the data_file
  if( !isUserFileLoaded() )
  {
    showInformationBox("Please load the relevant user file.");
    return false;
  }
  setProcessingState(true, -1);

  if( m_force_reload ) cleanup();

  QString run_number = m_run_no_boxes.value(0)->text();
  if( run_number.isEmpty() )
  {
    showInformationBox("Error: No sample run given, cannot continue.");
    setProcessingState(false, -1);
    return false;
  }

  if(!m_run_no_boxes.value(3)->text().isEmpty() && m_run_no_boxes.value(6)->text().isEmpty() )
  {
    showInformationBox("Error: Can run supplied without direct run, cannot continue.");
    setProcessingState(false, -1);
    return false;
  }

  QString sample_logs, can_logs;
  bool is_loaded(true);  
  //Quick check that there is a can direct run if a trans can is defined. If not use the sample one
  if( !m_run_no_boxes.value(4)->text().isEmpty() && m_run_no_boxes.value(7)->text().isEmpty() )
  {
    m_run_no_boxes.value(7)->setText(m_run_no_boxes.value(6)->text());
  }

  QHashIterator<int, QLineEdit*> itr(m_run_no_boxes);
  while( itr.hasNext() )
  {
    itr.next();
    int key = itr.key();
    // Skip background as we are not using those at the moment.
    if( key == 2 ) continue;
    if( key == 5 ) break;
    QString run_no = itr.value()->text();
    QString logs;
    if( run_no.isEmpty() ) 
    {
      m_workspace_names.insert(key, "");
      //Clear any that are assigned
      runAssign(key, logs);
      continue;
    }
    is_loaded &= runAssign(key, logs);
    if( !is_loaded )
    {
      showInformationBox("Error: Problem loading run \"" + run_no + "\", please check log window for details.");
      break;
    }
    if( key == 0 ) 
    { 
      sample_logs = logs;
      if( m_uiForm.inst_opt->currentIndex() == 1 && sample_logs.isEmpty() )
      {
        is_loaded = false;
        showInformationBox("Error: Cannot find log file for sample run, cannot continue.");
        break;
      }
    }
    if( key == 1 ) 
    { 
      can_logs = logs;
      if( m_uiForm.inst_opt->currentIndex() == 1 && can_logs.isEmpty() )
      {
        can_logs = sample_logs;
        showInformationBox("Warning: Cannot find log file for can run, using sample values.");
      }
    }
  }
  if (!is_loaded) 
  {
    setProcessingState(false, -1);
    return false;
  }

  // Sort out the log information
  setGeometryDetails(sample_logs, can_logs);
  
  // Enter information from sample workspace on to analysis and geometry tab
  Mantid::API::MatrixWorkspace_sptr sample_workspace;
  try
  {
    sample_workspace = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>
     (Mantid::API::AnalysisDataService::Instance().retrieve(getWorkspaceName(0).toStdString()));
  }
  catch(std::runtime_error &)
  {
    setProcessingState(false, -1);
    showInformationBox("Error: Could not retrieve sample workspace from Mantid");
    return false;
  }
  
  if( sample_workspace != boost::shared_ptr<Mantid::API::MatrixWorkspace>() && !sample_workspace->readX(0).empty() )
  {
    m_uiForm.tof_min->setText(QString::number(sample_workspace->readX(0).front())); 
    m_uiForm.tof_max->setText(QString::number(sample_workspace->readX(0).back()));
  }

  // Set the geometry
  //boost::shared_ptr<Mantid::API::Sample> sample_details = sample_workspace->getSample();
  int geomid  = sample_workspace->sample().getGeometryFlag();
  if( geomid > 0 && geomid < 4 )
  {
    m_uiForm.sample_geomid->setCurrentIndex(geomid - 1);
    m_uiForm.sample_thick->setText(QString::number(sample_workspace->sample().getThickness()));
    m_uiForm.sample_width->setText(QString::number(sample_workspace->sample().getWidth()));
    m_uiForm.sample_height->setText(QString::number(sample_workspace->sample().getHeight()));
  }
  else
  {
    m_uiForm.sample_geomid->setCurrentIndex(2);
    m_uiForm.sample_thick->setText("1");
    m_uiForm.sample_width->setText("8");
    m_uiForm.sample_height->setText("8");
    //Warn user
    raiseOneTimeMessage("Warning: Incorrect geometry flag encountered: " + QString::number(geomid) +". Using default values.", 2);
  }

  forceDataReload(false);


  for( int index = 1; index < m_uiForm.tabWidget->count(); ++index )
  {
    m_uiForm.tabWidget->setTabEnabled(index, true);
  }
 
  setProcessingState(false, -1);
  return true;
}

/** 
 * Construct the python code to perform the analysis based on the 
 * current settings
 * @param type The reduction type: 1D or 2D
 */
QString SANSRunWindow::createAnalysisDetailsScript(const QString & type)
{
  //Construct a run script based upon the current values within the various widgets
  QString exec_reduce = m_uiForm.inst_opt->itemData(m_uiForm.inst_opt->currentIndex()).toString() + 
      "\nDetector('" + m_uiForm.detbank_sel->currentText() + "')\n";

  //Add the path in the single mode data box if it is not empty
  QString data_path = m_uiForm.datadir_edit->text();
  if( !data_path.isEmpty() )
  {
    exec_reduce += "DataPath('" + data_path + "')\n";
  }

  if( type.startsWith("1D") )
  {
    exec_reduce += "Set1D()\n";
  }
  else
  {
    exec_reduce += "Set2D()\n";
  }
  //Analysis details
  exec_reduce += 
    "LimitsR(" + m_uiForm.rad_min->text() + "," + m_uiForm.rad_max->text() + ")\n" +
    "LimitsWav(" + m_uiForm.wav_min->text() + "," + m_uiForm.wav_max->text() + "," + 
        m_uiForm.wav_dw->text() + ",'" + m_uiForm.wav_dw_opt->itemData(m_uiForm.wav_dw_opt->currentIndex()).toString() + "')\n" +
    "LimitsQ(" + m_uiForm.q_min->text() + "," + m_uiForm.q_max->text() + "," + 
        m_uiForm.q_dq->text() + ",'" + m_uiForm.q_dq_opt->itemData(m_uiForm.q_dq_opt->currentIndex()).toString() + "')\n" +
    "LimitsQXY(0.0," + m_uiForm.qy_max->text() + "," + 
        m_uiForm.qy_dqy->text() + ",'" + m_uiForm.qy_dqy_opt->itemData(m_uiForm.qy_dqy_opt->currentIndex()).toString() + "')\n" +
    "LimitsPhi(" + m_uiForm.phi_min->text() + "," + m_uiForm.phi_max->text() + ")\n";  

  //Centre values
  exec_reduce += "SetCentre(" + m_uiForm.beam_x->text() + "," + m_uiForm.beam_y->text() + ")\n";
  //Gravity correction
  exec_reduce += "Gravity(";
  if( m_uiForm.gravity_check->isChecked() )
  {
    exec_reduce += "True)\n";
  }
  else
  {
    exec_reduce += "False)\n";
  }
  //Sample offset
  exec_reduce += "SetSampleOffset(" + m_uiForm.smpl_offset->text() + ")\n";
  //Monitor spectrum
  exec_reduce += "SetMonitorSpectrum(" + m_uiForm.monitor_spec->text() + ")\n";
  //Extra mask information
  addUserMaskStrings(exec_reduce);

  //Set geometry info
  exec_reduce += 
    "SampleHeight(" + m_uiForm.sample_height->text() + ")\n" + 
    "SampleWidth(" + m_uiForm.sample_width->text() + ")\n" + 
    "SampleThickness(" + m_uiForm.sample_thick->text() + ")\n"
    "SampleGeometry(" + m_uiForm.sample_geomid->currentText().at(0) + ")\n";
 
  return exec_reduce;
}


/**
 * Run the analysis script
 * @param type The data reduction type, 1D or 2D
 */
void SANSRunWindow::handleReduceButtonClick(const QString & type)
{
  QString py_code = createAnalysisDetailsScript(type);
  if( py_code.isEmpty() )
  {
    showInformationBox("Error: An error occurred while constructing the reduction code, please check installation.");
    return;
  }
  QString trans_behav;
  if( m_uiForm.def_trans->isChecked() )
  {
    trans_behav += "DefaultTrans";
  }
  else
  {
    trans_behav += "NewTrans";
  }

  //Need to check which mode we're in
  if( m_uiForm.single_mode_btn->isChecked() )
  {
    //Currently the components are moved with each reduce click. Check if a load is necessary
    handleLoadButtonClick();
    py_code += "\nreduced = WavRangeReduction(use_def_trans=" + trans_behav + ")\n";
    if( m_uiForm.plot_check->isChecked() )
    {
      py_code += "PlotResult(reduced)\n";
    }
  }
  else
  {
    //Have we got anything to reduce?
    if( m_uiForm.batch_table->rowCount() == 0 )
    {
      showInformationBox("Error: No run information specified.");
      return;
    }

    QString csv_file(m_uiForm.csv_filename->text());
    if( m_dirty_batch_grid )
    {
      QString selected_file = QFileDialog::getSaveFileName(this, "Save as CSV", m_last_dir);
      csv_file = saveBatchGrid(selected_file);
    }
    py_code = "import SANSBatchMode as batch\n" + py_code;
    py_code += "\nbatch.BatchReduce('" + csv_file + "','" + m_uiForm.file_opt->itemData(m_uiForm.file_opt->currentIndex()).toString() + "',"
      + trans_behav;
    if( m_uiForm.plot_check->isChecked() )
    {
      py_code += ", plotresults = True";
    }
    if( m_uiForm.log_colette->isChecked() )
    {
      py_code += ", verbose = True";
    }
    py_code += ")";
  }

  int idtype(0);
  if( type.startsWith("2") ) idtype = 1;
  //Disable buttons so that interaction is limited while processing data
  setProcessingState(true, idtype);
  m_lastreducetype = idtype;

  //Execute the code
  runPythonCode(py_code, false);
  // Mark that a reload is necessary to rerun the same reduction
  forceDataReload();
  //Reenable stuff
  setProcessingState(false, idtype);

  //If we used a temporary file in batch mode, remove it
  if( m_uiForm.batch_mode_btn->isChecked() && !m_tmp_batchfile.isEmpty() )
  {
    QFile tmp_file(m_tmp_batchfile);
    tmp_file.remove();
  }
  checkLogFlags();
}

/**
 * Plot button slot (deprecated)
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
  if( m_uiForm.beamstart_box->currentIndex() == 1 && (m_uiForm.beam_x->text().isEmpty() || m_uiForm.beam_y->text().isEmpty()) )
  {
    showInformationBox("Current centre postion is invalid, please check input.");
    return;
  }

  // Start iteration
  updateCentreFindingStatus("::SANS::Loading data");
  handleLoadButtonClick();

  // Disable interaction
  setProcessingState(true, 0);

  // This checks whether we have a sample run and that it has been loaded
  QString py_code = createAnalysisDetailsScript("1D");
  if( py_code.isEmpty() )
  {
    setProcessingState(false, 0);
    return;
  }

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

  //Find centre function
  py_code += "FindBeamCentre(rlow=" + m_uiForm.beam_rmin->text() + ",rupp=" + m_uiForm.beam_rmax->text() + 
      ",MaxIter=" + m_uiForm.beam_iter->text() + ",";


  if( m_uiForm.beamstart_box->currentIndex() == 0 )
  {
    py_code += "xstart = None, ystart = None)\n";
  }
  else
  {
    py_code += "xstart=float(" + m_uiForm.beam_x->text() + ")/1000.,ystart=float(" + m_uiForm.beam_y->text() + ")/1000.),\n";
  }

  updateCentreFindingStatus("::SANS::Iteration 1");
  m_uiForm.beamstart_box->setFocus();

  //Execute the code
  //Connect up the logger to handle updating the centre finding status box
  connect(this, SIGNAL(logMessageReceived(const QString&)), this, 
	  SLOT(updateCentreFindingStatus(const QString&)));
  disconnect(this, SIGNAL(logMessageReceived(const QString&)), this, SLOT(updateLogWindow(const QString&)));
  
  runReduceScriptFunction(py_code);
  
  disconnect(this, SIGNAL(logMessageReceived(const QString&)), this, 
	     SLOT(updateCentreFindingStatus(const QString&)));
  connect(this, SIGNAL(logMessageReceived(const QString&)), this, SLOT(updateLogWindow(const QString&)));

  QString coordstr = runReduceScriptFunction("printParameter('XBEAM_CENTRE');printParameter('YBEAM_CENTRE')\n");
  
  QString result("");
  if( coordstr.isEmpty() )
  {
    result = "::SANS::No coordinates returned!";
  }
  else
  {
    //Remove all internal whitespace characters and replace with single space
    coordstr = coordstr.simplified();
    QStringList xycoords = coordstr.split(" ");
    if( xycoords.count() == 2 )
    {
      double coord = xycoords[0].toDouble();
      m_uiForm.beam_x->setText(QString::number(coord*1000.));
      coord = xycoords[1].toDouble();
      m_uiForm.beam_y->setText(QString::number(coord*1000.));
      result = "::SANS::Coordinates updated";
    }
    else
    {
      result = "::SANS::Incorrect number of parameters returned from function, check script.";

    }
  }
  updateCentreFindingStatus(result);
  
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
  QString analysis_script = createAnalysisDetailsScript("1D");
  analysis_script += "\nViewCurrentMask()";

  m_uiForm.showMaskBtn->setEnabled(false);
  m_uiForm.showMaskBtn->setText("Working...");

  runReduceScriptFunction(analysis_script);

  m_uiForm.showMaskBtn->setEnabled(true);
  m_uiForm.showMaskBtn->setText("Display mask");

}

/**
 * A different instrument has been selected
 */
void SANSRunWindow::handleInstrumentChange(int index)
{
  if( index == 0 ) 
  {
    m_uiForm.detbank_sel->setItemText(0, "main-detector-bank");
    m_uiForm.detbank_sel->setItemText(1, "HAB");
    m_uiForm.beam_rmin->setText("60");
    m_uiForm.beam_rmax->setText("200");
    
    m_uiForm.geom_stack->setCurrentIndex(0);

    // Set allowed extensions
    m_uiForm.file_opt->clear();
    m_uiForm.file_opt->addItem("raw", QVariant(".raw"));
  }
  else
  { 
    m_uiForm.detbank_sel->setItemText(0, "rear-detector");
    m_uiForm.detbank_sel->setItemText(1, "front-detector");
    m_uiForm.beam_rmin->setText("60");
    m_uiForm.beam_rmax->setText("280");

    m_uiForm.geom_stack->setCurrentIndex(1);

    //File extensions
    m_uiForm.file_opt->clear();
    m_uiForm.file_opt->addItem("raw", QVariant(".raw"));
    m_uiForm.file_opt->addItem("nexus", QVariant(".nxs"));
  }
  m_cfg_loaded = false;
}

/**
 * Update the centre finding status label
 * @param msg The message string
 */
void SANSRunWindow::updateCentreFindingStatus(const QString & msg)
{
  static QString prefix = "::SANS";
  if( msg.startsWith(prefix) )
  {
    QStringList sections = msg.split("::");
    QString txt = sections.at(2);
    m_uiForm.centre_logging->append(txt);
    if( sections.at(1) == "SANSIter" )
    {
      m_uiForm.centre_stat->setText(txt);
    }
  }  
}

/**
 * Update the logging window with status messages
 * @param msg The message received
 */
void SANSRunWindow::updateLogWindow(const QString & msg)
{
  static QString prefix = "::SANS";
  if( msg.startsWith(prefix) )
  {
    QString txt = msg.section("::",2);
    bool logwarnings = txt.contains("warning", Qt::CaseInsensitive);
    if( m_uiForm.verbose_check->isChecked() || logwarnings || m_uiForm.log_colette->isChecked() )
    {
      if( logwarnings )
      {
	m_log_warnings = true;
	m_uiForm.logging_field->setTextColor(Qt::red);
      }
      else
      {
	m_uiForm.logging_field->setTextColor(Qt::black);
      }
      m_uiForm.logging_field->append(txt);
    }
  }
}

/**
* Switch between run modes
* @param mode_id Indicates which toggle has been pressed
*/
void SANSRunWindow::switchMode(int mode_id)
{
  if( mode_id == SANSRunWindow::SingleMode )
  {
    m_uiForm.mode_stack->setCurrentIndex(0);
    m_uiForm.load_dataBtn->setEnabled(true);
  }
  else if( mode_id == SANSRunWindow::BatchMode )
  {
    m_uiForm.mode_stack->setCurrentIndex(1);
    m_uiForm.load_dataBtn->setEnabled(false);
  }
  else {}
}

/**
 * Paste to the batch table
 */
void SANSRunWindow::pasteToBatchTable()
{
  if( !m_cfg_loaded )
  {
    showInformationBox("Please load the relevant user file before continuing.");
    return;
  }

  QClipboard *clipboard = QApplication::clipboard();
  QString copied_text = clipboard->text();
  if( copied_text.isEmpty() ) return;
  
  QStringList runlines = copied_text.split("\n");
  QStringListIterator sitr(runlines);
  int errors(0);
  while( sitr.hasNext() )
  {
    QString line = sitr.next().simplified();
    if( !line.isEmpty() )
    {
      errors += addBatchLine(line);
    }
  }
  if( errors > 0 )
  {
    showInformationBox("Warning: " + QString::number(errors) + " malformed lines detected in pasted text. Lines skipped.");
  }
  if( m_uiForm.batch_table->rowCount() > 0 )
  {
    m_dirty_batch_grid = true;
    setProcessingState(false, -1);
  }
}

/**
 * Clear the batch table
 */
void SANSRunWindow::clearBatchTable()
{
  int row_count = m_uiForm.batch_table->rowCount();
  for( int i = row_count - 1; i >= 0; --i )
  {
    m_uiForm.batch_table->removeRow(i);
  }
  m_dirty_batch_grid = false;
  m_tmp_batchfile = "";
}

/**
 * Handle a verbose mode check box state change
 * state The new state
 */
void SANSRunWindow::verboseMode(int state)
{
  if( state == Qt::Checked )
  {
    runReduceScriptFunction("SetVerboseMode(True)");
  }
  else if( state == Qt::Unchecked )
  {
    runReduceScriptFunction("SetVerboseMode(False)");
  }
  else {}
}

/** 
 * Run a SANS assign command
 * @param key The key of the edit box to assign from
 * @param logs An output parameter specifying the log data
 */
bool SANSRunWindow::runAssign(int key, QString & logs)
{
  //Work out if sans/trans and sample/can
  bool is_trans(false);
  if( key > 2 && key < 6 )
  {
    is_trans = true;
  }
  bool is_can(false);
  if( key == 1 || key == 4 )
  {
    is_can = true;
  }
  
  // Default extension if the box run number does not contain one
  QString extension = m_uiForm.file_opt->itemData(m_uiForm.file_opt->currentIndex()).toString();
  QString run_number = m_run_no_boxes.value(key)->text();
  if( QFileInfo(run_number).completeSuffix().isEmpty() )
  {
    if( run_number.endsWith(".") ) 
    {
      run_number.chop(1);
    }
    run_number += extension;
  }
  bool status(true);
  if( is_trans )
  {
    QString direct_run = m_run_no_boxes.value(key + 3)->text();
    if( QFileInfo(direct_run).completeSuffix().isEmpty() )
    {
      if( direct_run.endsWith(".") ) 
      {
        direct_run.chop(1);
      }
      direct_run += extension;
    }
    QString assign_fn;
    if( is_can )
    {
      assign_fn = "TransmissionCan";
    }
    else
    {
      assign_fn = "TransmissionSample";
    }
    assign_fn += "('" + run_number + "','" + direct_run + "')";
    QString ws_names = runReduceScriptFunction("t1, t2 = " + assign_fn + ";print t1,t2");
    QString trans_ws = ws_names.section(" ", 0,0);
    QString direct_ws = ws_names.section(" ", 1);
    status = setNumberPeriods(key, trans_ws);
    status &= setNumberPeriods(key + 3, direct_ws);
    if( status )
    {
      m_workspace_names.insert(key, trans_ws);
      m_workspace_names.insert(key + 3, direct_ws);
    }
  }
  else
  {
    QString assign_fn;
    if( is_can )
    {
      assign_fn = "AssignCan";
    }
    else
    {
      assign_fn = "AssignSample";
    }
    assign_fn += "('" + run_number + "', reload = True)";
    QString run_info = runReduceScriptFunction("t1, t2 = " + assign_fn + ";print t1,t2");
    QString base_workspace = run_info.section(" ",0,0);
    logs = run_info.section(" ", 1);
    if( !logs.isEmpty() )
    {
      trimPyMarkers(logs);
    }
    status = setNumberPeriods(key, base_workspace);
    if( status )
    {
      m_workspace_names.insert(key, base_workspace);
    }
  }
  return status;
}

 /** 
  * Set number of periods for the given workspace
  * @param key The box this applies to
  * @param workspace_name The name of the workspace to check
  * @returns A boolean indicating success/failure
  */
bool SANSRunWindow::setNumberPeriods(int key, const QString & workspace_name)
{
  int nperiods(0);
  QLabel *label = qobject_cast<QLabel*>(m_period_lbls.value(key));
  QLineEdit *userentry = qobject_cast<QLineEdit*>(label->buddy());
  bool is_loaded(true);
  using namespace Mantid::API;
  if( workspaceExists(workspace_name) )
  {
    Mantid::API::Workspace_sptr wksp = Mantid::API::AnalysisDataService::Instance().retrieve(workspace_name.toStdString()); 
    if( boost::shared_ptr<WorkspaceGroup> ws_group = boost::dynamic_pointer_cast<WorkspaceGroup>(wksp) )
    {
      nperiods = ws_group->getNames().size();
    }
    else
    {
      nperiods = 1;
    }
    label->setText("/ " + QString::number(nperiods));
    userentry->setText("1");
  }
  else
  {
    nperiods = 0;
    userentry->clear();
    label->setText("/ ??");
    is_loaded = false;
  }
  return is_loaded;
}

/**
 * Get a properly qualified workspace name for the given key
 */
QString SANSRunWindow::getWorkspaceName(int key)
{
  QString name = m_workspace_names.value(key);
  if( !name.isEmpty() )
  {
    QString period = qobject_cast<QLineEdit*>(m_period_lbls.value(key)->buddy())->text();
    if( period != "1" )
    {
      name += "_" + period;
    }
  }
  return name;
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
      forceDataReload();
      return;
    }
  }
}

/**
 * Format a double as a string
 * @param value The double to convert to a string
 * @param colour The colour
 * @param format The format char
 * @param precision The precision
 */
QString SANSRunWindow::formatDouble(double value, const QString & colour, char format, int precision)
{
  return QString("<font color='") + colour + QString("'>") + QString::number(value, format, precision)  + QString("</font>");
}

/**
 * Raise a message if current status allows
 * @param msg The message to include in the box
 * @param index The tab index to set as current
*/
void SANSRunWindow::raiseOneTimeMessage(const QString & msg, int index)
{
  if( m_warnings_issued ) return;
  if( index >= 0 )
  {
    m_uiForm.tabWidget->setCurrentIndex(index);
  }
  showInformationBox(msg);
  m_warnings_issued = true;
}


/**
 * Rest the geometry details box 
 */
void SANSRunWindow::resetGeometryDetailsBox()
{
  QString blank("-");
  //LOQ
  m_uiForm.dist_mod_mon->setText(blank);

  //SANS2D
  m_uiForm.dist_mon_s2d->setText(blank);
  m_uiForm.dist_sample_ms_s2d->setText(blank);
  m_uiForm.dist_can_ms_s2d->setText(blank);
  m_uiForm.dist_bkgd_ms_s2d->setText(blank);

  for(int i = 0; i < 3; ++i )
  {
    //LOQ
    QMutableHashIterator<QString,QLabel*> litr(m_loq_detlabels[i]);
    while(litr.hasNext())
    {
      litr.next();
      litr.value()->setText(blank);
    }
    //SANS2D
    QMutableHashIterator<QString,QLabel*> sitr(m_s2d_detlabels[i]);
    while(sitr.hasNext())
    {
      sitr.next();
      sitr.value()->setText(blank);
    }
  }
  
}

void SANSRunWindow::cleanup()
{
  Mantid::API::AnalysisDataServiceImpl & ads = Mantid::API::AnalysisDataService::Instance();
  std::set<std::string> workspaces = ads.getObjectNames();
  std::set<std::string>::const_iterator iend = workspaces.end();
  for( std::set<std::string>::const_iterator itr = workspaces.begin(); itr != iend; ++itr )
  {
    QString name = QString::fromStdString(*itr);
    if( name.endsWith("_raw") || name.endsWith("_nxs"))
    {
      ads.remove(*itr);
    }
  }
}

/**
 * Add a csv line to the batch grid
 * @param csv_line Add a line of csv text to the grid 
 * @param separator An optional separator, default = ","
*/
int SANSRunWindow::addBatchLine(QString csv_line, QString separator)
{
  //Try to detect separator if one is not specified
  if( separator.isEmpty() )
  {
    if( csv_line.contains(",") )
    {
      separator = ",";
    }
    else
    {
      separator = " ";
    }
  }
  QStringList elements = csv_line.split(separator);
  //Insert new row
  int row = m_uiForm.batch_table->rowCount();
  m_uiForm.batch_table->insertRow(row);

  int nelements = elements.count() - 1;
  bool error(false);
  for( int i = 0; i < nelements; )
  {
    QString cola = elements.value(i);
    QString colb = elements.value(i+1);
    if( m_allowed_batchtags.contains(cola) )
    {
      if( !m_allowed_batchtags.contains(colb) )
      {
        if( !colb.isEmpty() && !cola.contains("background") )
        {
          m_uiForm.batch_table->setItem(row, m_allowed_batchtags.value(cola), new QTableWidgetItem(colb));
        }
        i += 2;        
      }
      else
      {
        ++i;
      }
    }
    else
    {
      error = true;
      break;
    }
  }
  if( error ) 
  {
    m_uiForm.batch_table->removeRow(row);
    return 1;
  }
  return 0;
}

/**
 * Save the batch file to a CSV file.
 * @param filename An optional filename. If none is given then a temporary file is used and its name returned 
*/
QString SANSRunWindow::saveBatchGrid(const QString & filename)
{
  QString csv_filename = filename;
  if( csv_filename.isEmpty() )
  {
    //Generate a temporary filename
    QTemporaryFile tmp;
    tmp.open();
    csv_filename = tmp.fileName();
    tmp.close();
    m_tmp_batchfile = csv_filename;
  }

  QFile csv_file(csv_filename);
  if( !csv_file.open(QIODevice::WriteOnly|QIODevice::Text) )
  {
    showInformationBox("Error: Cannot write to CSV file \"" + csv_filename + "\".");
    return "";
  }
  
  QTextStream out_strm(&csv_file);
  int nrows = m_uiForm.batch_table->rowCount();
  const QString separator(",");
  for( int r = 0; r < nrows; ++r )
  {
    for( int c = 0; c < 7; ++c )
    {
      out_strm << m_allowed_batchtags.key(c) << separator;
      if( QTableWidgetItem* item = m_uiForm.batch_table->item(r, c) )
      {
        out_strm << item->text();
      }
      if( c < 6 ) out_strm << separator; 
    }
    out_strm << "\n";
  }
  csv_file.close();
  if( !filename.isEmpty() )
  {
    m_tmp_batchfile = "";
    m_dirty_batch_grid = false;
    m_uiForm.csv_filename->setText(csv_filename);
  }
  else
  {
     m_uiForm.csv_filename->clear();
  }
  return csv_filename;
}

void SANSRunWindow::checkLogFlags()
{
  if( m_log_warnings )
  {
    showInformationBox("Warning messages occurred during previous operation, see log for details.");
  }
  m_log_warnings = false;
}
