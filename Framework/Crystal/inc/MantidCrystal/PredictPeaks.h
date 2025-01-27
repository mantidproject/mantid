// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DetectorSearcher.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidGeometry/Crystal/StructureFactorCalculator.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/NearestNeighbours.h"

#include <tuple>

namespace Mantid {
namespace Crystal {

/** Using a known crystal lattice and UB matrix, predict where single crystal
 *peaks
 * should be found in detector/TOF space. Creates a PeaksWorkspace containing
 * the peaks at the expected positions.
 *
 * @author Janik Zikovsky
 * @date 2011-04-29 16:30:52.986094
 */
class MANTID_CRYSTAL_DLL PredictPeaks final : public API::Algorithm {
public:
  PredictPeaks();

  /// Algorithm's name for identification
  const std::string name() const override { return "PredictPeaks"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Using a known crystal lattice and UB matrix, predict where single "
           "crystal peaks should be found in detector/TOF space. Creates a "
           "PeaksWorkspace containing the peaks at the expected positions.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"CountReflections", "PredictFractionalPeaks"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\Peaks"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  void checkBeamDirection() const;
  void setInstrumentFromInputWorkspace(const API::ExperimentInfo_sptr &inWS);
  void setRunNumberFromInputWorkspace(const API::ExperimentInfo_sptr &inWS);

  void fillPossibleHKLsUsingGenerator(const Geometry::OrientedLattice &orientedLattice,
                                      std::vector<Kernel::V3D> &possibleHKLs) const;

  void fillPossibleHKLsUsingPeaksWorkspace(const API::IPeaksWorkspace_sptr &peaksWorkspace,
                                           std::vector<Kernel::V3D> &possibleHKLs) const;

  void setStructureFactorCalculatorFromSample(const API::Sample &sample);

  void calculateQAndAddToOutput(const Kernel::V3D &hkl, const Kernel::DblMatrix &orientedUB,
                                const Kernel::DblMatrix &goniometerMatrix);

  void calculateQAndAddToOutputLeanElastic(const Kernel::V3D &hkl, const Kernel::DblMatrix &UB);

private:
  /// Get the predicted detector direction from Q
  std::tuple<Kernel::V3D, double> getPeakParametersFromQ(const Kernel::V3D &q) const;
  /// Cache the reference frame and beam direction from the instrument
  void setReferenceFrameAndBeamDirection();
  void logNumberOfPeaksFound(size_t allowedPeakCount) const;

  /// Number of edge pixels with no peaks
  int m_edge;

  /// Reflection conditions possible
  std::vector<Mantid::Geometry::ReflectionCondition_sptr> m_refConds;
  /// Detector search cache for fast look-up of detectors
  std::unique_ptr<API::DetectorSearcher> m_detectorCacheSearch;
  /// Run number of input workspace
  int m_runNumber;
  /// Instrument reference
  Geometry::Instrument_const_sptr m_inst;
  /// Reference frame for the instrument
  std::shared_ptr<const Geometry::ReferenceFrame> m_refFrame;
  /// Direction of the beam for this instrument
  Kernel::V3D m_refBeamDir;
  /// Output peaks workspace
  Mantid::API::IPeaksWorkspace_sptr m_pw;
  Geometry::StructureFactorCalculator_sptr m_sfCalculator;
  bool m_leanElasticPeak = false;

  double m_qConventionFactor;
};

} // namespace Crystal
} // namespace Mantid
