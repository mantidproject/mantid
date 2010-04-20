#include "MantidAlgorithms/GetEi2.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileProperty.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidKernel/Exception.h" 
#include "MantidKernel/VectorHelper.h" 

#include <boost/lexical_cast.hpp>

#include <cmath>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace Mantid
{
  namespace Algorithms
  {

    // Register the algorithm into the algorithm factory
    DECLARE_ALGORITHM(GetEi2)

    using namespace Kernel;
    using namespace API;
    using namespace Geometry;
    using namespace DataObjects;

    // @todo: Move to statics on the class
    static const double MON_TOF_WIN=0.1;
    static const double SFAC_PEAK = 2.0, SFAC_DERIV=1.0;
    static const int NPOINTS=8;
    static const double BKGD_FAC=0.5;
    static const double NO_ESTIMATE=-1e200;

    // progress estimates
    const double GetEi2::CROP = 0.15;
    const double GetEi2::GET_COUNT_RATE = 0.15;
    const double GetEi2::FIT_PEAK = 0.2;


    void GetEi2::init()

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
    void GetEi2::exec()
    {
      Workspace2D_const_sptr inWS = getProperty("InputWorkspace");
      const int monitor1_spec = getProperty("Monitor1Spec");
      const int monitor2_spec = getProperty("Monitor2Spec");

      //Covert spectrum numbers to workspace indices
      std::vector<int> spec_nums(2, monitor1_spec);
      spec_nums[1] = monitor2_spec;
      m_mon_indices.clear();
      // get the index number of the histogram for the first monitor
      WorkspaceHelpers::getIndicesFromSpectra(inWS, spec_nums, m_mon_indices);

      if( m_mon_indices.size() != 2 )
      {
        g_log.error() << "Error retrieving monitor spectra from input workspace. Check input properties.\n";
        throw std::runtime_error("Error retrieving monitor spectra spectra from input workspace.");
      }

      double dist2moni0(-1.0), dist2moni1(-1.0);
      getGeometry(inWS, dist2moni0, dist2moni1);
      g_log.debug() << "Distance between monitors = " << dist2moni0 - dist2moni1 << " m\n";

      // the E_i estimate is used to find (identify) the monitor peaks, checking prior to fitting will throw an exception if this estimate is too big or small
      const double E_est = getProperty("EnergyEstimate");
      double peakLoc0(0.0);
      if( E_est != EMPTY_DBL() )
      {
        peakLoc0 = 1e6*timeToFly(dist2moni0, E_est);
      }

      g_log.information() << "Looking for a peak in the first monitor spectrum, workspace index " << m_mon_indices[0] << std::endl;
      double t_monitor0 = getPeakCentre(inWS, 1, peakLoc0);
      g_log.notice() << "The first peak has been found at TOF = " << t_monitor0 << " microseconds\n";
      setProperty("FirstMonitorPeak", t_monitor0);

      g_log.information() << "Looking for a peak in the second monitor spectrum, workspace index " << m_mon_indices[1] << std::endl;
      const double peakLoc1 = 1e6*timeToFly(dist2moni1, E_est);
      double t_monitor1 = getPeakCentre(inWS, 2, peakLoc1);
      g_log.information() << "The second peak has been found at TOF = " << t_monitor1 << " microseconds\n";

      // assumes that the source and the both mintors lie on one straight line, the 1e-6 converts microseconds to seconds as the mean speed needs to be in m/s
      double meanSpeed = 1e6 * (dist2moni1 - dist2moni0)/ (t_monitor1 - t_monitor0);

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
    *  @param monitor0Dist the calculated distance to the detector whose ID was passed to this function first
    *  @param monitor1Dist calculated distance to the detector whose ID was passed to this function second
    *  @throw NotFoundError if no detector is found for the detector ID given
    *  @throw runtime_error if there is a problem with the SpectraDetectorMap
    */
    void GetEi2::getGeometry(DataObjects::Workspace2D_const_sptr WS, double &monitor0Dist, double &monitor1Dist) const
    {
      const IObjComponent_sptr source = WS->getInstrument()->getSource();

      // retrieve a pointer to the first monitor and get its distance
      IDetector_sptr det = WS->getDetector(m_mon_indices.front());
      if( !det )
      {
        g_log.error() << "A detector for monitor at workspace index " << m_mon_indices.front() << " cannot be found. ";
        throw std::runtime_error("No detector found for the first monitor.");
      }
      if( boost::dynamic_pointer_cast<DetectorGroup>(det) )
      {
        g_log.error() << "The detector for spectrum number " << m_mon_indices.front() << " is a group, grouped monitors are not supported by this algorithm\n";
        g_log.error() << "Error retrieving data for the first monitor" << std::endl;
        throw std::runtime_error("Detector for first monitor is a DetectorGroup.");
      }
      monitor0Dist = det->getDistance(*source);
      g_log.information() << "L1 distance " << monitor0Dist << " m.\n";

      // second monitor
      det = WS->getDetector(m_mon_indices[1]);
      if( !det )
      {
        g_log.error() << "A detector for monitor at workspace index " << m_mon_indices[1] << " cannot be found. ";
        throw std::runtime_error("No detector found for the second monitor.");
      }
      if( boost::dynamic_pointer_cast<DetectorGroup>(det) )
      {
        g_log.error() << "The detector for spectrum number " << m_mon_indices.front() << " is a group, grouped monitors are not supported by this algorithm\n";
        g_log.error() << "Error retrieving data for the second monitor" << std::endl;
        throw std::runtime_error("Detector for second monitor is a DetectorGroup.");
      }
      monitor1Dist = det->getDistance(*source);
      g_log.information() << "L2 distance " << monitor1Dist << " m.\n";
    }

    /** Uses E_KE = mv^2/2 and s = vt to calculate the time required for a neutron
    *  to travel a distance, s
    * @param s ditance travelled in meters
    * @param E_KE kinetic energy in meV
    * @return the time to taken to travel that uninterrupted distance in seconds
    */
    double GetEi2::timeToFly(double s, double E_KE) const
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
    *  @param peak First or second monitor peak. 
    *  @param peakTime the estimated TOF of the monitor peak in the time units of the workspace
    *  @return a time of flight value in the peak in microseconds
    *  @throw invalid_argument if a good peak fit wasn't made or the input workspace does not have common binning
    *  @throw out_of_range if the peak runs off the edge of the histogram
    *  @throw runtime_error a sub-algorithm just falls over
    */
    double GetEi2::getPeakCentre(API::MatrixWorkspace_const_sptr WS, const int peak, const double peakTime)
    {
      double tMin(0.0), tMax(0.0);

      if ( peakTime > 0.0 )
      {
        tMin = (1 - MON_TOF_WIN)*peakTime;
        tMax = (1 + MON_TOF_WIN)*peakTime;
        switch (peak)
        {
        case 1:
          g_log.information() << "Based on the user selected energy the first peak will be searched for at TOF " << peakTime << " micro seconds +/-" << boost::lexical_cast<std::string>(100.0*MON_TOF_WIN) << "%\n";
          break;
        case 2:
          g_log.information() << "Based on the user selected energy the second peak will be searched for at TOF " << peakTime << " micro seconds +/-" << boost::lexical_cast<std::string>(100.0*MON_TOF_WIN) << "%\n";
          break;
        default:
          throw std::runtime_error("Invalid monitor selected");
        }
      }
      else
      {
        tMin = 400;
        tMax = 12000;
        g_log.information() << "No energy estimate given, using default window t0 = " << tMin << " microseconds, t1 = " << tMax << " microseconds\n";  
      }
      // runs CropWorkspace as a sub-algorithm to and puts the result in a new temporary workspace that will be deleted when this algorithm has finished
      extractSpec(m_mon_indices[peak-1], tMin, tMax);
      // converting the workspace to count rate is required by the fitting algorithm if the bin widths are not all the same, if the workspace is already a distribution this does nothing
      WorkspaceHelpers::makeDistribution(m_tempWS);

      return getPeakFirstMoments(m_tempWS, tMin, tMax);
    }
    /** Calls CropWorkspace as a sub-algorithm and passes to it the InputWorkspace property
    *  @param specInd the index number of the histogram to extract
    *  @param start the number of the first bin to include (starts counting bins at 0)
    *  @param end the number of the last bin to include (starts counting bins at 0)
    *  @throw out_of_range if start, end or specInd are set outside of the vaild range for the workspace
    *  @throw runtime_error if the algorithm just falls over
    *  @throw invalid_argument if the input workspace does not have common binning
    */
    void GetEi2::extractSpec(int specInd, double start, double end)
    {
      IAlgorithm_sptr childAlg =
        createSubAlgorithm("CropWorkspace", 100*m_fracCompl, 100*(m_fracCompl+CROP) );
      m_fracCompl += CROP;

      childAlg->setPropertyValue( "InputWorkspace",
        getPropertyValue("InputWorkspace") );
      childAlg->setProperty( "StartWorkspaceIndex", specInd);
      childAlg->setProperty( "EndWorkspaceIndex", specInd);

      if( start > 0 )
      {
        childAlg->setProperty( "XMin", start);
        childAlg->setProperty( "XMax", end);
      }

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

      progress(m_fracCompl);
      interruption_point();
    }

    /** Implements the Fortran subroute IXFmoments_dataset_2d() from the libISIS
    */
    double GetEi2::getPeakFirstMoments(API::MatrixWorkspace_sptr WS, const double tMin, const double tMax)
    {
      // @todo Original FORTRAN uses an unspike algorithm which we don't currently have
      double prominence = 4.0;                    
      int nvals = WS->dataY(0).size();
      MantidVec centredXs(nvals, 0.0);

      for( int i = 1; i <= nvals; i++ )
      {
        centredXs[i - 1] = (WS->readX(0)[i] + WS->readX(0)[i - 1])/2;
      }

      double A_M = 0, c = 0, c_fwhm = 0, w = 0, T_Mean = 0;
      getPeakMean(centredXs, WS->readY(0), WS->readE(0), prominence, A_M, c, c_fwhm, w, T_Mean);
      const double bmin = w/(1.5*NPOINTS);

      if (c_fwhm <= 0.0)
      {
        throw std::invalid_argument("No peak found, check tMin, tMax and the Monitor index");
      }

      WS = rebin(WS, tMin, bmin, tMax);
      
      nvals = WS->dataY(0).size();
      centredXs.resize(nvals, 0.0);

      for( int i = 1; i <= nvals; i++ )
      {
        centredXs[i - 1] = (WS->readX(0)[i] + WS->readX(0)[i - 1])/2;
      }
      //! call get moments again, prominence=4 still
      try {
        T_Mean = 0.0;
        getPeakMean(centredXs, WS->readY(0), WS->readE(0), prominence, A_M, c, c_fwhm, w, T_Mean);
      }
      catch(std::invalid_argument &)
      {
        prominence = 2.0;
        T_Mean = 0.0;
        getPeakMean(centredXs, WS->readY(0), WS->readE(0), prominence, A_M, c, c_fwhm, w, T_Mean);
      }

      if ((c == 0.0) || (w > (0.2*c_fwhm)))
      {
        throw std::invalid_argument("no valid peak found, check initial tMin, tMax and the Monitor index");
      }
      return T_Mean;
    }

    API::MatrixWorkspace_sptr GetEi2::rebin(API::MatrixWorkspace_sptr WS, const double first, const double width, const double end)
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

    void GetEi2::getPeakMean(const MantidVec& Xs, const MantidVec& Ys, const MantidVec& Es, const double prominence, double &area, double &c, double &c_fwhm, double &w, double &xbar)
    {
      MantidVec::const_iterator peakIt = std::max_element(Ys.begin(), Ys.end());    //! position of peak
      unsigned int iPeak = peakIt - Ys.begin();
      double peakY = Ys[iPeak];
      double peakE = Es[iPeak];

      const unsigned int nxvals = Xs.size();

      //! Find data range that satisfies prominence criterion: im < ipk < ip will be nearest points that satisfy this
      int im = iPeak-1;
      for( ; im >= 0; --im )
      {
        const double ratio = Ys[im]/peakY;
        const double ratio_err =
          std::sqrt( std::pow(Es[im],2) + std::pow(ratio*peakE,2) )/peakY;
        if ( ratio < (1.0/prominence - SFAC_PEAK*ratio_err) )
        {
          break;
        }
      }
      if( im < 0 ) im = 0;

      int ip = iPeak+1;
      for( ; ip < nxvals; ip++ )
      {
        const double ratio = Ys[ip]/peakY;
        const double ratio_err =
          std::sqrt( std::pow(Es[ip], 2) + std::pow(ratio*peakE, 2) )/peakY;
        if ( ratio < (1.0/prominence - SFAC_PEAK*ratio_err) )
        {
          break;
        }
      }
      if( ip == nxvals ) --ip;

      if ( ip < nxvals && im >= 0 )  //  ! peak in data
      {
        c = Xs[iPeak];
      }
      else
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
        while ( ( ip < nxvals - 1 ) && ( deriv < -SFAC_DERIV*error ) )
        {
          double dtp = Xs[ip+1] - Xs[ip];
          double dtm = Xs[ip] - Xs[ip-1];
          deriv = 0.5*( ((Ys[ip+1] - Ys[ip]) / dtp) + ((Ys[ip] - Ys[ip-1]) / dtm) );
          error = 0.5*std::sqrt( ( (std::pow(Es[ip+1], 2) + std::pow(Es[ip], 2) ) / std::pow(dtp,2) ) + ((std::pow(Es[ip], 2) + std::pow(Es[ip-1], 2) )/std::pow(dtm,2) )
            - 2.0*(std::pow(Es[ip], 2) / (dtp*dtm)) );
          ip = ip + 1;
        }
        ip = ip - 1;

        if (deriv < -error)
        {
          ip = nxvals -1;      //        ! derivative criterion not met
        }
      }

      if (im > 0)
      {
        double deriv = 1000.0;
        double error = 0.0;
        while ( (im > 0) && (deriv > SFAC_DERIV*error) )
        {
          double dtp = Xs[im+1] - Xs[im];
          double dtm = Xs[im] - Xs[im-1];
          deriv = 0.5*( ((Ys[im+1] - Ys[im]) / dtp) + ( (Ys[im] - Ys[im-1]) / dtm) );
          error = 0.5*std::sqrt( ( (std::pow(Es[im+1], 2) + std::pow(Es[im], 2) ) / std::pow(dtp, 2) ) + (( std::pow(Es[im], 2) + std::pow(Es[im-1], 2) ) / std::pow(dtm, 2) )
            - 2.0*std::pow(Es[im], 2)/(dtp*dtm) );
          im = im - 1;
        }
        im = im + 1;
        if (deriv > error) im = 0;//        ! derivative criterion not met
      }
      double pk_min = Xs[im];
      double pk_max = Xs[ip];
      double pk_width = Xs[ip] - Xs[im];

      // Determine background from either side of peak.
      // At this point, im and ip define the extreme points of the peak
      // Assume flat background
      double bkgd = 0.0;
      double bkgd_range = 0.0;
      double bkgd_min = std::max(Xs.front(), pk_min - BKGD_FAC*pk_width);
      double bkgd_max = std::min(Xs.back(), pk_max + BKGD_FAC*pk_width);

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
      std::vector<double> xint(ip-im+1);
      std::copy( Xs.begin()+im, Xs.begin()+ip+1, xint.begin());
      std::vector<double> yint(ip-im+1);
      std::transform( Ys.begin()+im, Ys.begin()+ip+1, yint.begin(), std::bind2nd(std::minus<double>(),bkgd) );
      std::vector<double> eint(ip-im+1);
      std::copy( Es.begin()+im, Es.begin()+ip+1, eint.begin());

      // !   FWHH:
      int ipk_int = iPeak-im;  //       ! peak position in internal array
      double hby2 = 0.5*yint[ipk_int];
      int ip1(0), ip2(0);
      double xp_hh(0);
 
      const int nyvals = yint.size();     
      if (yint[nyvals-1]<hby2)
      {
        for( int i = ipk_int; i < nyvals;  ++i )
        {
          if (yint[i] < hby2)
          {
             // after this point the intensity starts to go below half-height
            ip1 = i-1;       
            break;
          }
        }
        for ( int i = nyvals-1; i >= ipk_int; --i )
        {
          if (yint[i]>hby2)
          {
            ip2 = i+1;           //   ! point closest to peak after which the intensity is always below half height
            break;
          }
        }
        xp_hh = xint[ip2] + (xint[ip1]-xint[ip2])*((hby2-yint[ip2])/(yint[ip1]-yint[ip2]));
      }
      else
      {
        xp_hh = xint[nyvals-1];
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

    void GetEi2::integrate(double & integral_val, double &integral_err, const MantidVec &x, const MantidVec &s, const MantidVec &e, const double xmin, const double xmax)
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
        unsigned int ilo = std::max<unsigned int>(ml-1,0);
        unsigned int ihi = std::min<unsigned int>(mu+1,nx);
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
      if( mu < nx - 1)
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
        for( int i = ml; i < mu; ++i )
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

    /** Get the kinetic energy of a neuton in joules given it speed using E=mv^2/2
    *  @param speed the instantanious speed of a neutron in metres per second
    *  @return the energy in joules
    */
    double GetEi2::neutron_E_At(double speed) const
    {
      // E_KE = mv^2/2
      return PhysicalConstants::NeutronMass*speed*speed/(2);
    }

    /// Update the percentage complete estimate assuming that the algorithm has completed a task with estimated RunTime toAdd
    void GetEi2::advanceProgress(double toAdd)
    {
      m_fracCompl += toAdd;
      progress(m_fracCompl);
      // look out for user cancel messgages
      interruption_point();
    }


  } // namespace Algorithms
} // namespace Mantid
