#ifndef MANTID_CRYSTAL_CALCULATEUMATRIX_H_
#define MANTID_CRYSTAL_CALCULATEUMATRIX_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Crystal {

/** CalculateUMatrix : Algorithm to calculate the U matrix, given lattice
  parameters and a list of peaks

  @author Andrei Savici, ORNL
  @date 2011-08-05

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport CalculateUMatrix : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "CalculateUMatrix"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Calculate the U matrix from a peaks workspace, given lattice "
           "parameters.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"SetUB"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Crystal\\UBMatrix"; }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_CALCULATEUMATRIX_H_ */
