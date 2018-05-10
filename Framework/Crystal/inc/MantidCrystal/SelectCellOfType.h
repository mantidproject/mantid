#ifndef MANTID_CRYSTAL_SELECT_CELL_OF_TYPE_H_
#define MANTID_CRYSTAL_SELECT_CELL_OF_TYPE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {
/** ShowPossibleCells : Algorithm to display a list of possible conventional
    cells corresponding to the UB saved in the sample associated
    with the specified PeaksWorkspace, provided the saved UB is for a Niggli
    reduced cell.

    @author Dennis Mikkelson
    @date   2012-02-09

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
class DLLExport SelectCellOfType : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SelectCellOfType"; };

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"FindUBUsingFFT", "FindUBUsingIndexedPeaks",
            "FindUBUsingLatticeParameters"};
  }

  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\Cell"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Select a conventional cell with a specific lattice type and "
           "centering, corresponding to the UB stored with the sample for this "
           "peaks works space.";
  }

private:
  /// Initialise the properties
  void init() override;

  /// Run the algorithm
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_SELECT_CELL_OF_TYPE_H */
