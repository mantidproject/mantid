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
  /// (Empty) Constructor
  ConvertFromDistribution() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~ConvertFromDistribution() {}
  /// Algorithm's name
  virtual const std::string name() const { return "ConvertFromDistribution"; }
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Converts a histogram workspace from a distribution i.e. multiplies "
           "by the bin width.";
  }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const {
    return "Transforms\\Distribution";
  }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_CONVERTFROMDISTRIBUTION_H_*/
