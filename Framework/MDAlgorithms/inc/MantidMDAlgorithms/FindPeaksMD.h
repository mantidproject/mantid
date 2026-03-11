// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"
#include "MantidMDAlgorithms/DllConfig.h"

namespace Mantid {
namespace Geometry {
class ComponentInfo;
class InstrumentRayTracer;
} // namespace Geometry
namespace MDAlgorithms {

/** FindPeaksMD : TODO: DESCRIPTION
 *
 * @author
 * @date 2011-06-02
 */
class MANTID_MDALGORITHMS_DLL FindPeaksMD final : public API::Algorithm {
public:
  FindPeaksMD();
  /// Algorithm's name for identification
  const std::string name() const override { return "FindPeaksMD"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Find peaks in reciprocal space in a MDEventWorkspace or a "
           "MDHistoWorkspace.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"FindPeaks"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Optimization\\PeakFinding;MDAlgorithms\\Peaks"; }

  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  /// Read member variables from experiment info
  void readExperimentInfo(const Mantid::API::ExperimentInfo_sptr &ei);
  void checkWorkspaceDims(const Mantid::API::IMDWorkspace_sptr &ws);
  void determineOutputType(const std::string &peakType, const uint16_t numExperimentInfo);

  /// Adds a peak based on Q, bin count & a set of detector IDs
  void addPeak(const Geometry::ComponentInfo &compInfo, const Mantid::Kernel::V3D &Q, const double binCount,
               const Geometry::InstrumentRayTracer &tracer);

  /// Adds a peak based on Q, bin count
  void addLeanElasticPeak(const Mantid::Kernel::V3D &Q, const double binCount, const bool useGoniometer = false);

  /// Adds a peak based on Q, bin count
  std::shared_ptr<DataObjects::Peak> createPeak(const Mantid::Kernel::V3D &Q, const double binCount,
                                                const Geometry::InstrumentRayTracer &tracer);

  /// Adds a peak based on Q, bin count
  std::shared_ptr<DataObjects::LeanElasticPeak>
  createLeanElasticPeak(const Mantid::Kernel::V3D &Q, const double binCount, const bool useGoniometer = false);

  /// Run find peaks on an MDEventWorkspace
  template <typename MDE, size_t nd> void findPeaks(typename DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);
  /// Run find peaks on a histo workspace
  void findPeaksHisto(const Mantid::DataObjects::MDHistoWorkspace_sptr &ws);

  /// Output PeaksWorkspace
  Mantid::API::IPeaksWorkspace_sptr peakWS;

  /// Estimated radius of peaks. Boxes closer than this are rejected
  coord_t peakRadiusSquared;

  /// Thresholding factor
  double DensityThresholdFactor;

  /// Max # of peaks
  int64_t m_maxPeaks;

  /// Number of edge pixels with no peaks
  int m_edge;

  /// Flag to include the detectors within the peak
  bool m_addDetectors;

  /// Arbitrary scaling factor for density to make more manageable numbers,
  /// especially for older file formats.
  signal_t m_densityScaleFactor;

  /// Progress reporter.
  std::unique_ptr<Mantid::API::Progress> prog = nullptr;

  /** Enum describing which type of dimensions in the MDEventWorkspace */
  enum eDimensionType { HKL, QLAB, QSAMPLE };

  /// Instrument
  Mantid::Geometry::Instrument_const_sptr m_inst;
  /// Run number of the peaks
  int m_runNumber;
  /// Dimension type
  eDimensionType dimType;
  /// Goniometer matrix
  Mantid::Kernel::Matrix<double> m_goniometer;

  /// Use number of events normalization for event workspaces.
  bool m_useNumberOfEventsNormalization = false;
  /// Signal density factor
  double m_signalThresholdFactor = 1.5;
  /// VolumeNormalization
  static const std::string volumeNormalization;
  /// NumberOfEventNormalization
  static const std::string numberOfEventsNormalization;

  bool m_leanElasticPeak = false;
};

} // namespace MDAlgorithms
} // namespace Mantid
