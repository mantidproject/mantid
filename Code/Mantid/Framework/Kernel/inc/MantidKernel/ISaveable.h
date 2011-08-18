#ifndef MANTID_MDEVENTS_ISAVEABLE_H_
#define MANTID_MDEVENTS_ISAVEABLE_H_
    
#include "MantidKernel/System.h"

namespace Mantid
{
namespace Kernel
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
    ISaveable(const ISaveable & other);
    ISaveable(const size_t id);
    ~ISaveable();

    //-----------------------------------------------------------------------------------------------
    /** Returns the unique ID for this object/box     */
    size_t getId() const
    {
      return m_id;
    }

    /** Sets the unique ID for this object/box
     * @param newId :: new ID value. */
    void setId(size_t newId)
    {
      m_id = newId;
    }

    //-----------------------------------------------------------------------------------------------

    /// Save the data - to be overriden
    virtual void save() const = 0;

    /// Load the data - to be overriden
    virtual void load() = 0;

    /// Method to flush the data to disk and ensure it is written.
    virtual void flushData() const = 0;

    /// @return true if it the data of the object is busy and so cannot be cleared; false if the data was released and can be cleared/written.
    virtual bool dataBusy() const = 0;

    /** @return the position in the file where the data will be stored. This is used to optimize file writing. */
    virtual uint64_t getFilePosition() const = 0;

    /** @return the amount of memory that the object takes up in MEMORY.
     * This should be in the same units as getFilePosition(), e.g. the object will use a block
     * from getFilePosition() to getFilePosition()+getMRUMemorySize()-1 in the file.
     */
    virtual uint64_t getMRUMemorySize() const = 0;

  protected:
    /// Unique, sequential ID of the object/box within the containing workspace.
    size_t m_id;
  };


} // namespace Kernel
} // namespace Mantid

#endif  /* MANTID_MDEVENTS_ISAVEABLE_H_ */
