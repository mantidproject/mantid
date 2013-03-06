#ifndef MANTID_MDEVENTS_ISAVEABLE_H_
#define MANTID_MDEVENTS_ISAVEABLE_H_
    
#include "MantidKernel/System.h"
#include <vector>
#include <algorithm>

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
  class DLLExport ISaveable 
  {
  public:
    ISaveable();
    ISaveable(const size_t id);
    ISaveable(const ISaveable & other);
    virtual ~ISaveable();

    //-----------------------------------------------------------------------------------------------
    /** Returns the unique ID for this object/box     */
    size_t getId() const
    {  return m_id;   }

    /** Sets the unique ID for this object/box
     * @param newId :: new ID value. */
    virtual void setId(size_t newId)
    {    m_id = newId;   }

    //-----------------------------------------------------------------------------------------------

    /// Save the data - to be overriden
    virtual void save()const =0;
  
    /// Load the data - to be overriden
    virtual void load() = 0;

    /// Method to flush the data to disk and ensure it is written.
    virtual void flushData() const = 0;
    /// remove objects data from memory
    virtual void clearDataFromMemory() = 0;

    /**This method is for refactoring from here*/
    virtual bool isBox()const=0;

  /** @return the amount of memory that the object takes as a whole.
      For filebased objects it should be the amount the object occupies in memory plus the size it occupies in file if the object has not been fully loaded
      or modified.
     * If the object has never been loaded, this should be equal to number of data points in the file
     */
    virtual uint64_t getTotalDataSize() const=0;
    /// the data size kept in memory
    virtual size_t getDataMemorySize()const=0;


    /// @return true if it the data of the object is busy and so cannot be cleared; false if the data was released and can be cleared/written.
    bool isBusy() const
    {
      return m_Busy;
    }
    /// @ set the data busy to prevent from removing them from memory. The process which does that should clean the data when finished with them
    void setBusy(bool On=true)const
    {
     m_Busy=On;
    }
    /** Returns the state of the parameter, which tells disk buffer to force writing data 
     * to disk despite the size of the object have not changed (so one have probably done something with object contents. */
    bool isDataChanged()const{return m_dataChanged;}

    /** Call this method from the method which changes the object but keeps the object size the same to tell DiskBuffer to write it back
        the dataChanged ID is reset after save from the DataBuffer is emptied   */
    void setDataChanged()
    { 
      if(this->wasSaved())m_dataChanged=true;
    }
    /** this method has to be called if the object has been discarded from memory and is not changed any more. 
    It expected to be called from clearDataFromMemory. */
    void resetDataChanges()
    {
      m_dataChanged=false;
    }

    /** @return the position in the file where the data will be stored. This is used to optimize file writing. */
    virtual uint64_t getFilePosition() const
    {
      return m_fileIndexStart;
    }
    /**Return the number of units this block occipies on file */
    uint64_t getFileSize()const
    { 
      return   m_fileNumEvents;
    }

    /** Sets the location of the object on HDD */
    void setFilePosition(uint64_t newPos,uint64_t newSize,bool wasSaved=true);

    /** function returns true if the object have ever been saved on HDD and knows it place there*/
    bool wasSaved()const
    { // for speed it returns this boolean, but for relaibility this should be m_wasSaved&&(m_fileIndexStart!=max())
      return m_wasSaved;  
    }

  
  
    // ----------------------------- Helper Methods --------------------------------------------------------
    static void sortObjByFilePos(std::vector<ISaveable *> & boxes);
    // -----------------------------------------------------------------------------------------------------


  protected:
    /** Unique, sequential ID of the object/box within the containing workspace.
        This ID also relates to boxes order as the boxes with adjacent 
        ID should usually occupy adjacent places on HDD         */
    size_t m_id;
  private:
    /// Start point in the NXS file where the events are located
    uint64_t m_fileIndexStart;
    /// Number of events saved in the file, after the start index location
    uint64_t m_fileNumEvents;
    //-------------- 
    /// a user needs to set this variable to true preventing from deleting data from buffer
    mutable bool m_Busy;
    /** a user needs to set this variable to true to allow DiskBuffer saving the object to HDD 
        when it decides it suitable,  if the size of iSavable object in cache is unchanged from the previous 
        save/load operation */
    bool m_dataChanged;
    /// this 
    mutable bool m_wasSaved;

    /// the function saveAt has to be availible to DiskBuffer and nobody else. To highlight this we make it private
    friend class DiskBuffer;
   /** save at specific file location the specific amount of data; 
        used by DiskBuffer which asks this object where to save it and calling 
        overloaded object specific save operation above
     */
    void saveAt(uint64_t newPos, uint64_t newSize);

  };


} // namespace Kernel
} // namespace Mantid

#endif  /* MANTID_MDEVENTS_ISAVEABLE_H_ */
