#include "MantidAlgorithms/libISISGetEi.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileProperty.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <boost/lexical_cast.hpp>
#include "MantidKernel/Exception.h" 
#include "MantidKernel/VectorHelper.h" 
#include <cmath>
#include <algorithm>
#include <iostream>
#include <sstream>

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(libISISGetEi)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

static const double MON1_TOF_WIN=0.1, MON2_TOF_WIN=0.05;
static const double SFAC_PEAK = 2.0, SFAC_DERIV=1.0;
static const int NPOINTS=8;
static const double BKGD_FAC=0.5;
static const double NO_ESTIMATE=-1e200;

// progress estimates
const double libISISGetEi::CROP = 0.15;
const double libISISGetEi::GET_COUNT_RATE = 0.15;
const double libISISGetEi::FIT_PEAK = 0.2;


void libISISGetEi::init()

{// Declare required input parameters for algorithm and do some validation here
  CompositeValidator<Workspace2D> *val = new CompositeValidator<Workspace2D>;
  val->add(new WorkspaceUnitValidator<Workspace2D>("TOF"));
  val->add(new HistogramValidator<Workspace2D>);
  declareProperty(new WorkspaceProperty<Workspace2D>(
    "InputWorkspace","",Direction::Input,val),
    "The X units of this workspace must be time of flight with times in\n"
    "micro-seconds");
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("Monitor1Spec", -1, mustBePositive,
    "The spectrum number of the output of the first monitor, e.g. MAPS\n"
    "41474, MARI 2, MERLIN 69634");
  declareProperty("Monitor2Spec", -1, mustBePositive->clone(),
    "The spectrum number of the output of the second monitor e.g. MAPS\n"
    "41475, MARI 3, MERLIN 69638");
  BoundedValidator<double> *positiveDouble = new BoundedValidator<double>();
  positiveDouble->setLower(0.0);
  declareProperty("EnergyEstimate", EMPTY_DBL(), positiveDouble,
    "An approximate value for the typical incident energy, energy of\n"
    "neutrons leaving the source (meV)");
  declareProperty("IncidentEnergy", -1.0, Direction::Output);
  declareProperty("FirstMonitorPeak", -1.0, Direction::Output);

  m_fracCompl = 0.0;
}

/** Executes the algorithm
*  @throw out_of_range if the peak runs off the edge of the histogram
*  @throw NotFoundError if one of the requested spectrum numbers was not found in the workspace
*  @throw IndexError if there is a problem converting spectra indexes to spectra numbers, which would imply there is a problem with the workspace
*  @throw invalid_argument if a good peak fit wasn't made or the input workspace does not have common binning
*  @throw runtime_error if there is a problem with the SpectraDetectorMap or a sub-algorithm falls over
*/
void libISISGetEi::exec()
{
  Workspace2D_const_sptr inWS = getProperty("InputWorkspace");
  const int mon1Spec = getProperty("Monitor1Spec");
  const int mon2Spec = getProperty("Monitor2Spec");
  double dist2moni0 = -1, dist2moni1 = -1;
  getGeometry(inWS, mon1Spec, mon2Spec, dist2moni0, dist2moni1);
  g_log.debug() << "Distance between monitors = " << dist2moni0 - dist2moni1 << " m\n";

  // the E_i estimate is used to find (identify) the monitor peaks, checking prior to fitting will throw an exception if this estimate is too big or small
  const double E_est = getProperty("EnergyEstimate");
  double peakLoc0(0.0);
  if( E_est != EMPTY_DBL() )
  {
    peakLoc0 = 1e6*timeToFly(dist2moni0, E_est);
  }

  // get the histograms created by the monitors
  std::vector<int> indexes = getMonitorSpecIndexs(inWS, mon1Spec, mon2Spec);

  g_log.information() << "Looking for a peak in the first monitor spectrum, spectra index " << indexes[0] << std::endl;
  double t_monitor0 = getPeakCentre(inWS, indexes[0], peakLoc0);
  g_log.notice() << "The first peak has been found at TOF = " << t_monitor0 << " microseconds\n";
  setProperty("FirstMonitorPeak", t_monitor0);

  g_log.information() << "Looking for a peak in the second monitor spectrum, spectra index " << indexes[1] << std::endl;
  const double peakLoc1 = t_monitor0 * (dist2moni1/dist2moni0);
  double t_monitor1 = getPeakCentre(inWS, indexes[1], peakLoc1);
  g_log.information() << "The second peak has been found at TOF = " << t_monitor1 << " microseconds\n";

  // assumes that the source and the both mintors lie on one straight line, the 1e-6 converts microseconds to seconds as the mean speed needs to be in m/s
  double meanSpeed = (dist2moni1 - dist2moni0)/(1e-6*(t_monitor1 - t_monitor0));

  // uses 0.5mv^2 to get the kinetic energy in joules which we then convert to meV
  double E_i = neutron_E_At(meanSpeed)/PhysicalConstants::meV;
  g_log.notice() << "The incident energy has been calculated to be " << E_i << " meV";
  if( E_est != EMPTY_DBL() )
  {
    g_log.information () << " (your estimate was " << E_est << " meV)\n";
  }
  else 
  {
    g_log.information () << " (No estimate was supplied).\n"; 
  }
  setProperty("IncidentEnergy", E_i);
}

/** Gets the distances between the source and detectors whose IDs you pass to it
*  @param WS the input workspace
*  @param mon0Spec Spectrum number of the output from the first monitor
*  @param mon1Spec Spectrum number of the output from the second monitor
*  @param monitor0Dist the calculated distance to the detector whose ID was passed to this function first
*  @param monitor1Dist calculated distance to the detector whose ID was passed to this function second
*  @throw NotFoundError if no detector is found for the detector ID given
*  @throw runtime_error if there is a problem with the SpectraDetectorMap
*/
void libISISGetEi::getGeometry(DataObjects::Workspace2D_const_sptr WS, int mon0Spec, int mon1Spec, double &monitor0Dist, double &monitor1Dist) const
{
  const IObjComponent_sptr source = WS->getInstrument()->getSource();

  // retrieve a pointer to the first detector and get its distance
  std::vector<int> dets = WS->spectraMap().getDetectors(mon0Spec);
  if ( dets.size() != 1 )
  {
    g_log.error() << "The detector for spectrum number " << mon0Spec << " was either not found or is a group, grouped monitors are not supported by this algorithm\n";
    g_log.error() << "Error retrieving data for the first monitor" << std::endl;
    throw std::bad_cast();
  }
  IDetector_sptr det = WS->getInstrument()->getDetector(dets[0]);
  monitor0Dist = det->getDistance(*(source.get()));

  // repeat for the second detector
  dets = WS->spectraMap().getDetectors(mon1Spec);
  if ( dets.size() != 1 )
  {
    g_log.error() << "The detector for spectrum number " << mon1Spec << " was either not found or is a group, grouped monitors are not supported by this algorithm\n";
    g_log.error() << "Error retrieving data for the second monitor\n";
    throw std::bad_cast();
  }
  det = WS->getInstrument()->getDetector(dets[0]);
  monitor1Dist = det->getDistance(*(source.get()));
}

/** Converts detector IDs to spectra indexes
*  @param WS the workspace on which the calculations are being performed
*  @param specNum1 spectrum number of the output of the first monitor
*  @param specNum2 spectrum number of the output of the second monitor
*  @return the indexes of the histograms created by the detector whose ID were passed
*  @throw NotFoundError if one of the requested spectrum numbers was not found in the workspace
*/
std::vector<int> libISISGetEi::getMonitorSpecIndexs(DataObjects::Workspace2D_const_sptr WS, int specNum1, int specNum2) const
{// getting spectra numbers from detector IDs is hard because the map works the other way, getting index numbers from spectra numbers has the same problem and we are about to do both
  std::vector<int> specInds;
  
  // get the index number of the histogram for the first monitor
  std::vector<int> specNumTemp(&specNum1, &specNum1+1);
  WorkspaceHelpers::getIndicesFromSpectra(WS, specNumTemp, specInds);
  if ( specInds.size() != 1 )
  {// the monitor spectrum isn't present in the workspace, we can't continue from here
    g_log.error() << "Couldn't find the first monitor spectrum, number " << specNum1 << std::endl;
    throw Exception::NotFoundError("GetEi::getMonitorSpecIndexs()", specNum1);
  }

  // nowe the second monitor
  std::vector<int> specIndexTemp;
  specNumTemp[0] = specNum2;
  WorkspaceHelpers::getIndicesFromSpectra(WS, specNumTemp, specIndexTemp);
  if ( specIndexTemp.size() != 1 )
  {// the monitor spectrum isn't present in the workspace, we can't continue from here
    g_log.error() << "Couldn't find the second monitor spectrum, number " << specNum2 << std::endl;
    throw Exception::NotFoundError("GetEi::getMonitorSpecIndexs()", specNum2);
  }
  
  specInds.push_back(specIndexTemp[0]);
  return specInds;
}

/** Uses E_KE = mv^2/2 and s = vt to calculate the time required for a neutron
*  to travel a distance, s
* @param s ditance travelled in meters
* @param E_KE kinetic energy in meV
* @return the time to taken to travel that uninterrupted distance in seconds
*/
double libISISGetEi::timeToFly(double s, double E_KE) const
{
  // E_KE = mv^2/2, s = vt
  // t = s/v, v = sqrt(2*E_KE/m)
  // t = s/sqrt(2*E_KE/m)

  // convert E_KE to joules kg m^2 s^-2
  E_KE *= PhysicalConstants::meV;

  return s/sqrt(2*E_KE/PhysicalConstants::NeutronMass);
}

/** Looks for and examines a peak close to that specified by the input parameters and
*  examines it to find a representative time for when the neutrons hit the detector
*  @param WS the workspace containing the monitor spectrum
*  @param monitIn the index of the histogram that contains the monitor spectrum
*  @param peakTime the estimated TOF of the monitor peak in the time units of the workspace
*  @return a time of flight value in the peak in microseconds
*  @throw invalid_argument if a good peak fit wasn't made or the input workspace does not have common binning
*  @throw out_of_range if the peak runs off the edge of the histogram
*  @throw runtime_error a sub-algorithm just falls over
*/
double libISISGetEi::getPeakCentre(API::MatrixWorkspace_const_sptr WS, const int monitIn, const double peakTime)
{
  double tMin = 0;
  double tMax = 0;
  
  if ( peakTime > 0.0 )
  {
    switch (monitIn)
    {//MARI specific code
      case 1 :
        tMin = (1-MON1_TOF_WIN)*peakTime;
        tMax = (1+MON1_TOF_WIN)*peakTime;
        g_log.information() << "Based on the user selected energy the first peak will be searched for at TOF " << peakTime << " micro seconds +/-" << boost::lexical_cast<std::string>(100.0*MON1_TOF_WIN) << "%\n";
        break;
      //MARI specific code
      case 2 :
        tMin = (1-MON2_TOF_WIN)*peakTime;
        tMax = (1+MON2_TOF_WIN)*peakTime;
        g_log.information() << "Based on the user selected energy the second peak will be searched for at TOF " << peakTime << " micro seconds +/-" << boost::lexical_cast<std::string>(100.0*MON2_TOF_WIN) << "%\n";
        break;
    }
  }
  else
  {
    tMin = 400;
    tMax = 12000;
    g_log.information() << "No energy estimate given, using default window t0 = " << tMin << " microseconds, t1 = " << tMax << " microseconds\n";  
  }
  // runs CropWorkspace as a sub-algorithm to and puts the result in a new temporary workspace that will be deleted when this algorithm has finished
  extractSpec(monitIn, tMin, tMax);
  // converting the workspace to count rate is required by the fitting algorithm if the bin widths are not all the same, if the workspace is already a distribution this does nothing
  WorkspaceHelpers::makeDistribution(m_tempWS);
  // look out for user cancel messgages as the above command can take a bit of time
//  advanceProgress(GET_COUNT_RATE);

  return getPeakFirstMoments(m_tempWS, tMin, tMax );
}
/** Calls CropWorkspace as a sub-algorithm and passes to it the InputWorkspace property
*  @param specInd the index number of the histogram to extract
*  @param start the number of the first bin to include (starts counting bins at 0)
*  @param end the number of the last bin to include (starts counting bins at 0)
*  @throw out_of_range if start, end or specInd are set outside of the vaild range for the workspace
*  @throw runtime_error if the algorithm just falls over
*  @throw invalid_argument if the input workspace does not have common binning
*/
void libISISGetEi::extractSpec(int specInd, double start, double end)
{
  IAlgorithm_sptr childAlg =
    createSubAlgorithm("CropWorkspace", 100*m_fracCompl, 100*(m_fracCompl+CROP) );
  m_fracCompl += CROP;
  
  childAlg->setPropertyValue( "InputWorkspace",
                              getPropertyValue("InputWorkspace") );
  childAlg->setProperty( "XMin", start);
  childAlg->setProperty( "XMax", end);
  childAlg->setProperty( "StartWorkspaceIndex", specInd);
  childAlg->setProperty( "EndWorkspaceIndex", specInd);

  try
  {
    childAlg->execute();
  }
  catch (std::exception&)
  {
    g_log.error("Exception thrown while running CropWorkspace as a sub-algorithm");
    throw;
  }

  if ( ! childAlg->isExecuted() )
  {
    g_log.error("The CropWorkspace algorithm failed unexpectedly, aborting.");
    throw std::runtime_error(name() + " failed trying to run CropWorkspace");
  }
  m_tempWS = childAlg->getProperty("OutputWorkspace");

//DEBUGGING CODE uncomment out the line below if you want to see the TOF window that was analysed
//AnalysisDataService::Instance().addOrReplace("croped_dist_del", m_tempWS);
  progress(m_fracCompl);
  interruption_point();
}

/** Implements the Fortran subroute IXFmoments_dataset_2d() from the libISIS
*/
double libISISGetEi::getPeakFirstMoments(API::MatrixWorkspace_sptr WS, const double tMin, const double tMax)
{
  // ! the original Fortran subroutine IXFunspike_1d() used by libISIS is more thorough it checks the errors too
  // calls SmoothData as a subalgorithm
  WS =  smooth(WS);

  double prominence = 4.0;                    //! to start prominence=4
    
  MantidVec centredXs(WS->blocksize()-1, NO_ESTIMATE);
  
  for( int i = 1; i < WS->blocksize(); i++ )
  {
    centredXs[i-1] = (WS->readX(0)[i] + WS->readX(0)[i-1])/2;
  }
  
  double A_M = 0, c = 0, c_fwhm = 0, w = 0, T_Mean = 0;
  getPeakMean(centredXs, WS->readY(0), WS->readE(0), prominence, A_M, c, c_fwhm, w, T_Mean);

  const double bmin = w/(1.5*NPOINTS);
 
  if (c_fwhm <= 0.0)
  {//       !add status call  !!
    throw std::invalid_argument("No peak found, check tMin, tMax and the Monitor index");
  }
  
  // !regroup function acts on histogram data

  WS = reBin(WS, tMin, bmin, tMax);

  centredXs.resize(WS->blocksize()-1);
  for( int i = 1; i < WS->blocksize(); ++i )
  {
    centredXs[i-1] = (WS->readX(0)[i] + WS->readX(0)[i-1])/2;
  }

  //! call get moments again, prominence=4 still
  try {

  getPeakMean(centredXs, WS->readY(0), WS->readE(0), prominence, A_M, c, c_fwhm, w, T_Mean);
  }
  catch(std::invalid_argument &)
  {
    prominence=2.0;
    getPeakMean(centredXs, WS->readY(0), WS->readE(0), prominence, A_M, c, c_fwhm, w, T_Mean);
  }

  if ((c == 0.0) || (w > (0.2*c_fwhm)))
  {
    throw std::invalid_argument("no valid peak found, check initial tMin, tMax and the Monitor index");
  }
  //??STEVES?? implement this error checking Fortran, if required
/*    ! find time channels across the peak
    imin=IXFlower_index(x_p,c_fwhm-(0.5*w))
    imax=IXFupper_index(x_p,c_fwhm+(0.5*w))
    if(imax-imin < 6) call IXFwrite_line('check data Monitor index '//index_char//' (IXFmoments_dataset_2d)',status)*/

  return T_Mean;
}

API::MatrixWorkspace_sptr libISISGetEi::smooth(API::MatrixWorkspace_sptr WS)
{
  IAlgorithm_sptr childAlg =
    createSubAlgorithm("SmoothData");
  
  childAlg->setProperty( "InputWorkspace", WS );
  childAlg->setProperty( "NPoints", 3);

  try
  {
    childAlg->execute();
  }
  catch (std::exception&)
  {
    g_log.error("Exception thrown while running SmoothData as a sub-algorithm");
    throw;
  }

  if ( ! childAlg->isExecuted() )
  {
    g_log.error("The Smooth algorithm failed unexpectedly, aborting.");
    throw std::runtime_error(name() + " failed trying to run SmoothData");
  }
  return childAlg->getProperty("OutputWorkspace");
}

API::MatrixWorkspace_sptr libISISGetEi::reBin(API::MatrixWorkspace_sptr WS, const double first, const double width, const double end)
{
  IAlgorithm_sptr childAlg =
    createSubAlgorithm("Rebin");
  
  childAlg->setProperty( "InputWorkspace", WS );
  std::ostringstream binParams;
  binParams << first << "," << width << "," << end;
  childAlg->setPropertyValue( "Params", binParams.str());

  try
  {
    childAlg->execute();
  }
  catch (std::exception&)
  {
    g_log.error("Exception thrown while running Regroup as a sub-algorithm");
    throw;
  }

  if ( ! childAlg->isExecuted() )
  {
    g_log.error("The Rebin algorithm failed unexpectedly, aborting.");
    throw std::runtime_error(name() + " failed trying to run Rebin");
  }
  return childAlg->getProperty("OutputWorkspace");
}

void libISISGetEi::getPeakMean(const MantidVec& Xs, const MantidVec& Ys, const MantidVec& Es, const double prominence, double &area, double &c, double &c_fwhm, double &w, double &xbar)
{
  MantidVec::const_iterator peakIt = std::max_element(Ys.begin(), Ys.end());    //! position of peak
  unsigned int iPeak = peakIt - Ys.begin();
  double peakY = Ys[iPeak];
  double peakE = Es[iPeak];

  //! Find data range that satisfies prominence criterion: im < ipk < ip will be nearest points that satisfy this
  int iRightHand = iPeak-1;
  for( ; iRightHand >= 0; --iRightHand )
  {
    const double ratio = Ys[iRightHand]/peakY;
    const double ratio_err =
      std::sqrt( std::pow(Es[iRightHand],2) + std::pow(ratio*peakE,2) )/peakY;
    if ( ratio < (1.0/prominence - SFAC_PEAK*ratio_err) )
    {
      break;
    }
  }
  
  int iLeftHand = iPeak+1;
  for( ; iLeftHand <= Xs.size(); iLeftHand++ )
  {
    const double ratio = Ys[iLeftHand]/peakY;
    const double ratio_err =
      std::sqrt( std::pow(Es[iLeftHand], 2) + std::pow(ratio*peakE, 2) )/peakY;
    if ( ratio < (1.0/prominence - SFAC_PEAK*ratio_err) )
    {
      break;
    }
  }
  
  if ( iLeftHand < Xs.size() && iRightHand > 0 )  //  ! peak in data
  {
    c = Xs[iPeak];
  }
  else
  {
    throw std::invalid_argument("No peak found in data that satisfies prominence criterion");
  }
// ! We now have a peak, so can start filling output arguments
// ! Determine extent of peak using derivatives
// ! At this point 1 =< im < ipk < ip =< size(x)
// ! After this section, new values will be given to im, ip that still satisfy these inequalities.
// !
// ! The algorithm for negative side skipped if im=1; positive side skipped if ip=size(x); 
// ! if fails derivative criterion -> ip=size(x) (+ve)  im=1 (-ve)
// ! In either case, we deem that the peak has a tail(s) that extend outside the range of x

  if ( iLeftHand < Xs.size() )
  {
    double deriv = -1000.0;
    double error = 0.0;
    while ( ( iLeftHand < Xs.size() - 1 ) && ( deriv < -SFAC_DERIV*error ) )
    {
      double dtp = Xs[iLeftHand+1] - Xs[iLeftHand];
      double dtm = Xs[iLeftHand] - Xs[iLeftHand-1];
      deriv = 0.5*( ((Ys[iLeftHand+1] - Ys[iLeftHand]) / dtp) + ((Ys[iLeftHand] - Ys[iLeftHand-1]) / dtm) );
      error = 0.5*std::sqrt( ( (std::pow(Es[iLeftHand+1], 2) + std::pow(Es[iLeftHand], 2) ) / std::pow(dtp,2) ) + ((std::pow(Es[iLeftHand], 2) + std::pow(Es[iLeftHand-1], 2) )/std::pow(dtm,2) )
        - 2.0*(std::pow(Es[iLeftHand], 2) / (dtp*dtm)) );
      iLeftHand = iLeftHand + 1;
    }
    iLeftHand = iLeftHand - 1;

    if (deriv < -error)
    {
      iLeftHand = Xs.size() -1;      //        ! derivative criterion not met
    }
  }

  if (iRightHand > 0)
  {
    double deriv = 1000.0;
    double error = 0.0;
    while ( (iRightHand > 0) && (deriv > SFAC_DERIV*error) )
    {
      double dtp = Xs[iRightHand+1] - Xs[iRightHand];
      double dtm = Xs[iRightHand] - Xs[iRightHand-1];
      deriv = 0.5*( ((Ys[iRightHand+1] - Ys[iRightHand]) / dtp) + ( (Ys[iRightHand] - Ys[iRightHand-1]) / dtm) );
      error = 0.5*std::sqrt( ( (std::pow(Es[iRightHand+1], 2) + std::pow(Es[iRightHand], 2) ) / std::pow(dtp, 2) ) + (( std::pow(Es[iRightHand], 2) + std::pow(Es[iRightHand-1], 2) ) / std::pow(dtm, 2) )
        - 2.0*std::pow(Es[iRightHand], 2)/(dtp*dtm) );
      iRightHand = iRightHand - 1;
    }
    iRightHand = iRightHand + 1;
    if (deriv > error) iRightHand = 0;//        ! derivative criterion not met
  }
  double pk_min = Xs[iRightHand];
  double pk_max = Xs[iLeftHand];
  double pk_width = Xs[iLeftHand] - Xs[iRightHand];
        
// ! Determine background from either side of peak.
// ! At this point, iRightHand and iLeftHand define the extreme points of the peak
// ! Assume flat background
    double bkgd = 0.0;
    double bkgd_range = 0.0;
    double bkgd_min = std::max(Xs[0], pk_min - BKGD_FAC*pk_width);
    double bkgd_max = std::min(Xs[Xs.size()-1], pk_max + BKGD_FAC*pk_width);
    if (iRightHand>1)
    {
      double bkgd_m, bkgd_err_m;
      integrate(bkgd_m, bkgd_err_m, Xs, Ys, Es, bkgd_min, pk_min);
      bkgd = bkgd + bkgd_m;
      bkgd_range = bkgd_range + (pk_min-bkgd_min);
    }

    if (iLeftHand<Xs.size())
    {
      double bkgd_p, bkgd_err_p;
      integrate(bkgd_p, bkgd_err_p, Xs, Ys, Es, pk_max, bkgd_max);
      bkgd = bkgd + bkgd_p;
      bkgd_range = bkgd_range + (bkgd_max-pk_max);
    }
    
    if ( (iRightHand>1) || (iLeftHand<Xs.size()) ) //   ! background from at least one side
    {
      bkgd = bkgd / bkgd_range;
    }

// ! Perform moment analysis on the peak after subtracting the background
// !   Fill arrays with peak only:
    std::vector<double> xint(iLeftHand-iRightHand+1);
    std::copy( Xs.begin()+iRightHand, Xs.begin()+iLeftHand+1, xint.begin());
      
    std::vector<double> yint(iLeftHand-iRightHand+1);
    std::transform( Ys.begin()+iRightHand, Ys.begin()+iLeftHand+1, yint.begin(), std::bind2nd(std::minus<double>(),bkgd) );
    
    std::vector<double> eint(iLeftHand-iRightHand+1);
    std::copy( Es.begin()+iRightHand, Es.begin()+iLeftHand+1, eint.begin());

// !   FWHH:
    int ipk_int = iPeak-iRightHand+1;  //       ! peak position in internal array
    double hby2 = 0.5*yint[ipk_int];
    int ip1(0), ip2(0);
    double xp_hh(0);
    if (yint[yint.size()-1]<hby2)
    {
      for( int i = ipk_int; i < yint.size();  ++i )
      {
        if (yint[i]<hby2)
        {
          ip1 = i-1;         //   ! after this point the intensity starts to go below half-height
          break;//goto 901
        }
      }
      for ( int i=yint.size()-1; i >= ipk_int; --i )
      {
        if (yint[i]>hby2)
        {
          ip2 = i+1;           //   ! point closest to peak after which the intensity is always below half height
          break;//goto 902
        }
      }
      xp_hh = xint[ip2] + (xint[ip1]-xint[ip2])*((hby2-yint[ip2])/(yint[ip1]-yint[ip2]));
    }
    else
    {
      xp_hh = xint[yint.size()-1];
    }

    int im1(0), im2(0);
    double xm_hh(0);
    if (yint[0]<hby2)
    {
      for ( int i=ipk_int; i >= 0; --i )
      {
        if (yint[i]<hby2)
        {
          im1 = i+1;   // ! after this point the intensity starts to go below half-height
          break;
        }
      }
      for ( int i=0; i <= ipk_int; ++i )
      {
        if (yint[i]>hby2)
        {
          im2 = i-1;   // ! point closest to peak after which the intensity is always below half height
          break;
        }
      }
      xm_hh = xint[im2] + (xint[im1]-xint[im2])*((hby2-yint[im2])/(yint[im1]-yint[im2]));
    }
    else
    {
      xm_hh = xint[0];
    }
    
    c_fwhm = 0.5*(xp_hh + xm_hh);
    w = xp_hh - xm_hh;

// ! area:
    double dummy;
    integrate(area, dummy, xint, yint, eint, pk_min, pk_max);
// ! first moment:
    std::transform(yint.begin(), yint.end(), xint.begin(), yint.begin(), std::multiplies<double>());
    integrate(xbar, dummy, xint, yint, eint, pk_min, pk_max);
    xbar = xbar / area;
    
  // look out for user cancel messgages
  advanceProgress(FIT_PEAK);
}

void libISISGetEi::integrate(double &bkgd_m, double &bkgd_err_m, const MantidVec &x, const MantidVec &s, const MantidVec &e, const double xmin, const double xmax)
{
  MantidVec::const_iterator lowit=std::lower_bound(x.begin(), x.end(), xmin);
  MantidVec::difference_type distmin=std::distance(x.begin(),lowit);
  MantidVec::const_iterator highit=std::find_if(lowit,x.end(),std::bind2nd(std::greater<double>(),xmax));
  MantidVec::difference_type distmax=std::distance(x.begin(),highit);
  
  std::vector<double> widths(x.size());
  std::adjacent_difference(lowit,highit,widths.begin()); // highit+1 is safe while input workspace guaranteed to be histogram
  widths[0] = widths[1];
  bkgd_m=std::inner_product(s.begin()+distmin,s.begin()+distmax,widths.begin(),0.0);
  bkgd_err_m=std::inner_product(e.begin()+distmin,e.begin()+distmax,widths.begin(),0.0,std::plus<double>(),VectorHelper::TimesSquares<double>());

//    unsigned int nx = s.size();
//    unsigned int ml(0), mu(nx - 1);
//    MantidVec::const_iterator lowit = std::lower_bound(x.begin(), x.end(), xmin);
//    ml = lowit - x.begin();
//    
//    MantidVec::const_iterator highit = std::lower_bound(x.begin(), x.end(), xmax);
//    mu = highit - x.begin();
//   
//
//
//    //! note: At this point, 0=<ml=<nx & xmin=<x(ml); 1=<mu=<nx & x(mu)=<xmax; BUT mu > ml-1
//
//    //! Perform integration:
//    //! ----------------------
//
//    //! Calculate integral:
//
//    //!	if (mu<ml) then
//    //!	special case of no data points in the integration range
//    //!		ilo = max(ml-1,1)	! x(1) is end point if ml=1
//    //!		ihi = min(mu+1,nx)	! x(mu) is end point if mu=nx
//    //!		val = 0.5_dp * ((xmax-xmin)/(x(ihi)-x(ilo))) * &
//    //!			&( s(ihi)*((xmax-x(ilo))+(xmin-x(ilo))) + s(ilo)*((x(ihi)-xmax)+(x(ihi)-xmin)) )
//    //!	else
//    //!	xmin and xmax are separated by at least one data point in x(:)
//    //!	  sum over complete steps in the integration range:
//    //!		if (mu > ml) then	! at least one complete step
//    //!			val = sum((s(ml+1:mu)+s(ml:mu-1))*(x(ml+1:mu)-x(ml:mu-1)))
//    //!		else
//    //!			val = 0.0_dp
//    //!		endif
//    //!	  ends of the integration range:
//    //!		if (ml>1) then	! x(1) is end point if ml=1
//    //!			x1eff = (xmin*(xmin-x(ml-1)) + x(ml-1)*(x(ml)-xmin))/(x(ml)-x(ml-1))
//    //!			s1eff = s(ml-1)*(x(ml)-xmin)/((x(ml)-x(ml-1)) + (xmin-x(ml-1)))
//    //!			val = val + (x(ml)-x1eff)*(s(ml)+s1eff)
//    //!		endif
//    //!		if (mu<nx) then	! x(mu) is end point if mu=nx
//    //!			xneff = (xmax*(x(mu+1)-xmax) + x(mu+1)*(xmax-x(mu)))/(x(mu+1)-x(mu))
//    //!			sneff = s(mu+1)*(xmax-x(mu))/((x(mu+1)-x(mu)) + (x(mu+1)-xmax))
//    //!			val = val + (xneff-x(mu))*(s(mu)+sneff)
//    //!		endif
//    //!		val = 0.5_dp*val
//    //!	endif			
//
//    //! Calculate error on the integral:
//
//    if (mu<ml) 
//    {
//      throw std::out_of_range("Incorrect integration limits");
//    }
//
//    //!	xmin and xmax are separated by at least one data point in x(:)
//      // !	Set up effective end points:
//    double x1eff(0.0), s1eff(0.0), e1eff(0.0);
//    if (ml > 0) 
//    {
//      	// x(1) is end point if ml=1
//          x1eff = (xmin*(xmin-x[ml-1]) + x[ml-1]*(x[ml]-xmin))/(x[ml]-x[ml-1]);
//          s1eff = s[ml-1]*(x[ml]-xmin)/((x[ml]-x[ml-1]) + (xmin-x[ml-1]));
//          e1eff = e[ml-1]*(x[ml]-xmin)/((x[ml]-x[ml-1]) + (xmin-x[ml-1]));
//    }
//    else 
//    {
//         x1eff = x[ml];
//         s1eff = 0.0;
//         e1eff = 0.0;
//    }
//    double xneff(0.0), sneff(0.0), eneff(0.0); 
//    if ( mu < nx - 1 ) 
//    {	
//      // x(mu) is end point if mu=nx
//          xneff = (xmax*(x[mu+1]-xmax) + x[mu+1]*(xmax-x[mu]))/(x[mu+1]-x[mu]);
//          sneff = s[mu+1]*(xmax-x[mu])/((x[mu+1]-x[mu]) + (x[mu+1]-xmax));
//          eneff = e[mu+1]*(xmax-x[mu])/((x[mu+1]-x[mu]) + (x[mu+1]-xmax));
//    }
//       else
//       {
//          xneff = x[nx -1];
//          sneff = 0.0;
//          eneff = 0.0;
//      }
//       //	xmin to x(ml):
//       double val = (x[ml]-x1eff)*(s[ml]+s1eff);
//         double err = std::pow(e1eff*(x[ml]-x1eff), 2);
////       !	x(ml) to x(mu):
//       if (mu==ml)
//       {
//         //! one data point, no complete intervals
//         err = err + std::pow(e[ml]*(xneff-x1eff), 2);
//       }
//       else if (mu == ml+1)
//       {
//         //! one complete interval
//          val = val + (s[mu]+s[ml])*(x[mu]-x[ml]);
//          err = err + std::pow(e[ml]*(x[ml+1]-x1eff), 2) + std::pow(e[mu]*(xneff-x[mu-1]), 2);
//       }
//       else
//       {
//         // ! this is the whole trapezium summing
////          val = val + sum( (s(ml+1:mu)+s(ml:mu-1))*(x(ml+1:mu)-x(ml:mu-1)) );
//            std::vector<double> ssum(mu - ml, 0.0);
//            std::transform(s.begin()+ml+1, s.begin()+mu, s.begin()+ml, ssum.begin(), std::plus<double>());
//            std::vector<double> xdiff(mu - ml, 0.0);
//            std::transform(x.begin()+ml+1, x.begin()+mu, x.begin()+ml, xdiff.begin(), std::minus<double>());
//            val += std::inner_product(ssum.begin(), ssum.end(), xdiff.begin(), 0.0);
//
//          //err = err + (e(ml)*(x(ml+1)-x1eff))**2 + (e(mu)*(xneff-x(mu-1)))**2 
//          //err = err+ sum((e(ml+1:mu-1)*(x(ml+2:mu)-x(ml:mu-2)))**2)
//       }
//
//       //	x(mu) to xmax:
//       val = val + (xneff-x[mu])*(s[mu]+sneff);
//         err = err + std::pow(eneff*(xneff-x[mu]), 2);
//
//       bkgd_m = 0.5*val;
//       bkgd_err_m = 0.0;//0.5*std::sqrt(err);
}

/** Get the kinetic energy of a neuton in joules given it speed using E=mv^2/2
*  @param speed the instantanious speed of a neutron in metres per second
*  @return the energy in joules
*/
double libISISGetEi::neutron_E_At(double speed) const
{
  // E_KE = mv^2/2
  return PhysicalConstants::NeutronMass*speed*speed/(2);
}

/// Update the percentage complete estimate assuming that the algorithm has completed a task with estimated RunTime toAdd
void libISISGetEi::advanceProgress(double toAdd)
{
  m_fracCompl += toAdd;
  progress(m_fracCompl);
  // look out for user cancel messgages
  interruption_point();
}


} // namespace Algorithms
} // namespace Mantid
