#ifndef MANTID_ALGORITHMS_CONVERTFROMDISTRIBUTION_H_
#define MANTID_ALGORITHMS_CONVERTFROMDISTRIBUTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** Converts a histogram workspace from a distribution. i.e. multiplies by the
   bin width

    Required Properties:
    <UL>
    <LI> Workspace - The name of the Workspace to convert.</LI>
    </UL>

    @author Russell Taylor, Tessella Support Services plc
    @date 17/11/2008

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
class DLLExport ConvertFromDistribution : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ConvertFromDistribution"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Converts a histogram workspace from a distribution i.e. multiplies "
           "by the bin width.";
  }
  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"ConvertToDistribution"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Transforms\\Distribution";
  }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CONVERTFROMDISTRIBUTION_H_*/
