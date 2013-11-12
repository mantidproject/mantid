#ifndef MANTID_DATAHANDLING_APPLYGROUPINGFROMMUONNEXUS_H_
#define MANTID_DATAHANDLING_APPLYGROUPINGFROMMUONNEXUS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"

namespace Mantid
{
namespace DataHandling
{
  using namespace DataObjects;
  /** 
    Applies grouping information from Muon Nexus file to the workspace. 

    @author Arturs Bekasovs
    @date 10/10/2013 
    
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
  class DLLExport ApplyGroupingFromMuonNexus  : public API::Algorithm
  {
  public:
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;

  private:
    virtual void initDocs();
    void init();
    void exec();

    bool checkGroups();
    bool processGroups();

    /// Applies grouping to a given workspace
    Workspace2D_sptr applyGrouping(const std::vector<int>& detectorGrouping, Workspace2D_const_sptr inputWs);
  };


} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_APPLYGROUPINGFROMMUONNEXUS_H_ */