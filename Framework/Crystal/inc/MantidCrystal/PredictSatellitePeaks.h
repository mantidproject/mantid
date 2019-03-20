// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTID_CRYSTAL_PREDICTSATELLITEPEAKS_H_
#define MANTID_CRYSTAL_PREDICTSATELLITEPEAKS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/HKLFilterWavelength.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {
/** PredictSatellitePeaks : Algorithm to create a PeaksWorkspace with peaks
   corresponding
    to fractional h,k,and l values.

    @author Shiyun Liu/Vickie Lynch
    @date   2019-03-2019

*/
class DLLExport PredictSatellitePeaks : public API::Algorithm {
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
  const std::vector<std::string> seeAlso() const override {
    return {"PredictPeaks"};
  }

  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\Peaks"; }

private:
  const size_t MAX_NUMBER_HKLS = 10000000000;
  double m_qConventionFactor;
  DataObjects::PeaksWorkspace_sptr Peaks;
  API::IPeaksWorkspace_sptr outPeaks;
  /// Initialise the properties
  void init() override;

  /// Run the algorithm
  void exec() override;
  void exec_peaks();
  Kernel::V3D getOffsetVector(const std::string &label);
  void predictOffsets(int iVector, Kernel::V3D offsets, int &maxOrder,
                      Kernel::V3D &hkl,
                      Geometry::HKLFilterWavelength &lambdaFilter,
                      bool &includePeaksInRange, bool &includeOrderZero,
                      std::vector<std::vector<int>> &AlreadyDonePeaks);
  void predictOffsetsWithCrossTerms(
      Kernel::V3D offsets1, Kernel::V3D offsets2, Kernel::V3D offsets3,
      int &maxOrder, Kernel::V3D &hkl,
      Geometry::HKLFilterWavelength &lambdaFilter, bool &includePeaksInRange,
      bool &includeOrderZero, std::vector<std::vector<int>> &AlreadyDonePeaks);
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_PREDICTSATELLITEPEAKS */
