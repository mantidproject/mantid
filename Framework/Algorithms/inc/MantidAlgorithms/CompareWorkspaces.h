// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"

namespace Mantid {
namespace Algorithms {

/** Compares two workspaces for equality. This algorithm is mainly intended for
 * use by Mantid developers as part of the testing process.
 *
 * The data values (X,Y and error) are always checked. The algorithm can also
 * optionally check the axes (this includes the units), the spectra-detector
 * map, the instrument (the name and parameter map) and any bin masking.
 *
 * Required Properties:
 * <UL>
 * <LI> Workspace1 - The name of the first input workspace </LI>
 * <LI> Workspace2 - The name of the second input workspace </LI>
 * </UL>
 *
 * Optional Properties:
 * <UL>
 * <LI> Tolerance       - The maximum amount by which values may differ between
 * the workspaces (default: 0) </LI>
 * <LI> CheckAxes       - Whether to check that the axes match (default: true)
 * </LI>
 * <LI> CheckSpectraMap - Whether to check that the spectra-detector maps match
 * (default: true) </LI>
 * <LI> CheckInstrument - Whether to check that the instruments match (default:
 * true) </LI>
 * <LI> CheckMasking    - Whether to check that the bin masking matches
 * (default: true) </LI>
 * <LI> CheckSample     - Whether to check that the sample object mathces
 * (default: false) </LI>
 * </UL>
 *
 * Output Properties:
 * <UL>
 * <LI> Result - Contains boolean true if the workspaces match, false otherwise
 * </LI>
 * <LI> Messages - TableWorkspace with three columns. The first column contains
 * messages about any mismatches that were detected. The second and third
 * columns contain the names of the workspaces being compared. If no mismatches
 * were detected, this workspace will be empty.
 * </UL>
 *
 *
 */
class MANTID_ALGORITHMS_DLL CompareWorkspaces final : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "CompareWorkspaces"; }

  /// Algorithm's version for identification. @see Algorithm::version
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"CheckWorkspacesMatch", "CompareSampleLogs", "CloneWorkspace"};
  }

  /// Algorithm's category for identification. @see Algorithm::category
  const std::string category() const override { return "Utility\\Workspaces"; }

  /// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
  const std::string summary() const override {
    return "Compares two workspaces for equality. This algorithm is mainly "
           "intended for use by the Mantid development team as part of the "
           "testing process.";
  }

  static bool withinAbsoluteTolerance(double x1, double x2, double atol);
  static bool withinRelativeTolerance(double x1, double x2, double rtol);

private:
  /// Initialise algorithm
  void init() override;

  /// Execute algorithm
  void exec() override;

  /// Process two groups and ensure the Result string is set properly on the
  /// final algorithm
  bool processGroups() override;

  /// Process the two groups
  void processGroups(const std::shared_ptr<const API::WorkspaceGroup> &groupOne,
                     const std::shared_ptr<const API::WorkspaceGroup> &groupTwo);

  void doComparison();

  void doPeaksComparison(DataObjects::PeaksWorkspace_sptr tws1, DataObjects::PeaksWorkspace_sptr tws2);
  void doLeanElasticPeaksComparison(const DataObjects::LeanElasticPeaksWorkspace_sptr &tws1,
                                    const DataObjects::LeanElasticPeaksWorkspace_sptr &tws2);
  void doTableComparison(const API::ITableWorkspace_const_sptr &tws1, const API::ITableWorkspace_const_sptr &tws2);
  void doMDComparison(const API::Workspace_sptr &w1, const API::Workspace_sptr &w2);
  bool compareEventWorkspaces(const DataObjects::EventWorkspace &ews1, const DataObjects::EventWorkspace &ews2);
  bool checkData(const API::MatrixWorkspace_const_sptr &ws1, const API::MatrixWorkspace_const_sptr &ws2);
  bool checkAxes(const API::MatrixWorkspace_const_sptr &ws1, const API::MatrixWorkspace_const_sptr &ws2);
  bool checkSpectraMap(const API::MatrixWorkspace_const_sptr &ws1, const API::MatrixWorkspace_const_sptr &ws2);
  bool checkInstrument(const API::MatrixWorkspace_const_sptr &ws1, const API::MatrixWorkspace_const_sptr &ws2);
  bool checkMasking(const API::MatrixWorkspace_const_sptr &ws1, const API::MatrixWorkspace_const_sptr &ws2);
  bool checkSample(const API::Sample &sample1, const API::Sample &sample2);
  bool checkRunProperties(const API::Run &run1, const API::Run &run2);

  /// Compare 2 EventsList
  int compareEventsListInDetails(const DataObjects::EventList &el1, const DataObjects::EventList &el2, double tolTof,
                                 double tolWeight, int64_t tolPulse, bool printdetails, size_t &numdiffpulse,
                                 size_t &numdifftof, size_t &numdiffboth, size_t &numdiffweight) const;

  /// Records a mismatch in the Messages workspace and sets Result to false
  void recordMismatch(const std::string &msg, std::string ws1 = "", std::string ws2 = "");

  /// Result of comparison (true if equal, false otherwise)
  bool m_result{false};

  /// Mismatch messages that resulted from comparison
  API::ITableWorkspace_sptr m_messages;

  /// Report progress of comparison
  std::unique_ptr<API::Progress> m_progress = nullptr;

  /// Variable states if one wants to compare workspaces in parallell. This
  /// usully true but if one wants to look at the comparison logs, parallell
  /// comparison make things complicated as
  /// logs from different threads are mixed together.  In this case, it is
  /// better not to do parallell comparison.
  bool m_parallelComparison{false};
};

} // namespace Algorithms
} // namespace Mantid
