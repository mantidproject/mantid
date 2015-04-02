#include <iostream>
#include <sstream>

#include "MantidQtCustomInterfaces/MantidEVWorker.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/Logger.h"
#include <exception>

namespace MantidQt
{
namespace CustomInterfaces
{

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace
{
  /// static logger
  Mantid::Kernel::Logger g_log("MantidEV");
}

/**
 *  Default constructor
 */
MantidEVWorker::MantidEVWorker()
{
}


/**
 *  Default destructor
 */
MantidEVWorker::~MantidEVWorker()
{
}


/**
 *  Utility method to get the type of a workspace.  If the named workspace
 *  is NOT present in the ADS a zero length string is returned.  If the named
 *  workspace is in the ADS, the id string for the workspace is returned.  
 *  For example for a PeaksWorkspace, the id string returned is 
 *  "PeaksWorkspace".
 *
 *  @param  ws_name   The name of the workspace
 *
 *  @return The workspace id, or and zero length string if the workspace
 *          doesn't exist.
 */
std::string MantidEVWorker::workspaceType( const std::string & ws_name )
{
  const auto& ADS = AnalysisDataService::Instance();

  if ( !ADS.doesExist( ws_name ) )
    return std::string("");

  Workspace_const_sptr outWS = ADS.retrieveWS<Workspace>(ws_name);

  return outWS->id();
}


/**
 * Utility method to check if a name is the name of an MDWorkspace. 
 *
 *  @param  md_ws_name   The name of the workspace
 *
 *  @return true if the named workspace exists and is an MDWorkspace. 
 */
bool MantidEVWorker::isMDWorkspace( const std::string & md_ws_name )
{
  std::string ws_type = workspaceType(md_ws_name);

  if ( ws_type.length() == 0 )
    return false;

  if ( ws_type != "MDEventWorkspace<MDEvent,3>" && ws_type != "MDHistoWorkspace" ) 
    return false;

  return true;
}


/**
 * Utility method to check if a name is the name of a PeaksWorkspace. 
 *
 *  @param  peaks_ws_name   The name of the workspace
 *
 *  @return true if the named workspace exists and is a PeaksWorkspace. 
 */
bool MantidEVWorker::isPeaksWorkspace( const std::string & peaks_ws_name )
{
  std::string ws_type = workspaceType(peaks_ws_name);

  if ( ws_type.length() == 0 )
    return false;

  if ( ws_type != "PeaksWorkspace" )
    return false;

  return true;
}


/**
 * Utility method to check if a name is the name of an EventWorkspace. 
 *
 *  @param  event_ws_name   The name of the workspace
 *
 *  @return true if the named workspace exists and is an EventWorkspace. 
 */
bool MantidEVWorker::isEventWorkspace( const std::string & event_ws_name )
{
  std::string ws_type = workspaceType(event_ws_name);

  if ( ws_type.length() == 0 )
    return false;

  if ( ws_type != "EventWorkspace" )
  {
    return false;
  }

  return true;
}


/**
 *  Load the specified NeXus event file into the specified EventWorkspace
 *  and convert it to the specified MD workspace.
 *
 *  @param file_name        Name of the NeXus file to load
 *  @param ev_ws_name       Name of the event workspace to create
 *  @param md_ws_name       Name of the MD workspace to create
 *  @param minQ             The smallest absolute value of any component
 *                          of Q to include.
 *  @param maxQ             The largest absolute value of any component
 *                          of Q to include. When ConvertToMD is called,
 *                          MinValues = -maxQ,-maxQ,-maxQ   and 
 *                          MaxValues =  maxQ, maxQ, maxQ 
 *  @param do_lorentz_corr  Set true to do the Lorentz correction when
 *                          converting to reciprocal space. 
 *  @param load_data        Set true to load original data.
 *  @param load_det_cal     Set true to call LoadIsawDetCal after loading
 *                          the event file.
 *  @param det_cal_file     Fully qualified name of the .DetCal file.
 *  @param det_cal_file2    Fully qualified name of the second .DetCal 
 *                          file for the second panel on SNAP.
 *
 *  @return true if the file was loaded and MD workspace was 
 *          successfully created.
 */
bool MantidEVWorker::loadAndConvertToMD( const std::string & file_name,
                                         const std::string & ev_ws_name,
                                         const std::string & md_ws_name,
                                         const double        minQ,
                                         const double        maxQ,
                                         const bool          do_lorentz_corr,
                                         const bool          load_data,
                                         const bool          load_det_cal,
                                         const std::string & det_cal_file,
                                         const std::string & det_cal_file2 )
{
  try
  {
    IAlgorithm_sptr alg;
    if (load_data)
    {
      IAlgorithm_sptr alg = AlgorithmManager::Instance().create("Load");
      alg->setProperty("Filename",file_name);
      alg->setProperty("OutputWorkspace",ev_ws_name);
      alg->setProperty("Precount",true);
      alg->setProperty("LoadMonitors",true);

      if ( !alg->execute() )
        return false;

      if ( load_det_cal )
      {
        alg = AlgorithmManager::Instance().create("LoadIsawDetCal");
        alg->setProperty( "InputWorkspace", ev_ws_name );
        alg->setProperty( "Filename", det_cal_file );
        alg->setProperty( "Filename2", det_cal_file2 );

        if ( !alg->execute() )
          return false;
      }
    }

    std::ostringstream min_str;
    if (minQ != Mantid::EMPTY_DBL())
      min_str << minQ << "," << minQ << "," << minQ;
    else
      min_str << "-" << maxQ << ",-" << maxQ << ",-" << maxQ;

    std::ostringstream max_str;
    max_str << maxQ << "," << maxQ << "," << maxQ;

    alg = AlgorithmManager::Instance().create("ConvertToMD");
    alg->setProperty("InputWorkspace",ev_ws_name);
    alg->setProperty("OutputWorkspace",md_ws_name);
    alg->setProperty("OverwriteExisting",true);
    alg->setProperty("QDimensions","Q3D");
    alg->setProperty("dEAnalysisMode","Elastic");
    alg->setProperty("QConversionScales","Q in A^-1");
    alg->setProperty("Q3DFrames","Q_sample");
    alg->setProperty("LorentzCorrection",do_lorentz_corr);
    alg->setProperty("MinValues",min_str.str());
    alg->setProperty("MaxValues",max_str.str());
    alg->setProperty("SplitInto","2");
    alg->setProperty("SplitThreshold","50");
    alg->setProperty("MaxRecursionDepth","13");
    alg->setProperty("MinRecursionDepth","7");

    if ( !alg->execute() )
      return false;
  }
  catch( std::exception &e)
  {
    g_log.error()<<"Error:" << e.what() <<std::endl;
    return false;
  }
  catch(...)
  {
    g_log.error()<<"Error: Could Not load file and convert to MD" <<std::endl;
    return false; 
  }
  return true;
}


/**
 *  Find peaks in the specified MD workspace and save them in the
 *  specified peaks workspace.
 *
 *  @param ev_ws_name     Name of the event workspace to create
 *  @param md_ws_name     Name of the MD workspace to use 
 *  @param peaks_ws_name  Name of the peaks workspace to create
 *
 *  @param max_abc        Estimate of the maximum real space
 *                        edge length.  This is used to get an
 *                        estimate of the minimum peak separation
 *                        in reciprocal space.
 *  @param num_to_find    The number of peaks to find.
 *  @param min_intensity  Sets the minimum value to consider an
 *                        MD box to be a possible peak.  If this
 *                        is 10000, only boxes with intensity 10000 times
 *                        the average intensity will be considered.
 *
 *  @return true if FindPeaksMD completed successfully.
 */
bool MantidEVWorker::findPeaks( const std::string & ev_ws_name,
		                        const std::string & md_ws_name,
                                const std::string & peaks_ws_name,
                                      double        max_abc,
                                      size_t        num_to_find,
                                      double        min_intensity )
{
  try
  {
                     // Estimate a lower bound on the distance between
                     // based on the maximum real space cell edge
    double min_separation = 0.9 * 6.28 / max_abc;
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("FindPeaksMD");
    alg->setProperty("InputWorkspace",md_ws_name);
    alg->setProperty("PeakDistanceThreshold", min_separation);
    alg->setProperty("MaxPeaks",(int64_t)num_to_find);
    alg->setProperty("DensityThresholdFactor",min_intensity);
    alg->setProperty("OutputWorkspace", peaks_ws_name );
    const auto& ADS = AnalysisDataService::Instance();

    if ( alg->execute() )
    {
		bool use_monitor_counts = true;
    	double proton_charge = 0.0;
    	double monitor_count = 0.0;
		if (use_monitor_counts)
		{
		  Mantid::API::MatrixWorkspace_sptr mon_ws = ADS.retrieveWS<MatrixWorkspace>(ev_ws_name + "_monitors" );
		  IAlgorithm_sptr int_alg = AlgorithmManager::Instance().create("Integration");
		  int_alg->setProperty("InputWorkspace", mon_ws );
		  int_alg->setProperty("RangeLower", 1000.0 );
		  int_alg->setProperty("RangeUpper", 12500.0 );
		  int_alg->setProperty("OutputWorkspace", ev_ws_name + "_integrated_monitor" );
		  int_alg->execute();
		  Mantid::API::MatrixWorkspace_sptr int_ws = ADS.retrieveWS<MatrixWorkspace>(ev_ws_name + "_integrated_monitor");
		  monitor_count = int_ws->readY(0)[0];
		  std::cout << "Beam monitor counts used for scaling = " << monitor_count << "\n";
		}
		else
		{
		  Mantid::API::MatrixWorkspace_sptr ev_ws = ADS.retrieveWS<MatrixWorkspace>(ev_ws_name);
		  double proton_charge = ev_ws->run().getProtonCharge()  * 1000.0;  // get proton charge
		  std::cout << "Proton charge x 1000 used for scaling = " << proton_charge << "\n";
		}

		IPeaksWorkspace_sptr peaks_ws = ADS.retrieveWS<IPeaksWorkspace>(peaks_ws_name);
		for (int iPeak=0; iPeak < peaks_ws->getNumberPeaks(); iPeak++)
		{
		  Mantid::API::IPeak& peak = peaks_ws->getPeak( iPeak);
		  if (use_monitor_counts)
			peak.setMonitorCount( monitor_count );
		  else
			peak.setMonitorCount( proton_charge );
		}
      return true;
    }
  }
  catch( std::exception &e)
  {
    g_log.error()<<"Error:" << e.what() <<std::endl;
    return false;
  }
  catch(...)
  {
    g_log.error()<<"Error: Could Not findPeaks" <<std::endl;
    return false; 
  }
   return false;
}

/**
 *  Predict peaks and overwrite
 *  specified peaks workspace.
 *
 *  @param peaks_ws_name  Name of the peaks workspace to create
 *
 *  @param min_pred_wl        Minimum wavelength
 *  @param max_pred_wl        Maximum wavelength
 *  @param min_pred_dspacing   Minimum d-space
 *  @param max_pred_dspacing   Maximum d-space
 *
 *  @return true if PredictPeaks completed successfully.
 */
bool MantidEVWorker::predictPeaks( const std::string & peaks_ws_name,
                                         double        min_pred_wl,
                                         double        max_pred_wl,
                                         double        min_pred_dspacing,
                                         double        max_pred_dspacing )
{
  try
  {
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("PredictPeaks");
    alg->setProperty("InputWorkspace",peaks_ws_name);
    alg->setProperty("WavelengthMin", min_pred_wl);
    alg->setProperty("WavelengthMax", max_pred_wl);
    alg->setProperty("MinDSpacing",min_pred_dspacing);
    alg->setProperty("MaxDSpacing",max_pred_dspacing);
    alg->setProperty("ReflectionCondition","Primitive");
    alg->setProperty("OutputWorkspace", peaks_ws_name );

    if ( alg->execute() )
      return true;
  }
  catch( std::exception &e)
  {
    g_log.error()<<"Error:" << e.what() <<std::endl;
    return false;
  }
  catch(...)
  {
    g_log.error()<<"Error: Could Not predictPeaks" <<std::endl;
    return false;
  }
   return false;
}

/**
 *  Load the specified peaks workspace from the specified peaks file.
 *
 *  @param peaks_ws_name   The name of the peaks workspace to load/create.
 *  @param file_name       The name of the peaks file to load.
 *
 *  @return true if LoadIsawPeaks completed successfully.
 */
bool MantidEVWorker::loadIsawPeaks( const std::string & peaks_ws_name,
                                    const std::string & file_name )
{
 
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("LoadIsawPeaks");
  alg->setProperty("Filename",file_name );

  alg->setProperty("OutputWorkspace", peaks_ws_name );

  if ( alg->execute() )
    return true;
  
  return false;
}

/**
 *  Load the specified peaks workspace from the specified NeXus file.
 *
 *  @param peaks_ws_name   The name of the peaks workspace to load/create.
 *  @param file_name       The name of the NeXus file to load.
 *
 *  @return true if LoadNexusPeaks completed successfully.
 */
bool MantidEVWorker::loadNexusPeaks( const std::string & peaks_ws_name,
                                    const std::string & file_name )
{

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("Load");
  alg->setProperty("Filename",file_name );

  alg->setProperty("OutputWorkspace", peaks_ws_name );

  if ( alg->execute() )
    return true;

  return false;
}

/**
 *  Save the specified peaks workspace to the specified peaks file.
 *
 *  @param peaks_ws_name   The name of the peaks workspace to save.
 *  @param file_name       The name of the peaks file to write to.
 *  @param append          Append the peaks from the peaks workspace
 *                         onto the specified peaks file, if append
 *                         is true and the peaks file already exists.
 *
 *  @return true if SaveIsawPeaks completed successfully.
 */
bool MantidEVWorker::saveIsawPeaks( const std::string & peaks_ws_name,
                                    const std::string & file_name,
                                          bool          append )
{  

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("SaveIsawPeaks");
  alg->setProperty("InputWorkspace", peaks_ws_name );
  alg->setProperty("AppendFile", append );
  alg->setProperty("Filename",file_name );
  
  if ( alg->execute() )
    return true;

  return false;
}

/**
 *  Save the specified peaks workspace to the specified peaks file.
 *
 *  @param peaks_ws_name   The name of the peaks workspace to save.
 *  @param file_name       The name of the NeXus file to write to.
 *
 *  @return true if SaveNexusPeaks completed successfully.
 */
bool MantidEVWorker::saveNexusPeaks( const std::string & peaks_ws_name,
                                    const std::string & file_name,
                                    bool          append )
{
  if (append){
    std::string temp_peaks_ws_name = "__MantidEVWorker_peaks_ws";
    IAlgorithm_sptr load = AlgorithmManager::Instance().create("Load");
    load->setProperty("OutputWorkspace", temp_peaks_ws_name );
    load->setProperty("Filename",file_name );

    load->execute();

    IAlgorithm_sptr combine = AlgorithmManager::Instance().create("CombinePeaksWorkspaces");
    combine->setProperty("LHSWorkspace", temp_peaks_ws_name );
    combine->setProperty("RHSWorkspace", peaks_ws_name );
    combine->setProperty("OutputWorkspace", peaks_ws_name );

    combine->execute();
  }
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("SaveNexus");
  alg->setProperty("InputWorkspace", peaks_ws_name );
  alg->setProperty("Filename",file_name );

  if ( alg->execute() )
    return true;

  return false;
}


/**
 *  Find an optimized UB matrix that indexes the peaks in the specified
 *  peaks workspace.
 *
 *  @param peaks_ws_name   The name of the peaks workspace.
 *  @param min_abc         Lower bound for the real space edge lengths.
 *  @param max_abc         Upper bound for the real space edge lengths.
 *  @param tolerance       The tolerance on hkl values to use when 
 *                         determining whether or not a peak is indexed.
 *
 *  @return true if FindUBusingFFT completed successfully.
 */
bool MantidEVWorker::findUBUsingFFT( const std::string & peaks_ws_name,
                                           double              min_abc,
                                           double              max_abc,
                                           double              tolerance )
{
  if ( !isPeaksWorkspace( peaks_ws_name ) )
    return false;

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("FindUBUsingFFT");
  alg->setProperty("PeaksWorkspace",peaks_ws_name);
  alg->setProperty("MinD",min_abc);
  alg->setProperty("MaxD",max_abc);
  alg->setProperty("Tolerance",tolerance);

  if ( alg->execute() )
  { 
    return true;
  }

  return false;
}


/**
 *  Find an optimized UB matrix from the indexed peaks in the specified
 *  peaks workspace.
 * 
 *  @param peaks_ws_name   The name of the peaks workspace.
 *  @param tolerance  The tolerance for peak finding.
 *
 *  @return true if FindUBusingIndexedPeaks completed successfully.
 */
bool MantidEVWorker::findUBUsingIndexedPeaks(const std::string & peaks_ws_name, double tolerance )
{
  if ( !isPeaksWorkspace( peaks_ws_name ) )
    return false;

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("FindUBUsingIndexedPeaks");
  alg->setProperty("PeaksWorkspace",peaks_ws_name);
  alg->setProperty("Tolerance",tolerance);

  if ( alg->execute() )
    return true;

  return false;
}


/**
 *  Load a UB matrix from the specified ISAW peaks file into the specified
 *  peaks workspace.
 * 
 *  @param peaks_ws_name   The name of the peaks workspace.
 *  @param file_name       The name of the ISAW peaks file to load.
 *
 *  @return true if LoadIsawUB completed successfully.
 */
bool MantidEVWorker::loadIsawUB( const std::string & peaks_ws_name,
                                 const std::string & file_name)
{
  if ( !isPeaksWorkspace( peaks_ws_name ) )
    return false;

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("LoadIsawUB");
  alg->setProperty("InputWorkspace",peaks_ws_name);
  alg->setProperty("Filename",file_name);
  alg->setProperty("CheckUMatrix",true);

  if ( alg->execute() )
    return true;

  return false;
}


/**
 *  Save the UB matrix from the specified peaks workspace into the specified
 *  ISAW matrix file. 
 * 
 *  @param peaks_ws_name   The name of the peaks workspace.
 *  @param file_name       The name of the ISAW matrix file to write. 
 *
 *  @return true if SaveIsawUB completed successfully.
 */
bool MantidEVWorker::saveIsawUB( const std::string & peaks_ws_name,
                                 const std::string & file_name)
{
  if ( !isPeaksWorkspace( peaks_ws_name ) )
    return false;

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("SaveIsawUB");
  alg->setProperty("InputWorkspace",peaks_ws_name);
  alg->setProperty("Filename",file_name);

  if ( alg->execute() )
    return true;

  return false;
}


/**
 *  Adjust the goniometer angles in the specified peaks workspace to
 *  maximize the number of peaks that are indexed with the current UB
 *  matrix.
 *
 *  @param peaks_ws_name  The name of the peaks workspace.
 *  @param max_change     The maximum change allowed for any
 *                        goniometer angle, in degrees.
 *
 *  @return true if the OptimizeCrystalPlacement algorithm completes
 *          successfully.
 */
bool MantidEVWorker::optimizePhiChiOmega( const std::string & peaks_ws_name, 
                                                double        max_change )
{
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("OptimizeCrystalPlacement");
  alg->setProperty("PeaksWorkspace",peaks_ws_name);
  alg->setProperty("KeepGoniometerFixedfor","");
  alg->setProperty("ModifiedPeaksWorkspace",peaks_ws_name);
  std::string info_table = "_info";
  info_table = peaks_ws_name + info_table;
  alg->setProperty("FitInfoTable",info_table);
  alg->setProperty("AdjustSampleOffsets",false);
  alg->setProperty("OptimizeGoniometerTilt",false);
  alg->setProperty("MaxAngularChange",max_change);
  alg->setProperty("MaxIndexingError",0.20);
  alg->setProperty("MaxHKLPeaks2Use",-1.0);
  alg->setProperty("MaxSamplePositionChange_meters",0.05);
  if ( alg->execute() )
    return true;

  return false;
}


/**
 *  Actually index the peaks in the specified peaks workspace using the
 *  current UB matrix in the workspace.
 *
 *  @param peaks_ws_name  The name of the peaks workspace.
 *  @param tolerance      The tolerance on hkl to use while indexing.
 *  @param round_hkls     If true, the computed hkl values will be 
 *                        rounded to the nearest integer value.
 *
 *  @return true if the IndexPeaks algorithm completes successfully.
 */
bool MantidEVWorker::indexPeaksWithUB( const std::string & peaks_ws_name, 
                                             double        tolerance, 
                                             bool          round_hkls )
{
  if ( !isPeaksWorkspace( peaks_ws_name ) )
    return false;

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("IndexPeaks");
  alg->setProperty("PeaksWorkspace",peaks_ws_name);
  alg->setProperty("Tolerance",tolerance);
  alg->setProperty("RoundHKLs",round_hkls);

  if ( alg->execute() )
    return true;

  return false;
}


/**
 *  Display the possible conventional cells corresponding to the current
 *  UB.  NOTE: This only makes sense if the current UB matrix corresponds
 *  to the Niggli reduced cell.
 *
 *  @param peaks_ws_name     The name of the peaks workspace.
 *  @param max_scalar_error  The maximum error in the cell scalars that
 *                           is allowed for a possible cell to be listed.
 *  @param best_only         If true, only the best fitting cell of any
 *                           particular type will be displayed.
 *  @param allow_perm        If true, permutations are used to find the
 *                           best fitting cell of any
 *                           particular type.
 *
 *  @return true if the ShowPossibleCells algorithm completes successfully.
 */
bool MantidEVWorker::showCells( const std::string & peaks_ws_name,
                                      double        max_scalar_error,
                                      bool          best_only,
                                      bool          allow_perm)
{
  if ( !isPeaksWorkspace( peaks_ws_name ) )
    return false;

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("ShowPossibleCells");
  alg->setProperty("PeaksWorkspace",peaks_ws_name);
  alg->setProperty("MaxScalarError",max_scalar_error);
  alg->setProperty("BestOnly",best_only);
  alg->setProperty("AllowPermutations",allow_perm);

  if ( alg->execute() )
    return true;

  return false;
}


/**
 *  Change the UB matrix and indexing from the current Niggli reduced
 *  cell to the specified cell type and centering.
 *  NOTE: This only makes sense if the current UB matrix corresponds
 *  to the Niggli reduced cell.
 *
 *  @param peaks_ws_name     The name of the peaks workspace.
 *  @param cell_type         String with the cell type, such as "Cubic".
 *  @param centering         String with the centering such as "F".
 *
 *  @return true if the SelectCellOfType algorithm completes successfully.
 */
bool MantidEVWorker::selectCellOfType( const std::string & peaks_ws_name,
                                       const std::string & cell_type,
                                       const std::string & centering )
{
  if ( !isPeaksWorkspace( peaks_ws_name ) )
    return false;

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("SelectCellOfType");
  alg->setProperty("PeaksWorkspace",peaks_ws_name);
  alg->setProperty("CellType",cell_type);
  alg->setProperty("Centering",centering);
  alg->setProperty("Apply",true);
  alg->setProperty("tolerance",0.12);

  if ( alg->execute() )
    return true;

  return false;
}


/**
 *  Change the UB matrix and indexing from the current Niggli reduced
 *  cell to the cell with the specified form number.
 *  NOTE: This only makes sense if the current UB matrix corresponds
 *  to the Niggli reduced cell.        
 *
 *  @param peaks_ws_name     The name of the peaks workspace.
 *  @param form_num          The form number, 1..44.
 *
 *  @return true if the SelectCellWithForm algorithm completes successfully.
 */
bool MantidEVWorker::selectCellWithForm(  const std::string & peaks_ws_name,
                                                size_t        form_num )
{
  if ( !isPeaksWorkspace( peaks_ws_name ) )
    return false;

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("SelectCellWithForm");
  alg->setProperty("PeaksWorkspace",peaks_ws_name);
  alg->setProperty("FormNumber",(int)form_num);
  alg->setProperty("Apply",true);
  alg->setProperty("tolerance",0.12);
  
  if ( alg->execute() )
    return true;

  return false;
}


/**
 *  Change the UB matrix and indexing using the specified tranformation
 *  that maps the current hkl vectors to the desired hkl values.
 *
 *  @param peaks_ws_name     The name of the peaks workspace.
 *  @param row_1_str         String with the three entries from the
 *                           first row of the matrix.
 *  @param row_2_str         String with the three entries from the
 *                           second row of the matrix.
 *  @param row_3_str         String with the three entries from the
 *                           third row of the matrix.
 *
 *  @return true if the TransformHKL algorithm completes successfully.
 */
bool MantidEVWorker::changeHKL(  const std::string & peaks_ws_name,
                                 const std::string & row_1_str,
                                 const std::string & row_2_str,
                                 const std::string & row_3_str )
{
  if ( !isPeaksWorkspace( peaks_ws_name ) )
    return false;

  std::string transf_string = row_1_str + "," + row_2_str + "," + row_3_str;

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("TransformHKL");
  alg->setProperty("PeaksWorkspace",peaks_ws_name);
  alg->setProperty("HKLTransform",transf_string);

  if ( alg->execute() )
    return true;

  return false;
}


/**
 *  Integrate the peaks from the specified peaks workspace by generating
 *  raw unweighted events in reciprocal space from the specified events
 *  workspace and applying the IntegratePeaksMD algorithm to the raw
 *  event MD workspace.
 *
 *  @param peaks_ws_name   The name of the peaks workspace with the peaks
 *                         to integrate.
 *  @param event_ws_name   The name of the event workspace to use when
 *                         generating the temporary raw MD event workspace.
 *  @param peak_radius     The radius of the peak region.
 *  @param inner_radius    The radius of the inner surface of the background
 *                         region.
 *  @param outer_radius    The radius of the outer surface of the background
 *                         region.
 *  @param integrate_edge  If true, integrate peaks for which the sphere
 *                         goes off the edge of the detector.
 *  @param use_cylinder_integration   Set true to use cylinder integration.
 *  @param cylinder_length            The length of the cylinder to integrate.
 *  @param cylinder_percent_bkg       The percentage of the cylinder length
 *                                    that is background.
 *  @param cylinder_profile_fit       The fitting function for cylinder
 *                                    integration.
 *
 *  @return true if the unweighted workspace was successfully created and
 *          integrated using IntegratePeaksMD.
 */
bool MantidEVWorker::sphereIntegrate(  const std::string & peaks_ws_name,
                                       const std::string & event_ws_name,
                                             double        peak_radius,
                                             double        inner_radius,
                                             double        outer_radius,
                                             bool          integrate_edge,
                                             bool          use_cylinder_integration,
                                             double        cylinder_length,
                                             double        cylinder_percent_bkg,
                                       const std::string & cylinder_profile_fit)
{
  try
  {
    if ( !isPeaksWorkspace( peaks_ws_name ) )
      return false;

    if ( !isEventWorkspace( event_ws_name ) )
      return false;


    std::string temp_MD_ws_name = "__MantidEVWorker_sphere_integrate_temp_MD_ws";
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("ConvertToMD");
    alg->setProperty("InputWorkspace",event_ws_name);
    alg->setProperty("OutputWorkspace",temp_MD_ws_name);
    alg->setProperty("OverwriteExisting",true);
    alg->setProperty("QDimensions","Q3D");
    alg->setProperty("dEAnalysisMode","Elastic");
    alg->setProperty("QConversionScales","Q in A^-1");
    alg->setProperty("Q3DFrames","Q_sample");
    alg->setProperty("UpdateMasks",false);
    alg->setProperty("LorentzCorrection",false);
    alg->setProperty("MinValues","-30,-30,-30");
    alg->setProperty("MaxValues","30,30,30");
    alg->setProperty("SplitInto","2,2,2");
    alg->setProperty("SplitThreshold",200);
    alg->setProperty("MaxRecursionDepth",10);
    alg->setProperty("MinRecursionDepth",7);
    std::cout << "Making temporary MD workspace" << std::endl; 
    if ( !alg->execute() )
      return false;
    std::cout << "Made temporary MD workspace...OK" << std::endl; 

    alg = AlgorithmManager::Instance().create("IntegratePeaksMD");
    alg->setProperty("InputWorkspace", temp_MD_ws_name);
    alg->setProperty("PeakRadius",peak_radius);
    alg->setProperty("BackgroundInnerRadius",inner_radius);
    alg->setProperty("BackgroundOuterRadius",outer_radius);
    alg->setProperty("PeaksWorkspace",peaks_ws_name);
    alg->setProperty("OutputWorkspace",peaks_ws_name);
    alg->setProperty("ReplaceIntensity",true);
    alg->setProperty("IntegrateIfOnEdge",integrate_edge); 
    alg->setProperty("Cylinder",use_cylinder_integration);
    alg->setProperty("CylinderLength",cylinder_length);
    alg->setProperty("PercentBackground",cylinder_percent_bkg);
    alg->setProperty("ProfileFunction",cylinder_profile_fit);

    std::cout << "Integrating temporary MD workspace" << std::endl; 

    bool integrate_OK = alg->execute();
    auto& ADS = AnalysisDataService::Instance();
    std::cout << "Removing temporary MD workspace" << std::endl; 
    ADS.remove( temp_MD_ws_name );

    if ( integrate_OK )
    {
      std::cout << "Integrated temporary MD workspace...OK" << std::endl; 
      return true;
    }

    std::cout << "Integrated temporary MD workspace FAILED" << std::endl; 
    return false;
  }
  catch( std::exception &e)
  {
    g_log.error()<<"Error:" << e.what() <<std::endl;
    return false;
  }
  catch(...)
  {
    g_log.error()<<"Error: Could Not Integrated temporary MD workspace" <<std::endl;
    return false; 
  }
}


/**
 *  Integrate the peaks from the specified peaks workspace by applying 
 *  the PeakIntegration algorithm to the event workspace.
 *
 *  @param peaks_ws_name       The name of the peaks workspace with the
 *                             peaks to integrate.
 *  @param event_ws_name       The name of the event workspace to use.
 *  @param rebin_param_str     String listing the rebinning parameters 
 *                             to use when forming the event workspace
 *                             into a histogram workspace.
 *  @param n_bad_edge_pix      The number of pixels to omit at the edge
 *                             of all detectors.
 *  @param use_ikeda_carpenter If true, the integrated intensities on
 *                             the time-of-flight slices will be fit
 *                             using the Ikeda-Carpenter function to
 *                             obtain the final integrated intensities.
 *
 *  @return true if the PeakIntegration algorithm completed successfully.
 */
bool MantidEVWorker::fitIntegrate(  const std::string & peaks_ws_name,
                                    const std::string & event_ws_name,
                                    const std::string & rebin_param_str,
                                          size_t        n_bad_edge_pix,
                                          bool          use_ikeda_carpenter )
{
  try
  {
    if ( !isPeaksWorkspace( peaks_ws_name ) )
      return false;

    if ( !isEventWorkspace( event_ws_name ) )
      return false;

    std::string temp_FIT_ws_name = "__MantidEVWorker_FIT_integration_temp_event_ws";
    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("Rebin");
    alg->setProperty("InputWorkspace",event_ws_name);
    alg->setProperty("OutputWorkspace",temp_FIT_ws_name); 
    alg->setProperty("Params",rebin_param_str);
    alg->setProperty("PreserveEvents",true);

    std::cout << "Rebinning event workspace" << std::endl;
    if ( !alg->execute() )
      return false;

    alg = AlgorithmManager::Instance().create("PeakIntegration");
    alg->setProperty("InPeaksWorkspace", peaks_ws_name);
    alg->setProperty("InputWorkspace", temp_FIT_ws_name);
    alg->setProperty("OutPeaksWorkspace", peaks_ws_name);
    alg->setProperty("IkedaCarpenterTOF",use_ikeda_carpenter);
    alg->setProperty("MatchingRunNo",true);
    alg->setProperty("NBadEdgePixels",(int)n_bad_edge_pix);

    std::cout << "Integrating temporary Rebinned workspace" << std::endl;

    bool integrate_OK = alg->execute();
    auto& ADS = AnalysisDataService::Instance();
    std::cout << "Removing temporary Rebinned workspace" << std::endl;
    ADS.remove( temp_FIT_ws_name );

    if ( integrate_OK )
    {
      std::cout << "Integrated temporary FIT workspace...OK" << std::endl;
      return true;
    }

    std::cout << "Integrated temporary FIT workspace FAILED" << std::endl;
  }
  catch( std::exception &e)
  {
    g_log.error()<<"Error:" << e.what() <<std::endl;
    return false;
  }
  catch(...)
  {
    g_log.error()<<"Error: Could Not Integrated temporary FIT workspace" <<std::endl;
    return false; 
  }
  return false;
}


/**
 *  Integrate the peaks from the specified peaks workspace by applying 
 *  the IntegrateEllipsoids algorithm to the event workspace.
 *
 *  @param peaks_ws_name   The name of the peaks workspace with the peaks
 *                         to integrate.
 *  @param event_ws_name   The name of the event workspace to use.
 *  @param region_radius   The radius of the whole spherical region 
 *                         enclosing the peak and background ellipsoids.
 *  @param specify_size    If true the sizes of the peak and background
 *                         regions are specified by the last three 
 *                         parameters to this method.
 *  @param peak_size       The size of the major axis of the peak ellipsoid.
 *  @param inner_size      The size of the major axis of the inner surface 
 *                         of the background ellipsoid.
 *  @param outer_size      The size of the major axis of the outer surface 
 *                         of the background ellipsoid.
 *
 *  @return true if the IntegrateEllipsoids algorithm completed successfully.
 */
bool MantidEVWorker::ellipsoidIntegrate( const std::string & peaks_ws_name,
                                         const std::string & event_ws_name,
                                         double        region_radius,
                                         bool          specify_size,
                                         double        peak_size,
                                         double        inner_size,
                                         double        outer_size )
{ 
  try
  {
    if ( !isPeaksWorkspace( peaks_ws_name ) )
      return false;

    if ( !isEventWorkspace( event_ws_name ) )
      return false;

    IAlgorithm_sptr alg = AlgorithmManager::Instance().create("IntegrateEllipsoids");
    alg->setProperty("InputWorkspace", event_ws_name);
    alg->setProperty("PeaksWorkspace", peaks_ws_name);
    alg->setProperty("RegionRadius",region_radius);
    alg->setProperty("SpecifySize",specify_size);
    alg->setProperty("PeakSize",peak_size);
    alg->setProperty("BackgroundInnerSize",inner_size);
    alg->setProperty("BackgroundOuterSize",outer_size);
    alg->setProperty("OutputWorkspace",peaks_ws_name);

    std::cout << "Running IntegrateEllipsoids" << std::endl;

    if ( alg->execute() )
    {
      std::cout << "IntegrateEllipsoids Executed OK" << std::endl;
      return true;
    }

    std::cout << "IntegrateEllipsoids FAILED" << std::endl;
  }
  catch( std::exception &e)
  {
    g_log.error()<<"Error:" << e.what() <<std::endl;
    return false;
  }
  catch(...)
  {
    g_log.error()<<"Error: Could Not IntegratedEllipsoids" <<std::endl;
    return false; 
  }
  return false;
}


/**
 *  Show the current UB matrix from the specified peaks workspace
 *  in both the Mantid and ISAW forms.
 *
 *  @param peaks_ws_name  The name of the peaks workspace with the UB
 *                        matrix.
 *
 *  @return true if the peaks workspace had a UB matrix to show.
 */
bool MantidEVWorker::showUB( const std::string & peaks_ws_name )
{
  if ( !isPeaksWorkspace( peaks_ws_name ) )
  {
    return false;
  }   

  const auto& ADS = AnalysisDataService::Instance();
  IPeaksWorkspace_sptr peaks_ws = ADS.retrieveWS<IPeaksWorkspace>(peaks_ws_name);

  try
  {
    char logInfo[200];

    Mantid::Geometry::OrientedLattice o_lattice = peaks_ws->mutableSample().getOrientedLattice();
    Matrix<double> UB = o_lattice.getUB();

    g_log.notice() << std::endl;
    g_log.notice() << "Mantid UB = " << std::endl;
    sprintf( logInfo,
             std::string(" %12.8f %12.8f %12.8f\n %12.8f %12.8f %12.8f\n %12.8f %12.8f %12.8f\n").c_str(),
             UB[0][0], UB[0][1], UB[0][2],
             UB[1][0], UB[1][1], UB[1][2],
             UB[2][0], UB[2][1], UB[2][2] );
    g_log.notice( std::string(logInfo) );

    g_log.notice() << "ISAW UB = " << std::endl;
    sprintf( logInfo,
             std::string(" %12.8f %12.8f %12.8f\n %12.8f %12.8f %12.8f\n %12.8f %12.8f %12.8f\n").c_str(),
             UB[2][0], UB[0][0], UB[1][0],
             UB[2][1], UB[0][1], UB[1][1],
             UB[2][2], UB[0][2], UB[1][2] );
    g_log.notice( std::string(logInfo) );
    
    double calc_a = o_lattice.a();
    double calc_b = o_lattice.b();
    double calc_c = o_lattice.c();
    double calc_alpha = o_lattice.alpha();
    double calc_beta  = o_lattice.beta();
    double calc_gamma = o_lattice.gamma();
                                       // Show the modified lattice parameters
    sprintf( logInfo,
             std::string("Lattice Parameters: %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f").c_str(),
             calc_a, calc_b, calc_c, calc_alpha, calc_beta, calc_gamma);

    g_log.notice( std::string(logInfo) );

    sprintf( logInfo,
             std::string("%19s %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f").c_str(),
             "Lattice Errors    :",
             o_lattice.errora(),o_lattice.errorb(),o_lattice.errorc(),
             o_lattice.erroralpha(),o_lattice.errorbeta(),o_lattice.errorgamma());

    g_log.notice( std::string( logInfo));
  }
  catch(...)
  {
    return false;
  }

  return true;
}


/**
 *  Get the current UB matrix from the specified peaks workspace.
 *
 *  @param peaks_ws_name  The name of the peaks workspace with the UB
 *                        matrix.
 *  @param  lab_coords    If true, multiply the goniometer matrix
 *                        times UB before returning it, so that
 *                        the UB is expressed in lab-coordinates,
 *                        otherwise, return the UB as it is stored
 *                        in the sample.
 *  @param UB             3x3 matrix of doubles to be filled out with
 *                        the UB matrix if one exists in the specified
 *                        peaks workspace.
 *  @return true if the UB matrix was found and returned in the UB
 *               parameter.
 */
bool MantidEVWorker::getUB( const std::string & peaks_ws_name,
                                  bool          lab_coords,
                                  Mantid::Kernel::Matrix<double> & UB )
{
  if ( !isPeaksWorkspace( peaks_ws_name ) )
  {
    return false;
  }

  const auto& ADS = AnalysisDataService::Instance();
  IPeaksWorkspace_sptr peaks_ws = ADS.retrieveWS<IPeaksWorkspace>(peaks_ws_name);

  try
  {
    Mantid::Geometry::OrientedLattice o_lattice = peaks_ws->mutableSample().getOrientedLattice();
    UB = o_lattice.getUB();

    if ( lab_coords )    // Try to get goniometer matrix from first peak 
    {                    // and adjust UB for goniometer rotation
      const IPeak & peak = peaks_ws->getPeak(0);
      auto goniometer_matrix = peak.getGoniometerMatrix();
      UB = goniometer_matrix * UB;
    }
  }
  catch(...)
  {
    return false;
  }

  return true;
}


/**
 *  Copy the the current oriented lattice with the UB matrix from the 
 *  specified peaks workspace to the specified MD workspace.
 *
 *  @param peaks_ws_name  The name of the peaks workspace to copy the
 *                        lattice from.
 *  @param md_ws_name     The name of the md workspace to copy the
 *                        lattice to.
 *  @param event_ws_name  The name of the event workspace to copy the
 *                        lattice to.
 *  @return true if the copy was done, false if something went wrong.
 */
bool MantidEVWorker::copyLattice( const std::string & peaks_ws_name,
                                  const std::string & md_ws_name,
                                  const std::string & event_ws_name)
                           
{
  // fail if peaks workspace is not there
  if ( !isPeaksWorkspace(peaks_ws_name) )
  {
    return false;
  }

  // must have either md or event workspace
  if ((md_ws_name.empty()) && (event_ws_name.empty()))
  {
      return false;
  }

  // copy onto md workspace
  if (!md_ws_name.empty())
  {
    if (!isMDWorkspace(md_ws_name))
    {
      return false;
    }

    try
    {
      IAlgorithm_sptr alg = AlgorithmManager::Instance().create("CopySample");
      alg->setProperty("InputWorkspace",peaks_ws_name);
      alg->setProperty("OutputWorkspace",md_ws_name);
      alg->setProperty("CopyName",       false);
      alg->setProperty("CopyMaterial",   false);
      alg->setProperty("CopyEnvironment",false);
      alg->setProperty("CopyShape",      false);
      alg->setProperty("CopyLattice",    true);
      alg->execute();
    }
    catch(...)
    {
      g_log.notice() << "\n";
      g_log.notice() << "CopySample from " << peaks_ws_name <<
                        " to " << md_ws_name << " FAILED\n\n";
      return false;
    }
  }

  // copy onto
  if (!event_ws_name.empty())
  {
    if (!isEventWorkspace(event_ws_name))
    {
      return false;
    }

    try
    {
      IAlgorithm_sptr alg = AlgorithmManager::Instance().create("CopySample");
      alg->setProperty("InputWorkspace",peaks_ws_name);
      alg->setProperty("OutputWorkspace",event_ws_name);
      alg->setProperty("CopyName",       false);
      alg->setProperty("CopyMaterial",   false);
      alg->setProperty("CopyEnvironment",false);
      alg->setProperty("CopyShape",      false);
      alg->setProperty("CopyLattice",    true);
      alg->execute();
    }
    catch(...)
    {
      g_log.notice() << "\n";
      g_log.notice() << "CopySample from " << peaks_ws_name <<
                        " to " << event_ws_name << " FAILED\n\n";
      return false;
    }
  }

  return true;

}


/**
 * Get information about a specified Q-position from the specified peaks
 * workspace.
 *
 * @param  peaks_ws_name   The name of the peaks workspace
 * @param  lab_coords      This will be true if the Q-vector is
 *                         in lab coordinates and will be false if
 *                         it is in sample coordinates.
 * @param  Q               The Q-vector.
 */
std::vector< std::pair<std::string,std::string> > MantidEVWorker::PointInfo( const std::string & peaks_ws_name,
                                                                             bool lab_coords,
                                                                             Mantid::Kernel::V3D Q)
{
  IPeaksWorkspace_sptr peaks_ws = AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>(peaks_ws_name);
  return peaks_ws->peakInfo( Q , lab_coords);
}

} // namespace CustomInterfaces
} // namespace MantidQt
