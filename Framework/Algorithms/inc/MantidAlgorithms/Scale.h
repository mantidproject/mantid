#ifndef MANTID_ALGORITHMS_SCALE_H_
#define MANTID_ALGORITHMS_SCALE_H_

#include "MantidAPI/DistributedAlgorithm.h"

namespace Mantid {
namespace Algorithms {
/** Scales an input workspace by the given factor, which can be either
   multiplicative or additive.

    Properties:
    <UL>
    <LI> InputWorkspace  - The name of the input workspace. </LI>
    <LI> OutputWorkspace - The name of the output workspace. Can be the same as
   the input one. </LI>
    <LI> Factor          - The value by which to scale the input workspace.
   </LI>
    <LI> Operation       - Whether to multiply (the default) or add by Factor.
   </LI>
    </UL>

    @author Russell Taylor, Tessella plc
    @date 19/03/2010

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
class DLLExport Scale : public API::DistributedAlgorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "Scale"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Scales an input workspace by the given factor, which can be either "
           "multiplicative or additive.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"ScaleX"}; }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Arithmetic;CorrectionFunctions";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_SCALE_H_*/
