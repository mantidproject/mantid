// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IPowderDiffPeakFunction.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/Functions/BackgroundFunction.h"

namespace Mantid {
namespace HistogramData {
class HistogramX;
class HistogramY;
} // namespace HistogramData
namespace CurveFitting {
namespace Algorithms {

/** LeBailFunction : LeBailFunction is to calculate peak intensities in a
composite
*                   function including neutron peak and background functions.
*                   Note: This is not a Mantid Fit Function, just a helper
*                   class to the algorithm

@date 2013-04-26 : original LeBailFunction is not used by any other functions.
And thus
it is rewritten.
*/
class MANTID_CURVEFITTING_DLL LeBailFunction {
public:
  /// Constructor
  LeBailFunction(const std::string &peaktype);

  /// Destructor
  virtual ~LeBailFunction();

  /// From table/map to set parameters to all peaks.
  void setProfileParameterValues(std::map<std::string, double> parammap);

  /// Set up a parameter to fit but tied among all peaks
  void setFitProfileParameter(const std::string &paramname, double minvalue, double maxvalue);

  /// Function
  void setPeakHeights(const std::vector<double> &inheights);

  /// Check whether a parameter is a profile parameter
  bool hasProfileParameter(const std::string &paramname);

  /// Check whether the newly set parameters are correct, i.e., all peaks are
  /// physical
  bool isParameterValid(double maxfwhm = DBL_MAX) const;

  /// Set peak position tolerance during importing/adding peaks
  void setPeakCentreTolerance(double peakpostol, double tofmin, double tofmax);

  /// Generate peaks, and add them to this composite function
  void addPeaks(std::vector<std::vector<int>> &peakhkls);

  /// Add background function
  void addBackgroundFunction(const std::string &backgroundtype, const unsigned int &order,
                             const std::vector<std::string> &vecparnames, const std::vector<double> &vecparvalues,
                             double startx, double endx);

  /// Get number of peaks
  size_t getNumberOfPeaks() const { return m_numPeaks; }

  /// Calculate
  Mantid::HistogramData::HistogramY function(const Mantid::HistogramData::HistogramX &xvalues, bool calpeaks,
                                             bool calbkgd) const;

  ///  Calculate a single peak's value
  Mantid::HistogramData::HistogramY calPeak(size_t ipk, const std::vector<double> &xvalues, size_t ySize) const;

  /// Return the composite function
  API::IFunction_sptr getFunction();

  /// Get reference to a peak
  API::IPowderDiffPeakFunction_sptr getPeak(size_t peakindex);

  /// Force to make all peaks to calculate peak parameters
  void calPeaksParameters();

  /// Get peak parameters (calculated)
  double getPeakParameter(size_t index, const std::string &parname) const;

  /// Get peak parameters (calculated)
  double getPeakParameter(std::vector<int> hkl, const std::string &parname) const;

  /// Set up a parameter to be fixed
  void fixPeakParameter(const std::string &paramname, double paramvalue);

  /// Fix all background parameters
  void fixBackgroundParameters();

  /// Fix all peaks' intensity/height
  void setFixPeakHeights();

  /// Calculate peak intensities by Le Bail algorithm
  bool calculatePeaksIntensities(const std::vector<double> &vecX, const std::vector<double> &vecY,
                                 std::vector<double> &vec_summedpeaks);

  /// Get the maximum value of a peak in a given set of data points
  double getPeakMaximumValue(std::vector<int> hkl, const std::vector<double> &xvalues, size_t &ix);

private:
  /// Set peak parameters
  void setPeakParameters(const API::IPowderDiffPeakFunction_sptr &peak, const std::map<std::string, double> &parammap,
                         double peakheight, bool setpeakheight);

  /// Retrieve peak's parameter.  may be native or calculated
  double getPeakParameterValue(const API::IPowderDiffPeakFunction_sptr &peak, const std::string &parname) const;

  /// Calculate all peaks' parameter value
  void calculatePeakParameterValues() const;

  /// Generate a peak with parameter set by
  API::IPowderDiffPeakFunction_sptr generatePeak(int h, int k, int l);

  /// Calculate the peaks intensities in same group
  bool calculateGroupPeakIntensities(std::vector<std::pair<double, API::IPowderDiffPeakFunction_sptr>> peakgroup,
                                     const std::vector<double> &vecX, const std::vector<double> &vecY,
                                     std::vector<double> &vec_summedpeaks);

  /// Group close peaks together
  void groupPeaks(std::vector<std::vector<std::pair<double, API::IPowderDiffPeakFunction_sptr>>> &peakgroupvec,
                  std::vector<API::IPowderDiffPeakFunction_sptr> &outboundpeakvec, double xmin, double xmax);

  /// Peak type
  std::string m_peakType;

  /// Number of peaks
  size_t m_numPeaks;

  /// Name of peak parameter names (be same as the order in
  /// IPowderDiffPeakFunction)
  std::vector<std::string> m_peakParameterNameVec;
  /// Ordered profile parameter names for search
  std::vector<std::string> m_orderedProfileParameterNames;

  /// Vector of all peaks
  std::vector<API::IPowderDiffPeakFunction_sptr> m_vecPeaks;
  /// Vector of pair <peak position in d-space, Peak> sortable
  std::vector<std::pair<double, API::IPowderDiffPeakFunction_sptr>> m_dspPeakVec;
  /// Vector of all peak's Miller indexes
  std::map<std::vector<int>, API::IPowderDiffPeakFunction_sptr> m_mapHKLPeak;

  /// Composite functions for all peaks and background
  API::CompositeFunction_sptr m_compsiteFunction;
  /// Background function
  Functions::BackgroundFunction_sptr m_background;

  /// Parameters
  std::map<std::string, double> m_functionParameters;

  /// Has new peak values
  mutable bool m_hasNewPeakValue;

  /// Has first value set up
  bool m_isInputValue;

  std::vector<double> heights;

  double m_minTOFPeakCentre;
  double m_maxTOFPeakCentre;
  /*
  double mL1;
  double mL2;

  mutable double Alph0, Alph1, Alph0t, Alph1t;
  mutable double Beta0, Beta1, Beta0t, Beta1t;
  mutable double Sig0, Sig1, Sig2, Gam0, Gam1, Gam2;
  mutable double Dtt1, Dtt2, Dtt1t, Dtt2t, Zero, Zerot;

  // void calPeakParametersForD(double dh, double& alpha, double& beta, double
  &Tof_h, double &sigma_g2, double &gamma_l, std::map<std::string, double>&
  parmap) const;
  void adPeakPositionD(double dh);
  double calCubicDSpace(double a, int h, int k, int l) const;
  void addPeak(double d, double height);
  mutable std::vector<double> dvalues;
  /// vector of peak's parameters (It is in strict order with dvalues)
  mutable std::vector<std::map<std::string, double> > mPeakParameters;
  */
};

using LeBailFunction_sptr = std::shared_ptr<LeBailFunction>;

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
