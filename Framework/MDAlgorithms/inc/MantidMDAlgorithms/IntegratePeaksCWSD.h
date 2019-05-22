// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_INTEGRATEPEAKSCWSD_H_
#define MANTID_MDALGORITHMS_INTEGRATEPEAKSCWSD_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace MDAlgorithms {

/** Integrate single-crystal peaks in reciprocal-space.
 *
 * @author Janik Zikovsky
 * @date 2011-04-13 18:11:53.496539
 */
class DLLExport IntegratePeaksCWSD : public API::Algorithm {
public:
  IntegratePeaksCWSD();

  /// Algorithm's name for identification
  const std::string name() const override { return "IntegratePeaksCWSD"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Integrate single-crystal peaks in reciprocal space, for "
           "MDEventWorkspaces from reactor-source single crystal "
           "diffractometer.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"IntegratePeaksHybrid", "IntegratePeaksMDHKL", "IntegratePeaksMD",
            "IntegratePeaksUsingClusters"};
  }

  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\Integration"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  /// Process and check input properties
  void processInputs();

  void simplePeakIntegration(const std::vector<detid_t> &vecMaskedDetID,
                             const std::map<int, signal_t> &run_monitor_map);
  template <typename MDE, size_t nd>
  void integrate(typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws,
                 const std::map<uint16_t, signal_t> &run_monitor_map);

  /// Get the run/monitor counts map
  std::map<int, signal_t> getMonitorCounts();

  /// Get the run/measuring time map
  std::map<int, double> getMeasureTime();

  std::vector<detid_t>
  processMaskWorkspace(DataObjects::MaskWorkspace_const_sptr maskws);

  void getPeakInformation();

  DataObjects::PeaksWorkspace_sptr createOutputs();

  void mergePeaks();

  /// Implement this method to normalize the intensity of each Pt.
  void normalizePeaksIntensities();

  DataObjects::PeaksWorkspace_sptr
  createPeakworkspace(Kernel::V3D peakCenter, API::IMDEventWorkspace_sptr mdws);

  /// Input MDEventWorkspace
  Mantid::API::IMDEventWorkspace_sptr m_inputWS;

  /// Input PeaksWorkspace
  Mantid::DataObjects::PeaksWorkspace_sptr m_peaksWS;

  /// Peak centers
  bool m_haveMultipleRun;
  /// a map for run number and normalization value (monitor or time)
  std::map<int, double> m_runNormMap;

  std::map<int, Kernel::V3D> m_runPeakCenterMap;
  bool m_useSinglePeakCenterFmUser;
  Kernel::V3D m_peakCenter;
  double m_peakRadius;
  bool m_doMergePeak;
  bool m_normalizeByMonitor;
  bool m_normalizeByTime; // NormalizeByTime
  double m_scaleFactor;   // ScaleFactor

  /// Peaks
  std::vector<DataObjects::Peak> m_vecPeaks;
  /// Integrated peaks' intensity per run number
  std::map<int, double> m_runPeakCountsMap;

  DataObjects::MaskWorkspace_sptr m_maskWS;
  std::vector<detid_t> vecMaskedDetID;

  /// Peak workspace
  bool m_haveInputPeakWS;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_INTEGRATEPEAKSCWSD_H_ */
