#include "MantidAlgorithms/GetEi2.h"

#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h" 
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/IObjComponent.h"

#include <boost/lexical_cast.hpp>
#include <cmath>
#include <algorithm>
#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace Algorithms
{
  
  // Register the algorithm into the algorithm factory
  DECLARE_ALGORITHM(GetEi2)
  
  /// Sets documentation strings for this algorithm
  void GetEi2::initDocs()
  {
    this->setWikiSummary("Calculates the kinetic energy of neutrons leaving the source based on the time it takes for them to travel between two monitors. ");
    this->setOptionalMessage("Calculates the kinetic energy of neutrons leaving the source based on the time it takes for them to travel between two monitors.");
  }
  

/** 
* Default contructor
*/
GetEi2::GetEi2() : Algorithm(), m_input_ws(), m_peak1_pos(0, 0.0), m_fixedei(false), m_tof_window(0.1), m_peak_signif(2.0), m_peak_deriv(1.0),
  m_binwidth_frac(1.0/12.0), m_bkgd_frac(0.5)
{
  // Conversion factor common for converting between micro seconds and energy in meV
  m_t_to_mev = 5e11 * PhysicalConstants::NeutronMass / PhysicalConstants::meV;
}

void GetEi2::init()

{// Declare required input parameters for algorithm and do some validation here
  CompositeValidator<> *validator = new CompositeValidator<>;
  validator->add(new WorkspaceUnitValidator<>("TOF"));
  validator->add(new HistogramValidator<>);
  validator->add(new InstrumentValidator<>);

  declareProperty(new WorkspaceProperty<>(
    "InputWorkspace","",Direction::InOut, validator),
    "The X units of this workspace must be time of flight with times in\n"
    "microseconds");
  BoundedValidator<int> *mustBePositive = new BoundedValidator<int>();
  mustBePositive->setLower(0);
  declareProperty("Monitor1Spec", -1, mustBePositive,
    "The spectrum number to use as the first monitor\n");
  declareProperty("Monitor2Spec", -1, mustBePositive->clone(),
    "The spectrum number to use as the second monitor\n");
  BoundedValidator<double> *positiveDouble = new BoundedValidator<double>();
  positiveDouble->setLower(0.0);
  declareProperty("EnergyEstimate", -1.0 , positiveDouble,
    "An approximate value for the typical incident energy, energy of\n"
    "neutrons leaving the source (meV)");
  declareProperty("FixEi", false, "If true, the incident energy will be set to the value of the \n"
    "EnergyEstimate property.");
  declareProperty("IncidentEnergy", -1.0, Direction::Output);
  declareProperty("FirstMonitorPeak", -1.0, Direction::Output);
  declareProperty("FirstMonitorIndex", (size_t)0, Direction::Output);
  declareProperty("Tzero", EMPTY_DBL(), Direction::Output);

}

/** Executes the algorithm
*  @throw out_of_range if the peak runs off the edge of the histogram
*  @throw NotFoundError if one of the requested spectrum numbers was not found in the workspace
*  @throw IndexError if there is a problem converting spectra indexes to spectra numbers, which would imply there is a problem with the workspace
*  @throw invalid_argument if a good peak fit wasn't made or the input workspace does not have common binning
*  @throw runtime_error if there is a problem with the SpectraDetectorMap or a sub-algorithm falls over
*/
void GetEi2::exec()
{
  m_input_ws = getProperty("InputWorkspace");
  m_fixedei = getProperty("FixEi");
  double initial_guess = getProperty("EnergyEstimate");
  double incident_energy = calculateEi(initial_guess);
  if( !m_fixedei )
  {
    g_log.notice() << "Incident energy = " << incident_energy << " meV from initial guess = " << initial_guess << " meV\n";
  }
  
  storeEi(incident_energy);
  
  setProperty("InputWorkspace", m_input_ws);
  // Output properties
  setProperty("IncidentEnergy", incident_energy);
  setProperty("FirstMonitorIndex", m_peak1_pos.first);
  setProperty("FirstMonitorPeak", m_peak1_pos.second);
}

/** Calculate the incident energy of the neutrons for the input workspace on this algorithm
 *  @param initial_guess :: A guess for value of the incident energy
 *  @return The calculated incident energy
 */
double GetEi2::calculateEi(const double initial_guess)
{
  const specid_t monitor1_spec = getProperty("Monitor1Spec");
  const specid_t monitor2_spec = getProperty("Monitor2Spec");

  //Covert spectrum numbers to workspace indices
  std::vector<specid_t> spec_nums(2, monitor1_spec);
  spec_nums[1] = monitor2_spec;
  std::vector<size_t> mon_indices;
  mon_indices.reserve(2);
  // get the index number of the histogram for the first monitor
  m_input_ws->getIndicesFromSpectra(spec_nums, mon_indices);

  if( mon_indices.size() != 2 )
  {
    g_log.error() << "Error retrieving monitor spectra from input workspace. Check input properties.\n";
    throw std::runtime_error("Error retrieving monitor spectra spectra from input workspace.");
  }

  std::cerr << "Testing " << mon_indices[0] << " " << mon_indices[1] << "\n";

  // Calculate actual peak postion for each monitor peak
  double peak_times[2] = {0.0, 0.0};
  double det_distances[2] = {0.0, 0.0};
  for( unsigned int i = 0; i < 2; ++i )
  { 
    size_t ws_index = mon_indices[i];
    det_distances[i] = getDistanceFromSource(ws_index);
    const double peak_guess = det_distances[i]*std::sqrt(m_t_to_mev/initial_guess);
    if( m_fixedei && i == 0 )
    {
      m_peak1_pos = std::make_pair(ws_index, peak_guess);
      g_log.information() << "First monitor peak = " << peak_guess << " microseconds from fixed Ei = " << initial_guess << " meV\n"; 
      break;
    }
    const double t_min = (1.0 - m_tof_window)*peak_guess;
    const double t_max = (1.0 + m_tof_window)*peak_guess;
    g_log.information() << "Time-of-flight window for peak " << (i+1) << ": tmin = " << t_min << " microseconds, tmax = " << t_max << " microseconds\n";
    peak_times[i] = calculatePeakPosition(ws_index, t_min, t_max);
    g_log.information() << "Peak for monitor " << (i+1) << " (at " << det_distances[i] << " metres) = " << peak_times[i] << " microseconds\n";
    if( i == 0 )
    {  
      //Store for later adjustment of bins
      m_peak1_pos = std::make_pair(ws_index, peak_times[i]);
    }
  }
  
  if( m_fixedei )
  {
    return initial_guess;
  }
  else
  {
    double mean_speed = (det_distances[1] - det_distances[0])/(peak_times[1] - peak_times[0]);
    
    double tzero = peak_times[1] - ((1.0/mean_speed)*det_distances[1]);
    setProperty("Tzero", tzero);
    
    g_log.debug() << "T0 = " << tzero << std::endl;
    
    g_log.debug() << "Mean Speed = " << mean_speed << std::endl;
    g_log.debug() << "Energy (meV) = " << mean_speed*mean_speed*m_t_to_mev << std::endl;
        
    return mean_speed*mean_speed*m_t_to_mev;
  }
}

/** Gets the distance between the source and detectors whose workspace index is passed
 *  @param ws_index :: The workspace index of the detector
 *  @return The distance between the source and the given detector(or DetectorGroup)
 *  @throw runtime_error if there is a problem
 */
double GetEi2::getDistanceFromSource(size_t ws_index) const
{
  const IObjComponent_sptr source = m_input_ws->getInstrument()->getSource();
  // Retrieve a pointer detector
  IDetector_sptr det = m_input_ws->getDetector(ws_index);
  if( !det )
  {
    std::ostringstream msg;
        msg << "A detector for monitor at workspace index " << ws_index << " cannot be found. ";
    throw std::runtime_error(msg.str());
  }
  return det->getDistance(*source);
}

/**
 * Calculate the peak position of a given TOF range within the chosen spectrum
 * @param ws_index :: the wokspace index 
 * @param t_min :: the min time to consider
 * @param t_max :: the max time to consider
 * @return the peak position
 */
double GetEi2::calculatePeakPosition(size_t ws_index, double t_min, double t_max)
{
   //Crop out the current monitor workspace to the min/max times defined
  MatrixWorkspace_sptr monitor_ws = extractSpectrum(ws_index, t_min, t_max);
  // Workspace needs to be a count rate for the fitting algorithm
  WorkspaceHelpers::makeDistribution(monitor_ws);

  const double prominence(4.0);
  std::vector<double> dummyX, dummyY, dummyE;
  double peak_width = calculatePeakWidthAtHalfHeight(monitor_ws, prominence, dummyX, dummyY, dummyE);
  monitor_ws = rebin(monitor_ws, t_min, peak_width*m_binwidth_frac, t_max);

  double t_mean(0.0);
  try
  {
    t_mean = calculateFirstMoment(monitor_ws, prominence);
  }
  catch(std::runtime_error&)
  {
    //Retry with smaller prominence factor
    t_mean = calculateFirstMoment(monitor_ws, 2.0);
  }

  return t_mean;
}

/** Calls CropWorkspace as a sub-algorithm and passes to it the InputWorkspace property
 *  @param ws_index :: the index number of the histogram to extract
 *  @param start :: the number of the first bin to include (starts counting bins at 0)
 *  @param end :: the number of the last bin to include (starts counting bins at 0)
 *  @return The cropped workspace
 *  @throw out_of_range if start, end or specInd are set outside of the vaild range for the workspace
 *  @throw runtime_error if the algorithm just falls over
 *  @throw invalid_argument if the input workspace does not have common binning
 */
MatrixWorkspace_sptr GetEi2::extractSpectrum(size_t ws_index, const double start, const double end)
{
  IAlgorithm_sptr childAlg = createSubAlgorithm("CropWorkspace");
  childAlg->setProperty("InputWorkspace", m_input_ws);
  childAlg->setProperty<int>("StartWorkspaceIndex", static_cast<int>(ws_index));
  childAlg->setProperty<int>("EndWorkspaceIndex", static_cast<int>(ws_index));
  childAlg->setProperty("XMin", start);
  childAlg->setProperty("XMax", end);
  childAlg->executeAsSubAlg();
  return childAlg->getProperty("OutputWorkspace");
}

/**
 * Calculate the width of the peak within the given region
 * @param data_ws :: The workspace containg the window around the peak
 * @param prominence :: The factor that the peak must be above the error to count as a peak
 * @param peak_x :: An output vector containing just the X values of the peak data
 * @param peak_y :: An output vector containing just the Y values of the peak data
 * @param peak_e :: An output vector containing just the E values of the peak data
 * @returns The width of the peak at half height
*/
double GetEi2::calculatePeakWidthAtHalfHeight(API::MatrixWorkspace_sptr data_ws, const double prominence, 
                                              std::vector<double> & peak_x, std::vector<double> & peak_y, std::vector<double> & peak_e) const
{
  // First create a temporary vector of bin_centre values to work with
  std::vector<double> Xs(data_ws->readX(0).size());
  VectorHelper::convertToBinCentre(data_ws->readX(0), Xs);
  const MantidVec & Ys = data_ws->readY(0);
  const MantidVec & Es = data_ws->readE(0);

  MantidVec::const_iterator peakIt = std::max_element(Ys.begin(), Ys.end());
  MantidVec::difference_type iPeak = peakIt - Ys.begin();
  double peakY = Ys[iPeak];
  double peakE = Es[iPeak];

  const std::vector<double>::size_type nxvals = Xs.size();

  //! Find data range that satisfies prominence criterion: im < ipk < ip will be nearest points that satisfy this
  int64_t im = static_cast<int64_t>(iPeak-1);
  for( ; im >= 0; --im )
  {
    const double ratio = Ys[im]/peakY;
    const double ratio_err = std::sqrt( std::pow(Es[im],2) + std::pow(ratio*peakE,2) )/peakY;
    if ( ratio < (1.0/prominence - m_peak_signif*ratio_err) )
    {
      break;
    }
  }
  
  std::vector<double>::size_type ip = iPeak+1;
  for( ; ip < nxvals; ip++ )
  {
    const double ratio = Ys[ip]/peakY;
    const double ratio_err =
      std::sqrt( std::pow(Es[ip], 2) + std::pow(ratio*peakE, 2) )/peakY;
    if ( ratio < (1.0/prominence - m_peak_signif*ratio_err) )
    {
      break;
    }
  }
  
  if ( ip == nxvals || im < 0 )  
  {
    throw std::invalid_argument("No peak found in data that satisfies prominence criterion");
  }

  // We now have a peak, so can start filling output arguments
  // Determine extent of peak using derivatives
  // At this point 1 =< im < ipk < ip =< size(x)
  // After this section, new values will be given to im, ip that still satisfy these inequalities.
  // 
  // The algorithm for negative side skipped if im=1; positive side skipped if ip=size(x); 
  // if fails derivative criterion -> ip=size(x) (+ve)  im=1 (-ve)
  // In either case, we deem that the peak has a tail(s) that extend outside the range of x

  if ( ip < nxvals )
  {
    double deriv = -1000.0;
    double error = 0.0;
    while ( ( ip < nxvals - 1 ) && ( deriv < -m_peak_deriv*error ) )
    {
      double dtp = Xs[ip+1] - Xs[ip];
      double dtm = Xs[ip] - Xs[ip-1];
      deriv = 0.5*( ((Ys[ip+1] - Ys[ip]) / dtp) + ((Ys[ip] - Ys[ip-1]) / dtm) );
      error = 0.5*std::sqrt( ( (std::pow(Es[ip+1], 2) + std::pow(Es[ip], 2) ) / std::pow(dtp,2) ) + ((std::pow(Es[ip], 2) + std::pow(Es[ip-1], 2) )/std::pow(dtm,2) )
        - 2.0*(std::pow(Es[ip], 2) / (dtp*dtm)) );
      ip++;
    }
    ip--;

    if (deriv < -error)
    {
      ip = nxvals -1;      //        ! derivative criterion not met
    }
  }

  if (im > 0)
  {
    double deriv = 1000.0;
    double error = 0.0;
    while ( (im > 0) && (deriv > m_peak_deriv*error) )
    {
      double dtp = Xs[im+1] - Xs[im];
      double dtm = Xs[im] - Xs[im-1];
      deriv = 0.5*( ((Ys[im+1] - Ys[im]) / dtp) + ( (Ys[im] - Ys[im-1]) / dtm) );
      error = 0.5*std::sqrt( ( (std::pow(Es[im+1], 2) + std::pow(Es[im], 2) ) / std::pow(dtp, 2) ) + (( std::pow(Es[im], 2) + std::pow(Es[im-1], 2) ) / std::pow(dtm, 2) )
        - 2.0*std::pow(Es[im], 2)/(dtp*dtm) );
      im--;
    }
    im++;
    if (deriv > error) im = 0; //  derivative criterion not met
  }
  double pk_min = Xs[im];
  double pk_max = Xs[ip];
  double pk_width = Xs[ip] - Xs[im];

  // Determine background from either side of peak.
  // At this point, im and ip define the extreme points of the peak
  // Assume flat background
  double bkgd = 0.0;
  double bkgd_range = 0.0;
  double bkgd_min = std::max(Xs.front(), pk_min - m_bkgd_frac*pk_width);
  double bkgd_max = std::min(Xs.back(), pk_max + m_bkgd_frac*pk_width);

  if (im > 0)
  {
    double bkgd_m, bkgd_err_m;
    integrate(bkgd_m, bkgd_err_m, Xs, Ys, Es, bkgd_min, pk_min);
    bkgd = bkgd + bkgd_m;
    bkgd_range = bkgd_range + (pk_min - bkgd_min);
  }

  if (ip < nxvals - 1)
  {
    double bkgd_p, bkgd_err_p;
    integrate(bkgd_p, bkgd_err_p, Xs, Ys, Es, pk_max, bkgd_max);
    bkgd = bkgd + bkgd_p;
    bkgd_range = bkgd_range + (bkgd_max - pk_max);
  }

  if ( im > 0  || ip < nxvals - 1 ) //background from at least one side
  {
    bkgd = bkgd / bkgd_range;
  }
  // Perform moment analysis on the peak after subtracting the background
  // Fill arrays with peak only:
  const std::vector<double>::size_type nvalues = ip - im + 1;
  peak_x.resize(nvalues);
  std::copy( Xs.begin() + im, Xs.begin() + ip + 1, peak_x.begin());
  peak_y.resize(nvalues);
  std::transform( Ys.begin() + im, Ys.begin() + ip + 1, peak_y.begin(), std::bind2nd(std::minus<double>(),bkgd) );
  peak_e.resize(nvalues);
  std::copy( Es.begin()+im, Es.begin() + ip + 1, peak_e.begin());

  // FWHH:
  int64_t ipk_int = static_cast<int64_t>(iPeak) - im;  //       ! peak position in internal array
  double hby2 = 0.5*peak_y[ipk_int];
  int64_t ip1(0), ip2(0);
  double xp_hh(0);

  int64_t nyvals = static_cast<int64_t>(peak_y.size());
  if (peak_y[nyvals-1] < hby2)
  {
    for( int64_t i = ipk_int; i < nyvals;  ++i )
    {
      if (peak_y[i] < hby2)
      {
        // after this point the intensity starts to go below half-height
        ip1 = i-1;       
        break;
      }
    }
    for ( int64_t i = nyvals-1; i >= ipk_int; --i )
    {
      if (peak_y[i] > hby2)
      {
        ip2 = i+1;           //   ! point closest to peak after which the intensity is always below half height
        break;
      }
    }
    // A broad peak with many local maxima on the side can cause the algorithm to give the same indices
    // for the two points either side of the half-width point. We know the algorithm isn't perfect so
    // move one index such that there is at least a gap.
    if( ip1==ip2 )
    {
      g_log.warning() << "A peak with a local maxima on the trailing edge has been found. The estimation of the "
                      << "half-height point will not be as accurate.\n";
      ip1--;
    }
    xp_hh = peak_x[ip2] + (peak_x[ip1]-peak_x[ip2])*((hby2-peak_y[ip2])/(peak_y[ip1]-peak_y[ip2]));
  }
  else
  {
    xp_hh = peak_x[nyvals-1];
  }

  int64_t im1(0), im2(0);
  double xm_hh(0);
  if (peak_y[0]<hby2)
  {
    for( int64_t i = ipk_int; i >= 0; --i )
    {
      if (peak_y[i]<hby2)
      {
        im1 = i+1;   // ! after this point the intensity starts to go below half-height
        break;
      }
    }
    for ( int64_t i=0; i <= ipk_int; ++i )
    {
      if (peak_y[i]>hby2)
      {
        im2 = i-1;   // ! point closest to peak after which the intensity is always below half height
        break;
      }
    }
    // A broad peak with many local maxima on the side can cause the algorithm to give the same indices
    // for the two points either side of the half-width point. We know the algorithm isn't perfect so
    // move one index such that there is at least a gap.
    if( im1==im2 )
    {
      g_log.warning() << "A peak with a local maxima on the rising edge has been found. The estimation of the "
                      << "half-height point will not be as accurate.\n";
      im1++;
    }
    xm_hh = peak_x[im2] + (peak_x[im1]-peak_x[im2])*((hby2-peak_y[im2])/(peak_y[im1]-peak_y[im2]));
  }
  else
  {
    xm_hh = peak_x.front();
  }
  return (xp_hh - xm_hh);
}

/** Calculate the first moment of the given workspace
 *  @param monitor_ws :: The workspace containing a single spectrum to work on
 *  @param prominence :: The factor over the background by which a peak is to be considered a "real" peak
 *  @return The calculated first moment
 */
double GetEi2::calculateFirstMoment(API::MatrixWorkspace_sptr monitor_ws, const double prominence)
{
  std::vector<double> peak_x, peak_y, peak_e;
  calculatePeakWidthAtHalfHeight(monitor_ws, prominence, peak_x, peak_y, peak_e);

  // Area
  double area(0.0), dummy(0.0);
  double pk_xmin = peak_x.front();
  double pk_xmax = peak_x.back();
  integrate(area, dummy, peak_x, peak_y, peak_e, pk_xmin, pk_xmax);
  // First moment
  std::transform(peak_y.begin(), peak_y.end(), peak_x.begin(), peak_y.begin(), std::multiplies<double>());
  double xbar(0.0);
  integrate(xbar, dummy, peak_x, peak_y, peak_e, pk_xmin, pk_xmax);
  return xbar / area;
}

/**
 * Rebin the workspace using the given bin widths 
 * @param monitor_ws
::  * @param first :: The minimum value for the new bin range
 * @param width :: The new bin width
 * @param end :: The maximum value for the new bin range
 * @returns The rebinned workspace
*/
API::MatrixWorkspace_sptr GetEi2::rebin(API::MatrixWorkspace_sptr monitor_ws, const double first, const double width, const double end)
{
  IAlgorithm_sptr childAlg = createSubAlgorithm("Rebin");
  childAlg->setProperty("InputWorkspace", monitor_ws);
  std::ostringstream binParams;
  binParams << first << "," << width << "," << end;
  childAlg->setPropertyValue( "Params", binParams.str());
  childAlg->executeAsSubAlg();
  return childAlg->getProperty("OutputWorkspace");
}

/**
 * Integrate a point data set
 * @param integral_val :: The result of the integral
 * @param integral_err :: The error value
 * @param x :: The X values
 * @param s :: The Y values
 * @param e :: The E values
 * @param xmin :: The minimum value for the integration
 * @param xmax :: The maximum value for the integration
 */
void GetEi2::integrate(double & integral_val, double &integral_err, const Mantid::MantidVec &x, const Mantid::MantidVec &s, const Mantid::MantidVec &e, 
                       const double xmin, const double xmax) const
{
  // MG: Note that this is integration of a point data set from libisis
  // @todo: Move to Kernel::VectorHelper and improve performance

  MantidVec::const_iterator lowit = std::lower_bound(x.begin(), x.end(), xmin);
  MantidVec::difference_type ml = std::distance(x.begin(),lowit);
  MantidVec::const_iterator highit = std::upper_bound(lowit,x.end(), xmax);
  MantidVec::difference_type mu = std::distance(x.begin(),highit);
  if( mu > 0 ) --mu;

  MantidVec::size_type nx(x.size());
  if( mu < ml )
  {
    //special case of no data points in the integration range
    unsigned int ilo = std::max<unsigned int>((unsigned int)ml - 1, 0);
    unsigned int ihi = std::min<unsigned int>((unsigned int)mu + 1, (unsigned int)nx);
    double fraction = (xmax - xmin)/(x[ihi] - x[ilo]);
    integral_val = 0.5 * fraction * 
      ( s[ihi]*((xmax - x[ilo]) + (xmin - x[ilo])) + s[ilo]*((x[ihi] - xmax)+(x[ihi] - xmin)) );
    double err_hi = e[ihi]*((xmax - x[ilo]) + (xmin - x[ilo]));
    double err_lo = e[ilo]*((x[ihi] - xmax)+(x[ihi] - xmin));
    integral_err = 0.5*fraction*std::sqrt( err_hi*err_hi  + err_lo*err_lo );
    return;
  }

  double x1eff(0.0), s1eff(0.0), e1eff(0.0);
  if( ml > 0 )
  {
    x1eff = (xmin*(xmin - x[ml-1]) + x[ml-1]*(x[ml] - xmin))/(x[ml] - x[ml-1]);
    double fraction = (x[ml]-xmin) / ((x[ml]-x[ml-1]) + (xmin - x[ml-1]));
    s1eff = s[ml-1]* fraction;
    e1eff = e[ml-1]* fraction;
  }
  else
  {
    x1eff = x[ml];
    s1eff = 0.0;
  }

  double xneff(0.0), sneff(0.0), eneff;
  if( mu < static_cast<int>(nx - 1))
  {
    xneff = (xmax*(x[mu+1]-xmax) + x[mu+1]*(xmax - x[mu]))/(x[mu+1] - x[mu]);
    const double fraction = (xmax - x[mu])/((x[mu+1] - x[mu]) + (x[mu+1]-xmax));
    sneff = s[mu+1]*fraction;
    eneff = e[mu+1]*fraction;
  }
  else
  {
    xneff = x.back();
    sneff = 0.0;
    eneff = 0.0; //Make sure to initialize to a value.
  }

  //xmin -> x[ml]
  integral_val = (x[ml] - x1eff)*(s[ml] + s1eff);
  integral_err = e1eff*(x[ml] - x1eff);
  integral_err *= integral_err;

  if( mu == ml )
  {
    double ierr = e[ml]*(xneff - x1eff);
    integral_err += ierr * ierr;
  }
  else if( mu == ml + 1 )
  {
    integral_val += (s[mu] + s[ml])*(x[mu] - x[ml]);
    double err_lo = e[ml]*(x[ml+1] - x1eff);
    double err_hi = e[mu]*(x[mu-1] - xneff);
    integral_err += err_lo*err_lo + err_hi*err_hi;
  }
  else
  {
    for( int i = (int)ml; i < mu; ++i )
    {
      integral_val += (s[i+1] + s[i])*(x[i+1] - x[i]);
      if( i < mu - 1 )
      {
        double ierr = e[i+1]*(x[i+2] - x[i]);
        integral_err += ierr * ierr;
      }
    }
  }

  //x[mu] -> xmax
  integral_val += (xneff - x[mu])*(s[mu] + sneff);
  double err_tmp = eneff*(xneff - x[mu]);
  integral_err += err_tmp*err_tmp;

  integral_val *= 0.5;
  integral_err = 0.5*sqrt(integral_err);
}

/**
 * Store the value of Ei wihin the log data on the Sample object
 * @param ei :: The value of the incident energy of the neutron
 */
void GetEi2::storeEi(const double ei) const
{
  Property *incident_energy = new PropertyWithValue<double>("Ei",ei,Direction::Input);
  m_input_ws->mutableRun().addProperty(incident_energy, true);
}

}
}
