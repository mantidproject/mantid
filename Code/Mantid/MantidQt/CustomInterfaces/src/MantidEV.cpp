
#include <QMessageBox>
#include <QFileDialog>

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

/// Constructor
MantidEV::MantidEV(QWidget *parent)
  : UserSubWindow(parent)
{
  worker = new MantidEVWorker();
}

MantidEV::~MantidEV()
{
  delete worker;
}

/// Set up the dialog layout
void MantidEV::initLayout()
{
  m_uiForm.setupUi(this);

                          // connect the apply buttons to the code that
                          // gathers the parameters and will call a method
                          // to carry out the requested action
   QObject::connect( m_uiForm.ApplySelectData_btn, SIGNAL(clicked()),
                    this, SLOT(selectWorkspace_slot()) );

   QObject::connect( m_uiForm.ApplyFindPeaks_btn, SIGNAL(clicked()),
                    this, SLOT(findPeaks_slot()) );

   QObject::connect( m_uiForm.ApplyFindUB_btn, SIGNAL(clicked()),
                    this, SLOT(findUB_slot()) );

   QObject::connect( m_uiForm.SelectUBFile_btn, SIGNAL(clicked()),
                     this, SLOT(loadUB_slot()) );

   QObject::connect( m_uiForm.ApplyChooseCell_btn, SIGNAL(clicked()),
                    this, SLOT(chooseCell_slot()) );

   QObject::connect( m_uiForm.ApplyChangeHKL_btn, SIGNAL(clicked()),
                    this, SLOT(changeHKL_slot()) );

   QObject::connect( m_uiForm.ApplyIntegrate_btn, SIGNAL(clicked()),
                    this, SLOT(integratePeaks_slot()) );


                          // connect the slots for enabling and disabling
                          // various subsets of widgets
   QObject::connect( m_uiForm.FindPeaks_rbtn, SIGNAL(toggled(bool)),
                     this, SLOT( setEnabledFindPeaksParams_slot(bool) ) );

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

   m_uiForm.FindPeaks_rbtn->setChecked(true);
   m_uiForm.UseExistingPeaksWorkspace_rbtn->setChecked(false);
   setEnabledFindPeaksParams_slot(true);

   m_uiForm.FindUBUsingFFT_rbtn->setChecked(true);
   m_uiForm.FindUBUsingIndexedPeaks_rbtn->setChecked(false);
   m_uiForm.LoadISAWUB_rbtn->setChecked(false);
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

}


void MantidEV::selectWorkspace_slot()
{
   std::cout << std::endl << "Apply Select Data ....." << std::endl;
   std::string ws_name = m_uiForm.MDworkspace_ledt->text().toStdString();

   if ( ws_name.length() == 0 )
   {
     errorMessage("Specify the name of an MD Workspace on Select Data tab.");
     return;
   }

   if ( !worker->selectMDWorkspace( ws_name ) )
   {
     errorMessage("Requested Workspace is NOT an MD workspace");
   }
}


void MantidEV::findPeaks_slot()
{
   std::cout << std::endl << "Apply Find Peaks ....." << std::endl;

   std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().toStdString();
   if ( peaks_ws_name.length() == 0 )
   {
     errorMessage("Specify a peaks workspace name on Find Peaks tab.");
     return;
   }

   bool find_new_peaks        = m_uiForm.FindPeaks_rbtn->isChecked();
   bool use_existing_peaks    = !find_new_peaks;

   double max_abc       = 15;
   size_t num_to_find   = 50;
   double min_intensity = 10;

   if ( !getPositiveDouble( m_uiForm.MaxABC_ledt, max_abc ) )
     return;

   if ( !getPositiveInt( m_uiForm.NumToFind_ledt, num_to_find ) )
     return;

   if ( !getPositiveDouble( m_uiForm.MinIntensity_ledt, min_intensity ) )
     return;

   if ( use_existing_peaks )
   {
     if ( !worker->selectPeaksWorkspace( peaks_ws_name ) )
     {
       errorMessage("Requested Peaks Workspace Doesn't Exist");
     }
   }

   else if ( find_new_peaks )
   {
     if ( !worker->findPeaks( peaks_ws_name, max_abc, num_to_find, min_intensity ))
     {
       errorMessage("findPeaks failed");
     }
   }
}


void MantidEV::findUB_slot()
{
   std::cout << std::endl << "Apply Find UB ....." << std::endl;

   std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().toStdString();
   if ( peaks_ws_name.length() == 0 )
   {
     errorMessage("Specify a peaks workspace name on Find Peaks tab.");
     return;
   }

   bool use_FFT          = m_uiForm.FindUBUsingFFT_rbtn->isChecked();
   bool use_IndexedPeaks = m_uiForm.FindUBUsingIndexedPeaks_rbtn->isChecked();
   bool load_UB          = m_uiForm.LoadISAWUB_rbtn->isChecked();
   bool index_peaks      = m_uiForm.IndexPeaks_ckbx->isChecked();
   bool round_hkls       = m_uiForm.RoundHKLs_ckbx->isChecked();
   bool optimize_angles  = m_uiForm.OptimizeGoniometerAngles_ckbx->isChecked();

   double min_abc         = 3;
   double max_abc         = 15;
   double fft_tolerance   = 0.12;
   double max_degrees     = 5;
   double index_tolerance = 0.12;


   if ( use_FFT )
   {
     if ( !getPositiveDouble( m_uiForm.MinD_ledt, min_abc ) )
       return;

     if ( !getPositiveDouble( m_uiForm.MaxD_ledt, max_abc ) )
       return;

     if ( !getPositiveDouble( m_uiForm.FFTTolerance_ledt, fft_tolerance ) )
       return;

     if ( !worker->findUBUsingFFT( peaks_ws_name, min_abc, max_abc, fft_tolerance ) )
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
         if ( !getPositiveDouble( m_uiForm.MaxGoniometerChange_ledt, max_degrees ) )
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
     if ( !getPositiveDouble( m_uiForm.IndexingTolerance_ledt, index_tolerance ) )
       return;

     if ( !worker->indexPeaksWithUB( peaks_ws_name, index_tolerance, round_hkls ) )
     {
       errorMessage("Failed to Index Peaks with the Existing UB Matrix");
     }
   }
}


void MantidEV::loadUB_slot()
{
  std::cout << "Load UB file Browse button pushed... " << std::endl;

  QString file_path;
  if ( last_UB_file.length() != 0 )
  {
    QString Qfile_name = QString::fromUtf8( last_UB_file.c_str() );
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
                                                tr("Matrix Files (*.mat)"));

  last_UB_file = Qfile_name.toStdString();

  m_uiForm.SelectUBFile_ledt->setText( Qfile_name );
}


void MantidEV::chooseCell_slot()
{
   std::cout << std::endl << "Apply Choose Cell ....." << std::endl;
   std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().toStdString();
   if ( peaks_ws_name.length() == 0 )
   {
     errorMessage("Specify a peaks workspace name on Find Peaks tab.");
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


void MantidEV::changeHKL_slot()
{
   std::cout << std::endl << "Apply Change HKL ....." << std::endl;
   std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().toStdString();
   if ( peaks_ws_name.length() == 0 )
   {
     errorMessage("Specify a peaks workspace name on Find Peaks tab.");
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



void MantidEV::integratePeaks_slot()
{
   std::cout << std::endl <<"Apply Integrate ....."<<std::endl;
   std::string peaks_ws_name  = m_uiForm.PeaksWorkspace_ledt->text().toStdString();
   if ( peaks_ws_name.length() == 0 )
   {
     errorMessage("Specify a peaks workspace name on Find Peaks tab.");
     return;
   }

   std::string event_ws_name =
                            m_uiForm.SelectEventWorkspace_ledt->text().toStdString();
   if ( event_ws_name.length() == 0 )
   {
     errorMessage("Specify a time-of-flight event workspace name.");
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

     if ( !getPositiveDouble( m_uiForm.BackgroundInnerRadius_ledt, inner_radius ) )
       return;

     if ( !getPositiveDouble( m_uiForm.BackgroundOuterRadius_ledt, outer_radius ) )
       return;

     bool integrate_edge = m_uiForm.IntegrateEdge_ckbx->isChecked();
     if ( !worker->sphereIntegrate( peaks_ws_name, event_ws_name,
                                    peak_radius, inner_radius, outer_radius,
                                    integrate_edge ) )
     {
       errorMessage("Failed to Integrate Peaks using Sphere Integration");
     }
   }
   else if ( fit_integrate )
   {
     bool use_ikeda_carpenter = m_uiForm.IkedaCarpenter_ckbx->isChecked();
     std::string rebin_params = m_uiForm.FitRebinParams_ledt->text().toStdString();
     double n_bad_edge_pix = 5;
     if ( !getPositiveDouble( m_uiForm.NBadEdgePixels_ledt, n_bad_edge_pix ) )
       return;

     if ( !worker->fitIntegrate( peaks_ws_name, event_ws_name,
                                 rebin_params,
                                 (size_t)n_bad_edge_pix,
                                 use_ikeda_carpenter ) )
     {
       errorMessage( "Failed to Integrate Peaks using 2D Fit" );
     }
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

       if ( !getPositiveDouble( m_uiForm.BackgroundInnerSize_ledt, inner_size ) )
         return;

       if ( !getPositiveDouble( m_uiForm.BackgroundOuterSize_ledt, outer_size ) )
         return;
     }

     if ( !worker->ellipsoidIntegrate( peaks_ws_name, event_ws_name,
                                       region_radius,
                                       specify_size,
                                       peak_size, inner_size, outer_size ) )
     {
       errorMessage( "Failed to Integrate Peaks using 3D Ellipsoids" );
     }
   }
}


void MantidEV::setEnabledFindPeaksParams_slot( bool on )
{
  m_uiForm.MaxABC_lbl->setEnabled( on );
  m_uiForm.MaxABC_ledt->setEnabled( on );
  m_uiForm.NumToFind_lbl->setEnabled( on );
  m_uiForm.NumToFind_ledt->setEnabled( on );
  m_uiForm.MinIntensity_lbl->setEnabled( on );
  m_uiForm.MinIntensity_ledt->setEnabled( on );
}


void MantidEV::setEnabledFindUBFFTParams_slot( bool on )
{
  m_uiForm.MinD_lbl->setEnabled( on );
  m_uiForm.MinD_ledt->setEnabled( on );
  m_uiForm.MaxD_lbl->setEnabled( on );
  m_uiForm.MaxD_ledt->setEnabled( on );
  m_uiForm.FFTTolerance_lbl->setEnabled( on );
  m_uiForm.FFTTolerance_ledt->setEnabled( on );
}


void MantidEV::setEnabledLoadUBParams_slot( bool on )
{
  m_uiForm.SelectUBFile_lbl->setEnabled( on );
  m_uiForm.SelectUBFile_ledt->setEnabled( on );
  m_uiForm.SelectUBFile_btn->setEnabled( on );
  m_uiForm.OptimizeGoniometerAngles_ckbx->setEnabled( on );
  setEnabledMaxOptimizeDegrees_slot();
}


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


void MantidEV::setEnabledIndexParams_slot( bool on )
{
  m_uiForm.IndexingTolerance_lbl->setEnabled( on );
  m_uiForm.IndexingTolerance_ledt->setEnabled( on );
  m_uiForm.RoundHKLs_ckbx->setEnabled( on );
}


void MantidEV::setEnabledShowCellsParams_slot( bool on )
{
  m_uiForm.MaxScalarError_lbl->setEnabled( on );
  m_uiForm.MaxScalarError_ledt->setEnabled( on );
  m_uiForm.BestCellOnly_ckbx->setEnabled( on );
}


void MantidEV::setEnabledSetCellTypeParams_slot( bool on )
{
  m_uiForm.CellType_cmbx->setEnabled( on );
  m_uiForm.CellCentering_cmbx->setEnabled( on );
}


void MantidEV::setEnabledSetCellFormParams_slot( bool on )
{
  m_uiForm.CellFormNumber_cmbx->setEnabled( on );
}


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


void MantidEV::setEnabledFitIntParams_slot( bool on )
{
  m_uiForm.FitRebinParams_lbl->setEnabled( on );
  m_uiForm.FitRebinParams_ledt->setEnabled( on );
  m_uiForm.NBadEdgePixels_lbl->setEnabled( on );
  m_uiForm.NBadEdgePixels_ledt->setEnabled( on );
  m_uiForm.IkedaCarpenter_ckbx->setEnabled( on );
}


void MantidEV::setEnabledEllipseIntParams_slot( bool on )
{
  m_uiForm.RegionRadius_lbl->setEnabled( on );
  m_uiForm.RegionRadius_ledt->setEnabled( on );
  m_uiForm.SpecifySize_ckbx->setEnabled( on );
  setEnabledEllipseSizeOptions_slot();
}


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


void MantidEV::errorMessage( const std::string message )
{
  std::cout << "ERROR: " << message << std::endl;
  QMessageBox::critical(this,"ERROR", QString::fromStdString(message));
}


bool MantidEV::getDouble( std::string str, double &value )
{
  std::istringstream strs( str );
  if ( strs >> value )
  {
    return true;
  }
  return false;
}


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


} // namespace CustomInterfaces
} // namespace MantidQt
