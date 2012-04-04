#ifndef MANTID_DATAOBJECTS_SPLITTERSWORKSPACE_H_
#define MANTID_DATAOBJECTS_SPLITTERSWORKSPACE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/ISplittersWorkspace.h"
#include "MantidKernel/TimeSplitter.h"
#include "MantidDataObjects/TableWorkspace.h"


namespace Mantid
{
namespace DataObjects
{

  /** SplittersWorkspace : A TableWorkspace to contain TimeSplitters.
    It will be used as an input for FilterEvents, which is the ultimate method for event filtering.
    There can be various algorithms to generate an object like this.

    A SplittersWorkspace contains 3 columns as int64, int64 and int32 to denote
    (1) splitter start time (2) splitter end time and (3) group workspace index
    
    @date 2012-04-03

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport SplittersWorkspace : public API::ISplittersWorkspace, public DataObjects::TableWorkspace
  {
  public:
    SplittersWorkspace();
    virtual ~SplittersWorkspace();
    
    void addSplitter(Kernel::SplittingInterval splitter);

    Kernel::SplittingInterval getSplitter(size_t index);

    size_t getNumberSplitters();

    bool removeSplitter(unsigned long);

  };


} // namespace DataObjects
} // namespace Mantid

#endif  /* MANTID_DATAOBJECTS_SPLITTERSWORKSPACE_H_ */
