#ifndef MANTID_CRYSTAL_SETUB_H_
#define MANTID_CRYSTAL_SETUB_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Crystal {

/** SetUB : Algorithm to set the UB matrix, given lattice parameters and u and v
  vectors as defined in:
  http://horace.isis.rl.ac.uk/Getting_started


  @author Andrei Savici
  @date 2011-11-07

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport SetUB : public API::Algorithm {
public:
  SetUB();
  ~SetUB();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Set the UB matrix, given either lattice parametersand orientation "
           "vectors or the UB matrix elements";
  }

  virtual int version() const;
  virtual const std::string category() const;

private:
  void init();
  void exec();
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_SETUB_H_ */
