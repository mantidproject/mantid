#ifndef MANTID_WORKFLOWALGORITHMS_MUONLOAD_H_
#define MANTID_WORKFLOWALGORITHMS_MUONLOAD_H_

#include "MantidKernel/System.h"
#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{
  /** MuonLoad : loads Muon workspace ready for analysis. 
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport MuonLoad  : public API::DataProcessorAlgorithm
  {
  public:
    MuonLoad();
    virtual ~MuonLoad();
    
    virtual const std::string name() const;
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Loads Muon workspace ready for analysis.";}

    virtual int version() const;
    virtual const std::string category() const;

  private:
    void init();
    void exec();

    // We dont' want processGroups to be called
    virtual bool checkGroups() { return false; }

    /// Returns a workspace for the first period as specified using FirstPeriod property.
    API::MatrixWorkspace_sptr getFirstPeriodWS(API::WorkspaceGroup_sptr group);

    /// Returns a workspace for the second period as specified using SecondPeriod property.
    API::MatrixWorkspace_sptr getSecondPeriodWS(API::WorkspaceGroup_sptr group);

    /// Groups specified workspace according to specified DetectorGroupingTable.
    API::MatrixWorkspace_sptr groupWorkspace(API::MatrixWorkspace_sptr ws, DataObjects::TableWorkspace_sptr grouping);

    /// Applies dead time correction to the workspace.
    API::MatrixWorkspace_sptr applyDTC(API::MatrixWorkspace_sptr ws, DataObjects::TableWorkspace_sptr dt);

    /// Applies offset, crops and rebin the workspace according to specified params 
    API::MatrixWorkspace_sptr correctWorkspace(API::MatrixWorkspace_sptr ws, double loadedTimeZero);
  };


} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif  /* MANTID_WORKFLOWALGORITHMS_MUONLOAD_H_ */
