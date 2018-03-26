#ifndef MANTID_CRYSTAL_TRANSFORM_HKL_H_
#define MANTID_CRYSTAL_TRANSFORM_HKL_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Crystal {
/** TransformHKL : Algorithm to adjust the UB saved in the sample associated
    with the specified PeaksWorkspace, so the HKL values are reordered or
    otherwise transformed.

    @author Dennis Mikkelson
    @date   2012-04-24

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
class DLLExport TransformHKL : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override;

  /// Algorithm's version for identification
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"SortHKL"};
  }

  /// Algorithm's category for identification
  const std::string category() const override;

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Specify a 3x3 matrix to apply to (HKL) vectors as a list of 9 "
           "comma separated numbers. Both the UB and HKL values will be "
           "updated";
  }

private:
  /// Initialise the properties
  void init() override;

  /// Run the algorithm
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_TRANSFORM_HKL */
