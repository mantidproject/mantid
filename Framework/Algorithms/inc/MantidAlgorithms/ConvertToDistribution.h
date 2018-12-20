#ifndef MANTID_ALGORITHMS_CONVERTTODISTRIBUTION_H_
#define MANTID_ALGORITHMS_CONVERTTODISTRIBUTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/** Makes a histogram workspace a distribution. i.e. divides by the bin width

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
class DLLExport ConvertToDistribution : public API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "ConvertToDistribution"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Makes a histogram workspace a distribution i.e. divides by the bin "
           "width.";
  }
  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"ConvertFromDistribution"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Transforms\\Distribution";
  }

protected:
  /// Validate inputs
  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CONVERTTODISTRIBUTION_H_*/
