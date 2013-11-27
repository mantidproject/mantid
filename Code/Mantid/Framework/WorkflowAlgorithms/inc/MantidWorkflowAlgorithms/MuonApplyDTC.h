#ifndef MANTID_WORKFLOWALGORITHMS_MUONAPPLYDTC_H_
#define MANTID_WORKFLOWALGORITHMS_MUONAPPLYDTC_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
  using namespace Kernel;
  using namespace API;
  using namespace DataObjects;

  /** MuonApplyDTC : Applies Dead Time Correction to Muon data.
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport MuonApplyDTC  : public API::Algorithm
  {
  public:
    MuonApplyDTC();
    virtual ~MuonApplyDTC();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;

  private:
    virtual void initDocs();
    void init();
    void exec();

    /// Attempts to load dead time table from custom file
    Workspace_sptr loadDeadTimesFromNexus(const std::string& filename);

    /// Applies dead time table to a workspace
    Workspace_sptr applyDtc(Workspace_sptr ws, Workspace_sptr dt);

    /// Applies dead time table to a group of workspaces
    WorkspaceGroup_sptr applyDtcTableToGroup(WorkspaceGroup_sptr wsGroup, TableWorkspace_sptr dtTable);
    
    /// Applies a group of dead time tables to a group of workspaces
    WorkspaceGroup_sptr applyDtcGroupToGroup(WorkspaceGroup_sptr wsGroup, WorkspaceGroup_sptr dtGroup);
    
    /// Runs ApplyDeadTimeCorre algorithm
    MatrixWorkspace_sptr runApplyDtc(MatrixWorkspace_sptr ws, TableWorkspace_sptr dtt);
  };


} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif  /* MANTID_WORKFLOWALGORITHMS_MUONAPPLYDTC_H_ */
