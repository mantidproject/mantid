
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>

#include "MantidQtCustomInterfaces/MantidEV.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidKernel/InstrumentInfo.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace MantidQt
{
namespace CustomInterfaces
{

//Register the class with the factory
DECLARE_SUBWINDOW(MantidEV);

using namespace Mantid::Kernel;
using namespace Mantid::API;


/**
 *  Class to call loadAndConvertToMD in a separate thread.
 */
RunLoadAndConvertToMD::RunLoadAndConvertToMD(       MantidEVWorker * worker,
                                              const std::string    & file_name,
                                              const std::string    & ev_ws_name,
                                              const std::string    & md_ws_name )
{
  this->worker     = worker;
  this->file_name  = file_name;
  this->ev_ws_name = ev_ws_name;
  this->md_ws_name = md_ws_name;
}

void RunLoadAndConvertToMD::run()
{
  worker->loadAndConvertToMD( file_name, ev_ws_name, md_ws_name );
}


/**
 * Class to call findPeaks in a separate thread.
 */
RunFindPeaks::RunFindPeaks(        MantidEVWorker * worker,
                            const std::string     & md_ws_name,
                            const std::string     & peaks_ws_name,
                                  double            max_abc,
                                  size_t            num_to_find,
                                  double            min_intensity )
{
  this->worker        = worker;
  this->md_ws_name    = md_ws_name;
  this->peaks_ws_name = peaks_ws_name;
  this->max_abc       = max_abc;
  this->num_to_find   = num_to_find;
  this->min_intensity = min_intensity;
}

void RunFindPeaks::run()
{
  worker->findPeaks( md_ws_name, peaks_ws_name,
                     max_abc, num_to_find, min_intensity );
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
                                              bool             integrate_edge )
{ 
  this->worker         = worker;
  this->peaks_ws_name  = peaks_ws_name;
  this->event_ws_name  = event_ws_name;
  this->peak_radius    = peak_radius;
  this->inner_radius   = inner_radius;
  this->outer_radius   = outer_radius;
  this->integrate_edge = integrate_edge;
}

void RunSphereIntegrate::run()
{ 
  worker->sphereIntegrate( peaks_ws_name, event_ws_name,
                           peak_radius, inner_radius, outer_radius,
                           integrate_edge );
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
  worker        = new MantidEVWorker();
  m_thread_pool = new QThreadPool( this );
  m_thread_pool->setMaxThreadCount(1);
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
   QObject::connect( m_uiForm.ApplySelectData_btn, SIGNAL(clicked()),
                    this, SLOT(selectWorkspace_slot()) );

   QObject::connect( m_uiForm.SelectEventFile_btn, SIGNAL(clicked()),
                     this, SLOT(loadEventFile_slot()) );

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

                          // connect the slots for the menu items
  QObject::connect( m_uiForm.actionSave_State, SIGNAL(triggered()),
                    this, SLOT(saveState_slot()) );

  QObject::connect( m_uiForm.actionLoad_State, SIGNAL(triggered()),
                    this, SLOT(loadState_slot()) );

  QObject::connect( m_uiForm.actionSave_Isaw_UB, SIGNAL(triggered()),
                    this, SLOT(saveIsawUB_slot()) );

  QObject::connect( m_uiForm.actionLoad_Isaw_UB, SIGNAL(triggered()),
                    this, SLOT(loadIsawUB_slot()) );

  QObject::connect( m_uiForm.actionSave_Isaw_Peaks, SIGNAL(triggered()),
                    this, SLOT(saveIsawPeaks_slot()) );

  QObject::connect( m_uiForm.actionLoad_Isaw_Peaks, SIGNAL(triggered()),
                    this, SLOT(loadIsawPeaks_slot()) );

  QObject::connect( m_uiForm.actionShow_UB, SIGNAL(triggered()),
                    this, SLOT(showUB_slot()) );

                          // connect the slots for enabling and disabling
                          // various subsets of widgets
   QObject::connect( m_uiForm.LoadEventFile_rbtn, SIGNAL(toggled(bool)),
                     this, SLOT( setEnabledLoadEventFileParams_slot(bool) ) );

   QObject::connect( m_uiForm.FindPeaks_rbtn, SIGNAL(toggled(bool)),
                     this, SLOT( setEnabledFindPeaksParams_slot(bool) ) );

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

                          // set up defaults for the UI, and set the
                          // various groups of widgets to be enabled or
                          // disabled as needed
   m_uiForm.MantidEV_tabwidg->setCurrentIndex(0);

   m_uiForm.LoadEventFile_rbtn->setChecked(true);
   m_uiForm.UseExistingWorkspaces_rbtn->setChecked(false);
   setEnabledLoadEventFileParams_slot(true);

   m_uiForm.FindPeaks_rbtn->setChecked(true);
   m_uiForm.UseExistingPeaksWorkspace_rbtn->setChecked(false);
   m_uiForm.LoadIsawPeaks_rbtn->setChecked(false);
   setEnabledFindPeaksParams_slot(true);
   setEnabledLoadPeaksParams_slot(false);

   m_uiForm.FindUBUsingFFT_rbtn->setChecked(true);
   m_uiForm.FindUBUsingIndexedPeaks_rbtn->setChecked(false);
   m_uiForm.LoadISAWUB_rbtn->setChecked(false);
   m_uiForm.UseCurrentUB_rbtn->setChecked(false);
   setEnabledFindUBFFTParams_slot(true);
   setEnabledLoadUBParams_slot(false);
   setEnabledMaxOptimizeDegrees_slot();
   m_uiForm.IndexPeaks_ckbx->setChecked(true);
   m_uiForm.RoundHKLs_ckbx->setChecked( true );
   setEnabledIndexParams_slot(true);

   m_uiForm.ShowPossibleCells_rbtn->setChecked(true);
   m_uiForm.SelectCellOfType_rbtn->setChecked(false);
   m_uiForm.SelectCellWithForm_rbtn->setChecked(false);
   setEnabledShowCellsParams_slot(true);
   setEnabledSetCellTypeParams_slot(false);
   setEnabledSetCellFormParams_slot(false);

   m_uiForm.SphereIntegration_rbtn->setChecked(true);
   m_uiForm.IntegrateEdge_ckbx->setChecked(true);
   m_uiForm.TwoDFitIntegration_rbtn->setChecked(false);
   m_uiForm.EllipsoidIntegration_rbtn->setChecked(false);
   setEnabledSphereIntParams_slot(true);
   setEnabledFitIntParams_slot(false);
   setEnabledEllipseIntParams_slot(false);
   m_uiForm.SpecifySize_ckbx->setChecked(false);
   setEnabledEllipseSizeOptions_slot();

   // Add validators to all QLineEdit objects that require numeric values
   m_uiForm.MaxABC_ledt->setValidator( new QDoubleValidator(m_uiForm.MaxABC_ledt));
   m_uiForm.NumToFind_ledt->setValidator( new QDoubleValidator(m_uiForm.NumToFind_ledt));
   m_uiForm.MinIntensity_ledt->setValidator( new QDoubleValidator(m_uiForm.MinIntensity_ledt));
   m_uiForm.MinIntensity_ledt->setValidator( new QDoubleValidator(m_uiForm.MinIntensity_ledt));
   m_uiForm.MinD_ledt->setValidator( new QDoubleValidator(m_uiForm.MinD_ledt));
   m_uiForm.MaxD_ledt->setValidator( new QDoubleValidator(m_uiForm.MaxD_ledt));
   m_uiForm.FFTTolerance_ledt->setValidator( new QDoubleValidator(m_uiForm.FFTTolerance_ledt));
   m_uiForm.MaxGoniometerChange_ledt->setValidator( new QDoubleValidator(m_uiForm.MaxGoniometerChange_ledt));
   m_uiForm.IndexingTolerance_ledt->setValidator( new QDoubleValidator(m_uiForm.IndexingTolerance_ledt));
   m_uiForm.MaxScalarError_ledt->setValidator( new QDoubleValidator(m_uiForm.MaxScalarError_ledt));
   m_uiForm.PeakRadius_ledt->setValidator( new QDoubleValidator(m_uiForm.PeakRadius_ledt));
   m_uiForm.BackgroundInnerRadius_ledt->setValidator( new QDoubleValidator(m_uiForm.BackgroundInnerRadius_ledt));
   m_uiForm.BackgroundOuterRadius_ledt->setValidator( new QDoubleValidator(m_uiForm.BackgroundOuterRadius_ledt));
   m_uiForm.NBadEdgePixels_ledt->setValidator( new QDoubleValidator(m_uiForm.NBadEdgePixels_ledt));
   m_uiForm.RegionRadius_ledt->setValidator( new QDoubleValidator(m_uiForm.RegionRadius_ledt));
   m_uiForm.PeakSize_ledt->setValidator( new QDoubleValidator(m_uiForm.PeakSize_ledt));
   m_uiForm.BackgroundInnerSize_ledt->setValidator( new QDoubleValidator(m_uiForm.BackgroundInnerSize_ledt));
   m_uiForm.BackgroundOuterSize_ledt->setValidator( new QDoubleValidator(m_uiForm.BackgroundOuterSize_ledt));

   loadSettings("");
}


/**
 *  Slot called when the Apply button is pressed on the Select Data tab.
 */
void MantidEV::selectWorkspace_slot()
{
   // Check that the event workspace name is non-blank.
   std::string ev_ws_name = m_uiForm.SelectEventWorkspace_ledt->text().toStdString();

   if ( ev_ws_name.length() == 0 )
   {
     errorMessage("Specify the name of an Event Workspace on Select Data tab.");
     return;
   }

   // Check that the MD workspace name is non-blank.
   std::string md_ws_name = m_uiForm.MDworkspace_ledt->text().toStdString();

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

   if ( m_uiForm.LoadEventFile_rbtn->isChecked() )  // load file and
   {                                                // convert to MD workspace
     std::string file_name = m_uiForm.EventFileName_ledt->text().toStdString();
     if ( file_name.length() == 0 )
     {
       errorMessage("Specify the name of an event file to load.");
       return;
     }

     RunLoadAndConvertToMD* runner = new RunLoadAndConvertToMD(worker,file_name,
                                                       ev_ws_name, md_ws_name );
     bool running = m_thread_pool->tryStart( runner );
     if ( !running )
       errorMessage( "Failed to start Load and ConvertToMD thread...previous operation not complete" );
   }
   else if ( m_uiForm.UseExistingWorkspaces_rbtn->isChecked() )// check existing
   {                                                           // workspaces
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
 *  Slot called when the Browse button for loading data from an event file
 *  is pressed on the SelectData tab.
 */
void MantidEV::loadEventFile_slot()
{
  QString file_path;
  if ( last_event_file.length() != 0 )
  {
    QString Qfile_name = QString::fromStdString( last_event_file );
    QFileInfo file_info( Qfile_name );
    file_path = file_info.absolutePath();
  }
  else
  {
    file_path = QDir::homePath();
  }

  QString Qfile_name = QFileDialog::getOpenFileName( this,
                              tr("Load event file"),
                              file_path,
                              tr("Nexus Files (*.nxs);; All files(*.*)"));

  if ( Qfile_name.length() > 0 )
  {
    last_event_file = Qfile_name.toStdString();
    m_uiForm.EventFileName_ledt->setText( Qfile_name );
  }
}


/**
 *  Slot called when the Apply button is pressed on the Find Peaks tab.
 */
void MantidEV::findPeaks_slot()
{
   std::string peaks_ws_name = m_uiForm.PeaksWorkspace_ledt->text().toStdString();
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
     std::string md_ws_name  = m_uiForm.MDworkspace_ledt->text().toStdString();
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

     RunFindPeaks* runner = new RunFindPeaks( worker,
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
     std::string file_name = m_uiForm.SelectPeaksFile_ledt->text().toStdString();
     if ( file_name.length() == 0 )
     {
       errorMessage("Specify a peaks file with the peaks to be loaded.");
       return;
     }
 
     if ( !worker->loadIsawPeaks( peaks_ws_name, file_name ) )
     {
       errorMessage("Could not load requested peaks file");
     }
   }
}


/**
 *  Slot called when the Browse button for loading peaks from a peaks file
 *  is pressed on the FindPeaks tab.
 */
void MantidEV::getLoadPeaksFileName_slot()
{
  QString file_path;
  if ( last_peaks_file.length() != 0 )
  {
    QString Qfile_name = QString::fromStdString( last_peaks_file );
    QFileInfo file_info( Qfile_name );
    file_path = file_info.absolutePath();
  }
  else
  {
    file_path = QDir::homePath();
  }

  QString Qfile_name = QFileDialog::getOpenFileName( this,
                         tr("Load peaks file"),
                         file_path,
                         tr("Peaks Files (*.peaks *.integrate);; All files(*.*)"));

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
  QString file_path;
  if ( last_peaks_file.length() != 0 )
  {
    QString Qfile_name = QString::fromStdString( last_peaks_file );
    QFileInfo file_info( Qfile_name );
    file_path = file_info.absolutePath();
  }
  else
  {
    file_path = QDir::homePath();
  }

  QString Qfile_name = QFileDialog::getSaveFileName( this,
                          tr("Save peaks file"),
                          file_path,
                          tr("Peaks Files (*.peaks *.integrate);; All files(*.*) "));

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
   std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().toStdString();
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
     if ( !worker->findUBUsingIndexedPeaks( peaks_ws_name ) )
     {
       errorMessage( "Find UB Using Indexed Peaks Failed" );
       return;
     }
   }

   else if ( load_UB )
   {
     std::string file_name = m_uiForm.SelectUBFile_ledt->text().toStdString();
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
         if (!getPositiveDouble( m_uiForm.MaxGoniometerChange_ledt,max_degrees))
           return;

         if ( !worker->optimizePhiChiOmega( peaks_ws_name, max_degrees ) )
         {
           errorMessage("Failed to Optimize Phi, Chi and Omega");
           return;
         }
       }
     }
   }

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
}


/**
 *  Slot called when the brows button is pressed for getting the UB file name
 *  to load, on the Find UB tab.
 */
void MantidEV::getLoadUB_FileName_slot()
{
  QString file_path;
  if ( last_UB_file.length() != 0 )
  {
    QString Qfile_name = QString::fromStdString( last_UB_file );
    QFileInfo file_info( Qfile_name );
    file_path = file_info.absolutePath();
  }
  else
  {
    file_path = QDir::homePath();
  }

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
  QString file_path;
  if ( last_UB_file.length() != 0 )
  {
    QString Qfile_name = QString::fromStdString( last_UB_file );
    QFileInfo file_info( Qfile_name );
    file_path = file_info.absolutePath();
  }
  else
  {
    file_path = QDir::homePath();
  }

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
   std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().toStdString();
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
     double max_scalar_error = 0;
     if ( !getPositiveDouble( m_uiForm.MaxScalarError_ledt, max_scalar_error ) )
       return;
     if ( !worker->showCells( peaks_ws_name, max_scalar_error, best_only ) )
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
}


/**
 *  Slot called when the Apply button is pressed on the Change HKL tab.
 */
void MantidEV::changeHKL_slot()
{
   std::string peaks_ws_name = m_uiForm.PeaksWorkspace_ledt->text().toStdString();
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
}


/**
 *  Slot called when the Apply button is pressed on the Integrate tab.
 */
void MantidEV::integratePeaks_slot()
{
   std::string peaks_ws_name = m_uiForm.PeaksWorkspace_ledt->text().toStdString();

   if ( peaks_ws_name.length() == 0 )
   {
     errorMessage("Specify a peaks workspace name on the Find Peaks tab.");
     return;
   }

   std::string event_ws_name = m_uiForm.SelectEventWorkspace_ledt->text().toStdString();
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

   if ( sphere_integrate )
   {
     double peak_radius    = 0.20;
     double inner_radius   = 0.20;
     double outer_radius   = 0.25;
     if ( !getPositiveDouble( m_uiForm.PeakRadius_ledt, peak_radius ) )
       return;

     if ( !getPositiveDouble(m_uiForm.BackgroundInnerRadius_ledt, inner_radius))
       return;

     if ( !getPositiveDouble(m_uiForm.BackgroundOuterRadius_ledt, outer_radius))
       return;

     bool integrate_edge = m_uiForm.IntegrateEdge_ckbx->isChecked();

     RunSphereIntegrate * runner = new RunSphereIntegrate( worker,
                                        peaks_ws_name, event_ws_name,
                                        peak_radius, inner_radius, outer_radius,
                                        integrate_edge );

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
 *  Slot called when the Save Settings action is selected from the File menu.
 */
void MantidEV::saveState_slot()
{
  QString file_path;
  if ( last_ini_file.length() != 0 )
  {
    QString Qfile_name = QString::fromStdString( last_ini_file );
    QFileInfo file_info( Qfile_name );
    file_path = file_info.absolutePath();
  }
  else
  {
    file_path = QDir::homePath();
  }

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
  QString file_path;
  if ( last_ini_file.length() != 0 )
  {
    QString Qfile_name = QString::fromStdString( last_ini_file );
    QFileInfo file_info( Qfile_name );
    file_path = file_info.absolutePath();
  }
  else
  {
    file_path = QDir::homePath();
  }

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
  std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().toStdString();
  if ( peaks_ws_name.length() == 0 )
  {
    errorMessage("Specify a peaks workspace name on the Find Peaks tab.");
    return;
  }

  getSaveUB_FileName();

  std::string file_name = m_uiForm.SelectUBFile_ledt->text().toStdString();
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
  std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().toStdString();
  if ( peaks_ws_name.length() == 0 )
  {
    errorMessage("Specify a peaks workspace name on the Find Peaks tab.");
    return;
  }

  getLoadUB_FileName_slot();

  std::string file_name = m_uiForm.SelectUBFile_ledt->text().toStdString();
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
  std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().toStdString();
  if ( peaks_ws_name.length() == 0 )
  {
    errorMessage("Specify a peaks workspace name on the Find Peaks tab.");
    return;
  }

  getSavePeaksFileName();

  std::string file_name = m_uiForm.SelectPeaksFile_ledt->text().toStdString();
  if ( file_name.length() == 0 )
  {
     errorMessage("Specify a peaks file name for saving the peaks workspace.");
     return;
  }
  else
  {
    if ( !worker->saveIsawPeaks( peaks_ws_name, file_name, false ) )
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
  std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().toStdString();
  if ( peaks_ws_name.length() == 0 )
  {
    errorMessage("Specify a peaks workspace name on the Find Peaks tab.");
    return;
  }

  getLoadPeaksFileName_slot();

  std::string file_name = m_uiForm.SelectPeaksFile_ledt->text().toStdString();
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
 *  Slot called when the Show UB action is selected from the View menu.
 */
void MantidEV::showUB_slot()
{
  std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().toStdString();
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
  m_uiForm.EventFileName_lbl->setEnabled( on );
  m_uiForm.EventFileName_ledt->setEnabled( on );
  m_uiForm.SelectEventFile_btn->setEnabled( on );
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
  if ( getDouble( ledt->text().toStdString(), value ) )
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
  state->setValue("LoadEventFile_rbtn", m_uiForm.LoadEventFile_rbtn->isChecked());
  state->setValue("EventFileName_ledt", m_uiForm.EventFileName_ledt->text());
  state->setValue("UseExistingWorkspaces_rbtn", m_uiForm.UseExistingWorkspaces_rbtn->isChecked());

                                                // Save Tab 2, Find Peaks
  state->setValue("PeaksWorkspace_ledt", m_uiForm.PeaksWorkspace_ledt->text());
  state->setValue("FindPeaks_rbtn", m_uiForm.FindPeaks_rbtn->isChecked());
  state->setValue("MaxABC_ledt", m_uiForm.MaxABC_ledt->text());
  state->setValue("NumToFind_ledt", m_uiForm.NumToFind_ledt->text());
  state->setValue("MinIntensity_ledt", m_uiForm.MinIntensity_ledt->text());
  state->setValue("UseExistingPeaksWorkspace_rbtn", m_uiForm.UseExistingPeaksWorkspace_rbtn->isChecked());
  state->setValue("LoadIsawPeaks_rbtn", m_uiForm.LoadIsawPeaks_rbtn->isChecked());
  state->setValue("SelectPeaksFile_ledt", m_uiForm.SelectPeaksFile_ledt->text());

                                                // Save Tab 3, Find UB 
  state->setValue("FindUBUsingFFT_rbtn", m_uiForm.FindUBUsingFFT_rbtn->isChecked());
  state->setValue("MinD_ledt", m_uiForm.MinD_ledt->text());
  state->setValue("MaxD_ledt", m_uiForm.MaxD_ledt->text());
  state->setValue("FFTTolerance_ledt", m_uiForm.FFTTolerance_ledt->text());
  state->setValue("FindUBUsingIndexedPeaks_rbtn", m_uiForm.FindUBUsingIndexedPeaks_rbtn->isChecked());
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
  state->setValue("IntegrateEdge_ckbx",m_uiForm.IntegrateEdge_ckbx->isChecked());
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
  restore( state, "LoadEventFile_rbtn", m_uiForm.LoadEventFile_rbtn );
  restore( state, "EventFileName_ledt", m_uiForm.EventFileName_ledt );
  restore( state, "UseExistingWorkspaces_rbtn", m_uiForm.UseExistingWorkspaces_rbtn );

                                                  // Load Tab 2, Find Peaks
  restore( state, "PeaksWorkspace_ledt", m_uiForm.PeaksWorkspace_ledt );
  restore( state, "FindPeaks_rbtn", m_uiForm.FindPeaks_rbtn );
  restore( state, "MaxABC_ledt", m_uiForm.MaxABC_ledt );
  restore( state, "NumToFind_ledt", m_uiForm.NumToFind_ledt );
  restore( state, "MinIntensity_ledt", m_uiForm.MinIntensity_ledt );
  restore( state, "UseExistingPeaksWorkspace_rbtn", m_uiForm.UseExistingPeaksWorkspace_rbtn );
  restore( state, "LoadIsawPeaks_rbtn", m_uiForm.LoadIsawPeaks_rbtn );
  restore( state, "SelectPeaksFile_ledt", m_uiForm.SelectPeaksFile_ledt );

                                                  // Load Tab 3, Find UB 
  restore( state, "FindUBUsingFFT_rbtn", m_uiForm.FindUBUsingFFT_rbtn );
  restore( state, "MinD_ledt", m_uiForm.MinD_ledt );
  restore( state, "MaxD_ledt", m_uiForm.MaxD_ledt );
  restore( state, "FFTTolerance_ledt", m_uiForm.FFTTolerance_ledt );
  restore( state, "FindUBUsingIndexedPeaks_rbtn", m_uiForm.FindUBUsingIndexedPeaks_rbtn );
  restore( state, "LoadISAWUB_rbtn", m_uiForm.LoadISAWUB_rbtn );
  restore( state, "SelectUBFile_ledt", m_uiForm.SelectUBFile_ledt );
  restore( state, "OptimizeGoniometerAngles_ckbx", m_uiForm.OptimizeGoniometerAngles_ckbx );
  restore( state, "MaxGoniometerChange_ledt", m_uiForm.MaxGoniometerChange_ledt );
  restore( state, "UseCurrentUB_rbtn", m_uiForm.UseCurrentUB_rbtn );
  restore( state, "IndexPeaks_ckbx", m_uiForm.IndexPeaks_ckbx );
  restore( state, "IndexingTolerance_ledt", m_uiForm.IndexingTolerance_ledt );
  restore( state, "RoundHKLs_ckbx", m_uiForm.RoundHKLs_ckbx );

                                                // Load Tab 4, Choose Cell
  restore( state, "ShowPossibleCells_rbtn", m_uiForm.ShowPossibleCells_rbtn );
  restore( state, "MaxScalarError_ledt", m_uiForm.MaxScalarError_ledt );
  restore( state, "BestCellOnly_ckbx", m_uiForm.BestCellOnly_ckbx );
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
  restore( state, "IntegrateEdge_ckbx", m_uiForm.IntegrateEdge_ckbx );
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

                                                // load info for file paths
  last_UB_file    = state->value("last_UB_file", "").toString().toStdString();
  last_event_file = state->value("last_event_file", "").toString().toStdString();
  last_peaks_file = state->value("last_peaks_file", "").toString().toStdString();
  last_ini_file   = state->value("last_ini_file", "").toString().toStdString();

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
