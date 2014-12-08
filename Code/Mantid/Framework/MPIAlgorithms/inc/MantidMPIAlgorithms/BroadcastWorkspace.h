#ifndef MANTID_MPIALGORITHMS_BROADCASTWORKSPACE_H_
#define MANTID_MPIALGORITHMS_BROADCASTWORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace MPIAlgorithms
{
/** BroadcastWorkspace is used to copy a workspace from one process to all the others.

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of input workspace. Need only exist for the broadcasting process.</LI>
    <LI> OutputWorkspace - The name of the output workspace that will be created in all processes.</LI>
    <LI> BroadcasterRank - The rank of the process holding the workspace to be broadcast (default: 0).</LI>
    </UL>

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class BroadcastWorkspace : public API::Algorithm
{
public:
  /// (Empty) Constructor
  BroadcastWorkspace() : API::Algorithm() {}
  /// Virtual destructor
  virtual ~BroadcastWorkspace() {}
  /// Algorithm's name
  virtual const std::string name() const { return "BroadcastWorkspace"; }
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// @copydoc Algorithm::summary
  virtual const std::string summary() const { return "Copy a workspace from one process to all the others."; }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "MPI"; }

private:
  void init();
  void exec();
  //void execEvent(); TODO: Make event-aware? (might lead to transmission of too much data)

};

} // namespace MPIAlgorithms
} // namespace Mantid

#endif /*MANTID_MPIALGORITHMS_BROADCASTWORKSPACE_H_*/
