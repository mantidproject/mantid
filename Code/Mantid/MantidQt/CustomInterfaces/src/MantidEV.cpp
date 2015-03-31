
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <Poco/Path.h>

#include "MantidQtCustomInterfaces/MantidEV.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IEventWorkspace.h"


namespace MantidQt
{
namespace CustomInterfaces
{

//Register the class with the factory
DECLARE_SUBWINDOW(MantidEV)

using namespace Mantid::Kernel;
using namespace Mantid::API;


/**
 *  Class to call loadEvents in a separate thread.
 */
RunLoadAndConvertToMD::RunLoadAndConvertToMD(MantidEVWorker * worker,
                                             const std::string & file_name,
                                             const std::string & ev_ws_name,
                                             const std::string & md_ws_name,
                                             const double        minQ,
                                             const double        maxQ,
                                             const bool          do_lorentz_corr,
                                             const bool          load_data,
                                             const bool          load_det_cal,
                                             const std::string & det_cal_file,
                                             const std::string & det_cal_file2 ) :
  worker(worker),
  file_name(file_name), ev_ws_name(ev_ws_name), md_ws_name(md_ws_name),
  minQ(minQ), maxQ(maxQ), do_lorentz_corr(do_lorentz_corr),
  load_data(load_data), load_det_cal(load_det_cal),
  det_cal_file(det_cal_file), det_cal_file2(det_cal_file2)
{
}

void RunLoadAndConvertToMD::run()
{
  worker->loadAndConvertToMD( file_name, ev_ws_name, md_ws_name,
                              minQ, maxQ, do_lorentz_corr, load_data,
                              load_det_cal, det_cal_file, det_cal_file2 );
}

/**
 * Class to call findPeaks in a separate thread.
 */
RunFindPeaks::RunFindPeaks(        MantidEVWorker * worker,
                            const std::string     & ev_ws_name,
                            const std::string     & md_ws_name,
                            const std::string     & peaks_ws_name,
                                  double            max_abc,
                                  size_t            num_to_find,
                                  double            min_intensity )
{
  this->worker        = worker;
  this->ev_ws_name    = ev_ws_name;
  this->md_ws_name    = md_ws_name;
  this->peaks_ws_name = peaks_ws_name;
  this->max_abc       = max_abc;
  this->num_to_find   = num_to_find;
  this->min_intensity = min_intensity;
}


/**
 *  Class to call findPeaks in a separate thread.
 */
void RunFindPeaks::run()
{
  worker->findPeaks( ev_ws_name, md_ws_name, peaks_ws_name,
                     max_abc, num_to_find, min_intensity );
}

/**
 * Class to call predictPeaks in a separate thread.
 */
RunPredictPeaks::RunPredictPeaks(        MantidEVWorker * worker,
                            const std::string     & peaks_ws_name,
                                  double            min_pred_wl,
                                  double            max_pred_wl,
                                  double            min_pred_dspacing,
                                  double            max_pred_dspacing )
{
  this->worker        = worker;
  this->peaks_ws_name = peaks_ws_name;
  this->min_pred_wl       = min_pred_wl;
  this->max_pred_wl       = max_pred_wl;
  this->min_pred_dspacing = min_pred_dspacing;
  this->max_pred_dspacing = max_pred_dspacing;
}


/**
 *  Class to call predictPeaks in a separate thread.
 */
void RunPredictPeaks::run()
{
  worker->predictPeaks( peaks_ws_name, min_pred_wl, max_pred_wl, min_pred_dspacing, max_pred_dspacing );
}

/**
 *  Class to call sphereIntegrate in a separate thread.
 */
RunSphereIntegrate::RunSphereIntegrate(       MantidEVWorker * worker,
                                        const std::string    & peaks_ws_name,
                                        const std::string    & event_ws_name,
                                              double           peak_radius,
                                              double           inner_radius,
                                              double           outer_radius,
                                              bool             integrate_edge,
                                              bool          use_cylinder_integration,
                                              double        cylinder_length,
                                              double        cylinder_percent_bkg,
                                        const std::string & cylinder_profile_fit)

{ 
  this->worker         = worker;
  this->peaks_ws_name  = peaks_ws_name;
  this->event_ws_name  = event_ws_name;
  this->peak_radius    = peak_radius;
  this->inner_radius   = inner_radius;
  this->outer_radius   = outer_radius;
  this->integrate_edge = integrate_edge;
  this->use_cylinder_integration = use_cylinder_integration;
  this->cylinder_length = cylinder_length;
  this->cylinder_percent_bkg = cylinder_percent_bkg;
  this->cylinder_profile_fit = cylinder_profile_fit;
}


/**
 *  Class to call sphereIntegrate in a separate thread.
 */
void RunSphereIntegrate::run()
{ 
  worker->sphereIntegrate( peaks_ws_name, event_ws_name,
                           peak_radius, inner_radius, outer_radius,
                           integrate_edge, use_cylinder_integration,
                           cylinder_length, cylinder_percent_bkg,
                           cylinder_profile_fit);
}


/**
 *  Class to call fitIntegrate in a separate thread.
 */
RunFitIntegrate::RunFitIntegrate(       MantidEVWorker * worker,
                                  const std::string    & peaks_ws_name,
                                  const std::string    & event_ws_name,
                                  const std::string    & rebin_params,
                                        size_t           n_bad_edge_pix,
                                        bool             use_ikeda_carpenter )
{
  this->worker              = worker;
  this->peaks_ws_name       = peaks_ws_name;
  this->event_ws_name       = event_ws_name;
  this->rebin_params        = rebin_params;
  this->n_bad_edge_pix      = n_bad_edge_pix;
  this->use_ikeda_carpenter = use_ikeda_carpenter;
}


/**
 *  Class to call fitIntegrate in a separate thread.
 */
void RunFitIntegrate::run()
{
  worker->fitIntegrate( peaks_ws_name, event_ws_name,
                        rebin_params, n_bad_edge_pix, use_ikeda_carpenter );
}


/**
 *  Class to call ellipsoidIntegrate in a separate thread.
 */
RunEllipsoidIntegrate::RunEllipsoidIntegrate(   MantidEVWorker * worker,
                                          const std::string    & peaks_ws_name,
                                          const std::string    & event_ws_name,
                                                double           region_radius,
                                                bool             specify_size,
                                                double           peak_size,
                                                double           inner_size,
                                                double           outer_size )
{
  this->worker        = worker;
  this->peaks_ws_name = peaks_ws_name;
  this->event_ws_name = event_ws_name;
  this->region_radius = region_radius;
  this->specify_size  = specify_size;
  this->peak_size     = peak_size;
  this->inner_size    = inner_size;
  this->outer_size    = outer_size;
}


/**
 *  Class to call ellipsoidIntegrate in a separate thread.
 */
void RunEllipsoidIntegrate::run()
{
  worker->ellipsoidIntegrate( peaks_ws_name, event_ws_name,
                              region_radius, specify_size,
                              peak_size, inner_size, outer_size );
}


/// 
/// Start of the MantidEV class 
///


/**
 *  Constructor for MantidEV.  Makes the thread pool and instance of
 *  MantidEVWorker.
 */
MantidEV::MantidEV(QWidget *parent) : UserSubWindow(parent)
{
  last_Q        = V3D(0,0,0);
  worker        = new MantidEVWorker();
  m_thread_pool = new QThreadPool( this );
  m_thread_pool->setMaxThreadCount(1);
   
  QObject::connect( &(MantidQt::API::SelectionNotificationService::Instance()),
                    SIGNAL( QPointSelection_signal( bool, double, double, double )),
                    this, SLOT(QPointSelection_slot( bool, double, double, double )) );
}


/**
 *  Destructor for MantidEV.  Deletes the thread pool and instance of
 *  MantidEVWorker.
 */
MantidEV::~MantidEV()
{
  saveSettings("");
  delete worker;
  delete m_thread_pool;
}


/**
 *  This method is called by the super class to initialize the GUI.
 */
void MantidEV::initLayout()
{
  m_uiForm.setupUi(this);

                          // connect the apply buttons to the code that
                          // gathers the parameters and will call a method
                          // to carry out the requested action

  // apply button on "Select Data" tab
  QObject::connect( m_uiForm.ApplySelectData_btn, SIGNAL(clicked()),
                    this, SLOT(selectWorkspace_slot()) );

   // browse button for event filename
   QObject::connect( m_uiForm.SelectEventFile_btn, SIGNAL(clicked()),
                     this, SLOT(loadEventFile_slot()) );

   QObject::connect( m_uiForm.SelectCalFile_btn, SIGNAL(clicked()),
                     this, SLOT(selectDetCalFile_slot()) );

   QObject::connect( m_uiForm.SelectCalFile2_btn, SIGNAL(clicked()),
                     this, SLOT(selectDetCalFile2_slot()) );

   QObject::connect( m_uiForm.ApplyFindPeaks_btn, SIGNAL(clicked()),
                    this, SLOT(findPeaks_slot()) );

   QObject::connect( m_uiForm.SelectPeaksFile_btn, SIGNAL(clicked()),
                     this, SLOT(getLoadPeaksFileName_slot()) );

   QObject::connect( m_uiForm.ApplyFindUB_btn, SIGNAL(clicked()),
                    this, SLOT(findUB_slot()) );

   QObject::connect( m_uiForm.SelectUBFile_btn, SIGNAL(clicked()),
                     this, SLOT(getLoadUB_FileName_slot()) );

   QObject::connect( m_uiForm.ApplyChooseCell_btn, SIGNAL(clicked()),
                    this, SLOT(chooseCell_slot()) );

   QObject::connect( m_uiForm.ApplyChangeHKL_btn, SIGNAL(clicked()),
                    this, SLOT(changeHKL_slot()) );

   QObject::connect( m_uiForm.ApplyIntegrate_btn, SIGNAL(clicked()),
                    this, SLOT(integratePeaks_slot()) );

   QObject::connect( m_uiForm.ShowInfo_btn, SIGNAL(clicked()),
                    this, SLOT(showInfo_slot()) );

                          // connect the slots for the menu items
   QObject::connect( m_uiForm.actionSave_State, SIGNAL(triggered()),
                     this, SLOT(saveState_slot()) );

   QObject::connect( m_uiForm.actionLoad_State, SIGNAL(triggered()),
                     this, SLOT(loadState_slot()) );

   QObject::connect( m_uiForm.actionReset_Default_Settings, SIGNAL(triggered()),
                     this, SLOT( setDefaultState_slot()) );

   QObject::connect( m_uiForm.actionSave_Isaw_UB, SIGNAL(triggered()),
                     this, SLOT(saveIsawUB_slot()) );

   QObject::connect( m_uiForm.actionLoad_Isaw_UB, SIGNAL(triggered()),
                     this, SLOT(loadIsawUB_slot()) );

   QObject::connect( m_uiForm.actionSave_Isaw_Peaks, SIGNAL(triggered()),
                     this, SLOT(saveIsawPeaks_slot()) );

   QObject::connect( m_uiForm.actionLoad_Isaw_Peaks, SIGNAL(triggered()),
                     this, SLOT(loadIsawPeaks_slot()) );

   QObject::connect( m_uiForm.actionSave_Nexus_Peaks, SIGNAL(triggered()),
                     this, SLOT(saveNexusPeaks_slot()) );

   QObject::connect( m_uiForm.actionLoad_Nexus_Peaks, SIGNAL(triggered()),
                     this, SLOT(loadNexusPeaks_slot()) );

   QObject::connect( m_uiForm.actionShow_UB, SIGNAL(triggered()),
                     this, SLOT(showUB_slot()) );
   
   QObject::connect( m_uiForm.actionOnline_Help_Page, SIGNAL(triggered()),
                     this, SLOT(help_slot()) );

  QObject::connect( m_uiForm.EventFileName_ledt, SIGNAL(editingFinished()),
                     this, SLOT(loadEventFileEntered_slot()) );

                           // connect the slots for enabling and disabling
                           // various subsets of widgets
   QObject::connect( m_uiForm.   convertToMDGroupBox, SIGNAL(toggled(bool)),
                     this, SLOT( setEnabledLoadEventFileParams_slot(bool) ) );

    QObject::connect( m_uiForm.LoadDetCal_ckbx, SIGNAL(clicked()),
                     this, SLOT( setEnabledLoadCalFiles_slot() ) );

   QObject::connect( m_uiForm.FindPeaks_rbtn, SIGNAL(toggled(bool)),
                     this, SLOT( setEnabledFindPeaksParams_slot(bool) ) );

   QObject::connect( m_uiForm.PredictPeaks_ckbx, SIGNAL(clicked()),
                     this, SLOT( setEnabledPredictPeaksParams_slot() ) );

   QObject::connect( m_uiForm.LoadIsawPeaks_rbtn, SIGNAL(toggled(bool)),
                     this, SLOT( setEnabledLoadPeaksParams_slot(bool) ) );

   QObject::connect( m_uiForm.FindUBUsingFFT_rbtn, SIGNAL(toggled(bool)),
                     this, SLOT( setEnabledFindUBFFTParams_slot(bool) ) );

   QObject::connect( m_uiForm.LoadISAWUB_rbtn, SIGNAL(toggled(bool)),
                     this, SLOT( setEnabledLoadUBParams_slot(bool) ) );

   QObject::connect( m_uiForm.OptimizeGoniometerAngles_ckbx, SIGNAL(clicked()),
                     this, SLOT( setEnabledMaxOptimizeDegrees_slot() ) );

   QObject::connect( m_uiForm.IndexPeaks_ckbx, SIGNAL(clicked(bool)),
                     this, SLOT( setEnabledIndexParams_slot(bool) ) );

   QObject::connect( m_uiForm.ShowPossibleCells_rbtn, SIGNAL(toggled(bool)),
                     this, SLOT( setEnabledShowCellsParams_slot(bool) ) );

   QObject::connect( m_uiForm.SelectCellOfType_rbtn, SIGNAL(toggled(bool)),
                     this, SLOT( setEnabledSetCellTypeParams_slot(bool) ) );

   QObject::connect( m_uiForm.SelectCellWithForm_rbtn, SIGNAL(toggled(bool)),
                     this, SLOT( setEnabledSetCellFormParams_slot(bool) ) );

   QObject::connect( m_uiForm.SphereIntegration_rbtn, SIGNAL(toggled(bool)),
                     this, SLOT( setEnabledSphereIntParams_slot(bool) ) );

   QObject::connect( m_uiForm.TwoDFitIntegration_rbtn, SIGNAL(toggled(bool)),
                     this, SLOT( setEnabledFitIntParams_slot(bool) ) );

   QObject::connect( m_uiForm.EllipsoidIntegration_rbtn, SIGNAL(toggled(bool)),
                     this, SLOT( setEnabledEllipseIntParams_slot(bool) ) );

   QObject::connect( m_uiForm.SpecifySize_ckbx, SIGNAL(clicked(bool)),
                     this, SLOT( setEnabledEllipseSizeOptions_slot() ) );

   // Add validators to all QLineEdit objects that require numeric values
   m_uiForm.MaxMagQ_ledt->setValidator( new QDoubleValidator(m_uiForm.MaxMagQ_ledt));
   m_uiForm.MaxABC_ledt->setValidator( new QDoubleValidator(m_uiForm.MaxABC_ledt));
   m_uiForm.NumToFind_ledt->setValidator( new QDoubleValidator(m_uiForm.NumToFind_ledt));
   m_uiForm.MinIntensity_ledt->setValidator( new QDoubleValidator(m_uiForm.MinIntensity_ledt));
   m_uiForm.MinD_ledt->setValidator( new QDoubleValidator(m_uiForm.MinD_ledt));
   m_uiForm.MaxD_ledt->setValidator( new QDoubleValidator(m_uiForm.MaxD_ledt));
   m_uiForm.FFTTolerance_ledt->setValidator( new QDoubleValidator(m_uiForm.FFTTolerance_ledt));
   m_uiForm.IndexedPeaksTolerance_ledt->setValidator( new QDoubleValidator(m_uiForm.IndexedPeaksTolerance_ledt));
   m_uiForm.MaxGoniometerChange_ledt->setValidator( new QDoubleValidator(m_uiForm.MaxGoniometerChange_ledt));
   m_uiForm.IndexingTolerance_ledt->setValidator( new QDoubleValidator(m_uiForm.IndexingTolerance_ledt));
   m_uiForm.MaxScalarError_ledt->setValidator( new QDoubleValidator(m_uiForm.MaxScalarError_ledt));
   m_uiForm.min_pred_wl_ledt->setValidator( new QDoubleValidator( m_uiForm.min_pred_wl_ledt));
   m_uiForm.max_pred_wl_ledt->setValidator( new QDoubleValidator( m_uiForm.max_pred_wl_ledt));
   m_uiForm.min_pred_dspacing_ledt->setValidator( new QDoubleValidator( m_uiForm.min_pred_dspacing_ledt));
   m_uiForm.max_pred_dspacing_ledt->setValidator( new QDoubleValidator( m_uiForm.max_pred_dspacing_ledt));
   m_uiForm.PeakRadius_ledt->setValidator( new QDoubleValidator(m_uiForm.PeakRadius_ledt));
   m_uiForm.BackgroundInnerRadius_ledt->setValidator( new QDoubleValidator(m_uiForm.BackgroundInnerRadius_ledt));
   m_uiForm.BackgroundOuterRadius_ledt->setValidator( new QDoubleValidator(m_uiForm.BackgroundOuterRadius_ledt));
   m_uiForm.CylinderLength_ledt->setValidator( new QDoubleValidator(m_uiForm.CylinderLength_ledt));
   m_uiForm.CylinderPercentBkg_ledt->setValidator( new QDoubleValidator(m_uiForm.CylinderPercentBkg_ledt));
   m_uiForm.NBadEdgePixels_ledt->setValidator( new QDoubleValidator(m_uiForm.NBadEdgePixels_ledt));
   m_uiForm.RegionRadius_ledt->setValidator( new QDoubleValidator(m_uiForm.RegionRadius_ledt));
   m_uiForm.PeakSize_ledt->setValidator( new QDoubleValidator(m_uiForm.PeakSize_ledt));
   m_uiForm.BackgroundInnerSize_ledt->setValidator( new QDoubleValidator(m_uiForm.BackgroundInnerSize_ledt));
   m_uiForm.BackgroundOuterSize_ledt->setValidator( new QDoubleValidator(m_uiForm.BackgroundOuterSize_ledt));
   m_uiForm.Qx_ledt->setValidator( new QDoubleValidator(m_uiForm.Qx_ledt));
   m_uiForm.Qy_ledt->setValidator( new QDoubleValidator(m_uiForm.Qy_ledt));
   m_uiForm.Qz_ledt->setValidator( new QDoubleValidator(m_uiForm.Qz_ledt));

   setDefaultState_slot();      // call method to set all controls to default state

   loadSettings("");            // reload any previously saved user settings
}

/**
 * Set up default values for the input controls, and set groups of 
 * widgets to be enabled or disabled as needed.
 */
void MantidEV::setDefaultState_slot()
{
   m_uiForm.MantidEV_tabwidg->setCurrentIndex(0);
                                                    // Select Data tab
   m_uiForm.SelectEventWorkspace_ledt->setText("");
   m_uiForm.MDworkspace_ledt->setText("");
   m_uiForm.convertToMDGroupBox->setChecked(true);
   m_uiForm.loadDataGroupBox->setChecked(true);
   m_uiForm.EventFileName_ledt->setText(""); 
   m_uiForm.MaxMagQ_ledt->setText("25");
   m_uiForm.LorentzCorrection_ckbx->setChecked(true);
   setEnabledLoadEventFileParams_slot(true);
   m_uiForm.LoadDetCal_ckbx->setChecked(false);
   setEnabledLoadCalFiles_slot();
   m_uiForm.CalFileName_ledt->setText("");
   m_uiForm.CalFileName2_ledt->setText("");
   last_event_file.clear();
                                                    // Find Peaks tab
   m_uiForm.PeaksWorkspace_ledt->setText("");
   m_uiForm.FindPeaks_rbtn->setChecked(true);
   m_uiForm.MaxABC_ledt->setText("15");
   m_uiForm.NumToFind_ledt->setText("50");
   m_uiForm.MinIntensity_ledt->setText("100");
   m_uiForm.UseExistingPeaksWorkspace_rbtn->setChecked(false);
   m_uiForm.LoadIsawPeaks_rbtn->setChecked(false);
   m_uiForm.SelectPeaksFile_ledt->setText("");
   m_uiForm.PredictPeaks_ckbx->setChecked(false);
   m_uiForm.min_pred_wl_ledt->setText("0.4");
   m_uiForm.max_pred_wl_ledt->setText("3.5");
   m_uiForm.min_pred_dspacing_ledt->setText("0.4");
   m_uiForm.max_pred_dspacing_ledt->setText("8.5");
   setEnabledFindPeaksParams_slot(true);
   setEnabledPredictPeaksParams_slot();
   setEnabledLoadPeaksParams_slot(false);
   last_peaks_file.clear();
                                                    // Find UB tab
   m_uiForm.FindUBUsingFFT_rbtn->setChecked(true);
   m_uiForm.MinD_ledt->setText("3");
   m_uiForm.MaxD_ledt->setText("15");
   m_uiForm.FFTTolerance_ledt->setText("0.12");
   m_uiForm.FindUBUsingIndexedPeaks_rbtn->setChecked(false);
   m_uiForm.IndexedPeaksTolerance_ledt->setText("0.1");
   m_uiForm.LoadISAWUB_rbtn->setChecked(false);
   m_uiForm.SelectUBFile_ledt->setText("");
   m_uiForm.OptimizeGoniometerAngles_ckbx->setChecked(false);
   m_uiForm.MaxGoniometerChange_ledt->setText("5");
   m_uiForm.UseCurrentUB_rbtn->setChecked(false);
   m_uiForm.IndexPeaks_ckbx->setChecked(true);
   m_uiForm.IndexingTolerance_ledt->setText("0.12");
   m_uiForm.RoundHKLs_ckbx->setChecked( true );
   setEnabledFindUBFFTParams_slot(true);
   setEnabledLoadUBParams_slot(false);
   setEnabledMaxOptimizeDegrees_slot();
   setEnabledIndexParams_slot(true);
   last_UB_file.clear();
                                                     // Choose Cell tab
   m_uiForm.ShowPossibleCells_rbtn->setChecked(true);
   m_uiForm.MaxScalarError_ledt->setText("0.2");
   m_uiForm.BestCellOnly_ckbx->setChecked(true);
   m_uiForm.AllowPermutations_ckbx->setChecked(true);
   m_uiForm.SelectCellOfType_rbtn->setChecked(false);
   m_uiForm.CellType_cmbx->setCurrentIndex(0);
   m_uiForm.CellCentering_cmbx->setCurrentIndex(0);
   m_uiForm.SelectCellWithForm_rbtn->setChecked(false);
   m_uiForm.CellFormNumber_cmbx->setCurrentIndex(0);
   setEnabledShowCellsParams_slot(true);
   setEnabledSetCellTypeParams_slot(false);
   setEnabledSetCellFormParams_slot(false);
                                                     // Change HKL tab
   m_uiForm.HKL_tran_row_1_ledt->setText("1, 0, 0");
   m_uiForm.HKL_tran_row_2_ledt->setText("0, 1, 0");
   m_uiForm.HKL_tran_row_3_ledt->setText("0, 0, 1");
                                                     // Integrate tab
   m_uiForm.SphereIntegration_rbtn->setChecked(true);
   m_uiForm.PeakRadius_ledt->setText("0.18");
   m_uiForm.BackgroundInnerRadius_ledt->setText("0.18");
   m_uiForm.BackgroundOuterRadius_ledt->setText("0.23");
   m_uiForm.CylinderLength_ledt->setText("0.40");
   m_uiForm.CylinderPercentBkg_ledt->setText("20.0");
   m_uiForm.IntegrateEdge_ckbx->setChecked(true);
   m_uiForm.Cylinder_ckbx->setChecked(false);
   m_uiForm.CylinderProfileFit_cmbx->setCurrentIndex(5);
   m_uiForm.TwoDFitIntegration_rbtn->setChecked(false);
   m_uiForm.FitRebinParams_ledt->setText("1000,-0.004,16000");
   m_uiForm.NBadEdgePixels_ledt->setText("5");
   m_uiForm.IkedaCarpenter_ckbx->setChecked(false);
   m_uiForm.EllipsoidIntegration_rbtn->setChecked(false);
   m_uiForm.RegionRadius_ledt->setText("0.25");
   m_uiForm.SpecifySize_ckbx->setChecked(false);
   m_uiForm.PeakSize_ledt->setText("0.18");
   m_uiForm.BackgroundInnerSize_ledt->setText("0.18");
   m_uiForm.BackgroundOuterSize_ledt->setText("0.23");
   setEnabledSphereIntParams_slot(true);
   setEnabledFitIntParams_slot(false);
   setEnabledEllipseIntParams_slot(false);
   setEnabledEllipseSizeOptions_slot();
                                                     // Point Info tab
   m_uiForm.Qx_ledt->setText("");
   m_uiForm.Qy_ledt->setText("");
   m_uiForm.Qz_ledt->setText("");
   m_uiForm.SelectedPoint_tbl->clear();
}


/**
 * Go to MantidEV web page when help menu item is chosen
 */
void MantidEV::help_slot()
{
  QDesktopServices::openUrl(QUrl("http://www.mantidproject.org/SCD_Event_Data_Reduction_Interface_(MantidEV)"));
}


/**
 *  Slot called when the Apply button is pressed on the Select Data tab.
 */
void MantidEV::selectWorkspace_slot()
{
   // Check that the event workspace name is non-blank.
   std::string ev_ws_name = m_uiForm.SelectEventWorkspace_ledt->text().trimmed().toStdString();

   if ( ev_ws_name.length() == 0 )
   {
     errorMessage("Specify the name of an Event Workspace on Select Data tab.");
     return;
   }

   // Check that the MD workspace name is non-blank.
   std::string md_ws_name = m_uiForm.MDworkspace_ledt->text().trimmed().toStdString();

   if ( md_ws_name.length() == 0 )
   {
     errorMessage("Specify the name of an MD Workspace on Select Data tab.");
     return;
   }

   if ( m_thread_pool->activeThreadCount() >= 1 )
   {
     errorMessage("Previous operation still running, please wait until it is finished");
     return;
   }
   std::string file_name = m_uiForm.EventFileName_ledt->text().trimmed().toStdString();
   if (m_uiForm.convertToMDGroupBox->isChecked())
   {
     if (!m_uiForm.loadDataGroupBox->isChecked())
     {
       if ( !worker->isEventWorkspace( ev_ws_name ) )
       {
         errorMessage("Requested Event Workspace is NOT a valid Event workspace");
         return;
       }
     }
     else
     {
       if (file_name.empty())
       {
         errorMessage("Specify the name of an event file to load.");
         return;
       }
     }

     double minQ;
     getDouble( m_uiForm.MinMagQ_ledt, minQ );

     double maxQ;
     getDouble( m_uiForm.MaxMagQ_ledt, maxQ );

     std::string det_cal_file  = m_uiForm.CalFileName_ledt->text().trimmed().toStdString();
     std::string det_cal_file2 = m_uiForm.CalFileName2_ledt->text().trimmed().toStdString();
     bool        load_det_cal  = m_uiForm.LoadDetCal_ckbx->isChecked();
     if ( load_det_cal && det_cal_file.length() == 0 )
     {
       errorMessage("Specify the name of a .DetCal file if Load ISAW Detector Calibration is selected");
       return;
     }

     RunLoadAndConvertToMD* runner = new RunLoadAndConvertToMD(worker,file_name,
                                                               ev_ws_name, md_ws_name,
                                                               minQ, maxQ,
                                                               m_uiForm.LorentzCorrection_ckbx->isChecked(),
                                                               m_uiForm.loadDataGroupBox->isChecked(),
                                                               load_det_cal, det_cal_file, det_cal_file2 );
     bool running = m_thread_pool->tryStart( runner );
     if ( !running )
       errorMessage( "Failed to start Load and ConvertToMD thread...previous operation not complete" );
   }
   else // check existing workspaces
   {
     if ( !worker->isEventWorkspace( ev_ws_name ) )
     {
       errorMessage("Requested Event Workspace is NOT a valid Event workspace");
       return;
     }
     if ( !worker->isMDWorkspace( md_ws_name ) )
     {
       errorMessage("Requested MD Workspace is NOT a valid MD workspace");
       return;
     }
   }
}


/**
 *  Set the default workspace names when editing the event filename is 
 *  finished on the Select Data tab, or a file is selected with the
 *  Browse button.
 */
void MantidEV::loadEventFileEntered_slot()
{
  QString Qfile_name =m_uiForm.EventFileName_ledt->text().trimmed();
  if ( Qfile_name.length() > 0 )
  {
    last_event_file = Qfile_name.toStdString();   

    std::string base_name = extractBaseFileName( last_event_file );
    std::string event_ws_name = base_name + "_event";
    std::string md_ws_name    = base_name + "_md";
    std::string peaks_ws_name = base_name + "_peaks";

    m_uiForm.SelectEventWorkspace_ledt->setText( QString::fromStdString( event_ws_name ));
    m_uiForm.MDworkspace_ledt->setText( QString::fromStdString( md_ws_name ));
    m_uiForm.PeaksWorkspace_ledt->setText( QString::fromStdString( peaks_ws_name ));
  }
}


/**
 *  Slot called when the Browse button for loading data from an event file
 *  is pressed on the SelectData tab.
 */
void MantidEV::loadEventFile_slot()
{
  QString file_path  = getFilePath( last_event_file );
  QString Qfile_name = QFileDialog::getOpenFileName( this,
                              tr("Load event file"),
                              file_path,
                              tr("Nexus Files (*.nxs);; All files(*.*)"));

  if ( Qfile_name.length() > 0 )
  {
    m_uiForm.EventFileName_ledt->setText( Qfile_name );
    loadEventFileEntered_slot();    // set up the default workspace names
  }
}


/**
 *  Slot called when the Browse button for loading the first ISAW
 *  .DetCal file is pressed on the SelectData tab.
 */
void MantidEV::selectDetCalFile_slot()
{
  QString file_path  = getFilePath( last_cal_file );
  QString Qfile_name = QFileDialog::getOpenFileName( this,
                              tr("Load calibration file"),
                              file_path,
                              tr("ISAW .DetCal Files (*.DetCal);; All files(*.*)"));

  if ( Qfile_name.length() > 0 )
  {
    m_uiForm.CalFileName_ledt->setText( Qfile_name );
    last_cal_file = Qfile_name.toStdString();
  }
}


/**
 *  Slot called when the Browse button for loading the second ISAW
 *  .DetCal file is pressed on the SelectData tab.
 */
void MantidEV::selectDetCalFile2_slot()
{
  QString file_path  = getFilePath( last_cal_file2 );
  QString Qfile_name = QFileDialog::getOpenFileName( this,
                              tr("Load calibration file"),
                              file_path,
                              tr("ISAW .DetCal Files (*.DetCal);; All files(*.*)"));

  if ( Qfile_name.length() > 0 )
  {
    m_uiForm.CalFileName2_ledt->setText( Qfile_name );
    last_cal_file2 = Qfile_name.toStdString();
  }
}


/**
 *  Slot called when the Apply button is pressed on the Find Peaks tab.
 */
void MantidEV::findPeaks_slot()
{
   std::string peaks_ws_name = m_uiForm.PeaksWorkspace_ledt->text().trimmed().toStdString();
   if ( peaks_ws_name.length() == 0 )
   {
     errorMessage("Specify a peaks workspace name on the Find Peaks tab.");
     return;
   }

   if ( m_thread_pool->activeThreadCount() >= 1 )
   {
     errorMessage("Previous operation still running, please wait until it is finished");
     return;
   }

   bool find_new_peaks     = m_uiForm.FindPeaks_rbtn->isChecked();
   bool use_existing_peaks = m_uiForm.UseExistingPeaksWorkspace_rbtn->isChecked();
   bool load_peaks         = m_uiForm.LoadIsawPeaks_rbtn->isChecked();

   if ( find_new_peaks )
   {
     std::string md_ws_name  = m_uiForm.MDworkspace_ledt->text().trimmed().toStdString();
     if ( md_ws_name.length() == 0 )
     {
       errorMessage("Specify an MD workspace name on Select Data tab.");
       return;
     }

     double max_abc       = 15;
     size_t num_to_find   = 50;
     double min_intensity = 10;

     if ( !getPositiveDouble( m_uiForm.MaxABC_ledt, max_abc ) )
       return;

     if ( !getPositiveInt( m_uiForm.NumToFind_ledt, num_to_find ) )
       return;

     if ( !getPositiveDouble( m_uiForm.MinIntensity_ledt, min_intensity ) )
       return;
     std::string ev_ws_name = m_uiForm.SelectEventWorkspace_ledt->text().trimmed().toStdString();
     RunFindPeaks* runner = new RunFindPeaks( worker, ev_ws_name,
                                         md_ws_name, peaks_ws_name,
                                         max_abc, num_to_find, min_intensity );

     bool running = m_thread_pool->tryStart( runner );
     if ( !running )
       errorMessage( "Failed to start findPeaks thread...previous operation not complete" );   
   }
   else if ( use_existing_peaks )
   {
     if ( !worker->isPeaksWorkspace( peaks_ws_name ) )
     {
       errorMessage("Requested Peaks Workspace Doesn't Exist");
     }
   }
   else if ( load_peaks )
   {
     std::string file_name = m_uiForm.SelectPeaksFile_ledt->text().trimmed().toStdString();
     std::string extension = Poco::Path(file_name).getExtension();
     if ( file_name.length() == 0 )
     {
       errorMessage("Specify a peaks file with the peaks to be loaded.");
       return;
     }
     if (extension.compare("nxs") == 0 || extension.compare("h5") == 0){
       if ( !worker->loadNexusPeaks( peaks_ws_name, file_name ) ){
         errorMessage("Could not load requested peaks file");
       }
     }
     else {
       if ( !worker->loadIsawPeaks( peaks_ws_name, file_name ) )
       {
         errorMessage("Could not load requested NeXus file");
       }
     }
   }
}

/**
 *  Slot called when the Browse button for loading peaks from a peaks file
 *  is pressed on the FindPeaks tab.
 */
void MantidEV::getLoadPeaksFileName_slot()
{
  QString file_path  = getFilePath( last_peaks_file );
  QString Qfile_name = QFileDialog::getOpenFileName( this,
                         tr("Load peaks file"),
                         file_path,
                         tr("Peaks Files (*.peaks *.integrate *.nxs *.h5);; All files(*.*)"));

  if ( Qfile_name.length()> 0 )
  {
    last_peaks_file = Qfile_name.toStdString();
    m_uiForm.SelectPeaksFile_ledt->setText( Qfile_name );
  }
}


/**
 *  Utility to pop up a dialog box to get the name of a peaks file to
 *  save.
 */
void MantidEV::getSavePeaksFileName()
{
  QString file_path  = getFilePath( last_peaks_file );
  QString Qfile_name = QFileDialog::getSaveFileName( this,
                          tr("Save peaks file"),
                          file_path,
                          tr("Peaks Files (*.peaks *.integrate *.nxs *.h5);; All files(*.*)"),
                          0, QFileDialog::DontConfirmOverwrite );

  if ( Qfile_name.length() > 0 )
  {
    last_peaks_file = Qfile_name.toStdString();
    m_uiForm.SelectPeaksFile_ledt->setText( Qfile_name );
  }
}


/**
 *  Slot called when the Apply button is pressed on the Find UB tab
 */
void MantidEV::findUB_slot()
{
   std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().trimmed().toStdString();
   if ( peaks_ws_name.length() == 0 )
   {
     errorMessage("Specify a peaks workspace name on the Find Peaks tab.");
     return;
   }

   if ( m_thread_pool->activeThreadCount() >= 1 )
   {
     errorMessage("Previous operation still running, please wait until it is finished");
     return;
   }

   bool use_FFT          = m_uiForm.FindUBUsingFFT_rbtn->isChecked();
   bool use_IndexedPeaks = m_uiForm.FindUBUsingIndexedPeaks_rbtn->isChecked();
   bool load_UB          = m_uiForm.LoadISAWUB_rbtn->isChecked();
   bool index_peaks      = m_uiForm.IndexPeaks_ckbx->isChecked();
   bool round_hkls       = m_uiForm.RoundHKLs_ckbx->isChecked();
   bool optimize_angles  = m_uiForm.OptimizeGoniometerAngles_ckbx->isChecked();

   if ( use_FFT )
   {
     double min_abc         = 3;
     double max_abc         = 15;
     double fft_tolerance   = 0.12;

     if ( !getPositiveDouble( m_uiForm.MinD_ledt, min_abc ) )
       return;

     if ( !getPositiveDouble( m_uiForm.MaxD_ledt, max_abc ) )
       return;

     if ( !getPositiveDouble( m_uiForm.FFTTolerance_ledt, fft_tolerance ) )
       return;

     if (!worker->findUBUsingFFT(peaks_ws_name,min_abc,max_abc,fft_tolerance))
     {
       errorMessage( "Find UB Using FFT Failed" );
       return;
     }
   }

   else if ( use_IndexedPeaks )
   {
	   double indPeaks_tolerance   = 0.1;

	   if ( !getPositiveDouble( m_uiForm.IndexedPeaksTolerance_ledt, indPeaks_tolerance ) )
	     return;

     if ( !worker->findUBUsingIndexedPeaks( peaks_ws_name,indPeaks_tolerance) )
     {
       errorMessage( "Find UB Using Indexed Peaks Failed" );
       return;
     }
   }

   else if ( load_UB )
   {
     std::string file_name = m_uiForm.SelectUBFile_ledt->text().trimmed().toStdString();
     if ( file_name.length() == 0 )
     {
       errorMessage("Select a .mat file with the UB matrix to be loaded.");
       return;
     }
     else
     {
       if ( !worker->loadIsawUB( peaks_ws_name, file_name ) )
       {
         errorMessage( "Failed to Load UB Matrix" );
         return;
       }
       if ( optimize_angles )
       {
         double max_degrees = 5;
         if ( getPositiveDouble( m_uiForm.MaxGoniometerChange_ledt,max_degrees) )
         { 
           if ( !worker->optimizePhiChiOmega( peaks_ws_name, max_degrees ) )
           {
             errorMessage("Failed to Optimize Phi, Chi and Omega");
             // Don't return here, since we did still change UB by loading it.
             // proceed to copyLattice, below.
           }
         }
         else
         {
           errorMessage( "Enter a POSITIVE number for Maximum Change (degrees)" );
         }
       }
     }
   }
                               // Now that we set a UB copy it to md_workspace.  If
                               // copy fails a log notice is output by copyLattice
   std::string md_ws_name = m_uiForm.MDworkspace_ledt->text().trimmed().toStdString();
   std::string event_ws_name = m_uiForm.SelectEventWorkspace_ledt->text().trimmed().toStdString();
   worker->copyLattice(peaks_ws_name, md_ws_name, event_ws_name);

   if ( index_peaks )
   {
     double index_tolerance = 0.12;
     if ( !getPositiveDouble( m_uiForm.IndexingTolerance_ledt, index_tolerance ) )
       return;

     if ( !worker->indexPeaksWithUB( peaks_ws_name, index_tolerance, round_hkls ) )
     {
       errorMessage("Failed to Index Peaks with the Existing UB Matrix");
     }
   }
   bool predict_new_peaks     = m_uiForm.PredictPeaks_ckbx->isChecked();

   if ( predict_new_peaks )
   {
	 double min_pred_wl       =          0.4;
	 double max_pred_wl       =          3.5;
	 double min_pred_dspacing =          0.4;
	 double max_pred_dspacing =          8.5;

     if ( !getPositiveDouble( m_uiForm.min_pred_wl_ledt, min_pred_wl ) )
       return;

     if ( !getPositiveDouble( m_uiForm.max_pred_wl_ledt, max_pred_wl ) )
       return;

     if ( !getPositiveDouble( m_uiForm.min_pred_dspacing_ledt, min_pred_dspacing ) )
       return;

     if ( !getPositiveDouble( m_uiForm.max_pred_dspacing_ledt, max_pred_dspacing ) )
       return;

     RunPredictPeaks* runner = new RunPredictPeaks( worker,
                                         peaks_ws_name,
                                         min_pred_wl,
                                         max_pred_wl,
                                         min_pred_dspacing,
                                         max_pred_dspacing );

     bool running = m_thread_pool->tryStart( runner );
     if ( !running )
       errorMessage( "Failed to start predictPeaks thread...previous operation not complete" );
   }

}


/**
 *  Slot called when the brows button is pressed for getting the UB file name
 *  to load, on the Find UB tab.
 */
void MantidEV::getLoadUB_FileName_slot()
{
  QString file_path  = getFilePath( last_UB_file );
  QString Qfile_name = QFileDialog::getOpenFileName( this,
                         tr("Load matrix file"),
                         file_path,
                         tr("Matrix Files (*.mat);; All files(*.*)"));
  if ( Qfile_name.length()> 0 )
  {
    last_UB_file = Qfile_name.toStdString();
    m_uiForm.SelectUBFile_ledt->setText( Qfile_name );
  }
} 


/**
 *  Utility to get the name of a UB file to save.
 */
void MantidEV::getSaveUB_FileName()
{
  QString file_path  = getFilePath( last_UB_file );
  QString Qfile_name = QFileDialog::getSaveFileName( this,
                            tr("Save matrix file"),
                            file_path,
                            tr("Matrix Files (*.mat);; All files(*.*)"));

  if ( Qfile_name.length() > 0 )
  {
    last_UB_file = Qfile_name.toStdString();
    m_uiForm.SelectUBFile_ledt->setText( Qfile_name );
  }
}


/**
 *  Slot called when the apply button is pressed on the Choose Cell tab.
 */
void MantidEV::chooseCell_slot()
{
   std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().trimmed().toStdString();
   if ( peaks_ws_name.length() == 0 )
   {
     errorMessage("Specify a peaks workspace name on the Find Peaks tab.");
     return;
   }

   if ( m_thread_pool->activeThreadCount() >= 1 )
   {
     errorMessage("Previous operation still running, please wait until it is finished");
     return;
   }

   bool show_cells       = m_uiForm.ShowPossibleCells_rbtn->isChecked();
   bool select_cell_type = m_uiForm.SelectCellOfType_rbtn->isChecked();
   bool select_cell_form = m_uiForm.SelectCellWithForm_rbtn->isChecked();

   if ( show_cells )
   {
     bool best_only          = m_uiForm.BestCellOnly_ckbx->isChecked();
     bool allow_perm          = m_uiForm.AllowPermutations_ckbx->isChecked();
     double max_scalar_error = 0;
     if ( !getPositiveDouble( m_uiForm.MaxScalarError_ledt, max_scalar_error ) )
       return;
     if ( !worker->showCells( peaks_ws_name, max_scalar_error, best_only, allow_perm ) )
     {
       errorMessage("Failed to Show Conventional Cells");
     }
   }

   else if ( select_cell_type )
   {
     std::string cell_type = m_uiForm.CellType_cmbx->currentText().toStdString();
     std::string centering = m_uiForm.CellCentering_cmbx->currentText().toStdString();
     if ( !worker->selectCellOfType( peaks_ws_name, cell_type, centering ) )
     {
       errorMessage("Failed to Select Specified Conventional Cell");
     }
   }

   else if ( select_cell_form )
   {
     std::string form = m_uiForm.CellFormNumber_cmbx->currentText().toStdString();
     double form_num = 0;
     getDouble( form, form_num );
     if ( !worker->selectCellWithForm( peaks_ws_name, (size_t)form_num ) )
     {
       errorMessage("Failed to Select the Requested Form Number");
     }
   }

   if ( select_cell_type || select_cell_form )
   {                                 // Try to copy the UB to md_workspace.  If it
                                     // fails a log notice is output by copyLattice
     std::string md_ws_name = m_uiForm.MDworkspace_ledt->text().trimmed().toStdString();
     std::string event_ws_name = m_uiForm.SelectEventWorkspace_ledt->text().trimmed().toStdString();
     worker->copyLattice(peaks_ws_name, md_ws_name, event_ws_name);
   }
}


/**
 *  Slot called when the Apply button is pressed on the Change HKL tab.
 */
void MantidEV::changeHKL_slot()
{
   std::string peaks_ws_name = m_uiForm.PeaksWorkspace_ledt->text().trimmed().toStdString();
   if ( peaks_ws_name.length() == 0 )
   {
     errorMessage("Specify a peaks workspace name on the Find Peaks tab.");
     return;
   }

   if ( m_thread_pool->activeThreadCount() >= 1 )
   {
     errorMessage("Previous operation still running, please wait until it is finished");
     return;
   }

   std::string row_1_str = m_uiForm.HKL_tran_row_1_ledt->text().toStdString();
   std::string row_2_str = m_uiForm.HKL_tran_row_2_ledt->text().toStdString();
   std::string row_3_str = m_uiForm.HKL_tran_row_3_ledt->text().toStdString();

   if ( !worker->changeHKL( peaks_ws_name, row_1_str, row_2_str, row_3_str ) )
   {
     errorMessage( "Failed to Change the Miller Indicies and UB" );
   }

                                     // Try to copy the UB to md_workspace.  If it
                                     // fails a log notice is output by copyLattice
   std::string md_ws_name = m_uiForm.MDworkspace_ledt->text().trimmed().toStdString();
   std::string event_ws_name = m_uiForm.SelectEventWorkspace_ledt->text().trimmed().toStdString();
   worker->copyLattice(peaks_ws_name, md_ws_name, event_ws_name);
}


/**
 *  Slot called when the Apply button is pressed on the Integrate tab.
 */
void MantidEV::integratePeaks_slot()
{
   std::string peaks_ws_name = m_uiForm.PeaksWorkspace_ledt->text().trimmed().toStdString();

   if ( peaks_ws_name.length() == 0 )
   {
     errorMessage("Specify a peaks workspace name on the Find Peaks tab.");
     return;
   }

   std::string event_ws_name = m_uiForm.SelectEventWorkspace_ledt->text().trimmed().toStdString();
   if ( event_ws_name.length() == 0 )
   {
     errorMessage("Specify a time-of-flight event workspace name.");
     return;
   }

   if ( m_thread_pool->activeThreadCount() >= 1 )
   {
     errorMessage("Previous operation still running, please wait until it is finished");
     return;
   }

   bool sphere_integrate    = m_uiForm.SphereIntegration_rbtn->isChecked();
   bool fit_integrate       = m_uiForm.TwoDFitIntegration_rbtn->isChecked();
   bool ellipsoid_integrate = m_uiForm.EllipsoidIntegration_rbtn->isChecked();
   bool use_cylinder_integration = m_uiForm.Cylinder_ckbx->isChecked();

   if ( sphere_integrate || use_cylinder_integration)
   {
     double peak_radius    = 0.20;
     double inner_radius   = 0.20;
     double outer_radius   = 0.25;
     double cylinder_length = 0.0;
     double cylinder_percent_bkg = 0.0;

     if ( !getPositiveDouble( m_uiForm.PeakRadius_ledt, peak_radius ) )
       return;

     if ( !getPositiveDouble(m_uiForm.BackgroundInnerRadius_ledt, inner_radius))
       return;

     if ( !getPositiveDouble(m_uiForm.BackgroundOuterRadius_ledt, outer_radius))
       return;

     bool integrate_edge = m_uiForm.IntegrateEdge_ckbx->isChecked();

     if ( !getPositiveDouble(m_uiForm.CylinderLength_ledt, cylinder_length))
            return;

     if ( !getPositiveDouble(m_uiForm.CylinderPercentBkg_ledt, cylinder_percent_bkg))
            return;

     std::string cylinder_profile_fit = m_uiForm.CylinderProfileFit_cmbx->currentText().toStdString();

     RunSphereIntegrate * runner = new RunSphereIntegrate( worker,
                                        peaks_ws_name, event_ws_name,
                                        peak_radius, inner_radius, outer_radius,
                                        integrate_edge, use_cylinder_integration,
                                        cylinder_length, cylinder_percent_bkg,
                                        cylinder_profile_fit);

     bool running = m_thread_pool->tryStart( runner );
     if ( !running )
       errorMessage( "Failed to start sphere integrate thread...previous operation not complete" );
   }
   else if ( fit_integrate )
   {
     bool use_ikeda_carpenter = m_uiForm.IkedaCarpenter_ckbx->isChecked();
     std::string rebin_params = m_uiForm.FitRebinParams_ledt->text().toStdString();
     double n_bad_edge_pix = 5;
     if ( !getPositiveDouble( m_uiForm.NBadEdgePixels_ledt, n_bad_edge_pix ) )
       return;

     RunFitIntegrate * runner = new RunFitIntegrate( worker,
                                              peaks_ws_name, event_ws_name,
                                              rebin_params, 
                                              (size_t)n_bad_edge_pix,
                                              use_ikeda_carpenter );

     bool running = m_thread_pool->tryStart( runner );
     if ( !running )
       errorMessage( "Failed to start sphere integrate thread...previous operation not complete" );
   }
   else if ( ellipsoid_integrate )
   {
     double region_radius  = 0.20;
     if ( !getPositiveDouble( m_uiForm.RegionRadius_ledt, region_radius ) )
       return;

     double peak_size  = 0.20;
     double inner_size = 0.20;
     double outer_size = 0.25;
     bool specify_size = m_uiForm.SpecifySize_ckbx->isChecked();
     if ( specify_size )
     {
       if ( !getPositiveDouble( m_uiForm.PeakSize_ledt, peak_size ) )
         return;

       if ( !getPositiveDouble(m_uiForm.BackgroundInnerSize_ledt, inner_size) )
         return;

       if ( !getPositiveDouble(m_uiForm.BackgroundOuterSize_ledt, outer_size) )
         return;
     }

     RunEllipsoidIntegrate * runner = new RunEllipsoidIntegrate( worker,
                                            peaks_ws_name, event_ws_name,
                                            region_radius, specify_size,
                                            peak_size, inner_size, outer_size );

     bool running = m_thread_pool->tryStart( runner );
     if ( !running )
       errorMessage( "Failed to start sphere integrate thread...previous operation not complete" );
   }
}


/**
 *  Slot called when Show Info button is pressed
 */
void MantidEV::showInfo_slot()
{
   double qx = 0.0;
   double qy = 0.0;
   double qz = 0.0;
   getDouble( m_uiForm.Qx_ledt, qx );
   getDouble( m_uiForm.Qy_ledt, qy );
   getDouble( m_uiForm.Qz_ledt, qz );

   /// loop back test of SelectionNotificationService
   MantidQt::API::SelectionNotificationService::Instance().sendQPointSelection( true, qx, qy, qz );
}


/**
 *  This slot is connected to the QPointSelection signal, which will be emitted
 *  when a QPoint is selected by some participating object.
 *
 *  @param  lab_coords  Will be true if the Q-components are in lab coordinates
 *                      and false if they are in sample coordinates.
 *  @param qx           The x-component of the Mantid Q-vector.
 *  @param qy           The y-component of the Mantid Q-vector.
 *  @param qz           The z-component of the Mantid Q-vector.
 */
void MantidEV::QPointSelection_slot( bool lab_coords, double qx, double qy, double qz )
{
  Mantid::Kernel::V3D q_point( qx, qy, qz );
  showInfo( lab_coords, q_point );
}


/**
 *  Use the peaks workspace to get information about the specified
 *  Q-vector.
 *
 *  @param  lab_coords  Will be true if the Q-components are in lab coordinates
 *                      and false if they are in sample coordinates.
 *  @param  q_point     Vector containing the Q-coordinates.
 */
void MantidEV::showInfo( bool lab_coords, Mantid::Kernel::V3D  q_point )
{
   std::string peaks_ws_name = m_uiForm.PeaksWorkspace_ledt->text().trimmed().toStdString();

   std::vector< std::pair< std::string, std::string > > info;

   if ( !worker->isPeaksWorkspace( peaks_ws_name ) )  // just show the Q vector
   {
     errorMessage("NOTE: Peaks Workspace Doesn't Exist");
     if ( lab_coords )
     {
       std::pair<std::string, std::string> QlabStr("Qlab", boost::lexical_cast<std::string>(q_point));
       info.push_back( QlabStr );
     }
     else
     {
       std::pair<std::string, std::string> QSampleStr("QSample", boost::lexical_cast<std::string>(q_point));
       info.push_back( QSampleStr );
     }
   }
   else   // get the info from the peaks workspace
   {
     info = worker->PointInfo( peaks_ws_name, lab_coords, q_point );
   }

   double q_dist = ( q_point - last_Q ).norm();
   std::pair<std::string, std::string> Q_dist_str("|Q2-Q1|", boost::lexical_cast<std::string>(q_dist));
   info.push_back( Q_dist_str );

   Mantid::Kernel::Matrix<double> UB(3,3,false);
   if ( worker->getUB( peaks_ws_name, lab_coords, UB ) ) // if the peaks workspace has a UB, also find the
   {                                                     // distance between points in HKL.
     Mantid::Kernel::Matrix<double> UBinv( UB ); 
     UBinv.Invert();
     Mantid::Kernel::V3D  hkl_1 = UBinv * last_Q;
     Mantid::Kernel::V3D  hkl_2 = UBinv * q_point;
     hkl_1 = hkl_1 / (2 * M_PI); 
     hkl_2 = hkl_2 / (2 * M_PI);
     double hkl_dist = (hkl_2 - hkl_1).norm();
     std::pair<std::string, std::string> hkl_dist_str("|hkl2-hkl1|", boost::lexical_cast<std::string>(hkl_dist));
     info.push_back( hkl_dist_str );
   }

   last_Q = q_point;
   
   m_uiForm.SelectedPoint_tbl->setRowCount((int)info.size());
   m_uiForm.SelectedPoint_tbl->setColumnCount(2);
   m_uiForm.SelectedPoint_tbl->verticalHeader()->hide();
   m_uiForm.SelectedPoint_tbl->horizontalHeader()->hide();

   for ( size_t row = 0; row < info.size(); row++ )
   {
     QString first_str = QString::fromStdString( info[row].first );
     QTableWidgetItem *item = new QTableWidgetItem( first_str );
     m_uiForm.SelectedPoint_tbl->setItem( (int)row, 0, item );
     QString second_str = QString::fromStdString( info[row].second );
     item = new QTableWidgetItem( second_str );
     m_uiForm.SelectedPoint_tbl->setItem( (int)row, 1, item );
   }
   m_uiForm.SelectedPoint_tbl->resizeColumnToContents(0);
   m_uiForm.SelectedPoint_tbl->resizeColumnToContents(1);
}


/**
 *  Slot called when the Save Settings action is selected from the File menu.
 */
void MantidEV::saveState_slot()
{
  QString file_path  = getFilePath( last_ini_file );
  QString Qfile_name = QFileDialog::getSaveFileName( this,
                          tr("Save Settings File(.ini)"),
                          file_path,
                          tr("Settings Files (*.ini);; All files(*.*) "));

  if ( Qfile_name.length() > 0 )
  {
    last_ini_file = Qfile_name.toStdString();
    saveSettings( last_ini_file );
  }

}


/**
 *  Slot called when the Load Settings action is selected from the File menu.
 */
void MantidEV::loadState_slot()
{
  QString file_path  = getFilePath( last_ini_file );
  QString Qfile_name = QFileDialog::getOpenFileName( this,
                         tr("Load Settings File(.ini)"),
                         file_path,
                         tr("Settings Files (*.ini);; All files(*.*)"));

  if ( Qfile_name.length()> 0 )
  {
    last_ini_file = Qfile_name.toStdString();
    loadSettings( last_ini_file );
  }
}


/**
 *  Slot called when the Save Isaw UB action is selected from the File menu.
 */
void MantidEV::saveIsawUB_slot()
{
  std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().trimmed().toStdString();
  if ( peaks_ws_name.length() == 0 )
  {
    errorMessage("Specify a peaks workspace name on the Find Peaks tab.");
    return;
  }

  getSaveUB_FileName();

  std::string file_name = m_uiForm.SelectUBFile_ledt->text().trimmed().toStdString();
  if ( file_name.length() == 0 )
  {
     errorMessage("Select a .mat file with the UB matrix to be loaded.");
     return;
  }
  else
  {
    if ( !worker->saveIsawUB( peaks_ws_name, file_name ) )
    {
      errorMessage( "Failed to Save UB Matrix" );
      return;
    }
  }
}


/**
 *  Slot called when the Load Isaw UB action is selected from the File menu.
 */
void MantidEV::loadIsawUB_slot()
{
  std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().trimmed().toStdString();
  if ( peaks_ws_name.length() == 0 )
  {
    errorMessage("Specify a peaks workspace name on the Find Peaks tab.");
    return;
  }

  getLoadUB_FileName_slot();

  std::string file_name = m_uiForm.SelectUBFile_ledt->text().trimmed().toStdString();
  if ( file_name.length() == 0 )
  {
     errorMessage("Select a .mat file with the UB matrix to be loaded.");
     return;
  }
  else
  {
    if ( !worker->loadIsawUB( peaks_ws_name, file_name ) )
    {
      errorMessage( "Failed to Load UB Matrix" );
      return;
    }
  }
}


/**
 *  Slot called when the Save Isaw Peaks action is selected from the File menu.
 */
void MantidEV::saveIsawPeaks_slot()
{
  std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().trimmed().toStdString();
  if ( peaks_ws_name.length() == 0 )
  {
    errorMessage("Specify a peaks workspace name on the Find Peaks tab.");
    return;
  }

  getSavePeaksFileName();

  std::string file_name = m_uiForm.SelectPeaksFile_ledt->text().trimmed().toStdString();
  if ( file_name.length() == 0 )
  {
     errorMessage("Specify a peaks file name for saving the peaks workspace.");
     return;
  }
  else
  {
                              // if the file exists, check for overwrite or append
    bool append = false;
    QFile peaks_file( QString::fromStdString( file_name ) );
    if ( peaks_file.exists() )
    {
      QMessageBox message_box( this->window() );
      message_box.setText( tr("File Exists") );
      message_box.setInformativeText("Replace file, or append peaks to file?");
      QAbstractButton *replace_btn = message_box.addButton( tr("Replace"), QMessageBox::NoRole );
      QAbstractButton *append_btn  = message_box.addButton( tr("Append"),  QMessageBox::YesRole );
      message_box.setIcon( QMessageBox::Question );
                                                   // it should not be necessary to do the next
                                                   // four lines, but without them the message box
                                                   // appears at random locations on RHEL 6
      message_box.show();                               
      QSize box_size = message_box.sizeHint(); 
      QRect screen_rect = QDesktopWidget().screen()->rect();
      message_box.move( QPoint( screen_rect.width()/2 - box_size.width()/2, 
                                screen_rect.height()/2 - box_size.height()/2 ) );
      message_box.exec();
 
      if ( message_box.clickedButton() == append_btn )
      {
        append = true;
      } 
      else if ( message_box.clickedButton() == replace_btn )  // no strictly needed, but clearer
      {
        append = false;
      }
    }

    if ( !worker->saveIsawPeaks( peaks_ws_name, file_name, append ) )
    {
      errorMessage( "Failed to save peaks to file" );
      return;
    }
  }
}

/**
 *  Slot called when the Save Nexus Peaks action is selected from the File menu.
 */
void MantidEV::saveNexusPeaks_slot()
{
  std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().trimmed().toStdString();
  if ( peaks_ws_name.length() == 0 )
  {
    errorMessage("Specify a peaks workspace name on the Find Peaks tab.");
    return;
  }

  getSavePeaksFileName();

  std::string file_name = m_uiForm.SelectPeaksFile_ledt->text().trimmed().toStdString();
  if ( file_name.length() == 0 )
  {
     errorMessage("Specify a peaks file name for saving the peaks workspace.");
     return;
  }
  else
  {
                              // if the file exists, check for overwrite or append
    bool append = false;
    QFile peaks_file( QString::fromStdString( file_name ) );
    if ( peaks_file.exists() )
    {
      QMessageBox message_box( this->window() );
      message_box.setText( tr("File Exists") );
      message_box.setInformativeText("Replace file, or append peaks to file?");
      QAbstractButton *replace_btn = message_box.addButton( tr("Replace"), QMessageBox::NoRole );
      QAbstractButton *append_btn  = message_box.addButton( tr("Append"),  QMessageBox::YesRole );
      message_box.setIcon( QMessageBox::Question );
                                                   // it should not be necessary to do the next
                                                   // four lines, but without them the message box
                                                   // appears at random locations on RHEL 6
      message_box.show();
      QSize box_size = message_box.sizeHint();
      QRect screen_rect = QDesktopWidget().screen()->rect();
      message_box.move( QPoint( screen_rect.width()/2 - box_size.width()/2,
                                screen_rect.height()/2 - box_size.height()/2 ) );
      message_box.exec();

      if ( message_box.clickedButton() == append_btn )
      {
        append = true;
      }
      else if ( message_box.clickedButton() == replace_btn )  // no strictly needed, but clearer
      {
        append = false;
      }
    }

    if ( !worker->saveNexusPeaks( peaks_ws_name, file_name, append ) )
    {
      errorMessage( "Failed to save peaks to file" );
      return;
    }
  }
}


/**
 *  Slot called when the Load Isaw Peaks action is selected from the File menu.
 */
void MantidEV::loadIsawPeaks_slot()
{
  std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().trimmed().toStdString();
  if ( peaks_ws_name.length() == 0 )
  {
    errorMessage("Specify a peaks workspace name on the Find Peaks tab.");
    return;
  }

  getLoadPeaksFileName_slot();

  std::string file_name = m_uiForm.SelectPeaksFile_ledt->text().trimmed().toStdString();
  if ( file_name.length() == 0 )
  {
     errorMessage("Select a peaks file to be loaded.");
     return;
  }
  else
  {
    if ( !worker->loadIsawPeaks( peaks_ws_name, file_name ) )
    {
      errorMessage( "Failed to Load Peaks File" );
      return;
    }
  }
}

/**
 *  Slot called when the Load Nexus Peaks action is selected from the File menu.
 */
void MantidEV::loadNexusPeaks_slot()
{
  std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().trimmed().toStdString();
  if ( peaks_ws_name.length() == 0 )
  {
    errorMessage("Specify a peaks workspace name on the Find Peaks tab.");
    return;
  }

  getLoadPeaksFileName_slot();

  std::string file_name = m_uiForm.SelectPeaksFile_ledt->text().trimmed().toStdString();
  if ( file_name.length() == 0 )
  {
     errorMessage("Select a peaks file to be loaded.");
     return;
  }
  else
  {
    if ( !worker->loadNexusPeaks( peaks_ws_name, file_name ) )
    {
      errorMessage( "Failed to Load Peaks File" );
      return;
    }
  }
}


/**
 *  Slot called when the Show UB action is selected from the View menu.
 */
void MantidEV::showUB_slot()
{
  std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().trimmed().toStdString();
  if ( peaks_ws_name.length() == 0 )
  {
    errorMessage("Specify a peaks workspace name on the Find Peaks tab.");
    return;
  }

  if ( !worker->showUB( peaks_ws_name ) )
    errorMessage("The specified workspace does not have a UB matrix");
}


/**
 * Set the enabled state of the load event file components to the
 * specified value.
 *
 * @param on  If true, components will be enabled, if false, disabled.
 */
void MantidEV::setEnabledLoadEventFileParams_slot( bool on )
{
  m_uiForm.loadDataGroupBox->setEnabled(on);
}


/**
 * Set the enabled state of the calibration file labels,
 * line edits and browse buttons based on the state of the 
 * load from event file and load ISAW detector calibration
 * buttons.
 *
 */
void MantidEV::setEnabledLoadCalFiles_slot()
{
  bool enabled = m_uiForm.LoadDetCal_ckbx->isChecked();
  m_uiForm.CalFileName_lbl->setEnabled( enabled );
  m_uiForm.CalFileName_ledt->setEnabled( enabled );
  m_uiForm.SelectCalFile_btn->setEnabled( enabled );
  m_uiForm.CalFileName2_lbl->setEnabled( enabled );
  m_uiForm.CalFileName2_ledt->setEnabled( enabled );
  m_uiForm.SelectCalFile2_btn->setEnabled( enabled );
}


/**
 * Set the enabled state of the load find peaks components to the
 * specified value.
 *
 * @param on  If true, components will be enabled, if false, disabled.
 */
void MantidEV::setEnabledFindPeaksParams_slot( bool on )
{
  m_uiForm.MaxABC_lbl->setEnabled( on );
  m_uiForm.MaxABC_ledt->setEnabled( on );
  m_uiForm.NumToFind_lbl->setEnabled( on );
  m_uiForm.NumToFind_ledt->setEnabled( on );
  m_uiForm.MinIntensity_lbl->setEnabled( on );
  m_uiForm.MinIntensity_ledt->setEnabled( on );
}

/**
 * Set the enabled state of the load find peaks components to the
 * specified value.
 *
 */
void MantidEV::setEnabledPredictPeaksParams_slot()
{
  bool predict_new_peaks     = m_uiForm.PredictPeaks_ckbx->isChecked();
  if ( predict_new_peaks )
  {
    m_uiForm.min_pred_wl_lbl->setEnabled( true );
    m_uiForm.min_pred_wl_ledt->setEnabled( true );
    m_uiForm.max_pred_wl_lbl->setEnabled( true );
    m_uiForm.max_pred_wl_ledt->setEnabled( true );
    m_uiForm.min_pred_dspacing_lbl->setEnabled( true );
    m_uiForm.min_pred_dspacing_ledt->setEnabled( true );
    m_uiForm.max_pred_dspacing_lbl->setEnabled( true );
    m_uiForm.max_pred_dspacing_ledt->setEnabled( true );
  }
  else
  {
    m_uiForm.min_pred_wl_lbl->setEnabled( false );
    m_uiForm.min_pred_wl_ledt->setEnabled( false );
    m_uiForm.max_pred_wl_lbl->setEnabled( false );
    m_uiForm.max_pred_wl_ledt->setEnabled( false );
    m_uiForm.min_pred_dspacing_lbl->setEnabled( false );
    m_uiForm.min_pred_dspacing_ledt->setEnabled( false );
    m_uiForm.max_pred_dspacing_lbl->setEnabled( false );
    m_uiForm.max_pred_dspacing_ledt->setEnabled( false );
  }
}

/**
 * Set the enabled state of the load peaks file components to the
 * specified value.
 *
 * @param on  If true, components will be enabled, if false, disabled.
 */
void MantidEV::setEnabledLoadPeaksParams_slot( bool on )
{
  m_uiForm.SelectPeaksFile_lbl->setEnabled( on );
  m_uiForm.SelectPeaksFile_ledt->setEnabled( on );
  m_uiForm.SelectPeaksFile_btn->setEnabled( on );
}


/**
 * Set the enabled state of the find UB using FFT components to the
 * specified value.
 *
 * @param on  If true, components will be enabled, if false, disabled.
 */
void MantidEV::setEnabledFindUBFFTParams_slot( bool on )
{
  m_uiForm.MinD_lbl->setEnabled( on );
  m_uiForm.MinD_ledt->setEnabled( on );
  m_uiForm.MaxD_lbl->setEnabled( on );
  m_uiForm.MaxD_ledt->setEnabled( on );
  m_uiForm.FFTTolerance_lbl->setEnabled( on );
  m_uiForm.FFTTolerance_ledt->setEnabled( on );
}

/**
 * Set the enabled state of the find UB using Indexed Peaks components to the
 * specified value.
 *
 * @param on  If true, components will be enabled, if false, disabled.
 */
void MantidEV::setEnabledFindUBUsingIndexedPeaksParams_slot( bool on )
{
  m_uiForm.IndexedPeaksTolerance_lbl->setEnabled( on );
  m_uiForm.IndexedPeaksTolerance_ledt->setEnabled( on );
}

/**
 * Set the enabled state of the load UB file components to the
 * specified value.
 *
 * @param on  If true, components will be enabled, if false, disabled.
 */
void MantidEV::setEnabledLoadUBParams_slot( bool on )
{
  m_uiForm.SelectUBFile_lbl->setEnabled( on );
  m_uiForm.SelectUBFile_ledt->setEnabled( on );
  m_uiForm.SelectUBFile_btn->setEnabled( on );
  m_uiForm.OptimizeGoniometerAngles_ckbx->setEnabled( on );
  setEnabledMaxOptimizeDegrees_slot();
}


/**
 * Set the enabled state of the optimize goniometer angle components
 * based on the state of the Load UB button and the Optiimze 
 * Goniometer Angle buttons.
 *
 */
void MantidEV::setEnabledMaxOptimizeDegrees_slot()
{
  bool load_ub         = m_uiForm.LoadISAWUB_rbtn->isChecked();
  bool optimize_angles = m_uiForm.OptimizeGoniometerAngles_ckbx->isChecked();
  if ( load_ub && optimize_angles )
  {
    m_uiForm.MaxGoniometerChange_lbl->setEnabled( true );
    m_uiForm.MaxGoniometerChange_ledt->setEnabled( true );
  }
  else
  {
    m_uiForm.MaxGoniometerChange_lbl->setEnabled( false );
    m_uiForm.MaxGoniometerChange_ledt->setEnabled( false );
  }
}


/**
 * Set the enabled state of the index peaks components to the
 * specified value.
 *
 * @param on  If true, components will be enabled, if false, disabled.
 */
void MantidEV::setEnabledIndexParams_slot( bool on )
{
  m_uiForm.IndexingTolerance_lbl->setEnabled( on );
  m_uiForm.IndexingTolerance_ledt->setEnabled( on );
  m_uiForm.RoundHKLs_ckbx->setEnabled( on );
}


/**
 * Set the enabled state of the show cells components to the
 * specified value.
 *
 * @param on  If true, components will be enabled, if false, disabled.
 */
void MantidEV::setEnabledShowCellsParams_slot( bool on )
{
  m_uiForm.MaxScalarError_lbl->setEnabled( on );
  m_uiForm.MaxScalarError_ledt->setEnabled( on );
  m_uiForm.BestCellOnly_ckbx->setEnabled( on );
  m_uiForm.AllowPermutations_ckbx->setEnabled( on );
}


/**
 * Set the enabled state of the select cell of type components to the
 * specified value.
 *
 * @param on  If true, components will be enabled, if false, disabled.
 */
void MantidEV::setEnabledSetCellTypeParams_slot( bool on )
{
  m_uiForm.CellType_cmbx->setEnabled( on );
  m_uiForm.CellCentering_cmbx->setEnabled( on );
}


/**
 * Set the enabled state of the select cell with form components to the
 * specified value.
 *
 * @param on  If true, components will be enabled, if false, disabled.
 */
void MantidEV::setEnabledSetCellFormParams_slot( bool on )
{
  m_uiForm.CellFormNumber_cmbx->setEnabled( on );
}


/**
 * Set the enabled state of the sphere integration components to the
 * specified value.
 *
 * @param on  If true, components will be enabled, if false, disabled.
 */
void MantidEV::setEnabledSphereIntParams_slot( bool on )
{
  m_uiForm.PeakRadius_lbl->setEnabled( on );
  m_uiForm.PeakRadius_ledt->setEnabled( on );
  m_uiForm.BackgroundInnerRadius_lbl->setEnabled( on );
  m_uiForm.BackgroundInnerRadius_ledt->setEnabled( on );
  m_uiForm.BackgroundOuterRadius_lbl->setEnabled( on );
  m_uiForm.BackgroundOuterRadius_ledt->setEnabled( on );
  m_uiForm.IntegrateEdge_ckbx->setEnabled( on );
  m_uiForm.Cylinder_ckbx->setEnabled( on );
  m_uiForm.CylinderLength_ledt->setEnabled( on );
  m_uiForm.CylinderPercentBkg_ledt->setEnabled( on );
  m_uiForm.CylinderProfileFit_cmbx->setEnabled( on );
}


/**
 * Set the enabled state of the fit integration components to the
 * specified value.
 *
 * @param on  If true, components will be enabled, if false, disabled.
 */
void MantidEV::setEnabledFitIntParams_slot( bool on )
{
  m_uiForm.FitRebinParams_lbl->setEnabled( on );
  m_uiForm.FitRebinParams_ledt->setEnabled( on );
  m_uiForm.NBadEdgePixels_lbl->setEnabled( on );
  m_uiForm.NBadEdgePixels_ledt->setEnabled( on );
  m_uiForm.IkedaCarpenter_ckbx->setEnabled( on );
}


/**
 * Set the enabled state of the ellipse integration components to the
 * specified value.
 *
 * @param on  If true, components will be enabled, if false, disabled.
 */
void MantidEV::setEnabledEllipseIntParams_slot( bool on )
{
  m_uiForm.RegionRadius_lbl->setEnabled( on );
  m_uiForm.RegionRadius_ledt->setEnabled( on );
  m_uiForm.SpecifySize_ckbx->setEnabled( on );
  setEnabledEllipseSizeOptions_slot();
}


/**
 * Set the enabled state of the load event file components base on
 * the state of the ellipse integrate radio button and the
 * specify size checkbox.
 */
void MantidEV::setEnabledEllipseSizeOptions_slot()
{
  bool on = m_uiForm.EllipsoidIntegration_rbtn->isChecked() &&
            m_uiForm.SpecifySize_ckbx->isChecked();
  m_uiForm.PeakSize_lbl->setEnabled( on );
  m_uiForm.PeakSize_ledt->setEnabled( on );
  m_uiForm.BackgroundInnerSize_lbl->setEnabled( on );
  m_uiForm.BackgroundInnerSize_ledt->setEnabled( on );
  m_uiForm.BackgroundOuterSize_lbl->setEnabled( on );
  m_uiForm.BackgroundOuterSize_ledt->setEnabled( on );
}


/**
 * Utility to display an error message.
 *
 * @param  message  The error message to display.
 */
void MantidEV::errorMessage( const std::string & message )
{
  QMessageBox::critical(this,"ERROR", QString::fromStdString(message));
}


/**
 *  Utility to get a double precision value from the specified string.
 *
 *  @param  str    The string containing the string form of a double value.
 *  @param  value  Reference to a double that will be set to the value
 *                 extracted from the string.
 *
 *  @return true if a double was extacted from the string.
 */
bool MantidEV::getDouble( std::string str, double &value )
{
  std::istringstream strs( str );
  if ( strs >> value )
  {
    return true;
  }
  return false;
}


/**
 *  Utility to get a double precision value from the specified
 *  QLineEdit component.
 *
 *  @param  ledt   Pointer to a QLineEdit object that should contain
 *                 the string form of a double.
 *  @param  value  Reference to a double that will be set to the value
 *                 extracted from the QLineEdit object.
 *
 *  @return true if a double was successfully extacted.
 */
bool MantidEV::getDouble( QLineEdit *ledt,
                          double    &value )
{
  std::string strValue = ledt->text().trimmed().toStdString();
  if (strValue.empty())
  {
    value = Mantid::EMPTY_DBL();
    return true;
  }

  if ( getDouble( strValue, value ) )
  {
    return true;
  }

  std::string message( "Invalid Numeric Value: " );
  message += ledt->text().toStdString();
  errorMessage( message );
  return false;
}


/**
 *  Utility to get a positive double precision value from the specified
 *  QLineEdit component.
 *
 *  @param  ledt   Pointer to a QLineEdit object that should contain
 *                 the string form of a positive double.
 *  @param  value  Reference to a double that will be set to the value
 *                 extracted from the QLineEdit object.
 *
 *  @return true if a positive double value was successfully extacted.
 */
bool MantidEV::getPositiveDouble( QLineEdit *ledt,
                                  double    &value )
{
  if ( !getDouble( ledt, value ) )
    return false;

  if ( value > 0.0 )
  {
    return true;
  }

  std::string message( "Positive Double Value Required: " );
  message += ledt->text().toStdString();
  errorMessage( message );
  return false;
}


/**
 *  Utility to get a positive integer value from the specified
 *  QLineEdit component.          
 *
 *  @param  ledt   Pointer to a QLineEdit object that should contain
 *                 the string form of a positive integer.
 *  @param  value  Reference to an integer that will be set to the value
 *                 extracted from the QLineEdit object.
 *
 *  @return true if a positive integer value was successfully extacted.
 */
bool MantidEV::getPositiveInt( QLineEdit *ledt,
                               size_t    &value )
{
  double double_value = 0;
  if ( !getDouble( ledt, double_value ) )
    return false;

  if ( double_value > 0.0 )
  {
    value = (size_t)double_value;
    if ( value > 0 )
    {
      return true;
    }
  }

  std::string message( "Positive Integer Value Required: " );
  message += ledt->text().toStdString();
  errorMessage( message );
  return false;
}


/**
 * Get base file name to form names for event, MD and peaks workspaces
 *
 * @param  full_file_name   The full name of the file that is being loaded. 
 *
 * @return  The base file name, with the directory and extension removed.
 */
std::string MantidEV::extractBaseFileName( std::string full_file_name ) const
{
  size_t dot_index = full_file_name.find_last_of(".");

  if ( dot_index != std::string::npos )
  {
    full_file_name = full_file_name.substr( 0, dot_index );
  }

  size_t path_sep_index = full_file_name.find_last_of("/\\:");

  if ( path_sep_index != std::string::npos )
  {
    full_file_name = full_file_name.substr( path_sep_index + 1 );
  }

  size_t ev_suffix_index = full_file_name.rfind( "_event", std::string::npos );
  if ( ev_suffix_index != std::string::npos )
  {
    full_file_name = full_file_name.substr( 0, ev_suffix_index );
  }

  return full_file_name;
}


/**
 * Get a path to use in a file dialog from the specified file_name
 * or the user's home directory if the file_name has length 0.  
 *
 * @param file_name  The name of a file to use to determine a 
 *                   file path to use when starting a QFileDialog
 *
 * @return The path from the specified file_name or the user's
 *         home directory if the file_name has length 0.
 */
QString MantidEV::getFilePath( const std::string & file_name )
{
  QString file_path;
  if ( file_name.length() != 0 )
  {
    QString Qfile_name = QString::fromStdString( file_name );
    QFileInfo file_info( Qfile_name );
    file_path = file_info.absolutePath();
  }
  else
  {
    file_path = QDir::homePath();
  }
  return file_path;
}


/**
 *  Utility to save the current state of all GUI components into the
 *  specified file name.  If the filename has length zero, the settings
 *  will be saved to a system dependent default location.  This is 
 *  called in the destructor to automatically save the last settings to
 *  the default location.  It is also called to save the settings to 
 *  a specific file when the Save Settings File menu item is selected.
 *
 *  @param  filename   The name of the file to save the settings to, 
 *                     or a blank string to use the default location.
 */
void MantidEV::saveSettings( const std::string & filename )
{
  QSettings* state;
  if ( filename.length() > 0 )
    state = new QSettings( QString::fromStdString(filename), QSettings::IniFormat, this );
  else
    state = new QSettings;
                                                // Save Tab 1, Select Data
  state->setValue("SelectEventWorkspace_ledt", m_uiForm.SelectEventWorkspace_ledt->text());
  state->setValue("MDworkspace_ledt", m_uiForm.MDworkspace_ledt->text());
  state->setValue("LoadEventFile_rbtn", m_uiForm.loadDataGroupBox->isChecked());
  state->setValue("EventFileName_ledt", m_uiForm.EventFileName_ledt->text());
  state->setValue("MaxMagQ_ledt", m_uiForm.MaxMagQ_ledt->text());
  state->setValue("LorentzCorrection_ckbx", m_uiForm.LorentzCorrection_ckbx->isChecked());
  state->setValue("LoadDetCal_ckbx", m_uiForm.LoadDetCal_ckbx->isChecked());
  state->setValue("CalFileName_ledt", m_uiForm.CalFileName_ledt->text());
  state->setValue("CalFileName2_ledt", m_uiForm.CalFileName2_ledt->text());
  state->setValue("ConvertToMD_rbtn", m_uiForm.convertToMDGroupBox->isChecked());

                                                // Save Tab 2, Find Peaks
  state->setValue("PeaksWorkspace_ledt", m_uiForm.PeaksWorkspace_ledt->text());
  state->setValue("FindPeaks_rbtn", m_uiForm.FindPeaks_rbtn->isChecked());
  state->setValue("MaxABC_ledt", m_uiForm.MaxABC_ledt->text());
  state->setValue("NumToFind_ledt", m_uiForm.NumToFind_ledt->text());
  state->setValue("MinIntensity_ledt", m_uiForm.MinIntensity_ledt->text());
  state->setValue("UseExistingPeaksWorkspace_rbtn", m_uiForm.UseExistingPeaksWorkspace_rbtn->isChecked());
  state->setValue("LoadIsawPeaks_rbtn", m_uiForm.LoadIsawPeaks_rbtn->isChecked());
  state->setValue("SelectPeaksFile_ledt", m_uiForm.SelectPeaksFile_ledt->text());
  state->setValue("PredictPeaks_ckbx", m_uiForm.PredictPeaks_ckbx->isChecked());
  state->setValue("min_pred_wl_ledt", m_uiForm.min_pred_wl_ledt->text());
  state->setValue("max_pred_wl_ledt", m_uiForm.max_pred_wl_ledt->text());
  state->setValue("min_pred_dspacing_ledt", m_uiForm.min_pred_dspacing_ledt->text());
  state->setValue("max_pred_dspacing_ledt", m_uiForm.max_pred_dspacing_ledt->text());

                                                // Save Tab 3, Find UB 
  state->setValue("FindUBUsingFFT_rbtn", m_uiForm.FindUBUsingFFT_rbtn->isChecked());
  state->setValue("MinD_ledt", m_uiForm.MinD_ledt->text());
  state->setValue("MaxD_ledt", m_uiForm.MaxD_ledt->text());
  state->setValue("FFTTolerance_ledt", m_uiForm.FFTTolerance_ledt->text());
  state->setValue("FindUBUsingIndexedPeaks_rbtn", m_uiForm.FindUBUsingIndexedPeaks_rbtn->isChecked());
  state->setValue("IndexedPeaksTolerance_ledt", m_uiForm.IndexedPeaksTolerance_ledt->text());
  state->setValue("LoadISAWUB_rbtn", m_uiForm.LoadISAWUB_rbtn->isChecked());
  state->setValue("SelectUBFile_ledt", m_uiForm.SelectUBFile_ledt->text());
  state->setValue("OptimizeGoniometerAngles_ckbx", m_uiForm.OptimizeGoniometerAngles_ckbx->isChecked());
  state->setValue("MaxGoniometerChange_ledt", m_uiForm.MaxGoniometerChange_ledt->text());
  state->setValue("UseCurrentUB_rbtn", m_uiForm.UseCurrentUB_rbtn->isChecked());
  state->setValue("IndexPeaks_ckbx", m_uiForm.IndexPeaks_ckbx->isChecked());
  state->setValue("IndexingTolerance_ledt", m_uiForm.IndexingTolerance_ledt->text());
  state->setValue("RoundHKLs_ckbx", m_uiForm.RoundHKLs_ckbx->isChecked());

                                                // Save Tab 4, Choose Cell
  state->setValue("ShowPossibleCells_rbtn",m_uiForm.ShowPossibleCells_rbtn->isChecked());
  state->setValue("MaxScalarError_ledt",m_uiForm.MaxScalarError_ledt->text());
  state->setValue("BestCellOnly_ckbx",m_uiForm.BestCellOnly_ckbx->isChecked());
  state->setValue("BestCellOnly_ckbx",m_uiForm.AllowPermutations_ckbx->isChecked());
  state->setValue("SelectCellOfType_rbtn",m_uiForm.SelectCellOfType_rbtn->isChecked());
  state->setValue("CellType_cmbx",m_uiForm.CellType_cmbx->currentIndex());
  state->setValue("CellCentering_cmbx",m_uiForm.CellCentering_cmbx->currentIndex());
  state->setValue("SelectCellWithForm_rbtn",m_uiForm.SelectCellWithForm_rbtn->isChecked());
  state->setValue("CellFormNumber_cmbx",m_uiForm.CellFormNumber_cmbx->currentIndex());

                                                // Save Tab 5,Change HKL 
  state->setValue("HKL_tran_row_1_ledt",m_uiForm.HKL_tran_row_1_ledt->text());
  state->setValue("HKL_tran_row_2_ledt",m_uiForm.HKL_tran_row_2_ledt->text());
  state->setValue("HKL_tran_row_3_ledt",m_uiForm.HKL_tran_row_3_ledt->text());

                                                // Save Tab 6, Integrate
  state->setValue("SphereIntegration_rbtn",m_uiForm.SphereIntegration_rbtn->isChecked());
  state->setValue("PeakRadius_ledt",m_uiForm.PeakRadius_ledt->text());
  state->setValue("BackgroundInnerRadius_ledt",m_uiForm.BackgroundInnerRadius_ledt->text());
  state->setValue("BackgroundOuterRadius_ledt",m_uiForm.BackgroundOuterRadius_ledt->text());
  state->setValue("CylinderLength_ledt",m_uiForm.CylinderLength_ledt->text());
  state->setValue("CylinderPercentBkg_ledt",m_uiForm.CylinderPercentBkg_ledt->text());
  state->setValue("IntegrateEdge_ckbx",m_uiForm.IntegrateEdge_ckbx->isChecked());
  state->setValue("Cylinder_ckbx",m_uiForm.Cylinder_ckbx->isChecked());
  state->setValue("CylinderProfileFit_cmbx",m_uiForm.CylinderProfileFit_cmbx->currentIndex());
  state->setValue("TwoDFitIntegration_rbtn",m_uiForm.TwoDFitIntegration_rbtn->isChecked());
  state->setValue("FitRebinParams_ledt",m_uiForm.FitRebinParams_ledt->text());
  state->setValue("NBadEdgePixels_ledt",m_uiForm.NBadEdgePixels_ledt->text());
  state->setValue("IkedaCarpenter_ckbx",m_uiForm.IkedaCarpenter_ckbx->isChecked());
  state->setValue("EllipsoidIntegration_rbtn",m_uiForm.EllipsoidIntegration_rbtn->isChecked());
  state->setValue("RegionRadius_ledt",m_uiForm.RegionRadius_ledt->text());
  state->setValue("SpecifySize_ckbx",m_uiForm.SpecifySize_ckbx->isChecked());
  state->setValue("PeakSize_ledt",m_uiForm.PeakSize_ledt->text());
  state->setValue("BackgroundInnerSize_ledt",m_uiForm.BackgroundInnerSize_ledt->text());
  state->setValue("BackgroundOuterSize_ledt",m_uiForm.BackgroundOuterSize_ledt->text());

                                                // save info for file paths
  state->setValue("last_UB_file",QString::fromStdString(last_UB_file));
  state->setValue("last_event_file",QString::fromStdString(last_event_file));
  state->setValue("last_peaks_file",QString::fromStdString(last_peaks_file));
  state->setValue("last_ini_file",QString::fromStdString(last_ini_file));
  state->setValue("last_cal_file",QString::fromStdString(last_cal_file));
  state->setValue("last_cal_file2",QString::fromStdString(last_cal_file2));
  delete state;
}


/**
 *  Utility to load the current state of all GUI components from the 
 *  specified file name.  If the filename has length zero, the settings
 *  will be loaded from a system dependent default location.  This is 
 *  called at the end of initLayout() to automatically restore the last 
 *  settings from the default location.  It is also called to load the
 *  settings from a specific file when the Load Settings File menu item
 *  is selected.
 *
 *  @param  filename   The name of the file to load the settings from, 
 *                     or a blank string to use the default location.
 */
void MantidEV::loadSettings( const std::string & filename )
{
  QSettings* state;
  if ( filename.length() > 0 )
    state = new QSettings( QString::fromStdString(filename), QSettings::IniFormat, this );
  else
    state = new QSettings;

                                                  // Load Tab 1, Select Data 
  restore( state, "SelectEventWorkspace_ledt", m_uiForm.SelectEventWorkspace_ledt );
  restore( state, "MDworkspace_ledt", m_uiForm.MDworkspace_ledt );
  m_uiForm.loadDataGroupBox->setChecked( state->value("LoadEventFile_rbtn", true).toBool() );
  restore( state, "EventFileName_ledt", m_uiForm.EventFileName_ledt );
  restore( state, "MaxMagQ_ledt", m_uiForm.MaxMagQ_ledt );
  restore( state, "LorentzCorrection_ckbx", m_uiForm.LorentzCorrection_ckbx );
  restore( state, "LoadDetCal_ckbx", m_uiForm.LoadDetCal_ckbx );
  restore( state, "CalFileName_ledt", m_uiForm.CalFileName_ledt );
  restore( state, "CalFileName2_ledt", m_uiForm.CalFileName2_ledt );
  setEnabledLoadCalFiles_slot();
  m_uiForm.convertToMDGroupBox->setChecked( state->value("ConvertToMD_rbtn", true).toBool() );
                                                  // Load Tab 2, Find Peaks
  restore( state, "PeaksWorkspace_ledt", m_uiForm.PeaksWorkspace_ledt );
  restore( state, "FindPeaks_rbtn", m_uiForm.FindPeaks_rbtn );
  restore( state, "MaxABC_ledt", m_uiForm.MaxABC_ledt );
  restore( state, "NumToFind_ledt", m_uiForm.NumToFind_ledt );
  restore( state, "MinIntensity_ledt", m_uiForm.MinIntensity_ledt );
  restore( state, "UseExistingPeaksWorkspace_rbtn", m_uiForm.UseExistingPeaksWorkspace_rbtn );
  restore( state, "LoadIsawPeaks_rbtn", m_uiForm.LoadIsawPeaks_rbtn );
  restore( state, "SelectPeaksFile_ledt", m_uiForm.SelectPeaksFile_ledt );
  restore( state, "PredictPeaks_ckbx", m_uiForm.PredictPeaks_ckbx );
  setEnabledPredictPeaksParams_slot();
  restore( state, "min_pred_wl_ledt", m_uiForm.min_pred_wl_ledt );
  restore( state, "max_pred_wl_ledt", m_uiForm.max_pred_wl_ledt );
  restore( state, "min_pred_dspacing_ledt", m_uiForm.min_pred_dspacing_ledt );
  restore( state, "max_pred_dspacing_ledt", m_uiForm.max_pred_dspacing_ledt );

                                                  // Load Tab 3, Find UB 
  restore( state, "FindUBUsingFFT_rbtn", m_uiForm.FindUBUsingFFT_rbtn );
  restore( state, "MinD_ledt", m_uiForm.MinD_ledt );
  restore( state, "MaxD_ledt", m_uiForm.MaxD_ledt );
  restore( state, "FFTTolerance_ledt", m_uiForm.FFTTolerance_ledt );
  restore( state, "FindUBUsingIndexedPeaks_rbtn", m_uiForm.FindUBUsingIndexedPeaks_rbtn );
  restore( state, "IndexedPeaksTolerance_ledt", m_uiForm.IndexedPeaksTolerance_ledt );
  restore( state, "LoadISAWUB_rbtn", m_uiForm.LoadISAWUB_rbtn );
  restore( state, "SelectUBFile_ledt", m_uiForm.SelectUBFile_ledt );
  restore( state, "OptimizeGoniometerAngles_ckbx", m_uiForm.OptimizeGoniometerAngles_ckbx );
  restore( state, "MaxGoniometerChange_ledt", m_uiForm.MaxGoniometerChange_ledt );
  setEnabledMaxOptimizeDegrees_slot();
  restore( state, "UseCurrentUB_rbtn", m_uiForm.UseCurrentUB_rbtn );
  restore( state, "IndexPeaks_ckbx", m_uiForm.IndexPeaks_ckbx );
  restore( state, "IndexingTolerance_ledt", m_uiForm.IndexingTolerance_ledt );
  restore( state, "RoundHKLs_ckbx", m_uiForm.RoundHKLs_ckbx );

                                                // Load Tab 4, Choose Cell
  restore( state, "ShowPossibleCells_rbtn", m_uiForm.ShowPossibleCells_rbtn );
  restore( state, "MaxScalarError_ledt", m_uiForm.MaxScalarError_ledt );
  restore( state, "BestCellOnly_ckbx", m_uiForm.BestCellOnly_ckbx );
  restore( state, "AllowPermutations_ckbx", m_uiForm.AllowPermutations_ckbx );
  restore( state, "SelectCellOfType_rbtn", m_uiForm.SelectCellOfType_rbtn );
  restore( state, "CellType_cmbx", m_uiForm.CellType_cmbx );
  restore( state, "CellCentering_cmbx", m_uiForm.CellCentering_cmbx );
  restore( state, "SelectCellWithForm_rbtn", m_uiForm.SelectCellWithForm_rbtn );
  restore( state, "CellFormNumber_cmbx", m_uiForm.CellFormNumber_cmbx );
  
                                                // Load Tab 5,Change HKL 
  restore( state, "HKL_tran_row_1_ledt", m_uiForm.HKL_tran_row_1_ledt );
  restore( state, "HKL_tran_row_2_ledt", m_uiForm.HKL_tran_row_2_ledt );
  restore( state, "HKL_tran_row_3_ledt", m_uiForm.HKL_tran_row_3_ledt );

                                                // Load Tab 6, Integrate
  restore( state, "SphereIntegration_rbtn", m_uiForm.SphereIntegration_rbtn );
  restore( state, "PeakRadius_ledt", m_uiForm.PeakRadius_ledt );
  restore( state, "BackgroundInnerRadius_ledt", m_uiForm.BackgroundInnerRadius_ledt );
  restore( state, "BackgroundOuterRadius_ledt", m_uiForm.BackgroundOuterRadius_ledt );
  restore( state, "CylinderLength_ledt", m_uiForm.CylinderLength_ledt );
  restore( state, "CylinderPercentBkg_ledt", m_uiForm.CylinderPercentBkg_ledt );
  restore( state, "IntegrateEdge_ckbx", m_uiForm.IntegrateEdge_ckbx );
  restore( state, "Cylinder_ckbx", m_uiForm.Cylinder_ckbx );
  restore( state, "CylinderProfileFit_cmbx", m_uiForm.CylinderProfileFit_cmbx);
  restore( state, "TwoDFitIntegration_rbtn", m_uiForm.TwoDFitIntegration_rbtn );
  restore( state, "FitRebinParams_ledt", m_uiForm.FitRebinParams_ledt );
  restore( state, "NBadEdgePixels_ledt", m_uiForm.NBadEdgePixels_ledt );
  restore( state, "IkedaCarpenter_ckbx", m_uiForm.IkedaCarpenter_ckbx );
  restore( state, "EllipsoidIntegration_rbtn", m_uiForm.EllipsoidIntegration_rbtn );
  restore( state, "RegionRadius_ledt", m_uiForm.RegionRadius_ledt );
  restore( state, "SpecifySize_ckbx", m_uiForm.SpecifySize_ckbx );
  restore( state, "PeakSize_ledt", m_uiForm.PeakSize_ledt );
  restore( state, "BackgroundInnerSize_ledt", m_uiForm.BackgroundInnerSize_ledt );
  restore( state, "BackgroundOuterSize_ledt", m_uiForm.BackgroundOuterSize_ledt );
  setEnabledEllipseSizeOptions_slot();
                                                // load info for file paths
  last_UB_file    = state->value("last_UB_file", "").toString().toStdString();
  last_event_file = state->value("last_event_file", "").toString().toStdString();
  last_peaks_file = state->value("last_peaks_file", "").toString().toStdString();
  last_ini_file   = state->value("last_ini_file", "").toString().toStdString();
  last_cal_file   = state->value("last_cal_file", "").toString().toStdString();
  last_cal_file2  = state->value("last_cal_file2", "").toString().toStdString();

  delete state;
}


/*
 * Restore the value of the specified QLineEdit component from the 
 * specifed QSettings object.
 *
 * @param state    pointer to the QSettings object to use
 * @param name     the name of the setting to use
 * @param ledt     pointer to the QLineEdit component whose state
 *                 is to be restored.
 */
void MantidEV::restore( QSettings *state, QString name, QLineEdit *ledt )
{
  // NOTE: If state was not saved yet, we don't want to change the
  // default value, so we only change the text if it's non-empty
  QString sText = state->value(name, "").toString();
  if ( sText.length() > 0 )
  {
    ledt->setText( sText );
  }
}


/*
 * Restore the value of the QCheckbox or QRadioButton component from the
 * specifed QSettings object.
 *
 * @param state    pointer to the QSettings object to use
 * @param name     the name of the setting to use
 * @param btn      pointer to the QCheckbox or QRadioButton component 
 *                 whose state is to be restored.
 */
void MantidEV::restore( QSettings *state, QString name, QAbstractButton *btn )
{
  btn->setChecked( state->value(name, false).toBool() );
}


/*
 * Restore the value of a QComboBox from the specified QSettings object
 *
 * @param state    pointer to the QSettings object to use
 * @param name     the name of the setting to use
 * @param cmbx     pointer to the QComboBox component whose state is 
 *                 to be restored.
 */
void MantidEV::restore( QSettings *state, QString name, QComboBox *cmbx )
{
  // NOTE: If state was not saved yet, we don't want to change the
  // default value, so we only change the selected item if the index
  // has been set to a valid value. 
  int val = state->value(name, -1).toInt();
  if ( val > 0 )
  {
    cmbx->setCurrentItem( val );
  }
}


} // namespace CustomInterfaces
} // namespace MantidQt
