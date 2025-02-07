// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidMuon/DllConfig.h"

namespace Mantid {
//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace API {
class MatrixWorkspace;
}

namespace Algorithms {
/**Takes a muon workspace as input and sums all the spectra into two spectra
which represent
      the two detector groupings. The resultant spectra are used to calculate
(F-aB) / (F+aB) the results of which
      are stored in the output workspace.

Required Properties:
<UL>
<LI> InputWorkspace - The name of the Workspace2D to take as input </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the result
</LI>
<LI> ForwardSpectra - The spectrum numbers of the forward group </LI>
<LI> BackwardSpectra - The spectrum numbers of the backward group </LI>
<LI> Alpha - balance parameter </LI>
</UL>


@author
@date 11/07/2008
*/
class MANTID_MUON_DLL PlotAsymmetryByLogValue final : public API::Algorithm {
public:
  /// Default constructor
  PlotAsymmetryByLogValue();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "PlotAsymmetryByLogValue"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Calculates asymmetry for a series of log values"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"AsymmetryCalc", "CalculateMuonAsymmetry", "PlotPeakByLogValue"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Muon"; }
  std::map<std::string, std::string> validateInputs() override;
  int extractRunNumberFromRunName(std::string runName);

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  // Load run, apply dead time corrections and detector grouping
  API::Workspace_sptr doLoad(const std::string &fileName);
  // Analyse loaded run
  void doAnalysis(const API::Workspace_sptr &loadedWs, size_t index);
  // Parse run names
  void parseRunNames(std::string &firstFN, std::string &lastFN, std::string &fnBase, std::string &fnExt, int &fnZeros);
  // Load dead-time corrections from specified file
  API::Workspace_sptr loadCorrectionsFromFile(const std::string &deadTimeFile);
  // Apply dead-time corrections
  void applyDeadtimeCorr(API::Workspace_sptr &loadedWs, const API::Workspace_sptr &deadTimes);
  /// Create custom detector grouping
  API::Workspace_sptr createCustomGrouping(const std::vector<int> &fwd, const std::vector<int> &bwd);
  /// Group detectors
  void groupDetectors(API::Workspace_sptr &loadedWs, const API::Workspace_sptr &grouping);
  /// Calculate the integral asymmetry for a workspace (single period)
  void calcIntAsymmetry(const API::MatrixWorkspace_sptr &ws, double &Y, double &E);
  /// Calculate the integral asymmetry for a workspace (red & green)
  void calcIntAsymmetry(const API::MatrixWorkspace_sptr &ws_red, const API::MatrixWorkspace_sptr &ws_green, double &Y,
                        double &E);
  /// Group detectors
  void groupDetectors(API::MatrixWorkspace_sptr &ws, const std::vector<int> &spectraList);
  /// Get log value
  double getLogValue(const API::MatrixWorkspace &ws);
  /// Populate output workspace with results
  void populateOutputWorkspace(API::MatrixWorkspace_sptr &outWS, int nplots, const std::string &units);
  /// get log units
  const std::string getLogUnits(const std::string &fileName);
  /// Populate the hidden ws storing current results
  void saveResultsToADS(API::MatrixWorkspace_sptr &outWS, int nplots);
  /// Check input properties
  void checkProperties(size_t &firstRunNumber, size_t &lastRunNumber);
  /// Get path to the direcotry from a file name
  std::string getDirectoryFromFileName(const std::string &fileName) const;
  /// Uses FirstRun and LastRun to populate filenames vector
  void populateFileNamesFromFirstLast(std::string firstRun, std::string lastRun);

  /// Properties needed to load a run
  /// Stores base name shared by all runs
  std::string m_filenameBase;
  /// Stores extension shared by all runs
  std::string m_filenameExt;
  /// Stores number of zeros in run name
  int m_filenameZeros;
  /// Store type of dead time corrections
  std::string m_dtcType;
  /// File to read corrections from
  std::string m_dtcFile;
  /// Store forward spectra
  std::vector<int> m_forward_list;
  /// Store backward spectra
  std::vector<int> m_backward_list;
  /// Store workspaces
  std::vector<std::string> m_fileNames;
  /// The map holding extracted run numbers from filenames
  std::map<std::string, int> m_rmap;

  /// Properties needed to analyse a run
  /// Type of calculation: integral or differential
  bool m_int;
  /// Red period
  int m_red;
  /// Green period
  int m_green;
  /// Minimum time for the analysis
  double m_minTime;
  /// Maximum time for the analysis
  double m_maxTime;
  /// Balance parameter
  double m_alpha;

  /// Properties needed to get the log value
  // LogValue name
  std::string m_logName;
  /// LogValue function
  std::string m_logFunc;

  /// Mantid maps to store intermediate results
  // Map to store log value
  std::map<size_t, double> m_logValue;
  // Red values
  std::map<size_t, double> m_redY;
  std::map<size_t, double> m_redE;
  // Green values
  std::map<size_t, double> m_greenY;
  std::map<size_t, double> m_greenE;
  // Sum values (Red + Green)
  std::map<size_t, double> m_sumY;
  std::map<size_t, double> m_sumE;
  // Diff values (Red - Green)
  std::map<size_t, double> m_diffY;
  std::map<size_t, double> m_diffE;

  // String containing all the properties
  std::string m_allProperties;
  // Name of the hidden ws
  std::string m_currResName;
  /// Cached start time for first run
  int64_t m_firstStart_ns;
};

} // namespace Algorithms
} // namespace Mantid
