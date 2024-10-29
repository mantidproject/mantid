// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidCurveFitting/Algorithms/LeBailFunction.h"
#include "MantidCurveFitting/Functions/BackgroundFunction.h"
#include "MantidCurveFitting/Functions/ThermalNeutronBk2BkExpConvPVoigt.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/System.h"

#include <gsl/gsl_sf_erf.h>

namespace Mantid {
namespace HistogramData {
class HistogramX;
class HistogramY;
} // namespace HistogramData

namespace CurveFitting {
namespace Algorithms {

/** LeBailFit : Algorithm to do Le Bail Fit.
  The workflow and architecture of this algorithm is different from LeBailFit,
  though they hold the same interface to users.
*/

struct Parameter {
  // Regular
  std::string name;
  double curvalue = 0;
  double prevalue = 0;
  double minvalue = 0;
  double maxvalue = 0;
  bool fit = false;
  double stepsize = 0;
  double fiterror = 0;
  // Monte Carlo
  bool nonnegative = false;
  double mcA0 = 0;
  double mcA1 = 0;
  // Monte Carlo record
  double sumstepsize = 0;
  double maxabsstepsize = 0;
  double maxrecordvalue = 0;
  double minrecordvalue = 0;
  size_t numpositivemove = 0;
  size_t numnegativemove = 0;
  size_t numnomove = 0;
  int movedirection = 0;
};

class MANTID_CURVEFITTING_DLL LeBailFit final : public API::Algorithm {
public:
  /// Enumerate
  enum FunctionMode { CALCULATION, FIT, BACKGROUNDPROCESS, MONTECARLO };

  LeBailFit();

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LeBailFit"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Do LeBail Fit to a spectrum of powder diffraction data. "; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"CreateLeBailFitInput", "FitPowderDiffPeaks"}; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Diffraction\\Fitting"; }

private:
  // Implement abstract Algorithm methods
  void init() override;
  // Implement abstract Algorithm methods
  void exec() override;

  /// Process input properties
  void processInputProperties();

  //--------------  Pattern Calculation & Minimizing  -------------------
  /// Calculate LeBail pattern from from input peak parameters
  void execPatternCalculation();

  /// Calcualte background by fitting peak heights
  void execRefineBackground();

  //--------------  Functions to set up the Le Bail Fit -----------------
  /// Create LeBailFunction
  void createLeBailFunction();

  /// Crop the workspace for better usage
  API::MatrixWorkspace_sptr cropWorkspace(const API::MatrixWorkspace_sptr &inpws, size_t wsindex);

  /// Process and calculate input background
  void processInputBackground();

  //--------------  Le Bail Formular: Calculate Peak Intensities ------------
  /// Calcualte peak heights from model to data
  bool calculatePeaksIntensities(API::MatrixWorkspace_sptr dataws, size_t workspaceindex, bool zerobackground,
                                 std::vector<double> &allpeaksvalues);

  //--------------  Import and Export ---------------------------------------
  /// Import peak parameters
  void parseInstrumentParametersTable();

  /// Import Miller Indices (HKL)
  void parseBraggPeaksParametersTable();

  /// Parse content in a table workspace to vector for background parameters
  void parseBackgroundTableWorkspace(const DataObjects::TableWorkspace_sptr &bkgdparamws,
                                     std::vector<std::string> &bkgdparnames, std::vector<double> &bkgdorderparams);

  /// Create and set up output table workspace for peaks
  void exportBraggPeakParameterToTable();

  /// Output parameters (fitted or tied)
  void exportInstrumentParameterToTable(const std::map<std::string, Parameter> &parammap);

  /// Create output data workspace
  void createOutputDataWorkspace();

  //--------------  Random Walk Suite ----------------------------------------
  /// Main for random walk process
  void execRandomWalkMinimizer(size_t maxcycles, std::map<std::string, Parameter> &parammap);

  /// Work on Markov chain to 'solve' LeBail function
  void doMarkovChain(const std::map<std::string, Parameter> &parammap, const Mantid::HistogramData::HistogramX &vecX,
                     const Mantid::HistogramData::HistogramY &vecPurePeak, const std::vector<double> &vecBkgd,
                     size_t maxcycles, const Kernel::Rfactor &startR, int randomseed);

  /// Set up Monte Carlo random walk strategy
  void setupBuiltInRandomWalkStrategy();

  void setupRandomWalkStrategyFromTable(const DataObjects::TableWorkspace_sptr &tablews);

  /// Add parameter (to a vector of string/name) for MC random walk
  void addParameterToMCMinimize(std::vector<std::string> &parnamesforMC, const std::string &parname);

  /// Calculate diffraction pattern in Le Bail algorithm for MC Random walk
  bool calculateDiffractionPattern(const Mantid::HistogramData::HistogramX &vecX,
                                   const Mantid::HistogramData::HistogramY &vecY, bool inputraw, bool outputwithbkgd,
                                   const Mantid::HistogramData::HistogramY &vecBkgd, std::vector<double> &values,
                                   Kernel::Rfactor &rfactor);

  /// Determine whether the proposed value should be accepted or denied
  bool acceptOrDeny(Kernel::Rfactor currR, Kernel::Rfactor newR);

  /// Propose new parameters
  bool proposeNewValues(const std::vector<std::string> &mcgroup, Kernel::Rfactor r,
                        std::map<std::string, Parameter> &curparammap, std::map<std::string, Parameter> &newparammap,
                        bool prevBetterRwp);

  ///  Limit proposed value in the specified boundary
  double limitProposedValueInBound(const Parameter &param, double newvalue, double direction, int choice);

  /// Book keep the (sopposed) best MC result
  void bookKeepBestMCResult(const std::map<std::string, Parameter> &parammap, const std::vector<double> &bkgddata,
                            Kernel::Rfactor rfactor, size_t istep);

  /// Apply the value of parameters in the source to target
  void applyParameterValues(const std::map<std::string, Parameter> &srcparammap,
                            std::map<std::string, Parameter> &tgtparammap);

  /// Store/buffer current background parameters
  void storeBackgroundParameters(std::vector<double> &bkgdparamvec);

  /// Restore/recover the buffered background parameters to m_background
  /// function
  void recoverBackgroundParameters(const std::vector<double> &bkgdparamvec);

  /// Propose new background parameters
  void proposeNewBackgroundValues();

  //--------------------------------------------------------------------------------------------

  /// Le Bail Function (Composite)
  LeBailFunction_sptr m_lebailFunction;

  /// Instance data
  API::MatrixWorkspace_sptr m_dataWS;
  DataObjects::Workspace2D_sptr m_outputWS;
  DataObjects::TableWorkspace_sptr parameterWS;
  DataObjects::TableWorkspace_sptr reflectionWS;

  size_t m_wsIndex;

  double m_startX, m_endX;

  /// Input Bragg peak information for future processing;
  std::vector<std::pair<std::vector<int>, double>> m_inputPeakInfoVec;

  /// Background function
  Functions::BackgroundFunction_sptr m_backgroundFunction;

  /// Function parameters updated by fit
  std::map<std::string, Parameter> m_funcParameters; // char = f: fit... = t: tie to value
  /// Input function parameters that are stored for reference
  std::map<std::string, double> m_origFuncParameters;

  /// Convert a map of Parameter to a map of double
  std::map<std::string, double> convertToDoubleMap(std::map<std::string, Parameter> &inmap);

  /// =============================    =========================== ///

  std::string m_peakType;

  /// Background type
  std::string m_backgroundType;

  /// Background polynomials
  std::vector<double> m_backgroundParameters;
  std::vector<std::string> m_backgroundParameterNames;
  unsigned int m_bkgdorder;

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

/// Write a set of (XY) data to a column file
void writeRfactorsToFile(const std::vector<double> &vecX, const std::vector<Kernel::Rfactor> &vecR,
                         const std::string &filename);

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
