#ifndef MANTID_ALGORITHMS_CHOPDATA
#define MANTID_ALGORITHMS_CHOPDATA

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {
/**

  For use in TOSCA reduction. Splits a 0-100k microsecond workspace into either
  five 20k or
  three 20k and a 40k workspaces

  @author Michael Whitty
  @date 03/02/2011

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
class DLLExport ChopData : public API::Algorithm {
public:
  const std::string name() const override {
    return "ChopData";
  } ///< @return the algorithms name
  const std::string category() const override {
    return "Transforms\\Splitting";
  } ///< @return the algorithms category
  int version() const override {

    return (1);
  } ///< @return version number of algorithm

  const std::vector<std::string> seeAlso() const override {
    return {"ExtractSpectra"};
  }
  /// Algorithm's summary
  const std::string summary() const override {
    return "Splits an input workspace into a grouped workspace, where each "
           "spectra "
           "if 'chopped' at a certain point (given in 'Step' input value) "
           "and the X values adjusted to give all the workspace in the group "
           "the same binning.";
  }

private:
  void init() override; ///< Initialise the algorithm. Declare properties, etc.
  void exec() override; ///< Executes the algorithm.
};
}
}
#endif
