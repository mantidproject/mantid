#ifndef MANTID_MDALGORITHMS_CONVERTTODIFFRACTIONMDWORKSPACE_H_
#define MANTID_MDALGORITHMS_CONVERTTODIFFRACTIONMDWORKSPACE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidKernel/V3D.h"
#include "MantidMDAlgorithms/BoxControllerSettingsAlgorithm.h"

namespace Mantid {
namespace MDAlgorithms {

/** ConvertToDiffractionMDWorkspace :
 * Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, Qz) from
 *an input EventWorkspace.
 *
 * @author Janik Zikovsky, SNS
 * @date 2011-03-01 13:14:48.236513
 */
class DLLExport ConvertToDiffractionMDWorkspace
    : public BoxControllerSettingsAlgorithm {
public:
  ConvertToDiffractionMDWorkspace();

  /// Algorithm's name for identification
  const std::string name() const override {
    return "ConvertToDiffractionMDWorkspace";
  };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, "
           "Qz) for an elastic diffraction experiment.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "MDAlgorithms\\Creation";
  }

private:
  void init() override;
  void exec() override;

  template <class T>
  void convertEventList(int workspaceIndex, const API::SpectrumInfo &specInfo,
                        DataObjects::EventList &el);

  void convertSpectrum(const API::SpectrumInfo &specInfo, int workspaceIndex);

  /// The input MatrixWorkspace
  API::MatrixWorkspace_sptr m_inWS;

  /// The input event workspace
  DataObjects::EventWorkspace_sptr m_inEventWS;

  /// The output MDEventWorkspace<3>
  DataObjects::MDEventWorkspace3Lean::sptr ws;
  /// Do we clear events on the input during loading?
  bool ClearInputWorkspace;
  /// Use the histogram representation with one event per bin
  bool OneEventPerBin;
  /// Are we appending?
  bool Append;
  /// Perform LorentzCorrection on the fly.
  bool LorentzCorrection;
  /// Map of all the detectors in the instrument
  detid2det_map allDetectors;
  /// Primary flight path (source to sample)
  double l1;
  /// Beam direction and length
  Kernel::V3D beamline;
  /// Path length between source and sample
  double beamline_norm;
  /// Path length between source and sample
  size_t failedDetectorLookupCount;
  /// Beam direction (unit vector)
  Kernel::V3D beamDir;
  /// Sample position
  Kernel::V3D samplePos;
  /// Progress reporter (shared)
  boost::shared_ptr<Kernel::ProgressBase> prog;
  /// Matrix. Multiply this by the lab frame Qx, Qy, Qz to get the desired Q or
  /// HKL.
  Kernel::Matrix<double> mat;

  /// Minimum extents of the workspace. Cached for speed
  coord_t *m_extentsMin;
  /// Maximum extents of the workspace. Cached for speed
  coord_t *m_extentsMax;
};

} // namespace Mantid
} // namespace DataObjects

#endif /* MANTID_MDALGORITHMS_CONVERTTODIFFRACTIONMDWORKSPACE_H_ */
