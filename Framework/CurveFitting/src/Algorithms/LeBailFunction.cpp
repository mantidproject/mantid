// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Algorithms/LeBailFunction.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
#include "MantidHistogramData/HistogramX.h"

#include "MantidHistogramData/HistogramY.h"

#include <sstream>
#include <utility>

#include <gsl/gsl_sf_erf.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::HistogramData::HistogramY;

using namespace std;

const double NEG_DBL_MAX(-1. * DBL_MAX);

namespace Mantid::CurveFitting::Algorithms {
namespace {
const double PEAKRANGECONSTANT = 5.0;

const string CHEBYSHEV_BACKGROUND("Chebyshev");
const string POLYNOMIAL_BACKGROUND("Polynomial");
const string FULLPROF_POLYNOMIAL_BACKGROUND("FullprofPolynomial");
} // namespace

// Get a reference to the logger
Kernel::Logger g_log("LeBailFunction");

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LeBailFunction::LeBailFunction(const std::string &peaktype) {
  // Set initial values to some class variables
  CompositeFunction_sptr m_function(new CompositeFunction());
  m_compsiteFunction = m_function;

  m_numPeaks = 0;

  m_isInputValue = false;
  m_hasNewPeakValue = false;

  // Peak type, validate and parameter name vectors
  m_peakType = peaktype;
  IFunction_sptr ifunc = FunctionFactory::Instance().createFunction(m_peakType);
  if (!ifunc) {
    stringstream errss;
    errss << "Input peak type " << peaktype << " is not a recoganizable Mantid function.";
    throw runtime_error(errss.str());
  }
  IPowderDiffPeakFunction_sptr peakfunc = std::dynamic_pointer_cast<IPowderDiffPeakFunction>(ifunc);
  if (!peakfunc) {
    stringstream errss;
    errss << "Input peak type " << peaktype << " is not a IPowderDiffPeakFunction.";
    throw runtime_error(errss.str());
  }

  m_peakParameterNameVec = peakfunc->getParameterNames();
  m_orderedProfileParameterNames = m_peakParameterNameVec;
  sort(m_orderedProfileParameterNames.begin(), m_orderedProfileParameterNames.end());

  // Peak parameter values
  for (auto parname : m_peakParameterNameVec) {
    m_functionParameters.emplace(parname, 0.0);
  }

  // Importing peak position tolerance
  m_minTOFPeakCentre = 0;
  m_maxTOFPeakCentre = DBL_MAX;
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LeBailFunction::~LeBailFunction() = default;

//----------------------------------------------------------------------------------------------
/** Return the composite function
 */
API::IFunction_sptr LeBailFunction::getFunction() {
  return m_compsiteFunction;
  // return std::dynamic_pointer_cast<IFunction_sptr>(m_compsiteFunction);
}

//----------------------------------------------------------------------------------------------
/** Calculate powder diffraction pattern by Le Bail algorithm
 * @param xvalues :: input vector
 * @param calpeaks :: if true, calculate peaks
 * @param calbkgd :: if true, then calculate background and add to output.
 * otherwise, assume zero background
 * @return :: output vector
 */
HistogramY LeBailFunction::function(const Mantid::HistogramData::HistogramX &xvalues, bool calpeaks,
                                    bool calbkgd) const {

  // Reset output elements to zero
  std::vector<double> out(xvalues.size(), 0);
  const auto &xvals = xvalues.rawData();

  // Peaks
  if (calpeaks) {
    for (size_t ipk = 0; ipk < m_numPeaks; ++ipk) {
      // Reset temporary vector for output
      vector<double> temp(xvalues.size(), 0);
      IPowderDiffPeakFunction_sptr peak = m_vecPeaks[ipk];
      peak->function(temp, xvals);
      transform(out.begin(), out.end(), temp.begin(), out.begin(), ::plus<double>());
    }
  }

  // Background if required
  if (calbkgd) {
    if (!m_background) {
      throw runtime_error("Must define background first!");
    }

    FunctionDomain1DVector domain(xvals);
    FunctionValues values(domain);
    g_log.information() << "Background function (in LeBailFunction): " << m_background->asString() << ".\n";
    m_background->function(domain, values);
    size_t numpts = out.size();
    for (size_t i = 0; i < numpts; ++i)
      out[i] += values[i];
  }

  return HistogramY(out);
}

/**  Calculate a single peak's value
 */
HistogramY LeBailFunction::calPeak(size_t ipk, const std::vector<double> &xvalues, size_t ySize) const {

  if (ipk >= m_numPeaks) {
    stringstream errss;
    errss << "Try to calculate peak indexed " << ipk << ". But number of peaks = " << m_numPeaks;
    g_log.error(errss.str());
    throw runtime_error(errss.str());
  }

  std::vector<double> out(ySize, 0);
  IPowderDiffPeakFunction_sptr peak = m_vecPeaks[ipk];
  peak->function(out, xvalues);
  return HistogramY(out);
}

//----------------------------------------------------------------------------------------------
/** Check whether a parameter is a profile parameter
 * @param paramname :: parameter name to check with
 */
bool LeBailFunction::hasProfileParameter(const std::string &paramname) {
  auto fiter = lower_bound(m_orderedProfileParameterNames.cbegin(), m_orderedProfileParameterNames.cend(), paramname);

  bool found = true;
  if (fiter == m_orderedProfileParameterNames.end()) {
    // End of the vector
    found = false;
  } else {
    // Middle of vector
    string matchparname = *fiter;
    if (matchparname != paramname)
      found = false;
  }

  return found;
}

//----------------------------------------------------------------------------------------------
/** Check whether the newly set parameters are correct, i.e., all peaks are
 * physical
 * This function would be used with setParameters() and etc.
 */
bool LeBailFunction::isParameterValid(double maxfwhm) const {
  // Re-calculate peak parameter if there is some modification
  if (m_hasNewPeakValue) {
    calculatePeakParameterValues();
  }

  // Check whether each peak has valid value
  bool arevalid = true;
  for (size_t i = 0; i < m_numPeaks; ++i) {
    IPowderDiffPeakFunction_sptr peak = m_vecPeaks[i];
    bool isvalid = peak->isPhysical();
    if (isvalid && maxfwhm >= 0)
      isvalid = peak->fwhm() < maxfwhm;
    if (!isvalid) {
      arevalid = false;

      int h, k, l;
      peak->getMillerIndex(h, k, l);
      g_log.information() << "Peak [" << h << ", " << k << ", " << l << "] @ TOF = " << peak->centre()
                          << " has unphysical parameters or unreasonable large FWHM"
                          << ".\n";
      break;
    }
  }

  return arevalid;
}

//----------------------------------------------------------------------------------------------
/** Calculate all peaks' parameter value
 */
void LeBailFunction::calculatePeakParameterValues() const {
  for (size_t i = 0; i < m_numPeaks; ++i) {
    IPowderDiffPeakFunction_sptr peak = m_vecPeaks[i];
    peak->calculateParameters(false);
  }

  m_hasNewPeakValue = false;
}

//----------------------------------------------------------------------------------------------
/** Set peak position tolerance during importing/adding peaks
 * @param peakpostol :: tolerance for peak position
 * @param tofmin :: minimum TOF for peak position
 * @param tofmax :: maximum TOF for peak position
 */
void LeBailFunction::setPeakCentreTolerance(double peakpostol, double tofmin, double tofmax) {
  // m_usePeakPosTol = true;
  m_minTOFPeakCentre = tofmin - peakpostol;
  m_maxTOFPeakCentre = tofmax + peakpostol;
}

//----------------------------------------------------------------------------------------------
/** Generate peaks, and add them to this composite function
 * @param peakhkls :: list of Miller indexes (HKL)
 */
void LeBailFunction::addPeaks(std::vector<std::vector<int>> &peakhkls) {
  // Prerequisit
  if (!m_isInputValue)
    throw runtime_error("Client must set up profile parameter vlaues by calling "
                        "setProfileParameterValues() first! ");

  // Add peaks
  for (size_t ipk = 0; ipk < peakhkls.size(); ++ipk) {
    vector<int> hkl = peakhkls[ipk];

    // Check input Miller Index
    if (hkl.size() != 3) {
      stringstream errss;
      errss << "Error of " << ipk << "-th input Miller Index.  It has " << peakhkls[ipk].size()
            << " items, but not required 3 items.";
      g_log.error(errss.str());
      throw runtime_error(errss.str());
    }

    // Generate new peak
    int h = hkl[0];
    int k = hkl[1];
    int l = hkl[2];
    IPowderDiffPeakFunction_sptr newpeak = generatePeak(h, k, l);
    if (!newpeak) {
      g_log.error("Unable to generate peak. ");
      throw runtime_error("Unable to generate peak.");
    }

    double tofh = newpeak->centre();
    if (tofh < m_minTOFPeakCentre || tofh > m_maxTOFPeakCentre) {
      g_log.information() << "Peak " << h << ", " << k << ", " << l << " 's centre is at TOF = " << tofh
                          << ", which is out of user specified boundary (" << m_minTOFPeakCentre << ", "
                          << m_maxTOFPeakCentre << "). "
                          << ".\n";
    } else {
      double dsp = newpeak->getPeakParameter("d_h");

      // Add new peak to all related data storage
      m_vecPeaks.emplace_back(newpeak);
      // FIXME - Refining lattice size is not considered here!
      m_dspPeakVec.emplace_back(dsp, newpeak);
      m_mapHKLPeak.emplace(hkl, newpeak);
    }
  }

  m_numPeaks = m_vecPeaks.size();

  g_log.information() << "Total " << m_numPeaks << " after trying to add " << peakhkls.size() << " peaks. \n";
} // END of addPeaks()

//----------------------------------------------------------------------------------------------
/** Generate a peak with parameter set by
 * @param h :: H
 * @param k :: K
 * @param l :: L
 */
IPowderDiffPeakFunction_sptr LeBailFunction::generatePeak(int h, int k, int l) {
  IFunction_sptr f = FunctionFactory::Instance().createFunction(m_peakType);
  IPowderDiffPeakFunction_sptr peak = std::dynamic_pointer_cast<IPowderDiffPeakFunction>(f);

  peak->setMillerIndex(h, k, l);
  for (const auto &parname : m_peakParameterNameVec) {
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
 * (a) Assign peaks into groups; each group contains either (1) one peak or (2)
 *peaks overlapped
 * (b) Calculate peak intensities for every peak per group
 *
 * @param vecX :: vector for x values
 * @param vecY :: vector for data with background removed
 * @param vec_summedpeaks:  output vector storing peaks' values calculated
 *
 * Return: True if all peaks' height are physical.  False otherwise
 */
bool LeBailFunction::calculatePeaksIntensities(const vector<double> &vecX, const vector<double> &vecY,
                                               vector<double> &vec_summedpeaks) {
  // Clear inputs
  std::fill(vec_summedpeaks.begin(), vec_summedpeaks.end(), 0.0);

  // Divide peaks into groups from peak's parameters
  vector<vector<pair<double, IPowderDiffPeakFunction_sptr>>> peakgroupvec;
  vector<IPowderDiffPeakFunction_sptr> outboundpeakvec;
  double xmin = vecX.front();
  double xmax = vecX.back();
  groupPeaks(peakgroupvec, outboundpeakvec, xmin, xmax);

  // Calculate each peak's intensity and set
  bool allpeakheightsphysical = true;
  for (size_t ig = 0; ig < peakgroupvec.size(); ++ig) {
    g_log.debug() << "[Fx351] Calculate peaks heights for (peak) group " << ig
                  << " : number of peaks = " << peakgroupvec[ig].size() << "\n";

    bool peakheightsphysical = calculateGroupPeakIntensities(peakgroupvec[ig], vecX, vecY, vec_summedpeaks);

    if (!peakheightsphysical)
      allpeakheightsphysical = false;
  }

  // Set zero to all peaks out of boundary
  for (const auto &peak : outboundpeakvec) {
    peak->setHeight(0.);
  }

  return allpeakheightsphysical;
}

//----------------------------------------------------------------------------------------------
/** Calculate peak's intensities in a group and set the calculated peak height
 * to the corresponding peak function.
 * @param peakgroup:  vector of peak-centre-dpsace value and peak function pair
 * for peaks that are overlapped
 * @param vecX:  vector of X array
 * @param vecY:  vector for data with background removed.
 * @param vec_summedpeaks :: vector of summation of all peaks, i.e., output of
 * sum_peaks
 * @return :: boolean whether the peaks' heights are physical
 */
bool LeBailFunction::calculateGroupPeakIntensities(vector<pair<double, IPowderDiffPeakFunction_sptr>> peakgroup,
                                                   const vector<double> &vecX, const vector<double> &vecY,
                                                   vector<double> &vec_summedpeaks) {
  // Check input peaks group and sort peak by d-spacing
  if (peakgroup.empty()) {
    throw runtime_error("Programming error such that input peak group cannot be empty!");
  } else {
    g_log.debug() << "[Fx155] Peaks group size = " << peakgroup.size() << "\n";
  }
  if (peakgroup.size() > 1)
    sort(peakgroup.begin(), peakgroup.end());

  // Check input vector validity
  if (vec_summedpeaks.size() != vecY.size()) {
    stringstream errss;
    errss << "Input vector 'allpeaksvalues' has wrong size = " << vec_summedpeaks.size()
          << " != data workspace Y's size = " << vecY.size();
    g_log.error(errss.str());
    throw runtime_error(errss.str());
  }

  // Check boundary
  IPowderDiffPeakFunction_sptr leftpeak = peakgroup[0].second;
  double leftbound = leftpeak->centre() - PEAKRANGECONSTANT * leftpeak->fwhm();
  if (leftbound < vecX.front()) {
    stringstream msg;
    int h, k, l;
    leftpeak->getMillerIndex(h, k, l);
    msg << "Peak group (containing " << peakgroup.size() << " peaks) has its left boundary (TOF = " << leftbound
        << ") out side of input data workspace's left boundary (" << vecX.front()
        << ").  Accuracy of its peak intensity might be affected. "
        << "Group's left boundary is determined by its leftmost peak (" << h << ", " << k << ", " << l
        << ") at TOF = " << leftpeak->centre() << " with FWHM = " << leftpeak->fwhm() << ". ";

    g_log.information(msg.str());

    leftbound = vecX[0] + 0.1;
  }
  IPowderDiffPeakFunction_sptr rightpeak = peakgroup.back().second;
  double rightbound = rightpeak->centre() + PEAKRANGECONSTANT * rightpeak->fwhm();
  if (rightbound > vecX.back()) {
    stringstream msg;
    msg << "Peak group's right boundary " << rightbound << " is out side of "
        << "input data workspace's right bound (" << vecX.back()
        << ")! Accuracy of its peak intensity might be affected. ";

    g_log.information(msg.str());

    rightbound = vecX.back() - 0.1;
  }

  // Determine calculation range to input workspace: [ileft, iright)
  vector<double>::const_iterator cviter;

  cviter = lower_bound(vecX.begin(), vecX.end(), leftbound);
  size_t ileft = static_cast<size_t>(cviter - vecX.begin());
  if (ileft > 0)
    --ileft;

  cviter = lower_bound(vecX.begin(), vecX.end(), rightbound);
  size_t iright = static_cast<size_t>(cviter - vecX.begin());
  if (iright <= vecX.size() - 1)
    ++iright;

  size_t ndata = iright - ileft;
  if (ileft >= iright) {
    stringstream errss;
    errss << "[Calcualte Peak Intensity] Group range is unphysical.  iLeft = " << ileft << ", iRight = " << iright
          << "; Number of peaks = " << peakgroup.size() << "; Left boundary = " << leftbound
          << ", Right boundary = " << rightbound << "; Left peak FWHM = " << leftpeak->fwhm()
          << ", Right peak FWHM = " << rightpeak->fwhm();
    for (size_t ipk = 0; ipk < peakgroup.size(); ++ipk) {
      IPowderDiffPeakFunction_sptr thispeak = peakgroup[ipk].second;
      errss << "Peak " << ipk << ":  d_h = " << peakgroup[ipk].first << ", TOF_h = " << thispeak->centre()
            << ", FWHM = " << thispeak->fwhm() << "\n";
      vector<string> peakparamnames = thispeak->getParameterNames();
      for (auto &peakparamname : peakparamnames) {
        errss << "\t" << peakparamname << " = " << thispeak->getParameter(peakparamname) << "\n";
      }
    }

    g_log.error(errss.str());
    throw runtime_error(errss.str());
  }

  // Generate a subset of vecX and vecY to calculate peak intensities
  vector<double> datax(vecX.begin() + ileft, vecX.begin() + iright);
  vector<double> datay(vecY.begin() + ileft, vecY.begin() + iright);
  if (datax.size() != ndata) {
    stringstream errmsg;
    errmsg << "Impossible: Partial peak data size = " << datax.size() << " != ndata = " << ndata;
    g_log.error(errmsg.str());
    throw runtime_error(errmsg.str());
  }
  g_log.debug() << "[DBx356] Number of data points = " << ndata << " index from " << ileft << " to " << iright
                << ";  Size(datax, datay) = " << datax.size() << "\n";

  // Prepare to integrate dataY to calculate peak intensity
  vector<double> sumYs(ndata, 0.0);
  size_t numPeaks(peakgroup.size());
  vector<vector<double>> peakvalues(numPeaks);

  // Integrage peak by peak
  bool datavalueinvalid = false;
  for (size_t ipk = 0; ipk < numPeaks; ++ipk) {
    // calculate peak function value.  Peak height should be set to a non-zero
    // value
    IPowderDiffPeakFunction_sptr peak = peakgroup[ipk].second;
    peak->setHeight(1.0);
    vector<double> localpeakvalue(ndata, 0.0);
    peak->function(localpeakvalue, datax);

    // check data
    const auto numbadpts = std::count_if(localpeakvalue.cbegin(), localpeakvalue.cend(), [&](const auto &pt) {
      return (pt != 0.) && (pt < NEG_DBL_MAX || pt > DBL_MAX);
    });

    // report the problem and/or integrate data
    if (numbadpts == 0) {
      // Data is fine.  Integrate them all
      for (size_t i = 0; i < ndata; ++i) {
        // If value is physical
        sumYs[i] += localpeakvalue[i];
      }
    } else {
      // Report the problem
      int h, k, l;
      peak->getMillerIndex(h, k, l);
      stringstream warnss;
      warnss << "Peak (" << h << ", " << k << ", " << l << ") @ TOF = " << peak->centre() << " has " << numbadpts
             << " data points, "
             << "whose values exceed limit (i.e., not physical). ";
      g_log.debug(warnss.str());
      datavalueinvalid = true;
    }
    peakvalues[ipk].assign(localpeakvalue.begin(), localpeakvalue.end());
  } // For All peaks

  // Calculate intensity of all peaks
  bool peakheightsphysical = !datavalueinvalid;
  if (peakheightsphysical) {
    for (size_t ipk = 0; ipk < peakgroup.size(); ++ipk) {
      IPowderDiffPeakFunction_sptr peak = peakgroup[ipk].second;
      double intensity = 0.0;

      for (size_t i = 0; i < ndata; ++i) {
        double temp;
        if (sumYs[i] > 1.0E-5) {
          // Reasonable non-zero value
          double peaktogroupratio = peakvalues[ipk][i] / sumYs[i];
          temp = datay[i] * peaktogroupratio;
        } else {
          // SumY too smaller
          temp = 0.0;
        }
        double deltax;
        if (i == 0)
          deltax = datax[1] - datax[0];
        else
          deltax = datax[i] - datax[i - 1];

        intensity += temp * deltax;
      } // for data points

      if (intensity != intensity) {
        // Unphysical intensity: NaN
        intensity = 0.0;
        peakheightsphysical = false;

        int h, k, l;
        peak->getMillerIndex(h, k, l);
        g_log.warning() << "Peak (" << h << ", " << k << ", " << l << ") has unphysical intensity = NaN!\n";

      } else if (intensity <= -DBL_MAX || intensity >= DBL_MAX) {
        // Unphysical intensity: NaN
        intensity = 0.0;
        peakheightsphysical = false;

        int h, k, l;
        peak->getMillerIndex(h, k, l);
        g_log.warning() << "Peak (" << h << ", " << k << ", " << l << ") has unphysical intensity = Infty!\n";
      } else if (intensity < 0.0) {
        // No negative intensity
        g_log.debug() << "[Fx134] Set peak @ " << peak->centre() << "'s intensity to 0.0 instead of " << intensity
                      << ".\n";
        intensity = 0.0;
      }
      g_log.debug() << "[Fx407] Peak @ " << peak->centre() << ": Set Intensity = " << intensity << "\n";
      peak->setHeight(intensity);

      // Add peak's value to peaksvalues
      for (size_t i = ileft; i < iright; ++i) {
        vec_summedpeaks[i] += (intensity * peakvalues[ipk][i - ileft]);
      }

    } // ENDFOR each peak
  }

  return peakheightsphysical;
}

//----------------------------------------------------------------------------------------------
/** From table/map to set parameters to an individual peak.
 * It mostly is called by function in calculation.
 * @param peak :  ThermalNeutronBk2BkExpConvPVoigt function to have parameters'
 * value set
 * @param parammap:  map of Parameters to set to peak
 * @param peakheight: height of the peak
 * @param setpeakheight:  boolean as the option to set peak height or not.
 */
void LeBailFunction::setPeakParameters(const IPowderDiffPeakFunction_sptr &peak, const map<string, double> &parammap,
                                       double peakheight, bool setpeakheight) {
  UNUSED_ARG(peak);
  UNUSED_ARG(parammap);
  UNUSED_ARG(peakheight);
  UNUSED_ARG(setpeakheight);
  throw runtime_error("Requiring update flag: peak value changed and etc.");
  /*
  // FIXME - The best solution for speeding is to have a set of peak
  parameter listed in the order
  //         of peak function's parameters' indexed.  Then no need to do
  search anymore.

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
  g_log.debug() << "Parameter '" << parname << "' in input parameter
  table workspace "
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

  return;*/
}

//----------------------------------------------------------------------------------------------
/** From a parameter name/value map to
 * 1. store values to LeBailFunction;
 * 2. new values to each peak
 *
 * Request: order of parameter names in m_peakParameterNameVec must be same as
 *the order in
 *          IPowderDiffPeakFunction.
 *
 * @param parammap: map of Parameters to set to peak
 */
void LeBailFunction::setProfileParameterValues(map<std::string, double> parammap) {
  const double MINDIFF = 1.0E-10;

  map<std::string, double>::iterator inpiter, curiter;

  size_t numpars = m_peakParameterNameVec.size();
  for (size_t i = 0; i < numpars; ++i) {
    string &parname = m_peakParameterNameVec[i];

    // Find iterator of this parameter in input parammap
    inpiter = parammap.find(parname);
    if (inpiter != parammap.end()) {
      // Find iterator to parameter value in class' parameter map (parameter is
      // found in input map)
      curiter = m_functionParameters.find(parname);
      if (curiter == m_functionParameters.end()) {
        stringstream errmsg;
        errmsg << "Parameter " << parname << " is in parameter name list, but not in profile "
               << "parameter map.  It violates the programming logic.";
        g_log.error(errmsg.str());
        throw runtime_error(errmsg.str());
      }

      // Set value if difference is large
      double curvalue = curiter->second;
      double newvalue = inpiter->second;
      bool localnewvalue = false;
      if (fabs(curvalue - newvalue) > MINDIFF) {
        curiter->second = newvalue;
        m_hasNewPeakValue = true;
        localnewvalue = true;
      }

      // Set new value to each peak
      if (!localnewvalue)
        continue;

      // Set new parameter to each peak
      for (size_t ipk = 0; ipk < m_numPeaks; ++ipk) {
        IPowderDiffPeakFunction_sptr peak = m_vecPeaks[ipk];
        peak->setParameter(i, newvalue);
      }
    } // If parameter name is a profile parameter
    else {
      g_log.debug() << "Parameter " << parname << " is not a profile parameter. Length of string = " << parname.size()
                    << "\n";
    }
  } // ENDFOR [All profile parameter]

  // Set the flag to indicate that client has input parameters
  if (m_hasNewPeakValue && !m_isInputValue)
    m_isInputValue = true;
}

//----------------------------------------------------------------------------------------------
/** Group peaks together
 * @param peakgroupvec:  output vector containing peaks grouped together.
 * @param outboundpeakvec: output vector containing peaks out of bound range
 * @param xmin : minimim x value of the data
 * @param xmax : maximum x value of the data
 * Disabled argument: MatrixWorkspace_sptr dataws, size_t workspaceindex,
 */
void LeBailFunction::groupPeaks(vector<vector<pair<double, IPowderDiffPeakFunction_sptr>>> &peakgroupvec,
                                vector<IPowderDiffPeakFunction_sptr> &outboundpeakvec, double xmin, double xmax) {
  // Sort peaks
  if (m_numPeaks > 1) {
    sort(m_dspPeakVec.begin(), m_dspPeakVec.end());
  } else if (m_numPeaks == 0) {
    std::stringstream errmsg;
    errmsg << "Group peaks:  No peak is found in the peak vector. ";
    g_log.error() << errmsg.str() << "\n";
    throw std::runtime_error(errmsg.str());
  }

  // Set up starting value
  peakgroupvec.clear();
  outboundpeakvec.clear();
  vector<pair<double, IPowderDiffPeakFunction_sptr>> peakgroup; // one group of peaks
  size_t ipk = 0;

  // Group peaks from low-d to high-d
  bool outbound = true;
  while (outbound && ipk < m_numPeaks) {
    // Group peaks out of lower boundary to a separate vector of peaks
    IPowderDiffPeakFunction_sptr peak = m_dspPeakVec[ipk].second;
    if (peak->centre() <= xmin) {
      // Add peak
      outboundpeakvec.emplace_back(peak);
      ipk += 1;
    } else {
      // Get out of while loop if peak is in bound
      outbound = false;
    }
  }

  bool inbound = true;
  while (inbound && ipk < m_numPeaks) {
    // Group peaks in the boundary
    IPowderDiffPeakFunction_sptr thispeak = m_dspPeakVec[ipk].second;

    if (thispeak->centre() < xmax) {
      // Peak is in the boundary still

      // add peak to CURRENT peak group
      peakgroup.emplace_back(m_dspPeakVec[ipk]);

      if (ipk < m_numPeaks - 1) {
        // Any peak but not the last (rightmost) peak

        // test whether next peak will be in a different group
        IPowderDiffPeakFunction_sptr rightpeak = m_dspPeakVec[ipk + 1].second;

        double thispeak_rightbound = thispeak->centre() + PEAKRANGECONSTANT * thispeak->fwhm();
        double rightpeak_leftbound = rightpeak->centre() - PEAKRANGECONSTANT * rightpeak->fwhm();

        if (thispeak_rightbound < rightpeak_leftbound) {
          // this peak and its right peak are well separated.
          // finish this group by swapping values
          peakgroupvec.emplace_back(std::move(peakgroup));
          peakgroup = {};
        } else {
          // this peak and its right peak are close enough to be in same group.
          // do nothing
          ;
        }
      } else {
        // Rightmost peak.  Finish the current peak
        peakgroupvec.emplace_back(peakgroup);
      }

      ++ipk;
    } // still in bound
    else {
      // Peak is get out of boundary
      inbound = false;
      g_log.information() << "[Fx301] Group peak: peak @ " << thispeak->centre() << " causes grouping "
                          << "peak over at maximum TOF = " << xmax << ".\n";

      if (!peakgroup.empty()) {
        peakgroupvec.emplace_back(peakgroup);
      }
    } // FIRST out of boundary
  } // ENDWHILE

  while (ipk < m_numPeaks) {
    // Group peaks out of uppper boundary to a separate vector of peaks
    outboundpeakvec.emplace_back(m_dspPeakVec[ipk].second);
    ipk += 1;
  }

  g_log.debug() << "[Calculate Peak Intensity]:  Number of Peak Groups = " << peakgroupvec.size() << "\n";
}

//----------------------------------------------------------------------------------------------
/** Add background function.
 * The supported background types are Polynomial/Linear/Flat and Chebyshev
 * @param backgroundtype :: string, type of background, such as Polynomial,
 * Chebyshev
 * @param order :: polynomial order for the background
 * @param vecparnames :: vector of parameter names
 * @param vecparvalues :: vector of parameter values from order 0.
 * @param startx :: background's StartX.  Used by Chebyshev
 * @param endx :: background's EndX.  Used by Chebyshev
 */
void LeBailFunction::addBackgroundFunction(const string &backgroundtype, const unsigned int &order,
                                           const std::vector<std::string> &vecparnames,
                                           const std::vector<double> &vecparvalues, double startx, double endx) {
  // Check
  if (backgroundtype != POLYNOMIAL_BACKGROUND && backgroundtype != CHEBYSHEV_BACKGROUND &&
      backgroundtype != FULLPROF_POLYNOMIAL_BACKGROUND) {
    stringstream warnss;
    warnss << "Cliet specified background type " << backgroundtype << " may not be supported properly.";
    g_log.warning(warnss.str());
  }
  if (vecparnames.size() != vecparvalues.size())
    throw runtime_error("Input parameter names and parameter values are not matched. ");

  g_log.information() << "Add background: type = " << backgroundtype << ", order = " << order
                      << ", number of parameters/attributes = " << vecparnames.size() << "\n";

  // Create background function from factory
  auto background = FunctionFactory::Instance().createFunction(backgroundtype);
  m_background = std::dynamic_pointer_cast<Functions::BackgroundFunction>(background);

  // Set order and initialize
  m_background->setAttributeValue("n", static_cast<int>(order));
  m_background->initialize();

  // Set parameters & attribute
  size_t numpars = vecparnames.size();
  for (size_t i = 0; i < numpars; ++i) {
    const string &parname = vecparnames[i];
    if (parname != "Bkpos")
      m_background->setParameter(parname, vecparvalues[i]);
    else if (backgroundtype == FULLPROF_POLYNOMIAL_BACKGROUND)
      m_background->setAttributeValue("Bkpos", vecparvalues[i]);
    else
      throw runtime_error("Bkpos should not be in the parameter list. ");
  }

  if (backgroundtype == CHEBYSHEV_BACKGROUND) {
    if (startx > 0.)
      m_background->setAttributeValue("StartX", startx);
    if (endx > 0.)
      m_background->setAttributeValue("EndX", endx);
  }
}

//----------------------------------------------------------------------------------------------
/** Set up a profile parameter to fit but tied among all peaks
 * @param paramname :: name of parameter
 * @param minvalue :: lower boundary
 * @param maxvalue :: upper boundary
 */
void LeBailFunction::setFitProfileParameter(const string &paramname, double minvalue, double maxvalue) {
  // Make ties in composition function
  for (size_t ipk = 1; ipk < m_numPeaks; ++ipk) {
    stringstream ss1, ss2;
    ss1 << "f" << (ipk - 1) << "." << paramname;
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
  auto bc = std::make_unique<Constraints::BoundaryConstraint>(m_compsiteFunction.get(), parnamef0, minvalue, maxvalue);
  m_compsiteFunction->addConstraint(std::move(bc));
}

//----------------------------------------------------------------------------------------------
/** Set up a parameter to be fixed
 * @param paramname :: name of parameter
 * @param paramvalue :: value of parameter to be fixed to
 */
void LeBailFunction::fixPeakParameter(const string &paramname, double paramvalue) {
  for (size_t ipk = 0; ipk < m_numPeaks; ++ipk) {
    stringstream ss1, ss2;
    ss1 << "f" << ipk << "." << paramname;
    ss2 << paramvalue;
    string tiepart1 = ss1.str();
    string tievalue = ss2.str();
    m_compsiteFunction->tie(tiepart1, tievalue);

    g_log.debug() << "Set up tie | " << tiepart1 << " <---> " << tievalue << " | \n";

    // FIXME & TODO: Make a map between peak parameter name and index. And use
    // fix() to replace tie
    /*--  Code prepared to replace the existing block
    ThermalNeutronBk2BkExpConvPVoigt_sptr thispeak = m_dspPeaks[ipk].second;
    size_t iparam = findIndex(thispeak, funcparam.name);
    thispeak->fix(iparam);
    --*/

  } // For each peak
}

//----------------------------------------------------------------------------------------------
/** Fix all background parameters
 */
void LeBailFunction::fixBackgroundParameters() {
  size_t numbkgdparams = m_background->nParams();

  for (size_t iparam = 0; iparam < numbkgdparams; ++iparam)
    m_background->fix(iparam);
}

//----------------------------------------------------------------------------------------------
/** Fix all peaks' intensity/height
 */
void LeBailFunction::setFixPeakHeights() {
  for (size_t ipk = 0; ipk < m_numPeaks; ++ipk) {
    // a. Get peak height
    IPowderDiffPeakFunction_sptr thispeak = m_dspPeakVec[ipk].second;
    thispeak->fix(0);
  } // For each peak
}

//----------------------------------------------------------------------------------------------
/** Reset all peaks' height
 * @param inheights :: list of peak heights corresponding to each peak
 */
void LeBailFunction::setPeakHeights(const std::vector<double> &inheights) {
  UNUSED_ARG(inheights);
  throw runtime_error("It is not implemented properly.");
  /*
  if (inheights.size() != heights.size())
  {
  g_log.error() << "Input number of peaks (height) is not same as peaks. "
  << '\n';
  throw std::logic_error("Input number of peaks (height) is not same as
  peaks. ");
  }

  for (size_t ih = 0; ih < inheights.size(); ++ih)
  heights[ih] = inheights[ih];

  return;*/
}

//----------------------------------------------------------------------------------------------
/** Get the reference to a peak
 */
IPowderDiffPeakFunction_sptr LeBailFunction::getPeak(size_t peakindex) {
  if (peakindex >= m_numPeaks) {
    stringstream errmsg;
    errmsg << "Try to access peak " << peakindex << " out of range [0, " << m_numPeaks << ").";
    g_log.error(errmsg.str());
    throw runtime_error(errmsg.str());
  }

  IPowderDiffPeakFunction_sptr rpeak = m_vecPeaks[peakindex];

  return rpeak;
}

//----------------------------------------------------------------------------------------------
/** Get value of one specific peak's parameter
 */
double LeBailFunction::getPeakParameter(std::vector<int> hkl, const std::string &parname) const {
  // Search peak in map
  map<vector<int>, IPowderDiffPeakFunction_sptr>::const_iterator fiter;
  fiter = m_mapHKLPeak.find(hkl);
  if (fiter == m_mapHKLPeak.end()) {
    stringstream errss;
    errss << "Peak with Miller index (" << hkl[0] << ", " << hkl[1] << "," << hkl[2]
          << ") does not exist in Le Bail function.";
    g_log.error(errss.str());
    throw runtime_error(errss.str());
  }

  IPowderDiffPeakFunction_sptr peak = fiter->second;

  double parvalue = getPeakParameterValue(peak, parname);

  return parvalue;
}

//----------------------------------------------------------------------------------------------
/** Get value of one specific peak's parameter
 */
double LeBailFunction::getPeakParameter(size_t index, const std::string &parname) const {
  if (index >= m_numPeaks) {
    stringstream errss;
    errss << "getPeakParameter() tries to reach a peak with index " << index << ", which is out of range " << m_numPeaks
          << "/" << m_vecPeaks.size() << ".";
    g_log.error(errss.str());
    throw std::runtime_error(errss.str());
  }

  IPowderDiffPeakFunction_sptr peak = m_vecPeaks[index];
  double value = getPeakParameterValue(peak, parname);

  return value;
}

//----------------------------------------------------------------------------------------------
/** Retrieve peak's parameter.  may be native or calculated
 * @param peak :: shared pointer to peak function
 * @param parname :: name of the peak parameter
 */
double LeBailFunction::getPeakParameterValue(const API::IPowderDiffPeakFunction_sptr &peak,
                                             const std::string &parname) const {
  // Locate the category of the parameter name
  auto vsiter = lower_bound(m_orderedProfileParameterNames.cbegin(), m_orderedProfileParameterNames.cend(), parname);

  bool found = true;
  if (vsiter == m_orderedProfileParameterNames.end()) {
    // End of vector
    found = false;
  } else {
    // Middle of vector. But no match
    string matchparname = *vsiter;
    if (parname != matchparname)
      found = false;
  }

  // Get parameter
  double parvalue;
  if (found) {
    // It is a native peak parameter
    parvalue = peak->getParameter(parname);
  } else {
    // It is a calculated peak parameter
    parvalue = peak->getPeakParameter(parname);
  }

  return parvalue;
}

//----------------------------------------------------------------------------------------------
/** Get the maximum value of a peak in a given set of data points
 */
double LeBailFunction::getPeakMaximumValue(std::vector<int> hkl, const std::vector<double> &xvalues, size_t &ix) {
  // Search peak in map
  map<vector<int>, IPowderDiffPeakFunction_sptr>::const_iterator fiter;
  fiter = m_mapHKLPeak.find(hkl);
  if (fiter == m_mapHKLPeak.end()) {
    stringstream errss;
    errss << "Peak with Miller index (" << hkl[0] << ", " << hkl[1] << "," << hkl[2]
          << ") does not exist in Le Bail function.";
    g_log.error(errss.str());
    throw runtime_error(errss.str());
  }

  IPowderDiffPeakFunction_sptr peak = fiter->second;

  double maxvalue = peak->getMaximumValue(xvalues, ix);

  return maxvalue;
}

} // namespace Mantid::CurveFitting::Algorithms
