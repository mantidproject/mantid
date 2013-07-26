#ifndef MANTID_ALGORITHMS_RENAMEWORKSPACES_H_
#define MANTID_ALGORITHMS_RENAMEWORKSPACES_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace Algorithms
{
/** Renames a workspace to a different name in the data service.
    If the same name is provided for input and output then the algorithm will fail with an error.
    The renaming is implemented as a removal of the original workspace from the data service
    and re-addition under the new name.

    The algorithm will fail with an error if any of the OutputWorkspace names specified
    (or constructed using Prefix of Suffix) do already exist in ADS.

    Required Properties:
    <UL>
    <LI> InputWorkspace - Comma sepatated list of names of the Workspace to take as input </LI>
    <LI> OutputWorkspace - Comma separated list of new names </LI>
    <LI> Prefix - String which will be added to the front of every InputWorkspace name </LI>
    <LI> Suffix - String which will be added to the end of every InputWorkspace name </LI>
    </UL>

    Copyright &copy; 2007-2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport RenameWorkspaces : public API::Algorithm
{
public:
  /// Default constructor
  RenameWorkspaces() : API::Algorithm() {};
  /// Destructor
  virtual ~RenameWorkspaces() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "RenameWorkspaces";}
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return (1);}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Utility\\Workspaces";}

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  // Overridden Algorithm methods
  void init();
  void exec();
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_RENAMEWORKSPACES_H_*/
