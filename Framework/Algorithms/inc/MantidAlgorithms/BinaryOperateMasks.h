#ifndef MANTID_ALGORITHMS_BINARYOPERATEMASKS_H_
#define MANTID_ALGORITHMS_BINARYOPERATEMASKS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/** BinaryOperateMasks : TODO: DESCRIPTION

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport BinaryOperateMasks : public API::Algorithm {
public:
  BinaryOperateMasks();
  ~BinaryOperateMasks() override;

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "BinaryOperateMasks"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Performs binary operation, including and, or and xor, on two mask "
           "Workspaces, i.e., SpecialWorkspace2D.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"InvertMask"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Masking"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_BINARYOPERATEMASKS_H_ */
