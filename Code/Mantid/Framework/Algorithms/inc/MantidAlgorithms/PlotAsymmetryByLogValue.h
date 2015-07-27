#ifndef MANTID_ALGORITHM_PLOTASYMMETRYBULOGVALUE_H_
#define MANTID_ALGORITHM_PLOTASYMMETRYBULOGVALUE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"

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
<LI> ForwardSpectra - The detector number of the first group </LI>
<LI> BackwardSpectra - The detector number of the second group </LI>
<LI> Alpha - ?? </LI>
</UL>


@author
@date 11/07/2008

Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport PlotAsymmetryByLogValue : public API::Algorithm {
public:
  /// Default constructor
  PlotAsymmetryByLogValue();
  /// Destructor
  virtual ~PlotAsymmetryByLogValue(){};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "PlotAsymmetryByLogValue"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Calculates asymmetry for a series of log values";
  }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Muon"; }

private:
  // Overridden Algorithm methods
  void init();
  void exec();
  // Load run, apply dead time corrections and detector grouping
  API::Workspace_sptr doLoad (int64_t runNumber );
  // Analyse the asymmetry for a loaded run
  void doAnalysis(API::Workspace_sptr loadedWs, std::vector<double> &y,
                                                std::vector<double> &e);
  // Parse run names
  void parseRunNames (std::string& firstFN, std::string& lastFN, std::string& fnBase, std::string& fnExt, int& fnZeros);
  // Load dead-time corrections from specified file
  API::Workspace_sptr loadCorrectionsFromFile(const std::string &deadTimeFile);
  // Apply dead-time corrections
  void applyDeadtimeCorr (API::Workspace_sptr &loadedWs, API::Workspace_sptr deadTimes);
  /// Creates a table workspace containing custom grouping
  API::Workspace_sptr createCustomGrouping(const std::vector<int> &fwd,
                                           const std::vector<int> &bwd);
  /// Group detectors from run file
  void groupDetectors (API::Workspace_sptr &loadedWs, API::Workspace_sptr loadedDetGrouping);
  /// Calculates the asymmetry for a workspace (single period)
  void calculateAsymmetry(API::MatrixWorkspace_sptr ws, double &Y, double &E);
  /// Calculates the asymmetry for a pair of workspaces (red & green)
  void calculateAsymmetry(API::MatrixWorkspace_sptr ws_red, API::MatrixWorkspace_sptr ws_geen, double &Y, double &E);
  /// Get log value
  double getLogValue(API::Workspace_sptr ws);
  /// Populate output workspace with results
  void populateOutputWorkspace (API::MatrixWorkspace_sptr &outWS, int nplots);
  /// Check input properties
  void checkProperties ();
  /// Clear previous results
  void clearResultsFromTo (size_t is, size_t ie);

  /// Number of the first run in the set
  size_t m_is;
  /// Number of the last run in the set
  size_t m_ie;
  /// Stores base name shared by all runs
  std::string m_filenameBase;
  /// Stores extension shared by all runs
  std::string m_filenameExt;
  /// Sotres number of zeros in run name
  int m_filenameZeros;
  /// Stores property "Int"
  bool m_int;
  /// Store forward spectra
  std::vector<int> m_forward_list;
  /// Store backward spectra
  std::vector<int> m_backward_list;
  /// Store type of dead time corrections
  std::string m_dtcType;
  /// File to read corrections from
  std::string m_dtcFile;
  /// Store red period
  int m_red;
  /// Store green period
  int m_green;
  // LogValue name
  std::string m_logName;
  // LogValue function
  std::string m_logFunc;
  // Type of computation: integral or differential
  std::string m_stype;
  // Minimum time for the analysis
  double m_minTime;
  // Maximum time for the analysis
  double m_maxTime;
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_PLOTASYMMETRYBULOGVALUE_H_*/
