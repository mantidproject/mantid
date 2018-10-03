#ifndef MANTID_ALGORITHM_CHANGEBINOFFSET_H_
#define MANTID_ALGORITHM_CHANGEBINOFFSET_H_

#include "MantidAlgorithms/SpectrumAlgorithm.h"

namespace Mantid {
namespace Algorithms {
/**Takes a workspace and adjusts all the time bin values by the same amount.

Required Properties:
<UL>
<LI> InputWorkspace - The name of the Workspace to take as input </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the result
</LI>
<LI> Offset - The number by which to change the time bins by</LI>
</UL>

@author
@date 11/07/2008

Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport ChangeBinOffset : public SpectrumAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "ChangeBinOffset"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Adjusts all the time bin values in a workspace by a specified "
           "amount.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"ScaleX"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Axes"; }
  /// Algorithm's Alternate Name
  const std::string alias() const override { return "OffsetX"; }

private:
  /// Initialisation method. Declares properties to be used in algorithm.
  void init() override;
  /// Executes the algorithm
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_CHANGEBINOFFSET_H_*/
