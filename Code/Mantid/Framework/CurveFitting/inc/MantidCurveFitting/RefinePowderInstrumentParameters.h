#ifndef MANTID_CURVEFITTING_REFINEPOWDERINSTRUMENTPARAMETERS_H_
#define MANTID_CURVEFITTING_REFINEPOWDERINSTRUMENTPARAMETERS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidCurveFitting/BackgroundFunction.h"
#include "MantidCurveFitting/Polynomial.h"
#include "MantidCurveFitting/BackToBackExponential.h"
#include "MantidCurveFitting/ThermalNeutronDtoTOFFunction.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionValues.h"

using namespace std;
using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::API;

namespace Mantid
{
namespace CurveFitting
{

  /** RefinePowderInstrumentParameters : Algorithm to refine instrument geometry parameters only.
    This algorithm is the second part of the algorithm suite.
    It must use the output from FitPowderDiffPeaks() as the inputs.



    [ASSUMPTIONS]
    1.   CYRSTAL LATTICE IS CORRECT!  AS FOR FITTING INSTRUMENT PARAMETER, IT IS A GIVEN VALUE.
    
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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */  
  class DLLExport RefinePowderInstrumentParameters : public API::Algorithm
  {    
  public:
    RefinePowderInstrumentParameters();
    virtual ~RefinePowderInstrumentParameters();

    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "RefinePowderInstrumentParameters";}

    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return 2;}

    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "Diffraction";}

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    // Implement abstract Algorithm methods
    void init();
    // Implement abstract Algorithm methods
    void exec();

    //----------------  Processing Input ---------------------
    /// Import instrument parameter from table (workspace)
    void importParametersFromTable(DataObjects::TableWorkspace_sptr parameterWS, std::map<std::string, double>& parameters);

    /// Import the Monte Carlo related parameters from table
    void importMonteCarloParametersFromTable(TableWorkspace_sptr tablews, vector<string> parameternames,
                                             vector<double>& stepsizes, vector<double>& lowerbounds,
                                             vector<double>& upperbounds);

    /// Generate (output) workspace of peak centers
    void genPeakCentersWorkspace(bool montecarlo, size_t numbestfit);

    /// Generate peaks from table (workspace)
    void genPeaksFromTable(DataObjects::TableWorkspace_sptr peakparamws);

    //---------------  Processing Output ------------------
    /// Generate (output) table worksspace for instrument parameters
    DataObjects::TableWorkspace_sptr genOutputInstrumentParameterTable();

    /// Generate an output table workspace for N best fitting result
    DataObjects::TableWorkspace_sptr genMCResultTable();

    //--------------- Fit and MC methods -------------------
    /// Fit instrument geometry parameters by ThermalNeutronDtoTOFFunction
    void fitInstrumentParameters();

    /// Parse Fit() output parameter workspace
    std::string parseFitParameterWorkspace(API::ITableWorkspace_sptr paramws);

    /// Parse the fitting result
    std::string parseFitResult(API::IAlgorithm_sptr fitalg, double& chi2);

    /// Set up and run a monte carlo simulation to refine the peak parameters
    void refineInstrumentParametersMC(TableWorkspace_sptr parameterWS);

    /// Core Monte Carlo random walk on parameter-space
    void doParameterSpaceRandomWalk(vector<string> parnames, vector<double> lowerbounds,
                                    vector<double> upperbounds, vector<double> stepsizes, size_t maxsteps, double stepsizescalefactor);

    /// Get the names of the parameters of D-TOF conversion function
    void getD2TOFFuncParamNames(vector<string>& parnames);

    /// Calculate the value and chi2
    double calculateD2TOFFunction(FunctionDomain1DVector domain, FunctionValues& values, const MantidVec &rawY, const MantidVec& rawE);

    /// Calculate d-space value from peak's miller index for thermal neutron
    double calculateDspaceValue(std::vector<int> hkl, double lattice);

    /// Output Workspace containing the dspacing ~ TOF peak positions
    DataObjects::Workspace2D_sptr dataWS;

    /// Map for all peaks to fit individually
    std::map<std::vector<int>, CurveFitting::BackToBackExponential_sptr> mPeaks;

    /// Map for all peaks' error (fitted vs. experimental): [HKL]: Chi^2
    std::map<std::vector<int>, double> mPeakErrors;

    /// Map for function (instrument parameter)
    std::map<std::string, double> mFuncParameters;
    /// Map to store the original (input) parameters
    std::map<std::string, double> mOrigParameters;

    /// Peak function parameter names
    vector<string> mPeakFunctionParameterNames;
    /// N sets of the peak parameter values for the best N chi2.  It is paired with mPeakFunctionParameterNames
    std::vector<std::pair<double, std::vector<double> > > mBestParameters;

    /// Minimum allowed sigma of a peak
    double mMinSigma;

    /// Minimum number of fitted peaks for refinement
    size_t mMinNumFittedPeaks;

    /// Maximum number of data stored
    size_t mMaxNumberStoredParameters;

    ///
    CurveFitting::ThermalNeutronDtoTOFFunction_sptr mFunction;

  };

  /** Formular for linear iterpolation: X = [(xf-x0)*Y - (xf*y0-x0*yf)]/(yf-y0)
    */
  inline double linearInterpolateX(double x0, double xf, double y0, double yf, double y)
  {
    double x = ((xf-x0)*y - (xf*y0-x0*yf))/(yf-y0);
    return x;
  }

  /** Formula for linear interpolation: Y = ( (xf*y0-x0*yf) + x*(yf-y0) )/(xf-x0)
    */
  inline double linearInterpolateY(double x0, double xf, double y0, double yf, double x)
  {
    double y = ((xf*y0-x0*yf) + x*(yf-y0))/(xf-x0);
    return y;
  }

} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_REFINEPOWDERINSTRUMENTPARAMETERS_H_ */
