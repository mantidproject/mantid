// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/HKLFilterWavelength.h"
#include "MantidGeometry/Crystal/IPeak.h"

namespace Mantid {
namespace Crystal {
/** PredictSatellitePeaks : Algorithm to create a PeaksWorkspace with peaks
   corresponding
    to fractional h,k,and l values.

    @author Shiyun Liu/Vickie Lynch
    @date   2019-03-2019

*/
class MANTID_CRYSTAL_DLL PredictSatellitePeaks final : public API::Algorithm {
public:
  PredictSatellitePeaks();
  /// Algorithm's name for identification
  const std::string name() const override { return "PredictSatellitePeaks"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The offsets can be from hkl values in a range of hkl values or "
           "from peaks in the input PeaksWorkspace";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"PredictPeaks"}; }

  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\Peaks"; }

  /* Determine which type of workspace we're dealing with */
  enum class workspace_type_enum { regular_peaks, lean_elastic_peaks, invalid };

private:
  workspace_type_enum determineWorkspaceType(const API::IPeaksWorkspace_sptr &iPeaksWorkspace) const;

  std::shared_ptr<Geometry::IPeak> createPeakForOutputWorkspace(const Kernel::Matrix<double> &peakGoniometerMatrix,
                                                                const Kernel::V3D &satelliteHKL);

  void addPeakToOutputWorkspace(const std::shared_ptr<Geometry::IPeak> &iPeak,
                                const Kernel::Matrix<double> &peak_goniometer_matrix, const Kernel::V3D &hkl,
                                const Kernel::V3D &satelliteHKL, const int runNumber,
                                std::vector<std::vector<int>> &alreadyDonePeaks, const Kernel::V3D &mnp);

  const size_t MAX_NUMBER_HKLS = 10000000000;
  double m_qConventionFactor;
  API::IPeaksWorkspace_sptr Peaks;
  API::IPeaksWorkspace_sptr outPeaks;
  /// Initialise the properties
  void init() override;

  /// Run the algorithm
  void exec() override;
  void exec_peaks();
  Kernel::V3D getOffsetVector(const std::string &label);

  void predictOffsets(const int indexModulatedVector, const Kernel::V3D &offsets, const int maxOrder,
                      const int runNumber, const Kernel::Matrix<double> &goniometer, const Kernel::V3D &hkl,
                      const Geometry::HKLFilterWavelength &lambdaFilter, const bool includePeaksInRange,
                      const bool includeOrderZero, std::vector<std::vector<int>> &alreadyDonePeaks);

  void predictOffsetsWithCrossTerms(Kernel::V3D offsets1, Kernel::V3D offsets2, Kernel::V3D offsets3,
                                    const int maxOrder, const int RunNumber,
                                    Kernel::Matrix<double> const &peakGoniometerMatrix, Kernel::V3D &hkl,
                                    const Geometry::HKLFilterWavelength &lambdaFilter, const bool includePeaksInRange,
                                    const bool includeOrderZero, std::vector<std::vector<int>> &alreadyDonePeaks);
};

} // namespace Crystal
} // namespace Mantid
