#ifndef MANTID_MDALGORITHMS_COMPACTMD_H_
#define MANTID_MDALGORITHMS_COMPACTMD_H_

/** An algorithm used to crop an MDHistoWorkspace based on the first
    non-zero signals found in each dimension.

  @author Matt King
  @date 02-10-2015

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidMDAlgorithms/CutMD.h"
#include "boost/shared_ptr.hpp"
namespace Mantid {
namespace MDAlgorithms {
class DLLExport CompactMD : public API::Algorithm {
public:
  void init() override;
  void exec() override;
  /// Algorithm's name for identification
  const std::string name() const override { return "CompactMD"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Crops an MDHistoWorkspace based on the first non-zero signals "
           "giving a more focussed area of interest.";
  }
  const std::string category() const override {
    return "MDAlgorithms\\Utility\\Workspaces";
  }
  /// Algorithm's version for identification
  int version() const override { return 1; }
  /// Finding the extents of the first non-zero signals.
  void
  findFirstNonZeroMinMaxExtents(Mantid::API::IMDHistoWorkspace_sptr inputWs,
                                std::vector<Mantid::coord_t> &minVec,
                                std::vector<Mantid::coord_t> &maxVec);
};
} // namespace MDAlgorithms
} // namespace Mantid

#endif // MANTID_MDALGORITHMS_COMPACTMD_H_