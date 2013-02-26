#include <iostream>

#include "MantidQtCustomInterfaces/MantidEVWorker.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"

namespace MantidQt
{
namespace CustomInterfaces
{

using namespace Mantid::Kernel;
using namespace Mantid::API;


MantidEVWorker::MantidEVWorker()
{
  std::cout << "Constructed MantidEVWorker" << std::endl;
}


MantidEVWorker::~MantidEVWorker()
{
  std::cout << "Destroyed MantidEVWorker" << std::endl;
}


std::string MantidEVWorker::workspaceType( const std::string & ws_name )
{
  const auto& ADS = AnalysisDataService::Instance();

  if ( !ADS.doesExist( ws_name ) )
    return std::string("");

  Workspace_const_sptr outWS = ADS.retrieveWS<Workspace>(ws_name);

  return outWS->id();
}


// check for existence of specified MDWorkspace and record the name
// if it exists
bool MantidEVWorker::selectMDWorkspace( const std::string & md_ws_name )
{
  std::cout << "worker->selectMDWorkspace called" << md_ws_name << std::endl;

  std::string ws_type = workspaceType(md_ws_name);
  std::cout << "Workspace Type = " << ws_type << std::endl;

  if ( ws_type.length() == 0 )
    return false;

  if ( ws_type == "MDEventWorkspace<MDEvent,3>" || ws_type == "MDHistorWorkspace" ) 
  {
    this->md_ws_name = md_ws_name;
  }
  else
    return false;

  return true;
}


// check for existence of specified PeaksWorkspace and record the name
// if it exists 
bool MantidEVWorker::selectPeaksWorkspace( const std::string & peaks_ws_name )
{
  std::cout << "worker->selectPeaksWorkspace called" 
            << peaks_ws_name << std::endl;

  std::string ws_type = workspaceType(peaks_ws_name);
  std::cout << "Workspace Type = " << ws_type << std::endl;

  if ( ws_type.length() == 0 )
    return false;

  if ( ws_type == "PeaksWorkspace" )
  {
    this->peaks_ws_name = peaks_ws_name;
  }
  else
    return false;

  return true;
}


// check for existence of specified EventWorkspace
bool MantidEVWorker::isEventWorkspace( const std::string & event_ws_name )
{
  std::string ws_type = workspaceType(event_ws_name);
  std::cout << "Workspace Type = " << ws_type << std::endl;

  if ( ws_type.length() == 0 )
    return false;

  if ( ws_type != "EventWorkspace" )
  {
    return false;
  }

  return true;
}


bool MantidEVWorker::findPeaks( const std::string & peaks_ws_name,
                                      double        max_abc,
                                      size_t        num_to_find,
                                      double        min_intensity )
{
  std::cout << "worker->findPeaks called" << peaks_ws_name << std::endl;
  std::cout << "max_abc       = " << max_abc << std::endl;
  std::cout << "num_to_find   = " << num_to_find << std::endl;
  std::cout << "min_intensity = " << min_intensity << std::endl;

//  const auto& ADS = AnalysisDataService::Instance();
//  if ( ! ( ADS.isValid( peaks_ws_name ) ) )
//    return false;

  double min_separation = 0.9 * 6.28 / max_abc;
  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("FindPeaksMD");
  alg->setProperty("InputWorkspace",md_ws_name);
  alg->setProperty("PeakDistanceThreshold", min_separation);
  alg->setProperty("MaxPeaks",(int64_t)num_to_find);
  alg->setProperty("DensityThresholdFactor",min_intensity);
  alg->setProperty("OutputWorkspace", peaks_ws_name );

  if ( alg->execute() )
    return true;

  return false;
}


bool MantidEVWorker::findUBUsingFFT( const std::string & peaks_ws_name,
                                           double              min_abc,
                                           double              max_abc,
                                           double              tolerance )
{
  std::cout << "worker->findUBUsingFFT called" << std::endl;
  std::cout << "peaks_ws_name = " << peaks_ws_name << std::endl;
  std::cout << "min_abc       = " << min_abc << std::endl;
  std::cout << "max_abc       = " << max_abc << std::endl;
  std::cout << "tolerance     = " << tolerance << std::endl;

  if ( !selectPeaksWorkspace( peaks_ws_name ) )
    return false;

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("FindUBUsingFFT");
  alg->setProperty("PeaksWorkspace",peaks_ws_name);
  alg->setProperty("MinD",min_abc);
  alg->setProperty("MaxD",max_abc);
  alg->setProperty("Tolerance",tolerance);

  if ( alg->execute() )
    return true;

  return false;
}


bool MantidEVWorker::findUBUsingIndexedPeaks(const std::string & peaks_ws_name)
{
  std::cout << "worker->findUBUsingIndexedPeaks called" << std::endl;
  std::cout << "peaks_ws_name = " << peaks_ws_name << std::endl;

  if ( !selectPeaksWorkspace( peaks_ws_name ) )
    return false;

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("FindUBUsingIndexedPeaks");
  alg->setProperty("PeaksWorkspace",peaks_ws_name);

  if ( alg->execute() )
    return true;

  return false;
}


bool MantidEVWorker::loadIsawUB( const std::string & peaks_ws_name,
                                 const std::string & file_name)
{
  std::cout << "worker->loadIsawUB called" << std::endl;
  std::cout << "peaks_ws_name = " << peaks_ws_name << std::endl;
  std::cout << "file_name     = " << file_name << std::endl;

  if ( !selectPeaksWorkspace( peaks_ws_name ) )
    return false;

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("LoadIsawUB");
  alg->setProperty("InputWorkspace",peaks_ws_name);
  alg->setProperty("Filename",file_name);
  alg->setProperty("CheckUMatrix",true);

  if ( alg->execute() )
    return true;

  return false;
}


bool MantidEVWorker::optimizePhiChiOmega( const std::string & peaks_ws_name, 
                                                double        max_change )
{
  std::cout << "worker->optimizePhiChiOmega called" << std::endl;
  std::cout << "peaks_ws_name = " << peaks_ws_name << std::endl;
  std::cout << "max_change    = " << max_change << std::endl;

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("OptimizeCrystalPlacement");
  alg->setProperty("PeaksWorkspace",peaks_ws_name);
  alg->setProperty("ModifiedPeaksWorkspace",peaks_ws_name);
  std::string info_table = "_info";
  info_table = peaks_ws_name + info_table;
  alg->setProperty("FitInfoTable",info_table);
  alg->setProperty("ToleranceChiPhiOmega",max_change);
  alg->setProperty("MaxIntHKLOffsetPeaks2Use",0.12);
  alg->setProperty("MaxHKLPeaks2Use",-1.0);

  if ( alg->execute() )
    return true;

  return false;
}


bool MantidEVWorker::indexPeaksWithUB( const std::string & peaks_ws_name, 
                                             double        tolerance, 
                                             bool          round_hkls )
{
  std::cout << "worker->indexPeaksWithUB called" << std::endl;
  std::cout << "peaks_ws_name = " << peaks_ws_name << std::endl;
  std::cout << "tolerance     = " << tolerance     << std::endl;
  std::cout << "round_hkls    = " << round_hkls    << std::endl;

  if ( !selectPeaksWorkspace( peaks_ws_name ) )
    return false;

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("IndexPeaks");
  alg->setProperty("PeaksWorkspace",peaks_ws_name);
  alg->setProperty("Tolerance",tolerance);
  alg->setProperty("RoundHKLs",round_hkls);

  if ( alg->execute() )
    return true;

  return false;
}


bool MantidEVWorker::showCells( const std::string & peaks_ws_name,
                                      double        max_scalar_error,
                                      bool          best_only )
{
  std::cout << "worker->showCells called" << std::endl;
  std::cout << "peaks_ws_name    = " << peaks_ws_name    << std::endl;
  std::cout << "max_scalar_error = " << max_scalar_error << std::endl;
  std::cout << "best_only        = " << best_only        << std::endl;

  if ( !selectPeaksWorkspace( peaks_ws_name ) )
    return false;

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("ShowPossibleCells");
  alg->setProperty("PeaksWorkspace",peaks_ws_name);
  alg->setProperty("MaxScalarError",max_scalar_error);
  alg->setProperty("BestOnly",best_only);

  if ( alg->execute() )
    return true;

  return false;
}


bool MantidEVWorker::selectCellOfType( const std::string & peaks_ws_name,
                                       const std::string & cell_type,
                                       const std::string & centering )
{
  std::cout << "worker->selectCellOfType called" << std::endl;
  std::cout << "peaks_ws_name = " << peaks_ws_name    << std::endl;
  std::cout << "cell_type     = " << cell_type << std::endl;
  std::cout << "centering     = " << centering << std::endl;

  if ( !selectPeaksWorkspace( peaks_ws_name ) )
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


bool MantidEVWorker::selectCellWithForm(  const std::string & peaks_ws_name,
                                                size_t        form_num )
{
  std::cout << "worker->selectCellWithForm called" << std::endl;
  std::cout << "peaks_ws_name = " << peaks_ws_name << std::endl;
  std::cout << "form_num      = " << form_num      << std::endl;

  if ( !selectPeaksWorkspace( peaks_ws_name ) )
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


bool MantidEVWorker::changeHKL(  const std::string & peaks_ws_name,
                                 const std::string & row_1_str,
                                 const std::string & row_2_str,
                                 const std::string & row_3_str )
{
  std::cout << "worker->changeHKL called" << std::endl;
  std::cout << "peaks_ws_name = " << peaks_ws_name << std::endl;
  std::cout << "row_1         = " << row_1_str     << std::endl;
  std::cout << "row_2         = " << row_2_str     << std::endl;
  std::cout << "row_3         = " << row_3_str     << std::endl;

  if ( !selectPeaksWorkspace( peaks_ws_name ) )
    return false;

  std::string transf_string = row_1_str + "," + row_2_str + "," + row_3_str;

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("TransformHKL");
  alg->setProperty("PeaksWorkspace",peaks_ws_name);
  alg->setProperty("HKL_Transform",transf_string);

  if ( alg->execute() )
    return true;

  return false;
}


bool MantidEVWorker::sphereIntegrate(  const std::string & peaks_ws_name,
                                       const std::string & event_ws_name,
                                             double        peak_radius,
                                             double        inner_radius,
                                             double        outer_radius,
                                             bool          integrate_edge )
{
  std::cout << "worker->sphereIntegrate called" << std::endl;
  std::cout << "Peaks Workspace         = " << peaks_ws_name << std::endl;
  std::cout << "Event Workspace         = " << event_ws_name << std::endl;
  std::cout << "Peak Radius             = " << peak_radius << std::endl;
  std::cout << "Background Inner Radius = " << inner_radius << std::endl;
  std::cout << "Background Outer Radius = " << outer_radius << std::endl;
  std::cout << "Integrate if On Edge    = " << integrate_edge << std::endl;

  if ( !selectPeaksWorkspace( peaks_ws_name ) )
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
  alg->setProperty("UpdateMasks",false);
  alg->setProperty("LorentzCorrection",false);
  alg->setProperty("MinValues","-30,-30,-30");
  alg->setProperty("MaxValues","30,30,30");
  alg->setProperty("SplitInto","2,2,2");
  alg->setProperty("SplitThreshold",200);
  alg->setProperty("MaxRecursionDepth",12);
  alg->setProperty("MinRecursionDepth",1);
  std::cout << "Making temporary MD workspace" << std::endl; 
  if ( !alg->execute() )
    return false;
  std::cout << "Made temporary MD workspace...OK" << std::endl; 

  alg = AlgorithmManager::Instance().create("IntegratePeaksMD");
  alg->setProperty("InputWorkspace", temp_MD_ws_name);
  alg->setProperty("CoordinatesToUse","Q (sample frame)");
  alg->setProperty("PeakRadius",peak_radius);
  alg->setProperty("BackgroundInnerRadius",inner_radius);
  alg->setProperty("BackgroundOuterRadius",outer_radius);
  alg->setProperty("PeaksWorkspace",peaks_ws_name);
  alg->setProperty("OutputWorkspace",peaks_ws_name);
  alg->setProperty("ReplaceIntensity",true);
  alg->setProperty("IntegrateIfOnEdge",integrate_edge);

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


bool MantidEVWorker::fitIntegrate(  const std::string & peaks_ws_name,
                                    const std::string & event_ws_name,
                                    const std::string & rebin_param_str,
                                          size_t        n_bad_edge_pix,
                                          bool          use_ikeda_carpenter )
{
  std::cout << "worker->fitIntegrate called" << std::endl;
  std::cout << "Peaks Workspace            = " << peaks_ws_name << std::endl;
  std::cout << "Event Workspace            = " << event_ws_name << std::endl;
  std::cout << "Rebin Parameters           = " << rebin_param_str << std::endl;
  std::cout << "Number of Badd Edge Pixels = " << n_bad_edge_pix << std::endl;
  std::cout << "Ikeda-Carpenter TOF        = " << use_ikeda_carpenter << std::endl;
  if ( !selectPeaksWorkspace( peaks_ws_name ) )
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
  return false;
}


bool MantidEVWorker::ellipsoidIntegrate( const std::string & peaks_ws_name,
                                         const std::string & event_ws_name,
                                         double        region_radius,
                                         bool          specify_size,
                                         double        peak_size,
                                         double        inner_size,
                                         double        outer_size )
{
  std::cout << "worker->ellipsoidintegrate called" << std::endl;
  std::cout << "Peaks Workspace            = " << peaks_ws_name << std::endl;
  std::cout << "Event Workspace            = " << event_ws_name << std::endl;
  std::cout << "Region Radius              = " << region_radius << std::endl;
  std::cout << "Specify Size               = " << specify_size << std::endl;
  std::cout << "Peak Size                  = " << peak_size << std::endl;
  std::cout << "Background Inner Size      = " << inner_size << std::endl;
  std::cout << "Background Outer Size      = " << outer_size << std::endl;

  if ( !selectPeaksWorkspace( peaks_ws_name ) )
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
  return false;
}

} // namespace CustomInterfaces
} // namespace MantidQt
