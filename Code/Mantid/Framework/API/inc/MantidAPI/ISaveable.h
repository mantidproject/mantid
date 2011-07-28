#ifndef MANTID_MDEVENTS_ISAVEABLE_H_
#define MANTID_MDEVENTS_ISAVEABLE_H_
    
#include "MantidKernel/System.h"

namespace Mantid
{
namespace API
{

  /** An interface for objects that can be cached or saved to disk.
    This is implemented by MDBox and is used in the in-memory
    cache of file-backed MDEventWorkspaces.
    
    @author Janik Zikovsky
    @date 2011-07-28

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport ISaveable 
  {
  public:
    ISaveable();
    ISaveable(const size_t id);
    ~ISaveable();
    
    /// Save the data - to be overriden
    virtual void save() const {};

    /// Load the data - to be overriden
    virtual void load() {};

    //-----------------------------------------------------------------------------------------------
    /** Returns the unique ID for this object/box     */
    size_t getId() const
    {
      return m_id;
    }

    //-----------------------------------------------------------------------------------------------
    /** Sets the unique ID for this object/box
     * @param newId :: new ID value. */
    void setId(size_t newId)
    {
      m_id = newId;
    }

    //-----------------------------------------------------------------------------------------------
    /// Return the amount of memory that the object takes up in the MRU.
    virtual size_t getMRUMemory() const {return 0};

  protected:
    /// Unique, sequential ID of the object/box within the containing workspace.
    size_t m_id;
  };


} // namespace API
} // namespace Mantid

#endif  /* MANTID_MDEVENTS_ISAVEABLE_H_ */
