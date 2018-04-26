#ifndef MANTID_CRYSTAL_SAVEISAWUB_H_
#define MANTID_CRYSTAL_SAVEISAWUB_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

namespace Mantid {
namespace Crystal {

/** Algorithm to save  a UB matrix and lattice parameters to an ISAW-style
 * ASCII file.
 *
 * @author Ruth Mikkelson
 * @date 2011-08-10
 *
 * Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

 *This file is part of Mantid.

 *Mantid is free software; you can redistribute it and/or modify
 *it under the terms of the GNU General Public License as published by
 *the Free Software Foundation; either version 3 of the License, or
 *(at your option) any later version.

 *Mantid is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.

 *You should have received a copy of the GNU General Public License
 *along with this program.  If not, see <http://www.gnu.org/licenses/>.

 *File change history is stored at: <https://github.com/mantidproject/mantid>
 *Code Documentation is available at: <http://doxygen.mantidproject.org>
 *
 */

class DLLExport SaveIsawUB : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SaveIsawUB"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Save a UB matrix and lattice parameters from a workspace to an "
           "ISAW-style ASCII file.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"LoadIsawUB"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Crystal\\DataHandling;DataHandling\\Isaw";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  // Calculates the error in the volume
  double getErrorVolume(const Geometry::OrientedLattice &lattice);
};

} // namespace Mantid
} // namespace Crystal

#endif /* MANTID_CRYSTAL_LOADISAWUB_H_ */
