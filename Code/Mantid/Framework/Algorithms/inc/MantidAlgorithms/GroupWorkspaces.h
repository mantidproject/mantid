#ifndef MANTID_ALGORITHM_GROUP_H_
#define MANTID_ALGORITHM_GROUP_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceGroup.h"
namespace Mantid
{
namespace Algorithms
{
/** Takes   workspaces as input and groups the workspaces.

    Required Properties:
    <UL>
    <LI> InputWorkspaces - The name of the workspaces to group  </LI>
    <LI> OutputWorkspace - The name of the new group workspace created </LI>
    </UL>

    @author Sofia Antony
    @date 21/07/2008

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class DLLExport GroupWorkspaces : public API::Algorithm
{
public:
  /// Default constructor
  GroupWorkspaces() : API::Algorithm() {};
  /// Destructor
  virtual ~GroupWorkspaces() {};
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "GroupWorkspaces";}
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1;}
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling";}

private:
 /// Overridden Init method
  void init();
  /// overridden execute method
  void exec();
  /// method to check the input workspaces are of same types
  bool isCompatibleWorkspaces(const std::string & firstWS,const std::string& newWStoAdd );
  /// add member workspace to the groupworkspace
  void addworkspacetoGroup(Mantid::API::WorkspaceGroup_sptr  outgrp_sptr,const std::string &wsName);

 
};

} // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_ALGORITHM_REGROUP_H_*/
