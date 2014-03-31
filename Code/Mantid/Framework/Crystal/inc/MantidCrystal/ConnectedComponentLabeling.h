#ifndef MANTID_CRYSTAL_CONNECTEDCOMPONENTLABELING_H_
#define MANTID_CRYSTAL_CONNECTEDCOMPONENTLABELING_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace Crystal
{
  class BackgroundStrategy;

  /** ConnectedComponentLabelling : Implements connected component labeling on MDHistoWorkspaces.
    
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport ConnectedComponentLabeling
  {
  public:
    ConnectedComponentLabeling(const size_t&id = 1, const bool runMultiThreaded=true);
    size_t getStartLabelId() const;
    void startLabelingId(const size_t& id);
    boost::shared_ptr<Mantid::API::IMDHistoWorkspace> execute(Mantid::API::IMDHistoWorkspace_sptr ws, BackgroundStrategy * const strategy) const;
    virtual ~ConnectedComponentLabeling();
  private:
    int getNThreads() const;
    size_t m_startId;
    const bool m_runMultiThreaded;
    
  };


} // namespace Crystal
} // namespace Mantid

#endif  /* MANTID_CRYSTAL_CONNECTEDCOMPONENTLABELING_H_ */
