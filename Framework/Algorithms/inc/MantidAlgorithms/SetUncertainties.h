#ifndef MANTID_ALGORITHMS_SETUNCERTAINTIES_H_
#define MANTID_ALGORITHMS_SETUNCERTAINTIES_H_

#include "MantidAPI/ParallelAlgorithm.h"

namespace Mantid {
namespace Algorithms {
/** Set the uncertainties of the data to zero.

 Required Properties:
 <UL>
 <LI> InputWorkspace  - The name of the input workspace. </LI>
 <LI> OutputWorkspace - The name of the output workspace. </LI>
 </UL>

 @date 10/12/2010

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport SetUncertainties : public API::ParallelAlgorithm {
public:
  /// Algorithm's name
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "This algorithm creates a workspace which is the duplicate of the "
           "input, but where the error value for every bin has been set to "
           "zero.";
  }

  /// Algorithm's version
  int version() const override;

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Arithmetic\\Errors"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SETUNCERTAINTIES_H_*/
