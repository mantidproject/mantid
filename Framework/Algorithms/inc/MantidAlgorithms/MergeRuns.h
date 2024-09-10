// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultiPeriodGroupAlgorithm.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"

#include <optional>

namespace Mantid {
namespace API {
class WorkspaceGroup;
}
namespace HistogramData {
class HistogramX;
}
namespace Algorithms {
/** Combines the data contained in an arbitrary number of input workspaces.
    If the input workspaces do not have common binning, the bins in the output
   workspace
    will cover the entire range of all the input workspaces, with the largest
   bin widths
    used in areas of overlap.
    The input workspaces must contain histogram data with the same number of
   spectra,
    units and instrument name in order for the algorithm to succeed.
    Other than this it is currently left to the user to ensure that the
   combination of the
    workspaces is a valid operation.

    Required Properties:
    <UL>
    <LI> InputWorkspaces  - The names of the input workspace as a comma
   separated list. </LI>
    <LI> OutputWorkspace - The name of the output workspace which will contain
   the combined inputs. </LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 22/09/2008
*/

namespace MergeRunsParameter {
/// MergeRuns parameter names of the paramter file for sample log merging
static const std::string SUM_MERGE = "sample_logs_sum";
static const std::string TIME_SERIES_MERGE = "sample_logs_time_series";
static const std::string LIST_MERGE = "sample_logs_list";
static const std::string WARN_MERGE = "sample_logs_warn";
static const std::string WARN_MERGE_TOLERANCES = "sample_logs_warn_tolerances";
static const std::string FAIL_MERGE = "sample_logs_fail";
static const std::string FAIL_MERGE_TOLERANCES = "sample_logs_fail_tolerances";
} // namespace MergeRunsParameter

class MANTID_ALGORITHMS_DLL MergeRuns : public API::MultiPeriodGroupAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "MergeRuns"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Combines the data contained in an arbitrary number of input "
           "workspaces.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"ConjoinWorkspaces"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Merging"; }
  // Overriden MultiPeriodGroupAlgorithm method.
  bool useCustomInputPropertyName() const override;

protected:
  /// Overriden fillHistory method to correctly store history from merged
  /// workspaces
  void fillHistory() override;

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  void execEvent();
  void execHistogram(const std::vector<std::string> &inputs);
  void buildAdditionTables();
  // Overriden MultiPeriodGroupAlgorithm method.
  std::string fetchInputPropertyName() const override;

  /// An addition table is a list of pairs: First int = workspace index in the
  /// EW being added, Second int = workspace index to which it will be added in
  /// the OUTPUT EW. -1 if it should add a new entry at the end.
  using AdditionTable = std::vector<std::pair<int, int>>;
  /// Copy the history from the input workspaces to the output workspaces
  template <typename Container> void copyHistoryFromInputWorkspaces(const Container &workspaces) {
    API::Workspace_sptr outWS = this->getProperty("OutputWorkspace");

    // this is not a child algorithm. Add the history algorithm to the
    // WorkspaceHistory object.
    if (!isChild()) {
      // Loop over the input workspaces, making the call that copies their
      // history to the output ones
      // (Protection against copy to self is in
      // WorkspaceHistory::copyAlgorithmHistory)
      for (auto inWS = workspaces.begin(); inWS != workspaces.end(); ++inWS) {
        outWS->history().addHistory((*inWS)->getHistory());
      }
      // Add the history for the current algorithm to all the output workspaces
      outWS->history().addHistory(m_history);
    }
    // this is a child algorithm, but we still want to keep the history.
    else if (isRecordingHistoryForChild() && m_parentHistory) {
      m_parentHistory->addChildHistory(m_history);
    }
  }

  // Methods called by exec()
  using Mantid::API::Algorithm::validateInputs;
  bool validateInputsForEventWorkspaces(const std::vector<std::string> &inputWorkspaces);
  std::optional<std::vector<double>> checkRebinning();
  static std::vector<double> calculateRebinParams(const std::vector<double> &bins1, const std::vector<double> &bins2);
  static void noOverlapParams(const HistogramData::HistogramX &X1, const HistogramData::HistogramX &X2,
                              std::vector<double> &params);
  static void intersectionParams(const HistogramData::HistogramX &X1, size_t &i, const HistogramData::HistogramX &X2,
                                 std::vector<double> &params);
  static void inclusionParams(const HistogramData::HistogramX &X1, size_t &i, const HistogramData::HistogramX &X2,
                              std::vector<double> &params);
  API::MatrixWorkspace_sptr rebinInput(const API::MatrixWorkspace_sptr &workspace, const std::vector<double> &params);
  API::MatrixWorkspace_sptr buildScanningOutputWorkspace(const API::MatrixWorkspace_sptr &outWS,
                                                         const API::MatrixWorkspace_sptr &addee);
  /// Progress reporting
  std::unique_ptr<API::Progress> m_progress;

  /// List of input EVENT workspaces
  std::vector<Mantid::DataObjects::EventWorkspace_sptr> m_inEventWS;
  /// List of input matrix workspace
  std::list<API::MatrixWorkspace_sptr> m_inMatrixWS;
  /// Addition tables for event workspaces
  std::vector<AdditionTable> m_tables;
  /// Total number of histograms in the output workspace
  size_t m_outputSize = 0;

  std::vector<SpectrumDefinition> buildScanIntervals(const std::vector<SpectrumDefinition> &addeeSpecDefs,
                                                     const Geometry::DetectorInfo &addeeDetInfo,
                                                     const Geometry::DetectorInfo &newOutDetInfo);
};

} // namespace Algorithms
} // namespace Mantid
