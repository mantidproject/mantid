#ifndef MANTID_CURVEFITTING_LEBAILFIT_H_
#define MANTID_CURVEFITTING_LEBAILFIT_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidCurveFitting/LeBailFunction.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidCurveFitting/ThermalNeutronBk2BkExpConvPVoigt.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidCurveFitting/BackgroundFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IFunction.h"
#include <gsl/gsl_sf_erf.h>

namespace Mantid {
namespace CurveFitting {

/** LeBailFit : Algorithm to do Le Bail Fit.
  The workflow and architecture of this algorithm is different from LeBailFit,
  though they hold the same interface to users.

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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

struct Parameter {
  // Regular
  std::string name;
  double curvalue;
  double prevalue;
  double minvalue;
  double maxvalue;
  bool fit;
  double stepsize;
  double fiterror;
  // Monte Carlo
  bool nonnegative;
  double mcA0;
  double mcA1;
  // Monte Carlo record
  double sumstepsize;
  double maxabsstepsize;
  double maxrecordvalue;
  double minrecordvalue;
  size_t numpositivemove;
  size_t numnegativemove;
  size_t numnomove;
  int movedirection;
};

class DLLExport LeBailFit : public API::Algorithm {
public:
  /// Enumerate
  enum FunctionMode { CALCULATION, FIT, BACKGROUNDPROCESS, MONTECARLO };

  LeBailFit();
  virtual ~LeBailFit();

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "LeBailFit"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Do LeBail Fit to a spectrum of powder diffraction data. ";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }

  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Diffraction"; }

private:
  // Implement abstract Algorithm methods
  void init();
  // Implement abstract Algorithm methods
  void exec();

  /// Process input properties
  void processInputProperties();

  //--------------  Pattern Calculation & Minimizing  -------------------
  /// Calculate LeBail pattern from from input peak parameters
  void execPatternCalculation();

  /// LeBailFit
  void execLeBailFit();

  /// Do 1 iteration in Le Bail fit
  bool do1StepLeBailFit(std::map<std::string, Parameter> &parammap);

  /// Set up fit/tie/parameter values to all peaks functions (calling GSL
  /// library)
  void setLeBailFitParameters();

  /// Do 1 fit on LeBailFunction
  bool fitLeBailFunction(std::map<std::string, Parameter> &parammap);

  /// Calcualte background by fitting peak heights
  void execRefineBackground();

  //--------------  Functions to set up the Le Bail Fit -----------------
  /// Create LeBailFunction
  void createLeBailFunction();

  /// Crop the workspace for better usage
  API::MatrixWorkspace_sptr cropWorkspace(API::MatrixWorkspace_sptr inpws,
                                          size_t wsindex);

  //-------------- Operation with Bragg Peaks -------------------------------
  /// Create a list of peaks
  bool generatePeaksFromInput();

  /// Process and calculate input background
  void processInputBackground();

  /// Examine whether the insturment parameter set to a peak can cause a valid
  /// set of peak profile of that peak
  bool examinInstrumentParameterValid(
      CurveFitting::ThermalNeutronBk2BkExpConvPVoigt_sptr peak, double &d_h,
      double &tof_h, std::string &errmsg);

  /// Set parameters to each peak
  // void setPeakParameters(ThermalNeutronBk2BkExpConvPVoigt_sptr peak,
  // map<string, Parameter> parammap,
  // double peakheight, bool setpeakheight);

  /// From table/map to set parameters to all peaks.
  // void setPeaksParameters(vector<pair<double,
  // ThermalNeutronBk2BkExpConvPVoigt_sptr> > peaks,
  // map<std::string, Parameter> parammap,
  // double peakheight, bool setpeakheight);

  /// Check whether a parameter is a profile parameter
  bool hasProfileParameter(std::string paramname);

  //--------------  Le Bail Formular: Calculate Peak Intensities ------------
  /// Calcualte peak heights from model to data
  bool calculatePeaksIntensities(API::MatrixWorkspace_sptr dataws,
                                 size_t workspaceindex, bool zerobackground,
                                 std::vector<double> &allpeaksvalues);

  /// Group peaks together
  void groupPeaks(std::vector<std::vector<
      std::pair<double, CurveFitting::ThermalNeutronBk2BkExpConvPVoigt_sptr>>> &
                      peakgroupvec);

  /// Calcualate the peak heights of a group of overlapped peaks
  bool calculateGroupPeakIntensities(
      std::vector<std::pair<
          double, CurveFitting::ThermalNeutronBk2BkExpConvPVoigt_sptr>>
          peakgroup,
      API::MatrixWorkspace_sptr dataws, size_t wsindex, bool zerobackground,
      std::vector<double> &allpeaksvalues);

  //--------------  Import and Export ---------------------------------------
  /// Import peak parameters
  void parseInstrumentParametersTable();

  /// Import Miller Indices (HKL)
  void parseBraggPeaksParametersTable();

  /// Parse content in a table workspace to vector for background parameters
  void
  parseBackgroundTableWorkspace(DataObjects::TableWorkspace_sptr bkgdparamws,
                                std::vector<std::string> &bkgdparnames,
                                std::vector<double> &bkgdorderparams);

  /// Create and set up output table workspace for peaks
  void exportBraggPeakParameterToTable();

  /// Output parameters (fitted or tied)
  void
  exportInstrumentParameterToTable(std::map<std::string, Parameter> parammap);

  /// Create output data workspace
  void createOutputDataWorkspace();

  /// Fake calculated pattern
  // Disabled void writeFakedDataToOutputWS(size_t workspaceindex, int
  // functionmode);

  /// Write out (domain, values) to output workspace
  void writeToOutputWorkspace(size_t wsindex,
                              API::FunctionDomain1DVector domain,
                              API::FunctionValues values);
  // void writeToOutputWorkspace(API::FunctionDomain1DVector domain,
  // API::FunctionValues values);

  /// Write input data and difference to output workspace
  void writeInputDataNDiff(size_t workspaceindex,
                           API::FunctionDomain1DVector domain);

  //--------------  Random Walk Suite ----------------------------------------
  /// Main for random walk process
  void execRandomWalkMinimizer(size_t maxcycles,
                               std::map<std::string, Parameter> &parammap);

  /// Work on Markov chain to 'solve' LeBail function
  void doMarkovChain(const std::map<std::string, Parameter> &parammap,
                     const std::vector<double> &vecX,
                     const std::vector<double> &vecPurePeak,
                     const std::vector<double> &vecBkgd, size_t maxcycles,
                     const Kernel::Rfactor &startR, int randomseed);

  /// Set up Monte Carlo random walk strategy
  void setupBuiltInRandomWalkStrategy();

  void
  setupRandomWalkStrategyFromTable(DataObjects::TableWorkspace_sptr tablews);

  /// Add parameter (to a vector of string/name) for MC random walk
  void addParameterToMCMinimize(std::vector<std::string> &parnamesforMC,
                                std::string parname);

  /// Calculate diffraction pattern in Le Bail algorithm for MC Random walk
  bool calculateDiffractionPattern(const MantidVec &vecX, const MantidVec &vecY,
                                   bool inputraw, bool outputwithbkgd,
                                   const MantidVec &vecBkgd, MantidVec &values,
                                   Kernel::Rfactor &rfactor);

  /// Calculate powder diffraction statistic Rwp
  // void calculatePowderPatternStatistic(const MantidVec &values, const
  // vector<double> &background,
  //                                   double &rwp, double &rp);

  /// Determine whether the proposed value should be accepted or denied
  bool acceptOrDeny(Kernel::Rfactor currR, Kernel::Rfactor newR);

  /// Propose new parameters
  bool proposeNewValues(std::vector<std::string> mcgroup, Kernel::Rfactor r,
                        std::map<std::string, Parameter> &curparammap,
                        std::map<std::string, Parameter> &newparammap,
                        bool prevBetterRwp);

  ///  Limit proposed value in the specified boundary
  double limitProposedValueInBound(Parameter param, double newvalue,
                                   double direction, int choice);

  /// Book keep the (sopposed) best MC result
  void bookKeepBestMCResult(std::map<std::string, Parameter> parammap,
                            const std::vector<double> &bkgddata,
                            Kernel::Rfactor rfactor, size_t istep);

  /// Apply the value of parameters in the source to target
  void applyParameterValues(std::map<std::string, Parameter> &srcparammap,
                            std::map<std::string, Parameter> &tgtparammap);

  //--------------  Background function Suite
  //----------------------------------------
  /// Re-fit background according to the new values
  void fitBackground(size_t wsindex, API::FunctionDomain1DVector domain,
                     API::FunctionValues values,
                     std::vector<double> &background);

  /// Smooth background by exponential smoothing algorithm
  void smoothBackgroundExponential(size_t wsindex,
                                   API::FunctionDomain1DVector domain,
                                   API::FunctionValues peakdata,
                                   std::vector<double> &background);

  /// Smooth background by fitting the background to specified background
  /// function
  void smoothBackgroundAnalytical(size_t wsindex,
                                  API::FunctionDomain1DVector domain,
                                  API::FunctionValues peakdata,
                                  std::vector<double> &background);

  /// Store/buffer current background parameters
  void storeBackgroundParameters(std::vector<double> &bkgdparamvec);

  /// Restore/recover the buffered background parameters to m_background
  /// function
  void recoverBackgroundParameters(const std::vector<double> &bkgdparamvec);

  /// Propose new background parameters
  void proposeNewBackgroundValues();

  /// Minimize a give function
  bool minimizeFunction(API::MatrixWorkspace_sptr dataws, size_t wsindex,
                        double tofmin, double tofmax, std::string minimizer,
                        double dampfactor, int numiteration,
                        std::string &status, double &chi2,
                        bool outputcovarmatrix);

  //--------------------------------------------------------------------------------------------

  /// Map to contain function variables
  // map<string, Parameter> m_functionParameters;

  /// Le Bail Function (Composite) (old: API::CompositeFunction_sptr
  /// m_lebailFunction)
  LeBailFunction_sptr m_lebailFunction;

  /// Instance data
  API::MatrixWorkspace_sptr m_dataWS;
  DataObjects::Workspace2D_sptr m_outputWS;
  DataObjects::TableWorkspace_sptr parameterWS;
  DataObjects::TableWorkspace_sptr reflectionWS;

  size_t m_wsIndex;

  double m_startX, m_endX;

  /// Peaks about input and etc.
  //  These two are used for sorting peaks.
  //  std::vector<int> mPeakHKL2; // Peak's h^2+k^2+l^2: seaving as key for
  //  mPeakHeights adn mPeaks

  // std::vector<std::vector<int> > mPeakHKLs;

  /// Input Bragg peak information for future processing;
  std::vector<std::pair<std::vector<int>, double>> m_inputPeakInfoVec;

  /// Vector of pairs of d-spacing and peak reference for all Bragg peaks
  // std::vector<pair<double, ThermalNeutronBk2BkExpConvPVoigt_sptr> >
  // m_dspPeaks;

  /* Phase out (HKL)^2 may not be in order of peak position if lattice is not
  cubic
  std::map<int, CurveFitting::ThermalNeutronBk2BkExpConvPVoigt_sptr> m_peaks;
  This can be assimilated to m_dspPeaks()
  std::map<int, double> mPeakHeights;
  -------------------------------*/

  /// Background function
  CurveFitting::BackgroundFunction_sptr m_backgroundFunction;

  /// Function parameters updated by fit
  std::map<std::string, Parameter>
      m_funcParameters; // char = f: fit... = t: tie to value
  /// Input function parameters that are stored for reference
  std::map<std::string, double> m_origFuncParameters;
  /// Peak parameters list
  // std::vector<std::string> m_peakParameterNames; // Peak parameters' names of
  // the peak

  /// Parameter error: this should be a field in m_funcParameters
  // std::map<std::string, double> mFuncParameterErrors;

  /// Calculate some statistics for fitting/calculating result
  // void calChiSquare();

  /// Convert a map of Parameter to a map of double
  std::map<std::string, double>
  convertToDoubleMap(std::map<std::string, Parameter> &inmap);

  /// =============================    =========================== ///

  std::string m_peakType;

  /// Vector for miller indexes
  // std::vector<std::vector<int> > m_vecHKL;

  /// Background type
  std::string m_backgroundType;

  /// Background polynomials
  std::vector<double> m_backgroundParameters;
  std::vector<std::string> m_backgroundParameterNames;
  unsigned int m_bkgdorder;

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

  /// Flag to show whether the input profile parameters are physical to all
  /// peaks
  bool m_inputParameterPhysical;

  /// Fit mode
  FunctionMode m_fitMode;

  double m_indicatePeakHeight;

  //-------------------------- Monte Carlo Variables--------------------------
  std::map<int, std::vector<std::string>> m_MCGroups;
  size_t m_numMCGroups;

  double m_bestRwp;
  double m_bestRp;

  std::map<std::string, Parameter> m_bestParameters;
  std::vector<double> m_bestBackgroundData;
  size_t m_bestMCStep;

  /// Number of minimization steps.  For both MC and regular
  size_t m_numMinimizeSteps;

  /// Monte Carlo temperature
  double m_Temperature;

  /// Flag to use Annealing Simulation (i.e., use automatic adjusted
  /// temperature)
  bool m_useAnnealing;

  /// Monte Carlo algorithm
  enum { RANDOMWALK, DRUNKENWALK } m_walkStyle;

  /// Minimum height of a peak to be counted in smoothing background
  double m_minimumPeakHeight;

  /// Flag to allow peaks with duplicated (HKL)^2 in input .hkl file
  bool m_tolerateInputDupHKL2Peaks;

  //------------------------ Background Refinement Variables
  //-----------------------
  std::vector<std::string> m_bkgdParameterNames;
  size_t m_numberBkgdParameters;
  std::vector<double> m_bkgdParameterBuffer;
  std::vector<double> m_bestBkgdParams;
  int m_roundBkgd;
  std::vector<double> m_bkgdParameterStepVec;

  double m_peakCentreTol;
};

/// Auxiliary.  Split composite function name to function index and parameter
/// name
void parseCompFunctionParameterName(std::string fullparname,
                                    std::string &parname, size_t &funcindex);

/// Write domain and value to a column file
void exportDomainValueToFile(API::FunctionDomain1DVector domain,
                             API::FunctionValues values, std::string filename);

/// Write a set of (XY) data to a column file
void writeRfactorsToFile(std::vector<double> vecX,
                         std::vector<Kernel::Rfactor> vecR,
                         std::string filename);

/// Convert a Table to space to some vectors of maps
void convertTableWorkspaceToMaps(
    DataObjects::TableWorkspace_sptr tablews,
    std::vector<std::map<std::string, int>> intmaps,
    std::vector<std::map<std::string, std::string>> strmaps,
    std::vector<std::map<std::string, double>> dblmaps);

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_LEBAILFIT_H_ */
