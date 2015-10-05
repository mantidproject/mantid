#ifndef MANTID_CRYSTAL_DIFFPEAKSWORKSPACES_H_
#define MANTID_CRYSTAL_DIFFPEAKSWORKSPACES_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Crystal {
/** An algorithm that subtracts from a workspace (the LHSWorkspace) any peaks
   that match
    entries in a second workspace (the RHSWorkspace). Such peaks are those that
   are within
    the given tolerance in all components of Q. Note that a peak in the
   RHSWorkspace will
    only be matched to the first in the LHSWorkspace that is within tolerance.

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
*/
class DLLExport DiffPeaksWorkspaces : public API::Algorithm {
public:
  DiffPeaksWorkspaces();
  virtual ~DiffPeaksWorkspaces();

  virtual const std::string name() const;
  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Removes from a workspace any peaks that match a peak in a second "
           "workspace.";
  }

  virtual int version() const;
  virtual const std::string category() const;

private:
  void init();
  void exec();
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_DIFFPEAKSWORKSPACES_H_ */
