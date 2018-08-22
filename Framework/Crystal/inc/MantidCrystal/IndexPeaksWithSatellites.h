#ifndef MANTID_CRYSTAL_INDEX_PEAKS_WITH_SATELLITES_H_
#define MANTID_CRYSTAL_INDEX_PEAKS_WITH_SATELLITES_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Peak.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Crystal {
/** IndexPeaksWithSatellites : Algorithm to use the UB saved in the sample
   associated
    with the specified PeaksWorkspace, to index the peaks in the workspace.

    @author Dennis Mikkelson
    @date   2011-08-17

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory &
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
class DLLExport IndexPeaksWithSatellites : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override {
    return "IndexPeaksWithSatellites";
  };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Index the peaks using the UB from the sample.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"IndexPeaksWithSatellites"};
  }

  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\Peaks"; }

private:
  /// Initialise the properties
  void init() override;

  /// Run the algorithm
  void exec() override;
  void predictOffsets(DataObjects::Peak &peak, int &sate_indexed,
                      double &satetolerance, double &satellite_error,
                      int numberOffset,
                      Kernel::V3D offsets, int &maxOrder, Kernel::V3D &hkl);
  void predictOffsetsWithCrossTerms(DataObjects::Peak &peak, int &sate_indexed,
                                    double &satetolerance,
                                    double &satellite_error,
                                    Kernel::V3D offsets1, Kernel::V3D offsets2,
                                    Kernel::V3D offsets3, int &maxOrder,
                                    Kernel::V3D &hkl);
  Kernel::V3D getOffsetVector(const std::string &label);
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_INDEX_PEAKS */
