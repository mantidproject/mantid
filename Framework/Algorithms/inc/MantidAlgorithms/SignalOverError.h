#ifndef MANTID_ALGORITHMS_SIGNALOVERERROR_H_
#define MANTID_ALGORITHMS_SIGNALOVERERROR_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/UnaryOperation.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** Calculate Y/E for a Workspace2D

  @date 2011-12-05

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
class DLLExport SignalOverError : public UnaryOperation {
public:
  SignalOverError();

  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Replace Y by Y/E for a MatrixWorkspace";
  }

  int version() const override;
  const std::string category() const override;

private:
  // Overridden UnaryOperation methods
  void performUnaryOperation(const double XIn, const double YIn,
                             const double EIn, double &YOut,
                             double &EOut) override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_SIGNALOVERERROR_H_ */
