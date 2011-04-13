#ifndef MANTID_API_IEVENTWORKSPACE_H_
#define MANTID_API_IEVENTWORKSPACE_H_

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IEventList.h"

namespace Mantid
{
namespace API
{


  /** This class provides an interface to an EventWorkspace.

    @author Martyn Gigg, Tessella plc
    @date 13/08/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
  class DLLExport IEventWorkspace : public MatrixWorkspace
  {
  public:
    /// Return the workspace typeID
    virtual const std::string id() const { return "IEventWorkspace"; }
    virtual std::size_t getNumberEvents() const = 0;
    virtual double getTofMin() const = 0;
    virtual double getTofMax() const = 0;
    virtual EventType getEventType() const = 0;
    virtual IEventList * getEventListPtr(const int workspace_index) = 0;
    virtual void clearMRU() const = 0;
  };

  ///shared pointer to the matrix workspace base class
  typedef boost::shared_ptr<IEventWorkspace> IEventWorkspace_sptr;
  ///shared pointer to the matrix workspace base class (const version)
  typedef boost::shared_ptr<const IEventWorkspace> IEventWorkspace_const_sptr;


}
}

#endif //MANTID_API_IEVENTWORKSPACE_H_
