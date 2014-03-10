#ifndef MANTID_WORKFLOWALGORITHMS_MUONLOAD_H_
#define MANTID_WORKFLOWALGORITHMS_MUONLOAD_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
  using namespace Kernel;
  using namespace API;
  using namespace DataObjects;

  /** MuonLoad : loads Muon workspace ready for analysis. 
    
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
  class DLLExport MuonLoad  : public API::Algorithm
  {
  public:
    MuonLoad();
    virtual ~MuonLoad();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;

  private:
    virtual void initDocs();
    void init();
    void exec();

    // We dont' want processGroups to be called
    virtual bool checkGroups() { return false; }

    /// Returns a workspace for the first period as specified using FirstPeriod property.
    MatrixWorkspace_sptr getFirstPeriodWS(WorkspaceGroup_sptr ws);

    /// Returns a workspace for the second period as specified using SecondPeriod property.
    MatrixWorkspace_sptr getSecondPeriodWS(WorkspaceGroup_sptr ws);

    /// Groups specified workspace according to specified DetectorGroupingTable.
    MatrixWorkspace_sptr groupWorkspace(MatrixWorkspace_sptr ws, TableWorkspace_sptr grouping);

    /// Applies dead time correction to the workspace.
    MatrixWorkspace_sptr applyDTC(MatrixWorkspace_sptr ws, TableWorkspace_sptr dt);

    /// Applies offset, crops and rebin the workspace according to specified params 
    MatrixWorkspace_sptr correctWorkspace(MatrixWorkspace_sptr ws, double loadedTimeZero);
  };


} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif  /* MANTID_WORKFLOWALGORITHMS_MUONLOAD_H_ */
