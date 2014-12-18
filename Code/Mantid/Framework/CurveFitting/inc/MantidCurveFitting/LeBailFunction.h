#ifndef MANTID_CURVEFITTING_LEBAILFUNCTION_H_
#define MANTID_CURVEFITTING_LEBAILFUNCTION_H_

#include "MantidKernel/System.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IPowderDiffPeakFunction.h"
#include "MantidCurveFitting/BackgroundFunction.h"

namespace Mantid
{
namespace CurveFitting
{

  /** LeBailFunction : LeBailFunction is to calculate peak intensities in a composite
   *                   function including neutron peak and background functions.

    @date 2013-04-26 : original LeBailFunction is not used by any other functions. And thus
                       it is rewritten.

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport LeBailFunction
  {
  public:
    /// Constructor
    LeBailFunction(std::string peaktype);

    /// Destructor
    virtual ~LeBailFunction();

    /// From table/map to set parameters to all peaks.
    void setProfileParameterValues(std::map<std::string, double> parammap);

    /// Set up a parameter to fit but tied among all peaks
    void setFitProfileParameter(std::string paramname, double minvalue, double maxvalue);

    /// Function
    void setPeakHeights(std::vector<double> inheights);

    /// Check whether a parameter is a profile parameter
    bool hasProfileParameter(std::string paramname);

    /// Check whether the newly set parameters are correct, i.e., all peaks are physical
    bool isParameterValid(double maxfwhm=DBL_MAX) const;

    /// Set peak position tolerance during importing/adding peaks
    void setPeakCentreTolerance(double peakpostol,  double tofmin, double tofmax);

    /// Generate peaks, and add them to this composite function
    void addPeaks(std::vector<std::vector<int> > peakhkls);

    /// Add background function
    void addBackgroundFunction(std::string backgroundtype, const unsigned int& order, const std::vector<std::string>& vecparnames,
                               const std::vector<double>& vecparvalues, double startx, double endx);

    /// Get number of peaks
    size_t getNumberOfPeaks() const { return m_numPeaks; }

    /// Calculate
    void function(std::vector<double>& out, const std::vector<double> &xvalues, bool calpeaks, bool calbkgd) const;

    ///  Calculate a single peak's value
    void calPeak(size_t ipk, std::vector<double>& out, const std::vector<double>& xvalues) const;

    /// Return the composite function
    API::IFunction_sptr getFunction();

    /// Get reference to a peak
    API::IPowderDiffPeakFunction_sptr getPeak(size_t peakindex);

    /// Force to make all peaks to calculate peak parameters
    void calPeaksParameters();

    /// Get peak parameters (calculated)
    double getPeakParameter(size_t index, std::string parname) const;

    /// Get peak parameters (calculated)
    double getPeakParameter(std::vector<int> hkl, std::string parname) const;

    /// Set up a parameter to be fixed
    void fixPeakParameter(std::string paramname, double paramvalue);

    /// Fix all background parameters
    void fixBackgroundParameters();

    /// Fix all peaks' intensity/height
    void setFixPeakHeights();

    /// Calculate peak intensities by Le Bail algorithm
    bool calculatePeaksIntensities(const std::vector<double>& vecX, const std::vector<double>& vecY,
                                   std::vector<double> &vec_summedpeaks);

    /// Get the maximum value of a peak in a given set of data points
    double getPeakMaximumValue(std::vector<int> hkl, const std::vector<double> &xvalues, size_t& ix);

  private:

    /// Set peak parameters
    void setPeakParameters(API::IPowderDiffPeakFunction_sptr peak, std::map<std::string, double > parammap,
                           double peakheight, bool setpeakheight);

    /// Retrieve peak's parameter.  may be native or calculated
    double getPeakParameterValue(API::IPowderDiffPeakFunction_sptr peak, std::string parname) const;

    /// Calculate all peaks' parameter value
    void calculatePeakParameterValues() const;

    /// Generate a peak with parameter set by
    API::IPowderDiffPeakFunction_sptr generatePeak(int h, int k, int l);

    /// Calculate the peaks intensities in same group
    bool calculateGroupPeakIntensities(std::vector<std::pair<double, API::IPowderDiffPeakFunction_sptr> > peakgroup,
                                       const std::vector<double> &vecX, const std::vector<double> &vecY,
                                       std::vector<double> &vec_summedpeaks);

    /// Group close peaks together
    void groupPeaks(std::vector<std::vector<std::pair<double, API::IPowderDiffPeakFunction_sptr> > >& peakgroupvec,
                    std::vector<API::IPowderDiffPeakFunction_sptr> &outboundpeakvec, double xmin, double xmax);

    /// Peak type
    std::string m_peakType;

    /// Number of peaks
    size_t m_numPeaks;    

    /// Name of peak parameter names (be same as the order in IPowderDiffPeakFunction)
    std::vector<std::string> m_peakParameterNameVec;
    /// Ordered profile parameter names for search
    std::vector<std::string> m_orderedProfileParameterNames;

    /// Vector of all peaks
    std::vector<API::IPowderDiffPeakFunction_sptr> m_vecPeaks;
    /// Vector of pair <peak position in d-space, Peak> sortable
    std::vector<std::pair<double, API::IPowderDiffPeakFunction_sptr> > m_dspPeakVec;
    /// Vector of all peak's Miller indexes
    std::map<std::vector<int>, API::IPowderDiffPeakFunction_sptr> m_mapHKLPeak;

    /// Composite functions for all peaks and background
    API::CompositeFunction_sptr m_compsiteFunction;
    /// Background function
    BackgroundFunction_sptr m_background;

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

    // void calPeakParametersForD(double dh, double& alpha, double& beta, double &Tof_h, double &sigma_g2, double &gamma_l, std::map<std::string, double>& parmap) const;
    void adPeakPositionD(double dh);
    double calCubicDSpace(double a, int h, int k, int l) const;
    void addPeak(double d, double height);
        mutable std::vector<double> dvalues;
    /// vector of peak's parameters (It is in strict order with dvalues)
    mutable std::vector<std::map<std::string, double> > mPeakParameters;
    */
  };

  typedef boost::shared_ptr<LeBailFunction> LeBailFunction_sptr;


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_LEBAILFUNCTION_H_ */
