// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/WorkspaceGroup_fwd.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidMuon/DllConfig.h"

namespace Mantid {
namespace Indexing {
class SpectrumNumber;
}
namespace Algorithms {

/** CalMuonDetectorPhases : Calculates asymmetry and phase for each spectra in a
  workspace
*/
class MANTID_MUON_DLL CalMuonDetectorPhases : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CalMuonDetectorPhases"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculates the asymmetry and phase for each detector in a "
           "workspace.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Muon"; }
  /// See also
  const std::vector<std::string> seeAlso() const override { return {"PhaseQuad"}; };

protected:
  /// Validate the inputs
  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialise the algorithm
  void init() override;
  /// Execute the algorithm
  void exec() override;
  /// Prepare workspace for fit by extracting data
  API::MatrixWorkspace_sptr extractDataFromWorkspace(double startTime, double endTime);
  /// Remove exponential data from workspace
  API::MatrixWorkspace_sptr removeExpDecay(const API::MatrixWorkspace_sptr &wsInput);
  /// Fit the workspace
  void fitWorkspace(const API::MatrixWorkspace_sptr &ws, double freq, const std::string &groupName,
                    const API::ITableWorkspace_sptr &resTab, API::WorkspaceGroup_sptr &resGroup);
  /// Create the fitting function as string
  std::string createFittingFunction(double freq, bool fixFreq);
  /// Extract asymmetry and phase from fitting results
  void extractDetectorInfo(API::ITableWorkspace &paramTab, API::ITableWorkspace &resultsTab,
                           const Indexing::SpectrumNumber spectrumNumber);
  /// Find frequency to use in sequential fit
  double getFrequency(const API::MatrixWorkspace_sptr &ws);
  /// Get frequency hint to use when finding frequency
  double getFrequencyHint() const;
  /// Get start time for fit
  double getStartTime() const;
  /// Get end time for fit
  double getEndTime() const;
  /// Calculate detector efficiency (alpha)
  double getAlpha(const API::MatrixWorkspace_sptr &ws, const std::vector<int> &forward,
                  const std::vector<int> &backward);
  /// Calculate asymmetry
  API::MatrixWorkspace_sptr getAsymmetry(const API::MatrixWorkspace_sptr &ws, const std::vector<int> &forward,
                                         const std::vector<int> &backward, const double alpha);
  /// Fit asymmetry to get frequency
  double fitFrequencyFromAsymmetry(const API::MatrixWorkspace_sptr &wsAsym);
  /// Find the grouping from the instrument
  void getGroupingFromInstrument(const API::MatrixWorkspace_sptr &ws, std::vector<int> &forward,
                                 std::vector<int> &backward);
  /// Report progress in GUI
  void reportProgress(const int thisSpectrum, const int totalSpectra);
  /// Pointer to input workspace
  API::MatrixWorkspace_sptr m_inputWS;
};
} // namespace Algorithms
} // namespace Mantid
