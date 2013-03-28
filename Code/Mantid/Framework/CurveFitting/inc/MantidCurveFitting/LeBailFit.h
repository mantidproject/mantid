#ifndef MANTID_CURVEFITTING_LEBAILFIT_H_
#define MANTID_CURVEFITTING_LEBAILFIT_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidCurveFitting/ThermalNeutronBk2BkExpConvPVoigt.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidCurveFitting/BackgroundFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IFunction.h"
#include <gsl/gsl_sf_erf.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::CurveFitting;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

using namespace std;

namespace Mantid
{
namespace CurveFitting
{

  /** LeBailFit : Algorithm to do Le Bail Fit.
    The workflow and architecture of this algorithm is different from LeBailFit,
    though they hold the same interface to users.
    
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

  struct Parameter
  {
    // Regular
    std::string name;
    double value;
    double prevalue;
    double minvalue;
    double maxvalue;
    bool fit;
    double stepsize;
    double error;
    // Monte Carlo
    bool nonnegative;
    double mcA0;
    double mcA1;
    // Monte Carlo record
    double sumstepsize;
    double maxabsstepsize;
    size_t numpositivemove;
    size_t numnegativemove;
    size_t numnomove;
    int movedirection;
  };

  class DLLExport LeBailFit : public API::Algorithm
  {
  public:
    /// Enumerate
    enum FunctionMode
    {
      CALCULATION,
      FIT,
      BACKGROUNDPROCESS,
      MONTECARLO
    };

    LeBailFit();
    virtual ~LeBailFit();

    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "LeBailFit";}

    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return 1;}

    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "Diffraction";}

  private:
    /// Sets documentation strings for this algorithm
    virtual void initDocs();
    // Implement abstract Algorithm methods
    void init();
    // Implement abstract Algorithm methods
    void exec();

    /// Process input properties
    void processInputProperties();

    //--------------  Pattern Calculation & Minimizing  -------------------
    /// Calculate LeBail pattern from from input peak parameters
    void execPatternCalculation();

    /// Calculate diffraction pattern
    bool calculateDiffractionPattern(MatrixWorkspace_sptr dataws, size_t workspaceindex,
                                     FunctionDomain1DVector domain, FunctionValues &values,
                                     map<string, Parameter> parammap, bool recalpeakintesity);

    /// LeBailFit
    void execLeBailFit();

    /// Do 1 iteration in Le Bail fit
    bool do1StepLeBailFit(std::map<std::string, Parameter>& parammap);

    /// Set up Lebail
    void setLeBailFitParameters();

    /// Do 1 fit on LeBailFunction
    bool fitLeBailFunction(std::map<std::string, Parameter> &parammap);

    /// Minimize a give function
    bool minimizeFunction(MatrixWorkspace_sptr dataws, size_t wsindex, IFunction_sptr function,
                          double tofmin, double tofmax, string minimizer, double dampfactor,
                          int numiteration, string &status, double &chi2, bool outputcovarmatrix);

    /// Calcualte background by fitting peak heights
    void calBackground(size_t workspaceindex);

    //--------------  Functions to set up the Le Bail Fit -----------------
    /// Create LeBailFunction
    void createLeBailFunction(std::string backgroundtype, std::vector<double>& bkgdorderparams,
                              DataObjects::TableWorkspace_sptr bkgdparamws);

    /// Crop the workspace for better usage
    API::MatrixWorkspace_sptr cropWorkspace(API::MatrixWorkspace_sptr inpws, size_t wsindex);

    //-------------- Operation with Bragg Peaks -------------------------------
    /// Create a list of peaks
    bool generatePeaksFromInput();

    /// Examine whether the insturment parameter set to a peak can cause a valid set of peak profile of that peak
    bool examinInstrumentParameterValid(ThermalNeutronBk2BkExpConvPVoigt_sptr peak,
                                        double &d_h, double &tof_h, string &errmsg);

    /// Set parameters to each peak
    void setPeakParameters(ThermalNeutronBk2BkExpConvPVoigt_sptr peak, map<string, Parameter> parammap,
                           double peakheight, bool setpeakheight);

    /// From table/map to set parameters to all peaks.
    void setPeaksParameters(vector<pair<double, ThermalNeutronBk2BkExpConvPVoigt_sptr> > peaks,
                            map<std::string, Parameter> parammap,
                            double peakheight, bool setpeakheight);

    //--------------  Le Bail Formular: Calculate Peak Intensities ------------
    /// Calcualte peak heights from model to data
    bool calculatePeaksIntensities(MatrixWorkspace_sptr dataws, size_t workspaceindex, bool zerobackground,
                                   vector<double>& allpeaksvalues);

    /// Group peaks together
    void groupPeaks(vector<vector<pair<double, ThermalNeutronBk2BkExpConvPVoigt_sptr> > > &peakgroupvec);

    /// Calcualate the peak heights of a group of overlapped peaks
    bool calculateGroupPeakIntensities(vector<pair<double, ThermalNeutronBk2BkExpConvPVoigt_sptr> > peakgroup,
                                       MatrixWorkspace_sptr dataws, size_t wsindex, bool zerobackground,
                                       vector<double>& allpeaksvalues);

    //--------------  Import and Export ---------------------------------------
    /// Import peak parameters
    void parseInstrumentParametersTable();

    /// Import Miller Indices (HKL)
    void parseBraggPeaksParametersTable();

    /// Parse content in a table workspace to vector for background parameters
    void parseBackgroundTableWorkspace(TableWorkspace_sptr bkgdparamws, vector<double>& bkgdorderparams);

    /// Create and set up output table workspace for peaks
    void exportBraggPeakParameterToTable();

    /// Output parameters (fitted or tied)
    void exportInstrumentParameterToTable(map<string, Parameter> parammap);

    /// Create output data workspace
    void createOutputDataWorkspace();

    /// Fake calculated pattern
    void writeFakedDataToOutputWS(size_t workspaceindex, int functionmode);

    /// Write out (domain, values) to output workspace
    void writeToOutputWorkspace(size_t wsindex, FunctionDomain1DVector domain,  FunctionValues values);
    // void writeToOutputWorkspace(API::FunctionDomain1DVector domain,  API::FunctionValues values);

    /// Write input data and difference to output workspace
    void writeInputDataNDiff(size_t workspaceindex, API::FunctionDomain1DVector domain);

    //--------------  Random Walk Suite ----------------------------------------
    /// Main for random walk process
    void execRandomWalkMinimizer(size_t maxcycles, map<string, Parameter> &parammap);

    /// Set up Monte Carlo random walk strategy
    void setupRandomWalkStrategy();

    /// Add parameter (to a vector of string/name) for MC random walk
    void addParameterToMCMinimize(vector<string>& parnamesforMC, string parname);

    /// Calculate diffraction pattern in Le Bail algorithm for MC Random walk
    bool calculateDiffractionPatternMC(MatrixWorkspace_sptr dataws,
                                       size_t wsindex,
                                       map<string, Parameter> funparammap,
                                       MantidVec &background, MantidVec &values,
                                       double &rwp, double &rp);

    /// Calculate powder diffraction statistic Rwp
    //void calculatePowderPatternStatistic(const MantidVec &values, const vector<double> &background,
      //                                   double &rwp, double &rp);

    /// Determine whether the proposed value should be accepted or denied
    bool acceptOrDeny(double currwp, double newrwp);

    /// Propose new parameters
    bool proposeNewValues(vector<string> mcgroup, double m_totRwp,
                          map<string, Parameter> &curparammap, map<string, Parameter> &newparammap, bool prevBetterRwp);

    /// Book keep the (sopposed) best MC result
    void bookKeepBestMCResult(map<string, Parameter> parammap,
                              vector<double> &bkgddata, double rwp, size_t istep);

    /// Apply the value of parameters in the source to target
    void applyParameterValues(map<string, Parameter> &srcparammap,
                              map<string, Parameter> &tgtparammap);    

    //--------------  Background function Suite ----------------------------------------
    /// Re-fit background according to the new values
    void fitBackground(size_t wsindex, FunctionDomain1DVector domain,
                        FunctionValues values, vector<double>& background);

    /// Smooth background by exponential smoothing algorithm
    void smoothBackgroundExponential(size_t wsindex, FunctionDomain1DVector domain,
                                     FunctionValues peakdata, vector<double>& background);

    /// Smooth background by fitting the background to specified background function
    void smoothBackgroundAnalytical(size_t wsindex, FunctionDomain1DVector domain,
                                    FunctionValues peakdata, vector<double>& background);

    //--------------------------------------------------------------------------

    /// Instance data
    API::MatrixWorkspace_sptr m_dataWS;
    DataObjects::Workspace2D_sptr m_outputWS;
    DataObjects::TableWorkspace_sptr parameterWS;
    DataObjects::TableWorkspace_sptr reflectionWS;

    size_t m_wsIndex;

    /// Peaks about input and etc.
    //  These two are used for sorting peaks.
    //  std::vector<int> mPeakHKL2; // Peak's h^2+k^2+l^2: seaving as key for mPeakHeights adn mPeaks

    // std::vector<std::vector<int> > mPeakHKLs;

    /// Input Bragg peak information for future processing;
    vector<pair<vector<int>, double> > m_inputPeakInfoVec;

    /// Vector of pairs of d-spacing and peak reference for all Bragg peaks
    std::vector<pair<double, ThermalNeutronBk2BkExpConvPVoigt_sptr> > m_dspPeaks;

    /* Phase out (HKL)^2 may not be in order of peak position if lattice is not cubic
    std::map<int, CurveFitting::ThermalNeutronBk2BkExpConvPVoigt_sptr> m_peaks;
    This can be assimilated to m_dspPeaks()
    std::map<int, double> mPeakHeights;
    -------------------------------*/

    /// Background function
    CurveFitting::BackgroundFunction_sptr m_backgroundFunction;
    /// Le Bail Function (Composite)
    API::CompositeFunction_sptr m_lebailFunction;
    /// Vector to hold background
    vector<double> m_backgroundVec;

    /// Function parameters updated by fit
    std::map<std::string, Parameter> m_funcParameters; // char = f: fit... = t: tie to value
    /// Input function parameters that are stored for reference
    std::map<std::string, double> m_origFuncParameters;
    /// Peak parameters list
    // std::vector<std::string> m_peakParameterNames; // Peak parameters' names of the peak

    /// Parameter error: this should be a field in m_funcParameters
    // std::map<std::string, double> mFuncParameterErrors;

    /// Calculate some statistics for fitting/calculating result
    void doResultStatistics();

    /// =============================    =========================== ///
    // size_t mWSIndexToWrite;

    /// Map to store peak group information: key (int) = (hkl)^2; value = group ID
    std::map<int, size_t> mPeakGroupMap;

    /// Map to store fitting Chi^2: key = group index; value = chi^2
    std::map<size_t, double> mPeakGroupFitChi2Map;

    /// Map to store fitting Status: key = group index; value = fit status
    std::map<size_t, std::string> mPeakGroupFitStatusMap;

    /// Peak Radius
    int mPeakRadius;

    /// Fit Chi^2
    double m_lebailFitChi2;
    double m_lebailCalChi2;

    /// Minimizer
    std::string mMinimizer;
    /// Damping factor
    double m_dampingFactor;

    /// Flag to show whether the input profile parameters are physical to all peaks
    bool m_inputParameterPhysical;

    /// Fit mode
    FunctionMode m_fitMode;

    //-------------------------- Monte Carlo Variables--------------------------
    vector<vector<string> > m_MCGroups;
    size_t m_numMCGroups;

    double m_bestRwp;
    map<string, Parameter> m_bestParameters;
    vector<double> m_bestBackgroundData;
    size_t m_bestMCStep;

    /// Number of minimization steps.  For both MC and regular
    size_t m_numMinimizeSteps;

    /// Monte Carlo temperature
    double m_Temperature;

    /// Flag to use Annealing Simulation (i.e., use automatic adjusted temperature)
    bool m_useAnnealing;

    /// Monte Carlo algorithm
    enum {RANDOMWALK, DRUNKENWALK} m_walkStyle;

    /// Minimum height of a peak to be counted in smoothing background
    double m_minimumHeight;

    /// Flag to allow peaks with duplicated (HKL)^2 in input .hkl file
    bool m_tolerateInputDupHKL2Peaks;

  };

  /// Auxiliary.  Split composite function name to function index and parameter name
  void parseCompFunctionParameterName(std::string fullparname, std::string& parname, size_t& funcindex);

  /// Write domain and value to a column file
  void exportDomainValueToFile(FunctionDomain1DVector domain, FunctionValues values, string filename);

  /// Write a set of (XY) data to a column file
  void exportXYDataToFile(vector<double> vecX, vector<double> vecY, string filename);

  /// Convert a Table to space to some vectors of maps
  void convertTableWorkspaceToMaps(TableWorkspace_sptr tablews, vector<map<string, int> > intmaps,
                                   vector<map<string, string> > strmaps, vector<map<string, double> > dblmaps);

} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_LEBAILFIT_H_ */
