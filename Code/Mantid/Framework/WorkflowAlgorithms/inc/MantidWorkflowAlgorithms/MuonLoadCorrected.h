#ifndef MANTID_WORKFLOWALGORITHMS_MUONLOADCORRECTED_H_
#define MANTID_WORKFLOWALGORITHMS_MUONLOADCORRECTED_H_

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

  /** MuonLoadCorrected : loads Muon data with Dead Time Correction applied. 
    
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
  class DLLExport MuonLoadCorrected  : public API::Algorithm
  {
  public:
    MuonLoadCorrected();
    virtual ~MuonLoadCorrected();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;

  private:
    virtual void initDocs();
    void init();
    void exec();

    /// Attempts to load dead time table from given Muon Nexus file
    Workspace_sptr loadDeadTimesFromNexus(const std::string& filename, int numPeriods = 1);

    /// Applies dead time table to a workspace
    Workspace_sptr applyDtc(Workspace_sptr ws, Workspace_sptr dt);
    
    /// Runs ApplyDeadTimeCorre algorithm
    MatrixWorkspace_sptr runApplyDtc(MatrixWorkspace_sptr ws, TableWorkspace_sptr dtt);

    /// Creates Dead Time Table from the given list of dead times.
    TableWorkspace_sptr createDeadTimeTable( std::vector<double>::const_iterator begin, 
      std::vector<double>::const_iterator end);
  };


} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif  /* MANTID_WORKFLOWALGORITHMS_MUONLOADCORRECTED_H_ */
