#ifndef MANTIDQTCUSTOMINTERFACES_MANTID_EV_H_
#define MANTIDQTCUSTOMINTERFACES_MANTID_EV_H_

#include <QtCore/QtCore>
#include <QtGui/QWidget>
#include <QActionGroup>
#include <QRunnable>
#include <Poco/NObserver.h>

#include <MantidKernel/System.h>

#include "ui_MantidEV.h"
#include "MantidEVWorker.h"
#include "MantidQtAPI/UserSubWindow.h"
#include "MantidQtAPI/SelectionNotificationService.h"

namespace MantidQt
{
namespace CustomInterfaces
{

// NOTE: The first several internal classes are just simple QRunnable
// objects that will run worker code using one or more algorithms, in
// a separate thread.  This avoids blocking the MantidPlot GUI by 
// keeping the main Qt thread free.
//

//
// START OF SIMPLE QRunnable classes -------------------------------------
//

/// Local class to load file and convert to MD in a Non-Qt thread.
class RunLoadAndConvertToMD : public QRunnable
{
  public:

  /// Constructor just saves the info needed by the run() method
  RunLoadAndConvertToMD(MantidEVWorker * worker,
                         const std::string    & file_name,
                         const std::string    & ev_ws_name,
                         const std::string    & md_ws_name,
                         const double           minQ,
                         const double           maxQ,
                         const bool             do_lorentz_corr,
                         const bool             load_data,
                         const bool             load_det_cal,
                         const std::string    & det_cal_file,
                         const std::string    & det_cal_file2 );

  /// Calls worker->loadAndConvertToMD from a separate thread
  void run();

  private:
    MantidEVWorker * worker;
    std::string      file_name;
    std::string      ev_ws_name;
    std::string      md_ws_name;
    double           minQ;
    double           maxQ;
    bool             do_lorentz_corr;
    bool             load_data;
    bool             load_det_cal;
    std::string      det_cal_file;
    std::string      det_cal_file2;
};


/// Local class to run FindPeaks in a Non-Qt thread.
class RunFindPeaks : public QRunnable
{
  public:

  /// Constructor just saves the info needed by the run() method
  RunFindPeaks(       MantidEVWorker * worker,
                const std::string    & ev_ws_name,
                const std::string    & md_ws_name,
                const std::string    & peaks_ws_name,
                      double           max_abc,
                      size_t           num_to_find,
                      double           min_intensity );

  /// Calls worker->findPeaks from a separate thread
  void run();

  private:
    MantidEVWorker * worker;
    std::string      ev_ws_name;
    std::string      md_ws_name;
    std::string      peaks_ws_name;
    double           max_abc;
    size_t           num_to_find;
    double           min_intensity;
};

/// Local class to run PredictPeaks in a Non-Qt thread.
class RunPredictPeaks : public QRunnable
{
  public:

  /// Constructor just saves the info needed by the run() method
  RunPredictPeaks(       MantidEVWorker * worker,
                const std::string    & peaks_ws_name,
                      double           min_pred_wl,
                      double           max_pred_wl,
                      double           min_pred_dspacing,
                      double           max_pred_dspacing );

  /// Calls worker->predictPeaks from a separate thread
  void run();

  private:
    MantidEVWorker * worker;
    std::string      peaks_ws_name;
    double           min_pred_wl;
    double           max_pred_wl;
    double           min_pred_dspacing;
    double           max_pred_dspacing;
};

/// Local class to run IntegratePeaksMD in a Non-Qt thread.
class RunSphereIntegrate : public QRunnable
{
  public:

  /// Constructor just saves the info needed by the run() method
  RunSphereIntegrate(       MantidEVWorker * worker,
                      const std::string    & peaks_ws_name,
                      const std::string    & event_ws_name,
                            double           peak_radius,
                            double           inner_radius,
                            double           outer_radius,
                            bool             integrate_edge,
                            bool             use_cylinder_integration,
                            double           cylinder_length,
                            double           cylinder_percent_bkg,
                      const std::string &    cylinder_profile_fit);

  /// Calls worker->sphereIntegrate from a separate thread
  void run();

  private:
    MantidEVWorker * worker;
    std::string      peaks_ws_name;
    std::string      event_ws_name;
    double           peak_radius; 
    double           inner_radius; 
    double           outer_radius; 
    bool             integrate_edge; 
    bool             use_cylinder_integration;
    double           cylinder_length;
    double           cylinder_percent_bkg;
    std::string      cylinder_profile_fit;
};


/// Local class to run PeakIntegration in a Non-Qt thread.
class RunFitIntegrate : public QRunnable
{
  public:

  /// Constructor just saves the info needed by the run() method
  RunFitIntegrate(       MantidEVWorker * worker,
                   const std::string    & peaks_ws_name,
                   const std::string    & event_ws_name,
                   const std::string    & rebin_params,
                         size_t           n_bad_edge_pix,
                         bool             use_ikeda_carpenter );

  /// Calls worker->fitIntegrate from a separate thread
  void run();

  private:
    MantidEVWorker * worker;
    std::string      peaks_ws_name;
    std::string      event_ws_name;
    std::string      rebin_params;
    size_t           n_bad_edge_pix;
    bool             use_ikeda_carpenter;
};


/// Local class to run ellipsoidIntegrate in a Non-Qt thread.
class RunEllipsoidIntegrate : public QRunnable
{
  public:

  /// Constructor just saves the info needed by the run() method
  RunEllipsoidIntegrate(       MantidEVWorker * worker,
                         const std::string    & peaks_ws_name,
                         const std::string    & event_ws_name,
                               double           region_radius,
                               bool             specify_size,
                               double           peak_size, 
                               double           inner_size, 
                               double           outer_size );

  /// Calls worker->ellipsoidIntegrate from a separate thread
  void run();

  private:
    MantidEVWorker * worker;
    std::string      peaks_ws_name;
    std::string      event_ws_name;
    double           region_radius;
    bool             specify_size;
    double           peak_size;
    double           inner_size;
    double           outer_size;
};


//
// END OF SIMPLE QRunnable classes -------------------------------------
//
// START of the actual MantidEV Class ----------------------------------
//

/**
 *  The MantidEV class has slots that handle user input from the Qt GUI
 *  and then call methods in the MantidEVWorker class.  Roughly speaking,
 *  MantidEV deals with the Qt GUI and MantideEVWorker deals with Mantid.
 */
class MantidEV : public API::UserSubWindow
{
  Q_OBJECT

public:

  /// Constructor
  MantidEV(QWidget *parent = 0);

  /// Destructor
  ~MantidEV();

  /// The name of the interface as registered into the factory
  static std::string name() { return "SCD Event Data Reduction"; }
  // This interface's categories.
  static QString categoryInfo() { return "Diffraction"; }

public slots:
  /// Slot for Q-Point selection notification
  void QPointSelection_slot( bool, double, double, double );

private slots:

  /// Go to help page
  void help_slot();

  /// Slot for the select workspace tab's Apply button 
  void selectWorkspace_slot();

  /// Slot for the Finished Editing text for loading an event file 
  void loadEventFileEntered_slot();

  /// Slot for the Browse button for loading an event file 
  void loadEventFile_slot();

  /// Slot for the Browse button for loading the first calibration file 
  void selectDetCalFile_slot();

  /// Slot for the Browse button for loading the second calibration file 
  void selectDetCalFile2_slot();

  /// Slot for the find peaks tab's Apply button 
  void findPeaks_slot();

  /// Slot for choosing a peaks file name
  void getLoadPeaksFileName_slot();

  /// Slot for the find UB tab's Apply button 
  void findUB_slot();

  /// Slot for choosing a matrix file name
  void getLoadUB_FileName_slot();

  /// Slot for the choose cell tab's Apply button 
  void chooseCell_slot();

  /// Slot for the change HKL tab's Apply button 
  void changeHKL_slot();

  /// Slot for the integrate tab's Apply button 
  void integratePeaks_slot();

  /// Slot for Show Info button on Point Info form
  void showInfo_slot();

  // 
  // The following slots take care of the menu items
  //
 
  /// Slot to save the current MantidEV GUI state
  void saveState_slot();

  /// Slot to load a previous MantidEV GUI state
  void loadState_slot();

  /// Slot to restore default GUI state
  void setDefaultState_slot();

  /// Slot to save the UB matrix from the current MantidEV peaks workspace
  void saveIsawUB_slot();

  /// Slot to a previous UB matrix into the current MantidEV peaks workspace
  void loadIsawUB_slot();

  /// Slot to save the current MantidEV peaks workspace 
  void saveIsawPeaks_slot();

  /// Slot to load a peaks workspace to the current MantidEV named workspace
  void loadIsawPeaks_slot();

  /// Slot to save the current MantidEV peaks workspace
  void saveNexusPeaks_slot();

  /// Slot to load a peaks workspace to the current MantidEV named workspace
  void loadNexusPeaks_slot();

  /// Slot to show the UB matrix
  void showUB_slot();
  
  //
  // The following slots just take care of enabling and disabling
  // some of the controls, as needed
  //

  /// Slot to enable/disable the Load Event File controls
  void setEnabledLoadEventFileParams_slot( bool on );

  /// Slot to enable/disable the .DetCal file info
  void setEnabledLoadCalFiles_slot();

  /// Slot to enable/disable the find peaks controls
  void setEnabledFindPeaksParams_slot( bool on );

  /// Slot to enable/disable the predict peaks controls
  void setEnabledPredictPeaksParams_slot();

  /// Slot to enable/disable the Load Peaks File controls
  void setEnabledLoadPeaksParams_slot( bool on );

  /// Slot to enable/disable the find UB using FFT controls
  void setEnabledFindUBFFTParams_slot( bool on );

  /// Slot to enable/disable the find UB using Indexed Peaks controls
  void setEnabledFindUBUsingIndexedPeaksParams_slot( bool on );

  /// Slot to enable/disable the load UB controls
  void setEnabledLoadUBParams_slot( bool on );

  /// Slot to enable/disable the optimize goniometer angles controls
  void setEnabledMaxOptimizeDegrees_slot();

  /// Slot to enable/disable the index peaks controls
  void setEnabledIndexParams_slot( bool on );

  /// Slot to enable/disable the show conventional cell controls
  void setEnabledShowCellsParams_slot( bool on );

  /// Slot to enable/disable the select cell based on cell type controls
  void setEnabledSetCellTypeParams_slot(bool on );

  /// Slot to enable/disable the select cell based on cell number controls
  void setEnabledSetCellFormParams_slot(bool on );

  /// Slot to enable/disable the sphere integration controls
  void setEnabledSphereIntParams_slot( bool on );

  /// Slot to enable/disable the 2D fitting integration controls
  void setEnabledFitIntParams_slot( bool on );

  /// Slot to enable/disable the ellipsoidal integration controls
  void setEnabledEllipseIntParams_slot( bool on );

  /// Slot to enable/disable the ellipse size options controls
  void setEnabledEllipseSizeOptions_slot();

  /// Method to get and display info about the specified Q-vector
  void showInfo( bool lab_coords, Mantid::Kernel::V3D  q_point );

private:
  /// super class pure virtual method we MUST implement
  virtual void initLayout();

  /// Utility method to display an error message
  void errorMessage( const std::string & message );

  /// Utility method to parse a double value in a string
  bool getDouble( std::string str, double & value );

  /// Utility method to get a double value from a QtLineEdit widget 
  bool getDouble( QLineEdit *ledt, double & value );

  /// Utility method to get a positive double from a QtLineEdit widget 
  bool getPositiveDouble( QLineEdit *ledt, double & value );

  /// Utility method to get a positive integer value from a QtLineEdit widget 
  bool getPositiveInt( QLineEdit *ledt, size_t & value );

  /// Get base file name to form names for event, MD and peaks workspaces
  std::string extractBaseFileName( std::string FullFileName) const;

  /// Get path name from file, or user's home directory
  QString getFilePath( const std::string & file_name );

  /// Get name of file for saving peaks
  void getSavePeaksFileName();

  /// Get name of file for saving UB matrix 
  void getSaveUB_FileName();

  /// Save QSettings to specified file, or default, if filename empty
  void saveSettings( const std::string & filename );

  /// Load QSettings from specified file, or default, if filename empty
  void loadSettings( const std::string & filename );

  /// Restore the value of the QLineEdit component from QSettings
  void restore( QSettings *state, QString name, QLineEdit *ledt );

  /// Restore the value of the QCheckbox or QRadioButton from QSettings
  void restore( QSettings *state, QString name, QAbstractButton *btn );

  /// Restore the value of a QComboBox from QSettings
  void restore( QSettings *state, QString name, QComboBox *cmbx );


  Ui::MantidEV   m_uiForm;     /// The form generated by Qt Designer

  MantidEVWorker *worker;      /// class that uses Mantid algorithms
                               /// to do the actual work

  std::string  last_cal_file;  /// filename of last ISAW DetCal file for
                               /// the whole instrument or for panel 1 of SNAP

  std::string  last_cal_file2; /// filename of last ISAW DetCal file for
                               /// panel 2 of SNAP

  std::string  last_UB_file;   /// filename of last UB file that was loaded
                               /// or saved from MantidEV, if any.

  std::string  last_event_file;/// filename of last event file that was loaded
                               /// or saved from MantidEV, if any.

  std::string  last_peaks_file;/// filename of last peaks file that was loaded
                               /// or saved from MantidEV, if any.
 
  std::string  last_ini_file;  /// filename of last settings file that was
                               /// loaded or saved, if any. 

  Mantid::Kernel::V3D last_Q;  /// the last_Q vector that was received from
                               /// the SelectionNotificationService

  QThreadPool  *m_thread_pool; /// local thread pool with only one thread to 
                               /// allow running precisely one operation 
                               /// at a time in a separate thread.

};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif //MANTIDQTCUSTOMINTERFACES_MANTID_EV_H_
