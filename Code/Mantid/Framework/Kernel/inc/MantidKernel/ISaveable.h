#ifndef MANTID_KERNEL_ISAVEABLE_H_
#define MANTID_KERNEL_ISAVEABLE_H_
    
#include "MantidKernel/System.h"
#include <list>
#include <vector>
#include <algorithm>
#include <boost/optional.hpp>

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  // forward declaration
  class INode;

  class DLLExport ISaveable 
  {
  public:
    ISaveable();
    //ISaveable(const ISaveable &other); --> no pointers, standard CC
    virtual ~ISaveable(){};

    ////-----------------------------------------------------------------------------------------------
    ///** Returns the unique ID for this object/box     */
    //size_t getFileId() const
    //{  return m_FileId;   }

    ///** Sets the unique ID for this object/box
    // * @param newId :: new ID value. */
    //virtual void setFileId(size_t newId)
    //{    m_FileId = newId;   }

    ///** @return the position in the file where the data will be stored. This is used to optimize file writing. */
    virtual uint64_t getFilePosition() const
    {   return m_fileIndexStart;   }
    /**Return the number of units this block occipies on file */
    uint64_t getFileSize()const
    { return   m_fileNumEvents;   }
    //-----------------------------------------------------------------------------------------------
    // Saveable functions interface, which controls the logic of working with objects on HDD
    virtual bool isBusy()const=0;
    virtual bool isDataChanged()const=0;
    virtual bool wasSaved()const=0;
    virtual bool isLoaded()const=0;
    //-----------------------------------------------------------------------------------------------

    /// Save the data - to be overriden
    virtual void save()const =0;
  
    /// Load the data - to be overriden
    virtual void load() = 0;

    /// Method to flush the data to disk and ensure it is written.
    virtual void flushData() const = 0;
    /// remove objects data from memory
    virtual void clearDataFromMemory() = 0;

  /** @return the amount of memory that the object takes as a whole.
      For filebased objects it should be the amount the object occupies in memory plus the size it occupies in file if the object has not been fully loaded
      or modified.
     * If the object has never been loaded, this should be equal to number of data points in the file
     */
    virtual uint64_t getTotalDataSize() const=0;
    /// the data size kept in memory
    virtual size_t getDataMemorySize()const=0;

  protected:
    /** Unique, sequential ID of the object/box within the containing workspace.
        This ID also relates to boxes order as the boxes with adjacent 
        ID should usually occupy adjacent places on HDD         */
  //  size_t m_FileId;
    /// Start point in the NXS file where the events are located
    uint64_t m_fileIndexStart;
    /// Number of events saved in the file, after the start index location
    uint64_t m_fileNumEvents;  
  private:
    // the iterator which describes the position of this object in the DiskBuffer. Undefined if not placed to buffer
    boost::optional< std::list<ISaveable * const >::iterator> m_BufPosition;
    // the size of the object in the memory buffer, used to calculate the total amount of memory the objects occupy
    size_t m_BufMemorySize;


    /// the functions below have to be availible to DiskBuffer and nobody else. To highlight this we make them private
    friend class DiskBuffer;
   /** save at specific file location the specific amount of data; 
        used by DiskBuffer which asks this object where to save it and calling 
        overloaded object specific save operation above    */
    void saveAt(uint64_t newPos, uint64_t newSize);

    /// sets the iterator pointing to the location of this object in the memory buffer to write later
    size_t setBufferPosition(std::list<ISaveable *const >::iterator &bufPosition);
    /// returns the iterator pointing to the position of this object within the memory to-write buffer
    boost::optional<std::list<ISaveable *const>::iterator > & getBufPostion()
    {return m_BufPosition;}
    /// return the amount of memory, this object had when it was stored in buffer last time;
    size_t getBufferSize()const{return m_BufMemorySize;}
    void setBufferSize(size_t newSize){m_BufMemorySize = newSize;}

    /// clears the state of the object, and indicate that it is not stored in buffer any more 
    void clearBufferState();

  };


} // namespace Kernel
} // namespace Mantid

#endif  /* MANTID_KERNEL_ISAVEABLE_H_ */
