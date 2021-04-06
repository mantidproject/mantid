// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid {
namespace Algorithms {

/**  In normal circumstances an instrument in event mode counts neutrons with
  constant steady rate which depends on beam intensity, instrument settings and
  sample.
  Sometimes hardware issues cause it to count much faster or slower. This
  appears as spurious signals on the final neutron images and users want to
  filter these signals.

  The algorithm calculates neutrons counting rate as the function of the
  experiment's time and adds appropriate logs to the event workspace
  for further event filtering on the basis of these logs, if the log values in
  some parts differ strongly from the average values.
*/
class MANTID_ALGORITHMS_DLL CalculateCountRate : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"ChangePulsetime"}; }
  const std::string category() const override;
  const std::string summary() const override;
  /// Helper function: true if count rate should be normalized and false
  /// otherwise
  bool normalizeCountRate() const;
  /// Helper function to test if log derivative is used
  bool useLogDerivative() const;
  /// helper function to test if visualization workspace is requested
  bool buildVisWS() const;

private:
  void init() override;
  void exec() override;
  // holder of the temporary log, used for normalization, binning source etc...
  std::unique_ptr<Kernel::TimeSeriesProperty<double>> m_tmpLogHolder;
  // diable log normalization
  void disableNormalization(const std::string &NormLogError);

protected: // for testing, actually private
  /// pointer to the log used to normalize results or NULL if no such log
  /// present on input workspace.
  bool m_normalizeResult{false};
  Kernel::TimeSeriesProperty<double> const *m_pNormalizationLog{nullptr};
  /// default number of points in the target log
  int m_numLogSteps{200};
  /// specify if rate is calculated in selected frame interval (range defined)
  /// or all frame should be used
  bool m_rangeExplicit{false};
  bool m_useLogDerivative{false};
  /// spurion search ranges (TOF or other units requested)
  double m_XRangeMin{0}, m_XRangeMax{0};
  /// experiment time ranges:
  Types::Core::DateAndTime m_TRangeMin{0}, m_TRangeMax{0};
  /// temporary workspace used to keep intermediate results
  DataObjects::EventWorkspace_sptr m_workingWS;

  void setSourceWSandXRanges(DataObjects::EventWorkspace_sptr &InputWorkspace);

  void setOutLogParameters(const DataObjects::EventWorkspace_sptr &InputWorkspace);

  void calcRateLog(DataObjects::EventWorkspace_sptr &InputWorkspace, Kernel::TimeSeriesProperty<double> *const targLog);

  void checkAndInitVisWorkspace();

  void histogramEvents(const DataObjects::EventList &el, std::mutex *spectraLocks);

  void buildVisWSNormalization(std::vector<double> &normalization);

  /// should algo generate visualization VS
  bool m_doVis{false};
  /// shared pointer to the optional visualization workspace
  DataObjects::Workspace2D_sptr m_visWs;
  // variables used in 2D histogramming of the visualization workspace
  double m_visX0, m_visDX, m_visT0, m_visDT, m_visTmax;
  // vector used in normalization of the visualization workspace
  std::vector<double> m_visNorm;
};

} // namespace Algorithms
} // namespace Mantid
