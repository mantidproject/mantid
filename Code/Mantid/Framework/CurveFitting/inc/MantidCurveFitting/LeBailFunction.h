#ifndef MANTID_CURVEFITTING_LEBAILFUNCTION_H_
#define MANTID_CURVEFITTING_LEBAILFUNCTION_H_

#include "MantidKernel/System.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IPowderDiffPeakFunction.h"
#include "MantidCurveFitting/BackgroundFunction.h"

/*
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidCurveFitting/Bk2BkExpConvPV.h"
*/

using namespace Mantid::API;
using namespace Mantid::CurveFitting;

using namespace std;

namespace Mantid
{
namespace CurveFitting
{

  /** LeBailFunction : LeBailFunction is to calculate peak intensities in a composite
   *                   function including neutron peak and background functions.

    @date 2013-04-26 : original LeBailFunction is not used by any other functions. And thus
                       it is rewritten.

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    LeBailFunction();

    /// Destructor
    virtual ~LeBailFunction();

    /// Set peak parameters
    void setPeakParameters(map<string, double> peakparammap);

    /// From table/map to set parameters to all peaks.
    void setPeaksParameters(map<std::string, double> parammap);

    /// Add a new peak
    void addPeak(int h, int k, int l);

    /// Get a peak

    /// Calculate
    void function(std::vector<double>& out, std::vector<double>& xvalues);


    /// Function
    void setPeakHeights(std::vector<double> inheights);
    API::IPowderDiffPeakFunction_sptr getPeak(size_t peakindex);

    void calPeaksParameters();

    void calPeaks(double* out, const double* xValues, const size_t nData);

    double getPeakParameter(size_t index, std::string parname) const;

    double getPeakFWHM(size_t peakindex) const;

    /// Fix i-th parameter with given name.
    void fix(size_t paramindex, string paramname);

    /// Generate peaks, and add them to this composite function
    void addPeaks(std::vector<std::vector<int> > peakhkls);

  private:

    virtual void function1D(double* out, const double* xValues, const size_t nData)const;
    virtual void functionDeriv1D(API::Jacobian* out, const double* xValues, const size_t nData);
    virtual void functionDeriv(const API::FunctionDomain& domain, API::Jacobian& jacobian);

    /// overwrite IFunction base class method, which declare function parameters
    virtual void init();



    static Kernel::Logger& g_log;

    ///

    void calPeakParametersForD(double dh, double& alpha, double& beta, double &Tof_h, double &sigma_g2, double &gamma_l, std::map<std::string, double>& parmap) const;
    void adPeakPositionD(double dh);
    double calCubicDSpace(double a, int h, int k, int l) const;
    void addPeak(double d, double height);

    /// Generate a peak with parameter set by
    IPowderDiffPeakFunction_sptr generatePeak(int h, int k, int l);

    /// Set up fit/tie/parameter values to all peaks functions (calling GSL library)
    void setLeBailFitParameters();

    /// Calculate peak intensities by Le Bail algorithm
    bool calculatePeaksIntensities(vector<double>& vecX, vector<double>& vecY, bool zerobackground, vector<double>& allpeaksvalues);

    /// Calculate the peaks intensities in same group
    bool calculateGroupPeakIntensities(vector<pair<double, IPowderDiffPeakFunction_sptr> > peakgroup,
                                       vector<double>& vecX, vector<double> &vecY, bool zerobackground,
                                       vector<double>& allpeaksvalues);

    /// Group close peaks together
    void groupPeaks(vector<vector<pair<double, IPowderDiffPeakFunction_sptr> > >& peakgroupvec);

    /// Number of peaks
    size_t m_numPeaks;
    /// Vector of all peaks
    vector<API::IPowderDiffPeakFunction_sptr> m_peakvec;
    /// Vector of pair <peak position in d-space, Peak> sortable
    vector<pair<double, API::IPowderDiffPeakFunction_sptr> > m_dspPeakVec;
    /// order of parameter names in m_peakParameterNameVec must be same as the order in IPowderDiffPeakFunction.
    vector<string> m_peakParameterNameVec;

    /// Background function
    BackgroundFunction_sptr m_background;

    API::CompositeFunction_sptr m_compsiteFunction;

    /// Parameters
    map<string, double> m_functionParameters;


    /*
    double mL1;
    double mL2;

    mutable double Alph0, Alph1, Alph0t, Alph1t;
    mutable double Beta0, Beta1, Beta0t, Beta1t;
    mutable double Sig0, Sig1, Sig2, Gam0, Gam1, Gam2;
    mutable double Dtt1, Dtt2, Dtt1t, Dtt2t, Zero, Zerot;
    */

    mutable std::vector<double> dvalues;
    mutable std::vector<double> heights;
    std::vector<std::vector<int> > m_peakHKLVec;

    std::vector<API::IPowderDiffPeakFunction_sptr> m_peakVec;

    mutable std::vector<std::map<std::string, double> > mPeakParameters; // It is in strict order with dvalues;

    /// Add background function
    void addBackgroundFunction(string backgroundtype, map<string, double> bkgdparmap);



  };

  // typedef boost::shared_ptr<LeBailFunction> LeBailFunction_sptr;


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_LEBAILFUNCTION_H_ */
