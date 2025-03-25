// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/OffsetsWorkspace.h"

namespace Mantid {
namespace Kernel {
class PropertyManager;
}

namespace WorkflowAlgorithms {
/**
This is a parent algorithm that uses several different child algorithms to
perform it's task.
Takes a workspace as input and the filename of a grouping file of a suitable
format.
*/
class DLLExport AlignAndFocusPowder : public API::DataProcessorAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "AlignAndFocusPowder"; }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"AlignAndFocusPowderFromFiles"}; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Workflow\\Diffraction"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Algorithm to focus powder diffraction data into a number of "
           "histograms "
           "according to a grouping scheme defined in a CalFile.";
  }

  std::map<std::string, std::string> validateInputs() override;

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  void loadCalFile(const std::string &calFilename, const std::string &groupFilename);
  API::MatrixWorkspace_sptr rebin(API::MatrixWorkspace_sptr matrixws);
  API::MatrixWorkspace_sptr rebinRagged(API::MatrixWorkspace_sptr matrixws, const bool inDspace);

  API::MatrixWorkspace_sptr conjoinWorkspaces(const API::MatrixWorkspace_sptr &ws1,
                                              const API::MatrixWorkspace_sptr &ws2, size_t offset);

  /// Call diffraction focus to a matrix workspace.
  API::MatrixWorkspace_sptr diffractionFocus(API::MatrixWorkspace_sptr ws);

  /// Call diffraction focus to a matrix workspace with ragged rebin parameters
  API::MatrixWorkspace_sptr diffractionFocusRaggedRebinInDspace(API::MatrixWorkspace_sptr ws);

  /// Convert units
  API::MatrixWorkspace_sptr convertUnits(API::MatrixWorkspace_sptr matrixws, const std::string &target);

  /// Filter out absorption resonances
  API::MatrixWorkspace_sptr filterResonances(API::MatrixWorkspace_sptr matrixws);

  /// Call edit instrument geometry
  API::MatrixWorkspace_sptr editInstrument(API::MatrixWorkspace_sptr ws, const std::vector<double> &polars,
                                           const std::vector<specnum_t> &specids, const std::vector<double> &l2s,
                                           const std::vector<double> &phis);
  void convertOffsetsToCal(DataObjects::OffsetsWorkspace_sptr &offsetsWS);
  double getVecPropertyFromPmOrSelf(const std::string &name, std::vector<double> &avec);

  API::MatrixWorkspace_sptr m_inputW;
  API::MatrixWorkspace_sptr m_outputW;
  API::ITableWorkspace_sptr m_calibrationWS;
  DataObjects::MaskWorkspace_sptr m_maskWS;
  DataObjects::GroupingWorkspace_sptr m_groupWS;
  double m_l1{0.0};
  std::vector<int32_t> specids;
  std::vector<double> l2s;
  std::vector<double> tths;
  std::vector<double> phis;
  std::string m_instName;
  std::vector<double> m_params;
  int m_resampleX{0};
  std::vector<double> m_dmins;
  std::vector<double> m_dmaxs;
  std::vector<double> m_delta_ragged;
  std::vector<double> m_resonanceLower;
  std::vector<double> m_resonanceUpper;
  bool binInDspace{false};
  double xmin{0.0};
  double xmax{0.0};
  double DIFCref{0.0};
  double minwl{0.0};
  double maxwl{0.0};
  double tmin{0.0};
  double tmax{0.0};
  bool m_preserveEvents{false};
  void compressEventsOutputWS(const double compressEventsTolerance, const double wallClockTolerance);
  bool shouldCompressUnfocused(const double compressTolerance, const double tofmin, const double tofmax,
                               const bool hasWallClockTolerance);

  /// Low resolution TOF matrix workspace
  API::MatrixWorkspace_sptr m_lowResW;
  /// Low resolution TOF event workspace
  DataObjects::EventWorkspace_sptr m_lowResEW;
  /// Flag to process low resolution workspace
  bool m_processLowResTOF{false};
  /// Offset to low resolution TOF spectra
  size_t m_lowResSpecOffset{0};

  std::unique_ptr<API::Progress> m_progress = nullptr; ///< Progress reporting
};

} // namespace WorkflowAlgorithms
} // namespace Mantid
