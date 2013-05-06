#include "MantidAPI/Algorithm.h"
#include "MantidCurveFitting/LeBailFunction.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/Fit.h"

#include <sstream>

#include <gsl/gsl_sf_erf.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;

using namespace std;

const double NEG_DBL_MAX(-1.*DBL_MAX);

namespace Mantid
{
namespace CurveFitting
{
  namespace
  {
    const int PEAKRADIUS = 8;
    const double PEAKRANGECONSTANT = 5.0;
  }

  // Get a reference to the logger
  Mantid::Kernel::Logger& LeBailFunction::g_log = Mantid::Kernel::Logger::get("LeBailFunction");

  //----------------------------------------------------------------------------------------------
  /** Constructor
    */
  LeBailFunction::LeBailFunction()
  {
    CompositeFunction_sptr m_function(new CompositeFunction());
    m_compsiteFunction = m_function;

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
    */
  LeBailFunction::~LeBailFunction()
  {
    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Return the composite function
    */
  API::IFunction_sptr LeBailFunction::getFunction()
  {
    return m_compsiteFunction;
    // return boost::dynamic_pointer_cast<IFunction_sptr>(m_compsiteFunction);
  }

  //----------------------------------------------------------------------------------------------
  /** Calculate powder diffraction pattern by Le Bail algorithm
    * @param out :: output vector
    * @param xvalues :: input vector
    */
  void LeBailFunction::function(std::vector<double>& out, const std::vector<double>& xvalues) const
  {
    throw runtime_error("Implement LeBailFunction::function() ASAP!");

  }

  //----------------------------------------------------------------------------------------------
  /** Check whether a parameter is a profile parameter
    * @param parammane :: parameter name to check with
   */
  bool LeBailFunction::hasProfileParameter(std::string paramname)
  {
    vector<string>::iterator fiter = find(m_orderedProfileParameterNames.begin(), m_orderedProfileParameterNames.end(),
                                          paramname);

    return (fiter != m_orderedProfileParameterNames.end());
  }

  //----------------------------------------------------------------------------------------------
  /** Check whether the newly set parameters are correct, i.e., all peaks are physical
    * This function would be used with setParameters() and etc.
    */
  bool LeBailFunction::isParameterCorrect() const
  {
    throw runtime_error("Implement LeBailFunction::isParameterCorrect ASASP!");

    return false;
  }


  //----------------------------------------------------------------------------------------------
  /** Generate peaks, and add them to this composite function
    * @param peakhkls :: list of Miller indexes (HKL)
   */
  void LeBailFunction::addPeaks(std::vector<std::vector<int> > peakhkls)
  {
#if 0
    double lattice = getParameter("LatticeConstant");
#endif

    for (size_t ipk = 0; ipk < peakhkls.size(); ++ ipk)
    {
      // Check input Miller Index
      if (peakhkls[ipk].size() != 3)
      {
        stringstream errss;
        errss << "Error of " << ipk << "-th input Miller Index.  It has " << peakhkls[ipk].size()
              << " items, but not required 3 items.";
        g_log.error(errss.str());
        throw runtime_error(errss.str());
      }
      int h = peakhkls[ipk][0];
      int k = peakhkls[ipk][1];
      int l = peakhkls[ipk][2];

#if 0
      // Calculate peak position
      double peak_d = calCubicDSpace(lattice, h, k, l);
#endif

      IPowderDiffPeakFunction_sptr newpeak = generatePeak(h, k, l);
      double dsp = newpeak->getPeakParameter("dspace");

      // Add peak
      // addPeak(peak_d);
      m_peakvec.push_back(newpeak);
      m_dspPeakVec.push_back(make_pair(dsp, newpeak));
      m_peakHKLVec.push_back(peakhkls[ipk]);
    }

    return;
  } // END of addPeaks()


  //----------------------------------------------------------------------------------------------
  /** Generate a peak with parameter set by
    * @param h :: H
    * @param k :: K
    * @param l :: L
    */
  IPowderDiffPeakFunction_sptr LeBailFunction::generatePeak(int h, int k, int l)
  {
    IPowderDiffPeakFunction_sptr peak = boost::dynamic_pointer_cast<IPowderDiffPeakFunction>(
          FunctionFactory::Instance().create("ThermalNeutronBk2BkExpConvPVoigt"));

    peak->setMillerIndex(h, k, l);
    for (size_t i = 0; i < m_peakParameterNameVec.size(); ++i)
    {
      string parname = m_peakParameterNameVec[i];
      double parvalue = m_functionParameters[parname];
      peak->setParameter(parname, parvalue);
    }

    return peak;
  }


  //----------------------------------------------------------------------------------------------
  /** Calculate peak heights from the model to the observed data
  * Algorithm will deal with
  * (1) Peaks are close enough to overlap with each other
  * The procedure will be
  * (a) Assign peaks into groups; each group contains either (1) one peak or (2) peaks overlapped
  * (b) Calculate peak intensities for every peak per group
  *
  * @param dataws :  data workspace holding diffraction data for peak calculation
  * @param workspaceindex:  workpace index of the data for peak calculation in dataws
  * @param zerobackground:  flag if the data is zero background
  * @param allpeaksvalues:  output vector storing peaks' values calculated
  *
  * Return: True if all peaks' height are physical.  False otherwise
  */
  bool LeBailFunction::calculatePeaksIntensities(vector<double>& vecX, vector<double>& vecY, bool zerobackground, vector<double>& allpeaksvalues)
  {
    // 1. Group the peak
    vector<vector<pair<double, IPowderDiffPeakFunction_sptr> > > peakgroupvec;
    groupPeaks(peakgroupvec);

    // 2. Calculate each peak's intensity and set
    bool peakheightsphysical = true;
    for (size_t ig = 0; ig < peakgroupvec.size(); ++ig)
    {
      g_log.debug() << "[DBx351] Peak group " << ig << " : number of peaks = "
                    << peakgroupvec[ig].size() << "\n";
      bool localphysical = calculateGroupPeakIntensities(peakgroupvec[ig], vecX, vecY, zerobackground, allpeaksvalues);
      if (!localphysical)
      {
        peakheightsphysical = false;
      }
    }

    return peakheightsphysical;
  }

  //----------------------------------------------------------------------------------------------
  /** Calculate peak's intensities in a group and set the calculated peak height
   * to the corresponding peak function.
   * @param allpeaksvalues:  vector containing the peaks values.  Increment will be made on each
   *                      peak group
   * @param peakgroup:  vector of peak-centre-dpsace value and peak function pair for peaks that are overlapped
   * @param dataws:  data workspace for the peaks
   * @param wsindex: workspace index of the peaks data in dataws
   * @param zerobackground: true if background is zero
   */
  bool LeBailFunction::calculateGroupPeakIntensities(vector<pair<double, IPowderDiffPeakFunction_sptr> > peakgroup,
                                                     vector<double>& vecX, vector<double>& vecY, bool zerobackground,
                                                     vector<double>& allpeaksvalues)
  {
    // 1. Sort by d-spacing
    if (peakgroup.empty())
    {
      throw runtime_error("Programming error such that input peak group cannot be empty!");
    }
    else
    {
      g_log.debug() << "[DBx155] Peaks group size = " << peakgroup.size() << "\n";
    }
    if (peakgroup.size() > 1)
      sort(peakgroup.begin(), peakgroup.end());

    // Check input vector validity
    if (allpeaksvalues.size() != vecY.size())
    {
      stringstream errss;
      errss << "Input vector 'allpeaksvalues' has wrong size = " << allpeaksvalues.size()
            << " != data workspace Y's size = " << vecY.size();
      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }

    // 2. Check boundary
    IPowderDiffPeakFunction_sptr leftpeak = peakgroup[0].second;
    double leftbound = leftpeak->centre() - PEAKRANGECONSTANT * leftpeak->fwhm();
    if (leftbound < vecX[0])
    {
      g_log.information() << "Peak group's left boundary " << leftbound << " is out side of "
                          << "input data workspace's left bound (" << vecX[0]
                          << ")! Accuracy of its peak intensity might be affected.\n";
      leftbound = vecX[0] + 0.1;
    }
    IPowderDiffPeakFunction_sptr rightpeak = peakgroup.back().second;
    double rightbound = rightpeak->centre() + PEAKRANGECONSTANT * rightpeak->fwhm();
    if (rightbound > vecX.back())
    {
      g_log.information() << "Peak group's right boundary " << rightbound << " is out side of "
                          << "input data workspace's right bound (" << vecX.back()
                          << ")! Accuracy of its peak intensity might be affected.\n";
      rightbound = vecX.back() - 0.1;
    }

    // 3. Calculate calculation range to input workspace: [ileft, iright)
    vector<double>::const_iterator cviter;

    cviter = lower_bound(vecX.begin(), vecX.end(), leftbound);
    size_t ileft = static_cast<size_t>(cviter-vecX.begin());
    if (ileft > 0)
      --ileft;

    cviter = lower_bound(vecX.begin(), vecX.end(), rightbound);
    size_t iright = static_cast<size_t>(cviter-vecX.begin());
    if (iright <= vecX.size()-1)
      ++ iright;

    // 4. Integrate
    // a) Data structure to hold result
    size_t ndata = iright-ileft;
    if (ndata == 0 || ndata > iright)
    {
      stringstream errss;
      errss << "[Calcualte Peak Intensity] Group range is unphysical.  iLeft = " << ileft << ", iRight = "
            << iright << "; Number of peaks = " << peakgroup.size()
            << "; Left boundary = " << leftbound << ", Right boundary = " << rightbound
            << "; Left peak FWHM = " << leftpeak->fwhm() << ", Right peak FWHM = " << rightpeak->fwhm();
      for (size_t ipk = 0; ipk < peakgroup.size(); ++ipk)
      {
        IPowderDiffPeakFunction_sptr thispeak = peakgroup[ipk].second;
        errss << "Peak " << ipk << ":  d_h = " << peakgroup[ipk].first << ", TOF_h = " << thispeak->centre()
              << ", FWHM = " << thispeak->fwhm() << "\n";
        vector<string> peakparamnames = thispeak->getParameterNames();
        for (size_t ipar = 0; ipar < peakparamnames.size(); ++ipar)
        {
          errss << "\t" << peakparamnames[ipar] << " = " << thispeak->getParameter(peakparamnames[ipar]) << "\n";
        }
      }

      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }

    //   Partial data range
    vector<double> datax(vecX.begin()+ileft, vecX.begin()+iright);
    vector<double> datay(vecY.begin()+ileft, vecY.begin()+iright);
    if (datax.size() != ndata)
    {
      g_log.error() << "Partial peak size = " << datax.size() << " != ndata = " << ndata << "\n";
      throw runtime_error("ndata error!");
    }

    // FunctionDomain1DVector xvalues(datax);

    g_log.debug() << "[DBx356] Number of data points = " << ndata << " index from " << ileft
                  << " to " << iright << ";  Size(datax, datay) = " << datax.size() << "\n";

    vector<double> sumYs(ndata, 0.0);
    size_t numPeaks(peakgroup.size());
    vector<vector<double> > peakvalues(numPeaks);

    // b) Integrage peak by peak
    for (size_t ipk = 0; ipk < numPeaks; ++ipk)
    {
      // calculate peak function value
      IPowderDiffPeakFunction_sptr peak = peakgroup[ipk].second;
      // FunctionValues localpeakvalue(xvalues);
      vector<double> localpeakvalue(ndata, 0.0);

      // peak->function(xvalues, localpeakvalue);
      peak->function(localpeakvalue, datax);

      // check data
      size_t numbadpts(0);
      vector<double>::const_iterator localpeakvalue_end = localpeakvalue.end();
      for (auto it = localpeakvalue.begin(); it != localpeakvalue_end; ++it)
      {
        if ( (*it != 0.) && (*it < NEG_DBL_MAX || *it > DBL_MAX))
        {
          numbadpts++;
        }
      }

      // report the problem and/or integrate data
      if (numbadpts == 0)
      {
        // Data is fine.  Integrate them all
        for (size_t i = 0; i < ndata; ++i)
        {
          // If value is physical
          sumYs[i] += localpeakvalue[i];
        }
      }
      else
      {
        // Report the problem

        int h, k, l;
        peak->getMillerIndex(h, k, l);
        stringstream warnss;
        warnss << "Peak (" << h << ", " << k << ", " << l <<") has " << numbadpts << " data points whose "
               << "values exceed limit (i.e., not physical).\n";
        g_log.warning(warnss.str());
      }
      peakvalues[ipk].assign(localpeakvalue.begin(), localpeakvalue.end());
    } // For All peaks

    // 5. Calculate intensity of all peaks
    vector<double> pureobspeaksintensity(ndata);

    // a) Remove background
    if (zerobackground)
    {
      pureobspeaksintensity.assign(datay.begin(), datay.end());
    }
    else
    {
      // Non-zero background.  Remove the background
      FunctionDomain1DVector xvalues(datax);
      FunctionValues bkgdvalue(xvalues);
      m_background->function(xvalues, bkgdvalue);

      for (size_t i = 0; i < ndata; ++i)
        pureobspeaksintensity[i] = datay[i] - bkgdvalue[i];
    }

    bool peakheightsphysical = true;
    for (size_t ipk = 0; ipk < peakgroup.size(); ++ipk)
    {
      IPowderDiffPeakFunction_sptr peak = peakgroup[ipk].second;
      double intensity = 0.0;

      for (size_t i = 0; i < ndata; ++i)
      {
        double temp;
        if (sumYs[i] > 1.0E-5)
        {
          // Reasonable non-zero value
          double peaktogroupratio = peakvalues[ipk][i]/sumYs[i];
          temp = pureobspeaksintensity[i] * peaktogroupratio;
        }
        else
        {
          // SumY too smaller
          temp = 0.0;
        }
        double deltax;
        if (i == 0)
          deltax = datax[1] - datax[0];
        else
          deltax = datax[i] - datax[i-1];
        intensity += temp * deltax;
      } // for data points

      if (intensity != intensity)
      {
        // Unphysical intensity: NaN
        intensity = 0.0;
        peakheightsphysical = false;

        int h, k, l;
        peak->getMillerIndex(h, k, l);
        g_log.warning() << "Peak (" << h << ", " << k << ", " << l <<") has unphysical intensity = NaN!\n";

      }
      else if (intensity <= -DBL_MAX || intensity >= DBL_MAX)
      {
        // Unphysical intensity: NaN
        intensity = 0.0;
        peakheightsphysical = false;

        int h, k, l;
        peak->getMillerIndex(h, k, l);
        g_log.warning() << "Peak (" << h << ", " << k << ", " << l <<") has unphysical intensity = Infty!\n";
      }
      else if (intensity < 0.0)
      {
        // No negative intensity
        intensity = 0.0;
      }

      g_log.debug() << "[DBx407] Peak @ " << peak->centre() << ": Set Intensity = " << intensity << "\n";
      peak->setHeight(intensity);

      // Add peak's value to peaksvalues
      for (size_t i = ileft; i < iright; ++i)
      {
        allpeaksvalues[i] += (intensity * peakvalues[ipk][i-ileft]);
      }

    } // ENDFOR each peak

    return peakheightsphysical;
  }

  //----------------------------------------------------------------------------------------------
  /** From table/map to set parameters to an individual peak.
   * It mostly is called by function in calculation.
   * @param peak :  ThermalNeutronBk2BkExpConvPVoigt function to have parameters' value set
   * @param parammap:  map of Parameters to set to peak
   * @param peakheight: height of the peak
   * @param setpeakheight:  boolean as the option to set peak height or not.
   */
  void LeBailFunction::setPeakParameters(IPowderDiffPeakFunction_sptr peak, map<string, double > parammap,
                                         double peakheight, bool setpeakheight)
  {
    // FIXME - The best solution for speeding is to have a set of peak parameter listed in the order
    //         of peak function's parameters' indexed.  Then no need to do search anymore.

    // 1. Prepare, sort parameters by name
    std::map<std::string, double>::iterator pit;
    vector<string> peakparamnames = peak->getParameterNames();

    // 2. Apply parameters values to peak function
    for (pit = parammap.begin(); pit != parammap.end(); ++pit)
    {
      // a) Check whether the parameter is a peak parameter
      std::string parname = pit->first;
      std::vector<std::string>::iterator ifind =
          std::find(peakparamnames.begin(), peakparamnames.end(), parname);

      // b) Set parameter value
      if (ifind == peakparamnames.end())
      {
        // If not a peak profile parameter, skip
        g_log.debug() << "Parameter '" << parname << "' in input parameter table workspace "
                      << "is not for peak function " << peak->name() << ".\n";
      }
      else
      {
        // Set value
        double value = pit->second;
        peak->setParameter(parname, value);
        g_log.debug() << "LeBailFit Set " << parname << "= " << value << "\n";
      }
    } // ENDFOR: parameter iterator

    // 3. Peak height
    if (setpeakheight)
      peak->setHeight(peakheight);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** From table/map to set parameters to all peaks.
    * Peak height will not be set.
    *
    * Request: order of parameter names in m_peakParameterNameVec must be same as the order in
    *          IPowderDiffPeakFunction.
    *
    * @param parammap: map of Parameters to set to peak
   */
  void LeBailFunction::setPeaksParameters(map<std::string, double> parammap)
  {    
    // Define some variables and constants
    size_t numpeaks = m_peakvec.size();
    if (numpeaks == 0)
      throw runtime_error("Set parameters to empty peak list. ");
    size_t numparnames = m_peakParameterNameVec.size();
    map<std::string, double>::iterator pit;

    // Apply parameters values to all peaks
    for (size_t i = 0; i < numparnames; ++i)
    {
      string& parname = m_peakParameterNameVec[i];
      if (parname.compare("Height"))
      {
        // If parameter is not peak height.  Set to all peak

        // Get parameter value
        pit = parammap.find(parname);

        if (pit == parammap.end())
        {
          // Not found and then skip
          g_log.warning() << "Peak parameter " << parname
                          << "cannot be found in parameter map.\n";
        }
        else
        {
          double parvalue = pit->second;
          for (size_t ipk = 0; ipk < numpeaks; ++ipk)
            m_peakvec[ipk]->setParameter(i, parvalue);
        }
      }
    } // END of all parameters

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Group peaks together
    * @param peakgroupvec:  output vector containing peaks grouped together.
    * Disabled argument: MatrixWorkspace_sptr dataws, size_t workspaceindex,
   */
  void LeBailFunction::groupPeaks(vector<vector<pair<double, IPowderDiffPeakFunction_sptr> > >& peakgroupvec)
  {
    // Sort peaks
    if (m_numPeaks > 1)
    {
      sort(m_dspPeakVec.begin(), m_dspPeakVec.end());
    }
    else if (m_numPeaks == 0)
    {
      std::stringstream errmsg;
      errmsg << "Group peaks:  No peak is found in the peak vector. ";
      g_log.error() << errmsg.str() << "\n";
      throw std::runtime_error(errmsg.str());
    }

    // Group peaks
    //   Set up starting value
    peakgroupvec.clear();
    vector<pair<double, IPowderDiffPeakFunction_sptr> > peakgroup; // one group of peaks
    size_t ipk = 0;

    while (ipk < m_numPeaks)
    {
      // add peak to CURRENT peak group
      peakgroup.push_back(m_dspPeakVec[ipk]);

      if (ipk < m_numPeaks-1)
      {
        // Any peak but not the last (rightmost) peak

        // test whether next peak will be in a different group
        IPowderDiffPeakFunction_sptr thispeak = m_dspPeakVec[ipk].second;
        IPowderDiffPeakFunction_sptr rightpeak = m_dspPeakVec[ipk+1].second;

        double thispeak_rightbound = thispeak->centre() + PEAKRANGECONSTANT * thispeak->fwhm();
        double rightpeak_leftbound = rightpeak->centre() - PEAKRANGECONSTANT * rightpeak->fwhm();

        if (thispeak_rightbound < rightpeak_leftbound)
        {
          // this peak and its right peak are well separated.
          // finish this group by a copy
          vector<pair<double, IPowderDiffPeakFunction_sptr> > peakgroupcopy = peakgroup;
          peakgroupvec.push_back(peakgroupcopy);
          //  clear for the next group
          peakgroup.clear();
        }
        else
        {
          // this peak and its right peak are close enough to be in same group. do nothing
          ;
        }
      }
      else
      {
        // Rightmost peak.  Finish the current peak
        vector<pair<double, IPowderDiffPeakFunction_sptr> > peakgroupcopy = peakgroup;
        peakgroupvec.push_back(peakgroupcopy);
      }

      ++ ipk;
    } // ENDWHILE

    g_log.debug() << "[Calculate Peak Intensity]:  Number of Peak Groups = " << peakgroupvec.size()
                  << "\n";

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Add background function
    * @param backgroundtype :: string, type of background, such as Polynomial, Chebyshev
    * @param bkgdparmap :: map of parameter name (string) and value (double) of background function
    */
  void LeBailFunction::addBackgroundFunction(string backgroundtype, map<string, double> bkgdparmap)
  {
    size_t order = bkgdparmap.size();

    // Create background function from factory
    auto background = FunctionFactory::Instance().createFunction(backgroundtype);
    m_background = boost::dynamic_pointer_cast<BackgroundFunction>(background);

    // Set order and init
    m_background->setAttributeValue("n", int(order));
    m_background->initialize();

    // Set parameters
    map<string, double>::iterator miter;
    for (miter = bkgdparmap.begin(); miter != bkgdparmap.end(); ++miter)
    {
      string parname = miter->first;
      double parvalue = miter->second;
      m_background->setParameter(parname, parvalue);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Set up a profile parameter to fit but tied among all peaks
    * @param paramname :: name of parameter
    * @param minvalue :: lower boundary
    * @param maxvalue :: upper boundary
    */
  void LeBailFunction::setFitProfileParameter(string paramname, double minvalue, double maxvalue)
  {
    // Make ties in composition function
    for (size_t ipk = 1; ipk < m_numPeaks; ++ipk)
    {
      stringstream ss1, ss2;
      ss1 << "f" << (ipk-1) << "." << paramname;
      ss2 << "f" << ipk << "." << paramname;
      string tiepart1 = ss1.str();
      string tiepart2 = ss2.str();
      m_compsiteFunction->tie(tiepart1, tiepart2);
      g_log.debug() << "LeBailFunction::Fit(Tie) / " << tiepart1 << " / " << tiepart2 << " /\n";
    }

    // Set contrains of the parameter on any of the tied parameter.
    std::stringstream parss;
    parss << "f0." << paramname;
    string parnamef0 = parss.str();
    CurveFitting::BoundaryConstraint* bc =
        new BoundaryConstraint(m_compsiteFunction.get(), parnamef0, minvalue, maxvalue);
    m_compsiteFunction->addConstraint(bc);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Set up a parameter to be fixed
    * @param paramname :: name of parameter
    * @param paramvalue :: value of parameter to be fixed to
    */
  void LeBailFunction::setFixProfileParameter(string paramname, double paramvalue)
  {
    for (size_t ipk = 0; ipk < m_numPeaks; ++ipk)
    {
#if 1
      stringstream ss1, ss2;
      ss1 << "f" << ipk << "." << paramname;
      ss2 << paramvalue;
      string tiepart1 = ss1.str();
      string tievalue = ss2.str();
      m_compsiteFunction->tie(tiepart1, tievalue);

      g_log.debug() << "Set up tie | " << tiepart1 << " <---> " << tievalue << " | \n";

#else
      // FIXME - // TODO: Make a map between peak parameter name and index. And use fix() to replace tie
      /*--  Code prepared to replace the existing block
      ThermalNeutronBk2BkExpConvPVoigt_sptr thispeak = m_dspPeaks[ipk].second;
      size_t iparam = findIndex(thispeak, funcparam.name);
      thispeak->fix(iparam);
      --*/
#endif

    } // For each peak

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Fix all background parameters
    */
  void LeBailFunction::setFixBackgroundParameters()
  {
    size_t numbkgdparams = m_background->nParams();

    for (size_t iparam = 0; iparam < numbkgdparams; ++iparam)
      m_background->fix(iparam);

#if 0
    original code just for backup

    std::vector<std::string> bkgdparnames = m_background->getParameterNames();
    for (size_t ib = 0; ib < bkgdparnames.size(); ++ib)
    {
      std::string parname = bkgdparnames[ib];
      double parvalue = m_background->getParameter(parname);
      std::stringstream ss1, ss2;
      ss1 << "f" << funcindex << "." << parname;
      ss2 << parvalue;
      std::string tiepart1 = ss1.str();
      std::string tievalue = ss2.str();

      g_log.debug() << "Step 2: LeBailFit.  Tie / " << tiepart1 << " / " << tievalue << " /\n";

      m_compsiteFunction->tie(tiepart1, tievalue);
    }
#endif

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Fix all peaks' intensity/height
    */
  void LeBailFunction::setFixPeakHeights()
  {
    for (size_t ipk = 0; ipk < m_numPeaks; ++ipk)
    {
      // a. Get peak height
      IPowderDiffPeakFunction_sptr thispeak = m_dspPeakVec[ipk].second;
      thispeak->fix(0);
    } // For each peak

    return;
  }



  /*
   * Calculate peak parameters for a peak at d (d-spacing value)
   * Output will be written to parameter map too.

  void LeBailFunction::calPeakParametersForD(double dh, double& alpha, double& beta, double &Tof_h,
      double &sigma_g2, double &gamma_l, std::map<std::string, double>& parmap) const
  {
    // 1. Get some parameters
    double wcross = getParameter("Width");
    double Tcross = getParameter("Tcross");

    // 2. Start to calculate alpha, beta, sigma2, gamma,
    double n = 0.5*gsl_sf_erfc(wcross*(Tcross-1/dh));

    double alpha_e = Alph0 + Alph1*dh;
    double alpha_t = Alph0t - Alph1t/dh;
    alpha = 1/(n*alpha_e + (1-n)*alpha_t);

    double beta_e = Beta0 + Beta1*dh;
    double beta_t = Beta0t - Beta1t/dh;
    beta = 1/(n*beta_e + (1-n)*beta_t);

    double Th_e = Zero + Dtt1*dh;
    double Th_t = Zerot + Dtt1t*dh - Dtt2t/dh;
    Tof_h = n*Th_e + (1-n)*Th_t;

    sigma_g2 = Sig0 + Sig1*std::pow(dh, 2) + Sig2*std::pow(dh, 4);
    gamma_l = Gam0 + Gam1*dh + Gam2*std::pow(dh, 2);

    // 3. Add to parameter map
    parmap.insert(std::make_pair("Alpha", alpha));
    parmap.insert(std::make_pair("Beta", beta));
    parmap.insert(std::make_pair("Sigma2", sigma_g2));
    parmap.insert(std::make_pair("Gamma", gamma_l));
    parmap.insert(std::make_pair("TOF_h", Tof_h));

    g_log.debug() << "DB1214 D = " << dh << ", TOF = " << Tof_h << std::endl;

    return;
  }
     */

  /*
   * Calculate all peaks' parameters
  void LeBailFunction::calPeaksParameters()
  {
    // 1. Get parameters (class)
    Alph0 = getParameter("Alph0");
    Alph1 = getParameter("Alph1");
    Beta0 = getParameter("Beta0");
    Beta1 = getParameter("Beta1");
    Alph0t = getParameter("Alph0t");
    Alph1t = getParameter("Alph1t");
    Beta0t = getParameter("Beta0t");
    Beta1t = getParameter("Beta1t");
    Dtt1 = getParameter("Dtt1");
    Dtt1t = getParameter("Dtt1t");
    Dtt2t = getParameter("Dtt2t");
    Zero = getParameter("Zero");
    Zerot = getParameter("Zerot");
    Sig0 = getParameter("Sig0");
    Sig1 = getParameter("Sig1");
    Sig2 = getParameter("Sig2");
    Gam0 = getParameter("Gam0");
    Gam1 = getParameter("Gam1");
    Gam2 = getParameter("Gam2");

    // 2. Calcualte peak parameters for all peaks
    for (size_t id = 0; id < dvalues.size(); ++id)
    {
      double dh = dvalues[id];
      // a) Calculate all the parameters
      double alpha, beta, tof_h, sigma2, gamma;
      calPeakParametersForD(dh, alpha, beta, tof_h, sigma2, gamma, mPeakParameters[id]);

      // b) Set peak parameters
      m_peakVec[id]->setParameter("TOF_h", tof_h);
      m_peakVec[id]->setParameter("Height", heights[id]);
      m_peakVec[id]->setParameter("Alpha", alpha);
      m_peakVec[id]->setParameter("Beta", beta);
      m_peakVec[id]->setParameter("Sigma2", sigma2);
      m_peakVec[id]->setParameter("Gamma", gamma);
    }

    return;
  }
     */

  /*
  void LeBailFunction::function1D(double *out, const double *xValues, size_t nData) const
  {
    // 1. Get parameters (class)
    Alph0 = getParameter("Alph0");
    Alph1 = getParameter("Alph1");
    Beta0 = getParameter("Beta0");
    Beta1 = getParameter("Beta1");
    Alph0t = getParameter("Alph0t");
    Alph1t = getParameter("Alph1t");
    Beta0t = getParameter("Beta0t");
    Beta1t = getParameter("Beta1t");
    Dtt1 = getParameter("Dtt1");
    Dtt1t = getParameter("Dtt1t");
    Dtt2t = getParameter("Dtt2t");
    Zero = getParameter("Zero");
    Zerot = getParameter("Zerot");
    Sig0 = getParameter("Sig0");
    Sig1 = getParameter("Sig1");
    Sig2 = getParameter("Sig2");
    Gam0 = getParameter("Gam0");
    Gam1 = getParameter("Gam1");
    Gam2 = getParameter("Gam2");
    double latticeconstant = getParameter("LatticeConstant");


    std::cout << " \n-------------------------  being visited -----------------------\n" << std::endl;
    std::cout << "Alph0  = " << Alph0 << std::endl;
    std::cout << "Alph1  = " << Alph1 << std::endl;
    std::cout << "Alph0t = " << Alph0t << std::endl;
    std::cout << "Alph1t = " << Alph1t << std::endl;
    std::cout << "Zero   = " << Zero << std::endl;
    std::cout << "Zerot  = " << Zerot << std::endl;
    std::cout << "Lattice= " << latticeconstant << " Number of Peaks = " << mPeakHKLs.size() << std::endl;


    // 2.
    double *tempout = new double[nData];
    for (size_t iy = 0; iy < nData; ++iy)
    {
      out[iy] = 0.0;
    }

    for (size_t id = 0; id < m_peakHKLVec.size(); ++id)
    {
      int h = m_peakHKLVec[id][0];
      int k = m_peakHKLVec[id][1];
      int l = m_peakHKLVec[id][2];
      double dh = calCubicDSpace(latticeconstant, h, k, l);
      dvalues[id] = dh;

      // a) Calculate all the parameters
      double alpha, beta, tof_h, sigma2, gamma;
      calPeakParametersForD(dh, alpha, beta, tof_h, sigma2, gamma, mPeakParameters[id]);

      // b) Set peak parameters
      g_log.debug() << "DB546 Peak @ d = " << dh << " Set Height = " << dh << std::endl;
      m_peakVec[id]->setParameter("TOF_h", tof_h);
      m_peakVec[id]->setParameter("Height", heights[id]);
      m_peakVec[id]->setParameter("Alpha", alpha);
      m_peakVec[id]->setParameter("Beta", beta);
      m_peakVec[id]->setParameter("Sigma2", sigma2);
      m_peakVec[id]->setParameter("Gamma", gamma);

      // c) Calculate individual peak range
      m_peakVec[id]->setPeakRadius(PEAKRADIUS);

      // d) Calculate peak
      m_peakVec[id]->function1D(tempout, xValues, nData);
      for (size_t iy = 0; iy < nData; ++iy)
      {
        out[iy] += tempout[iy];
      }
    } // END-FOR D-values

    for (size_t n = 0; n < nData; ++n)
      g_log.debug() << "DB327 " << xValues[n] << "\t\t" << out[n] << std::endl;

    // 3. Clean
    delete[] tempout;

    return;
  }
  */

  /*
   * Using numerical derivative

  void LeBailFunction::functionDeriv(const API::FunctionDomain &domain, API::Jacobian &jacobian)
  {
    calNumericalDeriv(domain, jacobian);
    return;
  }
   */

  /*
   * Analytical

  void LeBailFunction::functionDeriv1D(API::Jacobian *out, const double* xValues, const size_t nData)
  {
    UNUSED_ARG(out);
    UNUSED_ARG(xValues);
    UNUSED_ARG(nData);

    throw std::runtime_error("LeBailFunction does not support analytical derivative. ");
  }
    */

  /** Add a peak with its d-value

  void LeBailFunction::addPeak(double dh, double height)
  {
    dvalues.push_back(dh);
    heights.push_back(height);

    // API::IPeakFunction* tpeak = new CurveFitting::Bk2BkExpConvPV();
    CurveFitting::Bk2BkExpConvPV* peakptr = new CurveFitting::Bk2BkExpConvPV();
    CurveFitting::Bk2BkExpConvPV_sptr tpeak(peakptr);
    tpeak->setPeakRadius(8);

    tpeak->initialize();
    m_peakVec.push_back(tpeak);

    std::map<std::string, double> parmap;
    mPeakParameters.push_back(parmap);

    return;
  }
     */


  /*
   * Reset all peaks' height
       * @param peakheights :: list of peak heights corresponding to each peak
   */
  void LeBailFunction::setPeakHeights(std::vector<double> inheights)
  {
    if (inheights.size() != heights.size())
    {
      g_log.error() << "Input number of peaks (height) is not same as peaks. " << std::endl;
      throw std::logic_error("Input number of peaks (height) is not same as peaks. ");
    }

    for (size_t ih = 0; ih < inheights.size(); ++ih)
      heights[ih] = inheights[ih];

    return;
  }


  IPowderDiffPeakFunction_sptr LeBailFunction::getPeak(size_t peakindex)
  {
    if (peakindex >= m_numPeaks)
    {
      g_log.error() << "Try to access peak " << peakindex << " out of range [0, " << m_peakVec.size() << ")." << std::endl;
      throw std::invalid_argument("getPeak() out of boundary");
    }

    IPowderDiffPeakFunction_sptr rpeak = m_peakVec[peakindex];

    return rpeak;
  }


  /*
   * Calculate d = a/sqrt(h**2+k**2+l**2)
   */
  double LeBailFunction::calCubicDSpace(double a, int h, int k, int l) const
  {
    double hklfactor = sqrt(double(h*h)+double(k*k)+double(l*l));
    double d = a/hklfactor;
    g_log.debug() << "DB143 a = " << a << " (HKL) = " << h << ", " << k << ", " << l << ": d = " << d << std::endl;

    return d;
  }

  /*
   * A public function API for function1D
   */
  void LeBailFunction::calPeaks(double* out, const double* xValues, const size_t nData)
  {
    this->function1D(out, xValues, nData);

    return;
  }

  /*
   * Return peak parameters
   */
  double LeBailFunction::getPeakParameter(size_t index, std::string parname) const
  {
    if (index >= mPeakParameters.size())
    {
      g_log.error() << "getParameter() Index out of range" << std::endl;
      throw std::runtime_error("Index out of range");
    }

    IPowderDiffPeakFunction_sptr peak = m_peakVec[index];

    double value = peak->getParameter(parname);

    /*
    std::map<std::string, double>::iterator mit;
    mit = mPeakParameters[index].find(parname);
    double value = 0.0;
    if (mit != mPeakParameters[index].end())
    {
      value = mit->second;
    }
    else
    {
      g_log.error() << "Unable to find parameter " << parname << " in PeakParameters[" << index << "]" << std::endl;
      throw std::invalid_argument("Non-existing parameter name.");
    }
    */

    return value;
  }

  /*
   * Return FWHM of the peak specified
   */
  double LeBailFunction::getPeakFWHM(size_t peakindex) const
  {
    if (peakindex >= m_peakVec.size())
    {
      g_log.error() << "LeBailFunction() cannot get peak indexed " << peakindex << ".  Number of peaks = " << m_peakVec.size() << std::endl;
      throw std::invalid_argument("LeBailFunction getPeakFWHM() cannot return peak indexed out of range. ");
    }

    return m_peakVec[peakindex]->fwhm();

  }


} // namespace Mantid
} // namespace CurveFitting
