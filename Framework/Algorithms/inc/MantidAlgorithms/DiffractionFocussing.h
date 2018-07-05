#ifndef MANTID_ALGORITHM_DIFFRACTIONFOCUSSING_H_
#define MANTID_ALGORITHM_DIFFRACTIONFOCUSSING_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include <Poco/NObserver.h>

namespace Mantid {
namespace Algorithms {
/**
This is a parent algorithm that uses several different child algorithms to
perform it's task.
Takes a workspace as input and the filename of a grouping file of a suitable
format.

The input workspace is
1) Converted to d-spacing units
2) Rebinnned to a common set of bins
3) The spectra are grouped according to the grouping file.

    Required Properties:
<UL>
<LI> InputWorkspace - The name of the 2D Workspace to take as input </LI>
<LI> GroupingFileName - The path to a grouping file</LI>
<LI> OutputWorkspace - The name of the 2D workspace in which to store the result
</LI>
</UL>

The structure of the grouping file is as follows:
# Format: number  UDET offset  select  group
0        611  0.0000000  1    0
1        612  0.0000000  1    0
2        601  0.0000000  0    0
3        602  0.0000000  0    0
4        621  0.0000000  1    0


@author Nick Draper, Tessella
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
class DLLExport DiffractionFocussing : public API::Algorithm,
                                       public API::DeprecatedAlgorithm {
public:
  /// Constructor
  DiffractionFocussing();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "DiffractionFocussing"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Algorithm to focus powder diffraction data into a number of "
           "histograms according to a grouping scheme defined in a CalFile.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Diffraction\\Focussing";
  }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  API::MatrixWorkspace_sptr
  convertUnitsToDSpacing(const API::MatrixWorkspace_sptr &workspace);
  void RebinWorkspace(API::MatrixWorkspace_sptr &workspace);
  void calculateRebinParams(const API::MatrixWorkspace_const_sptr &workspace,
                            double &min, double &max, double &step);
  std::multimap<int64_t, int64_t>
  readGroupingFile(std::string groupingFileName);
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHM_DIFFRACTIONFOCUSSING_H_*/
