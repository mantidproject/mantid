#ifndef MANTID_CRYSTAL_PREDICTSATELLITEPEAKS_H_
#define MANTID_CRYSTAL_PREDICTSATELLITEPEAKS_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/HKLFilterWavelength.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {
/** CreatSatellitePeaks : Algorithm to create a PeaksWorkspace with peaks
   corresponding
    to fractional h,k,and l values.

    @author Vickie Lynch
    @date   2018-07-13

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory &
                     NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at:
    <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
class DLLExport PredictSatellitePeaks : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override {
    return "PredictSatellitePeaks";
  };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "The offsets can be from hkl values in a range of hkl values or "
           "from peaks in the input PeaksWorkspace";
  }

  /// Algorithm's version for identification
  int version() const override {
    return 1;
  };
  const std::vector<std::string> seeAlso() const override {
    return { "PredictPeaks" };
  }

  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\Peaks"; }

private:
  const size_t MAX_NUMBER_HKLS = 10000000000;
  /// Initialise the properties
  void init() override;

  /// Run the algorithm
  void exec() override;
  void exec_peaks();
  Kernel::V3D getOffsetVector(const std::string &label);
  void predictOffsets(DataObjects::PeaksWorkspace_sptr Peaks,
                      boost::shared_ptr<Mantid::API::IPeaksWorkspace> &OutPeaks,
                      int iVector, Kernel::V3D offsets, int &maxOrder,
                      Kernel::V3D &hkl,
                      Geometry::HKLFilterWavelength &lambdaFilter,
                      bool &includePeaksInRange, bool includeOrderZero,
                      std::vector<std::vector<int> > &AlreadyDonePeaks);
  void predictOffsetsWithCrossTerms(
      DataObjects::PeaksWorkspace_sptr Peaks,
      boost::shared_ptr<Mantid::API::IPeaksWorkspace> &OutPeaks,
      Kernel::V3D offsets1, Kernel::V3D offsets2, Kernel::V3D offsets3,
      int &maxOrder, Kernel::V3D &hkl,
      Geometry::HKLFilterWavelength &lambdaFilter, bool &includePeaksInRange,
      bool &includeOrderZero,
      std::vector<std::vector<int> > &AlreadyDonePeaks);
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_PREDICTSATELLITEPEAKS */
